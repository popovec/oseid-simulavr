/*
 * $Id: utils.h,v 1.6 2003/12/01 07:35:54 troth Exp $
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

#ifndef SIM_UTILS_H
#define SIM_UTILS_H

/****************************************************************************\
 *
 * File Format Types
 *
\****************************************************************************/

enum FileFormatType
{
    FFMT_BIN,                   /* Raw Binary image */
    FFMT_IHEX,                  /* Intel hex format */
    FFMT_ELF,                   /* GCC ELF */
};

extern int str2ffmt (char *str);

/****************************************************************************\
 *
 * Utilities for setting or clearing a bit in a byte or word.
 *
\****************************************************************************/

extern inline uint8_t
set_bit_in_byte (uint8_t src, int bit, int val)
{
    return ((src & ~(1 << bit)) | ((val != 0) << bit));
}

extern inline uint16_t
set_bit_in_word (uint16_t src, int bit, int val)
{
    return ((src & ~(1 << bit)) | ((val != 0) << bit));
}

/****************************************************************************\
 *
 * Utility for getting elapsed program time in milliseconds.
 *
\****************************************************************************/

extern uint64_t get_program_time (void);

/****************************************************************************\
 *
 * DList(AvrClass) Methods : A doubly linked list.
 *
\****************************************************************************/

typedef struct _DList DList;

typedef int (*DListFP_Cmp) (AvrClass *d1, AvrClass *d2);
typedef int (*DListFP_Iter) (AvrClass *data, void *user_data);

extern DList *dlist_add (DList *head, AvrClass *data, DListFP_Cmp cmp);
extern DList *dlist_add_head (DList *head, AvrClass *data);
extern DList *dlist_delete (DList *head, AvrClass *data, DListFP_Cmp cmp);
extern void dlist_delete_all (DList *head);
extern AvrClass *dlist_lookup (DList *head, AvrClass *data, DListFP_Cmp cmp);
extern AvrClass *dlist_get_head_data (DList *head);
extern DList *dlist_iterator (DList *head, DListFP_Iter func,
                              void *user_data);

#endif /* SIM_UTILS_H */
