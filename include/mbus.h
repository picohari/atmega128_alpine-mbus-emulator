#ifndef _MBUS_H_
#define _MBUS_H_


#include "fifo.h"


// a place for platform dependent indirect definitions
#if defined(__AVR_AT90S2313__) // the only one used so far
#define PIN_MBUS_OUT PB2 // MBus output pin (active = pull low)
#define PIN_TX_OF    PB6 // TX overflow error, send buffer full
#define PIN_RX_OF    PB5 // RX overflow error, receive buffer full
#define PIN_RX_UF    PB4 // RX underflow error, no char available
#define PIN_DEBUG    PB7 // debug condition

#elif defined(__AVR_ATmega128__) // trying the Mega8 now
#define PIN_MBUS_OUT  PD5   // MBus output pin (active = pull low)
#define PORT_MBUS_OUT PORTD // MBus output pin (active = pull low)
#define DDR_MBUS_OUT  DDRD // MBus output pin (active = pull low)
//#define PIN_TX_OF    PB1 // Really strange: PIN_MBUS_OUT becomes weak
//#define PIN_RX_OF    PB1 //  if any other b-port pin than 3 used for it,
//#define PIN_RX_UF    PB0 //  or any other than 1 is used additionally.
#define PIN_DEBUG    PC0 	//  So I map all error pins to 1.
#define PORT_DEBUG   PORTC  //  So I map all error pins to 1.
#define DDR_DEBUG    DDRC  //  So I map all error pins to 1.

#define CTC1 	WGM12
#define OCR1H 	OCR1AH
#define OCR1L 	OCR1AL
#define TICIE 	TICIE1
#define UBRR 	UBRRL
#define UCR 	UCSRB

#else
#error "Code is not prepared for that MCU!"
#endif



// in timer ticks (SYSCLK / prescaler = 34.722 us)
#define DEFAULT_ZERO_TIME 38  	// low pulse 0.6 ms: "0" bit
#define DEFAULT_ONE_TIME  112 	// low pulse 1.9 ms: "1" bit
#define DEFAULT_BIT_TIME  187 	// time for a full bit cycle

#define DEFAULT_TOLERANCE 5 	// the allowed "jitter" on reception
#define DEFAULT_MIN_PAUSE 38 	// timeout for packet completion
#define DEFAULT_SPACE     50 	// pause before sending new packet

#define MBUS_BUFFER		  32	// buffer size of m-bus packets

// eeprom locations of constants, adapt init_eeprom() if changing these!
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

#define MIN_ZERO_TIME  eeprom_read_byte(EE_MIN_ZERO_TIME)
#define MAX_ZERO_TIME  eeprom_read_byte(EE_MAX_ZERO_TIME)
#define MIN_ONE_TIME   eeprom_read_byte(EE_MIN_ONE_TIME)
#define MAX_ONE_TIME   eeprom_read_byte(EE_MAX_ONE_TIME)
#define BIT_TIMEOUT    eeprom_read_byte(EE_BIT_TIMEOUT)
#define SEND_ZERO_TIME eeprom_read_byte(EE_SEND_ZERO_TIME)
#define SEND_ONE_TIME  eeprom_read_byte(EE_SEND_ONE_TIME)
#define SEND_BIT_TIME  eeprom_read_byte(EE_SEND_BIT_TIME)
#define SEND_SPACE     eeprom_read_byte(EE_SEND_SPACE)


// bitflags for content
#define F_DISK   0x00000001
#define F_TRACK  0x00000002
#define F_INDEX  0x00000004
#define F_MINUTE 0x00000008
#define F_SECOND 0x00000010
#define F_FLAGS  0x00000020




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
	volatile uint8_t num_bytes;	// # of stored bytes in buffer
} mbus_rx_t; 


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
} mbus_tx_t; 


typedef enum {
	eUnknown = 0,
	eRadio = 1,
	eCD = 9,
} source_t;

typedef enum {
	eInvalid = 0,
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
	cChanging4,
	cChanging,
	cNoMagzn,
	cChanging2,
	cChanging3,
	cChanging1,
	cStatus,
	cStat1,
	cStat2,
} command_t;


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


// globals
extern mbus_rx_t 	rx_packet;
extern mbus_tx_t 	tx_packet;

extern mbus_data_t packet;

extern char mbus_outbuffer[MBUS_BUFFER];
extern fifo_t mbus_outfifo;

extern char mbus_inbuffer[MBUS_BUFFER];
extern fifo_t mbus_infifo;



// prototypes
void init_mbus (void); // helper function for main(): setup timers and pins
void init_eeprom (void); // a convenience feature to populate the timings in eeprom with reasonable defaults

char int2hex(uint8_t n); // utility function: convert a number to a hex char
uint8_t hex2int(char c); // utility function: convert a hex char to a number
int8_t calc_checksum(char *buffer, uint8_t len);
uint8_t mbus_searchbuffer(uint8_t key);


uint8_t mbus_encode(mbus_data_t *mbuspacket, char *packet_dest);
uint8_t mbus_decode(mbus_data_t *mbuspacket, char *packet_src);

uint8_t mbus_process(const char *raw_received, const mbus_data_t *inpacket, char *buffer /*, uint16_t *sched_timer*/ ); 			


// interrupt handlers
//SIGNAL(SIG_OVERFLOW0); // timer 0 overflow, used for sending
//SIGNAL(SIG_INPUT_CAPTURE1); // edge detection interrupt, the heart of receiving
//SIGNAL(SIG_OUTPUT_COMPARE1A); // timeout interrupt, this terminates a received packet






#endif
