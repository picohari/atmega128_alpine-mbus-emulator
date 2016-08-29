/****************************************************************************
 * Copyright (C) 2016 by Harald W. Leschner (DK6YF)                         *
 *                                                                          *
 * This file is part of ALPINE M-BUS Interface Control Emulator             *
 *                                                                          *
 * This program is free software you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation either version 2 of the License, or         *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program if not, write to the Free Software               *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA*
 ****************************************************************************/

/**
 * @file display.c
 *
 * @author Harald W. Leschner (DK6YF)
 * @date 23.08.2016
 *
 * @brief HD44780 LCD driver routines
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 *
 * @see http://www.stack.nl/~dimitri/doxygen/docblocks.html
 * @see http://www.stack.nl/~dimitri/doxygen/commands.html
 */




#include "config.h"

#ifdef DISPLAY_AVAILABLE

#include <util/delay.h>
#include "display.h" 
#include "timer.h"

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdint.h>


/*! Buffersize for one row in the display in bytes */
#define DISPLAY_BUFFER_SIZE (DISPLAY_LENGTH + 1)

uint8 display_screen = 0; /*!< Currently active screen */


/*!
 * @brief   Transmit a command to the display
 * @param cmd The Command
 */
void display_cmd(unsigned char i)
{

    PORTG  &= 0b11101111;       // Instruction Select RS=0(PG4)

    PORTG &= 0xF0;              // Clear old LCD Data (Bit[3..0])
    PORTG |= (i >> 4) & 0x0F;   // Strobe High Nibble Command
    PORTD |= 0b10000000;        // Enable ON    EN=1(PD7)
    _delay_loop_2(F_CPU / 4000000L * 47);
    PORTD &= 0b01111111;        // Enable OFF   EN=0(PD7)
    PORTG &= 0xF0;              // Clear old LCD Data (Bit[3..0])
    PORTG |= i & 0x0F;          // Strobe Low Nibble Command
    PORTD |= 0b10000000;        // Enable ON    EN=1(PD7)
    _delay_loop_2(F_CPU / 4000000L * 47);
    PORTD &= 0b01111111;        // Enable OFF   EN=0(PD7)
    _delay_loop_2(F_CPU / 4000000L * 47);           // Wait LCD Busy

    return;
}


/*!
 * @brief     Writes one character to the display
 * @param data  The character
 */
void display_data(unsigned char i)
{

    PORTG  |= 0b00010000;       // Instruction Select  RS=1(PG4)
    PORTG &= 0xF0;              // Clear old LCD Data (Bit[3..0])
    PORTG |= (i >> 4) & 0x0F;   // Strobe High Nibble Command
    PORTD |= 0b10000000;        // Enable ON    EN=1(PD7)
    _delay_loop_2(F_CPU / 4000000L * 47);
    PORTD &= 0b01111111;        // Enable OFF   EN=0(PD7)
    PORTG &= 0xF0;              // Clear old LCD Data (Bit[3..0])
    PORTG |= i & 0x0F;          // Strobe Low Nibble Command
    PORTD |= 0b10000000;        // Enable ON    EN=1(PD7)
    _delay_loop_2(F_CPU / 4000000L * 47);
    PORTD &= 0b01111111;        // Enable OFF   EN=0(PD7)
    _delay_loop_2(F_CPU / 4000000L * 47);       // Wait LCD Busy

    return;

}

/*!
 * @brief Delete the whole display
 */
void display_clear(void)
{
    display_cmd(0x01);      // Display clear, cursor home

    /* 1.52 ms waiting... */
    _delay_loop_2(F_CPU / 4000000L * 1520);
}

/*!
 * @brief     Position of the cursor
 * @param row   Row
 * @param column  Column
 */
void display_cursor(uint8_t row, uint8_t column)
{
    switch (row) {
    case 1:
        display_cmd (0x80 + column - 1); break;
    case 2:
        display_cmd (0xc0 + column - 1); break;
    case 3:
        display_cmd (0x94 + column - 1); break;
    case 4:
        display_cmd (0xd4 + column - 1); break;
    default: break;
   }

}

/*!
 * @brief Initializes the display
 */
void display_init(void)
{

    DDRG=0xFF;     // PORTG as output (has only 5 bits)
    DDRD=0x80;     // PORTD, Pin PD7 as output
    PORTD &= 0b01111111;    // Start LCD Control   EN=0  (PD7)

    _delay_ms(5);           // Wait LCD Ready
    display_cmd(0x33);      // Initial (Set DL=1 3 Time, Reset DL=0 1 Time)
    display_cmd(0x32);
    display_cmd(0x28);      // Function Set (DL=0 4-Bit,N=1 2 Line,F=0 5X7)
    display_cmd(0x0C);      // Display on/off Control (Entry Display,Cursor off,Cursor not Blink)
    display_cmd(0x06);      // Entry Mode Set (I/D=1 Increment,S=0 Cursor Shift)
    display_cmd(0x01);      // Clear Display  (Clear Display,Set DD RAM Address=0)
    _delay_ms(5);           // Wait Initial Complete

    return;
}

/*!
 * @brief     Writes a string from the FLASH to the display.
 * @param format  Format, like printf
 * @param ...     Variable Argumentlist, like printf
 * @return      Number of characters written
 */
uint8_t display_flash_printf(const char *format, ...)
{
    char display_buf[DISPLAY_BUFFER_SIZE];  /*!< Bufferstring for display output */
    va_list args;

    /* Be shure that we do not get a buffer overflow... */
    va_start(args, format);
    uint8_t len = vsnprintf_P(display_buf, DISPLAY_BUFFER_SIZE, format, args);
    va_end(args);

    /* Output until buffer is empty */
    char *ptr = display_buf;
    uint8_t i;
    for (i = len; i > 0; i--) {
        display_data(*ptr++);
    }

    return len;
}

#endif //DISPLAY_AVAILABLE