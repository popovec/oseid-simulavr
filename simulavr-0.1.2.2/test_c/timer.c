/*
 * $Id: timer.c,v 1.2 2003/11/12 07:33:17 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2003,  Hermann Kraus
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

/* This test program has several functions:
 * - Testing interrupts in general.
 * - Testing the timer overflow and output compare match irqs.
 * - Showing how the timers work.
 * - Allowing the user to examine the "struct" and "array" debugging with
 *   Simulavr
 *
 * But be careful: This is an example for testing purposes only. For real
 * programs it wastes too much flash.
 */

#include <avr/signal.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include "common.h"

typedef struct _TimerInfo TimerInfo;

struct _TimerInfo
{
    uint16_t overflow_count;
    uint16_t oc_match_count[3];
};

static TimerInfo timer[4];
static uint32_t cycle;

SIGNAL(SIG_OVERFLOW0)
{
    timer[0].overflow_count++;
}

SIGNAL(SIG_OVERFLOW1)
{
    timer[1].overflow_count++;
}

SIGNAL(SIG_OVERFLOW2)
{
    timer[2].overflow_count++;
}

SIGNAL(SIG_OVERFLOW3)
{
    timer[4].overflow_count++;
}


SIGNAL(SIG_OUTPUT_COMPARE0)
{
    timer[0].oc_match_count[0]++;
}

SIGNAL(SIG_OUTPUT_COMPARE1A)
{
    timer[1].oc_match_count[0]++;
}

SIGNAL(SIG_OUTPUT_COMPARE1B)
{
    timer[1].oc_match_count[1]++;
}

SIGNAL(SIG_OUTPUT_COMPARE1C)
{
    timer[1].oc_match_count[2]++;
}

SIGNAL(SIG_OUTPUT_COMPARE2)
{
    timer[2].oc_match_count[0]++;
}

SIGNAL(SIG_OUTPUT_COMPARE3A)
{
    timer[3].oc_match_count[0]++;
}

SIGNAL(SIG_OUTPUT_COMPARE3B)
{
    timer[3].oc_match_count[1]++;
}

SIGNAL(SIG_OUTPUT_COMPARE3C)
{
    timer[3].oc_match_count[2]++;
}

void
clear_structs (void)
{
    uint8_t i, n;

    for (i=0; i<4; i++)
    {
        timer[i].overflow_count=0;
        for (n=0; n<3; n++)
        {
            timer[i].oc_match_count[n]=0;
        }
    }
}

int
main (void)
{
    /* Init the struct mem with zeros. */

    clear_structs();

    /* TODO: Add PWM test as PWM functionality is introduced in Simulavr.
       Start 8bit Timer/Counter 0 with prescaler 8. */

    TCCR0 = _BV(CS01);

    /* Start 16bit Timer/Counter 1 w/o prescaler. */

    TCCR1B = _BV(CS10);

    /* Start 8bit Timer/Counter 2 with prescaler 32. */

#if defined (TCCR2)
    TCCR2 = (_BV(CS20) | _BV(CS22));
#endif

    /* Start 16bit Timer/Counter 3 with prescaler 1024.
       This is not yet implemented in simulavr (as all extended registers). */

#if defined (TCCR3B)
    TCCR3B = (_BV(CS30) | _BV(CS31) | _BV(CS32));
#endif

    /* Now we set some OCR-Values. */

    /* 8 bit */

#if defined(OCR0)
    OCR0 = 12;
#endif
#if defined(OCR2)
    OCR2 = 34;
#endif

    /* 16 bit */

    OCR1A = 1234;
    OCR1B = 2345;
#if defined(OCR1C)
    OCR1C = 3456;
#endif
#if defined(OCR3A)
    OCR3A = 4567;
#endif
#if defined(OCR3B)
    OCR3B = 5678;
#endif
#if defined(OCR3C)
    OCR3C = 6789;
#endif

    /* Enable overflow irqs for timers 0-3 and output compare irq
       for timer 0, 1a, 1b, 3b. */

    TIMSK = (
             _BV(TOIE0)
             | _BV(TOIE1)
#if defined(TOIE2)
             | _BV(TOIE2)
#endif
#if defined(OCIE0)
             | _BV(OCIE0)
#endif
             | _BV(OCIE1A)
             | _BV(OCIE1B)
             );

#if defined(ETIMSK)
    ETIMSK = (
              _BV(TOIE3)
              | _BV(OCIE3B)
              );
#endif

    sei ();                     /* Enable irqs */

    for (;;)
    {
        cycle++;                /* Increase cycle counter */
    }

    return (0);
}
