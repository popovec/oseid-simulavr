/*
 * $Id: stack.c,v 1.15 2004/01/30 07:09:56 troth Exp $
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

/** \file stack.c
    \brief Module for the definition of the stack. 

    Defines the classes stack, hw_stack, and mem_stack.

    FIXME: Ted, I would really really really love to put in a description of
    what is the difference between these three classes and how they're used,
    but I don't understand it myself. */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#include "display.h"

static uint32_t hw_pop (Stack *stack, int bytes);
static void hw_push (Stack *stack, int bytes, uint32_t val);

static uint32_t mem_pop (Stack *stack, int bytes);
static void mem_push (Stack *stack, int bytes, uint32_t val);

/****************************************************************************\
 *
 * Stack(AvrClass) Definition. 
 *
\****************************************************************************/

/** \brief Allocates memory for a new Stack object

    This is a virtual method for higher level stack implementations and as
    such should not be used directly. */

Stack *
stack_new (StackFP_Pop pop, StackFP_Push push)
{
    Stack *st;

    st = avr_new (Stack, 1);
    stack_construct (st, pop, push);
    class_overload_destroy ((AvrClass *)st, stack_destroy);

    return st;
}

/** \brief Constructor for the Stack class.

    This is a virtual method for higher level stack implementations and as
    such should not be used directly. */

void
stack_construct (Stack *stack, StackFP_Pop pop, StackFP_Push push)
{
    if (stack == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)stack);

    stack->pop = pop;
    stack->push = push;
}

/** \brief Destructor for the Stack class.

    This is a virtual method for higher level stack implementations and as
    such should not be used directly. */

void
stack_destroy (void *stack)
{
    if (stack == NULL)
        return;

    class_destroy (stack);
}

/** \brief Pops a byte or a word off the stack and returns it.
    \param stack A pointer to the Stack object from which to pop
    \param bytes Number of bytes to pop off the stack (1 to 4 bytes).

    \return The 1 to 4 bytes value popped from the stack. 

    This method provides access to the derived class's pop() method. */

uint32_t
stack_pop (Stack *stack, int bytes)
{
    return stack->pop (stack, bytes);
}

/** \brief Pushes a byte or a word of data onto the stack.
    \param stack A pointer to the Stack object from which to pop.
    \param bytes Size of the value being pushed onto the stack (1 to 4 bytes).
    \param val The value to be pushed.

    This method provides access to the derived class's push() method. */

void
stack_push (Stack *stack, int bytes, uint32_t val)
{
    stack->push (stack, bytes, val);
}

/****************************************************************************\
 *
 * HWStack(Stack) Definition.
 *
\****************************************************************************/

/** \brief Allocate a new HWStack object

    This is the stack implementation used by devices which lack SRAM and only
    have a fixed size hardware stack (e.i., the at90s1200) */

HWStack *
hwstack_new (int depth)
{
    HWStack *st;

    st = avr_new (HWStack, 1);
    hwstack_construct (st, depth);
    class_overload_destroy ((AvrClass *)st, hwstack_destroy);

    return st;
}

/** \brief Constructor for HWStack object */

void
hwstack_construct (HWStack *stack, int depth)
{
    if (stack == NULL)
        avr_error ("passed null ptr");

    stack_construct ((Stack *)stack, hw_pop, hw_push);

    stack->depth = depth;
    stack->stack = avr_new0 (uint32_t, depth);
}

/** \brief Destructor for HWStack object */

void
hwstack_destroy (void *stack)
{
    if (stack == NULL)
        return;

    avr_free (((HWStack *)stack)->stack);
    stack_destroy (stack);
}

/* The HWStack pop method. */

static uint32_t
hw_pop (Stack *stack, int bytes)
{
    HWStack *hwst = (HWStack *)stack;
    int i;
    uint32_t val = hwst->stack[0];

    for (i = 0; i < (hwst->depth - 1); i++)
    {
        hwst->stack[i] = hwst->stack[i + 1];
    }

    return val;
}

/* The HWStack push method. */

static void
hw_push (Stack *stack, int bytes, uint32_t val)
{
    HWStack *hwst = (HWStack *)stack;
    int i;

    for (i = (hwst->depth - 1); i; i--)
    {
        hwst->stack[i - 1] = hwst->stack[i];
    }

    hwst->stack[0] = val;
}

/****************************************************************************\
 *
 * StackPointer(VDevice) Definition.
 *
\****************************************************************************/

#ifndef DOXYGEN                 /* don't expose to doxygen */

typedef struct _StackPointer StackPointer;
struct _StackPointer
{
    VDevice parent;

    uint16_t SPL_addr;          /* Since some devices don't have a SPH, we
                                   only track SPL address and assume the SPH
                                   address is SPL_addr + 1. */

    uint8_t SPL;                /* Low byte of stack pointer */
    uint8_t SPH;                /* High byte of stack pointer */
};

#endif

static StackPointer *sp_new (int addr, char *name);
static void sp_construct (StackPointer *sp, int addr, char *name);
static void sp_destroy (void *sp);
static uint8_t sp_read (VDevice *dev, int addr);
static void sp_write (VDevice *dev, int addr, uint8_t val);
static void sp_reset (VDevice *dev);
static uint16_t sp_get (VDevice *sp);
static void sp_set (VDevice *sp, uint16_t val);
static void sp_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                         void *data);

/** \brief Create the Stack Pointer VDevice.

    This should only be used in the DevSuppDefn io reg init structure. */

VDevice *
sp_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)sp_new (addr, name);
}

static StackPointer *
sp_new (int addr, char *name)
{
    StackPointer *sp;

    sp = avr_new (StackPointer, 1);
    sp_construct (sp, addr, name);
    class_overload_destroy ((AvrClass *)sp, sp_destroy);

    return sp;
}

static void
sp_construct (StackPointer *sp, int addr, char *name)
{
    if (sp == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)sp, sp_read, sp_write, sp_reset, sp_add_addr);

    sp_add_addr ((VDevice *)sp, addr, name, 0, NULL);

    sp_reset ((VDevice *)sp);
}

static void
sp_destroy (void *sp)
{
    if (sp == NULL)
        return;

    vdev_destroy (sp);
}

static uint8_t
sp_read (VDevice *dev, int addr)
{
    StackPointer *sp = (StackPointer *)dev;

    if (addr == sp->SPL_addr)
        return sp->SPL;
    else if (addr == (sp->SPL_addr + 1))
        return sp->SPH;
    else
        avr_error ("Bad address: 0x%04x", addr);

    return 0;
}

static void
sp_write (VDevice *dev, int addr, uint8_t val)
{
    /* Don't need display_io_reg() here since it's called higher up in mem
       chain. */

    StackPointer *sp = (StackPointer *)dev;

    if (addr == sp->SPL_addr)
        sp->SPL = val;
    else if (addr == (sp->SPL_addr + 1))
        sp->SPH = val;
    else
        avr_error ("Bad address: 0x%04x", addr);
}

static void
sp_reset (VDevice *dev)
{
    StackPointer *sp = (StackPointer *)dev;

    display_io_reg (SPL_IO_REG, sp->SPL = 0);
    display_io_reg (SPH_IO_REG, sp->SPH = 0);
}

static uint16_t
sp_get (VDevice *sp)
{
    return (((StackPointer *)sp)->SPH << 8) + ((StackPointer *)sp)->SPL;
}

static void
sp_set (VDevice *sp, uint16_t val)
{
    display_io_reg (SPL_IO_REG, ((StackPointer *)sp)->SPL = val & 0xff);
    display_io_reg (SPH_IO_REG, ((StackPointer *)sp)->SPH = val >> 8);
}

static void
sp_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    StackPointer *sp = (StackPointer *)vdev;

    if (strncmp ("SPL", name, 3) == 0)
        sp->SPL_addr = addr;

    else if (strncmp ("SPH", name, 3) == 0)
        ;

    else
        avr_error ("Bad address: 0x%04x", addr);
}

/****************************************************************************\
 *
 * MemStack(Stack) Definition.
 *
\****************************************************************************/

/** \brief Allocate a new MemStack object */

MemStack *
memstack_new (Memory *mem, int spl_addr)
{
    MemStack *st;

    st = avr_new (MemStack, 1);
    memstack_construct (st, mem, spl_addr);
    class_overload_destroy ((AvrClass *)st, memstack_destroy);

    return st;
}

/** \brief Constructor for MemStack object */

void
memstack_construct (MemStack *stack, Memory *mem, int spl_addr)
{
    if (stack == NULL)
        avr_error ("passed null ptr");

    stack_construct ((Stack *)stack, mem_pop, mem_push);

    class_ref ((AvrClass *)mem);
    stack->mem = mem;

    stack->SP = mem_get_vdevice_by_addr (mem, spl_addr);
    if (stack->SP == NULL)
    {
        avr_error ("attempt to attach non-extistant SPL register");
    }
    class_ref ((AvrClass *)stack->SP);
}

/** \brief Destructor for MemStack object */

void
memstack_destroy (void *stack)
{
    MemStack *_stack = (MemStack *)stack;

    if (stack == NULL)
        return;

    class_unref ((AvrClass *)_stack->SP);
    class_unref ((AvrClass *)_stack->mem);

    stack_destroy (stack);
}

/* The MemStack pop method */

static uint32_t
mem_pop (Stack *stack, int bytes)
{
    MemStack *mst = (MemStack *)stack;
    int i;
    uint32_t val = 0;
    uint16_t sp = sp_get (mst->SP);

    if ((bytes < 0) || (bytes >= sizeof (uint32_t)))
        avr_error ("bytes out of bounds: %d", bytes);

    for (i = bytes - 1; i >= 0; i--)
    {
        sp++;
        val |= (mem_read (mst->mem, sp) << (i * 8));
    }

    sp_set (mst->SP, sp);

    return val;
}

/* The MemStack push method. */

static void
mem_push (Stack *stack, int bytes, uint32_t val)
{
    MemStack *mst = (MemStack *)stack;
    int i;
    uint16_t sp = sp_get (mst->SP);

    if ((bytes < 0) || (bytes >= sizeof (uint32_t)))
        avr_error ("bytes out of bounds: %d", bytes);

    for (i = 0; i < bytes; i++)
    {
        mem_write (mst->mem, sp, val & 0xff);
        val >>= 8;
        sp--;
    }

    sp_set (mst->SP, sp);
}
