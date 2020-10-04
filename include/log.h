 /*! 
 * @file 	log.h
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
 * <pre>
 * Die Logausgaben werden generell mit der Definition von LOG_AVAILABLE eingeschaltet
 * und sind ansonsten nicht aktiv.
 * 
 * Loggings auf dem PC:
 * --------------------
 * Hier stehen drei Arten der Ausgabeschnittstellen zur Verfuegung.
 * 1. Logging ueber ct-Sim:		LOG_CTSIM_AVAILABLE muss definiert sein.
 * 2. Logging ueber Display:	LOG_DISPLAY_AVAILABLE muss definiert sein, sowie DISPLAY_AVAILABLE.
 * 3. Logging ueber Konsole:  	Es muss LOG_STDOUT_AVAILABLE definiert sein.
 * 
 * LOG_UART_AVAILABLE steht auf dem PC nicht zur Verfuegung.
 * 
 * Loggings auf dem MCU:
 * ---------------------
 * Hier stehen drei Arten der Ausgabeschnittstellen zur Verfuegung.
 * 1. Logging ueber UART:		LOG_UART_AVAILABLE muss definiert sein.
 * 								Es darf BOT_2_PC_AVAILABLE nicht definiert sein, da ansonsten
 * 								diese Kommunikation ueber den UART laeuft.
 * 2. Logging ueber ct-Sim:		LOG_CTSIM_AVAILABLE muss definiert sein.
 * 								BOT_2_PC_AVAILABLE muss zusaetzlich definiert sein.
 * 3. Logging ueber Display:	LOG_DISPLAY_AVAILABLE muss definiert sein, sowie DISPLAY_AVAILABLE.
 * 4. Logging in txt auf MMC:	MMC_AVAILABLE und MMC_VM_AVAILABLE muessen an sein.
 * </pre>
 * 
 * Alternativ schlankere Variante fuer MCU und CTSIM, indem man USE_MINILOG aktiviert. 
 * Das spart viel Platz in Flash und RAM.
 * 
 * @author	H9-Laboratory Ltd. (office@h9l.net)
 * @date 	18.10.2008
 */

#ifndef LOG_H_
#define LOG_H_

#include "config.h"
#include <avr/pgmspace.h>


#ifdef LOG_AVAILABLE
#include <stdlib.h>


/*! Dieser Typ definiert die Typen der Log-Ausgaben. */
typedef enum {
	LOG_TYPE_DEBUG = 0,	/*!< Allgemeines Debugging */
	LOG_TYPE_INFO,		/*!< Allgemeine Informationen */
	LOG_TYPE_WARN,		/*!< Auftreten einer unerwarteten Situation */
	LOG_TYPE_ERROR,		/*!< Fehler aufgetreten */
	LOG_TYPE_FATAL		/*!< Kritischer Fehler */
} LOG_TYPE;

/*!
 * Allgemeines Debugging (Methode DiesUndDas wurde mit Parameter SoUndSo 
 * aufgerufen ...)
 */
#define LOG_DEBUG(format, args...){	static const char file[] PROGMEM = __FILE__;		\
									log_flash_begin(file, __LINE__, LOG_TYPE_DEBUG);	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## args);					\
									log_end();											\
}

/*!
 * Allgemeine Informationen (Programm gestartet, Programm beendet, Verbindung 
 * zu Host Foo aufgebaut, Verarbeitung dauerte SoUndSoviel Sekunden ...)
 */
#define LOG_INFO(format, args...){	static const char file[] PROGMEM = __FILE__;		\
									log_flash_begin(file, __LINE__, LOG_TYPE_INFO); 	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## args);					\
									log_end();											\
}

/*!
 * Auftreten einer unerwarteten Situation.
 */
#define LOG_WARN(format, args...){	static const char file[] PROGMEM = __FILE__;		\
									log_flash_begin(file, __LINE__, LOG_TYPE_WARN); 	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## args);					\
									log_end();											\
}

/*!
 * Fehler aufgetreten, Bearbeitung wurde alternativ fortgesetzt.
 */
#define LOG_ERROR(format, args...){	static const char file[] PROGMEM = __FILE__;		\
									log_flash_begin(file, __LINE__, LOG_TYPE_ERROR); 	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## args);					\
									log_end();											\
}

/*!
 * Kritischer Fehler, Programmabbruch.
 */
#define LOG_FATAL(format, args...){	static const char file[] PROGMEM = __FILE__;		\
									log_flash_begin(file, __LINE__, LOG_TYPE_FATAL); 	\
									static const char data[] PROGMEM = format;			\
									log_flash_printf(data, ## args);					\
									log_end();											\
}

/*!
 * Schreibt Angaben ueber Datei, Zeilennummer und den Log-Typ in den Puffer.
 * @param filename Dateiname
 * @param line Zeilennummer
 * @param log_type Log-Typ
 */
void log_flash_begin(const char *filename, unsigned int line, LOG_TYPE log_type);

/*!
 * Schreibt die eigentliche Ausgabeinformation in den Puffer.
 * @param format Format-String
 */
void log_flash_printf(const char *format, ...);

/*!
 * Gibt den Puffer entsprechend aus.
 */
void log_end(void);


#ifdef LOG_DISPLAY_AVAILABLE	
/*!
 * @brief	Display-Handler fuer das Logging
 */
void log_display(void);
#endif	// LOG_DISPLAY_AVAILABLE




#else	// LOG_AVAILABLE

#define LOG_DEBUG(format, args...)
#define LOG_INFO(format, args...)
#define LOG_WARN(format, args...)
#define LOG_ERROR(format, args...)
#define LOG_FATAL(format, args...)
#endif	// LOG_AVAILABLE
#endif	// LOG_H_

