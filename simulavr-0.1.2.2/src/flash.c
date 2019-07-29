/*
 * $Id: flash.c,v 1.12 2003/12/02 08:25:00 troth Exp $
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

/**
 * \file flash.c
 * \brief Flash memory methods
 *
 * This module provides functions for reading and writing to flash memory.
 * Flash memory is the program (.text) memory in AVR's Harvard architecture.
 * It is completely separate from RAM, which is simulated in the memory.c
 * file.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "avrclass.h"
#include "utils.h"

#include "storage.h"
#include "flash.h"

#include "display.h"

/***************************************************************************\
 *
 * Local Static Function Prototypes
 *
\***************************************************************************/

static int flash_load_from_bin_file (Flash *flash, char *file);

/***************************************************************************\
 *
 * Flash(Storage) Methods
 *
\***************************************************************************/

/**
 * \brief Reads a 16-bit word from flash.
 * \return A word.
 */

extern inline uint16_t flash_read (Flash *flash, int addr);

/**
 * \brief Reads a 16-bit word from flash.
 * \param flash A pointer to a flash object.
 * \param addr The address to which to write.
 * \param val The byte to write there.
 */

void
flash_write (Flash *flash, int addr, uint16_t val)
{
    display_flash (addr, 1, &val);
    storage_writew ((Storage *)flash, addr * 2, val);
}

/** \brief Write the low-order byte of an address.
 *
 * AVRs are little-endian, so lo8 bits in odd addresses.
 */

void
flash_write_lo8 (Flash *flash, int addr, uint8_t val)
{
    storage_writeb ((Storage *)flash, addr * 2 + 1, val);
}

/** \brief Write the high-order byte of an address.
 *
 * AVRs are little-endian, so hi8 bits in even addresses.
 */

void
flash_write_hi8 (Flash *flash, int addr, uint8_t val)
{
    storage_writeb ((Storage *)flash, addr * 2, val);
}

/** \brief Allocate a new Flash object. */

Flash *
flash_new (int size)
{
    Flash *flash;

    flash = avr_new (Flash, 1);
    flash_construct (flash, size);
    class_overload_destroy ((AvrClass *)flash, flash_destroy);

    return flash;
}

/** \brief Constructor for the flash object. */

void
flash_construct (Flash *flash, int size)
{
    int base = 0;
    int i;

    if (flash == NULL)
        avr_error ("passed null ptr");

    storage_construct ((Storage *)flash, base, size);

    /* Init the flash to ones. */
    for (i = 0; i < size; i++)
        storage_writeb ((Storage *)flash, i, 0xff);
}

/**
 * \brief Destructor for the flash class.
 *
 * Not to be called directly, except by a derived class.
 * Called via class_unref.
 */
void
flash_destroy (void *flash)
{
    if (flash == NULL)
        return;

    storage_destroy (flash);
}

/** \brief Load program data into flash from a file. */

int
flash_load_from_file (Flash *flash, char *file, int format)
{
    switch (format)
    {
        case FFMT_BIN:
            return flash_load_from_bin_file (flash, file);
        case FFMT_IHEX:
        case FFMT_ELF:
        default:
            avr_warning ("Unsupported file format\n");
    }

    return -1;
}

static int
flash_load_from_bin_file (Flash *flash, char *file)
{
    int fd, res;
    int addr = 0;
    uint16_t inst;

    fd = open (file, O_RDONLY);
    if (fd < 0)
        avr_error ("Couldn't open binary flash image file: %s: %s", file,
                   strerror (errno));

    while ((res = read (fd, &inst, sizeof (inst))) != 0)
    {
        if (res == -1)
            avr_error ("Error reading binary flash image file: %s: %s", file,
                       strerror (errno));

        flash_write (flash, addr, inst);

        addr++;
    }

    close (fd);

    return 0;
}

/** \brief Accessor method to get the size of a flash. */

int
flash_get_size (Flash *flash)
{
    return storage_get_size ((Storage *)flash);
}

/**
 * \brief Dump the contents of the flash to a file descriptor in text format.
 *
 * \param flash A pointer to a flash object.
 * \param f_core An open file descriptor.
 */

void
flash_dump_core (Flash *flash, FILE * f_core)
{
    int size = storage_get_size ((Storage *)flash) / 2;
    int i;
    int dup = 0;
    int ndat = 8;
    char line[80];
    char last_line[80];
    char buf[80];

    line[0] = last_line[0] = '\0';

    fprintf (f_core, "Program Flash Memory Dump:\n");
    for (i = 0; i < size; i++)
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
        snprintf (buf, 80, "%04x ", flash_read (flash, i));
        strncat (line, buf, 80 - strnlen(line,79) - 1);
    }
    if (dup > 0)
    {
        fprintf (f_core, "  -- last line repeats --\n");
        fprintf (f_core, "%04x : %s\n", i - ndat, line);
    }
}
