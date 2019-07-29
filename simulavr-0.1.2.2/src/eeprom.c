/*
 * $Id: eeprom.c,v 1.20 2004/01/30 07:09:56 troth Exp $
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
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

static uint8_t eeprom_reg_read (VDevice *dev, int addr);
static void eeprom_reg_write (VDevice *dev, int addr, uint8_t val);
static void eeprom_reg_reset (VDevice *dev);
static void eeprom_wr_eecr (EEProm *ee, uint8_t val);

static int eeprom_wr_op_cb (uint64_t time, AvrClass *data);
static int eeprom_mwe_clr_cb (uint64_t time, AvrClass *data);

EEProm *
eeprom_new (int size, uint8_t eecr_mask)
{
    EEProm *eeprom;

    eeprom = avr_new (EEProm, 1);
    eeprom_construct (eeprom, size, eecr_mask);
    class_overload_destroy ((AvrClass *)eeprom, eeprom_destroy);

    return eeprom;
}

void
eeprom_construct (EEProm *eeprom, int size, uint8_t eecr_mask)
{
    int i;

    if (eeprom == NULL)
        avr_error ("passed null ptr");

    eeprom->stor = storage_new (0 /*base */ , size);

    /* init eeprom to ones */
    for (i = 0; i < size; i++)
        storage_writeb (eeprom->stor, i, 0xff);

    eeprom->eecr_mask = eecr_mask;

    eeprom_reg_reset ((VDevice *)eeprom);

    vdev_construct ((VDevice *)eeprom, eeprom_reg_read, eeprom_reg_write,
                    eeprom_reg_reset, vdev_def_AddAddr);
}

void
eeprom_destroy (void *eeprom)
{
    EEProm *_eeprom = (EEProm *)eeprom;

    if (eeprom == NULL)
        return;

    class_unref ((AvrClass *)_eeprom->stor);

    vdev_destroy (eeprom);
}

int
eeprom_get_size (EEProm *eeprom)
{
    return storage_get_size (eeprom->stor);
}

static uint8_t
eeprom_reg_read (VDevice *dev, int addr)
{
    EEProm *ee = (EEProm *)dev;

    switch (addr)
    {
        case EECR_ADDR:
            return ee->eecr;
        case EEDR_ADDR:
            return ee->eedr;
        case EEARL_ADDR:
            return ee->eearl;
        case EEARH_ADDR:
            return ee->eearh;
    }
    avr_error ("Bad address: %d", addr);
    return 0;
}

static void
eeprom_reg_write (VDevice *dev, int addr, uint8_t val)
{
    EEProm *ee = (EEProm *)dev;

    if (ee->eecr & mask_EEWE)
    {
        /*
         * From the 8515 data sheet: The user should poll the EEWE bit before
         * starting the read operaton. If a write operation is in progress
         * when new data or address is written to the EEPROM I/O registers,
         * the write operation will be interrupted, and the result is
         * undefined.
         */
        avr_error ("Attempt to write to EEPROM I/O reg during write "
                   "operation");
    }

    switch (addr)
    {
        case EECR_ADDR:
            eeprom_wr_eecr (ee, val);
            return;
        case EEDR_ADDR:
            ee->eedr = val;
            return;
        case EEARL_ADDR:
            ee->eearl = val;
            return;
        case EEARH_ADDR:
            ee->eearh = val;
            return;
    }
    avr_error ("Bad address: %d", addr);
}

static void
eeprom_reg_reset (VDevice *dev)
{
    EEProm *ee = (EEProm *)dev;

    ee->wr_op_cb = NULL;
    ee->wr_op_clk = 0;

    ee->mwe_clr_cb = NULL;
    ee->mwe_clk = 0;

    ee->eecr = ee->eedr = ee->eearl = ee->eearh = 0;
}

static void
eeprom_wr_eecr (EEProm *ee, uint8_t val)
{
    int addr = (ee->eearh << 8) | ee->eearl;

    CallBack *cb;

    switch (val & ee->eecr_mask)
    {
        case mask_EERE:
            /*
             * we never need to set EERE bit one,
             * just more data from eeprom array into eedr.
             */
            ee->eedr = storage_readb (ee->stor, addr);
            break;

        case mask_EEWE:
            if (((ee->eecr_mask & mask_EEMWE) == 0) /* device has no MWE
                                                       function */
                || (ee->eecr & ee->eecr_mask & mask_EEMWE)) /* or MWE bit is
                                                               set */
            {
                ee->eecr |= mask_EEWE;
                ee->wr_op_clk = EEPROM_WR_OP_CLKS;

                /* start write operation */
                if (ee->wr_op_cb == NULL)
                {
                    cb = callback_new (eeprom_wr_op_cb, (AvrClass *)ee);
                    ee->wr_op_cb = cb;
                    avr_core_async_cb_add ((AvrCore *)
                                           vdev_get_core ((VDevice *)ee), cb);
                }
            }
            break;

        case mask_EEMWE:
            ee->eecr |= mask_EEMWE;
            ee->mwe_clk = EEPROM_MWE_CLKS;

            if (ee->mwe_clr_cb == NULL)
            {
                cb = callback_new (eeprom_mwe_clr_cb, (AvrClass *)ee);
                ee->mwe_clr_cb = cb;
                avr_core_clk_cb_add ((AvrCore *)vdev_get_core ((VDevice *)ee),
                                     cb);
            }
            break;

        case (mask_EEMWE | mask_EEWE):
            /* just call this function again, but without EEMWE set in val */
            eeprom_wr_eecr (ee, mask_EEWE);
            break;

        default:
            avr_error ("Unknown eeprom control register write operation: "
                       "0x%02x", val);
    }
}

/*
 * The data sheets say that a write operation takes 2.5 to 4.0 ms to complete
 * depending on Vcc voltage. Since the get_program_time() function only has
 * 10 ms resolution, we'll just simulate a timer with counting down from
 * EEPROM_WR_OP_CLKS to zero. 2500 clocks would be 2.5 ms if simulator is
 * running at 1 MHz. I really don't think that this variation should be 
 * critical in most apps, but I'd wouldn't mind being proven wrong.
 */
static int
eeprom_wr_op_cb (uint64_t time, AvrClass *data)
{
    EEProm *ee = (EEProm *)data;
    int addr;

    /*
     * FIXME: At some point in the future, we might need to check if
     * any of the I/O registers have been written to during the write
     * operation which would cause the write op to be interrupted.
     * Right now, the simulator is aborted in that situation.
     */

    if (ee->wr_op_clk > 0)
    {
        /* write is not complete yet */
        ee->wr_op_clk--;
        return CB_RET_RETAIN;
    }

    /* write the data in eedr into eeprom at addr */
    addr = (ee->eearh << 8) | ee->eearl;
    avr_warning ("writing 0x%02x to eeprom at 0x%04x\n", ee->eedr, addr);
    display_eeprom (addr, 1, &ee->eedr);
    storage_writeb (ee->stor, addr, ee->eedr);

    /* Now it's ok to start another write operation */
    ee->eecr &= ~(mask_EEWE);   /* clear the write enable bit */
    ee->wr_op_cb = NULL;        /* remove callback */

    return CB_RET_REMOVE;
}

/*
 * Once set, the hardware will automatically clear the EEMWE bit
 * after EEPROM_MWE_CLKS clock cycles.
 */
static int
eeprom_mwe_clr_cb (uint64_t time, AvrClass *data)
{
    EEProm *ee = (EEProm *)data;

    if (ee->mwe_clk > 0)
    {
        ee->mwe_clk--;
        return CB_RET_RETAIN;
    }

    ee->eecr &= ~(mask_EEMWE);  /* clear the EEMWE bit */
    ee->mwe_clr_cb = NULL;      /* remove callback */

    return CB_RET_REMOVE;
}

static int
eeprom_load_from_bin_file (EEProm *eeprom, char *file)
{
    int fd, res;
    int addr = 0;
    uint8_t datum;

    fd = open (file, O_RDONLY);
    if (fd < 0)
        avr_error ("Couldn't open binary eeprom image file: %s: %s", file,
                   strerror (errno));

    while ((res = read (fd, &datum, sizeof (datum))) != 0)
    {
        if (res == -1)
            avr_error ("Error reading binary eeprom image file: %s: %s", file,
                       strerror (errno));

        storage_writeb (eeprom->stor, addr, datum);

        addr++;
    }

    close (fd);

    return 0;
}

/** \brief Load data into eeprom from a file. */

int
eeprom_load_from_file (EEProm *eeprom, char *file, int format)
{
    switch (format)
    {
        case FFMT_BIN:
            return eeprom_load_from_bin_file (eeprom, file);
        case FFMT_IHEX:
        case FFMT_ELF:
        default:
            avr_warning ("Unsupported file format\n");
    }

    return -1;
}

void
eeprom_dump_core (EEProm *eeprom, FILE * f_core)
{
    int i;
    int dup = 0;
    int ndat = 16;
    char line[80];
    char last_line[80];
    char buf[80];
    int size = storage_get_size (eeprom->stor);

    fprintf (f_core, "EEPROM Memory Dump:\n");

    line[0] = last_line[0] = '\0';

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
        snprintf (buf, 80, "%02x ", storage_readb (eeprom->stor, i));
        strncat (line, buf, 80 - strnlen(line,79) - 1);
    }
    if (dup > 0)
    {
        fprintf (f_core, "  -- last line repeats --\n");
        fprintf (f_core, "%04x : %s\n", i - ndat, line);
    }
    fprintf (f_core, "\n");
}
