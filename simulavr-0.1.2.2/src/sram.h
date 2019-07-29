/*
 * $Id: sram.h,v 1.6 2003/12/01 07:35:54 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002  Theodore A. Roth
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

#ifndef SIM_SRAM_H
#define SIM_SRAM_H

/****************************************************************************\
 *
 * SRAM(VDevice) Definition
 *
\****************************************************************************/

enum _sram_addr_info
{
    SRAM_BASE = 0x60,           /* base sram mem addr */
    SRAM_EXTENDED_IO_BASE = 0x100,
};

typedef struct _SRAM SRAM;

struct _SRAM
{
    VDevice parent;
    Storage *stor;
};

extern SRAM *sram_new (int base, int size);
extern void sram_construct (SRAM *sram, int base, int size);
extern void sram_destroy (void *sram);

extern int sram_get_size (SRAM *sram);
extern int sram_get_base (SRAM *sram);

#endif /* SIM_SRAM_H */
