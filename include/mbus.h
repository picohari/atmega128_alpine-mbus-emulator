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
 * @file mbus.h
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




#ifndef _MBUS_H_
#define _MBUS_H_


/*
 * OUTPUTS
 *
 * GPIO Pin to be used as M-BUS output (YELLOW)
 *
 * Connected to any pin on any port
 */
#if defined(__AVR_ATmega128__) 	// trying the Mega128 now (YELLOW)
#define PIN_MBUS_OUT	PD5   		// MBus output pin (active = pull low)
#define PORT_MBUS_OUT	PORTD 		// MBus output pin (active = pull low)
#define DDR_MBUS_OUT	DDRD		// MBus output pin (active = pull low)

#define PIN_DEBUG    	PC0 		//  So I map all error pins to 1.
#define PORT_DEBUG   	PORTC  		//  So I map all error pins to 1.
#define DDR_DEBUG    	DDRC  		//  So I map all error pins to 1.

/*
 * INPUTS
 *
 * GPIO Pin to be used as M-BUS input (ORANGE)
 *
 * connected to PD4 (ICP1), see datasheet for input capture
 */
#else
#error "Code is not prepared for that MCU!"
#endif


#if 0
// in timer ticks (SYSCLK / prescaler = 34.722 us)
#define DEFAULT_ZERO_TIME 37  	// low pulse 0.6 ms: "0" bit
#define DEFAULT_ONE_TIME  111 	// low pulse 1.9 ms: "1" bit
#define DEFAULT_BIT_TIME  187 	// time for a full bit cycle
#define DEFAULT_TOLERANCE 10 	// the allowed "jitter" on reception
#define DEFAULT_MIN_PAUSE 38 	// timeout for packet completion
#define DEFAULT_SPACE     50 	// pause before sending new packet
#endif

/* M-BUS timing in timer ticks (SYSCLK / prescaler = 34.722 us) */
#define DEFAULT_ZERO_TIME 38  	// low pulse 0.6 ms: "0" bit
#define DEFAULT_ONE_TIME  116 	// low pulse 1.9 ms: "1" bit
#define DEFAULT_BIT_TIME  192 	// time for a full bit cycle

#define DEFAULT_TOLERANCE 34 	// the allowed "jitter" on reception
#define DEFAULT_MIN_PAUSE 15 	// timeout for packet completion
#define DEFAULT_SPACE     55 	// pause before sending new packet

#define MBUS_BUFFER		  32	// buffer size of m-bus packets

/* EEPROM locations of constants, adapt init_eeprom() if changing these! */
#define EE_BAUDRATE       ((uint8_t*)0)
#define EE_MIN_ZERO_TIME  ((uint8_t*)1)
#define EE_MAX_ZERO_TIME  ((uint8_t*)2)
#define EE_MIN_ONE_TIME   ((uint8_t*)3)
#define EE_MAX_ONE_TIME   ((uint8_t*)4)
#define EE_BIT_TIMEOUT    ((uint8_t*)5)
#define EE_SEND_ZERO_TIME ((uint8_t*)6)
#define EE_SEND_ONE_TIME  ((uint8_t*)7)
#define EE_SEND_BIT_TIME  ((uint8_t*)8)
#define EE_SEND_SPACE     ((uint8_t*)9)

/*
#define MIN_ZERO_TIME  (DEFAULT_ZERO_TIME - DEFAULT_TOLERANCE)
#define MAX_ZERO_TIME  (DEFAULT_ZERO_TIME + DEFAULT_TOLERANCE)
#define MIN_ONE_TIME   (DEFAULT_ONE_TIME - DEFAULT_TOLERANCE)
#define MAX_ONE_TIME   (DEFAULT_ONE_TIME + DEFAULT_TOLERANCE)
#define BIT_TIMEOUT    (DEFAULT_BIT_TIME + DEFAULT_MIN_PAUSE)
#define SEND_ZERO_TIME (DEFAULT_ZERO_TIME)
#define SEND_ONE_TIME  (DEFAULT_ONE_TIME)
#define SEND_BIT_TIME  (DEFAULT_BIT_TIME)
#define SEND_SPACE     (DEFAULT_SPACE)
*/

/* M-BUS timing settings in EEPROM */
#define MIN_ZERO_TIME  eeprom_read_byte(EE_MIN_ZERO_TIME)
#define MAX_ZERO_TIME  eeprom_read_byte(EE_MAX_ZERO_TIME)
#define MIN_ONE_TIME   eeprom_read_byte(EE_MIN_ONE_TIME)
#define MAX_ONE_TIME   eeprom_read_byte(EE_MAX_ONE_TIME)
#define BIT_TIMEOUT    eeprom_read_byte(EE_BIT_TIMEOUT)
#define SEND_ZERO_TIME eeprom_read_byte(EE_SEND_ZERO_TIME)
#define SEND_ONE_TIME  eeprom_read_byte(EE_SEND_ONE_TIME)
#define SEND_BIT_TIME  eeprom_read_byte(EE_SEND_BIT_TIME)
#define SEND_SPACE     eeprom_read_byte(EE_SEND_SPACE)


/* bitflags for content */
#define F_DISK   0x00000001
#define F_TRACK  0x00000002
#define F_INDEX  0x00000004
#define F_MINUTE 0x00000008
#define F_SECOND 0x00000010
#define F_FLAGS  0x00000020



/* M-BUS receiver module */
typedef struct
{	// all the information for the receive state
	volatile enum
	{
		wait, 	// waiting for packet start
		high, 	// rising edge has been seen
		low,  	// falling edge has been seen
	} state;
	char rxbits[4]; 	// received bits as chars
	uint8_t num_bits; 	// # of received bits
	volatile uint8_t num_nibbles;	// # of stored nibbles in buffer
	volatile uint8_t decode;
} mbus_rx_t;


/* M-BUS transmiter module */
typedef struct
{	// all the information for the transmit state
	uint8_t num_bits; 	// # of sent bits
	uint8_t cur_nibble; // processed hex nibble
	enum
	{
		start,  // before sending a bit
		low_0,  // short pulse of a '0' is sent
		low_1,  // long pulse of a '1' is sent
		ende,   // end of sequence
	} state;
	volatile uint8_t send;
} mbus_tx_t;


/* M-BUS source device */
typedef enum {
	eUnknown = 0,
	eRadio = 1,
	eCD = 9,
} source_t;


/* M-BUS commands */
typedef enum {
	eInvalid,
	// radio to changer
	rPing,
	rPlay,
	rPause,
	rStop,
	rScnStop,
	rPlayFF,
	rPlayFR,
	rPauseFF,
	rPauseFR,
	rResume,
	rResumeP,
	rNextMix,
	rPrevMix,
	rRepeatOff,
	rRepeatOne,
	rRepeatAll,
	rScan,
	rMix,
	rSelect,
	rStatus,
	// changer to radio
	cPingOK,
	cAck,
	cPreparing,
	cStopped,
	cPaused,
	cPlaying,
	cSpinup,
	cForwarding,
	cReversing,
	cPwrUp,
	cLastInfo,
	cChanging,
	cChanging1,
	cChanging2,
	cChanging3,
	cChanging4,
	cNoMagzn,
	cStatus,
	cStat1,
	cStat2,
} command_t;


/* M-BUS packet type */
typedef struct {
//	char acRaw[100]; 			// hex packet including checksum
	source_t source; 			// origin

	int chksum; 				// checksum
	int chksumOK; 				// checksum OK

	command_t cmd; 				// command ID
	const char *description; 	// decoded desciption
	int flagdigits; 			// how many flag digits
	uint16_t validcontent : 16;
	//DWORD validcontent; // bit flags validating the following information items

	int disk;   	// BCD format
	int track;  	// BCD format
	int index;  	// BCD format
	int minutes; 	// BCD format
	int seconds; 	// BCD format
	int flags;  	// BCD format
} mbus_data_t;


#if 0
/* What to do after own echo has been received */
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
#endif

/* possible return codes from states */ 
enum ret_codes { 
	ok,
	reply, 
	end, 
	next, 
	fail,
	resume, 
	play, 
	stop, 
	changing
};

/* used for transition table */
struct transition {
    command_t 		src_state;
    enum ret_codes  ret_code;
    command_t 		dst_state;
};





/* One entry in the coding table */
typedef struct
{	// one entry in the coding table
	command_t cmd;
	char hexmask[32];
	char infotext[32];
} code_item_t;


/* Command code table */
static const code_item_t alpine_codetable[] =
{
	{ rPing, 			"18", 				"Ping               " },
	{ cPingOK, 			"98", 				"Ping OK            " },
	{ cAck, 			"9F0000f", 			"Ack/Wait           " }, 			// f0=0|1|6|7|9
	{ rStatus, 			"19",				"Some info?         " },
	{ cPreparing,  		"991ttiimmssff0f", 	"Preparing          " }, 			// f0=0:normal, f0=4:repeat one, f0=8:repeat all
	{ cStopped,    		"992ttiimmssff0f", 	"Stopped            " }, 			// f1=0:normal, f1=2:mix, f1=8:scan
	{ cPaused,     		"993ttiimmssff0f", 	"Paused             " }, 			// f3=1: play mode, f3=2:paused mode, f3=8: stopped
	{ cPlaying,    		"994ttiimmssff0f", 	"Playing            " },
	{ cSpinup,     		"995ttiimmssff0f", 	"Spinup             " },
	{ cForwarding, 		"996ttiimmssff0f", 	"FF                 " },
	{ cReversing,  		"997ttiimmssff0f", 	"FR                 " },
	{ rPlay,    		"11101", 			"Play               " },
	{ rPause,   		"11102", 			"Pause              " },
	{ rStop,    		"11140", 			"Stop               " },
	{ rScnStop, 		"11150", 			"Scan Stop          " },
	{ rPlayFF,  		"11105", 			"Play FF start      " },
	{ rPlayFR,  		"11109", 			"Play FR start      " },
	{ rPauseFF, 		"11106", 			"Pause FF start     " },
	{ rPauseFR, 		"1110A", 			"Pause FR start     " },
	{ rResume,  		"11181", 			"Play fr curr. pos. " },
	{ rResumeP, 		"11182", 			"Pause fr curr. pos." },
//	{ rNextMix, 	"1130A314", 			"next random" },
//	{ rPrevMix, 	"1130B314", 			"previous random" },
	{ rSelect,  		"113dttff", 		"Select             " }, 			// f0=1:playing, f0=2:paused, f1=4:random
	{ rRepeatOff, 		"11400000", 		"Repeat Off         " },
	{ rRepeatOne, 		"11440000", 		"Repeat One         " },
	{ rRepeatAll, 		"11480000", 		"Repeat All         " },
	{ rScan,      		"11408000", 		"Scan               " },
	{ rMix,       		"11402000", 		"Mix                " },
	{ cPwrUp, 			"9A0000000000", 	"Some powerup?      " },
	{ cLastInfo,  		"9B0dttfff0f", 		"Last played        " }, 		// f0=0:done, f0=1:busy, f0=8:eject, //f1=4: repeat1, f1=8:repeat all, f2=2:mix
	{ cNoMagzn,   		"9BAd00f00ff", 		"No Magazin         " },
	{ cChanging,  		"9B9dttfff0f", 		"Changing           " },
	{ cChanging1, 		"9BDd00fff0f", 		"Changing Phase 1   " },
	{ cChanging2, 		"9BBd00fff0f", 		"Changing Phase 2   " },
	{ cChanging3, 		"9BCd00fff0f", 		"Changing Phase 3   " },
	{ cChanging4, 		"9B8d00fff0f", 		"Changing Phase 4   " },
	{ cStatus, 			"9Cd01ttmmssf", 	"Disk Status        " },
	{ cStat1, 			"9D000fffff", 		"Some status?       " },
	{ cStat2, 			"9E0000000", 		"Some more status?  " },
	{ eInvalid, 		"0", 				"Idle               " },
	// also seen:
	// 11191
};

// globals
extern mbus_rx_t 	rx_packet;
extern mbus_tx_t 	tx_packet;

extern mbus_data_t in_packet;
extern mbus_data_t response_packet;
extern mbus_data_t status_packet;

extern char mbus_outbuffer[MBUS_BUFFER];
extern char mbus_inbuffer[MBUS_BUFFER];

extern uint16_t player_sec;
extern command_t last_radiocmd;


// prototypes
char int2hex(uint8_t n); 	// utility function: convert a number to a hex char
uint8_t hex2int(char c); 	// utility function: convert a hex char to a number
uint8_t mbus_searchbuffer(uint8_t key);

int8_t calc_checksum(char *buffer, uint8_t len);


void mbus_init (void); 		// helper function for main(): setup timers and pins

void init_eeprom (void); 	// a convenience feature to populate the timings in eeprom with reasonable defaults


uint8_t mbus_encode(mbus_data_t *mbuspacket, char *packet_dest);
uint8_t mbus_decode(mbus_data_t *mbuspacket, char *packet_src);

uint8_t mbus_process(const mbus_data_t *inpacket, char *buffer, uint8_t timercall);


void mbus_control (const mbus_data_t *inpacket);

void mbus_send(void);
uint8_t mbus_receive(void);

#endif
