/****************************************************************************
 * Copyright (C) 2016 by Harald W. Leschner (DK6YF)                         *
 *                                                                          *
 * This file is part of ALPINE M-BUS Interface Control Emulator             *
 *                                                                          *
 * This program is free software you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation either version 2 of the License, or         *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program if not, write to the Free Software               *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA*
 ****************************************************************************/

/**
 * @file main.c
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
	wdt_disable();		      // Watchdog off!
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

        /* Perform some player timings and simulations */
        static uint32_t player_ticks = 0;
        static uint32_t sending_ticks = 0;

        if (timer_ms_passed(&player_ticks, 1000)) {

            if (status_packet.cmd == cPlaying)
                player_sec++;

            if (player_sec == 5400)
                player_sec = 0;     
        }

        status_packet.minutes = player_sec / 60;
        status_packet.seconds = player_sec % 60;

        if (timer_ms_passed(&sending_ticks, 500)) {

            if (status_packet.cmd == cPlaying )
                mbus_process(&in_packet, mbus_outbuffer, True);
        }


        /* check if there is a command to be decoded */
        if (rx_packet.decode) {

            mbus_decode(&in_packet, mbus_inbuffer);

            mbus_process(&in_packet, mbus_outbuffer, False);

            rx_packet.decode = False;

            display_cursor(1, 1);
            display_printf("                    ");
            display_cursor(1, 1);
            display_printf("%s", in_packet.description);
        }

        display_cursor(4, 1);
        display_printf("D:%d T:%0d %0d:%02d", status_packet.disk, status_packet.track, status_packet.minutes, status_packet.seconds);






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