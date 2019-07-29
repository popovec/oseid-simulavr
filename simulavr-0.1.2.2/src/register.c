/*
 * $Id: register.c,v 1.35 2004/01/30 07:09:56 troth Exp $
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
#include "intvects.h"

/****************************************************************************\
 *
 * Status Register Methods.
 *
\****************************************************************************/

static inline uint8_t sreg_read (VDevice *dev, int addr);
static inline void sreg_write (VDevice *dev, int addr, uint8_t val);
static inline void sreg_reset (VDevice *dev);
static void sreg_add_addr (VDevice *dev, int addr, char *name, int rel_addr,
                           void *data);

VDevice *
sreg_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)sreg_new ();
}

SREG *
sreg_new (void)
{
    SREG *sreg;

    sreg = avr_new (SREG, 1);
    sreg_construct (sreg);
    class_overload_destroy ((AvrClass *)sreg, sreg_destroy);

    return sreg;
}

void
sreg_construct (SREG *sreg)
{
    if (sreg == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)sreg, sreg_read, sreg_write, sreg_reset,
                    sreg_add_addr);

    sreg->sreg.reg = 0;
}

void
sreg_destroy (void *sreg)
{
    if (sreg == NULL)
        return;

    vdev_destroy (sreg);
}

extern inline uint8_t sreg_get (SREG *sreg);

extern inline void sreg_set (SREG *sreg, uint8_t val);

extern inline uint8_t sreg_get_bit (SREG *sreg, int bit);

extern inline void sreg_set_bit (SREG *sreg, int bit, int val);

static inline uint8_t
sreg_read (VDevice *dev, int addr)
{
    return sreg_get ((SREG *)dev);
}

static inline void
sreg_write (VDevice *dev, int addr, uint8_t val)
{
    sreg_set ((SREG *)dev, val);
}

static inline void
sreg_reset (VDevice *dev)
{
    display_io_reg (SREG_IO_REG, 0);
    ((SREG *)dev)->sreg.reg = 0;
}

static void
sreg_add_addr (VDevice *dev, int addr, char *name, int rel_addr, void *data)
{
    /* Nothing to do here. */
}


/****************************************************************************\
 *
 * General Purpose Working Register (gpwr) Methods.
 *
\****************************************************************************/

static inline uint8_t gpwr_read (VDevice *dev, int addr);
static inline void gpwr_write (VDevice *dev, int addr, uint8_t val);
static inline void gpwr_reset (VDevice *dev);

GPWR *
gpwr_new (void)
{
    GPWR *gpwr;

    gpwr = avr_new (GPWR, 1);
    gpwr_construct (gpwr);
    class_overload_destroy ((AvrClass *)gpwr, gpwr_destroy);

    return gpwr;
}

void
gpwr_construct (GPWR *gpwr)
{
    if (gpwr == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)gpwr, gpwr_read, gpwr_write, gpwr_reset, NULL);

    gpwr_reset ((VDevice *)gpwr);
}

void
gpwr_destroy (void *gpwr)
{
    if (gpwr == NULL)
        return;

    vdev_destroy (gpwr);
}

extern inline uint8_t gpwr_get (GPWR *gpwr, int reg);

extern inline void gpwr_set (GPWR *gpwr, int reg, uint8_t val);

static inline uint8_t
gpwr_read (VDevice *dev, int addr)
{
    return gpwr_get ((GPWR *)dev, addr);
}

static inline void
gpwr_write (VDevice *dev, int addr, uint8_t val)
{
    gpwr_set ((GPWR *)dev, addr, val);
}

static void
gpwr_reset (VDevice *dev)
{
    int i;

    for (i = 0; i < GPWR_SIZE; i++)
        gpwr_set ((GPWR *)dev, i, 0);
}

/****************************************************************************\
 *
 * ACSR(VDevice) : Analog Comparator Control and Status Register Definition
 *
\****************************************************************************/

static uint8_t acsr_read (VDevice *dev, int addr);
static void acsr_write (VDevice *dev, int addr, uint8_t val);
static void acsr_reset (VDevice *dev);

ACSR *
acsr_new (uint8_t func_mask)
{
    ACSR *acsr;

    acsr = avr_new (ACSR, 1);
    acsr_construct (acsr, func_mask);
    class_overload_destroy ((AvrClass *)acsr, acsr_destroy);

    return acsr;
}

void
acsr_construct (ACSR *acsr, uint8_t func_mask)
{
    if (acsr == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)acsr, acsr_read, acsr_write, acsr_reset,
                    vdev_def_AddAddr);

    acsr->func_mask = func_mask;
    acsr->acsr = 0;
}

void
acsr_destroy (void *acsr)
{
    if (acsr == NULL)
        return;

    vdev_destroy (acsr);
}

int
acsr_get_bit (ACSR *acsr, int bit)
{
    return !!(acsr->acsr & acsr->func_mask & (1 << bit));
}

void
acsr_set_bit (ACSR *acsr, int bit, int val)
{
    /* the ACO bit is read only */
    acsr->acsr =
        set_bit_in_byte (acsr->acsr, bit,
                         val) & acsr->func_mask & ~(mask_ACO);
}

static uint8_t
acsr_read (VDevice *dev, int addr)
{
    ACSR *reg = (ACSR *)dev;

    return (reg->acsr & reg->func_mask);
}

static void
acsr_write (VDevice *dev, int addr, uint8_t val)
{
    ACSR *reg = (ACSR *)dev;

    /* the ACO bit is read only */
    reg->acsr = (val & reg->func_mask & ~(mask_ACO));
}

static void
acsr_reset (VDevice *dev)
{
    ((ACSR *)dev)->acsr = 0;
}

/****************************************************************************\
 *
 * MCUCR(VDevice) : MCU general control register
 *
\****************************************************************************/

static uint8_t mcucr_read (VDevice *dev, int addr);
static void mcucr_write (VDevice *dev, int addr, uint8_t val);
static void mcucr_reset (VDevice *dev);

MCUCR *
mcucr_new (uint8_t func_mask)
{
    MCUCR *mcucr;

    mcucr = avr_new (MCUCR, 1);
    mcucr_construct (mcucr, func_mask);
    class_overload_destroy ((AvrClass *)mcucr, mcucr_destroy);

    return mcucr;
}

void
mcucr_construct (MCUCR *mcucr, uint8_t func_mask)
{
    if (mcucr == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)mcucr, mcucr_read, mcucr_write, mcucr_reset,
                    vdev_def_AddAddr);

    mcucr->func_mask = func_mask;
    mcucr->mcucr = 0;
}

void
mcucr_destroy (void *mcucr)
{
    if (mcucr == NULL)
        return;

    vdev_destroy (mcucr);
}

int
mcucr_get_bit (MCUCR *reg, int bit)
{
    return !!(reg->mcucr & reg->func_mask & (1 << bit));
}

void
mcucr_set_bit (MCUCR *reg, int bit, int val)
{
    reg->mcucr = set_bit_in_byte (reg->mcucr, bit, val) & reg->func_mask;
}

static uint8_t
mcucr_read (VDevice *dev, int addr)
{
    MCUCR *reg = (MCUCR *)dev;

    return (reg->mcucr & reg->func_mask);
}

static void
mcucr_write (VDevice *dev, int addr, uint8_t val)
{
    MCUCR *reg = (MCUCR *)dev;

    reg->mcucr = (val & reg->func_mask);
}

static void
mcucr_reset (VDevice *dev)
{
    ((MCUCR *)dev)->mcucr = 0;
}

/****************************************************************************\
 *
 * WDTCR(VDevice) : Watchdog timer control register
 *
\****************************************************************************/

/*  static int    wdtcr_get_bit   ( WDTCR *wdtcr, int bit ); */
static void wdtcr_set_bit (WDTCR *wdtcr, int bit, int val);

static uint8_t wdtcr_read (VDevice *dev, int addr);
static void wdtcr_write (VDevice *dev, int addr, uint8_t val);
static void wdtcr_reset (VDevice *dev);

static int wdtcr_timer_cb (uint64_t time, AvrClass *data);
static int wdtcr_toe_clr_cb (uint64_t time, AvrClass *data);

WDTCR *
wdtcr_new (uint8_t func_mask)
{
    WDTCR *wdtcr;

    wdtcr = avr_new (WDTCR, 1);
    wdtcr_construct (wdtcr, func_mask);
    class_overload_destroy ((AvrClass *)wdtcr, wdtcr_destroy);

    return wdtcr;
}

void
wdtcr_construct (WDTCR *wdtcr, uint8_t func_mask)
{
    if (wdtcr == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)wdtcr, wdtcr_read, wdtcr_write, wdtcr_reset,
                    vdev_def_AddAddr);

    wdtcr->func_mask = func_mask;

    wdtcr_reset ((VDevice *)wdtcr);
}

void
wdtcr_destroy (void *wdtcr)
{
    if (wdtcr == NULL)
        return;

    vdev_destroy (wdtcr);
}

/*
 * Function wdtcr_update: Called when the WDR instruction is issued
 */
void
wdtcr_update (WDTCR *wdtcr)
{
    wdtcr->last_WDR = get_program_time ();
}

#if 0                           /* This doesn't seem to be used anywhere. */
static int
wdtcr_get_bit (WDTCR *reg, int bit)
{
    return !!(reg->wdtcr & reg->func_mask & (1 << bit));
}
#endif

static void
wdtcr_set_bit (WDTCR *reg, int bit, int val)
{
    reg->wdtcr = set_bit_in_byte (reg->wdtcr, bit, val) & reg->func_mask;
}

static uint8_t
wdtcr_read (VDevice *dev, int addr)
{
    WDTCR *reg = (WDTCR *)dev;

    return (reg->wdtcr & reg->func_mask);
}

/*
 * FIXME: Should the wdtcr->toe_clk counter be reset to TOE_CLKS
 * every time a WDTOE is set 1? I.E. does the hw reset the 4 cycle
 * counter every time WDTOE is set? This code assumes it does.
 */
static void
wdtcr_write (VDevice *dev, int addr, uint8_t val)
{
    WDTCR *reg = (WDTCR *)dev;
    uint8_t wd_enabled = (reg->wdtcr & mask_WDE);

    CallBack *cb;

    if (reg->func_mask & mask_WDTOE)
    {                           /* Device has WDTOE functionality */

        if ((reg->wdtcr & mask_WDE) && !(reg->wdtcr & mask_WDTOE))
        {
            /* WDE can _NOT_ be cleared if WDTOE is zero */
            val |= mask_WDE;
        }

        if (val & mask_WDTOE)
        {                       /* program has set WDTOE */
            reg->toe_clk = TOE_CLKS;

            /* create and install the callback if it not already installed */
            if (reg->toe_cb == NULL)
            {
                cb = callback_new (wdtcr_toe_clr_cb, (AvrClass *)reg);
                reg->toe_cb = cb;
                avr_core_clk_cb_add ((AvrCore *)vdev_get_core (dev), cb);
            }
        }
    }

    reg->wdtcr = (val & reg->func_mask);

    if ((wd_enabled == 0) && (val & mask_WDE) && (reg->timer_cb == NULL))
    {
        /* install the WD timer callback */
        cb = callback_new (wdtcr_timer_cb, (AvrClass *)reg);
        reg->timer_cb = cb;
        avr_core_async_cb_add ((AvrCore *)vdev_get_core (dev), cb);
    }

    if (wd_enabled && ((val & mask_WDE) == 0) && (reg->timer_cb != NULL))
    {
        /* tell callback to remove itself */
        reg->timer_cb = NULL;
    }
}

static void
wdtcr_reset (VDevice *dev)
{
    WDTCR *wdtcr = (WDTCR *)dev;

    wdtcr->wdtcr = 0;

    wdtcr->last_WDR = get_program_time (); /* FIXME: This might not be the
                                              right thing to do */
    wdtcr->timer_cb = NULL;

    wdtcr->toe_clk = TOE_CLKS;
    wdtcr->toe_cb = NULL;
}

/*
 * Timer callback will remove itself if wdtcr->timer_cb is set NULL.
 */
static int
wdtcr_timer_cb (uint64_t time, AvrClass *data)
{
    WDTCR *wdtcr = (WDTCR *)data;
    uint64_t time_diff;
    uint64_t time_out;

    if (wdtcr->timer_cb == NULL)
        return CB_RET_REMOVE;

    time_diff = time - wdtcr->last_WDR;
    time_out = TIMEOUT_BASE * (1 << (wdtcr->wdtcr & mask_WDP));

    if (time_diff > time_out)
    {
        avr_warning ("watchdog reset: time %lld\n", time_diff);

        /* reset the device, we timed out */
        avr_core_irq_raise ((AvrCore *)vdev_get_core ((VDevice *)wdtcr),
                            irq_vect_table_index (RESET));
    }

    return CB_RET_RETAIN;
}

/*
 * The WDTOE is cleared by hardware after TOE_CLKS clock cycles.
 */
static int
wdtcr_toe_clr_cb (uint64_t time, AvrClass *data)
{
    WDTCR *wdtcr = (WDTCR *)data;

    if (wdtcr->toe_cb == NULL)
        return CB_RET_REMOVE;

    if (wdtcr->toe_clk > 0)
    {
        wdtcr->toe_clk--;
    }
    else
    {
        wdtcr_set_bit (wdtcr, bit_WDTOE, 0);
        wdtcr->toe_cb = NULL;   /* So we know that cb is not installed */
        return CB_RET_REMOVE;
    }

    return CB_RET_RETAIN;
}

/****************************************************************************\
 *
 * RAMPZ(VDevice) : The RAMPZ register used by ELPM and ESPM instructions.
 *
 * Even though the rampz register is not available to all devices, we will
 * install it for all in the simulator. It just so much easier that way and
 * we're already assuming that the compiler generated the correct code in
 * many places anyways. Let's see if we get bit.
 *
\****************************************************************************/

static uint8_t rampz_read (VDevice *dev, int addr);
static void rampz_write (VDevice *dev, int addr, uint8_t val);
static void rampz_reset (VDevice *dev);

VDevice *
rampz_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)rampz_new ();
}

RAMPZ *
rampz_new (void)
{
    RAMPZ *rampz;

    rampz = avr_new (RAMPZ, 1);
    rampz_construct (rampz);
    class_overload_destroy ((AvrClass *)rampz, rampz_destroy);

    return rampz;
}

void
rampz_construct (RAMPZ *rampz)
{
    if (rampz == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)rampz, rampz_read, rampz_write, rampz_reset,
                    vdev_def_AddAddr);

    rampz->reg = 0;
}

void
rampz_destroy (void *rampz)
{
    if (rampz == NULL)
        return;

    vdev_destroy (rampz);
}

uint8_t
rampz_get (RAMPZ *rampz)
{
    return rampz->reg;
}

void
rampz_set (RAMPZ *rampz, uint8_t val)
{
    rampz->reg = val;
}

static uint8_t
rampz_read (VDevice *dev, int addr)
{
    return rampz_get ((RAMPZ *)dev);
}

static void
rampz_write (VDevice *dev, int addr, uint8_t val)
{
    rampz_set ((RAMPZ *)dev, val);
}

static void
rampz_reset (VDevice *dev)
{
    display_io_reg (RAMPZ_IO_REG, 0);
    ((RAMPZ *)dev)->reg = 0;
}
