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
 * @file fifo.h
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

#ifndef _FIFO_H_
#define _FIFO_H_

#include "config.h"
#include "global.h"

#include <avr/io.h>
#include <avr/interrupt.h>

	
/*! FIFO-Datentyp */
typedef struct {
	uint8_t volatile count;	/*!< # Zeichen im Puffer */
	uint8_t size;			/*!< Puffer-Grosse */
	uint8_t * pread;		/*!< Lesezeiger */
	uint8_t * pwrite;		/*!< Schreibzeiger */
	uint8_t read2end;		/*!< # Zeichen bis zum Ueberlauf Lesezeiger */
	uint8_t write2end;		/*!< # Zeichen bis zum Ueberlauf Schreibzeiger */
} fifo_t;

/*!
 * @brief			Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc. 
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
 */
extern void fifo_init(fifo_t *f, void *buffer, const uint8_t size);
	
/*!
 * @brief			Schreibt length Byte in die FIFO
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *data		Zeiger auf Quelldaten
 * @param length	Anzahl der zu kopierenden Bytes
 */	
extern void fifo_put_data(fifo_t *f, void *data, uint8_t length);

/*!
 * @brief			Liefert length Bytes aus der FIFO, nicht blockierend.
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *data		Zeiger auf Speicherbereich fuer Zieldaten
 * @param length	Anzahl der zu kopierenden Bytes
 * @return			Anzahl der tatsaechlich gelieferten Bytes
 */	
extern uint8_t fifo_get_data(fifo_t *f, void * data, uint8_t length);

/*!
 * @brief		Schreibt ein Byte in die FIFO.
 * @param *f	Zeiger auf FIFO-Datenstruktur
 * @param data	Das zu schreibende Byte
 */
static inline void _inline_fifo_put(fifo_t *f, const uint8_t data)
{
	uint8_t *pwrite = f->pwrite;
	*(pwrite++) = data;
	uint8_t write2end = f->write2end;

	if (--write2end == 0) {
		write2end = f->size;
		pwrite -= write2end;
	}
	
	f->write2end = write2end;
	f->pwrite = pwrite;
	f->count++;
}

/*!
 * @brief		Liefert das naechste Byte aus der FIFO. 
 * @param *f	Zeiger auf FIFO-Datenstruktur
 * @return		Das Byte aus der FIFO
 * Ob ueberhaupt ein Byte in der FIFO ist, muss vorher extra abgeprueft werden!
 */
static inline uint8_t _inline_fifo_get(fifo_t *f)
{
	uint8_t *pread = f->pread;
	uint8_t data = *(pread++);
	uint8_t read2end = f->read2end;
	
	if (--read2end == 0) {
		read2end = f->size;
		pread -= read2end;
	}
	
	f->pread = pread;
	f->read2end = read2end;
	f->count--;
	
	return data;
}

/*!
 * Liefert das naechste Byte aus der FIFO, bei leerer FIFO wird gewartet, bis das naechste Zeichen eintrifft.
 */	
extern uint8 fifo_get_wait(fifo_t *f);

/*!
 * Liefert das naechste Byte aus der FIFO als int8 bzw. 0, falls die FIFO leer ist.
 */	
extern uint8 fifo_get_nowait(fifo_t *f);

#endif	/* _FIFO_H_ */
