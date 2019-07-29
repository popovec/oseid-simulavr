/*
 * $Id: 90s1200.h,v 1.1 2004/02/02 01:49:25 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2004  Theodore A. Roth
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

#if defined (IN_DEVSUPP_C)

static DevSuppDefn defn_at90s1200 = {
    .name           = "at90s1200",
    .stack_type     = STACK_HARDWARE,
    .irq_vect_idx   = VTAB_AT90S1200,

    .size = {
        .pc         = 2,
        .stack      = 3,
        .flash      = 1024,
        .sram       = 0,
        .eeprom     = 64
    },

    .io_reg = {
        { .addr = 0x28, .name = "ACSR", },
        {
            .addr = 0x30,
            .name = "PIND",
            .vdev_create = port_create,
            .reset_value = 0x00,
            .rd_mask = 0x7f,
            .wr_mask = 0x7f,
        },
        {
            .addr = 0x31,
            .name = "DDRD",
            .ref_addr = 0x30,
            .reset_value = 0x00,
            .rd_mask = 0x7f,
            .wr_mask = 0x7f,
        },
        {
            .addr = 0x32,
            .name = "PORTD",
            .ref_addr = 0x30,
            .reset_value = 0x00,
            .rd_mask = 0x7f,
            .wr_mask = 0x7f,
        },
        {
            .addr = 0x36,
            .name = "PINB",
            .vdev_create = port_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x37,
            .name = "DDRB",
            .ref_addr = 0x36,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x38,
            .name = "PORTB",
            .ref_addr = 0x36,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { .addr = 0x3c, .name = "EECR", },
        { .addr = 0x3d, .name = "EEDR", },
        { .addr = 0x3e, .name = "EEAR", },
        { .addr = 0x41, .name = "WDTCR", },
        { .addr = 0x52, .name = "TCNT0", },
        { .addr = 0x53, .name = "TCCR0", },
        { .addr = 0x55, .name = "MCUCR", },
        { .addr = 0x58, .name = "TIFR", },
        { .addr = 0x59, .name = "TIMSK", },
        { .addr = 0x5b, .name = "GIMSK", },
        {
            .addr = 0x5f,
            .name = "SREG",
            .vdev_create = sreg_create,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        IO_REG_DEFN_TERMINATOR
    }
};

#endif /* IN_DEVSUPP_C */
