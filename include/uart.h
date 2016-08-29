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
 * @file uart.h
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
