/*
 * $Id: avrclass.c,v 1.8 2003/12/01 09:10:13 troth Exp $
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
 * \file avrclass.c
 * \brief Methods to provide user interfaces to the AvrClass structure.
 *
 * This module provides the basis for simulavr's object mechanism. For a
 * detailed discussion on using simulavr's class mechanism, see the simulavr
 * users manual. FIXME: [TRoth 2002/03/19] move the discussion here. */

#include <stdlib.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "avrclass.h"

/** \brief This function should never be used. 
 *
 *  The only potential use for it as a template for derived classes. 
 *  Do Not Use This Function! */

AvrClass *
class_new (void)
{
    AvrClass *klass = avr_new (AvrClass, 1);
    class_construct (klass);
    return klass;
}

/** \brief Initializes the AvrClass data structure. 
 *
 *  A derived class should call this function from their own 
 *  \<klass\>_construct() function. All classes should
 *  have their constructor function call their parent's constructor
 *  function. */

void
class_construct (AvrClass *klass)
{
    if (klass == NULL)
        avr_error ("passed null ptr");

    klass->ref_count = 1;
    class_overload_destroy (klass, class_destroy);
}

/** \brief Releases resources allocated by class's \<klass\>_new() function. 
 *
 * This function should never be called except as the last statement 
 * of a directly derived class's destroy method. 
 * All classes should have their destroy method call their parent's 
 * destroy method. */

void
class_destroy (void *klass)
{
    if (klass == NULL)
        return;

    avr_free (klass);
}

/** \brief Overload the default destroy method.
 *
 * Derived classes will call this to replace class_destroy() with their own
 * destroy method. */

void
class_overload_destroy (AvrClass *klass, AvrClassFP_Destroy destroy)
{
    if (klass == NULL)
        avr_error ("passed null ptr");

    klass->destroy = destroy;
}

/** \brief Increments the reference count for the klass object. 
 *
 * The programmer must call this whenever a reference to an object 
 * is stored in more than one place. */

void
class_ref (AvrClass *klass)
{
    if (klass == NULL)
        avr_error ("passed null ptr");

    klass->ref_count++;
}

/** \brief Decrements the reference count for the klass object. 
 *
 * When the reference count reaches zero, the class's destroy method 
 * is called on the object. */

void
class_unref (AvrClass *klass)
{
    if (klass == NULL)
        avr_error ("passed null ptr");

    klass->ref_count--;
    if (klass->ref_count == 0)
        klass->destroy (klass);
}
