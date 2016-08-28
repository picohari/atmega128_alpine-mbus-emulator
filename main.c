/*
 * USB Märklin Kran
 *
 * Copyright (C) 2016  H9-Laboratory Ltd.
 * All rights reserved
 *
 * CREATED			:	16.06.2016
 * VERSION			: 	1.0
 * TARGET DEVICE	: 	ATMEL ATmega128
 *
 * COMMENT			:	This is the main control application
 *
 *
 */


 /*!
 * @file 	main.c
 * @brief 	USB Märklin Kran Main Control Application
 *
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	16.06.2016
 */

#include "config.h"
#include "global.h"
 
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

#include "timer.h"

#include "fifo.h"
#include "uart.h"
#include "log.h"
#include "display.h"

#include "mbus.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>




static void init (void) {

	PORTA = 0; DDRA = 0;	// All Inputs -> all zeros
	PORTB = 0; DDRB = 0;
	PORTC = 0; DDRC = 0;
	PORTD = 0; DDRD = 0;
	PORTE = 0; DDRE = 0;
	PORTF = 0; DDRF = 0;
	PORTG = 0; DDRG = 0;

	//wdt_enable(WDTO_1S);
	wdt_disable();		  // Watchdog off!
	timer_2_init();		      // Activate timer 2 for interrupt

	/* Is this a power on reset? */
	if ((MCUCSR & 1) == 1) {
		MCUCSR &= ~1;	// delete bit

		_delay_ms(100);
		asm volatile("jmp 0");
	}

	_delay_ms(100);

	#ifdef UART_AVAILABLE
		uart_init();
	#endif

	#ifdef DISPLAY_AVAILABLE
		display_init();
		display_clear();
		display_cursor(0, 0);
		//display_printf("Hello world!");
	#endif

    LOG_INFO("M-BUS Adapter 1.0\n");
}






int main (void) {

	/* Initiate all */
	init();

    init_mbus();

    // setup the states, only the necessary
    rx_packet.state = wait;
    tx_packet.num_bits = 0;

    sei();


    
    for (;;) {


        //uint8_t new_uart = uart_searchbuffer('\r');

        if (rx_packet.decode) {

            mbus_decode(&packet, mbus_inbuffer);

            mbus_process(mbus_inbuffer, &packet, mbus_outbuffer);
        
            rx_packet.decode = False;

            display_clear();

            display_printf("%s", packet.description);
        }
        





#if 0
        /* Decode received packet */
        if (rx_packet.decode){

            mbus_decode(&packet, mbus_inbuffer);

            memset(&mbus_outbuffer, '\0', sizeof(mbus_outbuffer));

            mbus_process(mbus_inbuffer, &packet, mbus_outbuffer);

            memset(&mbus_inbuffer, '\0', sizeof(mbus_inbuffer));

            //rx_packet.decode = False;
            tx_packet.send = True;
            //memset(&mbus_inbuffer, 0, sizeof(mbus_inbuffer));
        }
#endif



        /* check if there is a command to be sent */
        if (!(TIMSK & _BV(TOIE0))                       // not already sending
            && rx_packet.state == wait                  // not receiving
            && ( /*new_uart ||*/ tx_packet.send )           // newline in receive buffers
            ) {


            // start sending the transmission
            tx_packet.state = start;
            TCCR0  = ((1 << CS01) | (1 << CS02));   // slow prescaling while sending
            TCNT0 = 0;                              // reset timer because ISR only offsets to it
            TIMSK |= _BV(TOIE0);                    // start the output handler with timer0

            tx_packet.send = False;
        }







#if 0

        char szResponseBuf[64] = {'\0'};
        m_dwRecallTime = 0;
        hr = m_ChangerEmu.ProcessCommand(pszLine, &Packet, szResponseBuf, &m_dwRecallTime);
        if (hr == S_OK)
        {   // wants to send a response
            strcat(szResponseBuf, "\r\n");
            m_serial.sendData(szResponseBuf, strlen(szResponseBuf));

        }
        if (m_dwRecallTime)
        {   // wants to be notified again
            SetTimer(IDD, m_dwRecallTime, NULL);
        }
#endif


	} /* End for (;;) */


	/* If we ever should get here ;-) */
	return 1;

}