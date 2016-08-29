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
 * @file timer.h
 *
 * @author Harald W. Leschner (DK6YF)
 * @date 23.08.2016
 *
 * @brief AVR timer driver routines
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 *
 * @see http://www.stack.nl/~dimitri/doxygen/docblocks.html
 * @see http://www.stack.nl/~dimitri/doxygen/commands.html
 */




#ifndef TIMER_H_
#define TIMER_H_

#include "config.h"

#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "timer.h"

#include "log.h"
/*!
 * Makro to calculate Ticks in ms
 * (ms / ticks opt. cast to uint32, if greater values)
 */
#define TICKS_TO_MS(ticks)	((ticks) * (TIMER_STEPS / 8) / (1000 / 8))

/*!
 * Makro to calculate ms in Ticks
 * (ms / ticks opt. cast to uint32, if greater values)
 */
#define MS_TO_TICKS(ms)		((ms) * (1000 / 8) / (TIMER_STEPS / 8))

/*!
 * Makro to calculate ms in Ticks
 * (ms / ticks opt. cast to uint32, if greater values)
 */
#define S_TO_TICKS(s)		MS_TO_TICKS(ms) * 1000

#ifdef TIME_AVAILABLE
/*!
 * This function returnes the system time in parts of milliseconds.
 * @return	Milliseconds of system time
 */
uint16_t timer_get_ms(void);

/*!
 * Diese Funktion liefert den Sekundenanteil der Systemzeit zurueck.
 * @return Sekunden der Systemzeit
 */
uint16_t timer_get_s(void);

/*!
 * Returns seconds passed since old_s, old_ms
 * @param old_s		old value for the seconds
 * @param old_ms	old value for the milliseconds
 */
uint16_t timer_get_ms_since(uint16_t old_s, uint16_t old_ms);
#endif // TIME_AVAILABLE



/*! Union fuer TickCount in 8, 16 und 32 Bit */
typedef union {
	uint32_t u32; /*!< 32 Bit Integer */
	uint16_t u16; /*!< 16 Bit Integer */
	uint8_t u8; /*!< 8 Bit Integer */
} tickCount_t;

extern volatile tickCount_t tickCount; /*!< ein Tick alle 176 us */

/*!
 * Setzt die Systemzeit zurueck auf 0
 */
static inline void timer_reset(void)
{
	uint8_t sreg = SREG;
	cli();
	tickCount.u32 = 0;
	SREG = sreg;
}

#define TIMER_GET_TICKCOUNT_8 tickCount.u8 /*!< Systemzeit [176 us] in 8 Bit */
#define TIMER_GET_TICKCOUNT_16 timer_get_tickcount_16() /*!< Systemzeit [176 us] in 16 Bit */
#define TIMER_GET_TICKCOUNT_32 timer_get_tickcount_32() /*!< Systemzeit [176 us] in 32 Bit */

/*!
 * Liefert die unteren 16 Bit der Systemzeit zurueck
 * @return	Ticks [176 us]
 */
static inline
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
uint16_t timer_get_tickcount_16(void)
{
	uint8_t sreg = SREG;
	cli();
	uint16_t ticks = tickCount.u16;
	SREG = sreg;
	return ticks;
}

/*!
 * Liefert die vollen 32 Bit der Systemzeit zurueck
 * @return	Ticks [176 us]
 */
static inline
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
uint32_t timer_get_tickcount_32(void)
{
	uint8_t sreg = SREG;
	cli();
	uint32_t ticks = tickCount.u32;
	SREG = sreg;
	return ticks;
}


// Values for TIMER_X_CLOCK are in ** Hz **

/*!
 * Frequency Timer 2 in Hz
 */
#define TIMER_2_CLOCK	5619	// Currently used for RC5

/*!
 * Microseconds passed since two Timer calls
 */
#define TIMER_STEPS 	176




/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * 32-Bit Version, fuer Code, der (teilweise) seltener als alle 11 s aufgerufen wird.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 *
 * Die Funktion aktualisiert den Timestamp, der die alte Zeit zum Vergleich speichert, automatisch,
 * falls ms Millisekunden vergangen sind.
 * Man verwendet sie z.B. wie folgt:
 * static uint32_t old_time;
 * ...
 * if (timer_ms_passed(&old_time, 50)) {
 * 		// wird alle 50 ms ausgefuehrt //
 * }
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed_32(uint32_t *old_ticks, uint32_t ms)
{
	uint32_t ticks = TIMER_GET_TICKCOUNT_32;
	if ((uint32_t)(ticks - *old_ticks) > MS_TO_TICKS(ms)) {
		*old_ticks = ticks;
		return True;
	}
	return False;
}

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * Siehe auch timer_ms_passed_32()
 * 16-Bit Version, fuer Code, der alle 11 s oder oefter ausgefuehrt werden soll.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed_16(uint16_t *old_ticks, uint32_t ms)
{
	uint16_t ticks = TIMER_GET_TICKCOUNT_16;
	if ((uint16_t)(ticks - *old_ticks) > MS_TO_TICKS(ms)) {
		*old_ticks = ticks;
		return True;
	}
	return False;
}

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * Siehe auch timer_ms_passed_32()
 * 8-Bit Version, fuer Code, der alle 40 ms oder oefter ausgefuehrt werden soll.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed_8(uint8_t *old_ticks, uint16_t ms)
{
	uint8_t ticks = TIMER_GET_TICKCOUNT_8;
	if ((uint8_t)(ticks - *old_ticks) > MS_TO_TICKS(ms)) {
		*old_ticks = ticks;
		return True;
	}
	return False;
}

/*!
 * Prueft, ob seit dem letzten Aufruf mindestens ms Millisekunden vergangen sind.
 * Siehe auch timer_ms_passed_32()
 * 32-Bit Version, fuer Code, der (teilweise) seltener als alle 11 s aufgerufen wird.
 * @param old_ticks		Zeiger auf eine Variable, die einen Timestamp speichern kann
 * @param ms			Zeit in ms, die vergangen sein muss, damit True geliefert wird
 * @return				True oder False
 */
static inline uint8_t
#ifndef DOXYGEN
__attribute__((always_inline))
#endif
timer_ms_passed(uint32_t *old_ticks, uint32_t ms)
{
	return timer_ms_passed_32(old_ticks, ms);
}




/*!
 * Initializes Timer 0 and starts it
 */
void timer_2_init(void);

/*!
 * Measures the timelapse executing the __code 
 * and outputs it on the LOG or Display 
 * @param __code	Code to be measured
 */
#define TIMER_MEASURE_TIME(__code) {			\
	uint32_t start = TIMER_GET_TICKCOUNT_32;	\
	{ __code; }									\
	uint32_t end = TIMER_GET_TICKCOUNT_32;		\
	uint16_t diff = end - start;				\
	LOG_DEBUG("%u Ticks", diff);				\
	display_cursor(4, 1);						\
	display_printf("%4u Ticks", diff);			\
}

				
	
	
	
#endif /* TIMER_H_ */
