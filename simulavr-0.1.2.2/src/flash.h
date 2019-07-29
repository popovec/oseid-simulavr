/*
 * $Id: flash.h,v 1.5 2003/12/02 08:25:00 troth Exp $
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

#ifndef SIM_FLASH_H
#define SIM_FLASH_H

/***************************************************************************\
 *
 * Flash(Storage) Object
 *
\***************************************************************************/

typedef struct _Flash Flash;

struct _Flash
{
    Storage parent;
};

extern Flash *flash_new (int size);
extern void flash_construct (Flash *flash, int size);
extern void flash_destroy (void *flash);

extern int flash_get_size (Flash *flash);
extern void flash_dump_core (Flash *flash, FILE * f_core);

extern inline uint16_t
flash_read (Flash *flash, int addr)
{
    return storage_readw ((Storage *)flash, addr * 2);
}

extern void flash_write (Flash *flash, int addr, uint16_t val);
extern void flash_write_lo8 (Flash *flash, int addr, uint8_t val);
extern void flash_write_hi8 (Flash *flash, int addr, uint8_t val);

extern int flash_load_from_file (Flash *flash, char *file, int format);

#endif /* SIM_FLASH_H */
