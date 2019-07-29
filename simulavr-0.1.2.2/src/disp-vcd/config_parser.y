/*
 * $Id: config_parser.y,v 1.2 2002/11/17 00:29:20 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr-vcd - A vcd file writer as display process for simulavr.
 * Copyright (C) 2002  Carsten Beth
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************
 */

%{

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vcd.h"

extern void config_scanner_init( FILE *infile );
extern int config_lex( void );

void config_error( char *s );
int parse_config( FILE *infile );

extern char *config_text;
int          errorFound;

%}

%token CP_FREQUENCY
%token CP_VCD_FILE
%token CP_TRACE

%token CP_SRAM
%token CP_EEPROM
%token CP_PROGMEM
%token CP_REG
%token CP_FREG
%token CP_IOREG
%token CP_SP
%token CP_PC

%token CP_NAME
%token CP_FILENAME
%token CP_NUMBER

%right '[' ']' ':' '=' '\n'

%union
{
    int  integer;
    char *string;
    struct
    {
        int from;
        int to;
    } t_range;
};

%type  <t_range> range
%type  <string>  name
%type  <string>  filename
%type  <integer> number

/* Grammar follows */

%%

conf_file:
    frequency_def vcd_file_def trace_defs;


frequency_def:
    CP_FREQUENCY '=' number '\n'
    {
        vcd_set_frequency( $3 );
    };

vcd_file_def:
    CP_VCD_FILE '=' name '\n'
    {
        vcd_set_file_name( $3 );
    }
    | CP_VCD_FILE '=' filename '\n'
    {
        vcd_set_file_name( $3 );
    };

trace_defs:
  /* empty */
  | '\n'
  | trace_defs CP_TRACE trace_def '\n'
  | error '\n' { yyerrok; };

trace_def:
  CP_SRAM range
    {
        int i;
        for( i = $2.from; i <= $2.to; i++ )
            vcd_trace_sram( i );
    }
  | CP_EEPROM range
  | CP_PROGMEM range
  | CP_REG range
    {
        int i;
        for( i = $2.from; i <= $2.to; i++ )
            vcd_trace_reg( i );
    }
  | CP_REG name
    {
        free( $2 );
    }
  | CP_IOREG range
    {
        int i;
        for( i = $2.from; i <= $2.to; i++ )
            vcd_trace_io_reg( NULL, i );
    }
  | CP_IOREG name
    {
        vcd_trace_io_reg( $2, -1 );
        free( $2 );
    }
  | CP_SP
    {
        vcd_trace_sp();
    }
  | CP_PC
    {
        vcd_trace_pc();
    };

range:
    number
    {
        $$.from = $1;
        $$.to   = $1;
    }
  | '[' number ':' number ']'
    {
        $$.from = $2;
        $$.to   = $4;
    };

name:
    CP_NAME
    {
        $$ = (char *)malloc( strlen( config_text ) + 1 );
        strcpy( $$, config_text );
    };

filename:
    CP_FILENAME
    {
        $$ = (char *)malloc( strlen( config_text ) + 1 );
        strcpy( $$, config_text );
    };

number:
    CP_NUMBER
    {
        $$ = atoi( config_text );
    };

/* End of grammar */

%%

extern int config_get_line (void);

/*************************************************************************
 * Funktion    : mapping_error() gibt den als Parameter übergebenen String
 *               auf dem Standard-Fehlerkanal aus. Diese Funktion wird von 
 *               mapping_parse() aufgerufen, wenn ein ihr ein Fehler gefunden 
 *               wurde.
 * Parameter   : s - Auszugebender Fehlertext
 * Vorbeding.  : -
 * Nachbeding. : -
 * Return      : -
 *************************************************************************/
void config_error( char *s )
{
  errorFound = 1;
  fprintf( stderr, "line %i: %s\n", config_get_line(), s );
}

/*************************************************************************
 * Funktion    : 
 * Parameter   : 
 * Vorbeding.  : -
 * Nachbeding. : 
 * Return      : 
 *************************************************************************/
int parse_config( FILE *infile )
{
    int rv;

    errorFound = 0;

    /* Scanner iniitialization. */
    config_scanner_init( infile );

    /* Parse the data. */
    rv = config_parse();
    if ( errorFound || (rv != 0) )
        return( 0 );

    return( 1 );
}

