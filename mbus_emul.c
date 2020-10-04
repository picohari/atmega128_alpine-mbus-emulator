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

mbus_data_t in_packet;				// received state - decoded incoming packet : may be from radio or echo of our own

mbus_data_t response_packet;		// message state - to be sent to the head unit as response to another command

uint16_t player_sec = 0;



/* Current command to be evaluated */
command_t cur_cmd = eInvalid;

/* Last reveived return code */
enum ret_codes rc;

/* Current function pointer for command execution */
int (*cmd_funct)(void);

/* Last received command from radio head unit and our own! */
command_t	last_radiocmd;
command_t	last_cdcmd;



/*  
 * Perfom action for reveived command
 *
 * The next action is programmed by setting a new command and return with reply.
 * Otherwise, if return ok, no more messages are sent.
 */
static int idle_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = eInvalid;
	status_packet.cmd   = eInvalid;
    return ok;
}

static int ping_state(void) 
{
	response_packet.cmd = cPingOK;
    return reply;
}

static int play_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPlaying;
	response_packet.flags &= ~0x00B;
	response_packet.flags |=  0x001;
    return reply;
}

static int pause_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPaused;
	response_packet.flags &= ~0x00B;
	response_packet.flags |=  0x002;
    return reply;
}

static int stop_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cStopped;
	response_packet.flags |=  0x008;
    return reply;
}

static int scnstop_state(void) 
{
    return ok;
}

static int playff_state(void) 
{
    return ok;
}

static int playfr_state(void) 
{
    return ok;
}

static int pauseff_state(void) 
{
    return ok;
}

static int pausefr_state(void) 
{
    return ok;
}

static int resume_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cChanging;
	response_packet.flags = 0x0001; // done
    return reply;
}

static int resumep_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cChanging;
	response_packet.flags = 0x0001; // done
    return reply;
}

static int nextmix_state(void) 
{
    return ok;
}

static int prevmix_state(void) 
{
    return ok;
}

static int repeatoff_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPreparing;
	response_packet.flags &= ~0xCA0;

	//status_packet.cmd = cPreparing;
	status_packet.flags &= ~0xCA0;
    return reply;
}

static int repeatone_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPreparing;
	response_packet.flags &= ~0xCA0;
	response_packet.flags |= 0x400;

	//status_packet.cmd = cPreparing;
	status_packet.flags &= ~0xCA0;
	status_packet.flags |=  0x400;
    return ok;
}

static int repeatall_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPreparing;
	response_packet.flags &= ~0xCA0;
	response_packet.flags |= 0x800;

	//status_packet.cmd = cPreparing;
	status_packet.flags &= ~0xCA0;
	status_packet.flags |=  0x800;
    return ok;
}

static int scan_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPreparing;
	response_packet.flags &= ~0xCA0;
	response_packet.flags |= 0x080;

	//status_packet.cmd = cPreparing;
	status_packet.flags &= ~0xCA0;
	status_packet.flags |=  0x080;
    return ok;
}

static int mix_state(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPreparing;
	response_packet.flags &= ~0xCA0;
	response_packet.flags |= 0x020;

	//status_packet.cmd = cPreparing;
	status_packet.flags &= ~0xCA0;
	status_packet.flags |=  0x020;
    return reply;
}

static int select_state(void) 
{
	response_packet.cmd = cChanging;

	if (in_packet.disk != 0 && in_packet.disk != status_packet.disk) {
		response_packet.disk = in_packet.disk;
		//status_packet.disk = 9; // test hack!!
		response_packet.track = 1;
		response_packet.flags = 0x1001; // busy

		status_packet.track = 1;
		status_packet.disk = in_packet.disk;
	} else {
		// zero indicates same disk
		response_packet.disk = status_packet.disk;
		response_packet.track = in_packet.track;
		response_packet.flags = 0x0001; // done
		
		status_packet.track = in_packet.track;
	}

	status_packet.minutes = 0;
	status_packet.seconds = 0;
	player_sec = 0; // restart timer
    return reply;
}

static int status_state(void) 
{
	response_packet.cmd = cAck;
    return reply;
}



/*  
 * If we send out a reply to a command, the receiver decodes it like any other one...
 * This "echo" can be used to trigger a new command coming from our emulator.
 *
 * The next action is programmed by setting a new command and return with reply.
 * Otherwise, if return ok, no more messages are sent.
 */
static int ping_self(void) 
{
    return ok;
}

static int ack_self(void) 
{
	response_packet = status_packet;

	if (last_radiocmd == rStatus) {
		response_packet.cmd = cStatus;
		response_packet.track = INT2BCD(99);
		response_packet.minutes = INT2BCD(99);
		response_packet.seconds = INT2BCD(99);
	    return reply;

	} else if (last_radiocmd == rResume) {
		response_packet.cmd = cPaused;
		response_packet.flags &= ~0x00B;
		response_packet.flags |=  0x002;

		status_packet.cmd = cPaused;
		status_packet.flags &= ~0x00B;
		status_packet.flags |=  0x002;
		return reply;

	} else if (last_radiocmd == rSelect) {
		response_packet.cmd = cPlaying;
		response_packet.flags &= ~0x00B;
		response_packet.flags |=  0x001;

		status_packet.cmd = cPlaying;
		status_packet.flags &= ~0x00B;
		status_packet.flags |=  0x001;
		return ok;

	} else
		return ok;
}

static int preparing_self(void) 
{
	response_packet = status_packet;

	if (status_packet.flags & 0x01) {
		response_packet.cmd = cPlaying;
		status_packet.cmd = cPlaying;
		return reply;

	} else if (status_packet.flags & 0x02) {
		response_packet.cmd = cPaused;
		status_packet.cmd = cPaused;
		return reply;
	} 
    return ok;
}

static int stopped_self(void) 
{
	status_packet.cmd = cStopped;
	//status_packet.flags &= ~0x00B;  // must not delete previous play/pause state, just add stop
	status_packet.flags |=  0x008;
    return ok;
}

static int paused_self(void) 
{
	status_packet.cmd = cPaused;
	status_packet.flags &= ~0x00B;
	status_packet.flags |=  0x002;
    return ok;
}

static int playing_self(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cPlaying;
	response_packet.flags &= ~0x00B;
	response_packet.flags |=  0x001;

	status_packet.cmd = cPlaying;
	status_packet.flags &= ~0x00B;
	status_packet.flags |=  0x001;
    return play;
}

static int spinup_self(void) 
{
	response_packet.cmd = cAck;
    return reply;
}

static int forwarding_self(void) 
{
    return ok;
}

static int reversing_self(void) 
{
    return ok;
}

static int powerup_self(void) 
{
    return ok;
}

static int lastinfo_self(void) 
{
    return ok;
}

static int changing_self(void) 
{
	response_packet.cmd = cAck;
    return reply;
}

static int changing1_self(void) 
{
    return ok;
}

static int changing2_self(void) 
{
    return ok;
}

static int changing3_self(void) 
{
    return ok;
}

static int changing4_self(void) 
{
    return ok;
}

static int nomagazin_self(void) 
{
    return ok;
}

static int status_self(void) 
{
	response_packet = status_packet;

	response_packet.cmd = cSpinup;
	response_packet.flags = 0x0001; // play
    return reply;
}

static int status1_self(void) 
{
    return ok;
}

static int status2_self(void) 
{
    return ok;
}


/* Array von Funktions-Pointern für jeden Zustand. Muss synchron mit o.g. Array-Auflistung (enum state_codes) sein !!! */
int (*cmd_state[])(void) = {
	idle_state,
	// radio to changer 
	ping_state,
	play_state,
	pause_state,
	stop_state, 
	scnstop_state, 
	playff_state, 
	playfr_state, 
	pauseff_state, 
	pausefr_state,
	resume_state,
	resumep_state,
	nextmix_state,
	prevmix_state,
	repeatoff_state,
	repeatone_state,
	repeatall_state,
	scan_state,
	mix_state,
	select_state,
	status_state,
	// changer to radio
	ping_self,
	ack_self,
	preparing_self,
	stopped_self,
	paused_self,
	playing_self,
	spinup_self,
	forwarding_self,
	reversing_self,
	powerup_self,
	lastinfo_self,
	changing_self, 
	changing1_self, 
	changing2_self, 
	changing3_self, 
	changing4_self,
	nomagazin_self,
	status_self,
	status1_self,
	status2_self,
};


/* Transitions-Tabelle für jeden Zustand */
struct transition state_transitions[] = {

#if 0
    {rPing, 		reply,  	cPingOK},
    {rStatus, 		reply,  	cAck},
    
    {rResume,		reply,		cChanging},
    {cChanging,		reply,		cAck},
    {cAck,			reply,		cStatus},
    {cStatus,		reply,		eInvalid},

    {rStop,			reply,  	cStopped},

    {rPause,		reply,   	cPaused},

    {rPlay,			reply,  	cPlaying},
    {cPlaying,		ok,     	cPlaying},

    {rSelect,		reply,		cAck},
    {cAck,			ok,			eInvalid},
#endif

    {eInvalid, 		ok,     	eInvalid},
	/* Transition für end wird nicht benötigt, da nie erreicht ... */
};


/* fetch new destination state depending on return code of current state */
static command_t lookup_transitions(command_t current, enum ret_codes ret)
{
	int i = 0;

	command_t temp = eInvalid;

	for (i = 0;; ++i) {

		if (state_transitions[i].src_state == current && state_transitions[i].ret_code == ret) {
			temp = state_transitions[i].dst_state;
			break;
		}

		if (state_transitions[i].src_state == eInvalid) {
			temp = eInvalid;
			ret = ok;
			break;
		}
	}

	return temp;
}


/* Main motor control routine */
void mbus_control (const mbus_data_t *inpacket)
{
	/* Determine current command of incoming packet */
	cur_cmd = inpacket->cmd;

	/* Store last command coming from radio (used for ACK/WAIT) */
	if (inpacket->source == eRadio)
		last_radiocmd = cur_cmd;

	/* Select function to handle the current command */
    cmd_funct = cmd_state[cur_cmd];

    /* Execute function assigned for reception */
    rc = cmd_funct();

    /* In case of no reply returned, skip */
    if (ok == rc)
    	return;

    /* In case of playing, skip bcse we are sending every 500ms anyway */
    if (play == rc)
    	return;

    if (reply == rc) {
    	mbus_encode(&response_packet, mbus_outbuffer);
    	mbus_send();
    	mbus_receive();
    }

    //cur_cmd = lookup_transitions(cur_cmd, rc);
}
