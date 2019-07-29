/*
 * $Id: timers.h,v 1.7 2004/03/13 19:55:34 troth Exp $
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

#ifndef SIM_TIMERS_H
#define SIM_TIMERS_H

/****************************************************************************\
 *
 * TimerInter_T(VDevice) : Timer/Counter Interrupt Mask/Flag Registers
 *
\****************************************************************************/

enum _timer_intr_constants
{
    TIMER_INTR_BASE = 0x58,     /* base address for vdev */
    TIMER_INTR_SIZE = 2,        /* TIFR and TIMSK */

    TIMER_INTR_TIFR_ADDR = 0,
    TIMER_INTR_TIMSK_ADDR = 1,
};

typedef enum
{
    bit_TOIE0 = 1,              /* timer/counter0 overflow interrupt enable */
    bit_TICIE1 = 3,             /* timer/counter1 input capture interrupt
                                   enable */
    bit_OCIE1B = 5,             /* timer/counter1 output compare B match
                                   interrupt enable */
    bit_OCIE1A = 6,             /* timer/counter1 output compare A match
                                   interrupt enable */
    bit_TOIE1 = 7,              /* timer/counter1 overflow interrupt enable */
} TIMSK_BITS;

typedef enum
{
    mask_TOIE0 = 1 << bit_TOIE0,
    mask_TICIE1 = 1 << bit_TICIE1,
    mask_OCIE1B = 1 << bit_OCIE1B,
    mask_OCIE1A = 1 << bit_OCIE1A,
    mask_TOIE1 = 1 << bit_TOIE1,
} TIMSK_MASKS;

typedef enum
{
    bit_TOV0 = 1,               /* timer/counter0 overflow flag */
    bit_ICF1 = 3,               /* input capture flag 1 */
    bit_OCF1B = 5,              /* output compare flag 1B */
    bit_OCF1A = 6,              /* output compare flag 1A */
    bit_TOV1 = 7,               /* timer/counter1 overflow flag */
} TIFR_BITS;

typedef enum
{
    mask_TOV0 = 1 << bit_TOV0,
    mask_ICF1 = 1 << bit_ICF1,
    mask_OCF1B = 1 << bit_OCF1B,
    mask_OCF1A = 1 << bit_OCF1A,
    mask_TOV1 = 1 << bit_TOV1,
} TIFR_MASKS;

typedef struct _TimerIntr_T TimerIntr_T;

/* FIXME: will timsk and tifr always have the same func mask? */
struct _TimerIntr_T
{
    VDevice parent;

    uint16_t timsk_addr;
    uint8_t timsk;              /* Timer/counter interrupt mask register */
    uint16_t tifr_addr;
    uint8_t tifr;               /* Timer/counter interrupt flag register */

    uint8_t func_mask;          /* mask of available register functions */
    CallBack *intr_cb;          /* callback for checking and raising
                                   interrupts */
};

extern VDevice *timer_int_create (int addr, char *name, int rel_addr,
                                  void *data);
extern TimerIntr_T *timer_intr_new (int addr, char *name, uint8_t func_mask);
extern void timer_intr_construct (TimerIntr_T *ti, int addr, char *name,
                                  uint8_t func_mask);
extern void timer_intr_destroy (void *ti);

/****************************************************************************\
 *
 * General Timer/Counter bits and masks.
 *
\****************************************************************************/

typedef enum
{
    bit_CS0 = 0,                /* timer/counter clock select bit 0 */
    bit_CS1 = 1,
    bit_CS2 = 2,
    bit_CTC = 3,                /* clear timer/counter on compare match */
    bit_ICES = 6,               /* input capture edge select */
    bit_ICNC = 7,               /* input capture noise canceler (4 CKs) */
} TCCR_BITS;

typedef enum
{
    mask_CS0 = 1 << bit_CS0,
    mask_CS1 = 1 << bit_CS1,
    mask_CS2 = 1 << bit_CS2,
    mask_CTC = 1 << bit_CTC,
    mask_ICES = 1 << bit_ICES,
    mask_ICNC = 1 << bit_ICNC,

    mask_CS = (mask_CS0 | mask_CS1 | mask_CS2),
} TCCR_MASKS;

enum _cs_constants
{
    CS_STOP = 0x00,             /* Stop, the Timer/Counter is stopped */
    CS_CK = 0x01,               /* CK                                 */
    CS_CK_8 = 0x02,             /* CK/8                               */
    CS_CK_64 = 0x03,            /* CK/64                              */
    CS_CK_256 = 0x04,           /* CK/256                             */
    CS_CK_1024 = 0x05,          /* CK/1024                            */
    CS_EXT_FALL = 0x06,         /* External Pin Tn, falling edge      */
    CS_EXT_RISE = 0x07,         /* External Pin Tn, rising edge       */
};

/****************************************************************************\
 *
 * Timer0(VDevice) : Timer/Counter 0
 *
\****************************************************************************/

enum _timer0_constants
{
    TIMER0_BASE = 0x52,         /* base memory address */
    TIMER0_SIZE = 2,            /* TCCR0 and TCNT0 */

    TIMER0_TCNT_ADDR = 0,       /* offset from base to TCNT Register */
    TIMER0_TCCR_ADDR = 1,
};

typedef struct _Timer0 Timer0_T;

struct _Timer0
{
    VDevice parent;

    uint16_t tccr_addr;
    uint8_t tccr;               /* control register */
    uint16_t tcnt_addr;
    uint8_t tcnt;               /* Timer/Counter up-counter register */
    int divisor;                /* clock divisor */
    uint16_t related_addr;      /* interrupt address register */
    CallBack *clk_cb;           /* incr timer tied to clock */
    CallBack *ext_cb;           /* incr timer tied to external event */
};

extern VDevice *timer0_create (int addr, char *name, int rel_addr,
                               void *data);
extern Timer0_T *timer0_new (int addr, char *name, int rel_addr);
extern void timer0_construct (Timer0_T *timer, int addr, char *name,
                              int rel_addr);
extern void timer0_destroy (void *timer);
#if 0
extern void timer_intr_set_flag (TimerIntr_T *ti, uint8_t bitnr);
extern void timer_intr_clear_flag (TimerIntr_T *ti, uint8_t bitnr);
#endif

/****************************************************************************\
 *
 * OCR16(VDevice) : 16bit - Output Compare Register
 *
\****************************************************************************/

enum _ocreg16_constants
{
    OCREG16_SIZE = 2,
    OCRL_ADDR = 0,
    OCRH_ADDR = 1,
};

enum _ocreg16_defs
{
    OCR1A_DEF = 0,
    OCR1B_DEF,
    OCR1C_DEF,
    OCR3A_DEF,
    OCR3B_DEF,
    OCR3C_DEF,
};

typedef struct _OCReg16Def OCReg16Def;

struct _OCReg16Def
{
    char *ocrdev_name;
    char *ocrl_name;
    char *ocrh_name;
    int base;
};

extern OCReg16Def global_ocreg16_defs[];

typedef struct _OCReg16 OCReg16_T;

struct _OCReg16
{
    VDevice parent;

    uint16_t ocr;               /* output compare register */
    uint16_t ocrl_addr;
    uint16_t ocrh_addr;

    uint8_t TEMP;               /* TEMP register for read and write */
    OCReg16Def ocrdef;
};

extern VDevice *ocreg16_create (int addr, char *name, int rel_addr,
                                void *data);
extern OCReg16_T *ocreg16_new (int addr, char *name, OCReg16Def ocrdef);
extern void ocreg16_construct (OCReg16_T *ocreg, int addr, char *name,
                               OCReg16Def ocrdef);

/****************************************************************************\
 *
 * Timer16(VDevice) : Timer/Counter 1/3 (16bit)
 *
\****************************************************************************/

enum _timer16_definitions
{
    TD_TIMER1 = 0,
    TD_TIMER3,
};

typedef struct _Timer16Def Timer16Def;

struct _Timer16Def
{
    char *timer_name;
    char *tcnth_name;
    char *tcntl_name;
    char *tccra_name;
    char *tccrb_name;
    int base;
    int tof;
    int ocf_a;
    int ocf_b;
    int ocf_c;
};

extern Timer16Def global_timer16_defs[];

enum _timer16_constants
{
    TIMER16_SIZE = 4,           /* TCCRxA, TCCRxB, and TCNTxH, TCNTxL */
    TCNTL_ADDR = 0,
    TCNTH_ADDR = 1,
    TCCRB_ADDR = 2,
    TCCRA_ADDR = 3,
    TIMER16_BASE = 0x4c,        /* base memory address */
};

typedef struct _Timer16 Timer16_T;

struct _Timer16
{
    VDevice parent;

    uint8_t tccra;              /* control register A */
    uint16_t tccra_addr;
    uint8_t tccrb;              /* control register B */
    uint16_t tccrb_addr;
    uint8_t tccrc;              /* control register C */
    uint16_t tccrc_addr;

    uint16_t tcnt;              /* Timer/Counter up-counter register
                                   (2bytes) */
    uint16_t tcntl_addr;
    uint16_t tcnth_addr;

    int divisor;                /* clock divisor */
    uint8_t TEMP;               /* TEMP register for read and write */

    uint16_t related_addr;      /* interrupt address register */

    CallBack *clk_cb;           /* incr timer tied to clock */
    CallBack *ext_cb;           /* incr timer tied to external event */
    OCReg16_T *ocra;            /* Output compare registers */
    OCReg16_T *ocrb;            /* Output compare registers */
    OCReg16_T *ocrc;            /* Output compare registers */
    TimerIntr_T *ti;
    Timer16Def timerdef;        /* Timer definition structure */
};

extern VDevice *timer16_create (int addr, char *name, int rel_addr,
                                void *data);
extern Timer16_T *timer16_new (int addr, char *name, int rel_addr,
                               Timer16Def timerdef);
extern void timer16_construct (Timer16_T *timer, int addr, char *name,
                               int rel_addr, Timer16Def timerdef);

#endif /* SIM_TIMERS_H */
