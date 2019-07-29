/*
 * $Id: adc.h,v 1.4 2004/03/13 19:55:34 troth Exp $
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

#ifndef ADC_H
#define ADC_H

/****************************************************************************\
 *
 * ADCInter_T(VDevice) : ADC Interrupt and Control Register
 *
\****************************************************************************/

enum _adc_intr_constants
{
    ADC_INTR_BASE_A = 0x26,     /* base address for vdev */
    ADC_INTR_BASE_U = 0x27,     /* base address for vdev */
    ADC_INTR_SIZE = 2,          /* SPCR, SPSR */

    ADC_INTR_ADCSR_ADDR = 0,
    ADC_INTR_ADMUX_ADDR = 1,
};

typedef enum
{
    bit_ADPS0 = 0,              /* Clock Rate Select 0   */
    bit_ADPS1 = 1,              /* Clock Rate Select 1   */
    bit_ADPS2 = 2,              /* Clock Rate Select 1   */
    bit_ADIE = 3,               /* ADC Interrupt Enable  */
    bit_ADIF = 4,               /* ADC Interrupt Flag    */
    bit_ADFR = 5,               /* Free Run Select       */
    bit_ADSC = 6,               /* ADC Start Conversion  */
    bit_ADEN = 7,               /* ADC Enable            */
} ADCSR_BITS;

typedef enum
{
    mask_ADPS0 = 1 << bit_ADPS0,
    mask_ADPS1 = 1 << bit_ADPS1,
    mask_ADPS2 = 1 << bit_ADPS2,
    mask_ADIE = 1 << bit_ADIE,
    mask_ADIF = 1 << bit_ADIF,
    mask_ADFR = 1 << bit_ADFR,
    mask_ADSC = 1 << bit_ADSC,
    mask_ADEN = 1 << bit_ADEN,
} ADCSR_MASKS;

typedef enum
{
    bit_MUX0 = 0,               /* MUX Channel Select Bit */
    bit_MUX1 = 1,               /* MUX Channel Select Bit */
    bit_MUX2 = 2,               /* MUX Channel Select Bit */
    bit_MUX3 = 3,               /* MUX Channel Select Bit */
} ADMUX_BITS;

typedef enum
{
    mask_MUX0 = 1 << bit_MUX0,
    mask_MUX1 = 1 << bit_MUX1,
    mask_MUX2 = 1 << bit_MUX2,
    mask_MUX3 = 1 << bit_MUX3,
} ADMUX_MASKS;

/* FIXME: This should be combined with the adc vdev (ADC_T). */

typedef struct _ADCIntr_T ADCIntr_T;

struct _ADCIntr_T
{
    VDevice parent;

    uint16_t adcsr_addr;
    uint16_t admux_addr;
    uint16_t rel_addr;          /* address of ADC data registers */

    uint8_t adcsr;              /* ADC Interrupt control and status register */
    uint8_t admux;              /* ADC Multiplexer Select register */

    CallBack *intr_cb;          /* callback for checking and raising
                                   interrupts */
};

extern VDevice *adc_int_create (int addr, char *name, int rel_addr,
                                void *data);
extern ADCIntr_T *adc_intr_new (int addr, char *name, int rel_addr);
extern void adc_intr_construct (ADCIntr_T *ti, int addr, char *name,
                                int rel_addr);
extern void adc_intr_destroy (void *ti);

/****************************************************************************\
 *
 * General ADC bits and masks.
 *
\****************************************************************************/

enum _adc_cs_constants
{
    ADC_CK_0 = 0x00,            /* CK/2 */
    ADC_CK_2 = 0x01,            /* CK/2 */
    ADC_CK_4 = 0x02,            /* CK/4 */
    ADC_CK_8 = 0x03,            /* CK/8 */
    ADC_CK_16 = 0x04,           /* CK/16 */
    ADC_CK_32 = 0x05,           /* CK/32 */
    ADC_CK_64 = 0x06,           /* CK/64 */
    ADC_CK_128 = 0x07,          /* CK/128 */
};

/****************************************************************************\
 *
 * ADC(VDevice) : ADC
 *
\****************************************************************************/

enum _adc_constants
{
    ADC_BASE_A = 0x24,          /* base memory address */
    ADC_BASE_U = 0x22,          /* base memory address */
    ADC_SIZE = 2,               /* ADC Data Register Size = 2 bytes */

    ADC_ADCL_ADDR = 0,          /* offset from base to ADCL Register */
    ADC_ADCH_ADDR = 1,          /* offset from base to ADCH Register */
};

typedef struct _ADC ADC_T;

struct _ADC
{
    VDevice parent;

    uint16_t adcl_addr;         /* data low register address */
    uint8_t adcl;               /* data low register */

    uint16_t adch_addr;         /* data high register address */
    uint8_t adch;               /* data high register */

    uint16_t rel_addr;          /* address of ADC int registers */

    uint16_t adc_count;         /* 16 bit count register */
    uint16_t adc_in;            /* new data register */
    uint16_t divisor;           /* clock divisor */
    uint8_t u_divisor;          /* extra usb clock divisor */
    CallBack *clk_cb;           /* incr timer tied to clock */
};

extern VDevice *adc_create (int addr, char *name, int rel_addr, void *data);
extern ADC_T *adc_new (int addr, char *name, uint8_t data, int rel_addr);
extern void adc_construct (ADC_T *adc, int addr, char *name, uint8_t data,
                           int rel_addr);
extern void adc_destroy (void *adc);
extern void adc_intr_set_flag (ADCIntr_T *ti);
extern void adc_intr_clear_flag (ADCIntr_T *ti);
extern uint16_t adc_port_rd (uint8_t mux);
extern void adc_port_wr (uint8_t val);

#endif
