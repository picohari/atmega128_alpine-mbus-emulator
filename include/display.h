/*
 * WEHRsim Control
 *
 * Copyright (C) 2008  H9-Laboratory Ltd.
 * All rights reserved
 *
 * CREATED			:	18.10.2008
 * VERSION			: 	1.0
 * TARGET DEVICE	: 	ATMEL ATmega128
 *
 *
 */


 /*!
 * @file 	display.h
 * @brief 	LCD-Module Functions
 *
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
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