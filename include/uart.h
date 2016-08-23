/*
 * NVT-Control 
 *
 * Copyright (C) 2008  H9-Laboratory Ltd.
 * All rights reserved
 * 
 * CREATED			:	18.10.2008
 * VERSION			: 	1.0
 * TARGET DEVICE	: 	ATMEL ATmega128
 * 
 * COMMENT			:	This is the main control application for
 *						the Night-Vision-Terrain light controller.
 *
 */
 
 
 /*! 
 * @file 	nvt-control.c
 * @brief 	NVT Main Control Application
 * 
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
 */

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include "config.h"
#include "fifo.h"


#if BAUDRATE == 115200
	#define UART_DOUBLESPEED	// 2X-Mode, clock too unprecise
#endif

#ifdef UART_DOUBLESPEED
	#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)(F_CPU) / ((uint32_t)(baudRate) *  8) -1)
#else
	#define UART_CALC_BAUDRATE(baudRate) ((uint32_t)(F_CPU) / ((uint32_t)(baudRate) * 16) -1)
#endif



#ifdef __AVR_ATmega128__
	/* We want to use here UART 0 */
	#define UBRRH	UBRR0H
	#define UBRRL	UBRR0L
	#define UCSRA	UCSR0A
	#define UCSRB	UCSR0B
	#define UCSRC	UCSR0C
	#define UDR		UDR0
//	#define RXEN	RXEN0
//	#define TXEN	TXEN0
//	#define RXCIE	RXCIE0
//	#define UDRIE	UDRIE0
//	#define UDRE0	UDRE0
//	#define UCSZ0	UCSZ00
//	#define UCSZ1	UCSZ01
//	#define RXC		RXC0
//	#define TXC		TXC0
//	#define U2X		U2X0
#endif	// __AVR_ATmega128__	
	


/*!
 * @brief			Sendet Daten per UART im Little Endian
 * @param data		Datenpuffer
 * @param length	Groesse des Datenpuffers in Bytes
 */
void uart_write(void *data, uint8_t length);

/*!
 * @brief			Liest Zeichen von der UART
 * @param data		Der Zeiger an den die gelesenen Zeichen kommen
 * @param length	Anzahl der zu lesenden Bytes
 * @return			Anzahl der tatsaechlich gelesenen Zeichen
 */
#define uart_read(data, length)	fifo_get_data(&infifo, data, length);

/*!
 * @brief	Initialisiert den UART und aktiviert Receiver und Transmitter sowie den Receive-Interrupt. 
 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
 */
extern void uart_init(void);

/*!
 * @brief	Wartet, bis die Uebertragung fertig ist.
 */
static inline void uart_flush(void) {
	while (UCSRB & (1 << UDRIE0));
}

extern fifo_t infifo;	/*!< FIFO fuer Empfangspuffer */

/*! 
 * @brief	Prueft, ob Daten verfuegbar 
 * @return	Anzahl der verfuegbaren Bytes
 */
#define uart_data_available()	infifo.count

uint8_t uart_searchbuffer(uint8_t key);
//void uart_clearfifo(void);

#endif /* UART_H_ */
