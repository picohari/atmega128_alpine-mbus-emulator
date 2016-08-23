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
 * @file 	timer.c
 * @brief 	Timing functions and counters
 * 
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
 */
 
 
#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"



/*! Protect the time-sync */
#define LOCK()		/* ISR */
/*! Release the time-sync lock */
#define UNLOCK()	/* ISR */

volatile tickCount_t tickCount;		/*!< one Tick every 176 us */


#ifdef TIME_AVAILABLE
/*!
 * This function returnes the system time in parts of milliseconds.
 * @return	Milliseconds of system time
 */
uint16_t timer_get_ms(void)
{
	return ((TIMER_GET_TICKCOUNT_32 * (TIMER_STEPS / 8)) / (1000 / 8)) % 1000;
}

/*!
 * This function returnes the system time in parts of seconds.
 * @return 	Seconds of system time
 */
uint16_t timer_get_s(void)
{
	return TIMER_GET_TICKCOUNT_32 * (TIMER_STEPS / 16) / (1000000 / 16);
}

/*!
 * Returns seconds passed since old_s, old_ms
 * @param old_s		old value for the seconds
 * @param old_ms	old value for the milliseconds
 */
uint16_t timer_get_ms_since(uint16_t old_s, uint16_t old_ms)
{
	return (timer_get_s() - old_s) * 1000 + timer_get_ms() - old_ms;
}
#endif // TIME_AVAILABLE


// ---- Timer 2 ------

/*!
 Interrupt Handler for Timer/Counter 2
 */

ISR(TIMER2_COMP_vect)
{

	/* ----- TIMER ----- */
	uint32_t ticks = tickCount.u32; // increment TickCounter [176 us]
	ticks++;
	tickCount.u32 = ticks; // optimiezes volatile, because Ints are off
	
	sei(); // Interrupts ON, e.g. UART can be used parallel to RC5 ...

	/* --- RADENCODER --- */
	//bot_encoder_isr();
	
}

/*!
 * Initializes Timer 0 and starts it
 */
void timer_2_init(void)
{
	TCNT2  = 0x00;            // TIMER preload

	// If you change the Prescaler, adapt the formula for OCR2 !!! 
	// Compare Register only 8-Bit  --> opt. prescaler change
	TCCR2 |= (1 << WGM21) | (1 << CS21) | (1 << CS20); // CTC to OCR2; Prescaler to 64
	OCR2 = ((XTAL / 64 / TIMER_2_CLOCK) - 1);	// Timer2A 16Mhz/64/5619= 44,49...
	TIMSK  |= (1 << OCIE2);	// TIMER2 Output Compare Match A Interrupt ON

	sei();                  // enable interrupts
}


