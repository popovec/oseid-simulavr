/*
 * $Id: storage.c,v 1.8 2003/12/02 08:25:00 troth Exp $
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "avrclass.h"
#include "storage.h"

/***************************************************************************\
 *
 * Storage(AvrClass) Methods
 *
\***************************************************************************/

Storage *
storage_new (int base, int size)
{
    Storage *stor;

    stor = avr_new (Storage, 1);
    storage_construct (stor, base, size);
    class_overload_destroy ((AvrClass *)stor, storage_destroy);

    return stor;
}

void
storage_construct (Storage *stor, int base, int size)
{
    if (stor == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)stor);

    stor->base = base;          /* address */
    stor->size = size;          /* bytes   */

    stor->data = avr_new0 (uint8_t, size);
}

/*
 * Not to be called directly, except by a derived class.
 * Called via class_unref.
 */
void
storage_destroy (void *stor)
{
    if (stor == NULL)
        return;

    avr_free (((Storage *)stor)->data);
    class_destroy (stor);
}

extern inline uint8_t
storage_readb (Storage *stor, int addr);

extern inline uint16_t
storage_readw (Storage *stor, int addr);

void
storage_writeb (Storage *stor, int addr, uint8_t val)
{
    int _addr = addr - stor->base;

    if (stor == NULL)
        avr_error ("passed null ptr");

    if ((_addr < 0) || (_addr >= stor->size))
        avr_error ("address out of bounds: 0x%x", addr);

    stor->data[_addr] = val;
}

void
storage_writew (Storage *stor, int addr, uint16_t val)
{
    int _addr = addr - stor->base;

    if (stor == NULL)
        avr_error ("passed null ptr");

    if ((_addr < 0) || (_addr >= stor->size))
        avr_error ("address out of bounds: 0x%x", addr);

    stor->data[_addr] = (uint8_t) (val >> 8 & 0xff);
    stor->data[_addr + 1] = (uint8_t) (val & 0xff);
}

int
storage_get_size (Storage *stor)
{
    return stor->size;
}

int
storage_get_base (Storage *stor)
{
    return stor->base;
}
