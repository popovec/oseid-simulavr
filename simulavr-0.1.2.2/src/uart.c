/*
 * $Id: uart.c,v 1.3 2004/03/13 19:55:34 troth Exp $
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
 * \file uart.c
 * \brief Module to simulate the AVR's uart module.
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
#include "uart.h"

#include "avrcore.h"

#include "intvects.h"

/****************************************************************************\
 *
 * uart Interrupts 
 *
\****************************************************************************/

static void uart_iadd_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                            void *data);
static uint8_t uart_intr_read (VDevice *dev, int addr);
static void uart_intr_write (VDevice *dev, int addr, uint8_t val);
static void uart_intr_reset (VDevice *dev);
static int uart_intr_cb (uint64_t time, AvrClass *data);

int UART_Int_Table[] = {
    irq_vect_table_index (UART_RX), /* uart Rx complete */
    irq_vect_table_index (UART_UDRE), /* uart data register empty */
    irq_vect_table_index (UART_TX) /* uart Tx complete */
};

int UART0_Int_Table[] = {
    irq_vect_table_index (USART0_RX), /* uart Rx complete */
    irq_vect_table_index (USART0_UDRE), /* uart data register empty */
    irq_vect_table_index (USART0_TX) /* uart Tx complete */
};

int UART1_Int_Table[] = {
    irq_vect_table_index (USART1_RX), /* uart Rx complete */
    irq_vect_table_index (USART1_UDRE), /* uart data register empty */
    irq_vect_table_index (USART1_TX) /* uart Tx complete */
};

/** \brief Allocate a new uart interrupt */

VDevice *
uart_int_create (int addr, char *name, int rel_addr, void *data)
{
    if (data)
        return (VDevice *)uart_intr_new (addr, name, data);
    else
        avr_error ("Attempted UART create with NULL data pointer");
    return 0;
}

UARTIntr_T *
uart_intr_new (int addr, char *name, void *data)
{
    uint8_t *uart_num = (uint8_t *) data;
    UARTIntr_T *uart;

    uart = avr_new (UARTIntr_T, 1);
    uart_intr_construct (uart, addr, name);
    class_overload_destroy ((AvrClass *)uart, uart_intr_destroy);

    if (*uart_num == USART0)
        uart->Int_Table = &UART0_Int_Table[0];
    else if (*uart_num == USART1)
        uart->Int_Table = &UART1_Int_Table[0];
    else
        uart->Int_Table = &UART_Int_Table[0];

    uart_iadd_addr ((VDevice *)uart, addr, name, 0, NULL);
    return uart;
}

/** \brief Constructor for uart interrupt object. */

void
uart_intr_construct (UARTIntr_T *uart, int addr, char *name)
{
    if (uart == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)uart, uart_intr_read, uart_intr_write,
                    uart_intr_reset, uart_iadd_addr);

    uart_intr_reset ((VDevice *)uart);
}

static void
uart_iadd_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    UARTIntr_T *uart = (UARTIntr_T *)vdev;

    if (strncmp ("UBRRH", name, 5) == 0)
    {
        uart->ubrrh_addr = addr;
    }

    else if ((strncmp ("UBRR", name, 4) == 0)
             || (strncmp ("UBRR0", name, 5) == 0)
             || (strncmp ("UBRR1", name, 5) == 0))
    {
        uart->ubrrl_addr = addr;
    }

    else if ((strncmp ("USR", name, 3) == 0)
             || (strncmp ("UCSR0A", name, 6) == 0)
             || (strncmp ("UCSR1A", name, 6) == 0))
    {
        uart->usr_addr = addr;
    }

    else if ((strncmp ("UCR", name, 3) == 0)
             || (strncmp ("UCSR0B", name, 6) == 0)
             || (strncmp ("UCSR1B", name, 6) == 0))
    {
        uart->ucr_addr = addr;
    }

    else
    {
        avr_error ("invalid UART register name: '%s' @ 0x%04x", name, addr);
    }
}

/** \brief Destructor for uart interrupt object. */

void
uart_intr_destroy (void *uart)
{
    if (uart == NULL)
        return;

    vdev_destroy (uart);
}

static uint8_t
uart_intr_read (VDevice *dev, int addr)
{
    UARTIntr_T *uart = (UARTIntr_T *)dev;

    if (addr == uart->ubrrl_addr)
    {
        return (uart->ubrr & 0xff);
    }

    else if (addr == uart->ubrrh_addr)
    {
        return (uart->ubrr >> 8);
    }

    else if (addr == uart->ucr_addr)
    {
        return (uart->ucr);
    }

    else if (addr == uart->usr_addr)
    {
        return (uart->usr);
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
uart_intr_write (VDevice *dev, int addr, uint8_t val)
{
    UARTIntr_T *uart = (UARTIntr_T *)dev;
    CallBack *cb;

    if (addr == uart->ubrrl_addr)
    {
        uart->ubrr = val + (uart->ubrr_temp << 8);
    }

    else if (addr == uart->ubrrh_addr)
    {
        uart->ubrr_temp = val;
    }

    else if (addr == uart->usr)
    {
        if (val & mask_TXC)
            uart->usr &= ~mask_TXC;
    }

    else if (addr == uart->ucr_addr)
    {
        (uart->ucr = val);      /* look for interrupt enables */

        if (((uart->ucr & mask_TXEN) && (uart->ucr & mask_TXCIE))
            || ((uart->ucr & mask_RXEN) && (uart->ucr & mask_RXCIE))
            || (uart->ucr & mask_UDRIE))
        {
            if (uart->intr_cb == NULL)
            {
                /* we need to install the intr_cb function */
                cb = callback_new (uart_intr_cb, (AvrClass *)uart);
                uart->intr_cb = cb;
                avr_core_async_cb_add ((AvrCore *)vdev_get_core (dev), cb);
            }
        }
        else
        {
            uart->intr_cb = NULL;
            /* no interrupt are enabled, remove the callback */
        }
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
uart_intr_reset (VDevice *dev)
{
    UARTIntr_T *uart = (UARTIntr_T *)dev;

    uart->intr_cb = NULL;

    uart->ubrr = 0;
    uart->usr = 0;
    uart->ucr = 0;
    uart->usr_shadow = 0;
}

static int
uart_intr_cb (uint64_t time, AvrClass *data)
{
    UARTIntr_T *uart = (UARTIntr_T *)data;

    if (uart->intr_cb == NULL)
        return CB_RET_REMOVE;

    if ((uart->ucr & mask_RXCIE) && (uart->usr & mask_RXC))
        /* an enabled interrupt occured */
    {
        AvrCore *core = (AvrCore *)vdev_get_core ((VDevice *)uart);
        avr_core_irq_raise (core, (uart->Int_Table[URX]));
    }

    if ((uart->ucr & mask_TXCIE) && (uart->usr & mask_TXC))
        /* an enabled interrupt occured */
    {
        AvrCore *core = (AvrCore *)vdev_get_core ((VDevice *)uart);
        avr_core_irq_raise (core, (uart->Int_Table[UTX]));
        uart->usr &= ~mask_TXC;
    }

    if ((uart->ucr & mask_UDRIE) && (uart->usr & mask_UDRE)
        && (uart->usr_shadow & mask_UDRE))
        /* an enabled interrupt occured */
    {
        AvrCore *core = (AvrCore *)vdev_get_core ((VDevice *)uart);
        avr_core_irq_raise (core, (uart->Int_Table[UUDRE]));
        uart->usr_shadow &= ~mask_UDRE; /* only issue one interrupt / udre */
    }

    return CB_RET_RETAIN;
}

/****************************************************************************\
 *
 * uart  
 *
\****************************************************************************/

static void uart_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                           void *data);
static uint8_t uart_read (VDevice *dev, int addr);
static void uart_write (VDevice *dev, int addr, uint8_t val);
static void uart_reset (VDevice *dev);
static int uart_clk_incr_cb (uint64_t ck, AvrClass *data);

/** \brief Allocate a new uart structure. */

VDevice *
uart_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)uart_new (addr, name, rel_addr);
}

UART_T *
uart_new (int addr, char *name, int rel_addr)
{
    UART_T *uart;

    uart = avr_new (UART_T, 1);
    uart_construct (uart, addr, name, rel_addr);

    class_overload_destroy ((AvrClass *)uart, uart_destroy);

    return uart;
}

/** \brief Constructor for uart object. */

void
uart_construct (UART_T *uart, int addr, char *name, int rel_addr)
{
    if (uart == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)uart, uart_read, uart_write, uart_reset,
                    uart_add_addr);

    uart_add_addr ((VDevice *)uart, addr, name, 0, NULL);
    if (rel_addr)
        uart->related_addr = rel_addr;
    uart_reset ((VDevice *)uart);
}

static void
uart_add_addr (VDevice *vdev, int addr, char *name, int ref_addr, void *data)
{
    UART_T *uart = (UART_T *)vdev;

    if (strncmp ("UDR", name, 3) == 0)
    {
        uart->udr_addr = addr;
    }

    else
    {
        avr_error ("invalid SPI register name: '%s' @ 0x%04x", name, addr);
    }
}

/** \brief Destructor for uart object. */

void
uart_destroy (void *uart)
{
    if (uart == NULL)
        return;

    vdev_destroy (uart);
}

static uint8_t
uart_read (VDevice *dev, int addr)
{
    UART_T *uart = (UART_T *)dev;
    UARTIntr_T *uart_t;
    uint16_t udr_temp;

    uart_t =
        (UARTIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                 vdev_get_core ((VDevice *)
                                                                uart),
                                                 uart->related_addr);

    if (addr == uart->udr_addr)
    {
        uart_t->usr &= ~mask_RXC; /* clear RXC bit in USR */
        if (uart->clk_cb)       /* call back already installed */
        {
            udr_temp = uart_port_rd (addr);
            uart->udr_rx = (uint8_t) udr_temp; /* lower 8 bits */
            if ((uart_t->ucr & mask_CHR9) && /* 9 bits rec'd */
                (udr_temp & (1 << 8))) /* hi bit set */
                uart_t->ucr |= mask_RXB8;
            else
                uart_t->ucr &= ~mask_RXB8;
        }
        return uart->udr_rx;
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }

    return 0;                   /* will never get here */
}

static void
uart_write (VDevice *dev, int addr, uint8_t val)
{
    UART_T *uart = (UART_T *)dev;
    UARTIntr_T *uart_t;
    CallBack *cb;

    uart_t =
        (UARTIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                 vdev_get_core ((VDevice *)
                                                                uart),
                                                 uart->related_addr);

    if (addr == uart->udr_addr)
    {
        if (uart_t->usr & mask_UDRE)
        {
            uart_t->usr &= ~mask_UDRE;
            uart_t->usr_shadow &= ~mask_UDRE;
        }
        else
        {
            uart_t->usr |= mask_UDRE;
            uart_t->usr_shadow |= mask_UDRE;
        }
        uart->udr_tx = val;

        /*
         * When the user writes to UDR, a callback is installed for 
         * clock generated increments. 
         */

        uart->divisor = (uart_t->ubrr + 1) * 16;

        /* install the clock incrementor callback (with flair!) */

        if (uart->clk_cb == NULL)
        {
            cb = callback_new (uart_clk_incr_cb, (AvrClass *)uart);
            uart->clk_cb = cb;
            avr_core_clk_cb_add ((AvrCore *)vdev_get_core ((VDevice *)uart),
                                 cb);
        }

        /* set up timer for 8 or 9 clocks based on ucr 
           (includes start and stop bits) */

        uart->tcnt = (uart_t->ucr & mask_CHR9) ? 11 : 10;
    }

    else
    {
        avr_error ("Bad address: 0x%04x", addr);
    }
}

static void
uart_reset (VDevice *dev)
{
    UART_T *uart = (UART_T *)dev;

    uart->clk_cb = NULL;

    uart->udr_rx = 0;
    uart->udr_tx = 0;
    uart->tcnt = 0;
    uart->divisor = 0;
}

static int
uart_clk_incr_cb (uint64_t ck, AvrClass *data)
{
    UART_T *uart = (UART_T *)data;
    UARTIntr_T *uart_t;
    uint8_t last = uart->tcnt;

    uart_t =
        (UARTIntr_T *)avr_core_get_vdev_by_addr ((AvrCore *)
                                                 vdev_get_core ((VDevice *)
                                                                uart),
                                                 uart->related_addr);

    if (uart->clk_cb == NULL)
        return CB_RET_REMOVE;

    if (uart->divisor <= 0)
        avr_error ("Bad divisor value: %d", uart->divisor);

    /* decrement clock if ck is a mutliple of divisor */

    uart->tcnt -= ((ck % uart->divisor) == 0);

    if (uart->tcnt != last)     /* we've changed the counter */
    {
        if (uart->tcnt == 0)
        {
            if (uart_t->usr & mask_UDRE) /* data register empty */
            {
                uart_t->usr |= mask_TXC;
                uart->clk_cb = NULL;
                return CB_RET_REMOVE;
            }
            else                /* there's a byte waiting to go */
            {
                uart_t->usr |= mask_UDRE;
                uart_t->usr_shadow |= mask_UDRE; /* also write shadow */

                /* set up timer for 8 or 9 clocks based on ucr,
                   (includes start and stop bits) */

                uart->tcnt = (uart_t->ucr & mask_CHR9) ? 11 : 10;
            }
        }
    }

    return CB_RET_RETAIN;
}

uint16_t
uart_port_rd (int addr)
{
    int data;
    char line[80];

    while (1)
    {
        fprintf (stderr,
                 "\nEnter 9 bits of hex data to read into the uart at "
                 "address 0x%04x: ", addr);

        /* try to read in a line of input */
        if (fgets (line, sizeof (line), stdin) == NULL)
            continue;

        /* try to parse the line for a byte of data */
        if (sscanf (line, "%x\n", &data) != 1)
            continue;

        break;
    }

    return (uint16_t) (data & 0x1ff);
}

void
uart_port_wr (uint8_t val)
{
    fprintf (stderr, "wrote 0x%02x to uart\n", val);
}
