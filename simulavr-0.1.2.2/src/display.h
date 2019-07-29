/*
 * $Id: display.h,v 1.8 2003/12/01 07:35:52 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003  Theodore A. Roth
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

#ifndef SIM_DISPLAY_H
#define SIM_DISPLAY_H

extern int display_open (char *prog, int no_xterm, int flash_sz, int sram_sz,
                         int sram_start, int eeprom_sz);
extern void display_close (void);

/* These functions will tell the display to update the given value */

extern void display_clock (int clock);
extern void display_pc (int val);
extern void display_reg (int reg, uint8_t val);
extern void display_io_reg (int reg, uint8_t val);
extern void display_io_reg_name (int reg, char *name);
extern void display_flash (int addr, int len, uint16_t * vals);
extern void display_sram (int addr, int len, uint8_t * vals);
extern void display_eeprom (int addr, int len, uint8_t * vals);

/* FIXME: this isn't going to be public for much longer */
extern void display_send_msg (char *msg);

#endif /* SIM_DISPLAY_H */
