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
 * @file 	nvt-control.h
 * @brief 	Global switches for activation of implemented features
 * 
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
 */

#ifndef CONFIGURE_H_
#define CONFIGURE_H_

#define F_CPU		16000000UL    	/*!< Crystal frequency in Hz */
#define XTAL		F_CPU

#define BAUDRATE 	115200



/******************************************************************
* Module switches, to make code smaller if features are not needed
*******************************************************************/
/*!< BASIC TIMING AND COMMUNICATION */
#define TIME_AVAILABLE			/*!< Is there a system time in s and ms? */
#define DISPLAY_AVAILABLE		/*!< Display for local control and debugging */
//#define WELCOME_AVAILABLE		/*!< Show company welcome message */	
#define UART_AVAILABLE			/*!< Serial Communication */

/*!< LOGGING AUSGANG - nur 1 gleichzeitig moeglich */
//#define LOG_CTSIM_AVAILABLE		/*!< Logging zum ct-Sim (PC und MCU) */
//#define LOG_DISPLAY_AVAILABLE		/*!< Logging ueber das LCD-Display (PC und MCU) */
#define LOG_UART_AVAILABLE			/*!< Logging ueber UART (NUR fuer MCU) */
//#define LOG_STDOUT_AVAILABLE 		/*!< Logging auf die Konsole (NUR fuer PC) */
//#define USE_MINILOG				/*!< schaltet fuer MCU auf schlankes Logging um (nur in Verbindung mit Log2Sim) */



//#define I2C_AVAILABLE				/*!< TWI-Schnittstelle (I2C) nutzen */
//#define SPI_AVAILABLE				/*!< SPI Schnittstelle aktivieren? */

/************************************************************
* Some Dependencies!!!
************************************************************/

#ifndef DISPLAY_AVAILABLE
	#undef WELCOME_AVAILABLE
#endif


#ifdef LOG_UART_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif
#ifdef LOG_CTSIM_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif
#ifdef LOG_DISPLAY_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif
#ifdef LOG_STDOUT_AVAILABLE
	#define LOG_AVAILABLE	/*!< LOG aktiv? */
#endif


#ifdef LOG_AVAILABLE
	#ifndef LOG_CTSIM_AVAILABLE
		#undef USE_MINILOG
	#endif


	/* Mit Bot zu PC Kommunikation auf dem MCU gibts kein Logging ueber UART.
	 * Ohne gibts keine Kommunikation ueber ct-Sim.
	 */
	#undef LOG_STDOUT_AVAILABLE		/*!< MCU hat kein STDOUT */
	#ifdef BOT_2_PC_AVAILABLE
		#undef LOG_UART_AVAILABLE
	#else
		#undef LOG_CTSIM_AVAILABLE
	#endif


	/* Ohne Display gibts auch keine Ausgaben auf diesem. */
	#ifndef DISPLAY_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
	#endif

	/* Es kann immer nur ueber eine Schnittstelle geloggt werden. */

	#ifdef LOG_UART_AVAILABLE
		#define UART_AVAILABLE			/*!< UART vorhanden? */
		#undef LOG_CTSIM_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
	#endif

	#ifdef LOG_CTSIM_AVAILABLE
		#undef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
	#endif

	#ifdef LOG_DISPLAY_AVAILABLE
		#undef LOG_STDOUT_AVAILABLE
	#endif

	// Wenn keine sinnvolle Log-Option mehr uebrig, loggen wir auch nicht
	#ifndef LOG_CTSIM_AVAILABLE
		#ifndef LOG_DISPLAY_AVAILABLE
			#ifndef LOG_UART_AVAILABLE
				#ifndef LOG_STDOUT_AVAILABLE
					#undef LOG_AVAILABLE
				#endif
			#endif
		#endif
	#endif

#endif



#include "global.h"




#endif /* CONFIGURE_H_ */
