/*
 * $Id: stack.h,v 1.6 2004/01/30 07:09:56 troth Exp $
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

#ifndef SIM_STACK_H
#define SIM_STACK_H

/****************************************************************************\
 *
 * Stack(AvrClass) Definition. 
 *
 * This is a virtual class for higher level stack implementations and as such
 * should not be used directly.
 *
\****************************************************************************/

typedef struct _Stack Stack;

typedef uint32_t (*StackFP_Pop) (Stack *stack, int bytes);
typedef void (*StackFP_Push) (Stack *stack, int bytes, uint32_t val);

typedef enum
{
    STACK_HARDWARE,
    STACK_MEMORY,
} StackType;

struct _Stack
{
    AvrClass parent;
    StackFP_Pop pop;
    StackFP_Push push;
};

extern Stack *stack_new (StackFP_Pop pop, StackFP_Push push);
extern void stack_construct (Stack *stack, StackFP_Pop pop,
                             StackFP_Push push);
extern void stack_destroy (void *stack);

extern uint32_t stack_pop (Stack *stack, int bytes);
extern void stack_push (Stack *stack, int bytes, uint32_t val);

/****************************************************************************\
 *
 * HWStack(Stack) Definition.
 *
\****************************************************************************/

typedef struct _Hardware_Stack HWStack;

struct _Hardware_Stack
{
    Stack parent;
    int depth;
    uint32_t *stack;            /* an array used as the stack */
};

extern HWStack *hwstack_new (int depth);
extern void hwstack_construct (HWStack *stack, int depth);
extern void hwstack_destroy (void *stack);

/****************************************************************************\
 *
 * MemStack(Stack) Definition.
 *
\****************************************************************************/

typedef struct _Memory_Stack MemStack;

enum _stack_point_constants
{
    STACK_POINTER_BASE = 0x5d,
    STACK_POINTER_SIZE = 2,

    SPL_ADDR = STACK_POINTER_BASE,
    SPH_ADDR = STACK_POINTER_BASE + 1,

    SPL_IO_REG = SPL_ADDR - IO_REG_ADDR_BEGIN,
    SPH_IO_REG = SPH_ADDR - IO_REG_ADDR_BEGIN,
};

struct _Memory_Stack
{
    Stack parent;
    Memory *mem;                /* Memory were the stack will live */
    VDevice *SP;                /* Virtual Device for the stack pointer */
};

extern MemStack *memstack_new (Memory *mem, int spl_addr);
extern void memstack_construct (MemStack *stack, Memory *mem, int spl_addr);
extern void memstack_destroy (void *stack);

extern VDevice *sp_create (int addr, char *name, int rel_addr, void *data);

#endif /* SIM_STACK_H */
