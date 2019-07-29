/*
 * $Id: callback.h,v 1.4 2003/12/01 07:35:52 troth Exp $
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

#ifndef SIM_CALLBACK_H
#define SIM_CALLBACK_H

/****************************************************************************\
 *
 * CallBack(AvrClass) : Callback Definition
 *
 * This structure provides a generic method for installing functions that
 * should be called by main loop or clock propagator on behave of a virtual
 * device.
 *
\****************************************************************************/

typedef struct _CallBack CallBack;

typedef int (*CallBack_FP) (uint64_t time, AvrClass *data);

enum _cb_ret_codes
{
    CB_RET_RETAIN = 0,          /* Give the callback in the list and run it
                                   again */
    CB_RET_REMOVE = 1,          /* Remove callback from list */
};

extern CallBack *callback_new (CallBack_FP func, AvrClass *data);
extern void callback_construct (CallBack *cb, CallBack_FP func,
                                AvrClass *data);
extern void callback_destroy (void *cb);

extern DList *callback_list_add (DList *head, CallBack *cb);
extern DList *callback_list_execute_all (DList *head, uint64_t time);

#endif /* SIM_CALLBACK_H */
