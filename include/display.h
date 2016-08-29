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
 * @file display.h
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




#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <avr/pgmspace.h>

#define DISPLAY_LENGTH	20	/*!< How many characters in one row? */

#define DISPLAY_SCREEN_TOGGLE	42	/*!< Screen number used for toggling */
uint8 display_screen;				/*!< Currently active screen */

/*! Buffersize for one row in the display in bytes */
#define DISPLAY_BUFFER_SIZE	(DISPLAY_LENGTH + 1)

/*!
 * @brief	Initializes the display
 */
void display_init(void);			// Initial Character LCD(4-Bit Interface)


void display_cmd(unsigned char i);


void display_data(unsigned char i);

/*!
 * @brief			Position of the cursor
 * @param row		Row
 * @param column	Column
 */
void display_cursor(uint8_t row, uint8_t column);		// Set Cursor LCD

/*!
 * @brief	Delete the whole display
 */
void display_clear(void);

/*!
 * @brief			Writes a string from the FLASH to the display.
 * @param format 	Format, like printf
 * @param ... 		Variable Argumentlist, like printf
 * @return			Number of characters written
 */
uint8_t display_flash_printf(const char *format, ...);

#define display_printf(format, args...)	display_flash_printf(PSTR(format), ## args)


#endif /*DISPLAY_H_*/