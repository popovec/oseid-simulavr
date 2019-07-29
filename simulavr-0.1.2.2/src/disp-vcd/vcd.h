/*
 * $Id: vcd.h,v 1.3 2004/03/11 19:02:48 troth Exp $
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

/* WARNING: This code is a hack and needs major improvements. */

#ifndef VCD_H
#define VCD_H

#define VCD_VERSION "0.0.1"

typedef enum { ST_REGISTER, ST_INTEGER, ST_REAL } t_signal_type;

/* Init */
int vcd_init( int sram_size, int eeprom_size );

/* Interface for parser */
void vcd_set_frequency( int f );
void vcd_set_file_name( char *name );
int vcd_trace_io_reg( char *io_reg_name, int io_reg_addr );
int vcd_trace_reg( int reg_num );
int vcd_trace_sram( int sram_addr );
int vcd_trace_sp( void );
int vcd_trace_pc( void );

/* Interface for disp.c */
int vcd_write_header( void );

int vcd_set_clock( unsigned int c );
int vcd_write_clock( void );

int vcd_bind_io_reg_shortcut( char *io_reg_name, int io_reg_addr );
int vcd_write_io_reg( int io_reg_addr, unsigned char val );
int vcd_write_reg( int reg_num, unsigned char val );
int vcd_write_sram( int sram_addr, unsigned char val );
int vcd_write_sp( int sp );
int vcd_write_pc( int pc );

#endif /* VCD_H */
