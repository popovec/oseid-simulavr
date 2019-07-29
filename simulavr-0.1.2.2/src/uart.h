/*
 * $Id: uart.h,v 1.3 2004/03/13 19:55:34 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2003  Keith Gudger
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

#ifndef SIM_UART_H
#define SIM_UART_H

enum _uart_definitions
{
    UART = 0x01,                /* only one uart */
    USART0 = 0x02,              /* two uarts, 2nd one low */
    USART1 = 0x06,              /* two uarts, 2nd one hi  */
};

/****************************************************************************\
 *
 * uartInter_T(VDevice) : uart Interrupt and Control Register
 *
\****************************************************************************/

enum _uart_intr_constants
{
    UART_INTR_BASE_1 = 0x20,    /* base address for vdev UART1 */
    UART_INTR_BASE_0 = 0x29,    /* base address for vdev UART0 */
    UART_INTR_BASE_128 = 0x99,  /* base address for vdev UART1 mega 128 */
    UART_INTR_SIZE = 3,         /* UBRR, UCR, USR */

    UART_INTR_UBRR_ADDR = 0,
    UART_INTR_UCR_ADDR = 1,
    UART_INTR_USR_ADDR = 2,
};

enum _uart_int_table_constants
{
    URX,                        /* uart Rx complete */
    UUDRE,                      /* uart data register empty */
    UTX                         /* uart Tx complete */
};

typedef enum
{
    bit_MPCM = 0,               /* Multi-processor Communication Mode  */
    bit_U2X = 1,                /* Double X-mission Speed   */
    bit_OR = 3,                 /* OverRun               */
    bit_FE = 4,                 /* Framing Error         */
    bit_UDRE = 5,               /* Data Register Empty   */
    bit_TXC = 6,                /* Transmit Complete     */
    bit_RXC = 7,                /* Receive Complete      */
} UCR_BITS;

typedef enum
{
    mask_MPCM = 1 << bit_MPCM,
    mask_U2X = 1 << bit_U2X,
    mask_OR = 1 << bit_OR,
    mask_FE = 1 << bit_FE,
    mask_UDRE = 1 << bit_UDRE,
    mask_TXC = 1 << bit_TXC,
    mask_RXC = 1 << bit_RXC,
} UCR_MASKS;

typedef enum
{
    bit_TXB8 = 0,               /* Transmit Data Bit 8  */
    bit_RXB8 = 1,               /* Receive  Data Bit 8  */
    bit_CHR9 = 2,               /* 9 bit characters     */
    bit_TXEN = 3,               /* Transmitter Enable   */
    bit_RXEN = 4,               /* Receiver    Enable   */
    bit_UDRIE = 5,              /* Data Register Interrupt Enable */
    bit_TXCIE = 6,              /* TX Complete   Interrupt Enable */
    bit_RXCIE = 7,              /* RX Complete   Interrupt Enable */
} USR_BITS;

typedef enum
{
    mask_TXB8 = 1 << bit_TXB8,
    mask_RXB8 = 1 << bit_RXB8,
    mask_CHR9 = 1 << bit_CHR9,
    mask_TXEN = 1 << bit_TXEN,
    mask_RXEN = 1 << bit_RXEN,
    mask_UDRIE = 1 << bit_UDRIE,
    mask_TXCIE = 1 << bit_TXCIE,
    mask_RXCIE = 1 << bit_RXCIE,
} USR_MASKS;

typedef struct _UARTIntr_T UARTIntr_T;

struct _UARTIntr_T
{
    VDevice parent;

    uint16_t ubrr;              /* uart BAUD Rate register */
    uint8_t ubrr_temp;
    uint16_t ubrrl_addr;
    uint16_t ubrrh_addr;

    uint8_t usr;                /* uart status register    */
    uint16_t usr_addr;

    uint8_t ucr;                /* uart control register   */
    uint16_t ucr_addr;

    uint8_t usr_shadow;         /* shadow uart status register   */
    CallBack *intr_cb;          /* callback - check and raise interrupts */
    int *Int_Table;             /* pointer to int index for this interrupt */
};

extern VDevice *uart_int_create (int addr, char *name, int rel_addr,
                                 void *data);
extern UARTIntr_T *uart_intr_new (int addr, char *name, void *data);
extern void uart_intr_construct (UARTIntr_T *uart, int addr, char *name);
extern void uart_intr_destroy (void *ti);

/****************************************************************************\
 *
 * uart(VDevice) : uart
 *
\****************************************************************************/

enum _uart_constants
{
    UART_BASE_1 = 0x23,         /* base memory address UART1 */
    UART_BASE_0 = 0x2c,         /* base memory address UART0 */
    UART_BASE_128 = 0x9c,       /* base memory address UART1, mega128 */
    UART_SIZE = 1,              /* SPDR */

    UART_UDR_ADDR = 0,          /* offset from base to SPDR Register */
};

typedef struct _UART UART_T;

struct _UART
{
    VDevice parent;

    uint8_t udr_rx;             /* receive  data  register */
    uint8_t udr_tx;             /* transmit data  register */
    uint16_t udr_addr;
    uint16_t related_addr;      /* interrupt address register */

    uint16_t tcnt;              /* uart timer up-counter register */
    uint16_t divisor;           /* clock divisor */
    CallBack *clk_cb;           /* incr timer tied to clock */
};

extern VDevice *uart_create (int addr, char *name, int rel_addr, void *data);
extern UART_T *uart_new (int addr, char *name, int rel_addr);
extern void uart_construct (UART_T *uart, int addr, char *name, int rel_addr);
extern void uart_destroy (void *uart);
extern void uart_intr_set_flag (UARTIntr_T *ti);
extern void uart_intr_clear_flag (UARTIntr_T *ti);
extern uint16_t uart_port_rd (int addr);
extern void uart_port_wr (uint8_t val);

#endif /* SIM_UART_H */
