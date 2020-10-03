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
 * @file mbus_emul.c
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


mbus_data_t status_packet;			// player state   - decoded outgoing packet : represents current player information

mbus_data_t in_packet;				// received state - decoded incoming packet : represents remote head-unit



uint16_t player_sec = 0;










/*
d88888b .88b  d88. db    db db       .d8b.  d888888b  .d88b.  d8888b. 
88'     88'YbdP`88 88    88 88      d8' `8b `~~88~~' .8P  Y8. 88  `8D 
88ooooo 88  88  88 88    88 88      88ooo88    88    88    88 88oobY' 
88~~~~~ 88  88  88 88    88 88      88~~~88    88    88    88 88`8b   
88.     88  88  88 88b  d88 88booo. 88   88    88    `8b  d8' 88 `88. 
Y88888P YP  YP  YP ~Y8888P' Y88888P YP   YP    YP     `Y88P'  88   YD 
*/

// returns S_OK if an answer is desired (then no timer should be spawned)
// or S_FALSE if not replying, in which case a timer may be spawned

// string version of the received packed, for echo test
// received packet in already decoded form
// destination for the answer packet string
// set to milliseconds units in wished to be called again

uint8_t mbus_process(const mbus_data_t *inpacket, char *buffer, uint8_t timercall)
{
	//uint8_t hr = 1; 		// default is do reply
	mbus_data_t response;

	memset(&response, 0, sizeof(response));

	/* Ok an dieser Stelle klappt die ACK/Wait Verbindung, alles andere noch nicht hier... */

#if 0
	if (timercall) {
		// evaluate state and send more or maybe spawn timer
		switch (echostate) {

		case get_state: // this state exists to send the first timestamp right away
			echostate = (status_packet.cmd == cPlaying) ? playing : quiet;
			response = status_packet;
			break;

		case playing:
			
			response = status_packet;

			if (status_packet.cmd == cPlaying) {
				mbus_encode(&response, buffer);
				return 1;
				//status_packet.track = INT2BCD(BCD2INT(status_packet.track) + 1); // count
				//if (status_packet.track > 0x99)
				//	status_packet.track = 0;
			}
#if 0
			else if (status_packet.cmd == cPaused) {
				mbus_encode(&response, buffer);
				return 1;
			}

			else if (status_packet.cmd == cStopped) {
				mbus_encode(&response, buffer);
				return 1;
			}
#endif
			else
				hr = 0;

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
	}
#endif

#if 0
	// check for echo
	if (echo_waitstate && inpacket != NULL) {


		// I should have received my own packet string back. If not, there might be a collision?
		if (strcmp(last_sent, raw_received) == 0) {
			// OK, we might send the next if desired
			echo_waitstate = false;

			uart_write((uint8_t *)"H", 1);


			if (hr == 1) {
				//*sched_timer = 0; // discipline myself: no timer if responding
				mbus_encode(&response, buffer);
				strcpy(last_sent, buffer); // remember for echo check
				echo_waitstate = true;
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
			echo_waitstate = true;
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

#if 1
	// generate immediate response to radio command, want to see no timer spawn here
	switch(inpacket->cmd) {

	case rPing:
		response.cmd = cPingOK;
		break;

	case rStatus:
		response.cmd = cAck;
		break;


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
			response.track = status_packet.track = 1;
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
		player_sec = 0; // restart timer
		break;

	case rResume:
	case rResumeP:
		echostate = resuming;
		status_packet.cmd = (inpacket->cmd == rResume) ? cPlaying : cPaused;
		response.cmd = cChanging;
		response.disk = status_packet.disk;
		response.track = status_packet.track;
		response.flags = 0x0001; // done
		break;



	default:
		response.cmd = cAck;
		//response.cmd = eInvalid;
		//response.cmd = cPingOK;

	} // switch(inpacket->cmd)


	if (response.cmd != eInvalid) {
		//*sched_timer = 0; // discipline myself: no timer if responding
		mbus_encode(&response, buffer);
		//strcpy(last_sent, buffer); // remember for echo check
		return 1;
	}

#endif

	return 0;
}


