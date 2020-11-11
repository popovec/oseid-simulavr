/*
 * $Id: decoder.c,v 1.35 2004/02/26 07:33:34 troth Exp $
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

/**
 * \file decoder.c
 * \brief Module for handling opcode decoding.
 *
 * The heart of the instruction decoder is the decode_opcode() function.
 *
 * The decode_opcode() function examines the given opcode to
 * determine which instruction applies and returns a pointer to a function to
 * handler performing the instruction's operation. If the given opcode does
 * not map to an instruction handler, NULL is returned.
 *
 * Nearly every instruction in Atmel's Instruction Set Data Sheet will have a
 * handler function defined. Each handler will perform all the operations
 * described in the data sheet for a given instruction. A few instructions
 * have synonyms. For example, CBR is a synonym for ANDI.
 *
 * This should all be fairly straight forward.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

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

#include "avrcore.h"

#include "decoder.h"

struct opcode_info *global_opcode_lookup_table;

/** \brief Masks to help extracting information from opcodes. */

enum decoder_operand_masks
{
    /** 2 bit register id  ( R24, R26, R28, R30 ) */
    mask_Rd_2 = 0x0030,
    /** 3 bit register id  ( R16 - R23 ) */
    mask_Rd_3 = 0x0070,
    /** 4 bit register id  ( R16 - R31 ) */
    mask_Rd_4 = 0x00f0,
    /** 5 bit register id  ( R00 - R31 ) */
    mask_Rd_5 = 0x01f0,

    /** 3 bit register id  ( R16 - R23 ) */
    mask_Rr_3 = 0x0007,
    /** 4 bit register id  ( R16 - R31 ) */
    mask_Rr_4 = 0x000f,
    /** 5 bit register id  ( R00 - R31 ) */
    mask_Rr_5 = 0x020f,

    /** for 8 bit constant */
    mask_K_8 = 0x0F0F,
    /** for 6 bit constant */
    mask_K_6 = 0x00CF,

    /** for 7 bit relative address */
    mask_k_7 = 0x03F8,
    /** for 12 bit relative address */
    mask_k_12 = 0x0FFF,
    /** for 22 bit absolute address */
    mask_k_22 = 0x01F1,

    /** register bit select */
    mask_reg_bit = 0x0007,
    /** status register bit select */
    mask_sreg_bit = 0x0070,
    /** address displacement (q) */
    mask_q_displ = 0x2C07,

    /** 5 bit register id  ( R00 - R31 ) */
    mask_A_5 = 0x00F8,
    /** 6 bit IO port id */
    mask_A_6 = 0x060F,
};

/* Some handlers need predeclared */
static int avr_op_CALL (AvrCore *core, uint16_t opcode, unsigned int arg1,
                        unsigned int arg2);
static int avr_op_JMP (AvrCore *core, uint16_t opcode, unsigned int arg1,
                       unsigned int arg2);
static int avr_op_LDS (AvrCore *core, uint16_t opcode, unsigned int arg1,
                       unsigned int arg2);
static int avr_op_STS (AvrCore *core, uint16_t opcode, unsigned int arg1,
                       unsigned int arg2);

/****************************************************************************\
 *
 * Helper functions to extract information from the opcodes.
 *
 \***************************************************************************/

static inline int
get_rd_2 (uint16_t opcode)
{
    int reg = ((opcode & mask_Rd_2) >> 4) & 0x3;
    return (reg * 2) + 24;
}

static inline int
get_rd_3 (uint16_t opcode)
{
    int reg = opcode & mask_Rd_3;
    return ((reg >> 4) & 0x7) + 16;
}

static inline int
get_rd_4 (uint16_t opcode)
{
    int reg = opcode & mask_Rd_4;
    return ((reg >> 4) & 0xf) + 16;
}

static inline int
get_rd_5 (uint16_t opcode)
{
    int reg = opcode & mask_Rd_5;
    return ((reg >> 4) & 0x1f);
}

static inline int
get_rr_3 (uint16_t opcode)
{
    return (opcode & mask_Rr_3) + 16;
}

static inline int
get_rr_4 (uint16_t opcode)
{
    return (opcode & mask_Rr_4) + 16;
}

static inline int
get_rr_5 (uint16_t opcode)
{
    int reg = opcode & mask_Rr_5;
    return (reg & 0xf) + ((reg >> 5) & 0x10);
}

static inline uint8_t
get_K_8 (uint16_t opcode)
{
    int K = opcode & mask_K_8;
    return ((K >> 4) & 0xf0) + (K & 0xf);
}

static inline uint8_t
get_K_6 (uint16_t opcode)
{
    int K = opcode & mask_K_6;
    return ((K >> 2) & 0x0030) + (K & 0xf);
}

static inline int
get_k_7 (uint16_t opcode)
{
    return (((opcode & mask_k_7) >> 3) & 0x7f);
}

static inline int
get_k_12 (uint16_t opcode)
{
    return (opcode & mask_k_12);
}

static inline int
get_k_22 (uint16_t opcode)
{
    /* Masks only the upper 6 bits of the address, the other 16 bits
     * are in PC + 1. */
    int k = opcode & mask_k_22;
    return ((k >> 3) & 0x003e) + (k & 0x1);
}

static inline int
get_reg_bit (uint16_t opcode)
{
    return opcode & mask_reg_bit;
}

static inline int
get_sreg_bit (uint16_t opcode)
{
    return (opcode & mask_sreg_bit) >> 4;
}

static inline int
get_q (uint16_t opcode)
{
    /* 00q0 qq00 0000 0qqq : Yuck! */
    int q = opcode & mask_q_displ;
    int qq = (((q >> 1) & 0x1000) + (q & 0x0c00)) >> 7;
    return (qq & 0x0038) + (q & 0x7);
}

static inline int
get_A_5 (uint16_t opcode)
{
    return (opcode & mask_A_5) >> 3;
}

static inline int
get_A_6 (uint16_t opcode)
{
    int A = opcode & mask_A_6;
    return ((A >> 5) & 0x0030) + (A & 0xf);
}

/****************************************************************************\
 *
 * Helper functions for calculating the status register bit values.
 * See the Atmel data sheet for the instuction set for more info.
 *
\****************************************************************************/

static inline int
get_add_carry (uint8_t res, uint8_t rd, uint8_t rr, int b)
{
    uint8_t resb = res >> b & 0x1;
    uint8_t rdb = rd >> b & 0x1;
    uint8_t rrb = rr >> b & 0x1;
    return (rdb & rrb) | (rrb & ~resb) | (~resb & rdb);
}

static inline int
get_add_overflow (uint8_t res, uint8_t rd, uint8_t rr)
{
    uint8_t res7 = res >> 7 & 0x1;
    uint8_t rd7 = rd >> 7 & 0x1;
    uint8_t rr7 = rr >> 7 & 0x1;
    return (rd7 & rr7 & ~res7) | (~rd7 & ~rr7 & res7);
}

static inline int
get_sub_carry (uint8_t res, uint8_t rd, uint8_t rr, int b)
{
    uint8_t resb = res >> b & 0x1;
    uint8_t rdb = rd >> b & 0x1;
    uint8_t rrb = rr >> b & 0x1;
    return (~rdb & rrb) | (rrb & resb) | (resb & ~rdb);
}

static inline int
get_sub_overflow (uint8_t res, uint8_t rd, uint8_t rr)
{
    uint8_t res7 = res >> 7 & 0x1;
    uint8_t rd7 = rd >> 7 & 0x1;
    uint8_t rr7 = rr >> 7 & 0x1;
    return (rd7 & ~rr7 & ~res7) | (~rd7 & rr7 & res7);
}

static inline int
get_compare_carry (uint8_t res, uint8_t rd, uint8_t rr, int b)
{
    uint8_t resb = res >> b & 0x1;
    uint8_t rdb = rd >> b & 0x1;
    uint8_t rrb = rr >> b & 0x1;
    return (~rdb & rrb) | (rrb & resb) | (resb & ~rdb);
}

static inline int
get_compare_overflow (uint8_t res, uint8_t rd, uint8_t rr)
{
    uint8_t res7 = res >> 7 & 0x1;
    uint8_t rd7 = rd >> 7 & 0x1;
    uint8_t rr7 = rr >> 7 & 0x1;
    /* The atmel data sheet says the second term is ~rd7 for CP
     * but that doesn't make any sense. You be the judge. */
    return (rd7 & ~rr7 & ~res7) | (~rd7 & rr7 & res7);
}

/****************************************************************************\
 *
 * Misc Helper functions
 *
\****************************************************************************/

static inline int
is_next_inst_2_words (AvrCore *core)
{
    /* See if next is a two word instruction
     * CALL, JMP, LDS, and STS are the only two word (32 bit) instructions. */
    uint16_t next_opcode =
        flash_read (core->flash, avr_core_PC_get (core) + 1);
    struct opcode_info *opi = decode_opcode (next_opcode);

    return ((opi->func == avr_op_CALL) || (opi->func == avr_op_JMP)
            || (opi->func == avr_op_LDS) || (opi->func == avr_op_STS));
}

static inline int
n_bit_unsigned_to_signed (unsigned int val, int n)
{
    /* Convert n-bit unsigned value to a signed value. */
    unsigned int mask;

    if ((val & (1 << (n - 1))) == 0)
        return (int)val;

    /* manually calculate two's complement */
    mask = (1 << n) - 1;
    return -1 * ((~val & mask) + 1);
}

/****************************************************************************\
 *
 * Opcode handler functions.
 *
\****************************************************************************/

static int
avr_op_ADC (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Add with Carry.
     *       
     * Opcode     : 0001 11rd dddd rrrr 
     * Usage      : ADC  Rd, Rr
     * Operation  : Rd <- Rd + Rr + C
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int H, V, N, S, Z, C;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint8_t res = rd + rr + avr_core_sreg_get_bit (core, SREG_C);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H = get_add_carry (res, rd, rr, 3));
    sreg = set_bit_in_byte (sreg, SREG_V, V = get_add_overflow (res, rd, rr));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C = get_add_carry (res, rd, rr, 7));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_ADC;
}

static int
avr_op_ADD (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Add without Carry.
     *
     * Opcode     : 0000 11rd dddd rrrr 
     * Usage      : ADD  Rd, Rr
     * Operation  : Rd <- Rd + Rr
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int H, V, N, S, Z, C;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint8_t res = rd + rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H = get_add_carry (res, rd, rr, 3));
    sreg = set_bit_in_byte (sreg, SREG_V, V = get_add_overflow (res, rd, rr));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C = get_add_carry (res, rd, rr, 7));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_ADD;
}

static int
avr_op_ADIW (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Add Immediate to Word.
     *
     * Opcode     : 1001 0110 KKdd KKKK 
     * Usage      : ADIW  Rd, K
     * Operation  : Rd+1:Rd <- Rd+1:Rd + K
     * Flags      : Z,C,N,V,S
     * Num Clocks : 2
     */
    int C, Z, S, N, V;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t rdl = avr_core_gpwr_get (core, Rd);
    uint8_t rdh = avr_core_gpwr_get (core, Rd + 1);

    uint16_t rd = (rdh << 8) + rdl;
    uint16_t res = rd + K;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            (~(rdh >> 7 & 0x1) & (res >> 15 & 0x1)));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 15) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            (~(res >> 15 & 0x1) & (rdh >> 7 & 0x1)));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res & 0xff);
    avr_core_gpwr_set (core, Rd + 1, res >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ADIW;
}

static int
avr_op_AND (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Logical AND.
     *
     * Opcode     : 0010 00rd dddd rrrr 
     * Usage      : AND  Rd, Rr
     * Operation  : Rd <- Rd & Rr
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);
    uint8_t res = rd & rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V = 0);
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_AND;
}

static int
avr_op_ANDI (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Logical AND with Immed.
     *
     * Opcode     : 0111 KKKK dddd KKKK 
     * Usage      : ANDI  Rd, K
     * Operation  : Rd <- Rd & K
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = rd & K;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V = 0);
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_ANDI;
}

static int
avr_op_ASR (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Arithmetic Shift Right.
     *
     * Opcode     : 1001 010d dddd 0101 
     * Usage      : ASR  Rd
     * Operation  : Rd(n) <- Rd(n+1), n=0..6
     * Flags      : Z,C,N,V,S
     * Num Clocks : 1
     */
    int Z, C, N, V, S;

    int Rd = arg1;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = (rd >> 1) + (rd & 0x80);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_C, C = (rd & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_V, V = (N ^ C));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_ASR;
}

static int
avr_op_BCLR (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Clear a single flag or bit in SREG.
     *
     * Opcode     : 1001 0100 1sss 1000 
     * Usage      : BCLR  
     * Operation  : SREG(s) <- 0
     * Flags      : SREG(s)
     * Num Clocks : 1
     */
    avr_core_sreg_set_bit (core, arg1, 0);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_BCLR;
}

static int
avr_op_BLD (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /* Bit load from T to Register.
     *
     * Opcode     : 1111 100d dddd 0bbb 
     * Usage      : BLD  Rd, b
     * Operation  : Rd(b) <- T
     * Flags      : None
     * Num Clocks : 1
     */
    int Rd = arg1;
    int bit = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    int T = avr_core_sreg_get_bit (core, SREG_T);
    uint8_t res;

    if (T == 0)
        res = rd & ~(1 << bit);
    else
        res = rd | (1 << bit);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_BLD;
}

static int
avr_op_BRBC (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Branch if Status Flag Cleared.
     *
     * Pass control directly to the specific bit operation.
     *
     * Opcode     : 1111 01kk kkkk ksss 
     * Usage      : BRBC  s, k
     * Operation  : if (SREG(s) = 0) then PC <- PC + k + 1
     * Flags      : None
     * Num Clocks : 1 / 2
     *
     * k is an relative address represented in two's complements.
     * (64 < k <= 64)
     */
    int bit = arg1;
    int k = arg2;

    if (avr_core_sreg_get_bit (core, bit) == 0)
    {
        avr_core_PC_incr (core, k + 1);
        avr_core_inst_CKS_set (core, 2);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_BRBC;
}

static int
avr_op_BRBS (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Branch if Status Flag Set.
     *
     * Pass control directly to the specific bit operation.
     *
     * Opcode     : 1111 00kk kkkk ksss 
     * Usage      : BRBS  s, k
     * Operation  : if (SREG(s) = 1) then PC <- PC + k + 1
     * Flags      : None
     * Num Clocks : 1 / 2
     *
     * k is an relative address represented in two's complements.
     * (64 < k <= 64)
     */
    int bit = arg1;
    int k = arg2;

    if (avr_core_sreg_get_bit (core, bit) != 0)
    {
        avr_core_PC_incr (core, k + 1);
        avr_core_inst_CKS_set (core, 2);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_BRBS;
}

static int
avr_op_BREAK (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * The BREAK instruction only available on devices with JTAG support. We
     * use it to implement break points for all devices though. When the
     * debugger sets a break point we will replace the insn at the requested
     * PC with the BREAK insn and save the PC and original insn on the break
     * point list. Thus, we only need to walk the break point list when we
     * reach a break insn.
     *
     * When a break occurs, we will return control to the caller _without_
     * incrementing PC as the insn set datasheet says.
     *
     * Opcode     : 1001 0101 1001 1000
     * Usage      : BREAK
     * Operation  : Puts the in Stopped Mode if supported, NOP otherwise.
     * Flags      : None
     * Num Clocks : 1
     */

    avr_message ("BREAK POINT: PC = 0x%08x: clock = %lld\n",
                 avr_core_PC_get (core), avr_core_CK_get (core));

    /* FIXME: TRoth/2002-10-15: Should we execute the original insn which the
       break replaced here or let the caller handle that? For now we defer
       that to the caller. */

/*     return opcode_BREAK; */
    return BREAK_POINT;
}

static int
avr_op_BSET (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Set a single flag or bit in SREG.
     *
     * Opcode     : 1001 0100 0sss 1000 
     * Usage      : BSET
     * Operation  : SREG(s) <- 1
     * Flags      : SREG(s)
     * Num Clocks : 1
     */
    avr_core_sreg_set_bit (core, arg1, 1);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_BSET;
}

static int
avr_op_BST (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Bit Store from Register to T.
     *
     * Opcode     : 1111 101d dddd 0bbb 
     * Usage      : BST  Rd, b
     * Operation  : T <- Rd(b)
     * Flags      : T
     * Num Clocks : 1
     */
    int Rd = arg1;
    int bit = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    avr_core_sreg_set_bit (core, SREG_T, (rd >> bit) & 0x1);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_BST;
}

static int
avr_op_CALL (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Call Subroutine.
     *
     * Opcode     : 1001 010k kkkk 111k kkkk kkkk kkkk kkkk
     * Usage      : CALL  k
     * Operation  : PC <- k
     * Flags      : None
     * Num Clocks : 4 / 5
     */
    int pc = avr_core_PC_get (core);
    int pc_bytes = avr_core_PC_size (core);

    int kh = arg1;
    int kl = flash_read (core->flash, pc + 1);

    int k = (kh << 16) + kl;

    if ((pc_bytes == 2) && (k > 0xffff))
        avr_error ("Address out of allowed range: 0x%06x", k);

    avr_core_stack_push (core, pc_bytes, pc + 2);

    avr_core_PC_set (core, k);
    avr_core_inst_CKS_set (core, pc_bytes + 2);

    return opcode_CALL;
}

static int
avr_op_CBI (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Clear Bit in I/O Register.
     *
     * Opcode     : 1001 1000 AAAA Abbb 
     * Usage      : CBI  A, b
     * Operation  : I/O(A, b) <- 0
     * Flags      : None
     * Num Clocks : 2
     */
    int A = arg1;
    int b = arg2;

    uint8_t val = avr_core_io_read (core, A);
    avr_core_io_write (core, A, val & ~(1 << b));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_CBI;
}

static int
avr_op_COM (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * One's Complement.
     *
     * Opcode     : 1001 010d dddd 0000 
     * Usage      : COM  Rd
     * Operation  : Rd <- $FF - Rd
     * Flags      : Z,C,N,V,S
     * Num Clocks : 1
     */
    int Z, C, N, V, S;

    int Rd = arg1;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = 0xff - rd;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_C, C = 1);
    sreg = set_bit_in_byte (sreg, SREG_V, V = 0);
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_COM;
}

static int
avr_op_CP (AvrCore *core, uint16_t opcode, unsigned int arg1,
           unsigned int arg2)
{
    /*
     * Compare.
     *
     * Opcode     : 0001 01rd dddd rrrr 
     * Usage      : CP  Rd, Rr
     * Operation  : Rd - Rr
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);
    uint8_t res = rd - rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            get_compare_carry (res, rd, rr, 3));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            get_compare_overflow (res, rd, rr));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            get_compare_carry (res, rd, rr, 7));

    avr_core_sreg_set (core, sreg);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_CP;
}

static int
avr_op_CPC (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Compare with Carry.
     *
     * Opcode     : 0000 01rd dddd rrrr 
     * Usage      : CPC  Rd, Rr
     * Operation  : Rd - Rr - C
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H, prev_Z;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);
    uint8_t res = rd - rr - avr_core_sreg_get_bit (core, SREG_C);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            get_compare_carry (res, rd, rr, 3));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            get_compare_overflow (res, rd, rr));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            get_compare_carry (res, rd, rr, 7));

    /* Previous value remains unchanged when result is 0; cleared otherwise */
    Z = ((res & 0xff) == 0);
    prev_Z = avr_core_sreg_get_bit (core, SREG_Z);
    sreg = set_bit_in_byte (sreg, SREG_Z, Z && prev_Z);

    avr_core_sreg_set (core, sreg);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_CPC;
}

static int
avr_op_CPI (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Compare with Immediate.
     *
     * Opcode     : 0011 KKKK dddd KKKK 
     * Usage      : CPI  Rd, K
     * Operation  : Rd - K
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = rd - K;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            get_compare_carry (res, rd, K, 3));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            get_compare_overflow (res, rd, K));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            get_compare_carry (res, rd, K, 7));

    avr_core_sreg_set (core, sreg);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_CPI;
}

static int
avr_op_CPSE (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Compare, Skip if Equal.
     *
     * Opcode     : 0001 00rd dddd rrrr 
     * Usage      : CPSE  Rd, Rr
     * Operation  : if (Rd = Rr) PC <- PC + 2 or 3
     * Flags      : None
     * Num Clocks : 1 / 2 / 3
     */
    int skip;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    if (is_next_inst_2_words (core))
        skip = 3;
    else
        skip = 2;

    if (rd == rr)
    {
        avr_core_PC_incr (core, skip);
        avr_core_inst_CKS_set (core, skip);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_CPSE;
}

static int
avr_op_DEC (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Decrement.
     *
     * Opcode     : 1001 010d dddd 1010 
     * Usage      : DEC  Rd
     * Operation  : Rd <- Rd - 1
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = rd - 1;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_V, V = (rd == 0x80));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_DEC;
}

static int
avr_op_EICALL (AvrCore *core, uint16_t opcode, unsigned int arg1,
               unsigned int arg2)
{
    /*
     * Extended Indirect Call to (Z).
     *
     * Opcode     : 1001 0101 0001 1001 
     * Usage      : EICALL  
     * Operation  : PC(15:0) <- Z, PC(21:16) <- EIND
     * Flags      : None
     * Num Clocks : 4
     */
    int pc = avr_core_PC_get (core);
    int pc_bytes = 3;

    /* Z is R31:R30 */
    int new_pc =
        ((core->EIND & 0x3f) << 16) + (avr_core_gpwr_get (core, 31) << 8) +
        avr_core_gpwr_get (core, 30);

    avr_warning ("needs serious code review\n");

    avr_core_stack_push (core, pc_bytes, pc + 1);

    avr_core_PC_set (core, new_pc);
    avr_core_inst_CKS_set (core, 4);

    return opcode_EICALL;
}

static int
avr_op_EIJMP (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Extended Indirect Jmp to (Z).
     *
     * Opcode     : 1001 0100 0001 1001 
     * Usage      : EIJMP  
     * Operation  : PC(15:0) <- Z, PC(21:16) <- EIND
     * Flags      : None
     * Num Clocks : 2
     */

    /* Z is R31:R30 */
    int new_pc =
        ((core->EIND & 0x3f) << 16) + (avr_core_gpwr_get (core, 31) << 8) +
        avr_core_gpwr_get (core, 30);

    avr_warning ("needs serious code review\n");

    avr_core_PC_set (core, new_pc);
    avr_core_inst_CKS_set (core, 2);

    return opcode_EIJMP;
}

static int
avr_op_ELPM_Z (AvrCore *core, uint16_t opcode, unsigned int arg1,
               unsigned int arg2)
{
    /*
     * Extended Load Program Memory.
     *
     * Opcode     : 1001 000d dddd 0110 
     * Usage      : ELPM  Rd, Z
     * Operation  : R <- (RAMPZ:Z)
     * Flags      : None
     * Num Clocks : 3
     */
    int Z, high_byte, flash_addr;
    uint16_t data;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined");

//    avr_warning ("needs serious code review\n");

    /* FIXME: Is this correct? */
    /* Z is R31:R30 */
    Z = ((avr_core_rampz_get (core) & 0x3f) << 16) +
        (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    high_byte = Z & 0x1;

    flash_addr = Z / 2;

    data = flash_read (core->flash, flash_addr);

    if (high_byte == 1)
        avr_core_gpwr_set (core, Rd, data >> 8);
    else
        avr_core_gpwr_set (core, Rd, data & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 3);

    return opcode_ELPM_Z;
}

static int
avr_op_ELPM_Z_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                    unsigned int arg2)
{
    /*
     * Extended Ld Prg Mem and Post-Incr.
     *
     * Opcode     : 1001 000d dddd 0111 
     * Usage      : ELPM  Rd, Z+
     * Operation  : Rd <- (RAMPZ:Z), Z <- Z + 1
     * Flags      : None
     * Num Clocks : 3
     */
    int Z, high_byte, flash_addr;
    uint16_t data;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined");

    /* TRoth/2002-08-14: This seems to work ok for me. */
    /* avr_warning( "needs serious code review\n" ); */

    /* FIXME: Is this correct? */
    /* Z is R31:R30 */
    Z = ((avr_core_rampz_get (core) & 0x3f) << 16) +
        (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    high_byte = Z & 0x1;

    flash_addr = Z / 2;

    data = flash_read (core->flash, flash_addr);

    if (high_byte == 1)
        avr_core_gpwr_set (core, Rd, data >> 8);
    else
        avr_core_gpwr_set (core, Rd, data & 0xff);

    /* post increment Z */
    Z += 1;
    avr_core_gpwr_set (core, 30, Z & 0xff);
    avr_core_gpwr_set (core, 31, Z >> 8);
    avr_core_rampz_set (core, (Z >> 16) & 0x3f);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 3);

    return opcode_ELPM_Z_incr;
}

static int
avr_op_ELPM (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Extended Load Program Memory.
     *
     * This is the same as avr_op_ELPM_Z with Rd = R0.
     *
     * Opcode     : 1001 0101 1101 1000 
     * Usage      : ELPM  
     * Operation  : R0 <- (RAMPZ:Z)
     * Flags      : None
     * Num Clocks : 3
     */
    avr_op_ELPM_Z (core, 0x9006, arg1, arg2);
    return opcode_ELPM;
}

static int
avr_op_EOR (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Exclusive OR.
     *
     * Opcode     : 0010 01rd dddd rrrr 
     * Usage      : EOR  Rd, Rr
     * Operation  : Rd <- Rd ^ Rr
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint8_t res = rd ^ rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V = 0);
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_EOR;
}

static int
avr_op_ESPM (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Extended Store Program Memory.
     *
     * Opcode     : 1001 0101 1111 1000 
     * Usage      : ESPM  
     * Operation  : (RAMPZ:Z) <- R1:R0
     * Flags      : None
     * Num Clocks : -
     */
    avr_error ("This opcode is not implemented yet: 0x%04x", opcode);
    return opcode_ESPM;
}

/**
 ** I don't know how this Fractional Multiplication works.
 ** If someone wishes to enlighten me, I write these.
 **/

static int
avr_op_FMUL (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Fractional Mult Unsigned.
     *
     * Opcode     : 0000 0011 0ddd 1rrr 
     * Usage      : FMUL  Rd, Rr
     * Operation  : R1:R0 <- (Rd * Rr)<<1 (UU)
     * Flags      : Z,C
     * Num Clocks : 2
     */
    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint16_t resp = rd * rr;
    uint16_t res = resp << 1;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_Z, ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, ((resp >> 15) & 0x1));

    avr_core_sreg_set (core, sreg);

    /* result goes in R1:R0 */
    avr_core_gpwr_set (core, 1, res >> 8);
    avr_core_gpwr_set (core, 0, res & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_FMUL;
}

static int
avr_op_FMULS (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Fractional Mult Signed.
     *
     * Opcode     : 0000 0011 1ddd 0rrr 
     * Usage      : FMULS  Rd, Rr
     * Operation  : R1:R0 <- (Rd * Rr)<<1 (SS)
     * Flags      : Z,C
     * Num Clocks : 2
     */
    int Rd = arg1;
    int Rr = arg2;

    int8_t rd = avr_core_gpwr_get (core, Rd);
    int8_t rr = avr_core_gpwr_get (core, Rr);

    uint16_t resp = rd * rr;
    uint16_t res = resp << 1;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_Z, ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, ((resp >> 15) & 0x1));

    avr_core_sreg_set (core, sreg);

    /* result goes in R1:R0 */
    avr_core_gpwr_set (core, 1, res >> 8);
    avr_core_gpwr_set (core, 0, res & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_FMULS;
}

static int
avr_op_FMULSU (AvrCore *core, uint16_t opcode, unsigned int arg1,
               unsigned int arg2)
{
    /*
     * Fract Mult Signed w/ Unsigned.
     *
     * Opcode     : 0000 0011 1ddd 1rrr 
     * Usage      : FMULSU  Rd, Rr
     * Operation  : R1:R0 <- (Rd * Rr)<<1 (SU)
     * Flags      : Z,C
     * Num Clocks : 2
     */
    int Rd = arg1;
    int Rr = arg2;

    int8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint16_t resp = rd * rr;
    uint16_t res = resp << 1;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_Z, ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, ((resp >> 15) & 0x1));

    avr_core_sreg_set (core, sreg);

    /* result goes in R1:R0 */
    avr_core_gpwr_set (core, 1, res >> 8);
    avr_core_gpwr_set (core, 0, res & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_FMULSU;
}

static int
avr_op_ICALL (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Indirect Call to (Z).
     *
     * Opcode     : 1001 0101 0000 1001 
     * Usage      : ICALL  
     * Operation  : PC(15:0) <- Z, PC(21:16) <- 0
     * Flags      : None
     * Num Clocks : 3 / 4
     */
    int pc = avr_core_PC_get (core);
    int pc_bytes = avr_core_PC_size (core);

    /* Z is R31:R30 */
    int new_pc =
        (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    avr_core_stack_push (core, pc_bytes, pc + 1);

    avr_core_PC_set (core, new_pc);
    avr_core_inst_CKS_set (core, pc_bytes + 1);

    return opcode_ICALL;
}

static int
avr_op_IJMP (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Indirect Jump to (Z).
     *
     * Opcode     : 1001 0100 0000 1001 
     * Usage      : IJMP  
     * Operation  : PC(15:0) <- Z, PC(21:16) <- 0
     * Flags      : None
     * Num Clocks : 2
     */

    /* Z is R31:R30 */
    int new_pc =
        (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);
    avr_core_PC_set (core, new_pc);
    avr_core_inst_CKS_set (core, 2);

    return opcode_IJMP;
}

static int
avr_op_IN (AvrCore *core, uint16_t opcode, unsigned int arg1,
           unsigned int arg2)
{
    /*
     * In From I/O Location.
     *
     * Opcode     : 1011 0AAd dddd AAAA 
     * Usage      : IN  Rd, A
     * Operation  : Rd <- I/O(A)
     * Flags      : None
     * Num Clocks : 1
     */
    int Rd = arg1;
    int A = arg2;

    avr_core_gpwr_set (core, Rd, avr_core_io_read (core, A));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_IN;
}

static int
avr_op_INC (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Increment.
     *
     * Opcode     : 1001 010d dddd 0011 
     * Usage      : INC  Rd
     * Operation  : Rd <- Rd + 1
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = rd + 1;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_V, V = (rd == 0x7f));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_INC;
}

static int
avr_op_JMP (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Jump.
     *
     * Opcode     : 1001 010k kkkk 110k kkkk kkkk kkkk kkkk
     * Usage      : JMP  k
     * Operation  : PC <- k
     * Flags      : None
     * Num Clocks : 3
     */
    int kh = arg1;
    int kl = flash_read (core->flash, avr_core_PC_get (core) + 1);

    int k = (kh << 16) + kl;

    avr_core_PC_set (core, k);
    avr_core_inst_CKS_set (core, 3);

    return opcode_JMP;
}

static int
avr_op_LDD_Y (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Load Indirect with Displacement using index Y.
     *
     * Opcode     : 10q0 qq0d dddd 1qqq 
     * Usage      : LDD  Rd, Y+q
     * Operation  : Rd <- (Y + q)
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Y;
    int Rd = arg1;
    int q = arg2;

    /* Y is R29:R28 */
    Y = (avr_core_gpwr_get (core, 29) << 8) + avr_core_gpwr_get (core, 28);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, Y + q));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LDD_Y;
}

static int
avr_op_LDD_Z (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Load Indirect with Displacement using index Z.
     *
     * Opcode     : 10q0 qq0d dddd 0qqq 
     * Usage      : LDD  Rd, Z+q
     * Operation  : Rd <- (Z + q)
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Z;

    int Rd = arg1;
    int q = arg2;

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, Z + q));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LDD_Z;
}

static int
avr_op_LDI (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Load Immediate.
     *
     * Opcode     : 1110 KKKK dddd KKKK 
     * Usage      : LDI  Rd, K
     * Operation  : Rd  <- K
     * Flags      : None
     * Num Clocks : 1
     */
    int Rd = arg1;
    uint8_t K = arg2;

    avr_core_gpwr_set (core, Rd, K);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_LDI;
}

static int
avr_op_LDS (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Load Direct from data space.
     *
     * Opcode     : 1001 000d dddd 0000 kkkk kkkk kkkk kkkk
     * Usage      : LDS  Rd, k
     * Operation  : Rd <- (k)
     * Flags      : None
     * Num Clocks : 2
     */
    int Rd = arg1;

    /* Get data at k in current data segment and put into Rd */
    int k_pc = avr_core_PC_get (core) + 1;
    int k = flash_read (core->flash, k_pc);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, k));

    avr_core_PC_incr (core, 2);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LDS;
}

static int
avr_op_LD_X (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Load Indirect using index X.
     *
     * Opcode     : 1001 000d dddd 1100 
     * Usage      : LD  Rd, X
     * Operation  : Rd <- (X)
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t X;

    int Rd = arg1;

    /* X is R27:R26 */
    X = (avr_core_gpwr_get (core, 27) << 8) + avr_core_gpwr_get (core, 26);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, X));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_X;
}

static int
avr_op_LD_X_decr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Load Indirect and Pre-Decrement using index X.
     *
     * Opcode     : 1001 000d dddd 1110 
     * Usage      : LD  Rd, -X
     * Operation  : X <- X - 1, Rd <- (X)
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t X;

    int Rd = arg1;

    if ((Rd == 26) || (Rd == 27))
        avr_error ("Results of operation are undefined");

    /* X is R27:R26 */
    X = (avr_core_gpwr_get (core, 27) << 8) + avr_core_gpwr_get (core, 26);

    /* Perform pre-decrement */
    X -= 1;
    avr_core_gpwr_set (core, 26, X & 0xff);
    avr_core_gpwr_set (core, 27, X >> 8);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, X));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_X_decr;
}

static int
avr_op_LD_X_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Load Indirect and Post-Increment using index X.
     *
     * Opcode     : 1001 000d dddd 1101 
     * Usage      : LD  Rd, X+
     * Operation  : Rd <- (X), X <- X + 1
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t X;

    int Rd = arg1;

    if ((Rd == 26) || (Rd == 27))
        avr_error ("Results of operation are undefined");

    /* X is R27:R26 */
    X = (avr_core_gpwr_get (core, 27) << 8) + avr_core_gpwr_get (core, 26);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, X));

    /* Perform post-increment */
    X += 1;
    avr_core_gpwr_set (core, 26, X & 0xff);
    avr_core_gpwr_set (core, 27, X >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_X_incr;
}

static int
avr_op_LD_Y_decr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Load Indirect and PreDecrement using index Y.
     *
     * Opcode     : 1001 000d dddd 1010 
     * Usage      : LD  Rd, -Y
     * Operation  : Y <- Y - 1, Rd <- (Y)
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Y;

    int Rd = arg1;

    if ((Rd == 28) || (Rd == 29))
        avr_error ("Results of operation are undefined");

    /* Y is R29:R28 */
    Y = (avr_core_gpwr_get (core, 29) << 8) + avr_core_gpwr_get (core, 28);

    /* Perform pre-decrement */
    Y -= 1;
    avr_core_gpwr_set (core, 28, Y & 0xff);
    avr_core_gpwr_set (core, 29, Y >> 8);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, Y));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_Y_decr;
}

static int
avr_op_LD_Y_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Load Indirect and Post-Increment using index Y.
     *
     * Opcode     : 1001 000d dddd 1001 
     * Usage      : LD  Rd, Y+
     * Operation  : Rd <- (Y), Y <- Y + 1
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Y;

    int Rd = arg1;

    if ((Rd == 28) || (Rd == 29))
        avr_error ("Results of operation are undefined");

    /* Y is R29:R28 */
    Y = (avr_core_gpwr_get (core, 29) << 8) + avr_core_gpwr_get (core, 28);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, Y));

    /* Perform post-increment */
    Y += 1;
    avr_core_gpwr_set (core, 28, Y & 0xff);
    avr_core_gpwr_set (core, 29, Y >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_Y_incr;
}

static int
avr_op_LD_Z_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Load Indirect and Post-Increment using index Z.
     *
     * Opcode     : 1001 000d dddd 0001 
     * Usage      : LD  Rd, Z+
     * Operation  : Rd <- (Z), Z <- Z+1
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Z;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined");

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, Z));

    /* Perform post-increment */
    Z += 1;
    avr_core_gpwr_set (core, 30, Z & 0xff);
    avr_core_gpwr_set (core, 31, Z >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_Z_incr;
}

static int
avr_op_LD_Z_decr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Load Indirect and Pre-Decrement using index Z.
     *
     * Opcode     : 1001 000d dddd 0010 
     * Usage      : LD  Rd, -Z
     * Operation  : Z <- Z - 1, Rd <- (Z)
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Z;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined");

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    /* Perform pre-decrement */
    Z -= 1;
    avr_core_gpwr_set (core, 30, Z & 0xff);
    avr_core_gpwr_set (core, 31, Z >> 8);

    avr_core_gpwr_set (core, Rd, avr_core_mem_read (core, Z));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_LD_Z_decr;
}

static int
avr_op_LPM_Z (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Load Program Memory.
     *
     * Opcode     : 1001 000d dddd 0100 
     * Usage      : LPM  Rd, Z
     * Operation  : Rd <- (Z)
     * Flags      : None
     * Num Clocks : 3
     */
    uint16_t Z, high_byte;
    uint16_t data;

    int Rd = arg1;

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);
    high_byte = Z & 0x1;

    /* FIXME: I don't know if this is the right thing to do. I'm not sure that
       I understand what the instruction data sheet is saying about Z.
       Dividing by 2 seems to give the address that we want though. */

    data = flash_read (core->flash, Z / 2);

    if (high_byte == 1)
        avr_core_gpwr_set (core, Rd, data >> 8);
    else
        avr_core_gpwr_set (core, Rd, data & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 3);

    return opcode_LPM_Z;
}

static int
avr_op_LPM (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /* Load Program Memory.
     *
     * This the same as avr_op_LPM_Z with Rd = R0.
     *
     * Opcode     : 1001 0101 1100 1000 
     * Usage      : LPM  
     * Operation  : R0 <- (Z)
     * Flags      : None
     * Num Clocks : 3
     */
    return avr_op_LPM_Z (core, 0x9004, 0, arg2);
}

static int
avr_op_LPM_Z_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                   unsigned int arg2)
{
    /*
     * Load Program Memory and Post-Incr.
     *
     * Opcode     : 1001 000d dddd 0101 
     * Usage      : LPM  Rd, Z+
     * Operation  : Rd <- (Z), Z <- Z + 1
     * Flags      : None
     * Num Clocks : 3
     */
    uint16_t Z, high_byte;
    uint16_t data;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined");

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);
    high_byte = Z & 0x1;

    /* FIXME: I don't know if this is the right thing to do. I'm not sure that
       I understand what the instruction data sheet is saying about Z.
       Dividing by 2 seems to give the address that we want though. */

    data = flash_read (core->flash, Z / 2);

    if (high_byte == 1)
        avr_core_gpwr_set (core, Rd, data >> 8);
    else
        avr_core_gpwr_set (core, Rd, data & 0xff);

    /* Perform post-increment */
    Z += 1;
    avr_core_gpwr_set (core, 30, Z & 0xff);
    avr_core_gpwr_set (core, 31, Z >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 3);

    return opcode_LPM_Z_incr;
}

static int
avr_op_LSR (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Logical Shift Right.
     *
     * Opcode     : 1001 010d dddd 0110 
     * Usage      : LSR  Rd
     * Operation  : Rd(n) <- Rd(n+1), Rd(7) <- 0, C <- Rd(0)
     * Flags      : Z,C,N,V,S
     * Num Clocks : 1
     */
    int Z, C, N, V, S;

    int Rd = arg1;
    uint8_t rd = avr_core_gpwr_get (core, Rd);

    uint8_t res = (rd >> 1) & 0x7f;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_C, C = (rd & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_N, N = (0));
    sreg = set_bit_in_byte (sreg, SREG_V, V = (N ^ C));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_LSR;
}

static int
avr_op_MOV (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /* Copy Register.
     *
     * Opcode     : 0010 11rd dddd rrrr 
     * Usage      : MOV  Rd, Rr
     * Operation  : Rd <- Rr
     * Flags      : None
     * Num Clocks : 1
     */
    int Rd = arg1;
    int Rr = arg2;

    avr_core_gpwr_set (core, Rd, avr_core_gpwr_get (core, Rr));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_MOV;
}

static int
avr_op_MOVW (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     *Copy Register Pair.
     *
     * Opcode     : 0000 0001 dddd rrrr 
     * Usage      : MOVW  Rd, Rr
     * Operation  : Rd+1:Rd <- Rr+1:Rr
     * Flags      : None
     * Num Clocks : 1
     */
    int Rd = arg1;
    int Rr = arg2;

    /* get_rd_4() returns 16 <= r <= 31, but here Rd and Rr */
    /* are even from 0 <= r <= 30. So we translate. */
    Rd = (Rd - 16) * 2;
    Rr = (Rr - 16) * 2;

    avr_core_gpwr_set (core, Rd, avr_core_gpwr_get (core, Rr));
    avr_core_gpwr_set (core, Rd + 1, avr_core_gpwr_get (core, Rr + 1));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_MOVW;
}

static int
avr_op_MUL (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Mult Unsigned.
     *
     * Opcode     : 1001 11rd dddd rrrr 
     * Usage      : MUL  Rd, Rr
     * Operation  : R1:R0 <- Rd * Rr (UU)
     * Flags      : Z,C
     * Num Clocks : 2
     */
    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint16_t res = rd * rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_Z, ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, ((res >> 15) & 0x1));

    avr_core_sreg_set (core, sreg);

    /* result goes in R1:R0 */

    avr_core_gpwr_set (core, 1, res >> 8);
    avr_core_gpwr_set (core, 0, res & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_MUL;
}

static int
avr_op_MULS (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Mult Signed.
     *
     * Opcode     : 0000 0010 dddd rrrr 
     * Usage      : MULS  Rd, Rr
     * Operation  : R1:R0 <- Rd * Rr (SS)
     * Flags      : Z,C
     * Num Clocks : 2
     */
    int Rd = arg1;
    int Rr = arg2;

    int8_t rd = (int8_t) avr_core_gpwr_get (core, Rd);
    int8_t rr = (int8_t) avr_core_gpwr_get (core, Rr);
    int16_t res = rd * rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_Z, ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, ((res >> 15) & 0x1));

    avr_core_sreg_set (core, sreg);

    /* result goes in R1:R0 */
    avr_core_gpwr_set (core, 1, res >> 8);
    avr_core_gpwr_set (core, 0, res & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_MULS;
}

static int
avr_op_MULSU (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Mult Signed with Unsigned.
     * 
     * Rd(unsigned),Rr(signed), result (signed)
     *
     * Opcode     : 0000 0011 0ddd 0rrr 
     * Usage      : MULSU  Rd, Rr
     * Operation  : R1:R0 <- Rd * Rr (SU)
     * Flags      : Z,C
     * Num Clocks : 2
     */
    int Rd = arg1;
    int Rr = arg2;

    int8_t rd = (int8_t) avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    int16_t res = rd * rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_Z, ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, ((res >> 15) & 0x1));

    avr_core_sreg_set (core, sreg);

    /* result goes in R1:R0 */
    avr_core_gpwr_set (core, 1, res >> 8);
    avr_core_gpwr_set (core, 0, res & 0xff);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_MULSU;
}

static int
avr_op_NEG (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Two's Complement.
     *
     * Opcode     : 1001 010d dddd 0001 
     * Usage      : NEG  Rd
     * Operation  : Rd <- $00 - Rd
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t res = (0x0 - rd) & 0xff;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            (((res >> 3) | (rd >> 3)) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_V, V = (res == 0x80));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = (res == 0x0));
    sreg = set_bit_in_byte (sreg, SREG_C, C = (res != 0x0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_NEG;
}

static int
avr_op_NOP (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    static uint64_t oldCK;
    uint64_t CK;
    /*
     * No Operation.
     *
     * Opcode     : 0000 0000 0000 0000 
     * Usage      : NOP  
     * Operation  : None
     * Flags      : None
     * Num Clocks : 1
     */
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);
    CK = avr_core_CK_get (core);
    fprintf(stderr, "NOP at address %x, clock cycles %"PRIu64", difference to previous CK: %"PRIu64"\n", avr_core_PC_get(core), CK, CK - oldCK);
    oldCK = CK;
    return opcode_NOP;
}

static int
avr_op_OR (AvrCore *core, uint16_t opcode, unsigned int arg1,
           unsigned int arg2)
{
    /*
     * Logical OR.
     *
     * Opcode     : 0010 10rd dddd rrrr 
     * Usage      : OR  Rd, Rr
     * Operation  : Rd <- Rd or Rr
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t res = avr_core_gpwr_get (core, Rd) | avr_core_gpwr_get (core, Rr);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V = (0));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = (res == 0x0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_OR;
}

static int
avr_op_ORI (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Logical OR with Immed.
     *
     * Opcode     : 0110 KKKK dddd KKKK 
     * Usage      : ORI  Rd, K
     * Operation  : Rd <- Rd or K
     * Flags      : Z,N,V,S
     * Num Clocks : 1
     */
    int Z, N, V, S;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t res = avr_core_gpwr_get (core, Rd) | K;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V = (0));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = (res == 0x0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_ORI;
}

static int
avr_op_OUT (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Out To I/O Location.
     *
     * Opcode     : 1011 1AAd dddd AAAA 
     * Usage      : OUT  A, Rd
     * Operation  : I/O(A) <- Rd
     * Flags      : None
     * Num Clocks : 1
     */

    /* Even though the args in the comment are reversed (out arg2, arg1), the
       following is correct: Rd=arg1, A=arg2. */
    int Rd = arg1;
    int A = arg2;

    avr_core_io_write (core, A, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_OUT;
}

static int
avr_op_POP (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Pop Register from Stack.
     *
     * Opcode     : 1001 000d dddd 1111 
     * Usage      : POP  Rd
     * Operation  : Rd <- STACK
     * Flags      : None
     * Num Clocks : 2
     */
    int Rd = arg1;

    avr_core_gpwr_set (core, Rd, avr_core_stack_pop (core, 1));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_POP;
}

static int
avr_op_PUSH (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Push Register on Stack.
     *
     * Opcode     : 1001 001d dddd 1111 
     * Usage      : PUSH  Rd
     * Operation  : STACK <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    int Rd = arg1;

    avr_core_stack_push (core, 1, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_PUSH;
}

static int
avr_op_RCALL (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Relative Call Subroutine.
     *
     * Opcode     : 1101 kkkk kkkk kkkk 
     * Usage      : RCALL  k
     * Operation  : PC <- PC + k + 1
     * Flags      : None
     * Num Clocks : 3 / 4
     */
    int k = arg1;

    int pc = avr_core_PC_get (core);
    int pc_bytes = avr_core_PC_size (core);

    avr_core_stack_push (core, pc_bytes, pc + 1);

    avr_core_PC_incr (core, k + 1);
    avr_core_inst_CKS_set (core, pc_bytes + 1);

    return opcode_RCALL;
}

static int
avr_op_RET (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Subroutine Return.
     *
     * Opcode     : 1001 0101 0000 1000 
     * Usage      : RET  
     * Operation  : PC <- STACK
     * Flags      : None
     * Num Clocks : 4 / 5
     */
    int pc_bytes = avr_core_PC_size (core);
    int pc = avr_core_stack_pop (core, pc_bytes);

    avr_core_PC_set (core, pc);
    avr_core_inst_CKS_set (core, pc_bytes + 2);

    return opcode_RET;
}

static int
avr_op_RETI (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Interrupt Return.
     *
     * Opcode     : 1001 0101 0001 1000 
     * Usage      : RETI  
     * Operation  : PC <- STACK
     * Flags      : I
     * Num Clocks : 4 / 5
     */
    int pc_bytes = avr_core_PC_size (core);
    int pc = avr_core_stack_pop (core, pc_bytes);

    avr_core_PC_set (core, pc);
    avr_core_inst_CKS_set (core, pc_bytes + 2);

    avr_core_sreg_set_bit (core, SREG_I, 1);

    return opcode_RETI;
}

static int
avr_op_RJMP (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Relative Jump.
     *
     * Opcode     : 1100 kkkk kkkk kkkk 
     * Usage      : RJMP  k
     * Operation  : PC <- PC + k + 1
     * Flags      : None
     * Num Clocks : 2
     */
    int k = arg1;

    avr_core_PC_incr (core, k + 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_RJMP;
}

static int
avr_op_ROR (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Rotate Right Though Carry.
     *
     * Opcode     : 1001 010d dddd 0111 
     * Usage      : ROR  Rd
     * Operation  : Rd(7) <- C, Rd(n) <- Rd(n+1), C <- Rd(0)
     * Flags      : Z,C,N,V,S
     * Num Clocks : 1
     */
    int Z, C, N, V, S;

    int Rd = arg1;
    uint8_t rd = avr_core_gpwr_get (core, Rd);

    uint8_t res =
        (rd >> 1) | ((avr_core_sreg_get_bit (core, SREG_C) << 7) & 0x80);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_C, C = (rd & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_V, V = (N ^ C));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = (res == 0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_ROR;
}

static int
avr_op_SBC (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Subtract with Carry.
     *
     * Opcode     : 0000 10rd dddd rrrr 
     * Usage      : SBC  Rd, Rr
     * Operation  : Rd <- Rd - Rr - C
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint8_t res = rd - rr - avr_core_sreg_get_bit (core, SREG_C);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            (get_sub_carry (res, rd, rr, 3)));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            (get_sub_overflow (res, rd, rr)));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            (get_sub_carry (res, rd, rr, 7)));

    if ((res & 0xff) != 0)
        sreg = set_bit_in_byte (sreg, SREG_Z, Z = (0));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_SBC;
}

static int
avr_op_SBCI (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Subtract Immediate with Carry.
     *
     * Opcode     : 0100 KKKK dddd KKKK 
     * Usage      : SBCI  Rd, K
     * Operation  : Rd <- Rd - K - C
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);

    uint8_t res = rd - K - avr_core_sreg_get_bit (core, SREG_C);

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            (get_sub_carry (res, rd, K, 3)));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            (get_sub_overflow (res, rd, K)));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            (get_sub_carry (res, rd, K, 7)));

    if ((res & 0xff) != 0)
        sreg = set_bit_in_byte (sreg, SREG_Z, Z = 0);

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_SBCI;
}

static int
avr_op_SBI (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Set Bit in I/O Register.
     *
     * Opcode     : 1001 1010 AAAA Abbb 
     * Usage      : SBI  A, b
     * Operation  : I/O(A, b) <- 1
     * Flags      : None
     * Num Clocks : 2
     */
    int A = arg1;
    int b = arg2;

    uint8_t val = avr_core_io_read (core, A);
    avr_core_io_write (core, A, val | (1 << b));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_SBI;
}

static int
avr_op_SBIC (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Skip if Bit in I/O Reg Cleared.
     *
     * Opcode     : 1001 1001 AAAA Abbb 
     * Usage      : SBIC  A, b
     * Operation  : if (I/O(A,b) = 0) PC <- PC + 2 or 3
     * Flags      : None
     * Num Clocks : 1 / 2 / 3
     */
    int skip;

    int A = arg1;
    int b = arg2;

    if (is_next_inst_2_words (core))
        skip = 3;
    else
        skip = 2;

    if ((avr_core_io_read (core, A) & (1 << b)) == 0)
    {
        avr_core_PC_incr (core, skip);
        avr_core_inst_CKS_set (core, skip);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_SBIC;
}

static int
avr_op_SBIS (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Skip if Bit in I/O Reg Set.
     *
     * Opcode     : 1001 1011 AAAA Abbb 
     * Usage      : SBIS  A, b
     * Operation  : if (I/O(A,b) = 1) PC <- PC + 2 or 3
     * Flags      : None
     * Num Clocks : 1 / 2 / 3
     */
    int skip;

    int A = arg1;
    int b = arg2;

    if (is_next_inst_2_words (core))
        skip = 3;
    else
        skip = 2;

    if ((avr_core_io_read (core, A) & (1 << b)) != 0)
    {
        avr_core_PC_incr (core, skip);
        avr_core_inst_CKS_set (core, skip);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_SBIS;
}

static int
avr_op_SBIW (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Subtract Immed from Word.
     *
     * Opcode     : 1001 0111 KKdd KKKK 
     * Usage      : SBIW  Rd, K
     * Operation  : Rd+1:Rd <- Rd+1:Rd - K
     * Flags      : Z,C,N,V,S
     * Num Clocks : 2
     */
    int Z, C, N, V, S;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t rdl = avr_core_gpwr_get (core, Rd);
    uint8_t rdh = avr_core_gpwr_get (core, Rd + 1);

    uint16_t rd = (rdh << 8) + rdl;

    uint16_t res = rd - K;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            ((rdh >> 7 & 0x1) & ~(res >> 15 & 0x1)));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 15) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xffff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            ((res >> 15 & 0x1) & ~(rdh >> 7 & 0x1)));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res & 0xff);
    avr_core_gpwr_set (core, Rd + 1, res >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_SBIW;
}

static int
avr_op_SBRC (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Skip if Bit in Reg Cleared.
     *
     * Opcode     : 1111 110d dddd 0bbb 
     * Usage      : SBRC  Rd, b
     * Operation  : if (Rd(b) = 0) PC <- PC + 2 or 3
     * Flags      : None
     * Num Clocks : 1 / 2 / 3
     */
    int skip;

    int Rd = arg1;
    int b = arg2;

    if (is_next_inst_2_words (core))
        skip = 3;
    else
        skip = 2;

    if (((avr_core_gpwr_get (core, Rd) >> b) & 0x1) == 0)
    {
        avr_core_PC_incr (core, skip);
        avr_core_inst_CKS_set (core, skip);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_SBRC;
}

static int
avr_op_SBRS (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Skip if Bit in Reg Set.
     *
     * Opcode     : 1111 111d dddd 0bbb 
     * Usage      : SBRS  Rd, b
     * Operation  : if (Rd(b) = 1) PC <- PC + 2 or 3
     * Flags      : None
     * Num Clocks : 1 / 2 / 3
     */
    int skip;

    int Rd = arg1;
    int b = arg2;

    if (is_next_inst_2_words (core))
        skip = 3;
    else
        skip = 2;

    if (((avr_core_gpwr_get (core, Rd) >> b) & 0x1) != 0)
    {
        avr_core_PC_incr (core, skip);
        avr_core_inst_CKS_set (core, skip);
    }
    else
    {
        avr_core_PC_incr (core, 1);
        avr_core_inst_CKS_set (core, 1);
    }

    return opcode_SBRS;
}

static int
avr_op_SLEEP (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Sleep.
     *
     * This is device specific and should be overridden by sub-class.
     *
     * Opcode     : 1001 0101 1000 1000 
     * Usage      : SLEEP  
     * Operation  : (see specific hardware specification for Sleep)
     * Flags      : None
     * Num Clocks : 1
     */
    MCUCR *mcucr = (MCUCR *)avr_core_get_vdev_by_name (core, "MCUCR");

    if (mcucr == NULL)
        avr_error ("MCUCR register not installed");

    /* See if sleep mode is enabled */
    if (mcucr_get_bit (mcucr, bit_SE))
    {
        if (mcucr_get_bit (mcucr, bit_SM) == 0)
        {
            /* Idle Mode */
            avr_core_set_sleep_mode (core, SLEEP_MODE_IDLE);
        }
        else
        {
            /* Power Down Mode */
            avr_core_set_sleep_mode (core, SLEEP_MODE_PWR_DOWN);
        }
    }

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_SLEEP;
}

static int
avr_op_SPM (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Store Program Memory.
     *
     * Opcode     : 1001 0101 1110 1000 
     * Usage      : SPM  
     * Operation  : (Z) <- R1:R0
     * Flags      : None
     * Num Clocks : -
     */
    int Z;

    Z = ((avr_core_rampz_get (core) & 0x3f) << 16) +(avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);
    avr_core_spm (core, avr_core_gpwr_get (core, 0), avr_core_gpwr_get (core, 1), Z);
    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_SPM;
}

static int
avr_op_STD_Y (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Store Indirect with Displacement.
     *
     * Opcode     : 10q0 qq1d dddd 1qqq 
     * Usage      : STD  Y+q, Rd
     * Operation  : (Y + q) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    int Y;

    int q = arg2;
    int Rd = arg1;

    /* Y is R29:R28 */
    Y = (avr_core_gpwr_get (core, 29) << 8) + avr_core_gpwr_get (core, 28);

    avr_core_mem_write (core, Y + q, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_STD_Y;
}

static int
avr_op_STD_Z (AvrCore *core, uint16_t opcode, unsigned int arg1,
              unsigned int arg2)
{
    /*
     * Store Indirect with Displacement.
     *
     * Opcode     : 10q0 qq1d dddd 0qqq 
     * Usage      : STD  Z+q, Rd
     * Operation  : (Z + q) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    int Z;

    int q = arg2;
    int Rd = arg1;

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    avr_core_mem_write (core, Z + q, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_STD_Z;
}

static int
avr_op_STS (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Store Direct to data space.
     *
     * Opcode     : 1001 001d dddd 0000 kkkk kkkk kkkk kkkk
     * Usage      : STS  k, Rd
     * Operation  : (k) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    int Rd = arg1;

    /* Get data at k in current data segment and put into Rd */
    int k_pc = avr_core_PC_get (core) + 1;
    int k = flash_read (core->flash, k_pc);

    avr_core_mem_write (core, k, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 2);
    avr_core_inst_CKS_set (core, 2);

    return opcode_STS;
}

static int
avr_op_ST_X (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Store Indirect using index X.
     *
     * Opcode     : 1001 001d dddd 1100 
     * Usage      : ST  X, Rd
     * Operation  : (X) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t X;

    int Rd = arg1;

    /* X is R27:R26 */
    X = (avr_core_gpwr_get (core, 27) << 8) + avr_core_gpwr_get (core, 26);

    avr_core_mem_write (core, X, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_X;
}

static int
avr_op_ST_X_decr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Store Indirect and Pre-Decrement using index X.
     *
     * Opcode     : 1001 001d dddd 1110 
     * Usage      : ST  -X, Rd
     * Operation  : X <- X - 1, (X) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t X;

    int Rd = arg1;

    if ((Rd == 26) || (Rd == 27))
        avr_error ("Results of operation are undefined: 0x%04x", opcode);

    /* X is R27:R26 */
    X = (avr_core_gpwr_get (core, 27) << 8) + avr_core_gpwr_get (core, 26);

    /* Perform pre-decrement */
    X -= 1;
    avr_core_gpwr_set (core, 26, X & 0xff);
    avr_core_gpwr_set (core, 27, X >> 8);

    avr_core_mem_write (core, X, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_X_decr;
}

static int
avr_op_ST_X_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Store Indirect and Post-Increment using index X.
     *
     * Opcode     : 1001 001d dddd 1101 
     * Usage      : ST  X+, Rd
     * Operation  : (X) <- Rd, X <- X + 1
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t X;

    int Rd = arg1;

    if ((Rd == 26) || (Rd == 27))
        avr_error ("Results of operation are undefined: 0x%04x", opcode);

    /* X is R27:R26 */
    X = (avr_core_gpwr_get (core, 27) << 8) + avr_core_gpwr_get (core, 26);

    avr_core_mem_write (core, X, avr_core_gpwr_get (core, Rd));

    /* Perform post-increment */
    X += 1;
    avr_core_gpwr_set (core, 26, X & 0xff);
    avr_core_gpwr_set (core, 27, X >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_X_incr;
}

static int
avr_op_ST_Y_decr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Store Indirect and Pre-Decrement using index Y.
     *
     * Opcode     : 1001 001d dddd 1010 
     * Usage      : ST  -Y, Rd
     * Operation  : Y <- Y - 1, (Y) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Y;

    int Rd = arg1;

    if ((Rd == 28) || (Rd == 29))
        avr_error ("Results of operation are undefined: 0x%04x", opcode);

    /* Y is R29:R28 */
    Y = (avr_core_gpwr_get (core, 29) << 8) + avr_core_gpwr_get (core, 28);

    /* Perform pre-decrement */
    Y -= 1;
    avr_core_gpwr_set (core, 28, Y & 0xff);
    avr_core_gpwr_set (core, 29, Y >> 8);

    avr_core_mem_write (core, Y, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_Y_decr;
}

static int
avr_op_ST_Y_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Store Indirect and Post-Increment using index Y.
     *
     * Opcode     : 1001 001d dddd 1001 
     * Usage      : ST  Y+, Rd
     * Operation  : (Y) <- Rd, Y <- Y + 1
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Y;

    int Rd = arg1;

    if ((Rd == 28) || (Rd == 29))
        avr_error ("Results of operation are undefined: 0x%04x", opcode);

    /* Y is R29:R28 */
    Y = (avr_core_gpwr_get (core, 29) << 8) + avr_core_gpwr_get (core, 28);

    avr_core_mem_write (core, Y, avr_core_gpwr_get (core, Rd));

    /* Perform post-increment */
    Y += 1;
    avr_core_gpwr_set (core, 28, Y & 0xff);
    avr_core_gpwr_set (core, 29, Y >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_Y_incr;
}

static int
avr_op_ST_Z_decr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Store Indirect and Pre-Decrement using index Z.
     *
     * Opcode     : 1001 001d dddd 0010 
     * Usage      : ST  -Z, Rd
     * Operation  : Z <- Z - 1, (Z) <- Rd
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Z;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined: 0x%04x", opcode);

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    /* Perform pre-decrement */
    Z -= 1;
    avr_core_gpwr_set (core, 30, Z & 0xff);
    avr_core_gpwr_set (core, 31, Z >> 8);

    avr_core_mem_write (core, Z, avr_core_gpwr_get (core, Rd));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_Z_decr;
}

static int
avr_op_ST_Z_incr (AvrCore *core, uint16_t opcode, unsigned int arg1,
                  unsigned int arg2)
{
    /*
     * Store Indirect and Post-Increment using index Z.
     *
     * Opcode     : 1001 001d dddd 0001 
     * Usage      : ST  Z+, Rd
     * Operation  : (Z) <- Rd, Z <- Z + 1
     * Flags      : None
     * Num Clocks : 2
     */
    uint16_t Z;

    int Rd = arg1;

    if ((Rd == 30) || (Rd == 31))
        avr_error ("Results of operation are undefined: 0x%04x", opcode);

    /* Z is R31:R30 */
    Z = (avr_core_gpwr_get (core, 31) << 8) + avr_core_gpwr_get (core, 30);

    avr_core_mem_write (core, Z, avr_core_gpwr_get (core, Rd));

    /* Perform post-increment */
    Z += 1;
    avr_core_gpwr_set (core, 30, Z & 0xff);
    avr_core_gpwr_set (core, 31, Z >> 8);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 2);

    return opcode_ST_Z_incr;
}

static int
avr_op_SUB (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /*
     * Subtract without Carry.
     *
     * Opcode     : 0001 10rd dddd rrrr 
     * Usage      : SUB  Rd, Rr
     * Operation  : Rd <- Rd - Rr
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;
    int Rr = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);
    uint8_t rr = avr_core_gpwr_get (core, Rr);

    uint8_t res = rd - rr;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            (get_sub_carry (res, rd, rr, 3)));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            (get_sub_overflow (res, rd, rr)));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            (get_sub_carry (res, rd, rr, 7)));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_SUB;
}

static int
avr_op_SUBI (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Subtract Immediate.
     *
     * Opcode     : 0101 KKKK dddd KKKK 
     * Usage      : SUBI  Rd, K
     * Operation  : Rd <- Rd - K
     * Flags      : Z,C,N,V,S,H
     * Num Clocks : 1
     */
    int Z, C, N, V, S, H;

    int Rd = arg1;
    uint8_t K = arg2;

    uint8_t rd = avr_core_gpwr_get (core, Rd);

    uint8_t res = rd - K;

    uint8_t sreg = avr_core_sreg_get (core);

    sreg = set_bit_in_byte (sreg, SREG_H, H =
                            (get_sub_carry (res, rd, K, 3)));
    sreg = set_bit_in_byte (sreg, SREG_V, V =
                            (get_sub_overflow (res, rd, K)));
    sreg = set_bit_in_byte (sreg, SREG_N, N = ((res >> 7) & 0x1));
    sreg = set_bit_in_byte (sreg, SREG_S, S = (N ^ V));
    sreg = set_bit_in_byte (sreg, SREG_Z, Z = ((res & 0xff) == 0));
    sreg = set_bit_in_byte (sreg, SREG_C, C =
                            (get_sub_carry (res, rd, K, 7)));

    avr_core_sreg_set (core, sreg);

    avr_core_gpwr_set (core, Rd, res);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_SUBI;
}

static int
avr_op_SWAP (AvrCore *core, uint16_t opcode, unsigned int arg1,
             unsigned int arg2)
{
    /*
     * Swap Nibbles.
     * 
     * Opcode     : 1001 010d dddd 0010 
     * Usage      : SWAP  Rd
     * Operation  : Rd(3..0) <--> Rd(7..4)
     * Flags      : None
     * Num Clocks : 1
     */
    int Rd = arg1;
    uint8_t rd = avr_core_gpwr_get (core, Rd);

    avr_core_gpwr_set (core, Rd, ((rd << 4) & 0xf0) | ((rd >> 4) & 0x0f));

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_SWAP;
}

static int
avr_op_WDR (AvrCore *core, uint16_t opcode, unsigned int arg1,
            unsigned int arg2)
{
    /* 
     * Watchdog Reset.
     * 
     * This is device specific and must be overridden by sub-class.
     *
     * Opcode     : 1001 0101 1010 1000 
     * Usage      : WDR  
     * Operation  : (see specific hardware specification for WDR)
     * Flags      : None
     * Num Clocks : 1
     */
    WDTCR *wdtcr = (WDTCR *)avr_core_get_vdev_by_name (core, "WDTCR");

    if (wdtcr == NULL)
        avr_error ("Core device doesn't have WDTCR attached");

    wdtcr_update (wdtcr);

    avr_core_PC_incr (core, 1);
    avr_core_inst_CKS_set (core, 1);

    return opcode_WDR;
}

int
avr_op_UNKNOWN (AvrCore *core, uint16_t opcode, unsigned int arg1,
                unsigned int arg2)
{
    /*
     * An unknown opcode was seen. Treat it as a NOP, but return the UNKNOWN
     * so that the main loop can issue a warning.
     */
    avr_op_NOP (core, opcode, arg1, arg2);
    return opcode_UNKNOWN;
}

/******************************************************************************\
 *
 * Decode an opcode into the opcode handler function.
 *
 * Generates a warning and returns NULL if opcode is invalid.
 *
 * Returns a pointer to the function to handle the opcode.
 *
\******************************************************************************/

static void
lookup_opcode (uint16_t opcode, struct opcode_info *opi)
{
    uint16_t decode;

    opi->arg1 = -1;
    opi->arg2 = -1;
    switch (opcode)
    {
            /* opcodes with no operands */
        case 0x9598:
            opi->func = avr_op_BREAK;
            return;             /* 1001 0101 1001 1000 | BREAK */
        case 0x9519:
            opi->func = avr_op_EICALL;
            return;             /* 1001 0101 0001 1001 | EICALL */
        case 0x9419:
            opi->func = avr_op_EIJMP;
            return;             /* 1001 0100 0001 1001 | EIJMP */
        case 0x95D8:
            opi->func = avr_op_ELPM;
            return;             /* 1001 0101 1101 1000 | ELPM */
        case 0x95F8:
            opi->func = avr_op_ESPM;
            return;             /* 1001 0101 1111 1000 | ESPM */
        case 0x9509:
            opi->func = avr_op_ICALL;
            return;             /* 1001 0101 0000 1001 | ICALL */
        case 0x9409:
            opi->func = avr_op_IJMP;
            return;             /* 1001 0100 0000 1001 | IJMP */
        case 0x95C8:
            opi->func = avr_op_LPM;
            return;             /* 1001 0101 1100 1000 | LPM */
        case 0x0000:
            opi->func = avr_op_NOP;
            return;             /* 0000 0000 0000 0000 | NOP */
        case 0x9508:
            opi->func = avr_op_RET;
            return;             /* 1001 0101 0000 1000 | RET */
        case 0x9518:
            opi->func = avr_op_RETI;
            return;             /* 1001 0101 0001 1000 | RETI */
        case 0x9588:
            opi->func = avr_op_SLEEP;
            return;             /* 1001 0101 1000 1000 | SLEEP */
        case 0x95E8:
            opi->func = avr_op_SPM;
            return;             /* 1001 0101 1110 1000 | SPM */
        case 0x95A8:
            opi->func = avr_op_WDR;
            return;             /* 1001 0101 1010 1000 | WDR */

        default:
            {
                /* opcodes with two 5-bit register (Rd and Rr) operands */
                decode = opcode & ~(mask_Rd_5 | mask_Rr_5);
                opi->arg1 = get_rd_5 (opcode);
                opi->arg2 = get_rr_5 (opcode);
                switch (decode)
                {
                    case 0x1C00:
                        opi->func = avr_op_ADC;
                        return; /* 0001 11rd dddd rrrr | ADC or ROL */
                    case 0x0C00:
                        opi->func = avr_op_ADD;
                        return; /* 0000 11rd dddd rrrr | ADD or LSL */
                    case 0x2000:
                        opi->func = avr_op_AND;
                        return; /* 0010 00rd dddd rrrr | AND or TST */
                    case 0x1400:
                        opi->func = avr_op_CP;
                        return; /* 0001 01rd dddd rrrr | CP */
                    case 0x0400:
                        opi->func = avr_op_CPC;
                        return; /* 0000 01rd dddd rrrr | CPC */
                    case 0x1000:
                        opi->func = avr_op_CPSE;
                        return; /* 0001 00rd dddd rrrr | CPSE */
                    case 0x2400:
                        opi->func = avr_op_EOR;
                        return; /* 0010 01rd dddd rrrr | EOR or CLR */
                    case 0x2C00:
                        opi->func = avr_op_MOV;
                        return; /* 0010 11rd dddd rrrr | MOV */
                    case 0x9C00:
                        opi->func = avr_op_MUL;
                        return; /* 1001 11rd dddd rrrr | MUL */
                    case 0x2800:
                        opi->func = avr_op_OR;
                        return; /* 0010 10rd dddd rrrr | OR */
                    case 0x0800:
                        opi->func = avr_op_SBC;
                        return; /* 0000 10rd dddd rrrr | SBC */
                    case 0x1800:
                        opi->func = avr_op_SUB;
                        return; /* 0001 10rd dddd rrrr | SUB */
                }

                /* opcode with a single register (Rd) as operand */
                decode = opcode & ~(mask_Rd_5);
                opi->arg1 = get_rd_5 (opcode);
                opi->arg2 = -1;
                switch (decode)
                {
                    case 0x9405:
                        opi->func = avr_op_ASR;
                        return; /* 1001 010d dddd 0101 | ASR */
                    case 0x9400:
                        opi->func = avr_op_COM;
                        return; /* 1001 010d dddd 0000 | COM */
                    case 0x940A:
                        opi->func = avr_op_DEC;
                        return; /* 1001 010d dddd 1010 | DEC */
                    case 0x9006:
                        opi->func = avr_op_ELPM_Z;
                        return; /* 1001 000d dddd 0110 | ELPM */
                    case 0x9007:
                        opi->func = avr_op_ELPM_Z_incr;
                        return; /* 1001 000d dddd 0111 | ELPM */
                    case 0x9403:
                        opi->func = avr_op_INC;
                        return; /* 1001 010d dddd 0011 | INC */
                    case 0x9000:
                        opi->func = avr_op_LDS;
                        return; /* 1001 000d dddd 0000 | LDS */
                    case 0x900C:
                        opi->func = avr_op_LD_X;
                        return; /* 1001 000d dddd 1100 | LD */
                    case 0x900E:
                        opi->func = avr_op_LD_X_decr;
                        return; /* 1001 000d dddd 1110 | LD */
                    case 0x900D:
                        opi->func = avr_op_LD_X_incr;
                        return; /* 1001 000d dddd 1101 | LD */
                    case 0x900A:
                        opi->func = avr_op_LD_Y_decr;
                        return; /* 1001 000d dddd 1010 | LD */
                    case 0x9009:
                        opi->func = avr_op_LD_Y_incr;
                        return; /* 1001 000d dddd 1001 | LD */
                    case 0x9002:
                        opi->func = avr_op_LD_Z_decr;
                        return; /* 1001 000d dddd 0010 | LD */
                    case 0x9001:
                        opi->func = avr_op_LD_Z_incr;
                        return; /* 1001 000d dddd 0001 | LD */
                    case 0x9004:
                        opi->func = avr_op_LPM_Z;
                        return; /* 1001 000d dddd 0100 | LPM */
                    case 0x9005:
                        opi->func = avr_op_LPM_Z_incr;
                        return; /* 1001 000d dddd 0101 | LPM */
                    case 0x9406:
                        opi->func = avr_op_LSR;
                        return; /* 1001 010d dddd 0110 | LSR */
                    case 0x9401:
                        opi->func = avr_op_NEG;
                        return; /* 1001 010d dddd 0001 | NEG */
                    case 0x900F:
                        opi->func = avr_op_POP;
                        return; /* 1001 000d dddd 1111 | POP */
                    case 0x920F:
                        opi->func = avr_op_PUSH;
                        return; /* 1001 001d dddd 1111 | PUSH */
                    case 0x9407:
                        opi->func = avr_op_ROR;
                        return; /* 1001 010d dddd 0111 | ROR */
                    case 0x9200:
                        opi->func = avr_op_STS;
                        return; /* 1001 001d dddd 0000 | STS */
                    case 0x920C:
                        opi->func = avr_op_ST_X;
                        return; /* 1001 001d dddd 1100 | ST */
                    case 0x920E:
                        opi->func = avr_op_ST_X_decr;
                        return; /* 1001 001d dddd 1110 | ST */
                    case 0x920D:
                        opi->func = avr_op_ST_X_incr;
                        return; /* 1001 001d dddd 1101 | ST */
                    case 0x920A:
                        opi->func = avr_op_ST_Y_decr;
                        return; /* 1001 001d dddd 1010 | ST */
                    case 0x9209:
                        opi->func = avr_op_ST_Y_incr;
                        return; /* 1001 001d dddd 1001 | ST */
                    case 0x9202:
                        opi->func = avr_op_ST_Z_decr;
                        return; /* 1001 001d dddd 0010 | ST */
                    case 0x9201:
                        opi->func = avr_op_ST_Z_incr;
                        return; /* 1001 001d dddd 0001 | ST */
                    case 0x9402:
                        opi->func = avr_op_SWAP;
                        return; /* 1001 010d dddd 0010 | SWAP */
                }

                /* opcodes with a register (Rd) and a constant data (K) as
                   operands */
                decode = opcode & ~(mask_Rd_4 | mask_K_8);
                opi->arg1 = get_rd_4 (opcode);
                opi->arg2 = get_K_8 (opcode);
                switch (decode)
                {
                    case 0x7000:
                        opi->func = avr_op_ANDI;
                        return; /* 0111 KKKK dddd KKKK | CBR or ANDI */
                    case 0x3000:
                        opi->func = avr_op_CPI;
                        return; /* 0011 KKKK dddd KKKK | CPI */
                    case 0xE000:
                        opi->func = avr_op_LDI;
                        return; /* 1110 KKKK dddd KKKK | LDI or SER */
                    case 0x6000:
                        opi->func = avr_op_ORI;
                        return; /* 0110 KKKK dddd KKKK | SBR or ORI */
                    case 0x4000:
                        opi->func = avr_op_SBCI;
                        return; /* 0100 KKKK dddd KKKK | SBCI */
                    case 0x5000:
                        opi->func = avr_op_SUBI;
                        return; /* 0101 KKKK dddd KKKK | SUBI */
                }

                /* opcodes with a register (Rd) and a register bit number (b)
                   as operands */
                decode = opcode & ~(mask_Rd_5 | mask_reg_bit);
                opi->arg1 = get_rd_5 (opcode);
                opi->arg2 = get_reg_bit (opcode);
                switch (decode)
                {
                    case 0xF800:
                        opi->func = avr_op_BLD;
                        return; /* 1111 100d dddd 0bbb | BLD */
                    case 0xFA00:
                        opi->func = avr_op_BST;
                        return; /* 1111 101d dddd 0bbb | BST */
                    case 0xFC00:
                        opi->func = avr_op_SBRC;
                        return; /* 1111 110d dddd 0bbb | SBRC */
                    case 0xFE00:
                        opi->func = avr_op_SBRS;
                        return; /* 1111 111d dddd 0bbb | SBRS */
                }

                /* opcodes with a relative 7-bit address (k) and a register
                   bit number (b) as operands */
                decode = opcode & ~(mask_k_7 | mask_reg_bit);
                opi->arg1 = get_reg_bit (opcode);
                opi->arg2 = n_bit_unsigned_to_signed (get_k_7 (opcode), 7);
                switch (decode)
                {
                    case 0xF400:
                        opi->func = avr_op_BRBC;
                        return; /* 1111 01kk kkkk kbbb | BRBC */
                    case 0xF000:
                        opi->func = avr_op_BRBS;
                        return; /* 1111 00kk kkkk kbbb | BRBS */
                }

                /* opcodes with a 6-bit address displacement (q) and a
                   register (Rd) as operands */
                decode = opcode & ~(mask_Rd_5 | mask_q_displ);
                opi->arg1 = get_rd_5 (opcode);
                opi->arg2 = get_q (opcode);
                switch (decode)
                {
                    case 0x8008:
                        opi->func = avr_op_LDD_Y;
                        return; /* 10q0 qq0d dddd 1qqq | LDD */
                    case 0x8000:
                        opi->func = avr_op_LDD_Z;
                        return; /* 10q0 qq0d dddd 0qqq | LDD */
                    case 0x8208:
                        opi->func = avr_op_STD_Y;
                        return; /* 10q0 qq1d dddd 1qqq | STD */
                    case 0x8200:
                        opi->func = avr_op_STD_Z;
                        return; /* 10q0 qq1d dddd 0qqq | STD */
                }

                /* opcodes with a absolute 22-bit address (k) operand */
                decode = opcode & ~(mask_k_22);
                opi->arg1 = get_k_22 (opcode);
                opi->arg2 = -1;
                switch (decode)
                {
                    case 0x940E:
                        opi->func = avr_op_CALL;
                        return; /* 1001 010k kkkk 111k | CALL */
                    case 0x940C:
                        opi->func = avr_op_JMP;
                        return; /* 1001 010k kkkk 110k | JMP */
                }

                /* opcode with a sreg bit select (s) operand */
                decode = opcode & ~(mask_sreg_bit);
                opi->arg1 = get_sreg_bit (opcode);
                opi->arg2 = -1;
                switch (decode)
                {
                        /* BCLR takes place of CL{C,Z,N,V,S,H,T,I} */
                        /* BSET takes place of SE{C,Z,N,V,S,H,T,I} */
                    case 0x9488:
                        opi->func = avr_op_BCLR;
                        return; /* 1001 0100 1sss 1000 | BCLR */
                    case 0x9408:
                        opi->func = avr_op_BSET;
                        return; /* 1001 0100 0sss 1000 | BSET */
                }

                /* opcodes with a 6-bit constant (K) and a register (Rd) as
                   operands */
                decode = opcode & ~(mask_K_6 | mask_Rd_2);
                opi->arg1 = get_rd_2 (opcode);
                opi->arg2 = get_K_6 (opcode);
                switch (decode)
                {
                    case 0x9600:
                        opi->func = avr_op_ADIW;
                        return; /* 1001 0110 KKdd KKKK | ADIW */
                    case 0x9700:
                        opi->func = avr_op_SBIW;
                        return; /* 1001 0111 KKdd KKKK | SBIW */
                }

                /* opcodes with a 5-bit IO Addr (A) and register bit number
                   (b) as operands */
                decode = opcode & ~(mask_A_5 | mask_reg_bit);
                opi->arg1 = get_A_5 (opcode);
                opi->arg2 = get_reg_bit (opcode);
                switch (decode)
                {
                    case 0x9800:
                        opi->func = avr_op_CBI;
                        return; /* 1001 1000 AAAA Abbb | CBI */
                    case 0x9A00:
                        opi->func = avr_op_SBI;
                        return; /* 1001 1010 AAAA Abbb | SBI */
                    case 0x9900:
                        opi->func = avr_op_SBIC;
                        return; /* 1001 1001 AAAA Abbb | SBIC */
                    case 0x9B00:
                        opi->func = avr_op_SBIS;
                        return; /* 1001 1011 AAAA Abbb | SBIS */
                }

                /* opcodes with a 6-bit IO Addr (A) and register (Rd) as
                   operands */
                decode = opcode & ~(mask_A_6 | mask_Rd_5);
                opi->arg1 = get_rd_5 (opcode);
                opi->arg2 = get_A_6 (opcode);
                switch (decode)
                {
                    case 0xB000:
                        opi->func = avr_op_IN;
                        return; /* 1011 0AAd dddd AAAA | IN */
                    case 0xB800:
                        opi->func = avr_op_OUT;
                        return; /* 1011 1AAd dddd AAAA | OUT */
                }

                /* opcodes with a relative 12-bit address (k) operand */
                decode = opcode & ~(mask_k_12);
                opi->arg1 = n_bit_unsigned_to_signed (get_k_12 (opcode), 12);
                opi->arg2 = -1;
                switch (decode)
                {
                    case 0xD000:
                        opi->func = avr_op_RCALL;
                        return; /* 1101 kkkk kkkk kkkk | RCALL */
                    case 0xC000:
                        opi->func = avr_op_RJMP;
                        return; /* 1100 kkkk kkkk kkkk | RJMP */
                }

                /* opcodes with two 4-bit register (Rd and Rr) operands */
                decode = opcode & ~(mask_Rd_4 | mask_Rr_4);
                opi->arg1 = get_rd_4 (opcode);
                opi->arg2 = get_rr_4 (opcode);
                switch (decode)
                {
                    case 0x0100:
                        opi->func = avr_op_MOVW;
                        return; /* 0000 0001 dddd rrrr | MOVW */
                    case 0x0200:
                        opi->func = avr_op_MULS;
                        return; /* 0000 0010 dddd rrrr | MULS */
                }

                /* opcodes with two 3-bit register (Rd and Rr) operands */
                decode = opcode & ~(mask_Rd_3 | mask_Rr_3);
                opi->arg1 = get_rd_3 (opcode);
                opi->arg2 = get_rr_3 (opcode);
                switch (decode)
                {
                    case 0x0300:
                        opi->func = avr_op_MULSU;
                        return; /* 0000 0011 0ddd 0rrr | MULSU */
                    case 0x0308:
                        opi->func = avr_op_FMUL;
                        return; /* 0000 0011 0ddd 1rrr | FMUL */
                    case 0x0380:
                        opi->func = avr_op_FMULS;
                        return; /* 0000 0011 1ddd 0rrr | FMULS */
                    case 0x0388:
                        opi->func = avr_op_FMULSU;
                        return; /* 0000 0011 1ddd 1rrr | FMULSU */
                }

            }                   /* default */
    }                           /* first switch */

    opi->func = avr_op_UNKNOWN;
    opi->arg1 = -1;
    opi->arg2 = -1;

}                               /* decode opcode function */

/**
 * \brief Initialize the decoder lookup table.
 *
 * This is automatically called by avr_core_construct().
 *
 * It is safe to call this function many times, since if will only create the
 * table the first time it is called.
 */

void
decode_init_lookup_table (void)
{
    if (global_opcode_lookup_table == NULL)
    {
        int num_ops = 0x10000;
        int i;
        avr_message ("generating opcode lookup_table\n");
        global_opcode_lookup_table = avr_new0 (struct opcode_info, num_ops);
        for (i = 0; i < num_ops; i++)
        {
            lookup_opcode (i, global_opcode_lookup_table + i);
        }
    }
}

/**
 * \brief Decode an opcode into the opcode handler function.
 *
 * Generates a warning and returns NULL if opcode is invalid.
 *
 * Returns a pointer to the function to handle the opcode.
 */

extern inline struct opcode_info *decode_opcode (uint16_t opcode);
