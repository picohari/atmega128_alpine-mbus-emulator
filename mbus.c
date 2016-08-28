///////////////////////////////////////////////////////////////////
// Alpine M-Bus interface to RS232
// (c) 2003 Joerg Hohensohn, allowed for non-commercial use only
//                           (feedback appreciated)
//
// This translates the M-Bus to RS232 and back (listen and send)
// For more info see http://joerg.hohensohn.bei.t-online.de/mbus
///////////////////////////////////////////////////////////////////

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
#include "display.h"



mbus_rx_t rx_packet;
mbus_tx_t tx_packet;

char mbus_outbuffer[MBUS_BUFFER];

uint8_t mbus_tobesend;

char mbus_inbuffer[MBUS_BUFFER];

// received state
mbus_data_t in_packet;

// internal state
mbus_data_t status_packet; 	// maintained packet

command_t 	m_LastCmd; 	// last command from radio

uint8_t		echo_waitstate; 	// set if a response has been sent
char 		last_sent[100]; 	// our last sent packet, for echo check


enum
{
	quiet,			// do nothing
	get_state, 		// issue play state
	playing, 		// issue play state in regular intervals
	resuming, 		// starting up
	changing1,
	changing2,
	changing3,
	changing4,
} echostate; 		// what to do after own echo has been received


typedef struct
{	// one entry in the coding table
	command_t cmd;
	char hexmask[32];
	char infotext[32];
} code_item_t;

static const code_item_t alpine_codetable[] = 
{
	{ rPing, 		"18", 				"Ping" },
	{ cPingOK, 		"98", 				"Ping OK" },
	{ cAck, 		"9F0000f", 			"Ack/Wait" }, 		// f0=0|1|6|7|9
	{ rStatus, 		"19",				"some info?" },
	{ cPreparing,  	"991ttiimmssff0f", 	"Preparing" }, 		// f0=0:normal, f0=4:repeat one, f0=8:repeat all
	{ cStopped,    	"992ttiimmssff0f", 	"Stopped" }, 		// f1=0:normal, f1=2:mix, f1=8:scan
	{ cPaused,     	"993ttiimmssff0f", 	"Paused" }, 		// f3=1: play mode, f3=2:paused mode, f3=8: stopped
	{ cPlaying,    	"994ttiimmssff0f", 	"Playing" },
	{ cSpinup,     	"995ttiimmssff0f", 	"Spinup" },
	{ cForwarding, 	"996ttiimmssff0f", 	"FF" },
	{ cReversing,  	"997ttiimmssff0f", 	"FR" },
	{ rPlay,    	"11101", 			"Play" },
	{ rPause,   	"11102", 			"Pause" },
	{ rStop,    	"11140", 			"Stop" },
	{ rScnStop, 	"11150", 			"ScanStop" },
	{ rPlayFF,  	"11105", 			"Play FF start" },
	{ rPlayFR,  	"11109", 			"Play FR start" },
	{ rPauseFF, 	"11106", 			"Pause FF start" },
	{ rPauseFR, 	"1110A", 			"Pause FR start" },
	{ rResume,  	"11181", 			"Play fr curr. pos." },
	{ rResumeP, 	"11182", 			"Pause fr curr. pos." },
//	{ rNextMix, 	"1130A314", 		"next random" },
//	{ rPrevMix, 	"1130B314", 		"previous random" },
	{ rSelect,  	"113dttff", 		"Select" }, // f0=1:playing, f0=2:paused, f1=4:random
	{ rRepeatOff, 	"11400000", 		"Repeat Off" },
	{ rRepeatOne, 	"11440000", 		"Repeat One" },
	{ rRepeatAll, 	"11480000", 		"Repeat All" },
	{ rScan,      	"11408000", 		"Scan" },
	{ rMix,       	"11402000", 		"Mix" },
	{ cPwrUp, 		"9A0000000000", 	"some powerup?" },
	{ cLastInfo,  	"9B0dttfff0f", 		"last played" }, // f0=0:done, f0=1:busy, f0=8:eject, //f1=4: repeat1, f1=8:repeat all, f2=2:mix
	{ cChanging4, 	"9B8d00fff0f", 		"Changing Phase 4" },
	{ cChanging,  	"9B9dttfff0f", 		"Changing" }, 
	{ cNoMagzn,   	"9BAd00f00ff", 		"No Magazin" },
	{ cChanging2, 	"9BBd00fff0f", 		"Changing Phase 2" },
	{ cChanging3, 	"9BCd00fff0f", 		"Changing Phase 3" },
	{ cChanging1, 	"9BDd00fff0f", 		"Changing Phase 1" },
	{ cStatus, 		"9Cd01ttmmssf", 	"Disk Status" },
	{ cStat1, 		"9D000fffff", 		"some status?" },
	{ cStat2, 		"9E0000000", 		"some more status?" },
	// also seen:
	// 11191
};


// a convenience feature to populate the timings in eeprom with reasonable defaults
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


// utility function: convert a number to a hex char
char int2hex(uint8_t n)
{
	if (n < 10)
		return ('0' + n);
	else if (n < 16)
		return ('A' + n - 10);

	return 'X'; 		// out of range
}


// utility function: convert a hex char to a number
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


// generate the checksum
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


// helper function for main(): setup timers and pins
void init_mbus (void)
{

	// init virgin EEPROM with defaults, in timer ticks (34.722 us)
	init_eeprom();

	// timer settings: prescale 256 = 28,8kHz
	TCCR0  = (1 << CS00); 		// fast prescale that would immediately generate interrups
	TCNT0 = -1; 				// next interrupt will be pending immediately, but is masked

	//TCCR1B = _BV(ICNC1) | _BV(CTC1) | _BV(CS12); // noise filter, reset on match, prescale
	//TCCR1B = _BV(ICNC1) | _BV(CS12); // noise filter, reset on match, prescale
	TCCR1B =  (1 << ICES1) | (1 << ICNC1) | (1 << CS12) ; 	// capture on rising edge, noise filter
	
	OCR1H = 0; 					// we use only the lower part, but have to write this first
	OCR1L = BIT_TIMEOUT; 		// have to complete a bit within this time

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
}







// edge detection interrupt, the heart of receiving
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

// timeout interrupt, this terminates a received packet
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

		rx_packet.decode = True;

	}
#endif

	//PORT_DEBUG &= ~_BV(PIN_DEBUG); // debug, indicate loop

}



// timer 0 overflow, used for sending
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

		PORT_DEBUG &= ~_BV(PIN_DEBUG); // debug, indicate loop

		break;
	}
}



// use acRaw member to generate the others
uint8_t mbus_decode(mbus_data_t *mbuspacket, char *packet_src)
{
	size_t len = strlen(packet_src);
	size_t i, j;
	
	// reset all the decoded information
	mbuspacket->source = eUnknown;
	mbuspacket->chksum = -1;
	mbuspacket->chksumOK = False;
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
					uart_write((uint8_t *)"W", 1);

				uart_write((uint8_t *)LINE_FEED, strlen(LINE_FEED));
				memset(&mbus_inbuffer, '\0', sizeof(mbus_inbuffer));
			}
			
			break; // exit the command loop
		}
	}

	return (mbuspacket->cmd == eInvalid) ? 0xFF : 0;

}



// compose packet_dest
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

	tx_packet.send = True;

	return hr;
}








// returns S_OK if an answer is desired (then no timer should be spawned)
// or S_FALSE if not replying, in which case a timer may be spawned

// string version of the received packed, for echo test
// received packet in already decoded form
// destination for the answer packet string
// set to milliseconds units in wished to be called again

uint8_t mbus_process(const char *raw_received, const mbus_data_t *inpacket, char *buffer /*, uint16_t *sched_timer*/ ) 			
{
	uint8_t hr = 1; 		// default is do reply
	mbus_data_t response;

	memset(&response, 0, sizeof(response));

	/* Ok an dieser Stelle klappt die ACK/Wait Verbindung, alles andere noch nicht hier... */

#if 0
	// check for echo
	if (echo_waitstate && inpacket != NULL) {


		// I should have received my own packet string back. If not, there might be a collision?
		if (strcmp(last_sent, raw_received) == 0) {
			// OK, we might send the next if desired
			echo_waitstate = False;

			uart_write((uint8_t *)"H", 1);
			// evaluate state and send more or maybe spawn timer
			switch (echostate) {

			case get_state: // this state exists to send the first timestamp right away
				echostate = (status_packet.cmd == cPlaying) ? playing : quiet;
				response = status_packet;
				break;

			case playing: // regular timestamps
				//*sched_timer = 1000;
				hr= 0;
				break;

			case resuming:
				response.cmd = cStatus;
				response.disk = status_packet.disk;
				response.track = INT2BCD(99);
				response.minutes = INT2BCD(99);
				response.seconds = INT2BCD(99);
				response.flags = 0xF;
				echostate = get_state;
				break;

			case changing1: // debug short cut
				response.cmd = cChanging;
				response.disk = status_packet.disk;
				response.track = status_packet.track;
				response.flags = 0x0001; // done
				echostate = get_state;
				break;

			default:
				hr = 0; // send nothing

			}

			if (hr == 1) {
				//*sched_timer = 0; // discipline myself: no timer if responding
				mbus_encode(&response, buffer);
				strcpy(last_sent, buffer); // remember for echo check
				echo_waitstate = True;
			}
			return hr;
		
		} else {	// what to do on collision? try sending again
			strcpy(buffer, last_sent); // copy old to output
			//return 1; // send again
		}

	}
#endif 


#if 0	
	if (inpacket == NULL ) {	// response from timer, not by reception (could be a seperate function in future)

		//uart_write((uint8_t *)"N", 1);

		switch (echostate) {

		case playing:
			if (status_packet.cmd == cPlaying) {
				status_packet.track = INT2BCD(BCD2INT(status_packet.track) + 1); // count
				if (status_packet.track > 0x99)
					status_packet.track = 0;
			}
			else 
				hr = 0;
			break;

		default:
			hr = 0; // send nothing

		}

		if (hr == 1) {
			//*sched_timer = 0; // discipline myself: no timer if responding
			mbus_encode(&status_packet, buffer);
			strcpy(last_sent, buffer); // remember for echo check
			echo_waitstate = True;
		}
		
		return hr;
	}
#endif

#if 1

	// sanity checks and debug output
	if (inpacket != NULL) {

		if (!inpacket->chksumOK) {
			//uart_write((uint8_t *)"c", 1);	// checksum error
			return 0;
		}
	

		if (inpacket->source != eRadio) {
			//uart_write((uint8_t *)"s", 1);	// destination error
			return 0; // ignore for now
		}

	}

#endif

	
	// generate immediate response to radio command, want to see no timer spawn here


	//if (inpacket->cmd != m_LastCmd) {	// we have received a new command
	//	m_LastCmd = inpacket->cmd;
	//}

	switch(inpacket->cmd) {

	case rPing:
		response.cmd = cPingOK;
		break;

	case rStatus:
		response.cmd = cAck;

		break;

	case rResume:
		response.cmd = cAck;
		break;

#if 1
	case rPlay:
		status_packet.cmd = cPlaying;
		status_packet.flags &= ~0x00B;
		status_packet.flags |=  0x001;
		response = status_packet;
		break;

	case rPause:
		status_packet.cmd = cPaused;
		status_packet.flags &= ~0x00B;
		status_packet.flags |=  0x002;
		response = status_packet;
		break;

	case rScnStop:
	case rStop:
		status_packet.cmd = cStopped;
		status_packet.flags |=  0x008;
		response = status_packet;
		break;

	case rPlayFF:
		status_packet.cmd = cForwarding;
		response = status_packet;
		break;

	case rPlayFR:
		status_packet.cmd = cReversing;
		response = status_packet;
		break;

	case rRepeatOff:
		status_packet.flags &= ~0xCA0;
		response = status_packet;
		break;

	case rRepeatOne:
		status_packet.flags &= ~0xCA0;
		status_packet.flags |=  0x400;
		response = status_packet;
		break;

	case rRepeatAll:
		status_packet.flags &= ~0xCA0;
		status_packet.flags |=  0x800;
		response = status_packet;
		break;

	case rScan:
		status_packet.flags &= ~0xCA0;
		status_packet.flags |=  0x080;
		response = status_packet;
		break;

	case rMix:
		status_packet.flags &= ~0xCA0;
		status_packet.flags |=  0x020;
		response = status_packet;
		break;

	case rSelect:
		response.cmd = cChanging;
		if (inpacket->disk != 0 && inpacket->disk != status_packet.disk) {
			echostate = changing1;
			response.disk = status_packet.disk = inpacket->disk;
			//status_packet.disk = 9; // test hack!!
			response.track = status_packet.track = 0;
			response.flags = 0x1001; // busy
		} else {
			// zero indicates same disk
			echostate = get_state;
			status_packet.minutes = 0;
			status_packet.seconds = 0;
			response.disk = status_packet.disk;
			response.track = status_packet.track = inpacket->track;
			response.flags = 0x0001; // done
		}
		break;




	case rResumeP:
		echostate = resuming;
		status_packet.cmd = (inpacket->cmd == rResume) ? cPlaying : cPaused;
		response.cmd = rStop;
		response.disk = status_packet.disk;
		response.track = status_packet.track;
		response.flags = 0x0001; // done
		break;
	


#endif

	default:
		//response.cmd = cAck;
		response.cmd = eInvalid;
		//response.cmd = cPingOK;

	} // switch(inpacket->cmd)
	
	
	if (response.cmd != eInvalid) {
		//*sched_timer = 0; // discipline myself: no timer if responding
		mbus_encode(&response, buffer);
		strcpy(last_sent, buffer); // remember for echo check
		echo_waitstate = True;

		return 1;
	}


	return 0;
}
