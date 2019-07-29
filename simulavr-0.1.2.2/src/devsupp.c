/*
 * $Id: devsupp.c,v 1.28 2004/02/26 07:22:15 troth Exp $
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

/**
 * \file devsupp.c
 * \brief Contains definitions for device types (i.e. at90s8515, at90s2313,
 * etc.)
 *
 * This module is used to define the attributes for each device in the AVR
 * family. A generic constructor is used to create a new AvrCore object with
 * the proper ports, built-in peripherals, memory layout, registers, and
 * interrupt vectors, etc.
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
#include "spi.h"
#include "adc.h"
#include "usb.h"
#include "uart.h"

#include "avrcore.h"

#include "devsupp.h"
#include "OsEID.h"
#ifndef DOXYGEN                 /* don't expose to doxygen */

/*
 * Used to select which vector table the device uses.
 * The value is an index into the global_vtable_list[] array 
 * defined in intvects.c.
 */
enum _vector_table_select
{
    VTAB_AT90S1200 = 0,
    VTAB_AT90S2313,
    VTAB_AT90S4414,
    VTAB_ATMEGA8,
    VTAB_ATMEGA16,
    VTAB_ATMEGA103,
    VTAB_ATMEGA128,
    VTAB_AT43USB355,
    VTAB_AT43USB320,
    VTAB_AT43USB325,
    VTAB_AT43USB326,
};

/* IO Register Definition.  */

struct io_reg_defn {
    uint16_t addr;              /* The address of the register (in memory
                                   space, not IO space). */
    char *name;                 /* The register name as given by the
                                   datasheet. */
    uint16_t ref_addr;          /* Use the vdev reference at the given
                                   address.  If not zero, use the same vdev
                                   pointer at the given register address. The
                                   vdev must have already been created at that
                                   address.  Remember that address zero is
                                   General Register 0 which is not an IO
                                   register. This value should never be less
                                   than 0x20. */
    uint16_t related;           /* A related address. For example, if a device
                                   has more than one timer, each timer has
                                   it's own instance of the timer vdev, but
                                   all timers need access to the TIMSK
                                   register which should have the addr
                                   specified here. There might be more than
                                   one related address for a vdev, so this may
                                   need to become an array. */
    VDevCreate vdev_create;     /* The vdev creation function to be used to
                                   create the vdev. Note that this should be a
                                   wrapper around the vdev's new method. This
                                   field is ignored if rel_addr is greater
                                   than 0x1f. */
    void *data;                 /* Optional data that may be needed by the
                                   vdev. May be address specific too. */
    int flags;                  /* Flags that can change the behaviour of the
                                   value. (e.g. FL_SET_ON_RESET) */
    uint8_t reset_value;        /* Initialize the register to this value after
                                   reset. */

    /* These should handle PORT width automagically. */

    uint8_t rd_mask;            /* Mask of the readable bits in the
                                   register. */
    uint8_t wr_mask;            /* Mask of the writable bits in the
                                   register. */
};

#define IO_REG_DEFN_TERMINATOR { .name = NULL, }

/* Structure for defining a supported device */

struct _DevSuppDefn
{
    char *name;                 /* name of device type */

    StackType stack_type;       /* STACK_HARDWARE or STACK_MEMORY */
    int has_ext_io_reg;         /* does the device have extened IO registers */
    int irq_vect_idx;           /* interrupt vector table index */

    struct
    {
        int pc;                 /* width of program counter (usually 2, maybe
                                   3) */
        int stack;              /* depth of stack (only used by hardware
                                   stack) */
        int flash;              /* bytes of flash memory */
        int sram;               /* bytes of sram memory */
        int eeprom;             /* bytes of eeprom memory */
    } size;

    /* This _must_ be the last field of the structure since it is a variable
       length array. (This is new in C99.) */

    struct io_reg_defn io_reg[]; /* Definitions for for all the IO registers
                                    the device provides. */
};

#endif /* DOXYGEN */

int
dev_supp_has_ext_io_reg (DevSuppDefn *dev)
{
    return dev->has_ext_io_reg;
}

int
dev_supp_get_flash_sz (DevSuppDefn *dev)
{
    return dev->size.flash;
}

int
dev_supp_get_PC_sz (DevSuppDefn *dev)
{
    return dev->size.pc;
}

int
dev_supp_get_stack_sz (DevSuppDefn *dev)
{
    return dev->size.stack;
}

int
dev_supp_get_vtab_idx (DevSuppDefn *dev)
{
    return dev->irq_vect_idx;
}

int
dev_supp_get_sram_sz (DevSuppDefn *dev)
{
    return dev->size.sram;
}

/*
 * Device Definitions
 */

#define IN_DEVSUPP_C

#include "defn/90s1200.h"
#include "defn/90s2313.h"
#include "defn/90s4414.h"
#include "defn/90s8515.h"

#include "defn/mega8.h"
#include "defn/mega16.h"
#include "defn/mega103.h"
#include "defn/mega128.h"
#include "defn/OsEID128.h"

#include "defn/43usb320.h"
#include "defn/43usb325.h"
#include "defn/43usb326.h"

#include "defn/43usb351.h"
#include "defn/43usb353.h"
#include "defn/43usb355.h"

#undef IN_DEVSUPP_C

/** \brief List of supported devices. */

static DevSuppDefn *devices_supported[] = {
    &defn_at90s1200,
    &defn_at90s2313,
    &defn_at90s4414,
    &defn_at90s8515,
    &defn_atmega8,
    &defn_atmega16,
    &defn_atmega103,
    &defn_atmega128,
    &defn_OsEID128,
    &defn_at43usb351,
    &defn_at43usb353,
    &defn_at43usb355,
    &defn_at43usb320,
    &defn_at43usb325,
    &defn_at43usb326,
    NULL
};

/**
 * \brief Look up a device name in support list.
 *
 * \returns An opaque pointer to DevSuppDefn or NULL if not found.
 */

DevSuppDefn *
dev_supp_lookup_device (char *dev_name)
{
    DevSuppDefn **dev = devices_supported;
    int len;

    while ((*dev))
    {
        len = strlen ((*dev)->name);

        if (strncmp ((*dev)->name, dev_name, len) == 0)
            return (*dev);

        dev++;
    }
    return NULL;
}

/** \brief Print a list of supported devices to a file pointer. */

void
dev_supp_list_devices (FILE * fp)
{
    DevSuppDefn **dev;

    for (dev = devices_supported; (*dev); dev++)
        fprintf (fp, "  %s\n", (*dev)->name);
}

void
dev_supp_attach_io_regs (AvrCore *core, DevSuppDefn *dev)
{
    VDevice *vdev;
    struct io_reg_defn *reg = dev->io_reg;

    while (reg->name)
    {
        if (reg->ref_addr)
        {
            if (reg->ref_addr < 0x20)
            {
                avr_error ("can't attach IO reg into general register space");
            }

            /* Get the referenced vdev. */

            vdev = avr_core_get_vdev_by_addr (core, reg->ref_addr);
            if (vdev == NULL)
            {
                /* This means that the implementor of the vdev screwed up. */
                avr_error ("reference vdev hasn't been created yet");
            }

            vdev_add_addr (vdev, reg->addr, reg->name, reg->related,
                           reg->data);

            avr_core_attach_vdev (core, reg->addr, reg->name, vdev, reg->flags,
                                  reg->reset_value, reg->rd_mask,
                                  reg->wr_mask);

            avr_message ("attach: IO Reg '%s' at 0x%04x: ref = 0x%04x\n",
                         reg->name, reg->addr, reg->ref_addr);
        }

        else if (reg->vdev_create)
        {
            vdev = reg->vdev_create (reg->addr, reg->name, reg->related,
                                     reg->data);
            avr_core_attach_vdev (core, reg->addr, reg->name, vdev, reg->flags,
                                  reg->reset_value, reg->rd_mask,
                                  reg->wr_mask);

            /* Attaching implicitly references the device so we need to unref
               the newly created vdev since we're done with it here. */
            class_unref ((AvrClass *)vdev);

            avr_message ("attach: IO Reg '%s' at 0x%04x: created\n",
                         reg->name, reg->addr);
        }

        else
        {
            avr_message ("TODO: attach IO Reg '%s' at 0x%04x\n", reg->name,
                         reg->addr);

            avr_core_set_addr_name (core, reg->addr, reg->name);
        }

        reg++;
    }
}
