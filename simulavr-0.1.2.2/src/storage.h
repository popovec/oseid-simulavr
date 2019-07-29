/*
 * $Id: storage.h,v 1.6 2003/12/02 08:25:00 troth Exp $
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

#ifndef SIM_STORAGE_H
#define SIM_STORAGE_H

/***************************************************************************\
 *
 * Storage(AvrClass) Object
 *
\***************************************************************************/

typedef struct _Storage Storage;

struct _Storage
{
    AvrClass parent;
    int base;                   /* address */
    int size;                   /* bytes */
    uint8_t *data;
};

extern Storage *storage_new (int base, int size);
extern void storage_construct (Storage *stor, int base, int size);
extern void storage_destroy (void *stor);

extern inline uint8_t
storage_readb (Storage *stor, int addr)
{
    int _addr = addr - stor->base;

    if (stor == NULL)
        avr_error ("passed null ptr");

    if ((_addr < 0) || (_addr >= stor->size))
        avr_error ("address out of bounds: 0x%x", addr);

    return stor->data[_addr];
}

extern inline uint16_t
storage_readw (Storage *stor, int addr)
{
    int _addr = addr - stor->base;
    uint8_t bl, bh;

    if (stor == NULL)
        avr_error ("passed null ptr");

    if ((_addr < 0) || (_addr >= stor->size))
        avr_error ("address out of bounds: 0x%x", addr);

    bh = stor->data[_addr];
    bl = stor->data[_addr + 1];

    return (uint16_t) ((bh << 8) | bl);
}

extern void storage_writeb (Storage *stor, int addr, uint8_t val);
extern void storage_writew (Storage *stor, int addr, uint16_t val);

extern int storage_get_size (Storage *stor);
extern int storage_get_base (Storage *stor);

#endif /* SIM_STORAGE_H */
