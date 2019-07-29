/*
 * $Id: register.h,v 1.7 2004/01/29 06:34:50 troth Exp $
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

#ifndef SIM_REGISTER_H
#define SIM_REGISTER_H

#include "display.h"

/****************************************************************************\
 *
 * SREG(VDevice) : Status Register Definition.
 *
\****************************************************************************/

typedef union _uSREG uSREG;

union _uSREG
{
    uint8_t reg;
    struct
    {
        uint8_t C:1;
        uint8_t Z:1;
        uint8_t N:1;
        uint8_t V:1;
        uint8_t S:1;
        uint8_t H:1;
        uint8_t T:1;
        uint8_t I:1;
    } bit;
};

enum _sreg_bits
{
    SREG_C = 0,
    SREG_Z = 1,
    SREG_N = 2,
    SREG_V = 3,
    SREG_S = 4,
    SREG_H = 5,
    SREG_T = 6,
    SREG_I = 7
};

enum _sreg_addr_info
{
    SREG_BASE = 0x5f,           /* base sreg mem addr */
    SREG_SIZE = 1,
    SREG_IO_REG = SREG_BASE - IO_REG_ADDR_BEGIN,
};

typedef struct _SREG SREG;

struct _SREG
{
    VDevice parent;
    uSREG sreg;
};

extern VDevice *sreg_create (int addr, char *name, int rel_addr, void *data);
extern SREG *sreg_new (void);
extern void sreg_construct (SREG *sreg);
extern void sreg_destroy (void *sreg);

extern inline uint8_t
sreg_get (SREG *sreg)
{
    return sreg->sreg.reg;
}

extern inline void
sreg_set (SREG *sreg, uint8_t val)
{
    sreg->sreg.reg = val;
    display_io_reg (SREG_IO_REG, sreg->sreg.reg);
}

extern inline uint8_t
sreg_get_bit (SREG *sreg, int bit)
{
    return !!(sreg->sreg.reg & (1 << bit));
}

extern inline void
sreg_set_bit (SREG *sreg, int bit, int val)
{
    sreg->sreg.reg = set_bit_in_byte (sreg->sreg.reg, bit, val);
    display_io_reg (SREG_IO_REG, sreg->sreg.reg);
}

/****************************************************************************\
 *
 * GPWR(VDevice) : General Purpose Working Registers Definition
 *
\****************************************************************************/

enum _gpwr_addr_info
{
    GPWR_BASE = 0x00,           /* base gpwr mem addr */
    GPWR_SIZE = 0x20,
};

typedef struct _GPWR GPWR;

struct _GPWR
{
    VDevice parent;
    uint8_t reg[32];
};

extern GPWR *gpwr_new (void);
extern void gpwr_construct (GPWR *gpwr);
extern void gpwr_destroy (void *gpwr);

extern inline uint8_t
gpwr_get (GPWR *gpwr, int reg)
{
#if defined(CHECK_REGISTER_BOUNDS)
    if ((reg < 0) || (reg >= 0x20))
        avr_error ("Invalid register: %d", reg);
#endif

    return gpwr->reg[reg];
}

extern inline void
gpwr_set (GPWR *gpwr, int reg, uint8_t val)
{
#if defined(CHECK_REGISTER_BOUNDS)
    if ((reg < 0) || (reg >= 0x20))
        avr_error ("Invalid register: %d", reg);
#endif

    gpwr->reg[reg] = val;

    display_reg (reg, val);
}

/****************************************************************************\
 *
 * AnaComp(VDevice) : Analog Comparator Definition
 *
\****************************************************************************/

typedef enum
{
    bit_ACIS0 = 0,              /* interrupt mode select bit 0 */
    bit_ACIS1 = 1,              /* interrupt mode select bit 1 */
    bit_ACIC = 2,               /* input capture enable        */
    bit_ACIE = 3,               /* interrupt enable            */
    bit_ACI = 4,                /* interrupt flag              */
    bit_ACO = 5,                /* output                      */
    bit_ACD = 7,                /* disable                     */
} ACSR_BITS;

typedef enum
{
    mask_ACIS0 = 1 << bit_ACIS0,
    mask_ACIS1 = 1 << bit_ACIS1,
    mask_ACIC = 1 << bit_ACIC,
    mask_ACIE = 1 << bit_ACIE,
    mask_ACI = 1 << bit_ACI,
    mask_ACO = 1 << bit_ACO,
    mask_ACD = 1 << bit_ACD,
} ACSR_MASKS;

enum _acsr_addr_info
{
    ACSR_BASE = 0x28,           /* acsr base mem addr */
    ACSR_SIZE = 1,
};

typedef struct _ACSR ACSR;

struct _ACSR
{
    VDevice parent;
    uint8_t acsr;               /* analog comparator control and status
                                   register */
    uint8_t func_mask;          /* mask of which bits in register are active
                                   for device */
};

extern ACSR *acsr_new (uint8_t func_mask);
extern void acsr_construct (ACSR *acsr, uint8_t func_mask);
extern void acsr_destroy (void *acsr);

extern int acsr_get_bit (ACSR *acsr, int bit);
extern void acsr_set_bit (ACSR *acsr, int bit, int val);

/****************************************************************************\
 *
 * MCUCR(VDevice) : MCU general control register
 *
\****************************************************************************/

typedef enum
{
    bit_ISC00 = 0,              /* interrupt sense control 0 bit 0 */
    bit_ISC01 = 1,              /* interrupt sense control 0 bit 1 */
    bit_ISC10 = 2,              /* interrupt sense control 1 bit 0 */
    bit_ISC11 = 3,              /* interrupt sense control 1 bit 1 */
    bit_SM = 4,                 /* sleep mode                      */
    bit_SE = 5,                 /* sleep enable                    */
    bit_SRW = 6,                /* external sram wait state        */
    bit_SRE = 7,                /* external sram enable            */
} MCUCR_BITS;

typedef enum
{
    mask_ISC00 = 1 << bit_ISC00,
    mask_ISC01 = 1 << bit_ISC01,
    mask_ISC10 = 1 << bit_ISC10,
    mask_ISC11 = 1 << bit_ISC11,
    mask_SM = 1 << bit_SM,
    mask_SE = 1 << bit_SE,
    mask_SRW = 1 << bit_SRW,
    mask_SRE = 1 << bit_SRE,
} MCUCR_MASKS;

enum _mcucr_addr_info
{
    MCUCR_BASE = 0x55,          /* mcucr base mem addr */
    MCUCR_SIZE = 1,
};

typedef struct _MCUCR MCUCR;

struct _MCUCR
{
    VDevice parent;
    uint8_t mcucr;              /* MCU control register */
    uint8_t func_mask;          /* mask of which bits in register are active
                                   for device */
};

extern MCUCR *mcucr_new (uint8_t func_mask);
extern void mcucr_construct (MCUCR *mcucr, uint8_t func_mask);
extern void mcucr_destroy (void *mcucr);

extern int mcucr_get_bit (MCUCR *mcucr, int bit);
extern void mcucr_set_bit (MCUCR *mcucr, int bit, int val);

/****************************************************************************\
 *
 * WDTCR(VDevice) : watch dog timer control register
 *
\****************************************************************************/

typedef enum
{
    bit_WDP0 = 0,               /* prescaler bit 0 */
    bit_WDP1 = 1,               /* prescaler bit 1 */
    bit_WDP2 = 2,               /* prescaler bit 2 */
    bit_WDE = 3,                /* watchdog enable */
    bit_WDTOE = 4,              /* turn-off enable */
} WDTCR_BITS;

typedef enum
{
    mask_WDP0 = 1 << bit_WDP0,
    mask_WDP1 = 1 << bit_WDP1,
    mask_WDP2 = 1 << bit_WDP2,
    mask_WDE = 1 << bit_WDE,
    mask_WDTOE = 1 << bit_WDTOE,

    mask_WDP = (mask_WDP2 | mask_WDP1 | mask_WDP0),
} WDTCR_MASKS;

enum _wdtcr_addr_info
{
    WDTCR_BASE = 0x41,          /* wdtcr base mem addr */
    WDTCR_SIZE = 1,
};

enum _wdtcr_constants
{
    TIMEOUT_BASE = 30,          /* about 30 ms for the base timeout before
                                   prescaling */
    TOE_CLKS = 4,               /* WDTOE is cleared by HW 4 cycles after be
                                   set */
};

typedef struct _WDTCR WDTCR;

struct _WDTCR
{
    VDevice parent;
    uint8_t wdtcr;
    uint8_t func_mask;
    uint64_t last_WDR;
    int toe_clk;
    CallBack *timer_cb;
    CallBack *toe_cb;
};

extern WDTCR *wdtcr_new (uint8_t func_mask);
extern void wdtcr_construct (WDTCR *wdtcr, uint8_t func_mask);
extern void wdtcr_destroy (void *wdtcr);

extern void wdtcr_update (WDTCR *wdtcr);

/****************************************************************************\
 *
 * RAMPZ(VDevice) : The RAMPZ register used by ELPM and ESPM instructions.
 *
\****************************************************************************/

enum _rampz_addr_info
{
    RAMPZ_BASE = 0x5b,          /* base rampz mem addr */
    RAMPZ_SIZE = 0x01,
    RAMPZ_IO_REG = RAMPZ_BASE - IO_REG_ADDR_BEGIN,
};

typedef struct _RAMPZ RAMPZ;

struct _RAMPZ
{
    VDevice parent;
    uint8_t reg;
};

extern VDevice *rampz_create (int addr, char *name, int rel_addr, void *data);
extern RAMPZ *rampz_new (void);
extern void rampz_construct (RAMPZ *rampz);
extern void rampz_destroy (void *rampz);

extern uint8_t rampz_get (RAMPZ *rampz);
extern void rampz_set (RAMPZ *rampz, uint8_t val);

#endif /* SIM_REGISTER_H */
