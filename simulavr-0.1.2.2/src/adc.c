/*
 * $Id: adc.c,v 1.4 2004/03/13 19:55:34 troth Exp $
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
 * \file adc.c
 * \brief Module to simulate the AVR's ADC module.
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
#include "adc.h"

#include "avrcore.h"

#include "intvects.h"

/****************************************************************************\
 *
 * ADC Interrupts 
 *
\****************************************************************************/

static void adc_iadd_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                           void *data);
static uint8_t adc_intr_read (VDevice *dev, int addr);
static void adc_intr_write (VDevice *dev, int addr, uint8_t val);
static void adc_intr_reset (VDevice *dev);
static int adc_intr_cb (uint64_t time, AvrClass *data);
static int adc_clk_incr_cb (uint64_t ck, AvrClass *data);

/** \brief Allocate a new ADC interrupt */

VDevice *
adc_int_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)adc_intr_new (addr, name, rel_addr);
}

ADCIntr_T *
adc_intr_new (int addr, char *name, int rel_addr)
{
    ADCIntr_T *adc;

    adc = avr_new (ADCIntr_T, 1);
    adc_intr_construct (adc, addr, name, rel_addr);
    class_overload_destroy ((AvrClass *)adc, adc_intr_destroy);

    return adc;
}

/** \brief Constructor for adc interrupt object. */

void
adc_intr_construct (ADCIntr_T *adc, int addr, char *name, int rel_addr)
{
    if (adc == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)adc, adc_intr_read, adc_intr_write,
                    adc_intr_reset, adc_iadd_addr);

    if (rel_addr)
        adc->rel_addr = rel_addr;
    adc_iadd_addr ((VDevice *)adc, addr, name, 0, NULL);

    adc_intr_reset ((VDevice *)adc);
}

static void
adc_iadd_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    ADCIntr_T *adc = (ADCIntr_T *)vdev;

    if (strncmp ("ADCSR", name, 5) == 0)
    {
        adc->adcsr_addr = addr;
    }

    else if (strncmp ("ADMUX", name, 5) == 0)
    {
        adc->admux_addr = addr;
    }

    else
    {
        avr_error ("invalid ADC register name: '%s' @ 0x%04x", name, addr);
    }
}

/** \brief Destructor for adc interrupt object. */

void
adc_intr_destroy (void *adc)
{
    if (adc == NULL)
        return;

    vdev_destroy (adc);
}

static uint8_t
adc_intr_read (VDevice *dev, int addr)
{
    ADCIntr_T *adc = (ADCIntr_T *)dev;

    if (addr == adc->adcsr_addr)
        return (adc->adcsr);

    else if (addr == adc->admux_addr)
        return (adc->admux);

    else
        avr_error ("Bad address: 0x%04x", addr);

    return 0;                   /* will never get here */
}

static void
adc_intr_write (VDevice *dev, int addr, uint8_t val)
{
    ADCIntr_T *adc = (ADCIntr_T *)dev;
    CallBack *cb;
    ADC_T *adc_d;

    adc_d =
        (ADC_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                            vdev_get_core ((VDevice *)adc),
                                            adc->rel_addr);

    if (addr == adc->adcsr_addr)
    {
        if (val & mask_ADIF)    /* clears interrupt flag */
            adc->adcsr = val & ~mask_ADIF;
        else
            adc->adcsr = val;

        if ((val & mask_ADSC) && (val & mask_ADEN))
        {
            if ((adc->intr_cb == NULL))
            {
                /* we need to install the intr_cb function */
                cb = callback_new (adc_intr_cb, (AvrClass *)adc);
                adc->intr_cb = cb;
                avr_core_async_cb_add ((AvrCore *)vdev_get_core (dev), cb);
            }
            if ((adc_d->clk_cb == NULL))
            {
                /* we need to install the clk_cb function */
                cb = callback_new (adc_clk_incr_cb, (AvrClass *)adc_d);
                adc_d->clk_cb = cb;
                avr_core_clk_cb_add ((AvrCore *)
                                     vdev_get_core ((VDevice *)adc_d), cb);
            }
            adc_d->adc_count = 13;
            switch ((adc->adcsr) & (mask_ADPS0 | mask_ADPS1 | mask_ADPS2))
            {
                case ADC_CK_0:
                case ADC_CK_2:
                    adc_d->divisor = 2;
                    break;
                case ADC_CK_4:
                    adc_d->divisor = 4;
                    break;
                case ADC_CK_8:
                    adc_d->divisor = 8;
                    break;
                case ADC_CK_16:
                    adc_d->divisor = 16;
                    break;
                case ADC_CK_32:
                    adc_d->divisor = 32;
                    break;
                case ADC_CK_64:
                    adc_d->divisor = 64;
                    break;
                case ADC_CK_128:
                    adc_d->divisor = 128;
                    break;
                default:
                    avr_error ("The impossible happened!");
            }
        }
        else
        {
            adc->intr_cb = NULL; /* no interrupt are enabled, remove
                                    the callback */
        }
    }

    else if (addr == adc->admux_addr)
    {
        adc->admux = val;
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
adc_intr_reset (VDevice *dev)
{
    ADCIntr_T *adc = (ADCIntr_T *)dev;

    adc->intr_cb = NULL;

    adc->adcsr = 0;
    adc->admux = 0;
}

static int
adc_intr_cb (uint64_t time, AvrClass *data)
{
    ADCIntr_T *adc = (ADCIntr_T *)data;

    if (adc->intr_cb == NULL)
        return CB_RET_REMOVE;

    if ((adc->adcsr & mask_ADEN) && (adc->adcsr & mask_ADIE)
        && (adc->adcsr & mask_ADIF))
    {
        /* an enabled interrupt occured */
        AvrCore *core = (AvrCore *)vdev_get_core ((VDevice *)adc);
        avr_core_irq_raise (core, irq_vect_table_index (ADC));
        adc->adcsr &= ~mask_ADIF;
    }

    return CB_RET_RETAIN;
}

/****************************************************************************\
 *
 * ADC  
 *
\****************************************************************************/

static void adc_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                          void *data);
static uint8_t adc_read (VDevice *dev, int addr);
static void adc_write (VDevice *dev, int addr, uint8_t val);
static void adc_reset (VDevice *dev);

/** \brief Allocate a new ADC structure. */

VDevice *
adc_create (int addr, char *name, int rel_addr, void *data)
{
    uint8_t *data_ptr = (uint8_t *) data;
    if (data)
        return (VDevice *)adc_new (addr, name, (uint8_t) * data_ptr,
                                   rel_addr);
    else
        avr_error ("Attempted A/D create with NULL data pointer");
    return 0;
}

ADC_T *
adc_new (int addr, char *name, uint8_t uier, int rel_addr)
{
    ADC_T *adc;

    adc = avr_new (ADC_T, 1);
    adc_construct (adc, addr, name, uier, rel_addr);
    class_overload_destroy ((AvrClass *)adc, adc_destroy);

    return adc;
}

/** \brief Constructor for ADC object. */

void
adc_construct (ADC_T *adc, int addr, char *name, uint8_t uier, int rel_addr)
{
    if (adc == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)adc, adc_read, adc_write, adc_reset,
                    adc_add_addr);

    if (rel_addr)
        adc->rel_addr = rel_addr;
    adc_add_addr ((VDevice *)adc, addr, name, 0, NULL);

    adc_reset ((VDevice *)adc);
    adc->u_divisor = uier ? 12 : 1;
}
static void
adc_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    ADC_T *adc = (ADC_T *)vdev;

    if (strncmp ("ADCL", name, 4) == 0)
    {
        adc->adcl_addr = addr;
    }

    else if (strncmp ("ADCH", name, 4) == 0)
    {
        adc->adch_addr = addr;
    }

    else
    {
        avr_error ("invalid ADC register name: '%s' @ 0x%04x", name, addr);
    }
}

/** \brief Destructor for ADC object. */

void
adc_destroy (void *adc)
{
    if (adc == NULL)
        return;

    vdev_destroy (adc);
}

static uint8_t
adc_read (VDevice *dev, int addr)
{
    ADC_T *adc = (ADC_T *)dev;

    if (addr == adc->adcl_addr)
        return adc->adcl;

    else if (addr == adc->adch_addr)
        return adc->adch;

    else
        avr_error ("Bad address: 0x%04x", addr);

    return 0;                   /* will never get here */
}

static void
adc_write (VDevice *dev, int addr, uint8_t val)
{
    avr_error ("Bad ADC write address: 0x%04x", addr);
}

static void
adc_reset (VDevice *dev)
{
    ADC_T *adc = (ADC_T *)dev;

    adc->clk_cb = NULL;

    adc->adcl = 0;
    adc->adch = 0;

    adc->adc_count = 0;
    adc->adc_in = 0;
    adc->divisor = 0;
}

static int
adc_clk_incr_cb (uint64_t ck, AvrClass *data)
{
    ADC_T *adc = (ADC_T *)data;
    uint8_t last = adc->adc_count;
    ADCIntr_T *adc_ti;

    adc_ti =
        (ADCIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                vdev_get_core ((VDevice *)
                                                               adc),
                                                adc->rel_addr);

    if (adc->clk_cb == NULL)
        return CB_RET_REMOVE;

    if (adc->divisor <= 0)
        avr_error ("Bad divisor value: %d", adc->divisor);

    /* decrement clock if ck is a multiple of divisor */
    adc->adc_count -= ((ck % (adc->divisor * adc->u_divisor)) == 0);

    if (adc->adc_count != last) /* we've changed the counter */
    {
        if (adc->adc_count == 0)
        {
            adc_ti->adcsr |= mask_ADIF;
            adc_ti->adcsr &= ~mask_ADSC;
            adc->adc_in = adc_port_rd (adc_ti->admux);
            adc->adcl = (adc->adc_in) & 0xff; /* update adcl to what we
                                                 read */
            adc->adch = ((adc->adc_in) >> 8) & 0x03; /* update adch */
            if (adc_ti->adcsr & mask_ADFR) /* free running mode */
                adc->adc_count = 13;
            else
            {
                adc->clk_cb = NULL;
                return CB_RET_REMOVE;
            }
        }
    }
    return CB_RET_RETAIN;
}

/* FIXME: TRoth/2003-11-29: These will eventually need to be plugged into an
   external connection interface. */

uint16_t
adc_port_rd (uint8_t mux)
{
    int data;
    char line[80];

    while (1)
    {
        fprintf (stderr, "\nEnter data to read into the ADC for channel %d: ",
                 mux);

        /* try to read in a line of input */
        if (fgets (line, sizeof (line), stdin) == NULL)
            continue;

        /* try to parse the line for a byte of data */
        if (sscanf (line, "%d\n", &data) != 1)
            continue;

        break;
    }
    return (uint16_t) (data & 0x3ff);
}

void
adc_port_wr (uint8_t val)
{
    fprintf (stderr, "wrote 0x%02x to ADC\n", val);
}
