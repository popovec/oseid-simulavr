/*
 * $Id: gdb.h,v 1.15 2003/12/02 03:56:45 troth Exp $
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

#ifndef SIM_GDB_H
#define SIM_GDB_H

#ifndef BREAK_POINT
#  define BREAK_POINT    -1
#endif

/* Prototypes for pointers to function. */

typedef uint8_t (*CommFuncReadReg) (void *user_data, int reg_num);
typedef void (*CommFuncWriteReg) (void *user_data, int reg_num, uint8_t val);

typedef uint8_t (*CommFuncReadSREG) (void *user_data);
typedef void (*CommFuncWriteSREG) (void *user_data, uint8_t val);

typedef int32_t (*CommFuncReadPC) (void *user_data);
typedef void (*CommFuncWritePC) (void *user_data, int32_t val);
typedef int32_t (*CommFuncMaxPC) (void *user_data);

typedef uint8_t (*CommFuncReadSRAM) (void *user_data, int addr);
typedef void (*CommFuncWriteSRAM) (void *user_data, int addr, uint8_t val);

typedef uint16_t (*CommFuncReadFlash) (void *user_data, int addr);
typedef void (*CommFuncWriteFlash) (void *user_data, int addr, uint16_t val);
typedef void (*CommFuncWriteFlashLo8) (void *user_data, int addr,
                                       uint8_t val);
typedef void (*CommFuncWriteFlashHi8) (void *user_data, int addr,
                                       uint8_t val);

typedef void (*CommFuncInsertBreak) (void *user_data, int addr);
typedef void (*CommFuncRemoveBreak) (void *user_data, int addr);
typedef void (*CommFuncDisableBrkpts) (void *user_data);
typedef void (*CommFuncEnableBrkpts) (void *user_data);

typedef int (*CommFuncStep) (void *user_data);

typedef void (*CommFuncReset) (void *user_data);

typedef void (*CommFuncIORegFetch) (void *user_data, int addr, uint8_t * val,
                                    char *reg_name, size_t reg_name_size);

typedef void (*CommFuncIrqRaise) (void *user_data, int irq);

/* This structure allows the target to supply handler functions to the gdb
   interact for performing various tasks. */

typedef struct GdbComm GdbComm_T;

/* *INDENT-OFF* */
struct GdbComm {
    void *user_data;            /* Pass anything you want here. Will be passed
                                   on to all CommFunc* functions you
                                   provide. */

    CommFuncReadReg        read_reg;
    CommFuncWriteReg       write_reg;

    CommFuncReadSREG       read_sreg;
    CommFuncWriteSREG      write_sreg;

    CommFuncReadPC         read_pc;
    CommFuncWritePC        write_pc;
    CommFuncMaxPC          max_pc;

    CommFuncReadSRAM       read_sram;
    CommFuncWriteSRAM      write_sram;

    CommFuncReadFlash      read_flash;
    CommFuncWriteFlash     write_flash;
    CommFuncWriteFlashLo8  write_flash_lo8;
    CommFuncWriteFlashHi8  write_flash_hi8;

    CommFuncInsertBreak    insert_break;
    CommFuncRemoveBreak    remove_break;
    CommFuncEnableBrkpts   enable_breakpts;
    CommFuncDisableBrkpts  disable_breakpts;

    CommFuncStep           step;
    CommFuncReset          reset;

    CommFuncIORegFetch     io_fetch;

    CommFuncIrqRaise       irq_raise;
};
/* *INDENT-ON* */

extern void gdb_interact (GdbComm_T *comm, int port, int debug_on);

#endif /* SIM_GDB_H */
