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
 * @file hd44780.c
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

#ifdef HD44780_AVAILABLE

#include <util/delay.h>
#include "hd44780.h" 
#include "timer.h"

#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <stdint.h>


/*! Buffersize for one row in the display in bytes */
#define HD44780_BUFFER_SIZE (HD44780_LENGTH + 1)

uint8 hd44780_screen = 0; /*!< Currently active screen */


/*!
 * @brief   Transmit a command to the display
 * @param cmd The Command
 */
void hd44780_cmd(unsigned char i)
{

    PORTG &= 0b11101111;        // Instruction Select RS=0(PG4)

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
void hd44780_data(unsigned char i)
{

    PORTG |= 0b00010000;        // Instruction Select  RS=1(PG4)

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
void hd44780_clear(void)
{
    hd44780_cmd(0x01);      // Display clear, cursor home

    /* 1.52 ms waiting... */
    _delay_loop_2(F_CPU / 4000000L * 1520);
}

/*!
 * @brief     Position of the cursor
 * @param row   Row
 * @param column  Column
 */
void hd44780_cursor(uint8_t row, uint8_t column)
{
    switch (row) {
    case 1:
        hd44780_cmd(0x80 + column - 1); break;
    case 2:
        hd44780_cmd(0xc0 + column - 1); break;
    case 3:
        hd44780_cmd(0x94 + column - 1); break;
    case 4:
        hd44780_cmd(0xd4 + column - 1); break;
    default: break;
   }

}







#if 0

## Instructions table

                DB7  DB6  DB5  DB4   DB3  DB2  DB1  DB0
CLEAR_DISPLAY   0    0    0    0     0    0    0    1      0x01  
RETURN_HOME     0    0    0    0     0    0    1    —      0x02
ENTRY_MODE      0    0    0    0     0    1    I/D  S      0x04   I/D = Increment/Decrement (right/left)  S = Shift entire display  
DISPLAY_ONOFF   0    0    0    0     1    D    C    B      0x08   D = Display On/Off  C = Cursor On/Off  B = Cursor blink character
CURSOR_SHIFT    0    0    0    1     S/C  R/L  —    —      0x10   S/C = Shift Cursor move  R/L = Shift right/left
FUNCTION_SET    0    0    1    DL    N    F    —    —      0x20   Set interface
SET_CGRAM       0    1    A    A     A    A    A    A      0x40   Set CGRAM adress 
DET_DDRAM       1    A    A    A     A    A    A    A      0x80   Set DDRAM adress


## Initialization 4 bit

Must be sent at beginning for 4 bit interface:

0 0 1 1   0 0 1 1   0x33    // Initial (Set DL=1 3 Time, Reset DL=0 1 Time)
0 0 1 1   0 0 1 0   0x32    

0 0 1 0   1 0 0 0   0x28    // Function Set (DL=0 4-Bit, N=1 2 Line, F=0 5X7)
          N F - -

These can be changed during operation:

0 0 0 0   0 1 1 0   0x06    // Entry Mode Set (I/D=1 Increment, S=0 Cursor Shift)
              I S

0 0 0 0   1 1 0 0   0x0c    // Display on/off Control (Entry Display, Cursor off, Cursor not Blink)
            D C B

## Configuration options

I/D = 1: Increment,     I/D = 0: Decrement
S   = 1: Accompanies display shift

D   = 1: Display on,    D   = 0: Display off
C   = 1: Cursor on,     C   = 0: Cursor off
B   = 1: Blink on,      B   = 0: Blink off

S/C = 1: Display shift, S/C = 0: Cursor move
R/L = 1: Shift to the right
R/L = 0: Shift to the left

DL  = 1: 8 bits,         DL = 0: 4 bits
N   = 1: 2 lines,        N  = 0: 1 line
F   = 1: 5 × 10 dots,    F  = 0: 5 × 8 dots

BF  = 1: Internally operating
BF  = 0: Instructions acceptable


## Example initialization 4 bit:

15ms
000011
4.1ms
000011
100µs
000011

0 0 1 0  // Interface 4 bits long
0 0 1 0  // Interface 8 bits length
N F * *  // 2lines, 5x8

0 0 0 0  // Display off
1 0 0 0  // 
0 0 0 0  // Display clear
0 0 0 1  // 
0 0 0 0  // increment right / shift
0 1 I S  //  

#endif


/*!
 * @brief Initializes the display
 */
void hd44780_init(void)
{
    /* Prepare LCD ports */
    DDRG |= 0xFF;           // PORTG as output (has only 5 bits)
    DDRD |= 0x80;           // PORTD, Pin PD7 as output

    PORTG |= 0x00;          // PORTG all pins low
    PORTD &= 0b01111111;    // Start LCD Control   EN=0  (PD7)
    _delay_ms(15);          // Wait LCD Ready

    /* Send nibbles (with each byte we send 2 configuration nibbles, first high, then low) */
    hd44780_cmd(0x33);      // Initial (Set DL=1 3 Time, Reset DL=0 1 Time)
    hd44780_cmd(0x32);
    hd44780_cmd(0x28);      // Function Set (DL=0 4-Bit,N=1 2 Line,F=0 5X7)
    
    /* Send configuration */
    hd44780_cmd(0x0C);      // Display Control (Display on, Cursor off, Cursor not Blink)
    hd44780_cmd(0x06);      // Entry Mode Set (I/D=1 Increment, S=0 Cursor Shift)
    
    hd44780_cmd(0x01);      // Clear Display  (Clear Display, Set DD RAM Address=0)
    _delay_ms(5);           // Wait Initial Complete

    return;
}

/*!
 * @brief     Writes a string from the FLASH to the display.
 * @param format  Format, like printf
 * @param ...     Variable Argumentlist, like printf
 * @return      Number of characters written
 */
uint8_t hd44780_flash_printf(const char *format, ...)
{
    char hd44780_buf[HD44780_BUFFER_SIZE];  /*!< Bufferstring for display output */
    va_list args;

    /* Be shure that we do not get a buffer overflow... */
    va_start(args, format);
    uint8_t len = vsnprintf_P(hd44780_buf, HD44780_BUFFER_SIZE, format, args);
    va_end(args);

    /* Output until buffer is empty */
    char *ptr = hd44780_buf;
    uint8_t i;
    for (i = len; i > 0; i--) {
        hd44780_data(*ptr++);
    }

    return len;
}

#endif //HD44780_AVAILABLE