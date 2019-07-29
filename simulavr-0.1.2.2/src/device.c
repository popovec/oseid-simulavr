/*
 * $Id: device.c,v 1.15 2004/01/30 07:09:56 troth Exp $
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
#include <string.h>

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

/** \file device.c
  * \brief VDevice methods
  *
  * These functions are the base for all other devices
  * mapped into the device space.
  */

/** \brief Create a new VDevice. */
VDevice *
vdev_new (char *name, VDevFP_Read rd, VDevFP_Write wr, VDevFP_Reset reset,
          VDevFP_AddAddr add_addr)
{
    VDevice *dev;

    dev = avr_new0 (VDevice, 1);
    vdev_construct (dev, rd, wr, reset, add_addr);
    class_overload_destroy ((AvrClass *)dev, vdev_destroy);

    return dev;
}

/** \brief Default AddAddr method.

    This generate a warning that the should let the developer know that the
    vdev needs to be updated. */

void
vdev_def_AddAddr (VDevice *dev, int addr, char *name, int related_addr,
                  void *data)
{
    avr_warning ("Default AddAddr called [addr=0x%x; '%s']\n", addr, name);
}


/** \brief Constructor for a VDevice. */
void
vdev_construct (VDevice *dev, VDevFP_Read rd, VDevFP_Write wr,
                VDevFP_Reset reset, VDevFP_AddAddr add_addr)
{
    if (dev == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)dev);

    dev->read = rd;
    dev->write = wr;
    dev->reset = reset;
    dev->add_addr = add_addr;
}

/** \brief Destructor for a VDevice. */
void
vdev_destroy (void *dev)
{
    if (dev == NULL)
        return;

    class_destroy (dev);
}

#if 0

/** \brief Compare the names of 2 devices
  * \param c1 The first device.
  * \param c2 is a string and not an AvrClass object, because this function is
  * called by dlist_lookup() which passes two AvrClass pointers. So the string
  * is casted to an *AvrClass.
  */
int
vdev_name_cmp (AvrClass *c1, AvrClass *c2)
{
    return strcmp (((VDevice *)c1)->name, (char *)c2);
}

#endif

#if 0
/** \brief Checks if a address is in the device's address range
  * \param c1 \c AvrClass to check.
  * \param c2 The address to check.
  *
  * \return The different between the device's address bounds and \a c2 or if
  * \a c2 is within the address range 0.
  *
  * \note When comparing an addr, c2 is really just a pointer (see
  * vdev_name_cmp() for details) to int and then we see if d1->base <= addr <
  * (d1->base+d1->size).
  */
int
vdev_addr_cmp (AvrClass *c1, AvrClass *c2)
{
    VDevice *d1 = (VDevice *)c1;
    int addr = *(int *)c2;

    if (addr < d1->base)
        return (addr - d1->base);

    if (addr >= (d1->base + d1->size))
        /* Add one to ensure we don't return zero. */
        return (1 + addr - (d1->base + d1->size));

    /* addr is in device's range */
    return 0;
}
#endif

/** \brief Reads the device's value in the register at \a addr. */
uint8_t
vdev_read (VDevice *dev, int addr)
{
    return dev->read (dev, addr);
}

/** \brief Writes an value to the register at \a addr. */
void
vdev_write (VDevice *dev, int addr, uint8_t val)
{
    dev->write (dev, addr, val);
}

/** \brief Resets a device. */
void
vdev_reset (VDevice *dev)
{
    dev->reset (dev);
}

/** \brief Set the core field. */
void
vdev_set_core (VDevice *dev, AvrClass *core)
{
    dev->core = (AvrClass *)core;
}

/** \brief Get the core field. */
extern inline AvrClass *vdev_get_core (VDevice *dev);

/** \brief Inform the vdevice that it needs to handle another address.

    This is primarily used when creating the core in dev_supp_create_core(). */

extern void
vdev_add_addr (VDevice *dev, int addr, char *name, int rel_addr, void *data)
{
    if (dev->add_addr)
    {
        dev->add_addr (dev, addr, name, rel_addr, data);
    }
    else
    {
        avr_warning ("attempt to add addr to vdev with no add_addr() method: "
                     "%s [0x%04x]\n", name, addr);
    }
}

#if 0
/** \brief Get the device's base address. */
int
vdev_get_base (VDevice *dev)
{
    return dev->base;
}
#endif

#if 0
/** \brief Set the device's size (the number of bytes of the address space it
 *  consumes). */
int
vdev_get_size (VDevice *dev)
{
    return dev->size;
}
#endif

#if 0
/** \brief Get the device's name. */
char *
vdev_get_name (VDevice *dev)
{
    return dev->name;
}
#endif
