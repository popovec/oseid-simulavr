/*
 * $Id: config_scanner.l,v 1.4 2004/02/26 07:33:34 troth Exp $
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

#include "config_parser.h"

static int line;

#define YY_USE_PROTOS

%}

%option noyywrap
%option nounput

%%

"frequency" return( CP_FREQUENCY );
"vcd_file"  return( CP_VCD_FILE );
"trace"     return( CP_TRACE );

"sram"      return( CP_SRAM );
"eeprom"    return( CP_EEPROM );
"progmem"   return( CP_PROGMEM );
"reg"       return( CP_REG );
"freg"      return( CP_FREG );
"ioreg"     return( CP_IOREG );
"sp"        return( CP_SP );
"pc"        return( CP_PC );

[a-zA-Z_][0-9a-zA-Z_]* return( CP_NAME );
[0-9]+                 return( CP_NUMBER );
[a-zA-Z0-9_.]+         return( CP_FILENAME );
"["|"]"|":"|"="        return( config_text[0] );
[\n]                   { line++; return( config_text[0] ); }
.

%%

void config_scanner_init( FILE *infile )
{
  config_restart( infile );

  line = 1;
}

int config_get_line(void)
{
  return( line );
}
