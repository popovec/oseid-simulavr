/*
 * $Id: spi.h,v 1.4 2004/03/13 19:55:34 troth Exp $
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

#ifndef SPI_H
#define SPI_H

/****************************************************************************\
 *
 * SPIInter_T(VDevice) : SPI Interrupt and Control Register
 *
\****************************************************************************/

enum _spi_intr_constants
{
    SPI_INTR_BASE = 0x2d,       /* base address for vdev */
    SPI_INTR_SIZE = 2,          /* SPCR, SPSR */

    SPI_INTR_SPCR_ADDR = 0,
    SPI_INTR_SPSR_ADDR = 1,
};

typedef enum
{
    bit_SPR0 = 0,               /* Clock Rate Select 0   */
    bit_SPR1 = 1,               /* Clock Rate Select 1   */
    bit_CPHA = 2,               /* Clock Phase           */
    bit_CPOL = 3,               /* Clock Polarity        */
    bit_MSTR = 4,               /* Master / Slave Select */
    bit_DORD = 5,               /* Data Order            */
    bit_SPE = 6,                /* SPI Enable            */
    bit_SPIE = 7,               /* SPI Interrupt Enable  */
} SPCR_BITS;

typedef enum
{
    mask_SPR0 = 1 << bit_SPR0,
    mask_SPR1 = 1 << bit_SPR1,
    mask_SPE = 1 << bit_SPE,
    mask_SPIE = 1 << bit_SPIE,
} SPCR_MASKS;

typedef enum
{
    bit_WCOL = 6,               /* Write Collision flag */
    bit_SPIF = 7,               /* SPI Interrupt flag   */
} SPSR_BITS;

typedef enum
{
    mask_WCOL = 1 << bit_WCOL,
    mask_SPIF = 1 << bit_SPIF,
} SPSR_MASKS;

/* FIXME: Combine this with SPI_T. */

typedef struct _SPIIntr_T SPIIntr_T;

struct _SPIIntr_T
{
    VDevice parent;

    uint16_t spcr_addr;
    uint8_t spcr;               /* SPI Interrupt and control mask register */

    uint16_t spsr_addr;
    uint8_t spsr;               /* SPI Interrupt and Write Coll flag
                                   register */
    uint8_t spsr_read;          /* SPCR read occured with SPIF set */
    CallBack *intr_cb;          /* callback for checking and raising
                                   interrupts */
};

extern VDevice *spii_create (int addr, char *name, int rel_addr, void *data);
extern SPIIntr_T *spi_intr_new (int addr, char *name);
extern void spi_intr_construct (SPIIntr_T *ti, int addr, char *name);
extern void spi_intr_destroy (void *ti);

/****************************************************************************\
 *
 * General SPI bits and masks.
 *
\****************************************************************************/

enum _spi_cs_constants
{
    SPI_CK_4 = 0x00,            /* CK/4 */
    SPI_CK_16 = 0x01,           /* CK/16 */
    SPI_CK_64 = 0x02,           /* CK/64 */
    SPI_CK_128 = 0x03,          /* CK/128 */
};

/****************************************************************************\
 *
 * SPI(VDevice) : SPI
 *
\****************************************************************************/

enum _spi_constants
{
    SPI_BASE = 0x2f,            /* base memory address */
    SPI_SIZE = 1,               /* SPDR */

    SPI_SPDR_ADDR = 0,          /* offset from base to SPDR Register */
};

typedef struct _SPI SPI_T;

struct _SPI
{
    VDevice parent;

    uint16_t spdr_addr;
    uint16_t rel_addr;          /* interrupt address */
    uint8_t spdr;               /* data  register */
    uint8_t spdr_in;            /* new data  register */
    uint8_t tcnt;               /* SPI timer up-counter register */
    uint8_t divisor;            /* clock divisor */
    CallBack *clk_cb;           /* incr timer tied to clock */
};

extern VDevice *spi_create (int addr, char *name, int rel_addr, void *data);
extern SPI_T *spi_new (int addr, char *name, int rel_addr);
extern void spi_construct (SPI_T *spi, int addr, char *name, int rel_addr);
extern void spi_destroy (void *spi);
extern void spi_intr_set_flag (SPIIntr_T *ti);
extern void spi_intr_clear_flag (SPIIntr_T *ti);
extern uint8_t spi_port_rd (int addr);
extern void spi_port_wr (uint8_t val);

#endif
