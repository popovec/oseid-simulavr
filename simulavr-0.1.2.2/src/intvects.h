/*
 * $Id: intvects.h,v 1.7 2003/12/01 07:35:53 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003  Theodore A. Roth
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

#ifndef SIM_INTVECTS_H
#define SIM_INTVECTS_H

enum _sleep_modes
{
    SLEEP_MODE_IDLE,
    SLEEP_MODE_ADC_REDUX,
    SLEEP_MODE_PWR_DOWN,
    SLEEP_MODE_PWR_SAVE,
    SLEEP_MODE_reserved1,
    SLEEP_MODE_reserved2,
    SLEEP_MODE_STANDBY,
    SLEEP_MODE_EXT_STANDBY
};

/* The reset address is always 0x00. */

#define IRQ_RESET_ADDR 0x00

/* NOTE: When an interrupt occurs, we just place a pointer to IntVect into the
   pending vector list (sorted by addr). */

typedef struct _IrqCtrlBit IrqCtrlBit;

struct _IrqCtrlBit
{
    uint16_t addr;              /* Address of the IO Register which controlls
                                   wether the interrupt is enabled or not. */
    uint8_t mask;               /* Mask denoting which bit (or bits) in the
                                   register to consider. */
};

#define NO_BIT { 0, 0 }

typedef struct _IntVect IntVect;

struct _IntVect
{
    char *name;                 /* The name of the interrupt. */
    int addr;                   /* Where to vector to when interrupt occurs. */
    uint32_t can_wake;          /* If the interrupt occurs while in sleep
                                   mode, can it wake the processor? Each bit
                                   represents a different sleep mode which the
                                   processor might be in. */

    /* For an interrupt to be serviced, both enable and flag bits must be
       set. The exception is if the interrupt flag doesn't exist, in which
       case, the flag mask is set to zero and having only the enable bit set
       is sufficient to have the interrupt be sreviced. */

    IrqCtrlBit enable;          /* This bit, if set, marks the interrupt as
                                   enabled. */
    IrqCtrlBit flag;            /* This bit, if set, marks the interrupt as
                                   flagged pending. */
};

/* Macro to calculate the field index into the structure instead of maintaining
   an index enumeration. */

#include <stddef.h>
#define irq_vect_table_index(vect) \
	(offsetof(IntVectTable, vect) / sizeof (IntVect))

/* No device will have all of these vectors, but must define a structure which
   has a slot for each interrupt. If the device doesn't support the
   interrrupt, a NULL vector will be held in the slot. */

typedef struct _IntVectTable IntVectTable;
struct _IntVectTable
{
    IntVect RESET;              /* external reset, power-on reset and watchdog
                                   reset */

    IntVect INT0;               /* external interrupt request 0 */
    IntVect INT1;               /* external interrupt request 1 */
    IntVect INT2;               /* external interrupt request 2 */
    IntVect INT3;               /* external interrupt request 3 */
    IntVect INT4;               /* external interrupt request 4 */
    IntVect INT5;               /* external interrupt request 5 */
    IntVect INT6;               /* external interrupt request 6 */
    IntVect INT7;               /* external interrupt request 7 */

    IntVect TIMER0_COMP;        /* timer/counter 0 compare match */
    IntVect TIMER0_OVF;         /* timer/counter 0 overflow */

    IntVect TIMER1_CAPT;        /* timer/counter 1 capture event */
    IntVect TIMER1_COMPA;       /* timer/counter 1 compare match A */
    IntVect TIMER1_COMPB;       /* timer/counter 1 compare match B */
    IntVect TIMER1_COMPC;       /* timer/counter 1 compare match C */
    IntVect TIMER1_OVF;         /* timer/counter 1 overflow */

    IntVect TIMER2_COMP;        /* timer/counter 2 compare match */
    IntVect TIMER2_OVF;         /* timer/counter 2 overflow */

    IntVect TIMER3_CAPT;        /* timer/counter 3 capture event */
    IntVect TIMER3_COMPA;       /* timer/counter 3 compare match A */
    IntVect TIMER3_COMPB;       /* timer/counter 3 compare match B */
    IntVect TIMER3_COMPC;       /* timer/counter 3 compare match C */
    IntVect TIMER3_OVF;         /* timer/counter 3 overflow */

    IntVect SPI_STC;            /* serial transfer complete */
    IntVect TWI;                /* Two-wire Serial Interface */

    IntVect UART_RX;            /* uart Rx complete */
    IntVect UART_UDRE;          /* uart data register empty */
    IntVect UART_TX;            /* uart Tx complete */

    IntVect USART0_RX;          /* usart0 Rx complete */
    IntVect USART0_UDRE;        /* usart0 data register empty */
    IntVect USART0_TX;          /* usart0 Tx complete */

    IntVect USART1_RX;          /* usart1 Rx complete */
    IntVect USART1_UDRE;        /* usart1 data register empty */
    IntVect USART1_TX;          /* usart1 Tx complete */

    IntVect ADC;                /* ADC conversion complete */
    IntVect ANA_COMP;           /* analog comparator */

    IntVect EE_READY;           /* EEPROM Ready */
    IntVect SPM_READY;          /* Store Program Memeory Ready */

    IntVect USB_HW;             /* USB Hardware  */
};

/* Global list of vector tables defined in intvects.c */
extern IntVectTable *global_vtable_list[];

#endif /* SIM_INTVECTS_H */
