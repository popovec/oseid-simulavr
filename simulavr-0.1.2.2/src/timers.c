/*
 * $Id: timers.c,v 1.15 2004/03/13 19:55:34 troth Exp $
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
 * \file timers.c
 * \brief Module to simulate the AVR's on-board timer/counters.
 *
 * This currently only implements the timer/counter 0.
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

#include "intvects.h"

#ifndef DOXYGEN

/* *INDENT-OFF* */

Timer16Def global_timer16_defs[] = {
    {
        .timer_name = "Timer1",
        .tcnth_name = "TCNT1H",
        .tcntl_name = "TCNT1L",
        .tccra_name = "TCCR1A",
        .tccrb_name = "TCCR1B",
        .base = 0x4c,
        .tof = bit_TOV1,
        .ocf_a = bit_OCF1A,
        .ocf_b = bit_OCF1B,
        .ocf_c = 8
    }
};

OCReg16Def global_ocreg16_defs[] = {
    {
        .ocrdev_name = "OCR1A",
        .ocrl_name = "OCR1AL",
        .ocrh_name = "OCR1AH",
        .base = 0x4a
    },
    {
        .ocrdev_name = "OCR1B",
        .ocrl_name = "OCR1BL",
        .ocrh_name = "OCR1BH",
        .base = 0x48
    },
    {
        .ocrdev_name = "OCR1C",
        .ocrl_name = "OCR1CL",
        .ocrh_name = "OCR1CH",
        .base = 0x78},
    {
        .ocrdev_name = "OCR3A",
        .ocrl_name = "OCR3AL",
        .ocrh_name = "OCR3A",
        .base = 0x86
    },
    {
        .ocrdev_name = "OCR3B",
        .ocrl_name = "OCR3BL",
        .ocrh_name = "OCR3BH",
        .base = 0x84
    },
    {
        .ocrdev_name = "OCR3C",
        .ocrl_name = "OCR3CL",
        .ocrh_name = "OCR3CH",
        .base = 0x82
    }
};
/* *INDENT-ON* */

#endif /* not DOXYGEN */

/****************************************************************************\
 *
 * Timer/Counter 0 
 *
\****************************************************************************/

static void timer_iadd_addr (VDevice *vdev, int addr, char *name,
                             int rel_addr, void *data);
static uint8_t timer_intr_read (VDevice *dev, int addr);
static void timer_intr_write (VDevice *dev, int addr, uint8_t val);
static void timer_intr_reset (VDevice *dev);
static int timer_intr_cb (uint64_t time, AvrClass *data);

/** \brief Allocate a new timer interrupt */

VDevice *
timer_int_create (int addr, char *name, int rel_addr, void *data)
{
    uint8_t *func_mask = (uint8_t *) data;
    if (data)
        return (VDevice *)timer_intr_new (addr, name, *func_mask);
    else
        avr_error ("Attempted timer interrupt create with NULL data pointer");
    return 0;
}

TimerIntr_T *
timer_intr_new (int addr, char *name, uint8_t func_mask)
{
    TimerIntr_T *ti;

    ti = avr_new (TimerIntr_T, 1);
    timer_intr_construct (ti, addr, name, func_mask);
    class_overload_destroy ((AvrClass *)ti, timer_intr_destroy);

    return ti;
}

/** \brief Constructor for timer interrupt object. */

void
timer_intr_construct (TimerIntr_T *ti, int addr, char *name,
                      uint8_t func_mask)
{
    if (ti == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)ti, timer_intr_read, timer_intr_write,
                    timer_intr_reset, timer_iadd_addr);

    ti->func_mask = func_mask;

    timer_iadd_addr ((VDevice *)ti, addr, name, 0, NULL);

    timer_intr_reset ((VDevice *)ti);
}

static void
timer_iadd_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                 void *data)
{
    TimerIntr_T *ti = (TimerIntr_T *)vdev;

    if (strncmp ("TIFR", name, 4) == 0)
    {
        ti->tifr_addr = addr;
    }

    else if (strncmp ("TIMSK", name, 5) == 0)
    {
        ti->timsk_addr = addr;
    }

    else
    {
        avr_error ("invalid Timer Interrupt register name: '%s' @ 0x%04x",
                   name, addr);
    }
}

/** \brief Destructor for timer interrupt object. */

void
timer_intr_destroy (void *ti)
{
    if (ti == NULL)
        return;

    vdev_destroy (ti);
}

static uint8_t
timer_intr_read (VDevice *dev, int addr)
{
    TimerIntr_T *ti = (TimerIntr_T *)dev;

    if (addr == ti->timsk_addr)
    {
        return (ti->timsk & ti->func_mask);
    }

    else if (addr == ti->tifr_addr)
    {
        return (ti->tifr & ti->func_mask);
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
timer_intr_write (VDevice *dev, int addr, uint8_t val)
{
    TimerIntr_T *ti = (TimerIntr_T *)dev;
    CallBack *cb;

    if (addr == ti->timsk_addr)
    {
        ti->timsk = (val & ti->func_mask);
        if (ti->timsk == 0)
        {
            ti->intr_cb = NULL; /* no interrupt are enabled, remove the
                                   callback */
        }
        else if (ti->intr_cb == NULL)
        {
            /* we need to install the intr_cb function */
            cb = callback_new (timer_intr_cb, (AvrClass *)ti);
            ti->intr_cb = cb;
            avr_core_async_cb_add ((AvrCore *)vdev_get_core (dev), cb);
        }
    }

    else if (addr == ti->tifr_addr)
    {
        ti->tifr &= ~(val & ti->func_mask);
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
timer_intr_reset (VDevice *dev)
{
    TimerIntr_T *ti = (TimerIntr_T *)dev;

    ti->intr_cb = NULL;

    ti->timsk = 0;
    ti->tifr = 0;
}

static int
timer_intr_cb (uint64_t time, AvrClass *data)
{
    TimerIntr_T *ti = (TimerIntr_T *)data;
    uint8_t intrs = ti->timsk & ti->tifr & ti->func_mask;

    if (ti->intr_cb == NULL)
        return CB_RET_REMOVE;

    if (intrs)
    {
        AvrCore *core = (AvrCore *)vdev_get_core ((VDevice *)ti);

        /*
         * FIXME: Once an irq has been raised, the flag should be cleared,
         *   _BUT_ should it be done here? Might be a problem if there are
         *   many interrupts pending and then the user wants to clear one.
         */

        if (intrs & mask_TOV0)
        {
            avr_core_irq_raise (core, irq_vect_table_index (TIMER0_OVF));
            ti->tifr &= ~mask_TOV0;
        }
        else if (intrs & mask_ICF1)
        {
            avr_core_irq_raise (core, irq_vect_table_index (TIMER1_CAPT));
            ti->tifr &= ~mask_ICF1;
        }
        else if (intrs & mask_OCF1B)
        {
            avr_core_irq_raise (core, irq_vect_table_index (TIMER1_COMPB));
            ti->tifr &= ~mask_OCF1B;
        }
        else if (intrs & mask_OCF1A)
        {
            avr_core_irq_raise (core, irq_vect_table_index (TIMER1_COMPA));
            ti->tifr &= ~mask_OCF1A;
        }
        else if (intrs & mask_TOV1)
        {
            avr_core_irq_raise (core, irq_vect_table_index (TIMER1_OVF));
            ti->tifr &= ~mask_TOV1;
        }
        else
        {
            avr_error ("An invalid interrupt was flagged");
        }
    }

    return CB_RET_RETAIN;
}

/****************************************************************************\
 *
 * Timer/Counter 0 
 *
\****************************************************************************/

static void timer0_add_addr (VDevice *vdev, int addr, char *name,
                             int rel_addr, void *data);
static uint8_t timer0_read (VDevice *dev, int addr);
static void timer0_write (VDevice *dev, int addr, uint8_t val);
static void timer0_reset (VDevice *dev);
static int timer0_clk_incr_cb (uint64_t ck, AvrClass *data);

/** \brief Allocate a new timer/counter 0. */

VDevice *
timer0_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)timer0_new (addr, name, rel_addr);
}

Timer0_T *
timer0_new (int addr, char *name, int rel_addr)
{
    Timer0_T *timer;

    timer = avr_new (Timer0_T, 1);
    timer0_construct (timer, addr, name, rel_addr);
    class_overload_destroy ((AvrClass *)timer, timer0_destroy);

    return timer;
}

/** \brief Constructor for timer/counter 0 object. */

void
timer0_construct (Timer0_T *timer, int addr, char *name, int rel_addr)
{
    if (timer == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)timer, timer0_read, timer0_write, timer0_reset,
                    timer0_add_addr);

    timer0_add_addr ((VDevice *)timer, addr, name, 0, NULL);
    if (rel_addr)
        timer->related_addr = rel_addr;
    timer0_reset ((VDevice *)timer);
}

/** \brief Destructor for timer/counter 0 object. */

void
timer0_destroy (void *timer)
{
    if (timer == NULL)
        return;

    vdev_destroy (timer);
}

static void
timer0_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                 void *data)
{
    Timer0_T *ti = (Timer0_T *)vdev;

    if (strncmp ("TCNT", name, 4) == 0)
    {
        ti->tcnt_addr = addr;
    }

    else if (strncmp ("TCCR", name, 4) == 0)
    {
        ti->tccr_addr = addr;
    }

    else
    {
        avr_error ("invalid Timer register name: '%s' @ 0x%04x", name, addr);
    }
}

static uint8_t
timer0_read (VDevice *dev, int addr)
{
    Timer0_T *timer = (Timer0_T *)dev;

    if (addr == timer->tcnt_addr)
        return timer->tcnt;

    else if (addr == timer->tccr_addr)
        return timer->tccr;

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
timer0_write (VDevice *dev, int addr, uint8_t val)
{
    Timer0_T *timer = (Timer0_T *)dev;
    CallBack *cb;

    if (addr == timer->tcnt_addr)
    {
        timer->tcnt = val;
    }

    else if (addr == timer->tccr_addr)
    {
        /*
         * When the user writes toe TCCR, a callback is installed for either
         * clock generated increments or externally generated increments. The
         * two incrememtor callback are mutally exclusive, only one or the 
         * other can be installed at any given instant.
         */

        /* timer 0 only has clock select function. */
        timer->tccr = val & mask_CS;

        switch (timer->tccr)
        {
            case CS_STOP:
                /* stop either of the installed callbacks */
                timer->clk_cb = timer->ext_cb = NULL;
                timer->divisor = 0;
                return;
            case CS_EXT_FALL:
            case CS_EXT_RISE:
                /* FIXME: not implemented yet */
                avr_error ("external timer/counter sources is not implemented"
                           " yet");
                return;
            case CS_CK:
                timer->divisor = 1;
                break;
            case CS_CK_8:
                timer->divisor = 8;
                break;
            case CS_CK_64:
                timer->divisor = 64;
                break;
            case CS_CK_256:
                timer->divisor = 256;
                break;
            case CS_CK_1024:
                timer->divisor = 1024;
                break;
            default:
                avr_error ("The impossible happened!");
        }
        /* remove external incrementor if installed */
        if (timer->ext_cb)
            timer->ext_cb = NULL;

        /* install the clock incrementor callback (with flair!) */
        if (timer->clk_cb == NULL)
        {
            cb = callback_new (timer0_clk_incr_cb, (AvrClass *)timer);
            timer->clk_cb = cb;
            avr_core_clk_cb_add ((AvrCore *)vdev_get_core ((VDevice *)timer),
                                 cb);
        }
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
timer0_reset (VDevice *dev)
{
    Timer0_T *timer = (Timer0_T *)dev;

    timer->clk_cb = NULL;
    timer->ext_cb = NULL;

    timer->tccr = 0;
    timer->tcnt = 0;

    timer->divisor = 0;
}

static int
timer0_clk_incr_cb (uint64_t ck, AvrClass *data)
{
    Timer0_T *timer = (Timer0_T *)data;
    uint8_t last = timer->tcnt;
    TimerIntr_T *ti;

    ti = (TimerIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                   vdev_get_core ((VDevice *)
                                                                  timer),
                                                   timer->related_addr);

    if (timer->clk_cb == NULL)
        return CB_RET_REMOVE;

    if (timer->divisor <= 0)
        avr_error ("Bad divisor value: %d", timer->divisor);

    /* Increment clock if ck is a mutliple of divisor. Since divisor is always
       a power of 2, it's much faster to do the bitwise AND instead of using
       the integer modulus operator (%). */
    timer->tcnt += ((ck & (timer->divisor - 1)) == 0);

    /* Check if tcnt rolled over and if so, set the overflow flag.  If
       overflow interrupts are set? what if they aren't? This is set
       irregardless of whether SREG-I or TOIE0 are set (overflow interrupt
       enabled) and thus allows the interrupt to be pending until manually
       cleared (writing a one to the TOV0 flag) or interrupts are enabled. My
       interpretation of the datasheets. See datasheet discussion of TIFR.
       TRoth */
    if ((timer->tcnt == 0) && (timer->tcnt != last))
        ti->tifr |= mask_TOV0;

    return CB_RET_RETAIN;
}

/****************************************************************************\
*
* Timer/Counter 1/3 (16 bit)
*
\****************************************************************************/

/** \name 16 Bit Timer Functions */

/*@{*/

static void timer16_add_addr (VDevice *vdev, int addr, char *name,
                              int rel_addr, void *data);
static void timer16_destroy (void *timer);
static uint8_t timer16_read (VDevice *dev, int addr);
static void timer16_write (VDevice *dev, int addr, uint8_t val);
static void timer16_reset (VDevice *dev);
static int timer16_clk_incr_cb (uint64_t time, AvrClass *data);
static void timer16_handle_tccr_write (Timer16_T *timer);

/** \brief Allocate a new 16 bit timer/counter. */

VDevice *
timer16_create (int addr, char *name, int rel_addr, void *data)
{
    uint8_t *def_data = (uint8_t *) data;
    if (data)
        return (VDevice *)timer16_new (addr, name, rel_addr,
                                       global_timer16_defs[*def_data]);
    else
        avr_error ("Attempted timer 16 create with NULL data pointer");
    return 0;
}

Timer16_T *
timer16_new (int addr, char *name, int rel_addr, Timer16Def timerdef)
{
    Timer16_T *timer;

    timer = avr_new (Timer16_T, 1);
    timer16_construct (timer, addr, name, rel_addr, timerdef);
    class_overload_destroy ((AvrClass *)timer, timer16_destroy);

    return timer;
}

/** \brief Constructor for 16 bit timer/counter object. */

void
timer16_construct (Timer16_T *timer, int addr, char *name, int rel_addr,
                   Timer16Def timerdef)
{
    if (timer == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)timer, timer16_read, timer16_write,
                    timer16_reset, timer16_add_addr);

    timer->timerdef = timerdef;

    timer16_add_addr ((VDevice *)timer, addr, name, 0, NULL);
    if (rel_addr)
        timer->related_addr = rel_addr;
    timer16_reset ((VDevice *)timer);
}

static void
timer16_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                  void *data)
{
    Timer16_T *ti = (Timer16_T *)vdev;

    if (strncmp ("TCNTL", name, 5) == 0)
    {
        ti->tcntl_addr = addr;
    }

    else if (strncmp ("TCNTH", name, 5) == 0)
    {
        ti->tcnth_addr = addr;
    }

    else if (strncmp ("TCCRA", name, 5) == 0)
    {
        ti->tccra_addr = addr;
    }

    else if (strncmp ("TCCRB", name, 5) == 0)
    {
        ti->tccrb_addr = addr;
    }

    else if (strncmp ("TCCRC", name, 5) == 0)
    {
        ti->tccrc_addr = addr;
    }

    else
    {
        avr_error ("invalid Timer16 register name: '%s' @ 0x%04x", name,
                   addr);
    }
}

static void
timer16_destroy (void *timer)
{
    if (timer == NULL)
        return;

    vdev_destroy (timer);
}

static uint8_t
timer16_read (VDevice *dev, int addr)
{
    Timer16_T *timer = (Timer16_T *)dev;

    if (addr == timer->tcntl_addr)
    {
        timer->TEMP = (uint8_t) ((timer->tcnt) >> 8);
        return (timer->tcnt) & 0xFF;
    }

    else if (addr == timer->tcnth_addr)
    {
        return timer->TEMP;
    }

    else if (addr == timer->tccra_addr)
    {
        return timer->tccra;
    }

    else if (addr == timer->tccrb_addr)
    {
        return timer->tccrb;
    }

    else if (addr == timer->tccrc_addr)
    {
        return timer->tccrc;
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
timer16_write (VDevice *dev, int addr, uint8_t val)
{
    Timer16_T *timer = (Timer16_T *)dev;

    if (addr == timer->tcntl_addr)
    {
        timer->tcnt = (((timer->TEMP) << 8) & 0xFF00) | val;
    }

    else if (addr == timer->tcnth_addr)
    {
        timer->TEMP = val;
    }

    else if (addr == timer->tccra_addr)
    {
        timer->tccra = val;
        timer16_handle_tccr_write (timer);
    }

    else if (addr == timer->tccrb_addr)
    {
        timer->tccrb = val;
        timer16_handle_tccr_write (timer);
    }

    else if (addr == timer->tccrc_addr)
    {
        timer->tccrc = val;
        timer16_handle_tccr_write (timer);
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
timer16_reset (VDevice *dev)
{
    Timer16_T *timer = (Timer16_T *)dev;

    timer->clk_cb = NULL;
    timer->ext_cb = NULL;

    timer->tccra = 0;
    timer->tccrb = 0;
    timer->tccrc = 0;
    timer->tcnt = 0;

    timer->divisor = 0;
}

static void
timer_intr_set_flag (TimerIntr_T *ti, uint8_t bitnr)
{
    ti->tifr |= bitnr;
}

static int
timer16_clk_incr_cb (uint64_t ck, AvrClass *data)
{
    Timer16_T *timer = (Timer16_T *)data;
    uint16_t last = timer->tcnt;

    if (!timer->ti)
        timer->ti =
            (TimerIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                      vdev_get_core ((VDevice
                                                                      *)
                                                                     timer),
                                                      timer->related_addr);

    if (timer->clk_cb == NULL)
        return CB_RET_REMOVE;

    /* Increment clock if ck is a mutliple of divisor. Since divisor is always
       a power of 2, it's much faster to do the bitwise AND instead of using
       the integer modulus operator (%). */
    timer->tcnt += ((ck & (timer->divisor - 1)) == 0);

    if (timer->divisor <= 0)
        avr_error ("Bad divisor value: %d", timer->divisor);

    /* The following things only have to be checked if the counter value has
       changed */
    if (timer->tcnt != last)
    {
        /* An overflow occurred */
        if (timer->tcnt == 0)
            timer_intr_set_flag (timer->ti, mask_TOV1);

        /* The counter value matches one of the ocr values */
        if (timer->ocra && (timer->tcnt == timer->ocra->ocr))
        {
            timer_intr_set_flag (timer->ti, mask_OCF1A);
        }

        if (timer->ocrb && (timer->tcnt == timer->ocrb->ocr))
        {
            timer_intr_set_flag (timer->ti, mask_OCF1B);
        }
    }
    return CB_RET_RETAIN;
}

#if 0
static void
timer_intr_clear_flag (TimerIntr_T *ti, uint8_t bitnr)
{
    ti->tifr &= ~(bitnr);
}
#endif

static void
timer16_handle_tccr_write (Timer16_T *timer)
{
    int cs;
    CallBack *cb;
    /*
     * When the user writes toe TCCR, a callback is installed for either
     * clock generated increments or externally generated increments. The
     * two incrememtor callback are mutally exclusive, only one or the 
     * other can be installed at any given instant.
     */

    cs = timer->tccrb & 0x07;

    switch (cs)
    {
        case CS_STOP:
            /* stop either of the installed callbacks */
            timer->clk_cb = timer->ext_cb = NULL;
            timer->divisor = 0;
            return;
        case CS_EXT_FALL:
        case CS_EXT_RISE:
            /* FIXME: not implemented yet */
            avr_error ("external timer/counter sources is not implemented"
                       "yet");
            return;
        case CS_CK:
            timer->divisor = 1;
            break;
        case CS_CK_8:
            timer->divisor = 8;
            break;
        case CS_CK_64:
            timer->divisor = 64;
            break;
        case CS_CK_256:
            timer->divisor = 256;
            break;
        case CS_CK_1024:
            timer->divisor = 1024;
            break;
        default:
            avr_error ("The impossible happened!");
    }
    /* remove external incrementor if installed */
    if (timer->ext_cb)
        timer->ext_cb = NULL;

    /* install the clock incrementor callback (with flair!) */
    if (timer->clk_cb == NULL)
    {
        cb = callback_new (timer16_clk_incr_cb, (AvrClass *)timer);
        timer->clk_cb = cb;
        avr_core_clk_cb_add ((AvrCore *)vdev_get_core ((VDevice *)timer), cb);
    }
}

/*@}*/

/****************************************************************************\
 *
 * Timer16OCR(VDevice) : 16bit - Timer/Counter - Output Compare Register
 *
\****************************************************************************/

/** \name 16 Bit Output Compare Register Functions */

/*@{*/

static void ocr_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                          void *data);
static void ocreg16_destroy (void *ocr);
static uint8_t ocreg16_read (VDevice *dev, int addr);
static void ocreg16_write (VDevice *dev, int addr, uint8_t val);
static void ocreg16_reset (VDevice *dev);

/** \brief Allocate a new 16 bit Output Compare Register
  * \param ocrdef The definition struct for the \a OCR to be created
  */

VDevice *
ocreg16_create (int addr, char *name, int rel_addr, void *data)
{
    uint8_t *def_data = (uint8_t *) data;
    if (data)
        return (VDevice *)ocreg16_new (addr, name,
                                       global_ocreg16_defs[*def_data]);
    else
        avr_error ("Attempted OCReg create with NULL data pointer");
    return 0;
}

OCReg16_T *
ocreg16_new (int addr, char *name, OCReg16Def ocrdef)
{
    OCReg16_T *ocreg;

    ocreg = avr_new (OCReg16_T, 1);
    ocreg16_construct (ocreg, addr, name, ocrdef);
    class_overload_destroy ((AvrClass *)ocreg, ocreg16_destroy);

    return ocreg;
}

/** \brief Constructor for 16 bit Output Compare Register object. */

void
ocreg16_construct (OCReg16_T *ocreg, int addr, char *name, OCReg16Def ocrdef)
{
    if (ocreg == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)ocreg, ocreg16_read, ocreg16_write,
                    ocreg16_reset, ocr_add_addr);

    ocreg->ocrdef = ocrdef;

    ocr_add_addr ((VDevice *)ocreg, addr, name, 0, NULL);
    ocreg16_reset ((VDevice *)ocreg);
}

static void
ocr_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    OCReg16_T *ocreg = (OCReg16_T *)vdev;

    if ((strncmp ("OCRAL", name, 5) == 0) || (strncmp ("OCRBL", name, 5) == 0)
        || (strncmp ("OCRCL", name, 5) == 0))
    {
        ocreg->ocrl_addr = addr;
    }

    else if ((strncmp ("OCRAH", name, 5) == 0)
             || (strncmp ("OCRBH", name, 5) == 0)
             || (strncmp ("OCRCH", name, 5) == 0))

    {
        ocreg->ocrh_addr = addr;
    }

    else
    {
        avr_error ("invalid Timer16 register name: '%s' @ 0x%04x", name,
                   addr);
    }
}

static void
ocreg16_destroy (void *ocreg)
{
    if (ocreg == NULL)
        return;

    vdev_destroy (ocreg);
}

static uint8_t
ocreg16_read (VDevice *dev, int addr)
{
    OCReg16_T *ocreg = (OCReg16_T *)dev;

    if (addr == ocreg->ocrl_addr)
    {
        return (ocreg->ocr) & 0xFF;
    }

    else if (addr == ocreg->ocrh_addr)
    {
        return (ocreg->ocr) >> 8;
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;
}

static void
ocreg16_write (VDevice *dev, int addr, uint8_t val)
{
    OCReg16_T *ocreg = (OCReg16_T *)dev;

    if (addr == ocreg->ocrl_addr)
    {
        ocreg->ocr = (((ocreg->TEMP) << 8) & 0xFF00) | val;
    }

    else if (addr == ocreg->ocrh_addr)
    {
        ocreg->TEMP = val;
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
ocreg16_reset (VDevice *dev)
{
    OCReg16_T *ocreg = (OCReg16_T *)dev;

    ocreg->ocr = 0;
}

/*@}*/
