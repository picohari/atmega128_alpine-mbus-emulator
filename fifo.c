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
 * @file 	fifo.c
 * @brief 	Implementation of a FIFO (First In, First Out)
 * 
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
 * Thread-Safe, secured against Interrupts, as long as Producer or Consumer are on the same Interrupt-Layer.
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
