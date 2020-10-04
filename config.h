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
 * @file mbus.h
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

#ifndef CONFIGURE_H_
#define CONFIGURE_H_

/******************************************************************
* CPU clock settings, timing parameters depends on these!
*******************************************************************/
/*!< Master CLOCK */
#define F_CPU		16000000UL    	/*!< Crystal frequency in Hz */
#define XTAL		F_CPU

/*!< UART baudrate */
#define BAUDRATE 	115200



/******************************************************************
* Module switches, to make code smaller if features are not needed
*******************************************************************/
/*!< BASIC TIMING AND COMMUNICATION */
#define TIME_AVAILABLE			/*!< Is there a system time in s and ms? */
#define UART_AVAILABLE			/*!< Serial Communication */

/*!< LOGGING OUTPUT - nur 1 gleichzeitig moeglich */
#define LOG_UART_AVAILABLE			/*!< Logging ueber UART (NUR fuer MCU) */
//#define LOG_DISPLAY_AVAILABLE		/*!< Logging ueber das LCD-Display (PC und MCU) */
//#define LOG_STDOUT_AVAILABLE 		/*!< Logging auf die Konsole (NUR fuer PC) */

/*!< MORE FEATURES */
//#define WELCOME_AVAILABLE		/*!< Show company welcome message */	

/*!< HARDWARE AVAILABLE */
#define HD44780_AVAILABLE		/*!< HD44780 display for local control and debugging */

//#define SSD1306_AVAILABLE		/*!< HD44780 display for local control and debugging */
//#define SSD1306_SPI4_SUPPORT	/*!< SSD1306 display for local control and debugging */


/************************************************************
* Some Dependencies!!!
************************************************************/

#ifndef HD44780_AVAILABLE
	#undef WELCOME_AVAILABLE
#endif


#ifdef LOG_UART_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif
#ifdef LOG_DISPLAY_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif
#ifdef LOG_STDOUT_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif


#ifdef LOG_AVAILABLE

	#undef LOG_STDOUT_AVAILABLE		/*!< MCU hat kein STDOUT */

	/* Ohne Display gibts auch keine Ausgaben auf diesem. */
	#ifndef HD44780_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
	#endif

	/* Es kann immer nur ueber eine Schnittstelle geloggt werden. */
	#ifdef LOG_UART_AVAILABLE
		#define UART_AVAILABLE			/*!< UART vorhanden? */
		#undef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
	#endif

	#ifdef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
	#endif

	/* Wenn keine sinnvolle Log-Option mehr uebrig, loggen wir auch nicht */
	#ifndef LOG_DISPLAY_AVAILABLE
		#ifndef LOG_UART_AVAILABLE
			#ifndef LOG_STDOUT_AVAILABLE
				#undef LOG_AVAILABLE
			#endif
		#endif
	#endif

#endif


#include "global.h"


#endif /* CONFIGURE_H_ */
