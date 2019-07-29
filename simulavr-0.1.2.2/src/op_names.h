/*
 * $Id: op_names.h,v 1.6 2003/12/01 07:35:53 troth Exp $
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

#ifndef SIM_OP_NAMES_H
#define SIM_OP_NAMES_H

enum _opcode_name
{
    /* opcodes with no operands */
    opcode_BREAK,               /* 0x9598 - 1001 0101 1001 1000 | BREAK */
    opcode_EICALL,              /* 0x9519 - 1001 0101 0001 1001 | EICALL */
    opcode_EIJMP,               /* 0x9419 - 1001 0100 0001 1001 | EIJMP */
    opcode_ELPM,                /* 0x95D8 - 1001 0101 1101 1000 | ELPM */
    opcode_ESPM,                /* 0x95F8 - 1001 0101 1111 1000 | ESPM */
    opcode_ICALL,               /* 0x9509 - 1001 0101 0000 1001 | ICALL */
    opcode_IJMP,                /* 0x9409 - 1001 0100 0000 1001 | IJMP */
    opcode_LPM,                 /* 0x95C8 - 1001 0101 1100 1000 | LPM */
    opcode_NOP,                 /* 0x0000 - 0000 0000 0000 0000 | NOP */
    opcode_RET,                 /* 0x9508 - 1001 0101 0000 1000 | RET */
    opcode_RETI,                /* 0x9518 - 1001 0101 0001 1000 | RETI */
    opcode_SLEEP,               /* 0x9588 - 1001 0101 1000 1000 | SLEEP */
    opcode_SPM,                 /* 0x95E8 - 1001 0101 1110 1000 | SPM */
    opcode_WDR,                 /* 0x95A8 - 1001 0101 1010 1000 | WDR */

    /* opcode with a single register (Rd) as operand */
    opcode_ASR,                 /* 0x9405 - 1001 010d dddd 0101 | ASR */
    opcode_COM,                 /* 0x9400 - 1001 010d dddd 0000 | COM */
    opcode_DEC,                 /* 0x940A - 1001 010d dddd 1010 | DEC */
    opcode_ELPM_Z,              /* 0x9006 - 1001 000d dddd 0110 | ELPM */
    opcode_ELPM_Z_incr,         /* 0x9007 - 1001 000d dddd 0111 | ELPM */
    opcode_INC,                 /* 0x9403 - 1001 010d dddd 0011 | INC */
    opcode_LDS,                 /* 0x9000 - 1001 000d dddd 0000 | LDS */
    opcode_LD_X,                /* 0x900C - 1001 000d dddd 1100 | LD */
    opcode_LD_X_decr,           /* 0x900E - 1001 000d dddd 1110 | LD */
    opcode_LD_X_incr,           /* 0x900D - 1001 000d dddd 1101 | LD */
    opcode_LD_Y_decr,           /* 0x900A - 1001 000d dddd 1010 | LD */
    opcode_LD_Y_incr,           /* 0x9009 - 1001 000d dddd 1001 | LD */
    opcode_LD_Z_decr,           /* 0x9002 - 1001 000d dddd 0010 | LD */
    opcode_LD_Z_incr,           /* 0x9001 - 1001 000d dddd 0001 | LD */
    opcode_LPM_Z,               /* 0x9004 - 1001 000d dddd 0100 | LPM */
    opcode_LPM_Z_incr,          /* 0x9005 - 1001 000d dddd 0101 | LPM */
    opcode_LSR,                 /* 0x9406 - 1001 010d dddd 0110 | LSR */
    opcode_NEG,                 /* 0x9401 - 1001 010d dddd 0001 | NEG */
    opcode_POP,                 /* 0x900F - 1001 000d dddd 1111 | POP */
    opcode_PUSH,                /* 0x920F - 1001 001d dddd 1111 | PUSH */
    opcode_ROR,                 /* 0x9407 - 1001 010d dddd 0111 | ROR */
    opcode_STS,                 /* 0x9200 - 1001 001d dddd 0000 | STS */
    opcode_ST_X,                /* 0x920C - 1001 001d dddd 1100 | ST */
    opcode_ST_X_decr,           /* 0x920E - 1001 001d dddd 1110 | ST */
    opcode_ST_X_incr,           /* 0x920D - 1001 001d dddd 1101 | ST */
    opcode_ST_Y_decr,           /* 0x920A - 1001 001d dddd 1010 | ST */
    opcode_ST_Y_incr,           /* 0x9209 - 1001 001d dddd 1001 | ST */
    opcode_ST_Z_decr,           /* 0x9202 - 1001 001d dddd 0010 | ST */
    opcode_ST_Z_incr,           /* 0x9201 - 1001 001d dddd 0001 | ST */
    opcode_SWAP,                /* 0x9402 - 1001 010d dddd 0010 | SWAP */

    /* opcodes with two 5-bit register (Rd and Rr) operands */
    opcode_ADC,                 /* 0x1C00 - 0001 11rd dddd rrrr | ADC or ROL */
    opcode_ADD,                 /* 0x0C00 - 0000 11rd dddd rrrr | ADD or LSL */
    opcode_AND,                 /* 0x2000 - 0010 00rd dddd rrrr | AND or TST
                                   or LSL */
    opcode_CP,                  /* 0x1400 - 0001 01rd dddd rrrr | CP */
    opcode_CPC,                 /* 0x0400 - 0000 01rd dddd rrrr | CPC */
    opcode_CPSE,                /* 0x1000 - 0001 00rd dddd rrrr | CPSE */
    opcode_EOR,                 /* 0x2400 - 0010 01rd dddd rrrr | EOR or CLR */
    opcode_MOV,                 /* 0x2C00 - 0010 11rd dddd rrrr | MOV */
    opcode_MUL,                 /* 0x9C00 - 1001 11rd dddd rrrr | MUL */
    opcode_OR,                  /* 0x2800 - 0010 10rd dddd rrrr | OR */
    opcode_SBC,                 /* 0x0800 - 0000 10rd dddd rrrr | SBC */
    opcode_SUB,                 /* 0x1800 - 0001 10rd dddd rrrr | SUB */

    /* opcodes with two 4-bit register (Rd and Rr) operands */
    opcode_MOVW,                /* 0x0100 - 0000 0001 dddd rrrr | MOVW */
    opcode_MULS,                /* 0x0200 - 0000 0010 dddd rrrr | MULS */
    opcode_MULSU,               /* 0x0300 - 0000 0011 dddd rrrr | MULSU */

    /* opcodes with two 3-bit register (Rd and Rr) operands */
    opcode_FMUL,                /* 0x0308 - 0000 0011 0ddd 1rrr | FMUL */
    opcode_FMULS,               /* 0x0380 - 0000 0011 1ddd 0rrr | FMULS */
    opcode_FMULSU,              /* 0x0388 - 0000 0011 1ddd 1rrr | FMULSU */

    /* opcodes with a register (Rd) and a constant data (K) as operands */
    opcode_ANDI,                /* 0x7000 - 0111 KKKK dddd KKKK | CBR or
                                   ANDI */
    opcode_CPI,                 /* 0x3000 - 0011 KKKK dddd KKKK | CPI */
    opcode_LDI,                 /* 0xE000 - 1110 KKKK dddd KKKK | LDI */
    opcode_ORI,                 /* 0x6000 - 0110 KKKK dddd KKKK | SBR or ORI */
    opcode_SBCI,                /* 0x4000 - 0100 KKKK dddd KKKK | SBCI */
    opcode_SUBI,                /* 0x5000 - 0101 KKKK dddd KKKK | SUBI */

    /* opcodes with a register (Rd) and a register bit number (b) as
       operands */
    opcode_BLD,                 /* 0xF800 - 1111 100d dddd 0bbb | BLD */
    opcode_BST,                 /* 0xFA00 - 1111 101d dddd 0bbb | BST */
    opcode_SBRC,                /* 0xFC00 - 1111 110d dddd 0bbb | SBRC */
    opcode_SBRS,                /* 0xFE00 - 1111 111d dddd 0bbb | SBRS */

    /* opcodes with a relative 7-bit address (k) and a register bit number (b)
       as operands */
    opcode_BRBC,                /* 0xF400 - 1111 01kk kkkk kbbb | BRBC */
    opcode_BRBS,                /* 0xF000 - 1111 00kk kkkk kbbb | BRBS */

    /* opcodes with a 6-bit address displacement (q) and a register (Rd) as
       operands */
    opcode_LDD_Y,               /* 0x8008 - 10q0 qq0d dddd 1qqq | LDD */
    opcode_LDD_Z,               /* 0x8000 - 10q0 qq0d dddd 0qqq | LDD */
    opcode_STD_Y,               /* 0x8208 - 10q0 qq1d dddd 1qqq | STD */
    opcode_STD_Z,               /* 0x8200 - 10q0 qq1d dddd 0qqq | STD */

    /* opcodes with a absolute 22-bit address (k) operand */
    opcode_CALL,                /* 0x940E - 1001 010k kkkk 111k | CALL */
    opcode_JMP,                 /* 0x940C - 1001 010k kkkk 110k | JMP */

    /* opcode with a sreg bit select (s) operand */
    opcode_BCLR,                /* 0x9488 - 1001 0100 1sss 1000 | BCLR or
                                   CL{C,Z,N,V,S,H,T,I} */
    opcode_BSET,                /* 0x9408 - 1001 0100 0sss 1000 | BSET or
                                   SE{C,Z,N,V,S,H,T,I} */

    /* opcodes with a 6-bit constant (K) and a register (Rd) as operands */
    opcode_ADIW,                /* 0x9600 - 1001 0110 KKdd KKKK | ADIW */
    opcode_SBIW,                /* 0x9700 - 1001 0111 KKdd KKKK | SBIW */

    /* opcodes with a 5-bit IO Addr (A) and register bit number (b) as
       operands */
    opcode_CBI,                 /* 0x9800 - 1001 1000 AAAA Abbb | CBI */
    opcode_SBI,                 /* 0x9A00 - 1001 1010 AAAA Abbb | SBI */
    opcode_SBIC,                /* 0x9900 - 1001 1001 AAAA Abbb | SBIC */
    opcode_SBIS,                /* 0x9B00 - 1001 1011 AAAA Abbb | SBIS */

    /* opcodes with a 6-bit IO Addr (A) and register (Rd) as operands */
    opcode_IN,                  /* 0xB000 - 1011 0AAd dddd AAAA | IN */
    opcode_OUT,                 /* 0xB800 - 1011 1AAd dddd AAAA | OUT */

    /* opcodes with a relative 12-bit address (k) operand */
    opcode_RCALL,               /* 0xD000 - 1101 kkkk kkkk kkkk | RCALL */
    opcode_RJMP,                /* 0xC000 - 1100 kkkk kkkk kkkk | RJMP */

    opcode_UNKNOWN,

    NUM_OPCODE_HANLDERS,
};

#ifndef BREAK_POINT
#  define BREAK_POINT    -1
#endif

/* global array for mapping handler codes to name strings */
extern char *global_opcode_name[NUM_OPCODE_HANLDERS];

#endif /* SIM_OP_NAMES_H */
