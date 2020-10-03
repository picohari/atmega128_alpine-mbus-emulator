/*! 
 * @file 	log.c
 * 
 * @brief 	Routinen zum Loggen von Informationen. Es sollten ausschliesslich nur
 * die Log-Makros: LOG_DEBUG(), LOG_INFO(), LOG_WARN(), LOG_ERROR() und LOG_FATAL()
 * verwendet werden.
 * Eine Ausgabe kann wie folgt erzeugt werden:
 * LOG_DEBUG("Hallo Welt!");
 * LOG_INFO("Wert x=%d", x);
 * Bei den Ausgaben kann auf ein Line Feed '\n' am Ende des Strings verzichtet werden, 
 * da dies automatisch angehaengt hinzugefuegt wird.
 * Die frueher noetigen Doppelklammern sind nicht mehr noetig, einfach normale Klammern
 * verwenden, siehe Bsp. oben. 
 * (Die Doppelklammern funktionieren nicht mit Var-Arg-Makros, die wir aber brauchen, da 
 * nun fuer MCU alle Strings im Flash belassen werden sollen, das spart viel RAM :-) )
 * 
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
 */

//TODO: Umblaetterfunktion fuer mehr als 4 Logausgaben bei LOG_DISPLAY_AVAILABLE => Keyhandler aehnlich wie bei der Verhaltensanzeige

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "hd44780.h"
#include "uart.h"


#ifdef LOG_AVAILABLE
#ifndef USE_MINILOG

#ifdef LOG_DISPLAY_AVAILABLE
	/*! Groesse des Puffers fuer die Logausgaben bei Verwendung des LCD-Displays. */
	#define LOG_BUFFER_SIZE		(DISPLAY_LENGTH + 1)
#else
	/*! Groesse des Puffers fuer die Logausgaben ueber UART und ueber TCP/IP. */
	#define LOG_BUFFER_SIZE		200
#endif	/* LOG_DISPLAY_AVAILABLE */


/*! Schuetzt den Ausgabepuffer */
#define LOCK()
/*! Hebt den Schutz fuer den Ausgabepuffer wieder auf */
#define UNLOCK()


/* Log-Typen als String, auf MCU im Flash */
static const char debug_str[] PROGMEM = "- DEBUG -";
static const char info_str[] PROGMEM = "- INFO -";
static const char warn_str[] PROGMEM = "- WARNING -";
static const char error_str[] PROGMEM = "- ERROR -";
static const char fatal_str[] PROGMEM = "- FATAL -";

#if 0
/*!
 * Liefert den Log-Typ als String (auf MCU als Flash-Referenz).
 * @param log_type Log-Typ
 * @return char*
 */
static const char *log_get_type_str(LOG_TYPE log_type);
#endif

/*! Puffer fuer das Zusammenstellen einer Logausgabe */
static char log_buffer[LOG_BUFFER_SIZE];


#ifdef LOG_DISPLAY_AVAILABLE
	/*! Zeile in der die naechste Logausgabe erfolgt. */
	static uint16 log_line = 1;
	static char screen_output[4][LOG_BUFFER_SIZE];	// Puffer, damit mehr als eine Zeile pro Hauptschleifendurchlauf geloggt werden kann
#endif	/* LOG_DISPLAY_AVAILABLE */

/*!
 * @brief	Kopiert einen String wortweise vom Flash ins Ram
 * @param flash	Zeiger auf einen String im FLASH
 * @param ram	Zeiger auf den Zielpuffer im RAM
 * @param n		Anzahl der zu kopierenden WORTE
 * Es werden maximal n Worte kopiert, ist der String schon zuvor nullterminiert, 
 * wird bei Auftreten von 0 abgebrochen. 
 */
static void get_str_from_flash(const char *flash, char *ram, uint8_t n)
{
	uint8_t i;
	/* Zeichen des Strings wortweise aus dem Flash holen */
	uint16_t *p_ram  = (uint16_t *)ram;
	uint16_t *p_flash = (uint16_t *)flash;
	for (i = 0; i < n; i++) {
		uint16_t tmp = pgm_read_word(p_flash++);
		*(p_ram++) = tmp;
		if ((uint8_t)tmp == 0 || (tmp & 0xFF00) == 0)
			break;	// Stringende erreicht
	}
	*((char *)p_ram) = 0;	// evtl. haben wir ein Byte zu viel gelesen, das korrigieren wir hier	
}

/*!
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * Achtung, Mutex wird gelockt und muss explizit durch log_end() wieder
 * freigegeben werden!
 * @param filename Dateiname
 * @param line Zeilennummer
 * @param log_type Log-Typ
 */
void log_flash_begin(const char *filename, unsigned int line, LOG_TYPE log_type)
{
#if 0
	/* Ausgaben ueber das LCD-Display werden ohne Dateiname und Zeilennumer
	 * gemacht. Der Log-Typ versteckt sich im ersten Buchstaben. Durch eine
	 * die Markierung mit '>' erkennt man das letzte Logging.
	 * Nur bei Ausgaben ueber UART und an ct-Sim werden Dateiname, Zeilennummer
	 * und der Log-Typ vollstaendig ausgegeben.
	 */		 
	#ifdef LOG_DISPLAY_AVAILABLE
		LOCK();
		/* Alte Markierung loeschen */
		if (log_line == 1)
			screen_output[3][0] = ' ';
		else
			screen_output[log_line-2][0] = ' ';

		log_buffer[0] = '>';
		log_buffer[1] = pgm_read_byte(log_get_type_str(log_type) + 2);
		log_buffer[2] = ':';
		log_buffer[3] = '\0';
	#else
		/* Zeichen des Strings fuer Dateiname wortweise aus dem Flash holen */
		char flash_filen[80];
		get_str_from_flash(filename, flash_filen, 40 / 2);
		/* Zeichen des Strings fuer Typ wortweise aus dem Flash holen */
		char flash_type[12];
		get_str_from_flash(log_get_type_str(log_type), flash_type, 12 / 2);
		const char *ptr = NULL;
		
		/* Nur den Dateinamen loggen, ohne Verzeichnisangabe */
		ptr = strrchr(flash_filen, '/');
		
		if (ptr == NULL)
			ptr = flash_filen;
		else 
			ptr++;
	
		LOCK();
	
		snprintf(log_buffer, LOG_BUFFER_SIZE, "%s(%d)\t%s\t", ptr, line, flash_type);
		
	#endif
		
	return;
#endif	
}


/*!
 * Schreibt die eigentliche Ausgabeinformation (aus dem Flash) in den Puffer.
 * @param format Format
 */
void log_flash_printf(const char *format, ...)
{
	char flash_str[LOG_BUFFER_SIZE + 4];	// bissel groesser, weil die % ja noch mit drin sind
	get_str_from_flash(format, flash_str, LOG_BUFFER_SIZE / 2 + 2);	// String aus dem Flash holen
	va_list	args;
	unsigned int len = strlen(log_buffer);
	
	va_start(args, format);
	vsnprintf(&log_buffer[len], LOG_BUFFER_SIZE - len, flash_str, args);
	va_end(args);

	return;
}

/*!
 * Gibt den Puffer entsprechend aus.
 */
void log_end(void)
{

	#ifdef LOG_UART_AVAILABLE
		/* String ueber UART senden, ohne '\0'-Terminierung */
		uart_write((uint8_t *)log_buffer, strlen(log_buffer));
		/* Line feed senden */
		uart_write((uint8_t *)LINE_FEED,strlen(LINE_FEED));
	#endif	/* LOG_UART_AVAILABLE */
	
	#ifdef LOG_CTSIM_AVAILABLE
		/* Kommando an ct-Sim senden, ohne Line feed am Ende. */
		command_write_data(CMD_LOG, SUB_CMD_NORM, NULL, NULL, log_buffer);
	#endif	/* LOG_CTSIM_AVAILABLE */
	
	#ifdef LOG_DISPLAY_AVAILABLE
		/* aktuelle Log-Zeile in Ausgabepuffer kopieren */
		memcpy(screen_output[log_line - 1], log_buffer, strlen(log_buffer));
		/* Zeile weiterschalten */  
		log_line++;
		if (log_line > 4)
			log_line = 1;
	#endif	/* LOG_DISPLAY_AVAILABLE */

	/* Wenn das Logging aktiviert und keine Ausgabeschnittstelle
	 * definiert ist, dann wird auf dem PC auf die Konsole geschrieben.
	 */
	//#ifdef LOG_STDOUT_AVAILABLE
	//	printf("%s\n", log_buffer);
	//#endif	/* LOG_STDOUT_AVAILABLE */
	
	UNLOCK();
	
	return;
}

#ifdef LOG_DISPLAY_AVAILABLE
	/*!
	 * @brief	Display-Handler fuer das Logging
	 */
	void log_display(void)
	{	
		/* Strings aufs LCD-Display schreiben */
		uint8_t i;
		for (i = 0; i < 4; i++) {	// Das Display hat 4 Zeilen
			display_cursor(i + 1, 1);
			display_printf("%s", screen_output[i]);
		}
	}

#endif	// LOG_DISPLAY_AVAILABLE

#if 0
/*!
 * Liefert einen Zeiger auf den Log-Typ als String.
 * @param log_type Log-Typ
 * @return char*
 */
static const char *log_get_type_str(LOG_TYPE log_type)
{

	switch(log_type) {

	case LOG_TYPE_DEBUG:
		return debug_str;
		
	case LOG_TYPE_INFO:
		return info_str;
		
	case LOG_TYPE_WARN:
		return warn_str;
		
	case LOG_TYPE_ERROR:
		return error_str;
		
	case LOG_TYPE_FATAL:
		return fatal_str;
	}

	return debug_str;
}
#endif


#endif	// USE_MINILOG
#endif	/* LOG_AVAILABLE */
