/*
 * $Id: callback.c,v 1.5 2003/12/01 09:10:14 troth Exp $
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "avrclass.h"
#include "utils.h"
#include "callback.h"

/****************************************************************************\
 *
 * Clock Call Back methods
 *
\****************************************************************************/

#ifndef DOXYGEN                 /* don't expose to doxygen */

struct _CallBack
{
    AvrClass parent;
    CallBack_FP func;           /* the callback function */
    AvrClass *data;             /* user data to be passed to callback
                                   function */
};

#endif /* DOXYGEN */

CallBack *
callback_new (CallBack_FP func, AvrClass *data)
{
    CallBack *cb;

    cb = avr_new (CallBack, 1);
    callback_construct (cb, func, data);
    class_overload_destroy ((AvrClass *)cb, callback_destroy);

    return cb;
}

void
callback_construct (CallBack *cb, CallBack_FP func, AvrClass *data)
{
    if (cb == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)cb);

    cb->func = func;

    cb->data = data;

    if (data)
        class_ref (data);
}

void
callback_destroy (void *cb)
{
    CallBack *_cb = (CallBack *)cb;

    if (cb == NULL)
        return;

    if (_cb->data)
        class_unref (_cb->data);

    class_destroy (cb);
}

DList *
callback_list_add (DList *head, CallBack *cb)
{
    return dlist_add (head, (AvrClass *)cb, NULL);
}

static int
callback_execute (AvrClass *data, void *user_data)
{
    CallBack *cb = (CallBack *)data;
    uint64_t time = *(uint64_t *) user_data;

    return cb->func (time, cb->data);
}

DList *
callback_list_execute_all (DList *head, uint64_t time)
{
    return dlist_iterator (head, callback_execute, &time);
}
