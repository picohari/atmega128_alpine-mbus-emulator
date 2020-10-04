/****************************************************************************
 * Copyright (C) 2016 by Harald W. Leschner (DK6YF)                         *
 *                                                                          *
 * This file is part of ALPINE M-BUS Interface Control Emulator             *
 *                                                                          *
 * This program is free software you can redistribute it and/or modify		*
 * it under the terms of the GNU General Public License as published by 	*
 * the Free Software Foundation either version 2 of the License, or 		*
 * (at your option) any later version. 										*
 *  																		*
 * This program is distributed in the hope that it will be useful, 			*
 * but WITHOUT ANY WARRANTY without even the implied warranty of 			*
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 			*
 * GNU General Public License for more details. 							*
 *  																		*
 * You should have received a copy of the GNU General Public License 		*
 * along with this program if not, write to the Free Software 				*
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA*
 ****************************************************************************/

/**
 * @file mbus_proto.c
 *
 * @author Harald W. Leschner (DK6YF)
 * @date 23.08.2016
 *
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 *
 * @see http://www.stack.nl/~dimitri/doxygen/docblocks.html
 * @see http://www.stack.nl/~dimitri/doxygen/commands.html
 */

#include <inttypes.h>      	// common scalar types
#include <avr/io.h>        	// for device register definitions
#include <avr/interrupt.h> 	// for interrupt enable
#include <avr/sleep.h>     	// for power-save idle sleep
#include <avr/wdt.h>       	// for watchdog, used to prevent deadlocks in interrupts
#include <avr/eeprom.h>    	// EEPROM access
#include <string.h>    		// EEPROM access

#include "mbus.h"         	// look for definitions here
#include "uart.h"         	// my UART "driver"

#include "log.h"
#include "hd44780.h"


/* Global variables */
mbus_rx_t rx_packet;		// global accessible received packet
mbus_tx_t tx_packet;		// global accessible outgoing packet

char mbus_outbuffer[MBUS_BUFFER];	// global codec buffer for the driver 
char mbus_inbuffer[MBUS_BUFFER];	// stores incoming message

uint8_t mbus_tobesend = 0;			// current index of buffer (debugging?)




/* A convenience feature to populate the timings in eeprom with reasonable defaults */
void init_eeprom (void)
{
	uint8_t i;
	static const uint8_t ee_table[] =
	{	// default initialization values, positions must match the EE_xx_TIME order
		(uint8_t)((F_CPU / (16L * BAUDRATE)) - 1), 	// default baudrate as register value
		DEFAULT_ZERO_TIME - DEFAULT_TOLERANCE, 		// EE_MIN_ZERO_TIME
		DEFAULT_ZERO_TIME + DEFAULT_TOLERANCE, 		// EE_MAX_ZERO_TIME
		DEFAULT_ONE_TIME - DEFAULT_TOLERANCE, 		// EE_MIN_ONE_TIME
		DEFAULT_ONE_TIME + DEFAULT_TOLERANCE, 		// EE_MAX_ONE_TIME
		DEFAULT_BIT_TIME + DEFAULT_MIN_PAUSE, 		// EE_BIT_TIMEOUT
		DEFAULT_ZERO_TIME, 		// EE_SEND_ZERO_TIME
		DEFAULT_ONE_TIME, 		// EE_SEND_ONE_TIME
		DEFAULT_BIT_TIME, 		// EE_SEND_BIT_TIME
		DEFAULT_SPACE, 			// EE_SEND_SPACE
	};

	for (i = 0; i < sizeof(ee_table) / sizeof(*ee_table); i++) {
		if (eeprom_read_byte((uint8_t *)(uint16_t)i) == 0xFF) { 		// virgin?
			eeprom_write_byte((uint8_t *)(uint16_t)i, ee_table[i]);
			wdt_reset(); // bear in mind that writing can take up to 5ms, beware of the watchdog
		}
	}
}


/* Utility function: convert a number to a hex char */
char int2hex(uint8_t n)
{
	if (n < 10)
		return ('0' + n);
	else if (n < 16)
		return ('A' + n - 10);

	return 'X'; 		// out of range
}


/* Utility function: convert a hex char to a number */
uint8_t hex2int(char c)
{
	if (c < '0')
		return 0xFF; 	// illegal hex digit
	else if (c <= '9')
		return (c - '0');
	else if (c < 'A')
		return 0xFF; 	// illegal hex digit
	else if (c <= 'F')
		return (c - 'A' + 10);
	else if (c < 'a')
		return 0xFF; 	// illegal hex digit
	else if (c <= 'f')
		return (c - 'a' + 10);

	return 0xFF; 		// default
}


/* Generate the checksum */
int8_t calc_checksum(char *buffer, uint8_t len)
{
	int8_t checksum = 0;
	int8_t i;

	for (i = 0; i < len; i++) {
		checksum ^= hex2int(buffer[i]);
	}
	checksum = (checksum + 1) % 16;

	return checksum;
}


/* Find key in buffer */
uint8_t mbus_searchbuffer(uint8_t key)
{
	uint8_t i;
	//uint8_t level = mbus_infifo.count;
	uint8_t level = sizeof(mbus_inbuffer);

	for (i = 0; i <= level; i++) {
		if (mbus_inbuffer[i] == key)
			return i; // found
	}
	return 0; // not found
}




void mbus_receive_wait(void)
{
	while (true) {
	    /* check if there is a command to be sent */
	    if (!(TIMSK & _BV(TOIE0))                       // not already sending
	        && rx_packet.state == wait                  // not receiving
	        && ( /*new_uart ||*/ tx_packet.send )       // have something to send
	        ) {

	        // start sending the transmission
	        tx_packet.state = start;
	        TCCR0 = ((1 << CS01) | (1 << CS02));   // slow prescaling while sending
	        TCNT0 = 0;                              // reset timer because ISR only offsets to it
	        TIMSK |= _BV(TOIE0);                    // start the output handler with timer0

	        tx_packet.send = false;
	    }

	}
} 


void mbus_receive(void)
{
    /* check if there is a command to be decoded */
    if (rx_packet.decode) {

        mbus_decode(&in_packet, mbus_inbuffer);
        //mbus_process(&in_packet, mbus_outbuffer, false);

        mbus_control(&in_packet);

        rx_packet.decode = false;
    }

}


void mbus_send(void)
{
    /* check if there is a command to be sent */
    if (!(TIMSK & _BV(TOIE0))                   // not already sending
        && (rx_packet.state == wait)            // not receiving
        && ( /*new_uart ||*/ tx_packet.send)    // have something to send
        ) {

        // start sending the transmission
        tx_packet.state = start;
        TCCR0 = ((1 << CS01) | (1 << CS02));    // slow prescaling while sending
        TCNT0 = 0;                              // reset timer because ISR only offsets to it
        TIMSK |= _BV(TOIE0);                    // start the output handler with timer0

        tx_packet.send = false;
    }
} 






/*
 * Initialisation : Setup hardware timers, pins and buffers
 */
void mbus_init(void)
{

	// init virgin EEPROM with defaults, in timer ticks (34.722 us)
	init_eeprom();

	// timer settings: prescale 256 = 28,8kHz
	TCCR0  = (1 << CS00); 		// fast prescale that would immediately generate interrups
	TCNT0 = -1; 				// next interrupt will be pending immediately, but is masked

	//TCCR1B = _BV(ICNC1) | _BV(CTC1) | _BV(CS12); // noise filter, reset on match, prescale
	//TCCR1B = _BV(ICNC1) | _BV(CS12); // noise filter, reset on match, prescale
	TCCR1B =  (1 << ICES1) | (1 << ICNC1) | (1 << CS12) ; 	// capture on rising edge, noise filter

	OCR1AH = 0; 				// we use only the lower part, but have to write this first
	OCR1AL = BIT_TIMEOUT; 		// have to complete a bit within this time

    // enable capture and compare match interrupt for timer 1
	TIMSK |= (1 << TICIE1) | (1 << OCIE1A);

	// set my outputs, all but PB2 are for debugging / error signaling
	//DDRB =  _BV(PIN_MBUS_OUT | _BV(PIN_TX_OF) | _BV(PIN_RX_OF) | _BV(PIN_RX_UF) | _BV(PIN_DEBUG)); // output and debug pins
	DDR_MBUS_OUT |= (1 << PIN_MBUS_OUT);

	DDR_DEBUG  |=  (1 << PIN_DEBUG);
	PORT_DEBUG &= ~(1 << PIN_DEBUG);
	//TCCR1A = _BV(COM1A0); // test: toggle OC1 at compare match

	/* FIFOs für Ein- und Ausgabe initialisieren */
  	memset(&mbus_inbuffer, '\0', sizeof(mbus_inbuffer));

	rx_packet.num_nibbles = 0;

  	/* Changer simulator setup */
  	echostate = quiet;

	status_packet.source = eCD;
	status_packet.chksum = -1;
	status_packet.chksumOK = false;
	status_packet.cmd = eInvalid;
	status_packet.description = "Init";
	status_packet.flagdigits = 0;
	status_packet.validcontent = 0;
	status_packet.disk = 1;
	status_packet.track = 1;
	status_packet.index = 1;
	status_packet.minutes = 0;
	status_packet.seconds = 0;
	status_packet.flags = 0;

	in_packet.cmd = eInvalid;
	in_packet.description = "Idle";
}





/*
d8888b. d88888b  .o88b. d88888b d888888b db    db d88888b d8888b. 
88  `8D 88'     d8P  Y8 88'       `88'   88    88 88'     88  `8D 
88oobY' 88ooooo 8P      88ooooo    88    Y8    8P 88ooooo 88oobY' 
88`8b   88~~~~~ 8b      88~~~~~    88    `8b  d8' 88~~~~~ 88`8b   
88 `88. 88.     Y8b  d8 88.       .88.    `8bd8'  88.     88 `88. 
88   YD Y88888P  `Y88P' Y88888P Y888888P    YP    Y88888P 88   YD 
*/

/*
 * TIMER 1 Capture interrupt : Heart of receiving, measure the pulse-width / length of pulses
 *
 * - 16bit timer
 * - Edge detection is done in hardware.
 */
ISR(TIMER1_CAPT_vect)
{
	char outchar = 0;

	//PORT_DEBUG |= _BV(PIN_DEBUG); 	// debug, indicate loop

	switch (rx_packet.state) {

	case wait: 						// a packet is starting
		rx_packet.num_bits = 0;
		//TIMSK |= (1 << OCIE1A);		// Enable overflow/compare
		// no break, fall through

	case low: // high phase between bits has ended, start of low pulse
		// could check the remain high time to verify bit, but won't work for the last (timed out)
		TCNT1H = 0; // reset the timer, high byte first
		TCNT1L = 0;
		TIFR |= (1 << OCF1A); 		// clear timeout pending, to be shure

		rx_packet.state = high;
		TCCR1B &= ~(1 << ICES1); 	// capture on falling edge
		break;

	case high: // end of the pulldown phase of a bit
		rx_packet.state = low;
		TCCR1B |= (1 << ICES1); 	// capture on rising edge

		// check the low time to determine bit value
		if (ICR1L < MIN_ZERO_TIME)
			outchar = '<';
		else if (ICR1L <= MAX_ZERO_TIME)
			outchar = '0';
		else if (ICR1L < MIN_ONE_TIME)
			outchar = '=';
		else if (ICR1L <= MAX_ONE_TIME)
			outchar = '1';
		else
			outchar = '>';

#if 0
		// test, write length for bad bits
		if (outchar != '0' && outchar != '1')
		{
			UartTransmitByte('(');
			UartTransmitByte(int2hex(ICR1L >> 4));
			UartTransmitByte(int2hex(ICR1L & 0x0F));
			UartTransmitByte(')');
		}
		//UartTransmitByte(outchar); // sending here gives binary
#endif

		// hex conversion & output, for convenience
		rx_packet.rxbits[rx_packet.num_bits % 4] = outchar;
		rx_packet.num_bits++;

		if ((rx_packet.num_bits % 4) == 0) { 	// 4 bits completed?

			uint8_t i;
			uint8_t uHexDigit = 0;

			for (i = 0; i < 4; i++) {			// convert to binary
				if (rx_packet.rxbits[i] == '1')
					uHexDigit |= (1 << (3 - i));
				else if (rx_packet.rxbits[i] != '0')
					uHexDigit = 0xFF; // mark error
			}

			/* HEX-DIGIT 0..9-A..F and send it out */
			uint8_t value = int2hex(uHexDigit);

			/* Send via UART */
			uint8_t *res_ptr = &value;
			uart_write(res_ptr, 1);

			/* Store received data into DECODER buffer */
			mbus_inbuffer[rx_packet.num_nibbles] = value;
			rx_packet.num_nibbles++;

		}

		break;
	}
}


/*
 * TIMER 1 Compare interrupt : Called in regular interval, this routine checks the completition of a received message, kind of timeout ...
 */
ISR(TIMER1_COMPA_vect)
{

	if (rx_packet.state == wait)
		return; // timeouts don't matter

	//PORT_DEBUG |= _BV(PIN_DEBUG); 	// debug, indicate loop

	rx_packet.state = wait; 	// start looking for a new packet
	//TCCR1B &= ~_BV(ICES1); 	// capture on falling edge
	TCCR1B |= (1 << ICES1); 	// capture on rising edge


	// else the packet is completed
	if ((rx_packet.num_bits % 4) != 0) 			// there should be no data waiting for output
		uart_write((uint8_t *)('X'), 1); 		// but if, then mark it

#if 1
	if (rx_packet.num_nibbles > 2 && rx_packet.num_bits % 4 == 0) {

		mbus_inbuffer[rx_packet.num_nibbles] = '\r';		// insert final newline char

		//uart_write((uint8_t *)LINE_FEED, strlen(LINE_FEED));
		uart_write((uint8_t *)"|", 1);

		rx_packet.num_nibbles = 0;

		rx_packet.decode = true;

	}
#endif

	//PORT_DEBUG &= ~_BV(PIN_DEBUG); // debug, indicate loop

}



/*
d888888b d8888b.  .d8b.  d8b   db .d8888. .88b  d88. d888888b d888888b d888888b d88888b d8888b. 
`~~88~~' 88  `8D d8' `8b 888o  88 88'  YP 88'YbdP`88   `88'   `~~88~~' `~~88~~' 88'     88  `8D 
   88    88oobY' 88ooo88 88V8o 88 `8bo.   88  88  88    88       88       88    88ooooo 88oobY' 
   88    88`8b   88~~~88 88 V8o88   `Y8b. 88  88  88    88       88       88    88~~~~~ 88`8b   
   88    88 `88. 88   88 88  V888 db   8D 88  88  88   .88.      88       88    88.     88 `88. 
   YP    88   YD YP   YP VP   V8P `8888Y' YP  YP  YP Y888888P    YP       YP    Y88888P 88   YD 
*/

/*
 * TIMER 0 Overflow interrupt : Generates the pulse-width modulated signal on PIN_MBUS_OUT
 */
ISR(TIMER0_OVF_vect)
{

	switch (tx_packet.state)
	{
	case start: // start of a bit, or maybe the packet

		PORT_DEBUG |= _BV(PIN_DEBUG); 	// debug, indicate loop

		if (tx_packet.num_bits % 4 == 0) {		// need a new hex nibble

			uint8_t fetched = 0;

			do {	// end with '\r', skip all other control codes (e.g. '\n') and non-hex chars

				/*
				An dieser Stelle müssen die Bytes aus dem Array mbus_outbuffer[]
				EINZELN geholt werden, damit sie gesendet werden können.
				*/

				/* ENCODER */
				fetched = mbus_outbuffer[mbus_tobesend++];	// Version for EMULATOR input

				/* UART */
				//fetched = fifo_get_nowait(&infifo);		// Version for UART input

				/* SEND also via UART */
				//uint8_t *res_ptr = &fetched;
				//uart_write(res_ptr, 1);

				tx_packet.cur_nibble = hex2int(fetched);

			} while (fetched != '\r' && tx_packet.cur_nibble == 0xFF);

			//PORT_DEBUG &= ~_BV(PIN_DEBUG); // debug end

			if (fetched == '\r') {		// done with this line (packet)
				TCNT0 -= SEND_SPACE; 	// space til the next transmision can start
				tx_packet.state = ende;
				break; // exit
			}

		}
		PORT_MBUS_OUT |= _BV(PIN_MBUS_OUT); // pull the M-BUS line low (active high bcse of transistor pulling down)

		if (tx_packet.cur_nibble & (1 << (3 - (tx_packet.num_bits % 4)))) {
		 	// 1
			tx_packet.state = low_1;
			TCNT0 -= SEND_ONE_TIME; 	// next edge for the long pulse
		} else {
			// 0
			tx_packet.state = low_0;
			TCNT0 -= SEND_ZERO_TIME; 	// next edge for the short pulse
		}
		tx_packet.num_bits++; 	// next bit
		break;

	case low_0:
		PORT_MBUS_OUT &= ~_BV(PIN_MBUS_OUT); 		// release the line
		tx_packet.state = start;
		TCNT0 -= (SEND_BIT_TIME - SEND_ZERO_TIME); 	// next edge
		break;

	case low_1:
		PORT_MBUS_OUT &= ~_BV(PIN_MBUS_OUT); 		// release the line
		tx_packet.state = start;
		TCNT0 -= (SEND_BIT_TIME - SEND_ONE_TIME); 	// next edge
		break;

	case ende:
		TIMSK &= ~_BV(TOIE0); 		// stop timer0 interrupts
		TCCR0  = (1 << CS00); 		// set to "immediate interrupt" mode again
		TCNT0 = -1; 				// next interrupt will be pending immediately, but is masked
		tx_packet.state = start;
		tx_packet.num_bits = 0; 	// reset the bit counter again

		mbus_tobesend =  0;
		//memset(&mbus_outbuffer, '\0', sizeof(mbus_outbuffer));	// delete content of inbuffer and start over
		PORT_DEBUG &= ~_BV(PIN_DEBUG); // debug, indicate loop

		break;
	}
}



/*
d8888b. d88888b  .o88b.  .d88b.  d8888b. d88888b d8888b. 
88  `8D 88'     d8P  Y8 .8P  Y8. 88  `8D 88'     88  `8D 
88   88 88ooooo 8P      88    88 88   88 88ooooo 88oobY' 
88   88 88~~~~~ 8b      88    88 88   88 88~~~~~ 88`8b   
88  .8D 88.     Y8b  d8 `8b  d8' 88  .8D 88.     88 `88. 
Y8888D' Y88888P  `Y88P'  `Y88P'  Y8888D' Y88888P 88   YD 
*/

/*
 * Decode incoming packet: Analyze the received message for known commands and parse the data
 *
 * packet_src is filled by receiving interrupt routine (see mbus_inbuffer[] and ISR(TIMER1_CAPT_vect) )
 * mbuspacket contains the resulting decoded information
 */
uint8_t mbus_decode(mbus_data_t *mbuspacket, char *packet_src)
{
	size_t len = strlen(packet_src);
	size_t i, j;

	// reset all the decoded information
	mbuspacket->source = eUnknown;
	mbuspacket->chksum = -1;
	mbuspacket->chksumOK = false;
	mbuspacket->cmd = eInvalid;
	mbuspacket->description = "";
	mbuspacket->flagdigits = 0;
	mbuspacket->validcontent = 0;
	mbuspacket->disk = 0;
	mbuspacket->track = 0;
	mbuspacket->index = 0;
	mbuspacket->minutes = 0;
	mbuspacket->seconds = 0;
	mbuspacket->flags = 0;


	if (len < 3)
		return 0xFF;

	len--;	// remove last '\r'
	len--;	// remove checksum
	mbuspacket->source = (source_t)(hex2int(packet_src[0])); // determine source from first digit
	mbuspacket->chksum = calc_checksum(packet_src, len);
	mbuspacket->chksumOK = (mbuspacket->chksum == hex2int(packet_src[len])); // verify checksum

	if (!mbuspacket->chksumOK) {
		uart_write((uint8_t *)"?", 1);
		memset(&mbus_inbuffer, '\0', sizeof(mbus_inbuffer));	// delete content of inbuffer and start over
		return 0xFF;
	}

	for (i = 0; i < sizeof(alpine_codetable) / sizeof(*alpine_codetable); i++) {
		// try all commands
		if (len != strlen(alpine_codetable[i].hexmask))
			continue; // size mismatch

		const char *src_ptr = packet_src; 						// source read pointer
		const char *cmp_ptr = alpine_codetable[i].hexmask; 		// current compare

		for (j = 0; j < len; j++) {
			// all (upper case) hex digits of the hexmask must match
			if ((*cmp_ptr >= '0' &&  *cmp_ptr <= '9') || (*cmp_ptr >= 'A' &&  *cmp_ptr <= 'F')) {

				if (*cmp_ptr != *src_ptr)
					break; // exit the char loop
			}
			src_ptr++;
			cmp_ptr++;
		}

		if (j == len) {
			// a match, now decode parameters if present
			for (j = 0; j < len; j++) {

				switch (alpine_codetable[i].hexmask[j]) {

				case 'd': // disk
					mbuspacket->disk = (mbuspacket->disk << 4) | hex2int(packet_src[j]);
					mbuspacket->validcontent |= F_DISK;
					break;
				case 't': // track
					mbuspacket->track = (mbuspacket->track << 4) | hex2int(packet_src[j]);
					mbuspacket->validcontent |= F_TRACK;
					break;
				case 'i': // index
					mbuspacket->index = (mbuspacket->index << 4) | hex2int(packet_src[j]);
					mbuspacket->validcontent |= F_INDEX;
					break;
				case 'm': // minute
					mbuspacket->minutes = (mbuspacket->minutes << 4) | hex2int(packet_src[j]);
					mbuspacket->validcontent |= F_MINUTE;
					break;
				case 's': // second
					mbuspacket->seconds = (mbuspacket->seconds << 4) | hex2int(packet_src[j]);
					mbuspacket->validcontent |= F_SECOND;
					break;
				case 'f': // flags
					mbuspacket->flags = (mbuspacket->flags << 4) | hex2int(packet_src[j]);
					mbuspacket->validcontent |= F_FLAGS;
					mbuspacket->flagdigits++;
					break;
				} // switch

			} // for j

			mbuspacket->cmd = alpine_codetable[i].cmd;
			mbuspacket->description = alpine_codetable[i].infotext;

			if (mbuspacket->chksumOK) {

				if (mbuspacket->source == eRadio)
					uart_write((uint8_t *)"R", 1);
				else if (mbuspacket->source == eCD)
					uart_write((uint8_t *)"C", 1);

				uart_write((uint8_t *)"| ", 2);
				uart_write((uint8_t *)mbuspacket->description, strlen(mbuspacket->description));


				uart_write((uint8_t *)LINE_FEED, strlen(LINE_FEED));
				memset(&mbus_inbuffer, '\0', sizeof(mbus_inbuffer));
			}

			break; // exit the command loop
		}
	}

	return (mbuspacket->cmd == eInvalid) ? 0xFF : 0;

}



/*
d88888b d8b   db  .o88b.  .d88b.  d8888b. d88888b d8888b. 
88'     888o  88 d8P  Y8 .8P  Y8. 88  `8D 88'     88  `8D 
88ooooo 88V8o 88 8P      88    88 88   88 88ooooo 88oobY' 
88~~~~~ 88 V8o88 8b      88    88 88   88 88~~~~~ 88`8b   
88.     88  V888 Y8b  d8 `8b  d8' 88  .8D 88.     88 `88. 
Y88888P VP   V8P  `Y88P'  `Y88P'  Y8888D' Y88888P 88   YD 
*/

/*
 * Compose outgoing packet: Depending on the command to be sent, all status information is inserted into the message using the template
 *
 * mbuspacket contains the status information to be sent
 * packet_dest is the buffer to be filled with the encoded message
 */
uint8_t mbus_encode(mbus_data_t *mbuspacket, char *packet_dest)
{
	uint8_t hr = 0;
	int8_t i,j;
	size_t len;
	mbus_data_t packet = *mbuspacket; // a copy which I can modify

	// seach the code table entry
	for (i = 0; i < sizeof(alpine_codetable) / sizeof(*alpine_codetable); i++) {
	 	// try all commands
		if (packet.cmd == alpine_codetable[i].cmd)
			break;
	}

	if (i == sizeof(alpine_codetable) / sizeof(*alpine_codetable)) {
		packet_dest = '\0'; 	// return an empty string
		return 0xFF; 			// not found
	}

	const char *pkt_template = alpine_codetable[i].hexmask;
	char *pkt_writeout = packet_dest;
	len = strlen(pkt_template);

	for (j = len - 1; j >= 0; j--) { // reverse order works better for multi-digit numbers

		if ((pkt_template[j] >= '0' &&  pkt_template[j] <= '9') || (pkt_template[j] >= 'A' &&  pkt_template[j] <= 'F')) {
			// copy regular hex digits
			pkt_writeout[j] = pkt_template[j];
			continue;
		}

		switch (pkt_template[j]) {
		 	// I just assume that any necessary parameter data is present
		case 'd': // disk
			pkt_writeout[j] = int2hex(packet.disk & 0x0F);
			packet.disk >>= 4;
			break;
		case 't': // track
			pkt_writeout[j] = int2hex(packet.track & 0x0F);
			packet.track >>= 4;
			break;
		case 'i': // index
			pkt_writeout[j] = int2hex(packet.index & 0x0F);
			packet.index >>= 4;
			break;
		case 'm': // minute
			pkt_writeout[j] = int2hex(packet.minutes & 0x0F);
			packet.minutes >>= 4;
			break;
		case 's': // second
			pkt_writeout[j] = int2hex(packet.seconds & 0x0F);
			packet.seconds >>= 4;
			break;
		case 'f': // flags
			pkt_writeout[j] = int2hex(packet.flags & 0x0F);
			packet.flags >>= 4;
			break;
		default: // unknow format char
			pkt_writeout[j] = '?';
			hr = 0x7F; // not quite OK
		}
	}


	int8_t checksum = calc_checksum(pkt_writeout, len);
	pkt_writeout[len] = int2hex(checksum); 	// add checksum
	pkt_writeout[len+1] = '\r'; 			// string termination
	pkt_writeout[len+2] = '\0'; 			// string termination

	tx_packet.send = true;

	return hr;
}

