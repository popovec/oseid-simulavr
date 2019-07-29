/*
 * $Id: avrcore.c,v 1.81 2005/01/13 20:27:59 zfrdh Exp $
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

/**
 *  \file avrcore.c
 *  \brief Module for the core AvrCore object, which is the AVR CPU to be
 *  simulated.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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
#include "ports.h"

#include "avrcore.h"

#include "display.h"
#include "decoder.h"
#include "sig.h"
#include "devsupp.h"
#include "spm_helper.h"

/** \brief Flag for enabling output of instruction debug messages. */
int global_debug_inst_output = 0;

/***************************************************************************\
 *
 * BreakPt(AvrClass) Methods
 *
\***************************************************************************/

#ifndef DOXYGEN                 /* don't expose to doxygen */

typedef struct _BreakPt BreakPt;
struct _BreakPt
{
    AvrClass parent;
    int pc;
    uint16_t opcode;
};

#endif

static inline BreakPt *brk_pt_new (int pc, uint16_t opcode);
static inline void brk_pt_construct (BreakPt *bp, int pc, uint16_t opcode);
static inline void brk_pt_destroy (void *bp);

static inline void
brk_pt_construct (BreakPt *bp, int pc, uint16_t opcode)
{
    if (bp == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)bp);

    bp->pc = pc;
    bp->opcode = opcode;
}

static inline BreakPt *
brk_pt_new (int pc, uint16_t opcode)
{
    BreakPt *bp;

    bp = avr_new (BreakPt, 1);
    brk_pt_construct (bp, pc, opcode);
    class_overload_destroy ((AvrClass *)bp, brk_pt_destroy);

    return bp;
}

static inline void
brk_pt_destroy (void *bp)
{
    BreakPt *_bp = (BreakPt *)bp;

    if (_bp == NULL)
        return;

    class_destroy (bp);
}

static inline DList *brk_pt_list_add (DList *head, int pc, uint16_t opcode);
static inline DList *brk_pt_list_delete (DList *head, int pc);
static inline BreakPt *brk_pt_list_lookup (DList *head, int pc);
static int brk_pt_cmp (AvrClass *d1, AvrClass *d2);

/* Compare function for break points. */

static int
brk_pt_cmp (AvrClass *d1, AvrClass *d2)
{
    return ((BreakPt *)d1)->pc - ((BreakPt *)d2)->pc;
}

static inline DList *
brk_pt_list_add (DList *head, int pc, uint16_t opcode)
{
    BreakPt *bp = brk_pt_new (pc, opcode);

    return dlist_add (head, (AvrClass *)bp, brk_pt_cmp);
}

static inline DList *
brk_pt_list_delete (DList *head, int pc)
{
    BreakPt *bp = brk_pt_new (pc, 0);

    head = dlist_delete (head, (AvrClass *)bp, brk_pt_cmp);
    class_unref ((AvrClass *)bp);

    return head;
}

static inline BreakPt *
brk_pt_list_lookup (DList *head, int pc)
{
    BreakPt *found;
    BreakPt *bp = brk_pt_new (pc, 0);

    found = (BreakPt *)dlist_lookup (head, (AvrClass *)bp, brk_pt_cmp);
    class_unref ((AvrClass *)bp);

    return found;
}

static inline DList *
brk_pt_iterator (DList *head, DListFP_Iter func, void *user_data)
{
    return dlist_iterator (head, func, user_data);
}

/***************************************************************************\
 *
 * Irq(AvrClass) Methods: For managing the irq_pending list.
 *
\***************************************************************************/

#ifndef DOXYGEN                 /* don't expose to doxygen */

typedef struct _Irq Irq;
struct _Irq
{
    AvrClass parent;
    IntVect *vector;

    /* These are only used for storing lookup information. Copies of
       core->{state,sleep_mode}. */
    int state;
    unsigned int sleep_mode;
};

#endif

static inline Irq *irq_new (IntVect *vector, int state,
                            unsigned int sleep_mode);
static inline void irq_construct (Irq *irq, IntVect *vector, int state,
                                  unsigned int sleep_mode);
static inline void irq_destroy (void *irq);

static inline void
irq_construct (Irq *irq, IntVect *vector, int state, unsigned int sleep_mode)
{
    if (irq == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)irq);

    irq->vector = vector;
    irq->state = state;
    irq->sleep_mode = sleep_mode;
}

static inline Irq *
irq_new (IntVect *vector, int state, unsigned int sleep_mode)
{
    Irq *irq;

    irq = avr_new (Irq, 1);
    irq_construct (irq, vector, state, sleep_mode);
    class_overload_destroy ((AvrClass *)irq, irq_destroy);

    return irq;
}

static inline void
irq_destroy (void *irq)
{
    Irq *_irq = (Irq *)irq;

    if (_irq == NULL)
        return;

    class_destroy (irq);
}

static inline DList *irq_list_add (DList *head, IntVect *vector);
static inline DList *irq_list_delete (DList *head, IntVect *vector);
#if 0                           /* TRoth/2002-09-15: This isn't used. ??? */
static inline Irq *irq_list_lookup_addr (DList *head, IntVect *vector);
#endif
static int irq_cmp_addr (AvrClass *d1, AvrClass *d2);
static int irq_cmp_pending (AvrClass *d1, AvrClass *d2);

/* Compare function for break points. */

static int
irq_cmp_addr (AvrClass *d1, AvrClass *d2)
{
    return ((Irq *)d1)->vector->addr - ((Irq *)d2)->vector->addr;
}

static inline DList *
irq_list_add (DList *head, IntVect *vector)
{
    Irq *irq = irq_new (vector, 0, 0);

    return dlist_add (head, (AvrClass *)irq, irq_cmp_addr);
}

static inline DList *
irq_list_delete (DList *head, IntVect *vector)
{
    Irq *irq = irq_new (vector, 0, 0);

    head = dlist_delete (head, (AvrClass *)irq, irq_cmp_addr);
    class_unref ((AvrClass *)irq);

    return head;
}

#if 0                           /* TRoth/2002-09-15: This isn't used. ??? */
static inline Irq *
irq_list_lookup_addr (DList *head, IntVect *vector)
{
    Irq *found;
    Irq *irq = irq_new (vector, 0, 0);

    found = (Irq *)dlist_lookup (head, (AvrClass *)irq, irq_cmp_addr);
    class_unref ((AvrClass *)irq);

    return found;
}
#endif

static int
irq_cmp_pending (AvrClass *d1, AvrClass *d2)
{
    Irq *i1 = (Irq *)d1;        /* This is the irq which might be ready to be
                                   vectored into. */
    int state = ((Irq *)d2)->state; /* The state the device is currently
                                       in. */
    unsigned int sleep_mode = ((Irq *)d2)->sleep_mode; /* This is the sleep
                                                          mode the device in
                                                          currently in. Only
                                                          one bit should be
                                                          set. */

    if (state == STATE_SLEEP)
    {
        /* If device is in the sleep state, the irq will only pending if it
           can wake up the device. */

        if (sleep_mode & i1->vector->can_wake)
            return 0;           /* vector into the irq */
        else
            return -1;          /* try the next irq */
    }

    /* If the state is not STATE_SLEEP, any irq we see is automatically
       pending, so vector it. */

    return 0;
}

/* Walk the list looking for a pending irq which can be handled. If the device
   is in a sleep state, the can_wake mask could force the head of the list to
   not be the irq which gets vectored. */

static inline IntVect *
irq_get_pending_vector (DList *head, int state, unsigned int sleep_mode)
{
    Irq *found;
    Irq *irq = irq_new (NULL, state, sleep_mode);

    found = (Irq *)dlist_lookup (head, (AvrClass *)irq, irq_cmp_pending);
    class_unref ((AvrClass *)irq);

    return found->vector;
}

#if 0                           /* TRoth/2002-09-15: This isn't used. ??? */
static inline IntVect *
irq_get_head_vector (DList *head)
{
    return ((Irq *)dlist_get_head_data (head))->vector;
}
#endif

/***************************************************************************\
 *
 * AvrCore(AvrClass) Methods
 *
\***************************************************************************/

static void avr_core_construct (AvrCore *core, DevSuppDefn *dev);

/** \name AvrCore handling methods */

/*@{*/

/** \brief Allocate a new AvrCore object. */

AvrCore *
avr_core_new (char *dev_name)
{
    AvrCore *core = NULL;
    DevSuppDefn *dev = dev_supp_lookup_device (dev_name);

    if (dev)
    {
        fprintf (stderr, "\nSimulating a %s device.\n\n", dev_name);

        core = avr_new (AvrCore, 1);
        avr_core_construct (core, dev);
        class_overload_destroy ((AvrClass *)core, avr_core_destroy);
    }

    return core;
}

/** \brief Constructor for the AvrCore class. */

static void
avr_core_construct (AvrCore *core, DevSuppDefn *dev)
{
    int flash_sz = dev_supp_get_flash_sz (dev);
    int PC_sz = dev_supp_get_PC_sz (dev);
    int stack_sz = dev_supp_get_stack_sz (dev);
    int sram_sz = dev_supp_get_sram_sz (dev);
    int vtab_idx = dev_supp_get_vtab_idx (dev);
    int addr;

    if (core == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)core);

    core->state = STATE_STOPPED;
    core->sleep_mode = 0;       /* each bit represents a sleep mode */
    core->PC = 0;
    core->PC_size = PC_sz;
    core->PC_max = flash_sz / 2; /* flash_sz is in bytes, need number of
                                    words here */

    core->flash = flash_new (flash_sz);

    core->breakpoints = NULL;

    core->irq_pending = NULL;
    core->irq_vtable = (IntVect *)(global_vtable_list[vtab_idx]);
    core->irq_offset = 0;

    core->CK = 0;
    core->inst_CKS = 0;

    core->clk_cb = NULL;
    core->async_cb = NULL;

    /* FIXME: hack to get it to compile. */
    if (dev_supp_has_ext_io_reg (dev))
        core->mem = mem_new (0x1f, 0xff, 0xff + sram_sz, 0xffff);
    else
        core->mem = mem_new (0x1f, 0x5f, 0x5f + sram_sz, 0xffff);

    /* Attach the gpwr's to the memory bus. */

    core->gpwr = gpwr_new ();
    for (addr = 0; addr < 0x20; addr++)
    {
        static char *reg_name[] = { "r00", "r01", "r02", "r03", "r04", "r05",
                                    "r06", "r07", "r08", "r09", "r10", "r11",
                                    "r12", "r13", "r14", "r15", "r16", "r17",
                                    "r18", "r19", "r20", "r21", "r22", "r23",
                                    "r24", "r25", "r26", "r27", "r28", "r29",
                                    "r30", "r31" };

        avr_core_attach_vdev (core, addr, reg_name[addr],
                              (VDevice *)core->gpwr, 0, 0, 0xff, 0xff);
    }

    dev_supp_attach_io_regs (core, dev);

    /* Set up the stack. */

    if (stack_sz)
    {
        core->stack = (Stack *)hwstack_new (stack_sz);
    }
    else
    {
        /* Assuming that SPL is always at 0x5d. */

        core->stack = (Stack *)memstack_new (core->mem, 0x5d);
    }

    /* SPM instruction helper */
    core->spmhelper = (SPMhelper *) spmhelper_new (core->flash);

    /* Assuming the SREG is always at 0x5f. */

    core->sreg = (SREG *)avr_core_get_vdev_by_addr (core, 0x5f);
    class_ref ((AvrClass *)core->sreg);

    /* Assuming that RAMPZ is always at 0x5b. If the device doesn't support
       RAMPZ, install a NULL pointer. */

    core->rampz = (RAMPZ *)avr_core_get_vdev_by_addr (core, 0x5b);
    if (core->rampz)
        class_ref ((AvrClass *)core->rampz);

    /* Attach the internal sram to the memory bus if needed. */

    if (sram_sz)
    {
        int base;
        VDevice *sram;

        if (dev_supp_has_ext_io_reg (dev))
            base = SRAM_EXTENDED_IO_BASE;
        else
            base = SRAM_BASE;

        core->sram = sram_new (base, sram_sz);
        sram = (VDevice *)core->sram;

        avr_message ("attach: Internal SRAM from 0x%04x to 0x%04x\n", base,
                     (base + sram_sz - 1));

        for (addr = base; addr < (base + sram_sz); addr++)
        {
            avr_core_attach_vdev (core, addr, "Internal SRAM", sram, 0, 0,
                                  0xff, 0xff);
        }
    }
    else
    {
        core->sram = NULL;
    }

    /* Initialize the decoder lookup table */

    decode_init_lookup_table ();
}

/**
 * \brief Destructor for the AvrCore class.
 * 
 * Not to be called directly, except by a derived class.
 * Called via class_unref.
 */
void
avr_core_destroy (void *core)
{
    AvrCore *_core = (AvrCore *)core;

    if (_core == NULL)
        return;

    class_unref ((AvrClass *)_core->sreg);
    class_unref ((AvrClass *)_core->flash);
    class_unref ((AvrClass *)_core->gpwr);
    class_unref ((AvrClass *)_core->mem);
    class_unref ((AvrClass *)_core->stack);
    class_unref ((AvrClass *)_core->spmhelper);

    dlist_delete_all (_core->breakpoints);
    dlist_delete_all (_core->clk_cb);
    dlist_delete_all (_core->async_cb);
    dlist_delete_all (_core->irq_pending);

    class_destroy (core);
}

/** \brief Query the sizes of the 3 memory spaces: flash, sram, and eeprom. */
void
avr_core_get_sizes (AvrCore *core, int *flash, int *sram, int *sram_start,
                    int *eeprom)
{
    *flash = flash_get_size (core->flash);

    if (core->sram)
    {
        *sram = sram_get_size (core->sram);
        *sram_start = sram_get_base (core->sram);
    }
    else
    {
        *sram = 0;
        *sram_start = 0;
    }

    if (core->eeprom)
        *eeprom = eeprom_get_size (core->eeprom);
    else
        *eeprom = 0;
}

/** \brief Attach a virtual device into the Memory. */
extern inline void avr_core_attach_vdev (AvrCore *core, uint16_t addr,
                                         char *name, VDevice *vdev,
                                         int flags, uint8_t reset_value,
                                         uint8_t rd_mask, uint8_t wr_mask);

/** \brief Returns the \c VDevice with the name \a name. */
extern inline VDevice *avr_core_get_vdev_by_name (AvrCore *core, char *name);

/** \brief Returns the \c VDevice which handles the address \a addr. */
extern inline VDevice *avr_core_get_vdev_by_addr (AvrCore *core, int addr);

/** \brief Sets the device's state (running, stopped, breakpoint, sleep). */
extern inline void avr_core_set_state (AvrCore *core, StateType state);

/** \brief Returns the device's state (running, stopped, breakpoint, sleep). */
extern inline int avr_core_get_state (AvrCore *core);

/** \brief Sets the device to a sleep state.
  * \param core Pointer to the core.
  * \param sleep_mode The BITNUMBER of the sleepstate.
  */
extern inline void avr_core_set_sleep_mode (AvrCore *core, int sleep_mode);

/** \brief Return the device's sleepmode. */
extern inline int avr_core_get_sleep_mode (AvrCore *core);

/*@}*/

/** \name Program Memory Space Access Methods */

/*@{*/

/** \brief Reads a word from flash memory. */
static inline uint16_t avr_core_flash_read (AvrCore *core, int addr);

/** \brief Writes a word to flash memory. */
static inline void avr_core_flash_write (AvrCore *core, int addr,
                                         uint16_t val);

/** \brief Writes a byte to flash memory.
  *
  * This function writes the lower 8 bit of a flash word.
  * Use avr_core_flash_write() write to write a full word,
  * or avr_core_flash_write_hi8() to write the upper 8 bits.
  */
static inline void avr_core_flash_write_lo8 (AvrCore *core, int addr,
                                             uint8_t val);

/** \brief Writes a byte to flash memory.
  *
  * This function writes the upper 8 bit of a flash word.
  * Use avr_core_flash_write() write to write a full word,
  * or avr_core_flash_write_lo8() to write the lower 8 bits.
  */
static inline void avr_core_flash_write_hi8 (AvrCore *core, int addr,
                                             uint8_t val);

/*@}*/

/** \name Data Memory Space Access Methods */

/*@{*/

/** \brief Reads a byte from memory.
  *
  * This accesses the \a register \a file and the \a SRAM.
  */
static inline uint8_t avr_core_mem_read (AvrCore *core, int addr);

/** \brief Writes a byte to memory.
  *
  * This accesses the \a register \a file and the \a SRAM.
  */
static inline void avr_core_mem_write (AvrCore *core, int addr, uint8_t val);

/*@}*/

/** \name Status Register Access Methods */

/*@{*/

/** \brief Get the value of the status register. */

static inline uint8_t avr_core_sreg_get (AvrCore *core);

/** \brief Set the value of the status register. */

static inline void avr_core_sreg_set (AvrCore *core, uint8_t v);

/** \brief Get the value of bit \c b of the status register. */

extern inline int avr_core_sreg_get_bit (AvrCore *core, int b);

/** \brief Set the value of bit \c b of the status register. */

extern inline void avr_core_sreg_set_bit (AvrCore *core, int b, int v);

/*@}*/

/** \name RAMPZ access methods */

/*@{*/

/** \brief Get the value of the rampz register. */

extern inline uint8_t avr_core_rampz_get (AvrCore *core);

/** \brief Set the value of the rampz register. */

extern inline void avr_core_rampz_set (AvrCore *core, uint8_t v);

/*@}*/

/**
 * \Name General Purpose Working Register Access Methods
 */

/*@{*/

/** \brief Returns a GPWR's(\a r0-r31) value. */
static inline uint8_t avr_core_gpwr_get (AvrCore *core, int reg);

/** \brief Writes a GPWR's (\a r0-r31) value. */
static inline void avr_core_gpwr_set (AvrCore *core, int reg, uint8_t val);

/*@}*/

/**
 * \name Direct I/O Register Access Methods
 *
 * IO Registers are mapped in memory directly after the 32 (0x20)
 * general registers.
 */

/*@{*/

/** \brief Displays all registers. */
void
avr_core_io_display_names (AvrCore *core)
{
    int i;
    uint8_t val;
    char name[80];

    for (i = IO_REG_ADDR_BEGIN; i < IO_REG_ADDR_END; i++)
    {
        mem_io_fetch (core->mem, i, &val, name, sizeof (name) - 1);
        display_io_reg_name (i - IO_REG_ADDR_BEGIN, name);
    }
}

/** \brief Reads the value of a register.
  * \param core Pointer to the core.
  * \param reg The registers address. This address is counted above the
  *  beginning of the registers memory block (0x20).
  */
extern inline uint8_t avr_core_io_read (AvrCore *core, int reg);

/** \brief Writes the value of a register.
  * See avr_core_io_read() for a discussion of \a reg. */
extern inline void avr_core_io_write (AvrCore *core, int reg, uint8_t val);

/** \brief Read an io register into val and put the name of the register into
  * buf. */
static inline void avr_core_io_fetch (AvrCore *core, int reg, uint8_t * val,
                                      char *buf, int bufsiz);

/*@}*/

/** \name Stack Methods */

/*@{*/

/** \brief Pop 1-4 bytes off of the stack.
 *
 * See stack_pop() for more details.
 */
extern inline uint32_t avr_core_stack_pop (AvrCore *core, int bytes);

/** \brief Push 1-4 bytes onto the stack.
 *
 * See stack_push() for more details.
 */
extern inline void avr_core_stack_push (AvrCore *core, int bytes,
                                        uint32_t val);

/*@}*/

/** \name Program Counter Methods */

/*@{*/

/** \brief Returns the size of the Program Counter in bytes.
 *
 * Most devices have a 16-bit PC (2 bytes), but some larger ones
 * (e.g. mega256), have a 22-bit PC (3 bytes).
 */
extern inline int32_t avr_core_PC_size (AvrCore *core);

/** \brief Returns the maximum value of the Program Counter.
 *
 * This is flash_size / 2.
 */
static inline int32_t avr_core_PC_max (AvrCore *core);

/** \brief Return the current of the Program Counter. */
static inline int32_t avr_core_PC_get (AvrCore *core);

/** \brief Set the Program Counter to val.
 *
 * If val is not in the valid range of PC values, it is adjusted to fall in
 * the valid range.
 */
static inline void avr_core_PC_set (AvrCore *core, int32_t val);

/** \brief Increment the Program Counter by val.
 *
 * val can be either positive or negative.
 *
 * If the result of the incrememt is outside the valid range for PC, it is
 * adjusted to fall in the valid range. This allows addresses to wrap around
 * the end of the insn space.
 */
extern inline void avr_core_PC_incr (AvrCore *core, int val);

/*@}*/

/** \name Methods for accessing CK and instruction Clocks */

/*@{*/

/** \brief Get the current clock counter. */
extern inline uint64_t avr_core_CK_get (AvrCore *core);

/** \brief Increment the clock counter. */
extern inline void avr_core_CK_incr (AvrCore *core);

/** \brief Get the number of clock cycles remaining for the currently
 *  executing instruction. */
extern inline int avr_core_inst_CKS_get (AvrCore *core);

/** \brief Set the number of clock cycles for the instruction being
 *  executed. */
extern inline void avr_core_inst_CKS_set (AvrCore *core, int val);

/** \name Interrupt Access Methods. */

/*@{*/

/** \brief Gets the first pending irq. */
IntVect *
avr_core_irq_get_pending (AvrCore *core)
{
    return irq_get_pending_vector (core->irq_pending, core->state,
                                   core->sleep_mode);
}

/** \brief Raises an irq by adding it's data to the irq_pending list. */
void
avr_core_irq_raise (AvrCore *core, int irq)
{
    IntVect *irq_ptr = &core->irq_vtable[irq];

#if !defined(DISABLE_IRQ_MESSAGES)
    avr_message ("Raising irq # %d [%s at 0x%x]\n", irq, irq_ptr->name,
                 irq_ptr->addr * 2);
#endif
    core->irq_pending = irq_list_add (core->irq_pending, irq_ptr);
}

/** \brief Calls the interrupt's callback to clear the flag. */
void
avr_core_irq_clear (AvrCore *core, IntVect *irq)
{
    core->irq_pending = irq_list_delete (core->irq_pending, irq);
}

/** \brief Removes all irqs from the irq_pending list. */
extern inline void avr_core_irq_clear_all (AvrCore *core);

/*@}*/

/** \name Break point access methods. */

/*@{*/

/** \brief Inserts a break point. */

void
avr_core_insert_breakpoint (AvrCore *core, int pc)
{
#define BREAK_OPCODE 0x9598

    uint16_t insn = flash_read (core->flash, pc);

    core->breakpoints = brk_pt_list_add (core->breakpoints, pc, insn);

    flash_write (core->flash, pc, BREAK_OPCODE);
}

/** \brief Removes a break point. */

void
avr_core_remove_breakpoint (AvrCore *core, int pc)
{
    BreakPt *bp;

    bp = brk_pt_list_lookup (core->breakpoints, pc);
    if (bp)
    {
        uint16_t insn = bp->opcode;

        core->breakpoints = brk_pt_list_delete (core->breakpoints, pc);

        flash_write (core->flash, pc, insn);
    }
}

#ifndef DOXYGEN                 /* don't expose to doxygen */

struct bp_enable_data
{
    AvrCore *core;
    int enable;
};

#endif /* DOXYGEN */

static int
iter_enable_breakpoint (AvrClass *data, void *user_data)
{
    BreakPt *bp = (BreakPt *)data;
    struct bp_enable_data *bed = (struct bp_enable_data *)user_data;

    if (bed->enable)
    {
        uint16_t insn = flash_read (bed->core->flash, bp->pc);

        if (insn != BREAK_OPCODE)
        {
            /* Enable the breakpoint */
            bp->opcode = insn;
            flash_write (bed->core->flash, bp->pc, BREAK_OPCODE);
        }
    }
    else
    {
        /* Disable the breakpoint */
        flash_write (bed->core->flash, bp->pc, bp->opcode);
    }

    return 0;                   /* Don't delete any item from the list. */
}

/** \brief Disable breakpoints.

    Disables all breakpoints that where set using avr_core_insert_breakpoint().
    The breakpoints are not removed from the breakpoint list.  */

void
avr_core_disable_breakpoints (AvrCore *core)
{
    struct bp_enable_data bed = { core, 0 };

    core->breakpoints =
        brk_pt_iterator (core->breakpoints, iter_enable_breakpoint, &bed);
}

/** \brief Enable breakpoints. 

    Enables all breakpoints that where previous disabled. */

void
avr_core_enable_breakpoints (AvrCore *core)
{
    struct bp_enable_data bed = { core, 1 };

    core->breakpoints =
        brk_pt_iterator (core->breakpoints, iter_enable_breakpoint, &bed);
}

/*@}*/

/* Private
  
   Execute an instruction.
   Presets the number of instruction clocks to zero so that break points and
   invalid opcodes don't add extraneous clock counts.
   
   Also checks for software breakpoints. 
   Any opcode except 0xffff can be a breakpoint.
   
   Returns BREAK_POINT, or >= 0. */

static int
exec_next_instruction (AvrCore *core)
{
    int result, pc;
    uint16_t opcode;
    struct opcode_info *opi;

    pc = avr_core_PC_get (core);
    opcode = flash_read (core->flash, pc);

    /* Preset the number of instruction clocks to zero so that break points
       and invalid opcodes don't add extraneous clock counts. */
    avr_core_inst_CKS_set (core, 0);

    opi = decode_opcode (opcode);

    result = opi->func (core, opcode, opi->arg1, opi->arg2);

    if (global_debug_inst_output)
        fprintf (stderr, "0x%06x (0x%06x) : 0x%04x : %s\n", pc, pc * 2,
                 opcode, global_opcode_name[result]);

    return result;
}

/** \name Program control methods */

/*@{*/

/*
 * Private
 *
 * Checks to see if an interrupt is pending. If any are pending, and
 * if SREG(I) is set, the following will occur:
 *   - push current PC onto stack
 *   - PC <- interrupt vector
 *   - I flag of SREG is cleared
 *
 * Reset vector is not controlled by the SREG(I) flag, thus if reset
 * interrupt has occurred, the device will be reset irregardless of state
 * of SREG(I).
 *
 * \note There are different ways of doing this:
 * - In register.c  wdtcr_intr_cb() we can directly reset the MCU without
 *   the use of irqs. This would require to make the callback an async
 *   callback and it would allow the speed improvments commented out below
 *   to be activated.
 * - Keep it as an interrupt an waste CPU time.
 *
 * Ted, what do you think we should do?
 */

static void
avr_core_check_interrupts (AvrCore *core)
{
    IntVect *irq;

    if (core->irq_pending)
    {
        irq = avr_core_irq_get_pending (core);

        if (irq)
        {
            if (irq->name == NULL)
            {
                avr_error ("Raised an invalid irq for device");
            }

            if (irq->addr == IRQ_RESET_ADDR)
            {
                /* The global interrupt (SREG.I) never blocks a reset. We
                   don't need to clear the irq since a reset clears all
                   pending irq's. */
                avr_core_reset (core);
            }

            if (avr_core_sreg_get_bit (core, SREG_I))
            {
                int pc = avr_core_PC_get (core);
                int pc_bytes = avr_core_PC_size (core);

                avr_core_stack_push (core, pc_bytes, pc);
                avr_core_sreg_set_bit (core, SREG_I, 0);

#if !defined(DISABLE_IRQ_MESSAGES)
                avr_message ("Vectoring to irq at addr:0x%x offset:0x%x\n",
                             irq->addr * 2, core->irq_offset * 2);
#endif

                avr_core_PC_set (core, irq->addr + core->irq_offset);

                avr_core_irq_clear (core, irq);
            }
        }
    }
}

/**
 * \brief Process a single program instruction, all side effects and
 * peripheral stimulii.
 *
 * Executes instructions, calls callbacks, and checks for interrupts.  */

int
avr_core_step (AvrCore *core)
{
    int res = 0;
    int state;

    /* The MCU is stopped when in one of the many sleep modes */
    state = avr_core_get_state (core);
    if (state != STATE_SLEEP)
    {
        /* execute an instruction; may change state */
        res = exec_next_instruction (core);
    }

    /* Execute the clock callbacks */
    while (core->inst_CKS > 0)
    {
        /* propagate clocks here */
        avr_core_clk_cb_exec (core);

        avr_core_CK_incr (core);

        core->inst_CKS--;
    }

    /* FIXME: async cb's and interrupt checking might need to be put 
       somewhere else. */

    /* Execute the asynchronous callbacks */
    avr_core_async_cb_exec (core);

    /* Check interrupts here. If the previous instruction was a reti, then we
       need to delay handling of any pending IRQs until after the next
       instruction is executed. */
    if (res != opcode_RETI)
        avr_core_check_interrupts (core);

    return res;
}

/** \brief Start the processing of instructions by the simulator.
 *
 * The simulated device will run until one of the following occurs:
 *   - The state of the core is no longer STATE_RUNNING.
 *   - The simulator receives a SIGINT signal.
 *   - A breakpoint is reached (currently causes core to stop running).
 *   - A fatal internal error occurs.
 *
 * \note When running simulavr in gdb server mode, this function is not
 * used. The avr_core_step() function is called repeatedly in a loop when the
 * continue command is issued from gdb. As such, the functionality in this
 * loop should be kept to a minimum.
 *
 * \todo Should add some basic breakpoint handling here. Maybe allow
 * continuing, and simple breakpoint management (disable, delete, set)
 */

void
avr_core_run (AvrCore *core)
{
    uint64_t cnt = 0;
    int res;
    uint64_t start_time, run_time;

    avr_core_reset (core);      /* make sure the device is in a sane state. */

    core->state = STATE_RUNNING;

    signal_watch_start (SIGINT);

    /* FIXME: [TRoth 2002/03/19] This loop isn't going to handle sleep or idle
       modes properly. */

    start_time = get_program_time ();
    while (core->state == STATE_RUNNING)
    {
        if (signal_has_occurred (SIGINT))
            break;

        res = avr_core_step (core);

        if (res == BREAK_POINT)
            break;

        cnt++;
    }
    run_time = get_program_time () - start_time;

    signal_watch_stop (SIGINT);
    
    /* avoid division by zero below */
    if (run_time == 0) run_time = 1;
    
    avr_message ("Run time was %lld.%03lld seconds.\n", run_time / 1000,
                 run_time % 1000);

    if (run_time == 0)
         run_time = 1;          /* Avoid division by zero. */

    avr_message ("Executed %lld instructions.\n", cnt);
    avr_message ("   %lld insns/sec\n", (cnt * 1000) / run_time);
    avr_message ("Executed %lld clock cycles.\n", avr_core_CK_get (core));
    avr_message ("   %lld clks/sec\n",
                 (avr_core_CK_get (core) * 1000) / run_time);
}

/** \brief Sets the simulated CPU back to its initial state.
 *
 *  Zeroes out PC, IRQ's, clock, and memory.
 */

void
avr_core_reset (AvrCore *core)
{
    avr_core_PC_set (core, 0);
    avr_core_irq_clear_all (core);

    avr_core_inst_CKS_set (core, 0);

    /* Send clock cycles to display.
       Normaly the clockcycles must not be reset here!
       This leads to an error in the vcd file. */

    display_clock (core->CK);

    mem_reset (core->mem);
}

/*@}*/

/** \name Callback Handling Methods */

/*@{*/

/**
 * \brief For adding external read and write callback functions.
 *
 * rd and wr should come in pairs, but it is safe to add
 * empty function via passing a NULL pointer for either function.
 *
 * \param core A pointer to an AvrCore object.
 *
 * \param port_id The ID for handling the simulavr inheritance model.
 *
 * \param ext_rd Function for the device core to call when it needs to
 * communicate with the external world via I/O Ports.
 *
 * \param ext_wr Function for the device core to call when it needs to
 * communicate with the external world via I/O Ports.
 *
 */
void
avr_core_add_ext_rd_wr (AvrCore *core, int addr, PortFP_ExtRd ext_rd,
                        PortFP_ExtWr ext_wr)
{
    Port *p = (Port *)mem_get_vdevice_by_addr (core->mem, addr);

    if (p == NULL)
    {
        avr_warning ("Device does not have vdevice at 0x%04x.\n", addr);
        return;
    }

    port_add_ext_rd_wr (p, ext_rd, ext_wr);
}

/**
 * \brief Add a new clock callback to list.
 */
extern inline void avr_core_clk_cb_add (AvrCore *core, CallBack *cb);

/**
 * \brief Add a new asynchronous callback to list.
 */
extern inline void avr_core_async_cb_add (AvrCore *core, CallBack *cb);

/** \brief Run all the callbacks in the list.
 
    If a callback returns non-zero (true), then it is done with it's job and
    wishes to be removed from the list.
 
    The time argument has dual meaning. If the callback list is for the clock
    callbacks, time is the value of the CK clock counter. If the callback list
    is for the asynchronous callback, time is the number of milliseconds from
    some unknown, arbitrary time on the host system. */

extern inline void avr_core_clk_cb_exec (AvrCore *core);

/**
 * \brief Run all the asynchronous callbacks.
 */
extern inline void avr_core_async_cb_exec (AvrCore *core);

/*@}*/

/**
 * \brief Dump the contents of the entire CPU core.
 *
 * \param core A pointer to an AvrCore object.
 * \param f_core An open file descriptor.
 */
void
avr_core_dump_core (AvrCore *core, FILE * f_core)
{
    unsigned int pc = avr_core_PC_get (core);

    fprintf (f_core, "PC = 0x%06x (PC*2 = 0x%06x)\n\n", pc, pc * 2);
    mem_dump_core (core->mem, f_core);
    flash_dump_core (core->flash, f_core);
}

/**
 * \brief Load a program from an input file.
 */
int
avr_core_load_program (AvrCore *core, char *file, int format)
{
    return flash_load_from_file (core->flash, file, format);
}

/** \brief Load a program from an input file. */
int
avr_core_load_eeprom (AvrCore *core, char *file, int format)
{
    EEProm *ee = (EEProm *)mem_get_vdevice_by_name (core->mem, "EEProm");

    return eeprom_load_from_file (ee, file, format);
}
