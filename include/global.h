/*
 * c't-Bot
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/**
 * \file 	global.h
 * \brief 	Allgemeine Definitionen und Datentypen
 * \author 	Benjamin Benz (bbe@heise.de)
 * \date 	20.12.2005
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_


#include <stdint.h>
#include <math.h>

#ifdef DOXYGEN
#define PC
#define MCU
#define WIN32
#define __linux__
#endif // DOXYGEN


#define byte	uint8_t		/*!< unsigned 8-Bit-Zahl */
#define uint8	uint8_t		/*!< unsigned 8-Bit-Zahl */
#define int8	int8_t		/*!< signed 8-Bit-Zahl */
#define uint16	uint16_t	/*!< unsigned 16-Bit-Zahl */
#define int16	int16_t		/*!< signed 16-Bit-Zahl */
#define uint32	uint32_t	/*!< unsigned 32-Bit-Zahl */
#define	int32	int32_t		/*!< signed 32-Bit-Zahl */

#define True	1			/**< wahr */
#define False	0			/**< falsch */

#define On		1			/**< an */
#define Off		0			/**< aus */

#define binary(var, bit) ((var >> bit) & 1)	/**< gibt das Bit "bit" von "var" zurueck */

#ifdef WIN32
#define LINE_FEED "\r\n"	/**< Linefeed fuer Windows */
#else
#define LINE_FEED "\n"		/**< Linefeed fuer nicht Windows */
#endif


//#include "builtins.h"

#include <avr/interrupt.h>

#ifdef SIGNAL
#define NEW_AVR_LIB	/**< neuere AVR_LIB-Version */
#else // ! SIGNAL
#include <avr/signal.h>
#endif // SIGNAL

#if defined __AVR_ATmega644__ || defined __AVR_ATmega644P__
#define MCU_ATMEGA644X /**< ATmega644-Familie (ATmega644 oder ATmega644P) */
#endif



#ifndef DOXYGEN
#define PACKED __attribute__ ((packed)) /**< packed-Attribut fuer Strukturen und Enums */
#else
#define PACKED
#endif


#include <avr/pgmspace.h>



/** Repraesentation eines Bits, dem ein Byte-Wert zugewiesen werden kann */
typedef union {
	uint8_t byte;
	unsigned bit:1;
} PACKED bit_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846 /** pi */
#endif
#ifndef M_PI_2
#define M_PI_2	(M_PI / 2.0) /**< pi / 2 */
#endif

// utility macros
#define BCD2INT(n) (((n)/16) * 10 + ((n) % 16)) // for max. 2 digits
#define INT2BCD(n) (((n)/10) * 16 + ((n) % 10)) // for max. 2 digits
#define INT2HEX(n) (((n) < 10) ? '0' + (n) : 'A' - 10 + (n)) // single digit

#endif // GLOBAL_H_