/*
 * $Id: avrcore.h,v 1.15 2004/01/30 07:09:56 troth Exp $
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

#ifndef SIM_AVRCORE_H
#define SIM_AVRCORE_H

#include "intvects.h"

#include "display.h"
#include "spm_helper.h"
/****************************************************************************\
 *
 * AvrCore(AvrClass) Definition
 *
\****************************************************************************/

extern int global_debug_inst_output;

typedef enum
{
    STATE_RUNNING,              /* normal running */
    STATE_STOPPED,              /* stopped, not running */
    STATE_BREAK_PT,             /* at a break point */
    STATE_SLEEP,                /* Sleep mode (there are many sleep modes. */
} StateType;

typedef struct _AvrCore AvrCore;

struct _AvrCore
{
    AvrClass parent;
    int state;                  /* What state is the device in */
    int sleep_mode;             /* If in sleep state, what mode? */
    int32_t PC;                 /* Program Counter */
    int32_t PC_size;            /* size of Program Counter in bytes */
    int32_t PC_max;             /* maximum value PC can hold for a given
                                   device (is flash_sz/2) */
    SREG *sreg;                 /* Status Register */
    GPWR *gpwr;                 /* General Purpose Working Registers */
    Flash *flash;               /* flash program memory */
    EEProm *eeprom;             /* internal eeprom memory */

    SRAM *sram;                 /* internal sram memory */
    Memory *mem;                /* memory space (gp reg, io reg, sram, ext
                                   sram, etc) */
    Stack *stack;               /* a stack implementaton */

    SPMhelper *spmhelper;       /* SPM instruction helper */

    DList *breakpoints;         /* head of list of active breakpoints */

    DList *irq_pending;         /* head of list of pending interrupts (sorted
                                   by priority) */
    IntVect *irq_vtable;        /* interrupt vector table array */
    int irq_offset;             /* Some devices (e.g. mega128) will let you
                                   add an offset to the vector addr. */

    uint64_t CK;                /* for keeping track of how many clock cycles
                                   have occurred */
    int inst_CKS;               /* number of clocks the previously executed
                                   instruction took */

    DList *clk_cb;              /* head of list of clock callback items. If a
                                   clock callback function uses the time
                                   argument, it is to be interpreted as the
                                   number of clock cycles that have occured
                                   since a reset. */

    DList *async_cb;            /* head of list of asynchronous callback
                                   items. If an async callback function uses
                                   the time argument, it is to be interpreted
                                   as the approximate number of milliseconds
                                   that have elapsed from some unknown time on
                                   the host system. */

    /*
     * These registers will go somewhere else once I see a device that
     * actually uses them.
     */
    uint8_t RAMPX;              /* Registers concatenated with the X, Y and Z
                                   registers */
    uint8_t RAMPY;              /* enabling indirect addressing of the wholw
                                   data space */
    RAMPZ *rampz;               /* on MCUs with more than 64K bytes date
                                   space, and constant */
    /* data fetch on MCUs with more than 64K bytes profram space. */

    uint8_t RAMPD;              /* Register concatenated with the Z register
                                   enabling direct addressing of the while
                                   data space on MCUs with more than 64K bytes
                                   data space */

    uint8_t EIND;               /* Register concatenated with the instruction
                                   word enabling indirect jump and call to the
                                   whole program space on MCUs with more than
                                   64K bytes program space. */
};

extern AvrCore *avr_core_new (char *dev_name);
extern void avr_core_destroy (void *core);

extern void avr_core_get_sizes (AvrCore *core, int *flash, int *sram,
                                int *sram_start, int *eeprom);

/* Attach a Virtual Device to the core */

extern inline void
avr_core_attach_vdev (AvrCore *core, uint16_t addr, char *name, VDevice *vdev,
                      int flags, uint8_t reset_value, uint8_t rd_mask,
                      uint8_t wr_mask)
{
    vdev_set_core (vdev, (AvrClass *)core);
    mem_attach (core->mem, addr, name, vdev, flags, reset_value, rd_mask,
                wr_mask);
}

extern inline VDevice *
avr_core_get_vdev_by_name (AvrCore *core, char *name)
{
    return mem_get_vdevice_by_name (core->mem, name);
}

extern inline VDevice *
avr_core_get_vdev_by_addr (AvrCore *core, int addr)
{
    return mem_get_vdevice_by_addr (core->mem, addr);
}

extern inline void
avr_core_set_addr_name (AvrCore *core, int addr, char *name)
{
    mem_set_addr_name (core->mem, addr, name);
}


/* State Access Methods */

extern inline int
avr_core_get_state (AvrCore *core)
{
    return core->state;
}

extern inline void
avr_core_set_state (AvrCore *core, StateType state)
{
    core->state = state;
}

/* Sleep Mode Access Methods */

extern inline void
avr_core_set_sleep_mode (AvrCore *core, int sleep_mode)
{
    core->state = STATE_SLEEP;
    core->sleep_mode = ((unsigned int)1 << sleep_mode);
}

extern inline int
avr_core_get_sleep_mode (AvrCore *core)
{
    return core->sleep_mode;
}

/* Program Memory Space Access Methods */

static inline uint16_t
avr_core_flash_read (AvrCore *core, int addr)
{
    return flash_read (core->flash, addr);
}

static inline void
avr_core_flash_write (AvrCore *core, int addr, uint16_t val)
{
    flash_write (core->flash, addr, val);
}

static inline void
avr_core_flash_write_lo8 (AvrCore *core, int addr, uint8_t val)
{
    flash_write_lo8 (core->flash, addr, val);
}

static inline void
avr_core_flash_write_hi8 (AvrCore *core, int addr, uint8_t val)
{
    flash_write_hi8 (core->flash, addr, val);
}

/* Data Memory Space Access Methods */

static inline uint8_t
avr_core_mem_read (AvrCore *core, int addr)
{
    return mem_read (core->mem, addr);
}

static inline void
avr_core_mem_write (AvrCore *core, int addr, uint8_t val)
{
    mem_write (core->mem, addr, val);
}

/* Status Register Access Methods */

static inline uint8_t
avr_core_sreg_get (AvrCore *core)
{
    return sreg_get (core->sreg);
}

static inline void
avr_core_sreg_set (AvrCore *core, uint8_t v)
{
    sreg_set (core->sreg, v);
}

extern inline int
avr_core_sreg_get_bit (AvrCore *core, int b)
{
    return sreg_get_bit (core->sreg, b);
}

extern inline void
avr_core_sreg_set_bit (AvrCore *core, int b, int v)
{
    sreg_set_bit (core->sreg, b, v);
}

/* RAMPZ Access Methods */

extern inline uint8_t
avr_core_rampz_get (AvrCore *core)
{
    if (core->rampz)
        return rampz_get (core->rampz);
    else
        return 0;
}

extern inline void
avr_core_rampz_set (AvrCore *core, uint8_t v)
{
    if (core->rampz)
        rampz_set (core->rampz, v);
}

/* General Purpose Working Register Access Methods */

static inline uint8_t
avr_core_gpwr_get (AvrCore *core, int reg)
{
    return gpwr_get (core->gpwr, reg);
}

static inline void
avr_core_gpwr_set (AvrCore *core, int reg, uint8_t val)
{
    gpwr_set (core->gpwr, reg, val);
}

/* Direct I/O Register Access Methods */

extern void avr_core_io_display_names (AvrCore *core);

static inline uint8_t
avr_core_io_read (AvrCore *core, int reg)
{
    return avr_core_mem_read (core, reg + IO_REG_ADDR_BEGIN);
}

static inline void
avr_core_io_write (AvrCore *core, int reg, uint8_t val)
{
    avr_core_mem_write (core, reg + IO_REG_ADDR_BEGIN, val);
}

static inline void
avr_core_io_fetch (AvrCore *core, int reg, uint8_t * val, char *buf,
                   int bufsiz)
{
    mem_io_fetch (core->mem, reg + IO_REG_ADDR_BEGIN, val, buf, bufsiz);
}

/* Stack Access Methods */

extern inline uint32_t
avr_core_stack_pop (AvrCore *core, int bytes)
{
    return stack_pop (core->stack, bytes);
}

extern inline void
avr_core_stack_push (AvrCore *core, int bytes, uint32_t val)
{
    stack_push (core->stack, bytes, val);
}
/* spm emulation */
extern inline void
avr_core_spm(AvrCore *core, int reg0, int reg1, int Z)
{
    spm_run (core->spmhelper, reg0, reg1, Z);
}

/* Private
 
   Deal with PC reach-arounds.

   It's possible to set/incrment the program counter with large negative
   values which go past zero. These should be interpreted as wrapping back
   around the last address in the flash. */

extern inline void
_adjust_PC_to_max (AvrCore *core)
{
    if (core->PC < 0)
        core->PC = core->PC_max + core->PC;

    if (core->PC >= core->PC_max)
        core->PC -= core->PC_max;
}

/* Program Counter Access Methods */

extern inline int32_t
avr_core_PC_size (AvrCore *core)
{
    return core->PC_size;
}

static inline int32_t
avr_core_PC_max (AvrCore *core)
{
    return core->PC_max;
}

static inline int32_t
avr_core_PC_get (AvrCore *core)
{
    return core->PC;
}

static inline void
avr_core_PC_set (AvrCore *core, int32_t val)
{
    core->PC = val;
    _adjust_PC_to_max (core);
    display_pc (core->PC);
}

extern inline void
avr_core_PC_incr (AvrCore *core, int val)
{
    core->PC += val;
    _adjust_PC_to_max (core);
    display_pc (core->PC);
}

/* Interrupt Access Methods */

extern void avr_core_irq_raise (AvrCore *core, int irq);
extern void avr_core_irq_clear (AvrCore *core, IntVect *irq);

extern inline void
avr_core_irq_clear_all (AvrCore *core)
{
    dlist_delete_all (core->irq_pending);
    core->irq_pending = NULL;
}

extern IntVect *avr_core_irq_get_pending (AvrCore *core);

/* Loading files into various memory areas */
extern int avr_core_load_program (AvrCore *core, char *file, int format);
extern int avr_core_load_eeprom (AvrCore *core, char *file, int format);

/* Dump a core file */
extern void avr_core_dump_core (AvrCore *core, FILE * f_core);

/* Methods for running programs */
extern int avr_core_step (AvrCore *core);
extern void avr_core_run (AvrCore *core);
extern void avr_core_reset (AvrCore *core);

/* Methods for accessing CK and inst_CKS */

extern inline uint64_t
avr_core_CK_get (AvrCore *core)
{
    return core->CK;
}

extern inline void
avr_core_CK_incr (AvrCore *core)
{
    core->CK++;

    /* Send clock cycles to display */
    display_clock (core->CK);
}

extern inline int
avr_core_inst_CKS_get (AvrCore *core)
{
    return core->inst_CKS;
}

extern inline void
avr_core_inst_CKS_set (AvrCore *core, int val)
{
    core->inst_CKS = val;
}

/* Methods for handling clock callbacks */

extern inline void
avr_core_clk_cb_add (AvrCore *core, CallBack *cb)
{
    core->clk_cb = callback_list_add (core->clk_cb, cb);
}

extern inline void
avr_core_clk_cb_exec (AvrCore *core)
{
    core->clk_cb =
        callback_list_execute_all (core->clk_cb, avr_core_CK_get (core));
}

/* Methods for handling asynchronous callbacks */

extern inline void
avr_core_async_cb_add (AvrCore *core, CallBack *cb)
{
    core->async_cb = callback_list_add (core->async_cb, cb);
}

extern inline void
avr_core_async_cb_exec (AvrCore *core)
{
    core->async_cb =
        callback_list_execute_all (core->async_cb, get_program_time ());
}

/* For adding external read and write callback functions */
extern void avr_core_add_ext_rd_wr (AvrCore *core, int addr,
                                    PortFP_ExtRd ext_rd, PortFP_ExtWr ext_wr);

/* Break point access methods */
extern void avr_core_insert_breakpoint (AvrCore *core, int pc);
extern void avr_core_remove_breakpoint (AvrCore *core, int pc);
extern void avr_core_enable_breakpoints (AvrCore *core);
extern void avr_core_disable_breakpoints (AvrCore *core);

#endif /* SIM_AVRCORE_H */
