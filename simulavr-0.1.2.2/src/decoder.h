/*
 * $Id: decoder.h,v 1.6 2003/12/02 08:25:00 troth Exp $
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

#ifndef SIM_DECODER_H
#define SIM_DECODER_H

/****************************************************************************\
 *
 * Avr Opcode Decoder and Opcode handler routines
 *
\****************************************************************************/

typedef int (*Opcode_FP) (AvrCore *core, uint16_t opcode, unsigned int arg1,
                          unsigned int arg2);

struct opcode_info
{
    Opcode_FP func;
    unsigned int arg1;
    unsigned int arg2;
};

extern struct opcode_info *global_opcode_lookup_table;

extern void decode_init_lookup_table (void);
extern int  avr_op_UNKNOWN (AvrCore *core, uint16_t opcode, unsigned int arg1,
                            unsigned int arg2);

extern inline struct opcode_info *
decode_opcode (uint16_t opcode)
{
    struct opcode_info *opi;

    opi = global_opcode_lookup_table + opcode;

    if (opi->func == avr_op_UNKNOWN)
        avr_warning ("Unknown opcode: 0x%04x\n", opcode);

    return opi;
}

#endif /* SIM_DECODER_H */
