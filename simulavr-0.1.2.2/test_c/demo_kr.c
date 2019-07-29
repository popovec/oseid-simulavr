/*
 * $Id: demo_kr.c,v 1.4 2003/09/10 04:59:36 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002, 2003  Ken Restivo
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

/* This simple program was contributed by Ken Restivo. */

#include <avr/io.h>
#include "common.h"

void debugTest( void )
{
    PORTC |= _BV(PC0);
}

int main( void )
{
    DDRC = (_BV(PC0) | _BV(PC1) | _BV(PC2));

    debugTest();

    return (0);
}
