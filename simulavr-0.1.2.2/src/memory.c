/*
 * $Id: memory.c,v 1.19 2004/05/19 23:02:11 troth Exp $
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
 * \file memory.c
 * \brief Memory access functions.
 *
 * This module provides functions for reading and writing to simulated memory.
 * The Memory class is a subclass of AvrClass.
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

#include "display.h"

/** \brief Allocates memory for a new memory object. */

Memory *
mem_new (int gpwr_end, int io_reg_end, int sram_end, int xram_end)
{
    Memory *mem;

    mem = avr_new0 (Memory, 1);
    mem_construct (mem, gpwr_end, io_reg_end, sram_end, xram_end);
    class_overload_destroy ((AvrClass *)mem, mem_destroy);

    return mem;
}

/** \brief Constructor for the memory object. */

void
mem_construct (Memory *mem, int gpwr_end, int io_reg_end, int sram_end,
               int xram_end)
{
    if (mem == NULL)
        avr_error ("passed null ptr");

    mem->gpwr_end = gpwr_end;
    mem->io_reg_end = io_reg_end;
    mem->sram_end = sram_end;
    mem->xram_end = xram_end;

    mem->cell = avr_new0 (MemoryCell, xram_end + 1);

    class_construct ((AvrClass *)mem);
}

/** \brief Descructor for the memory object. */

void
mem_destroy (void *mem)
{
    int i;

    Memory *this = (Memory *)mem;

    if (mem == NULL)
        return;

    for (i = 0; i < this->xram_end; i++)
    {
        if (this->cell[i].vdev)
        {
            class_unref ((AvrClass *)this->cell[i].vdev);
        }
    }

    avr_free (this->cell);

    class_destroy (mem);
}

/** \brief Attach a device to the device list.
 
  Devices that are accessed more often should be attached
  last so that they will be at the front of the list.
 
  A default virtual device can be overridden by attaching
  a new device ahead of it in the list.  */

void
mem_attach (Memory *mem, int addr, char *name, VDevice *vdev, int flags,
            uint8_t reset_value, uint8_t rd_mask, uint8_t wr_mask)
{
    MemoryCell *cell;

    if (mem == NULL)
        avr_error ("passed null ptr");

    if (vdev == NULL)
        avr_error ("attempt to attach null device");

    if ((addr < 0) || (addr >= mem->xram_end))
        avr_error ("address out of range");

    cell = &mem->cell[addr];

    cell->name = name;
    cell->flags = flags;
    cell->reset_value = reset_value;
    cell->rd_mask = rd_mask;
    cell->wr_mask = wr_mask;

    class_ref ((AvrClass *)vdev);
    cell->vdev = vdev;
}

/** \brief Find the VDevice associated with the given address. */

VDevice *
mem_get_vdevice_by_addr (Memory *mem, int addr)
{
    return mem->cell[addr].vdev;
}

/** \brief Find the VDevice associated with the given name. 

    \deprecated */

VDevice *
mem_get_vdevice_by_name (Memory *mem, char *name)
{
#if 0
    return (VDevice *)dlist_lookup (mem->dev, (AvrClass *)name,
                                    vdev_name_cmp);
#else
    avr_error ("use of deprecated interface");
    return NULL;
#endif
}

static inline MemoryCell *
mem_get_cell (Memory *mem, int addr)
{
    return mem->cell + addr;
}

static inline int
mem_is_io_reg (Memory *mem, int addr)
{
    return ((addr > mem->gpwr_end) && (addr <= mem->io_reg_end));
}

static inline char *
mem_get_name (Memory *mem, int addr)
{
    return mem->cell[addr].name;
}

void
mem_set_addr_name (Memory *mem, int addr, char *name)
{
    mem->cell[addr].name = name;
}

/** \brief Reads byte from memory and sanity-checks for valid address. 
 * 
 * \param mem A pointer to the memory object
 * \param addr The address to be read 
 * \return The byte found at that address addr
 */

uint8_t
mem_read (Memory *mem, int addr)
{
    MemoryCell *cell = mem_get_cell (mem, addr);

    if (cell->vdev == NULL)
    {
        char *name = mem_get_name (mem, addr);

        if (name)
        {
            avr_warning ("**** Attempt to read invalid %s: %s at 0x%04x\n",
                         mem_is_io_reg (mem, addr) ? "io reg" : "mem addr",
                         name, addr);
        }
        else
        {
            avr_warning ("**** Attempt to read invalid %s: 0x%04x\n",
                         mem_is_io_reg (mem, addr) ? "io reg" : "mem addr",
                         addr);
        }

        return 0;
    }

    return (vdev_read (cell->vdev, addr) & cell->rd_mask);
}

/** \brief Writes byte to memory and updates display for io registers. 
 * 
 * \param mem A pointer to a memory object
 * \param addr The address to be written to
 * \param val The value to be written there
 */

void
mem_write (Memory *mem, int addr, uint8_t val)
{
    MemoryCell *cell = mem_get_cell (mem, addr);

    if (cell->vdev == NULL)
    {
        char *name = mem_get_name (mem, addr);

        if (name)
        {
            avr_warning ("**** Attempt to write invalid %s: %s at 0x%04x\n",
                         mem_is_io_reg (mem, addr) ? "io reg" : "mem addr",
                         name, addr);
        }
        else
        {
            avr_warning ("**** Attempt to write invalid %s: 0x%04x\n",
                         mem_is_io_reg (mem, addr) ? "io reg" : "mem addr",
                         addr);
        }

        return;
    }

    /* update the display for io registers here */

    if (mem_is_io_reg (mem, addr))
        display_io_reg (addr - (mem->gpwr_end + 1), val & cell->wr_mask);

    vdev_write (cell->vdev, addr, val & cell->wr_mask);
}

/** \brief Resets every device in the memory object.
 * \param mem A pointer to the memory object.
 */

void
mem_reset (Memory *mem)
{
    int i;

    for (i = 0; i < mem->xram_end; i++)
    {
        MemoryCell *cell = mem_get_cell (mem, i);

        if (cell->vdev)
            vdev_reset (cell->vdev);
    }
}

static void
mem_reg_dump_core (Memory *mem, FILE * f_core)
{
    int i, j;

    fprintf (f_core, "General Purpose Register Dump:\n");
    for (i = 0; i < 32; i += 8)
    {
        for (j = i; j < (i + 8); j++)
            fprintf (f_core, "r%02d=%02x  ", j, mem_read (mem, j));
        fprintf (f_core, "\n");
    }
    fprintf (f_core, "\n");
}

/** \brief Fetch the name and value of the io register (addr). 
 *
 * \param mem A pointer to the memory object.
 * \param addr The address to fetch from.
 * \param val A pointer where the value of the register is to be copied.
 * \param buf A pointer to where the name of the register should be copied.
 * \param bufsiz The maximum size of the the buf string.
 */

void
mem_io_fetch (Memory *mem, int addr, uint8_t * val, char *buf, int bufsiz)
{
    MemoryCell *cell;

    if (mem_is_io_reg (mem, addr))
    {
        cell = mem_get_cell (mem, addr);

        if (cell->name == NULL)
        {
            strncpy (buf, "Reserved", bufsiz);
            *val = 0;
        }
        else
        {
            strncpy (buf, cell->name, bufsiz);

            if (cell->vdev)
            {
                /* FIXME: Add vdev_read_no_ext () interface to avoid calling
                   the external functions during a read. This will require a
                   reworking of how the vdev invokes the external read
                   method. */

                *val = (vdev_read (cell->vdev, addr) & cell->rd_mask);
            }
            else
            {
                *val = 0;
            }
        }
    }
    else
    {
        *val = 0;
        strncpy (buf, "NOT AN IO REG", bufsiz);
    }
}

static void
mem_io_reg_dump_core (Memory *mem, FILE * f_core)
{
    int i, j;
    char name[80];
    uint8_t val;

    int begin = mem->gpwr_end + 1;
    int end = mem->io_reg_end;
    int half = (end - begin) / 2;
    int mid = begin + half;

    fprintf (f_core, "IO Register Dump:\n");
    for (i = begin; i < mid; i++)
    {
        for (j = i; j < end; j += half)
        {
            memset (name, '\0', sizeof (name));
            mem_io_fetch (mem, j, &val, name, sizeof (name) - 1);

            fprintf (f_core, "%02x : %-10s : 0x%02x               ", j - half,
                     name, val);
        }
        fprintf (f_core, "\n");
    }
    fprintf (f_core, "\n");
}

static void
mem_sram_display (Memory *mem, FILE * f_core, int base, int size)
{
    int i;
    int dup = 0;
    int ndat = 16;
    char line[80];
    char last_line[80];
    char buf[80];
    line[0] = last_line[0] = '\0';

    for (i = base; i < (base + size); i++)
    {
        if (((i % ndat) == 0) && strlen (line))
        {
            if (strncmp (line, last_line, 80) == 0)
            {
                dup++;
            }
            else
            {
                if (dup > 0)
                    fprintf (f_core, "  -- last line repeats --\n");
                fprintf (f_core, "%04x : %s\n", i - ndat, line);
                dup = 0;
            }
            strncpy (last_line, line, 80);
            line[0] = '\0';
        }
        snprintf (buf, 80, "%02x ", mem_read (mem, i));
        strncat (line, buf, 80 - strlen(line) - 1);
    }
    if (dup > 0)
    {
        fprintf (f_core, "  -- last line repeats --\n");
        fprintf (f_core, "%04x : %s\n", i - ndat, line);
    }
    fprintf (f_core, "\n");
}

static void
mem_sram_dump_core (Memory *mem, FILE * f_core)
{
    int size, base;

    /*
     * Dump the internal sram
     */

    if (mem->io_reg_end == mem->sram_end)
        return;                 /* device has no sram */

    fprintf (f_core, "Internal SRAM Memory Dump:\n");
    base = mem->io_reg_end + 1;
    size = mem->sram_end;
    mem_sram_display (mem, f_core, base, size);

    /*
     * If external sram present, dump it too.
     */

    if (mem->sram_end == mem->xram_end)
        return;                 /* device has no xram */

    fprintf (f_core, "External SRAM Memory Dump:\n");
    base = mem->sram_end + 1;
    size = mem->xram_end;
    mem_sram_display (mem, f_core, base, size);

}

#if 0

/* FIXME: Still need to figure out a sane way to look up a specific type of
   vdev by generic name. */

static void
mem_eeprom_dump_core (Memory *mem, FILE * f_core)
{
    VDevice *dev = mem_get_vdevice_by_name (mem, "EEProm");

    if (dev != NULL)
        eeprom_dump_core ((EEProm *)dev, f_core);
}
#endif

/** \brief Dump all the various memory locations to a file descriptor 
 *         in text format.
 *
 *  \param mem A memory object.
 *  \param f_core An open file descriptor.
 */

void
mem_dump_core (Memory *mem, FILE * f_core)
{
    mem_reg_dump_core (mem, f_core);
    mem_io_reg_dump_core (mem, f_core);
    mem_sram_dump_core (mem, f_core);
/*     mem_eeprom_dump_core (mem, f_core); */
}
