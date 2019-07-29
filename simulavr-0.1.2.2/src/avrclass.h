/*
 * $Id: avrclass.h,v 1.3 2003/12/01 07:35:52 troth Exp $
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

#ifndef SIM_AVRCLASS_H
#define SIM_AVRCLASS_H

/****************************************************************************\
 *
 * OOP Implementaion built up from AvrClass structure.
 *
\****************************************************************************/

typedef struct _AvrClass AvrClass;
typedef void (*AvrClassFP_Destroy) (void *klass);

struct _AvrClass
{
    int type;
    int ref_count;
    AvrClassFP_Destroy destroy; /* can be overridden */
};

extern AvrClass *class_new (void);
extern void class_construct (AvrClass *klass);
extern void class_destroy (void *klass);
extern void class_overload_destroy (AvrClass *klass,
                                    AvrClassFP_Destroy destroy);
extern void class_ref (AvrClass *klass);
extern void class_unref (AvrClass *klass);

#endif /* SIM_AVRCLASS_H */
