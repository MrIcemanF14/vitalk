#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "vito_parameter.h"
#include "vito_io.h"
#include "fehlerliste.h"

// Das prologue() Makro wird verwendet, um den Anfang der
// (meisten) Parameterfunktionen zu bauen:
#define prologue( address, length, cache_time, bufferlen ) \
   static time_t old_time = 0; \
   static char value_str[(bufferlen)]=""; \
   time_t new_time; \
   uint8_t content[30]; \
   new_time = time(NULL) / (cache_time); \
   if ( new_time > old_time ) \
   { \
     old_time = new_time; \
     if ( vito_read( (address), (length), content ) < 0 ) \
       return -1; \
     else \
     { \

// Das epilogue() Makro wird verwendet, um das Ende der
// Parameterfunktionen zu bauen:
#define epilogue() \
   } } \
   value_ptr = value_str; \
   return 0;

// Funktion zum interpretieren von 16bit Daten im content-array als
// (signed) int16_t mit LSB first, MSB last (Viessmann Data Byteorder)
// G�ltig f�r �bliche 2-byte Temperaturparameter mit 0,1�C Aufl�sung
// Dieses Format tritt am h�ufigsten auf und wird deshalb hier als
// Funktion bereitgestellt:
static void interpret_int16_t( uint8_t *content, char *valuestr )
{
  int16_t value;
  
  value = content[0];
  value += ( content[1] << 8 );

  sprintf( valuestr, "%3.2f", value / 10.0 );
}

////////////////////////// PARAMETERFUNKTIONEN ////////////////////

int read_deviceid( char *value_ptr )
{
  prologue( 0x00f8, 2, 240, 10 )
      // Normalerweise sind die Parameter in Little Endian
      // Byteorder, aber bei der Deviceid hat sich offenbar
      // die umgekehrte Interpretation durchgesetzt:
    sprintf( value_str, "0x%4x", (content[0] << 8) + content[1] );
  epilogue()
}

int read_mode( char *value_ptr )
{
  prologue( 0x2323, 1, 6, 5 )
    sprintf( value_str, "%u", content[0] );
  epilogue()
}

int write_mode( char *value_str )
{
  uint8_t content[10];
  int mode;
  
  mode = atoi( value_str );
  // Dauernd reduziert und dauernd normal unterst�tzt meine Vitodens offenbar nicht:
  if ( mode < 0 || mode > 2 )
    {
      fprintf( stderr, "Illegal Mode!\n");
      return -1;
    }
  
  content[0] = mode & 0xff; // unn�tig, aber deutlicher
  return vito_write(0x2323, 1, content);
}

int read_mode_text( char *value_ptr )
{
  static char value_str[50] = "";
  char mode[10];
  
  if ( read_mode( mode ) < 0 )
    return -1;
  
  switch ( atoi(mode) )
	{
	case 0: strcpy( value_str, "Abschaltbetrieb" );
	  break;
	case 1: strcpy( value_str, "Nur Warmwasser" );
	  break;
	case 2: strcpy( value_str, "Heizen und Warmwasser" );
	  break;
	default: sprintf( value_str, "UNKNOWN: %u", content[0] );
	  break;
	}
  
  value_ptr = value_str;
  return 0;
}

// Das Auslesen der Fehlerliste ist etwas konfus, denn in dem
// vito.xml file von ceteris paribus werden 9 byte pro Eintrag
// gelesen. Ich sehe den Sinn aber nicht? Ich muss mal noch beobachten was passiert
// wenn mehr als ein Eintrag in der Fehlerliste steht!
int read_error_history( char *value_ptr )
{
  static time_t old_time = 0;
  static char value_str[80] = "";
  time_t new_time;
  uint8_t content[15];
  int address, i;
  
  new_time = time(NULL) / 60;
  if ( new_time > old_time )
    {
      old_time = new_time;
      
      // Die 10 Fehlermeldungen numerisch ins content Array lesen:
      i = 0;
      for ( address = 0x7507; address <= 0x7558; address += 9 )
	if ( vito_read( address, 1, &content[i++] ) < 0 )
	  return -1;
   
      // String der Form 0,0,0,0,0,0,0,0,0,0 basteln:
      value_str[0] = '\0';
      for ( i = 0, i <= 9 ; i++ )
	sprintf( value_str,"%s%u%c", value_str, content[i], i == 9 ? '\n' : ',' );
    }
  
  value_ptr = value_str;
  return 0;
}

int read_error_history_text( char *value_ptr )
{
  static char value_str[80*10] = "";
  char errors[80] = "";
  char *error[15];
  int i;
  
  if ( read_error_history( errors ) < 0 )
    return -1;
  
  // parse string from read_error_history():
  error[0] = strtok( errors, "," )
  for ( i=1; i<=9; i++ )
    error[i] = strtok( NULL, "," );
  
  // String mit 10 Zeilen der Fehlerbeschreibungen basteln:
  value_str[0]='\0';
  for ( i=0; i<=9; i++ )
    sprintf( value_str, "%s0x%02x %s                   \n", 
		       value_str, atoi(error[i]), fehlerliste[atoi(error[i])] );
  value_ptr = value_str;
  return 0;
}


//////////////////// KESSEL
char * read_K_abgas_temp( void )
{
  prologue( 0x0808, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_K_ist_temp( void )
{
  prologue( 0x0802, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_K_istTP_temp( void )
{
  prologue( 0x0810, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_K_soll_temp( void )
{
  prologue( 0x555a, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

//////////////////// WARMWASSER
char * read_WW_soll_temp( void)
{
  prologue( 0x6300, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}

int write_WW_soll_temp( int temp )
{
  uint8_t content[30];
  
  if ( temp < 5 || temp > 60 )
    {
      fprintf( stderr, "WW_soll_temp: range exceeded!\n");
      return -1;
    }
  
  content[0] = temp & 0xff; // unn�tig, aber deutlicher
  return vito_write(0x6300, 1, content);
}

char * read_WW_offset( void )
{
  prologue( 0x6760, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}

char * read_WW_istTP_temp( void )
{
  prologue( 0x0812, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_WW_ist_temp( void )
{
  prologue( 0x0804, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}


/////////////////// AUSSENTEMPERATUR
char * read_outdoor_TP_temp( void )
{
  prologue( 0x5525, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_outdoor_smooth_temp( void )
{
  prologue( 0x5527, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_outdoor_temp( void )
{
  prologue( 0x0800, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

/////////////////// BRENNER
char * read_starts( void )
{
  prologue( 0x088A, 4 )
    {
      unsigned int value;
  
      value = content[0] + (content[1] << 8) + (content[2] << 16) + (content[3] << 24);
      sprintf( valuestr, "%u", value );
    }
  epilogue()
}

char * read_runtime( void )
{
  prologue( 0x0886, 4 )
    {
      unsigned int value;
  
      value = content[0] + (content[1] << 8) + (content[2] << 16) + (content[3] << 24);
      sprintf( valuestr, "%u", value );
    }
  epilogue()
}

char * read_runtime_h( void )
{
  prologue( 0x0886, 4 )
    {
      float value;
  
      value = content[0] + (content[1] << 8) + (content[2] << 16) + (content[3] << 24);
      value = value / 3600;
      sprintf( valuestr, "%06.1f", value );
    }
  epilogue()
}

char * read_power( void )
{
  prologue( 0xa38f, 1 )
    sprintf( valuestr, "%3.1f", content[0] / 2.0 );
  epilogue()
}

/////////////////// HYDRAULIK
char * read_ventil_numeric( void )
{
  prologue( 0x0a10, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}

char * read_ventil( void )
{
  prologue( 0x0a10, 1 )
    {
      switch (content[0])
	{
	case 0: strcpy( valuestr, "undefiniert" );
	  break;
	case 1: strcpy( valuestr, "Heizkreis" );
	  break;
	case 2: strcpy( valuestr, "Mittelstellung" );
	  break;
	case 3: strcpy( valuestr, "Warmwasserbereitung" );
	  break;
	default: sprintf( valuestr, "UNKNOWN: %u", content[0] );
	  break;
	}
    }
  epilogue()
}

char * read_pump_power( void )
{
  prologue( 0x0a3c, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}

char * read_flow( void )
{
  prologue( 0x0c24, 2 )
    {    
      unsigned int value;
  
      value = content[0] + (content[1] << 8);
      sprintf( valuestr, "%u", value );
    }
  epilogue()
}
    
/////////////////// HEIZKREIS
char * read_VL_soll_temp( void )
{
  prologue( 0x2544, 2 )
    interpret_int16_t( content, valuestr );
  epilogue()
}

char * read_raum_soll_temp( void)
{
  prologue( 0x2306, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}

int write_raum_soll_temp( int temp )
{
  uint8_t content[30];
  
  if ( temp < 10 || temp > 30 )
    {
      fprintf( stderr, "Raum_soll_temp: range exceeded!\n");
      return -1;
    }
  
  content[0] = temp & 0xff; // unn�tig, aber deutlicher
  return vito_write(0x2306, 1, content);
}

char * read_red_raum_soll_temp( void)
{
  prologue( 0x2307, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}

int write_red_raum_soll_temp( int temp )
{
  uint8_t content[30];
  
  if ( temp < 10 || temp > 30 )
    {
      fprintf( stderr, "Raum_soll_temp: range exceeded!\n");
      return -1;
    }
  
  content[0] = temp & 0xff; // unn�tig, aber deutlicher
  return vito_write(0x2307, 1, content);
}

char * read_neigung( void )
{
  prologue( 0x27d3, 1 )
    sprintf( valuestr, "%2.1f", content[0] / 10.0 );
  epilogue()
}

char * read_niveau( void )
{
  prologue( 0x27d4, 1 )
    sprintf( valuestr, "%u", content[0] );
  epilogue()
}


//////////////////////////////////////////////////////////////////////////

// Struktur zur Verwaltung der Parameter
struct s_parameter {
  char *p_name;         // Parameter Kurzname
  char *p_description;  // Beschreibung des Parameters
  char *p_einheit;      // Einheit (String)
  int *p_class;         // Parameterklasse, siehe #define oben
  int (*f_read) (char *valuestr);   // Funktion zum lesen aus der Vitodens
  int (*f_write) (char *valuestr);  // Funktion zum Schreiben in die Vitodens
};

const static struct s_parameter parameter_liste[] = {
  { "deviceid", "Geraeteidentifikation", "", P_GENERAL, &read_deviceid, NULL },
  { "mode", "Betriebsmodus (numerisch)", "", P_GENERAL, &read_mode, &write_mode },
  { "mode_text", "Betriebsmodus (text)", "", P_GENERAL, &read_mode_text, NULL },
  { "errors", "Error History (numerisch)", "", P_GENERAL, &read_errors, NULL },
  { "errors_text", "Error History (text)", "", P_GENERAL, &read_errors_text, NULL },
  { "outdoor_temp", "Aussentemperatur", "�C", P_GENERAL, &read_outdoor_temp, NULL },
  { "outdoor_temp_tp", "Aussentemperatur Tiefpass", "�C", P_GENERAL, &read_outdoor_temp_tp, NULL },
  { "outdoor_temp_smooth", "Aussentemperatur Ged�mpft", "�C", P_GENERAL, &read_outdoor_temp_smooth, NULL },
  { "k_ist_temp", "Kessel Ist Temperatur", "�C", P_KESSEL, &read_k_ist_temp, NULL },
  { "k_ist_temp_tp", "Kessel Ist T. nach Tiefpass", "�C", P_KESSEL, &read_k_ist_temp_tp, NULL },
  { "k_soll_temp", "Kessel Soll Temperatur", "�C", P_KESSEL, &read_k_soll_temp, NULL },
  { "k_abgas_temp", "Kessel Abgastemperatur", "�C", P_KESSEL, &read_abgas_temp, NULL },
  { "ww_soll_temp", "Warmwasser Soll Temperatur", "�C", P_WARMWASSER, &read_ww_soll_temp, &write_ww_soll_temp },
  { "ww_ist_temp", "Warmwasser Ist Temperatur", "�C", P_WARMWASSER, &read_ww_ist_temp, NULL },
  { "ww_ist_temp_tp", "Warmwasser Ist Temp. Tiefpass", "�C", P_WARMWASSER, &read_ww_ist_temp_tp, NULL },
  { "ww_offset", "Offset Kessel/WW Soll", "K", P_WARMWASSER, &read_ww_offset, NULL },
  { "starts", "Brennerstarts", "", P_BRENNER, &read_starts, NULL },
  { "starts_h", "Brennerlaufzeit", "h", P_BRENNER, &read_runtime_h, NULL },
  { "runtime", "Brennerlaufzeit", "s", P_BRENNER, &read_runtime, NULL },
  { "power", "Brennerleistung", "%%", P_BRENNER, &read_power, NULL },
  { "ventil", "Ventilstellung", "", P_HYDRAULIK, &read_ventil, NULL },
  { "ventil_text", "Ventilstellung", "", P_HYDRAULIK, &read_ventil_text, NULL },
  { "pump_power", "Pumpenleistung", "%%", P_HYDRAULIK, &read_pump_power, NULL },
  { "flow", "Volumenstrom", "l/h", P_HYDRAULIK, &read_flow, NULL },
  { "vl_soll_temp", "Vorlauf Solltemperatur", "�C", P_HEIZKREIS, &read_vl_soll_temp, NULL },
  { "raum_soll_temp", "Raum Solltemperatur", "�C", P_HEIZKREIS, &read_raum_soll_temp, &write_raum_soll_temp },
  { "red_raum_soll_temp", "Reduzierte Raum Solltemperatur", "�C", P_HEIZKREIS, &read_red_raum_soll_temp, &write_red_raum_soll_temp },
  { "niveau", "Heizkurve Niveau", "K", P_HEIZKREIS, &read_niveau, NULL },
  { "neigung", "Heizkurve Neigung", "", P_HEIZKREIS, &read_neigung, NULL },
  { "", "", 0, NULL, NULL }
};

  