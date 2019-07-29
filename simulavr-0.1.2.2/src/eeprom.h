/*
 * $Id: eeprom.h,v 1.5 2003/12/01 07:35:52 troth Exp $
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

#ifndef SIM_EEPROM_H
#define SIM_EEPROM_H

/****************************************************************************\
 *
 * EEPRom(VDevice) Definition
 *
\****************************************************************************/

typedef enum
{
    bit_EERE = 0,               /* eeprom read enable strobe         */
    bit_EEWE = 1,               /* eeprom write enable strobe        */
    bit_EEMWE = 2,              /* eeprom master write enable strobe */
} EECR_BITS;

typedef enum
{
    mask_EERE = 1 << bit_EERE,
    mask_EEWE = 1 << bit_EEWE,
    mask_EEMWE = 1 << bit_EEMWE,
} EECR_MASKS;

enum _eeprom_constants
{
    EEPROM_BASE = 0x3c,         /* base eeprom mem addr */
    EEPROM_SIZE = 4,            /* EECR, EEDR, EEARL, EEARH */

    EECR_ADDR = 0x3c,
    EEDR_ADDR = 0x3d,
    EEARL_ADDR = 0x3e,
    EEARH_ADDR = 0x3f,

    EEPROM_WR_OP_CLKS = 2500,   /* clocks used to simulate write 2.5-4.0 ms
                                   write time */
    EEPROM_MWE_CLKS = 4,        /* hw clears EEMWE after 4 clocks */
};

typedef struct _EEProm EEProm;

struct _EEProm
{
    VDevice parent;
    Storage *stor;              /* storage for eeprom memory */
    uint8_t eecr;               /* eeprom control register */
    uint8_t eecr_mask;          /* mask of bits available for device */
    uint8_t eedr;               /* eeprom data register */
    uint8_t eearl;              /* eeprom address register low byte */
    uint8_t eearh;              /* eeprom address register high byte */

    CallBack *wr_op_cb;         /* clock callback for write to eeprom
                                   operation */
    int wr_op_clk;              /* clock counter for write to eeprom
                                   operation */

    CallBack *mwe_clr_cb;       /* clock callback for hw clearing of MWE
                                   bit */
    int mwe_clk;                /* clock counter for hw clearing of MWE bit */
};

extern EEProm *eeprom_new (int size, uint8_t eecr_mask);
extern void eeprom_construct (EEProm *eeprom, int size, uint8_t eecr_mask);
extern void eeprom_destroy (void *eeprom);

extern int eeprom_get_size (EEProm *eeprom);

extern int eeprom_load_from_file (EEProm *eeprom, char *file, int format);

extern void eeprom_dump_core (EEProm *eeprom, FILE * f_core);

#endif /* SIM_EEPROM_H */
