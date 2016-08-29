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
 * @file fifo.c
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




#include "fifo.h"

/*!
 * @brief			Initialisiert die FIFO, setzt Lese- und Schreibzeiger, etc. 
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *buffer	Zeiger auf den Puffer der Groesse size fuer die FIFO
 * @param size		Anzahl der Bytes, die die FIFO speichern soll	.
 */
void fifo_init(fifo_t *f, void *buffer, const uint8_t size)
{
	f->count = 0;
	f->pread = f->pwrite = buffer;
	f->read2end = f->write2end = f->size = size;
}
	
/*!
 * @brief			Schreibt length Byte in die FIFO
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *data		Zeiger auf Quelldaten
 * @param length	Anzahl der zu kopierenden Bytes
 */
void fifo_put_data(fifo_t *f, void *data, uint8_t length)
{
	uint8_t *src = data;
	uint8_t *pwrite = f->pwrite;
	uint8_t write2end = f->write2end;
	uint8_t n = length > write2end ? write2end : length;
	uint8_t i,j;

	for (j = 0; j < 2; j++) {
		for (i = 0; i < n; i++) {
			*(pwrite++) = *(src++);
		}

		write2end -= n;
		if (write2end == 0) {
			write2end = f->size;
			pwrite -= write2end;
		}
		n = length - n;
	}
			
	f->write2end = write2end;
	f->pwrite = pwrite;

	uint8_t sreg = SREG;
	cli();

	f->count += length;

	SREG = sreg;
}

/*!
 * @brief			Liefert length Bytes aus der FIFO, nicht blockierend fuer MCU, aber fuer PC / phtreads
 * @param *f		Zeiger auf FIFO-Datenstruktur
 * @param *data		Zeiger auf Speicherbereich fuer Zieldaten
 * @param length	Anzahl der zu kopierenden Bytes
 * @return			Anzahl der tatsaechlich gelieferten Bytes
 */	
uint8_t fifo_get_data(fifo_t *f, void *data, uint8_t length)
{

	uint8_t count = f->count;

	if (count < length)
		length = count;
	
	uint8_t *pread = f->pread;
	uint8_t read2end = f->read2end;
	uint8_t n = length > read2end ? read2end : length;
	uint8_t *dest = data;
	uint8_t i,j;

	for (j = 0; j < 2; j++) {
		for (i = 0; i < n; i++) {
			*(dest++) = *(pread++);
		}
		read2end -= n;
		if (read2end == 0) {
			read2end = f->size;
			pread -= read2end;
		}
		n = length - n;
	}

	f->pread = pread;
	f->read2end = read2end;
		
	uint8_t sreg = SREG;
	cli();

	f->count -= length;

	SREG = sreg;
		
	return length;
}

/*!
 * Liefert das naechste Byte aus der FIFO, bei leerer FIFO wird gewartet, bis das naechste Zeichen eintrifft.
 */	
uint8 fifo_get_wait(fifo_t *f)
{
	while (!f->count);
	return _inline_fifo_get(f);	
}

/*!
 * Liefert das naechste Byte aus der FIFO als int8 bzw. 0, falls die FIFO leer ist.
 */	
uint8 fifo_get_nowait(fifo_t *f)
{
	if (!f->count)
		return 0;
	return (uint8)_inline_fifo_get(f);	
}






#if 0
void uart_clearfifo(void)
{
	memset(inbuf, 0, BUFSIZE_IN);
	fifo_init(&infifo, inbuf, BUFSIZE_IN);
}
#endif
