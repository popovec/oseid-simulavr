/*
 * $Id: intvects.c,v 1.17 2004/02/26 07:22:15 troth Exp $
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

#include "intvects.h"

/****************************************************************************\
 *
 * Interrupt Vector Tables:
 *
 *   Since each device could have a different set of available interrupts, the
 *   following tables map all interrupts to the addr to jump to when the
 *   interrupt happens. If the device doesn't support an interrupt, the table
 *   will contain a NULL entry. Only one table will be installed into the core
 *   for a given device.
 *
 \****************************************************************************/

/* *INDENT-OFF* */

/*
 * Vector Table for devices:
 *   at90s1200
 */
static IntVectTable vtab_at90s1200 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x01, 0x00, { 0x5b, 1<<6 }, NO_BIT },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x02, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x03, 0x00, { 0x28, 1<<3 }, { 0x28, 1<<4 } }
};

/*
 * Vector Table for devices:
 *   at90s2313
 */
static IntVectTable vtab_at90s2313 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x01, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x02, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x03, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x04, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x05, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x06, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .UART_RX        = { "IRQ_UART_RX",
                        0x07, 0x00, { 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .UART_UDRE      = { "IRQ_UART_UDRE",
                        0x08, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .UART_TX        = { "IRQ_UART_TX",
                        0x09, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x0a, 0x00, { 0x28, 1<<3 }, { 0x28, 1<<4 } }
};

/*
 * Vector Table for devices:
 *   at90s4414, at90s8515
 */
static IntVectTable vtab_at90s4414 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x01, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x02, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x03, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x04, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x05, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x06, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x07, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x08, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .UART_RX        = { "IRQ_UART_RX",
                        0x09, 0x00, { 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .UART_UDRE      = { "IRQ_UART_UDRE",
                        0x0a, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .UART_TX        = { "IRQ_UART_TX",
                        0x0b, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x0c, 0x00, { 0x28, 1<<3 }, { 0x28, 1<<4 } }
};

/*
 * Vector Table for devices:
 *   atmega8
 */

static IntVectTable vtab_atmega8 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x01, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x02, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER2_COMP    = { "IRQ_TIMER2_COMP",
                        0x03, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER2_OVF     = { "IRQ_TIMER2_OVF",
                        0x04, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x05, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x06, 0x00, { 0x59, 1<<4 }, { 0x58, 1<<4 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x07, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x08, 0x00, { 0x59, 1<<2 }, { 0x58, 1<<2 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x09, 0x00,	{ 0x59, 1<<0 }, { 0x58, 1<<0 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x0a, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .UART_RX        = { "IRQ_UART_RX",
                        0x0b, 0x00,	{ 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .UART_UDRE      = { "IRQ_UART_UDRE",
                        0x0c, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .UART_TX        = { "IRQ_UART_TX",
                        0x0d, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .ADC            = { "IRQ_ADC",
                        0x0e, 0x00,	{ 0x26, 1<<3 }, { 0x26, 1<<4 } },
    .EE_READY       = { "IRQ_EE_READY",
                        0x0f, 0x00,	{ 0x3c, 1<<3 }, NO_BIT },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x10, 0x00,	{ 0x28, 1<<3 }, { 0x28, 1<<4 } },
    .TWI            = { "IRQ_TWI",
                        0x11, 0x00, { 0x56, 1<<0 }, { 0x56, 1<<7 } },
    .SPM_READY      = { "IRQ_SPM_READY",
                        0x12, 0x00, { 0x57, 1<<7 }, NO_BIT }
};



/*
 * Vector Table for devices:
 *   atmega16
 */

static IntVectTable vtab_atmega16 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x04, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER2_COMP    = { "IRQ_TIMER2_COMP",
                        0x06, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER2_OVF     = { "IRQ_TIMER2_OVF",
                        0x08, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x0a, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x0c, 0x00, { 0x59, 1<<4 }, { 0x58, 1<<4 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x0e, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x10, 0x00, { 0x59, 1<<2 }, { 0x58, 1<<2 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x12, 0x00, { 0x59, 1<<0 }, { 0x58, 1<<0 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x14, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .UART_RX        = { "IRQ_UART_RX",
                        0x16, 0x00, { 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .UART_UDRE      = { "IRQ_UART_UDRE",
                        0x18, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .UART_TX        = { "IRQ_UART_TX",
                        0x1a, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .ADC            = { "IRQ_ADC",
                        0x1c, 0x00, { 0x26, 1<<3 }, { 0x26, 1<<4 } },
    .EE_READY       = { "IRQ_EE_READY",
                        0x1e, 0x00, { 0x3c, 1<<3 }, NO_BIT },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x20, 0x00, { 0x28, 1<<3 }, { 0x28, 1<<4 } },
    .TWI            = { "IRQ_TWI",
                        0x22, 0x00, { 0x57, 1<<0 }, { 0x56, 1<<7 } },
    .INT2           = { "IRQ_INT2",
                        0x24, 0x00, { 0x5b, 1<<5 }, { 0x5a, 1<<5 } },
    .TIMER0_COMP    = { "IRQ_TIMER0_COMP",
                        0x26, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .SPM_READY      = { "IRQ_SPM_READY",
                        0x28, 0x00, { 0x57, 1<<7 }, NO_BIT }
};


/*
 * Vector Table for devices:
 *   atmega103
 */

static IntVectTable vtab_atmega103 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x59, 1<<0 }, NO_BIT },
    .INT1           = { "IRQ_INT1",
                        0x04, 0x00, { 0x59, 1<<1 }, NO_BIT },
    .INT2           = { "IRQ_INT2",
                        0x06, 0x00, { 0x59, 1<<2 }, NO_BIT },
    .INT3           = { "IRQ_INT3",
                        0x08, 0x00, { 0x59, 1<<3 }, NO_BIT },
    .INT4           = { "IRQ_INT4",
                        0x0a, 0x00, { 0x59, 1<<4 }, { 0x58, 1<<4 } },
    .INT5           = { "IRQ_INT5",
                        0x0c, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .INT6           = { "IRQ_INT6",
                        0x0e, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .INT7           = { "IRQ_INT7",
                        0x10, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER2_COMP    = { "IRQ_TIMER2_COMP",
                        0x12, 0x00, { 0x57, 1<<7 }, { 0x56, 1<<7 } },
    .TIMER2_OVF     = { "IRQ_TIMER2_OVF",
                        0x14, 0x00, { 0x57, 1<<6 }, { 0x56, 1<<6 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x16, 0x00, { 0x57, 1<<5 }, { 0x56, 1<<5 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x18, 0x00, { 0x57, 1<<4 }, { 0x56, 1<<4 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x1a, 0x00, { 0x57, 1<<3 }, { 0x56, 1<<3 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x1c, 0x00, { 0x57, 1<<2 }, { 0x56, 1<<2 } },
    .TIMER0_COMP    = { "IRQ_TIMER0_COMP",
                        0x1e, 0x00, { 0x57, 1<<1 }, { 0x56, 1<<1 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x20, 0x00, { 0x57, 1<<0 }, { 0x56, 1<<0 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x22, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .UART_RX        = { "IRQ_UART_RX",
                        0x24, 0x00, { 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .UART_UDRE      = { "IRQ_UART_UDRE",
                        0x26, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .UART_TX        = { "IRQ_UART_TX",
                        0x28, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .ADC            = { "IRQ_ADC",
                        0x2a, 0x00, { 0x26, 1<<3 }, { 0x26, 1<<4 } },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x2e, 0x00, { 0x28, 1<<3 }, { 0x28, 1<<4 } },
    .EE_READY       = { "IRQ_EE_READY",
                        0x2c, 0x00, { 0x3c, 1<<3 }, NO_BIT }
};

/*
 * Vector Table for devices:
 *   atmega128
 */

/* Note that the mega128 has BOOTRST and IVSEL fuses which can be used to
   change the interrupt vectors. If used, the new vectors are just the
   following plus some Boot Reset Address. This could be implemented just as
   we vector to handler. */

/* Note that the vectors address for mega128 are two insn's. This is needed
   since they can use jmp (32-bit) insn at the vector address. */

static IntVectTable vtab_atmega128 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .INT1           = { "IRQ_INT1",
                        0x04, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .INT2           = { "IRQ_INT2",
                        0x06, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .INT3           = { "IRQ_INT3",
                        0x08, 0x00, { 0x59, 1<<4 }, { 0x58, 1<<4 } },
    .INT4           = { "IRQ_INT4",
                        0x0a, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .INT5           = { "IRQ_INT5",
                        0x0c, 0x00, { 0x59, 1<<2 }, { 0x58, 1<<2 } },
    .INT6           = { "IRQ_INT6",
                        0x0e, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .INT7           = { "IRQ_INT7",
                        0x10, 0x00, { 0x59, 1<<0 }, { 0x58, 1<<0 } },
    .TIMER2_COMP    = { "IRQ_TIMER2_COMP",
                        0x12, 0x00, { 0x57, 1<<7 }, { 0x56, 1<<7 } },
    .TIMER2_OVF     = { "IRQ_TIMER2_OVF",
                        0x14, 0x00, { 0x57, 1<<6 }, { 0x56, 1<<6 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x16, 0x00, { 0x57, 1<<5 }, { 0x56, 1<<5 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x18, 0x00, { 0x57, 1<<4 }, { 0x56, 1<<4 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x1a, 0x00, { 0x57, 1<<3 }, { 0x56, 1<<3 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x1c, 0x00, { 0x57, 1<<2 }, { 0x56, 1<<2 } },
    .TIMER0_COMP    = { "IRQ_TIMER0_COMP",
                        0x1e, 0x00, { 0x57, 1<<1 }, { 0x56, 1<<1 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x20, 0x00, { 0x57, 1<<0 }, { 0x56, 1<<0 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x22, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .USART0_RX      = { "IRQ_USART0_RX",
                        0x24, 0x00, { 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .USART0_UDRE    = { "IRQ_USART0_UDRE",
                        0x26, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .USART0_TX      = { "IRQ_USART0_TX",
                        0x28, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .ADC            = { "IRQ_ADC",
                        0x2a, 0x00, { 0x26, 1<<3 }, { 0x26, 1<<4 } },
    .EE_READY       = { "IRQ_EE_READY",
                        0x2c, 0x00, { 0x3c, 1<<3 }, NO_BIT },
    .ANA_COMP       = { "IRQ_ANA_COMP",
                        0x2e, 0x00, { 0x28, 1<<3 }, { 0x28, 1<<4 } },
    .TIMER1_COMPC   = { "IRQ_TIMER1_COMPC",
                        0x30, 0x00, { 0x7d, 1<<0 }, { 0x7c, 1<<0 } },
    .TIMER3_CAPT    = { "IRQ_TIMER3_CAPT",
                        0x32, 0x00, { 0x7d, 1<<5 }, { 0x7c, 1<<5 } },
    .TIMER3_COMPA   = { "IRQ_TIMER3_COMPA",
                        0x34, 0x00, { 0x7d, 1<<4 }, { 0x7c, 1<<4 } },
    .TIMER3_COMPB   = { "IRQ_TIMER3_COMPB",
                        0x36, 0x00, { 0x7d, 1<<3 }, { 0x7c, 1<<3 } },
    .TIMER3_COMPC   = { "IRQ_TIMER3_COMPC",
                        0x38, 0x00, { 0x7d, 1<<1 }, { 0x7c, 1<<1 } },
    .TIMER3_OVF     = { "IRQ_TIMER3_OVF",
                        0x3a, 0x00, { 0x7d, 1<<2 }, { 0x7c, 1<<2 } },
    .USART1_RX      = { "IRQ_USART1_RX",
                        0x3c, 0x00, { 0x9a, 1<<7 }, { 0x9b, 1<<7 } },
    .USART1_UDRE    = { "IRQ_USART1_UDRE",
                        0x3e, 0x00, { 0x9a, 1<<5 }, { 0x9b, 1<<5 } },
    .USART1_TX      = { "IRQ_USART1_TX",
                        0x40, 0x00, { 0x9a, 1<<6 }, { 0x9b, 1<<6 } },
    .TWI            = { "IRQ_TWI",
                        0x42, 0x00, { 0x74, 1<<0 }, { 0x74, 1<<7 } },
    .SPM_READY      = { "IRQ_SPM_READY",
                        0x44, 0x00, { 0x68, 1<<7 }, NO_BIT }
};

/* supports 355, 353, 351
   NOTE: The vector addresses are not sequential. */
static IntVectTable vtab_at43usb355 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x04, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x06, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x08, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x0a, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x0c, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x0e, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x10, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .ADC            = { "IRQ_ADC",
                        0x16, 0x00, { 0x27, 1<<3 }, { 0x27, 1<<4 } },
    .USB_HW         = { "IRQ_USB_HW",
                        0x18, 0x00, NO_BIT, NO_BIT } /* TODO */
};

/* supports 320 */
static IntVectTable vtab_at43usb320 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x04, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x06, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x08, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x0a, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x0c, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x0e, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .SPI_STC        = { "IRQ_SPI_STC",
                        0x10, 0x00, { 0x2d, 1<<7 }, NO_BIT },
    .UART_RX        = { "IRQ_UART_RX",
                        0x12, 0x00, { 0x2a, 1<<7 }, { 0x2b, 1<<7 } },
    .UART_UDRE      = { "IRQ_UART_UDRE",
                        0x14, 0x00, { 0x2a, 1<<5 }, { 0x2b, 1<<5 } },
    .UART_TX        = { "IRQ_UART_TX",
                        0x16, 0x00, { 0x2a, 1<<6 }, { 0x2b, 1<<6 } },
    .USB_HW         = { "IRQ_USB",
                        0x18, 0x00, NO_BIT, NO_BIT } /* TODO */
};

/* supports 325
   NOTE: The vector addresses are not sequential. */
static IntVectTable vtab_at43usb325 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .INT1           = { "IRQ_INT1",
                        0x04, 0x00, { 0x5b, 1<<7 }, { 0x5a, 1<<7 } },
    .TIMER1_CAPT    = { "IRQ_TIMER1_CAPT",
                        0x06, 0x00, { 0x59, 1<<3 }, { 0x58, 1<<3 } },
    .TIMER1_COMPA   = { "IRQ_TIMER1_COMPA",
                        0x08, 0x00, { 0x59, 1<<6 }, { 0x58, 1<<6 } },
    .TIMER1_COMPB   = { "IRQ_TIMER1_COMPB",
                        0x0a, 0x00, { 0x59, 1<<5 }, { 0x58, 1<<5 } },
    .TIMER1_OVF     = { "IRQ_TIMER1_OVF",
                        0x0c, 0x00, { 0x59, 1<<7 }, { 0x58, 1<<7 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x0e, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .USB_HW         = { "IRQ_USB_HW",
                        0x18, 0x00, NO_BIT, NO_BIT } /* TODO */
};

/* supports 326
   NOTE: The vector addresses are not sequential. */
static IntVectTable vtab_at43usb326 = {
    .RESET          = { "IRQ_RESET",
                        0x00, 0x00, NO_BIT, NO_BIT },
    .INT0           = { "IRQ_INT0",
                        0x02, 0x00, { 0x5b, 1<<6 }, { 0x5a, 1<<6 } },
    .TIMER0_OVF     = { "IRQ_TIMER0_OVF",
                        0x0e, 0x00, { 0x59, 1<<1 }, { 0x58, 1<<1 } },
    .USB_HW         = { "IRQ_USB_HW",
                        0x18, 0x00, NO_BIT, NO_BIT } /* TODO */
};

/* *INDENT-ON* */

/*
 * Vector Table Lookup List.
 *
 * Maps a _vector_table_name to a device vector table.
 */
IntVectTable *global_vtable_list[] = {
    &vtab_at90s1200,
    &vtab_at90s2313,
    &vtab_at90s4414,
    &vtab_atmega8,
    &vtab_atmega16,
    &vtab_atmega103,
    &vtab_atmega128,
    &vtab_at43usb355,
    &vtab_at43usb320,
    &vtab_at43usb325,
    &vtab_at43usb326,
    NULL
};
