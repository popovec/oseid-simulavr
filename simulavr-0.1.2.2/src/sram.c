/*
 * $Id: sram.c,v 1.10 2004/01/30 07:09:56 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003, 2004  Theodore A. Roth
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
#include "utils.h"
#include "callback.h"
#include "op_names.h"

#include "storage.h"
#include "flash.h"

#include "vdevs.h"
#include "memory.h"
#include "stack.h"
#include "register.h"
#include "sram.h"
#include "eeprom.h"
#include "timers.h"
#include "ports.h"

#include "avrcore.h"

#include "display.h"

static uint8_t sram_read (VDevice *dev, int addr);
static void sram_write (VDevice *dev, int addr, uint8_t val);
static void sram_reset (VDevice *dev);

SRAM *
sram_new (int base, int size)
{
    SRAM *sram;

    sram = avr_new (SRAM, 1);
    sram_construct (sram, base, size);
    class_overload_destroy ((AvrClass *)sram, sram_destroy);

    return sram;
}

void
sram_construct (SRAM *sram, int base, int size)
{
    if (sram == NULL)
        avr_error ("passed null ptr");

    sram->stor = storage_new (base, size);
    vdev_construct ((VDevice *)sram, sram_read, sram_write, sram_reset,
                    vdev_def_AddAddr);
}

void
sram_destroy (void *sram)
{
    SRAM *_sram = (SRAM *)sram;

    if (sram == NULL)
        return;

    class_unref ((AvrClass *)_sram->stor);

    vdev_destroy (sram);
}

int
sram_get_size (SRAM *sram)
{
    return storage_get_size (sram->stor);
}

int
sram_get_base (SRAM *sram)
{
    return storage_get_base (sram->stor);
}

static uint8_t
sram_read (VDevice *dev, int addr)
{
    SRAM *sram = (SRAM *)dev;

    return storage_readb (sram->stor, addr);
}

static void
sram_write (VDevice *dev, int addr, uint8_t val)
{
    SRAM *sram = (SRAM *)dev;

    display_sram (addr, 1, &val);

    storage_writeb (sram->stor, addr, val);
}

static void
sram_reset (VDevice *dev)
{
    return;                     /* FIXME: should the array be cleared? */
}
