/*
 * $Id: spi.c,v 1.5 2004/03/13 19:55:34 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2003, 2004  Keith Gudger
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
 * \file spi.c
 * \brief Module to simulate the AVR's SPI module.
 *
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

#include "avrcore.h"

#include "intvects.h"

/****************************************************************************\
 *
 * SPI Interrupts 
 *
\****************************************************************************/

static void spii_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                           void *data);
static uint8_t spi_intr_read (VDevice *dev, int addr);
static void spi_intr_write (VDevice *dev, int addr, uint8_t val);
static void spi_intr_reset (VDevice *dev);
static int spi_intr_cb (uint64_t time, AvrClass *data);

/** \brief Allocate a new SPI interrupt */

VDevice *
spii_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)spi_intr_new (addr, name);
}

SPIIntr_T *
spi_intr_new (int addr, char *name)
{
    SPIIntr_T *spi;

    spi = avr_new (SPIIntr_T, 1);
    spi_intr_construct (spi, addr, name);
    class_overload_destroy ((AvrClass *)spi, spi_intr_destroy);

    return spi;
}

/** \brief Constructor for spi interrupt object. */

void
spi_intr_construct (SPIIntr_T *spi, int addr, char *name)
{
    if (spi == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)spi, spi_intr_read, spi_intr_write,
                    spi_intr_reset, spii_add_addr);

    spii_add_addr ((VDevice *)spi, addr, name, 0, NULL);
    spi_intr_reset ((VDevice *)spi);
}

static void
spii_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    SPIIntr_T *spi = (SPIIntr_T *)vdev;

    if (strncmp ("SPCR", name, 4) == 0)
    {
        spi->spcr_addr = addr;
    }

    else if (strncmp ("SPSR", name, 4) == 0)
    {
        spi->spsr_addr = addr;
    }

    else
    {
        avr_error ("invalid ADC register name: '%s' @ 0x%04x", name, addr);
    }
}

/** \brief Destructor for spi interrupt object. */

void
spi_intr_destroy (void *spi)
{
    if (spi == NULL)
        return;

    vdev_destroy (spi);
}

static uint8_t
spi_intr_read (VDevice *dev, int addr)
{
    SPIIntr_T *spi = (SPIIntr_T *)dev;

    if (addr == spi->spcr_addr)
    {
        return (spi->spcr);
    }

    else if (addr == spi->spsr_addr)
    {
        if (spi->spsr & mask_SPIF)
            spi->spsr_read |= mask_SPIF;
        if (spi->spsr & mask_WCOL)
            spi->spsr_read |= mask_WCOL;
        return (spi->spsr);
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
spi_intr_write (VDevice *dev, int addr, uint8_t val)
{
    SPIIntr_T *spi = (SPIIntr_T *)dev;
    CallBack *cb;

    if (addr == spi->spcr_addr)
    {
        spi->spcr = val;
        if (spi->spcr & mask_SPE)
        {
            /* we need to install the intr_cb function */
            cb = callback_new (spi_intr_cb, (AvrClass *)spi);
            spi->intr_cb = cb;
            avr_core_async_cb_add ((AvrCore *)vdev_get_core (dev), cb);
        }
        else
        {
            spi->intr_cb = NULL; /* no interrupt are enabled, remove the
                                    callback */
        }
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
spi_intr_reset (VDevice *dev)
{
    SPIIntr_T *spi = (SPIIntr_T *)dev;

    spi->intr_cb = NULL;

    spi->spcr = 0;
    spi->spsr = 0;
    spi->spsr_read = 0;
}

static int
spi_intr_cb (uint64_t time, AvrClass *data)
{
    SPIIntr_T *spi = (SPIIntr_T *)data;

    if (spi->intr_cb == NULL)
        return CB_RET_REMOVE;

    if ((spi->spcr & mask_SPE) && (spi->spcr & mask_SPIE)
        && (spi->spsr & mask_SPIF))
    {
        /* an enabled interrupt occured */
        AvrCore *core = (AvrCore *)vdev_get_core ((VDevice *)spi);
        avr_core_irq_raise (core, irq_vect_table_index (SPI_STC));
        spi->spsr &= ~mask_SPIF;
        spi->spsr = 0;
    }

    return CB_RET_RETAIN;
}

/****************************************************************************\
 *
 * SPI  
 *
\****************************************************************************/

static void spi_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                          void *data);
static uint8_t spi_read (VDevice *dev, int addr);
static void spi_write (VDevice *dev, int addr, uint8_t val);
static void spi_reset (VDevice *dev);
static int spi_clk_incr_cb (uint64_t ck, AvrClass *data);

/** \brief Allocate a new SPI structure. */

VDevice *
spi_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)spi_new (addr, name, rel_addr);
}

SPI_T *
spi_new (int addr, char *name, int rel_addr)
{
    SPI_T *spi;

    spi = avr_new (SPI_T, 1);
    spi_construct (spi, addr, name, rel_addr);
    class_overload_destroy ((AvrClass *)spi, spi_destroy);

    return spi;
}

/** \brief Constructor for SPI object. */

void
spi_construct (SPI_T *spi, int addr, char *name, int rel_addr)
{
    if (spi == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)spi, spi_read, spi_write, spi_reset,
                    spi_add_addr);

    spi_add_addr ((VDevice *)spi, addr, name, 0, NULL);
    if (rel_addr)
        spi->rel_addr = rel_addr;
    spi_reset ((VDevice *)spi);
}

static void
spi_add_addr (VDevice *vdev, int addr, char *name, int ref_addr, void *data)
{
    SPI_T *spi = (SPI_T *)vdev;

    if (strncmp ("SPDR", name, 4) == 0)
    {
        spi->spdr_addr = addr;
    }

    else
    {
        avr_error ("invalid SPI register name: '%s' @ 0x%04x", name, addr);
    }
}

/** \brief Destructor for SPI object. */

void
spi_destroy (void *spi)
{
    if (spi == NULL)
        return;

    vdev_destroy (spi);
}

static uint8_t
spi_read (VDevice *dev, int addr)
{
    SPI_T *spi = (SPI_T *)dev;
    SPIIntr_T *spi_ti;

    spi_ti =
        (SPIIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                vdev_get_core ((VDevice *)
                                                               spi),
                                                spi->rel_addr);

    if (addr == spi->spdr_addr)
    {
        if (spi_ti->spsr_read)
        {
            spi_ti->spsr &= ~spi_ti->spsr_read;
            spi_ti->spsr_read = 0;
        }
        return spi->spdr;

    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
spi_write (VDevice *dev, int addr, uint8_t val)
{
    SPI_T *spi = (SPI_T *)dev;
    CallBack *cb;
    SPIIntr_T *spi_ti;

    spi_ti =
        (SPIIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                vdev_get_core ((VDevice *)
                                                               spi),
                                                spi->rel_addr);

    if (addr == spi->spdr_addr)
    {
        if (spi_ti->spsr_read)
        {
            spi_ti->spsr &= ~spi_ti->spsr_read;
            spi_ti->spsr_read = 0;
        }

        if (spi->tcnt != 0)
        {
            spi_ti->spsr |= mask_WCOL;
        }

        spi->spdr = val;

        /* When the user writes to SPDR, a callback is installed for either
           clock generated increments or externally generated increments. The
           two incrememtor callback are mutally exclusive, only one or the
           other can be installed at any given instant. */

        switch ((spi_ti->spcr) & (mask_SPR0 | mask_SPR1))
        {
            case SPI_CK_4:
                spi->divisor = 4;
                break;
            case SPI_CK_16:
                spi->divisor = 16;
                break;
            case SPI_CK_64:
                spi->divisor = 64;
                break;
            case SPI_CK_128:
                spi->divisor = 128;
                break;
            default:
                avr_error ("The impossible happened!");
        }

        /* install the clock incrementor callback (with flair!) */
        if (spi->clk_cb == NULL)
        {
            cb = callback_new (spi_clk_incr_cb, (AvrClass *)spi);
            spi->clk_cb = cb;
            avr_core_clk_cb_add ((AvrCore *)vdev_get_core ((VDevice *)spi),
                                 cb);
        }
        spi->tcnt = 8;          /* set up timer for 8 clocks */
        spi->spdr_in = spi_port_rd (addr);
    }
    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
spi_reset (VDevice *dev)
{
    SPI_T *spi = (SPI_T *)dev;

    spi->clk_cb = NULL;

    spi->spdr = 0;
    spi->tcnt = 0;

    spi->divisor = 0;
}

static int
spi_clk_incr_cb (uint64_t ck, AvrClass *data)
{
    SPI_T *spi = (SPI_T *)data;
    uint8_t last = spi->tcnt;
    SPIIntr_T *spi_ti;

    spi_ti =
        (SPIIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                vdev_get_core ((VDevice *)
                                                               spi),
                                                spi->rel_addr);

    if (spi->clk_cb == NULL)
        return CB_RET_REMOVE;

    if (spi->divisor <= 0)
        avr_error ("Bad divisor value: %d", spi->divisor);

    /* Decrement clock if ck is a mutliple of divisor. Since divisor is always
       a power of 2, it's much faster to do the bitwise AND instead of using
       the integer modulus operator (%). */
    spi->tcnt -= ((ck & (spi->divisor - 1)) == 0);

    if (spi->tcnt != last)      /* we've changed the counter */
    {
        if (spi->tcnt == 0)
        {
            spi_ti->spsr |= mask_SPIF; /* spdr is not guaranteed until
                                          operation complete */
            spi_port_wr (spi->spdr); /* tell what we wrote */
            spi->spdr = spi->spdr_in; /* update spdr to what we read */

            spi->clk_cb = NULL;
            return CB_RET_REMOVE;
        }
    }

    return CB_RET_RETAIN;
}

/* FIXME: TRoth/2003-11-28: These will eventually need to be plugged into an
   external connection interface. */

uint8_t
spi_port_rd (int addr)
{
    int data;
    char line[80];

    while (1)
    {
        fprintf (stderr,
                 "\nEnter a byte of hex data to read into the SPI at"
                 " address 0x%04x: ", addr);

        /* try to read in a line of input */
        if (fgets (line, sizeof (line), stdin) == NULL)
            continue;

        /* try to parse the line for a byte of data */
        if (sscanf (line, "%x\n", &data) != 1)
            continue;

        break;
    }

    return (uint8_t) (data & 0xff);
}

void
spi_port_wr (uint8_t val)
{
    fprintf (stderr, "wrote 0x%02x to SPI\n", val);
}
