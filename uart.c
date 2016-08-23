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
 
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#include "uart.h"


#ifdef UART_AVAILABLE

#define BUFSIZE_IN 0x30
uint8_t inbuf[BUFSIZE_IN];	/*!< Eingangspuffer */
fifo_t infifo;				/*!< Eingangs-FIFO */

#if BAUDRATE == 115200
#define BUFSIZE_OUT	0x70	// wer schneller sendet, braucht auch weniger Pufferspeicher ;)
#else
#define BUFSIZE_OUT 0x80
#endif	// BAUDRATE
uint8_t outbuf[BUFSIZE_OUT];	/*!< Ausgangspuffer */
fifo_t outfifo;					/*!< Ausgangs-FIFO */

#define UART_RX_BUFFER_MASK (BUFSIZE_IN - 1)

/*!
 * @brief	Initialisiert den UART und aktiviert Receiver und Transmitter sowie den Receive-Interrupt. 
 * Die Ein- und Ausgebe-FIFO werden initialisiert. Das globale Interrupt-Enable-Flag (I-Bit in SREG) wird nicht veraendert.
 */
void uart_init(void)
{	 
    uint8_t sreg = SREG;
    UBRRH = (UART_CALC_BAUDRATE(BAUDRATE)>>8) & 0xFF;
    UBRRL = (UART_CALC_BAUDRATE(BAUDRATE) & 0xFF);
    
	/* Interrupts kurz deaktivieren */ 
	cli();

	/* UART Receiver und Transmitter anschalten, Receive-Interrupt aktivieren */ 
	UCSRB = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	/* Data mode 8N1, asynchron */
	uint8_t ucsrc = (1 << UCSZ01) | (1 << UCSZ00);
	#ifdef URSEL 
		ucsrc |= (1 << URSEL);	// fuer ATMega32
	#endif    
	UCSRC = ucsrc;

    /* Flush Receive-Buffer (entfernen evtl. vorhandener ungueltiger Werte) */ 
    do {
		UDR;	// UDR auslesen (Wert wird nicht verwendet)
    } while (UCSRA & (1 << RXC0));

    /* Ruecksetzen von Receive und Transmit Complete-Flags */ 
    UCSRA = (1 << RXC0) | (1 << TXC0)
#ifdef UART_DOUBLESPEED
    		| (1 << U2X0)
#endif
    		;
	
    /* Global Interrupt-Flag wiederherstellen */
    SREG = sreg;

    /* FIFOs für Ein- und Ausgabe initialisieren */ 
    fifo_init(&infifo, inbuf, BUFSIZE_IN);
    fifo_init(&outfifo, outbuf, BUFSIZE_OUT);
}

/*!
 * @brief	Interrupthandler fuer eingehende Daten
 * Empfangene Zeichen werden in die Eingabgs-FIFO gespeichert und warten dort.
 */ 
ISR(USART0_RX_vect)
{
	_inline_fifo_put(&infifo, UDR);
}

/*!
 * @brief	Interrupthandler fuer ausgehende Daten
 * Ein Zeichen aus der Ausgabe-FIFO lesen und ausgeben.
 * Ist das Zeichen fertig ausgegeben, wird ein neuer SIG_UART_DATA-IRQ getriggert.
 * Ist die FIFO leer, deaktiviert die ISR ihren eigenen IRQ.
 */ 
ISR(USART0_UDRE_vect)
{

	if (outfifo.count > 0)
		UDR = _inline_fifo_get(&outfifo);
	else
		UCSRB &= ~(1 << UDRIE0);	// diesen Interrupt aus
}

/*!
 * @brief			Sendet Daten per UART im Little Endian
 * @param data		Datenpuffer
 * @param length	Groesse des Datenpuffers in Bytes
 */
void uart_write(void *data, uint8_t length)
{
	if (length > BUFSIZE_OUT) {
		/* das ist zu viel auf einmal => teile und herrsche */
		uart_write(data, length / 2);
		uart_write(data + length / 2, length - length / 2);
		return;
	}

	/* falls Sendepuffer voll, diesen erst flushen */ 
	uint8_t space = BUFSIZE_OUT - outfifo.count;
	
	if (space < length)
		uart_flush();

	/* Daten in Ausgangs-FIFO kopieren */
	fifo_put_data(&outfifo, data, length);

	/* Interrupt an */
	UCSRB |= (1 << UDRIE0);
}


/*!
 * @brief			Search for a specific char in buffer
 * @param key		Char to search for
 * @return 			Position in buffer of character, otherwise 0
 */

uint8_t uart_searchbuffer(uint8_t key)
{
	uint8_t i;
	uint8_t level = infifo.count;

	for (i = 0; i <= level; i++) {
		if (inbuf[i] == key)
			return i; // found
	}
	return 0; // not found
}


#endif	// UART_AVAILABLE
