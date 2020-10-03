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
#include "hd44780.h"

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

	#ifdef HD44780_AVAILABLE
		hd44780_init();
		hd44780_clear();
		hd44780_cursor(0, 0);
	#endif

    LOG_INFO("M-BUS Adapter 1.0\n");
}


int main (void) {

	/* Initiate all */
	init();

    init_mbus();

    /* setup the states, only the necessary */
    rx_packet.state = wait;
    tx_packet.num_bits = 0;

    sei();


    for (;;) {

        /* Perform some player timings and simulations */
        static uint32_t player_ticks = 0;
        static uint32_t sending_ticks = 0;

        /* normal CD play action updates only every second */
        if (timer_ms_passed(&player_ticks, 1000)) {

            if (status_packet.cmd == cPlaying)
                player_sec++;

            if (player_sec == 5400)   // after 90 minutes reset the counter
                player_sec = 0;
        }

        /* update status packet information about playing time */
        status_packet.minutes = INT2BCD(player_sec / 60);
        status_packet.seconds = INT2BCD(player_sec % 60);

        /* send every 500ms a new status packet to the head-unit */
        if (timer_ms_passed(&sending_ticks, 500)) {

            if (status_packet.cmd == cPlaying ) {

                /* Show info about selected repeat mode */
                #ifdef HD44780_AVAILABLE
                    hd44780_cursor(1, 16);
                    if (status_packet.flags & 0x400)
                        hd44780_printf("R-ONE");
                    if (status_packet.flags & 0x800)
                        hd44780_printf("R-ALL");
                    if (status_packet.flags & 0x080)
                        hd44780_printf("SCAN ");
                    if (status_packet.flags & 0x020)
                        hd44780_printf(" MIX ");
                    else
                        hd44780_printf("     ");
                #endif

                mbus_process(&in_packet, mbus_outbuffer, True);
            }
        }


        /* check if there is a command to be decoded */
        if (rx_packet.decode) {


            mbus_decode(&in_packet, mbus_inbuffer);

            mbus_process(&in_packet, mbus_outbuffer, False);

            rx_packet.decode = False;

            /* show the actual decoded command on LCD */
            #ifdef HD44780_AVAILABLE
                hd44780_cursor(4, 1);
                hd44780_printf("                    ");
                hd44780_cursor(4, 1);
                hd44780_printf("%s", in_packet.description);
            #endif
        }


         /* Show info about disk, track and playing status */
        #ifdef HD44780_AVAILABLE
            hd44780_cursor(1, 1);
            hd44780_printf("D:%d T:%02d %02d:%02d", status_packet.disk, status_packet.track, player_sec / 60, player_sec % 60);
        #endif


        /* check if there is a command to be sent */
        if (!(TIMSK & _BV(TOIE0))                       // not already sending
            && rx_packet.state == wait                  // not receiving
            && ( /*new_uart ||*/ tx_packet.send )       // newline in receive buffers
            ) {


            // start sending the transmission
            tx_packet.state = start;
            TCCR0  = ((1 << CS01) | (1 << CS02));   // slow prescaling while sending
            TCNT0 = 0;                              // reset timer because ISR only offsets to it
            TIMSK |= _BV(TOIE0);                    // start the output handler with timer0

            tx_packet.send = False;
        }


	} /* End for (;;) */


	/* If we ever should get here ;-) */
	return 1;

}
