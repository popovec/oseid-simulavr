/*
 * $Id: utils.c,v 1.19 2003/12/01 09:10:17 troth Exp $
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
 * \file utils.c
 * \brief Utility functions.
 *
 * This module provides general purpose utilities.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "avrclass.h"
#include "utils.h"

/** \brief Utility function to convert a string to a FileFormatType code. */

int
str2ffmt (char *str)
{
    if (strncmp (str, "bin", 3) == 0)
        return FFMT_BIN;
    if (strncmp (str, "ihex", 4) == 0)
        return FFMT_IHEX;
    if (strncmp (str, "elf", 3) == 0)
        return FFMT_ELF;

    return -1;
}

/** \brief Set a bit in src to 1 if val != 0, clears bit if val == 0. */

extern inline uint8_t set_bit_in_byte (uint8_t src, int bit, int val);

/** \brief Set a bit in src to 1 if val != 0, clears bit if val == 0. */

extern inline uint16_t set_bit_in_word (uint16_t src, int bit, int val);

/** \brief Return the number of milliseconds of elapsed program time.

    \return an unsigned 64 bit number. Time zero is not well
    defined, so only time differences should be used. */

uint64_t
get_program_time (void)
{
    static uint64_t result;

// do not call gettimeofday() - this slows down the simulation
// (due meltdown/spectre mitigation patches in kernel)

/*
    uint64_t result;
    struct timeval tv;

    if (gettimeofday (&tv, NULL) < 0)
        avr_error ("Failed to get program time.");

    result = ((uint64_t) tv.tv_sec * 1000) + ((uint64_t) tv.tv_usec / 1000);
*/
    return ++result;
}

/***************************************************************************\
 *
 * DList(AvrClass) Methods : A doubly linked list.
 *
\***************************************************************************/

static DList *dlist_new_node (AvrClass *data);
static void dlist_construct_node (DList *node, AvrClass *data);
static void dlist_destroy_node (void *node);

#ifndef DOXYGEN                 /* Don't expose to doxygen, structure is
                                   opaque. */

struct _DList
{
    AvrClass parent;
    struct _DList *prev;
    struct _DList *next;
    AvrClass *data;
};

#endif

static DList *
dlist_new_node (AvrClass *data)
{
    DList *node;

    node = avr_new (DList, 1);
    dlist_construct_node (node, data);
    class_overload_destroy ((AvrClass *)node, dlist_destroy_node);

    return node;
}

static void
dlist_construct_node (DList *node, AvrClass *data)
{
    if (node == NULL)
        avr_error ("passed null ptr");

    class_construct ((AvrClass *)node);

    node->prev = NULL;
    node->next = NULL;

    node->data = data;
}

static void
dlist_destroy_node (void *node)
{
    DList *_node = (DList *)node;

    if (_node == NULL)
        return;

    class_unref (_node->data);

    class_destroy (node);
}

/** \brief Add a new node to the end of the list.

   If cmp argument is not NULL, use cmp() to see if node already exists and
   don't add node if it exists.

   It is the responsibility of this function to unref data if not added. */

DList *
dlist_add (DList *head, AvrClass *data, DListFP_Cmp cmp)
{
    DList *node = head;

    if (head == NULL)
        /* The list is empty, make new node the head. */
        return dlist_new_node (data);

    /* Walk the list to find the end */

    while (node)
    {
        if (cmp && ((*cmp) (node->data, data) == 0))
        {
            /* node already exists and we were asked to keep nodes unique */
            class_unref (data);
            break;
        }

        if (node->next == NULL)
        {
            /* at the tail */
            node->next = dlist_new_node (data);
            node->next->prev = node;
            break;
        }

        /* move on to next node */
        node = node->next;
    }

    return head;
}

/** \brief Add a new node at the head of the list. */

DList *
dlist_add_head (DList *head, AvrClass *data)
{
    DList *node = dlist_new_node (data);;

    if (head)
    {
        head->prev = node;
        node->next = head;
    }

    return node;
}

/** \brief Conditionally delete a node from the list.

    Delete a node from the list if the node's data matches the specified
    data. Returns the head of the modified list. */

DList *
dlist_delete (DList *head, AvrClass *data, DListFP_Cmp cmp)
{
    DList *node = head;

    if (cmp == NULL)
        avr_error ("compare function not specified");

    while (node)
    {
        if ((*cmp) (node->data, data) == 0)
        {
            if ((node->prev == NULL) && (node->next == NULL))
            {
                /* deleting only node in list (node is head and tail) */
                head = NULL;
            }
            else if (node->prev == NULL)
            {
                /* node is head, but other nodes exist */
                node->next->prev = NULL;
                head = node->next;
            }
            else if (node->next == NULL)
            {
                /* node is tail, but other nodes exist */
                node->prev->next = NULL;
            }
            else
            {
                /* node is not head nor tail */
                node->prev->next = node->next;
                node->next->prev = node->prev;
            }

            /* this will also unref the node->data */
            class_unref ((AvrClass *)node);

            return head;
        }

        /* move on to next node */
        node = node->next;
    }

    /* if we get here, data wasn't found, just return original head */
    return head;
}

/** \brief Blow away the entire list. */

void
dlist_delete_all (DList *head)
{
    DList *node;

    while (head)
    {
        node = head;
        head = head->next;

        class_unref ((AvrClass *)node);
    }
}

/** \brief Lookup an item in the list.

    Walk the list pointed to by head and return a pointer to the data if
    found. If not found, return NULL. 

    \param head The head of the list to be iterated.
    \param data The data to be passed to the func when it is applied.
    \param cmp  A function to be used for comparing the items.

    \return     A pointer to the data found, or NULL if not found. */

AvrClass *
dlist_lookup (DList *head, AvrClass *data, DListFP_Cmp cmp)
{
    DList *node = head;

    if (cmp == NULL)
        avr_error ("compare function not specified");

    while (node)
    {
        if ((*cmp) (node->data, data) == 0)
            return node->data;

        node = node->next;
    }

    /* If we get here, no node was found, return NULL. */

    return NULL;
}

/** \brief Extract the data from the head of the list.

    Returns the data element for the head of the list. If the list is empty,
    return a NULL pointer.

    \param head The head of the list.

    \return     A pointer to the data found, or NULL if not found. */

AvrClass *
dlist_get_head_data (DList *head)
{

    if (head == NULL)
    {
        return NULL;
    }

    return head->data;
}

/* a simple node compare function for the iterator. */

static int
dlist_iterator_cmp (AvrClass *n1, AvrClass *n2)
{
    /* Since this is only used in the iterator, we are guaranteed that it is
       safe to compare data pointers because both n1 and n2 came from the
       list. */

    return (int)(n1 - n2);
}

/** \brief Iterate over all elements of the list.

    For each element, call the user supplied iterator function and pass it the
    node data and the user_data. If the iterator function return non-zero,
    remove the node from the list.

    \param head The head of the list to be iterated.
    \param func The function to be applied.
    \param user_data The data to be passed to the func when it is applied.

    \return A pointer to the head of the possibly modified list. */

DList *
dlist_iterator (DList *head, DListFP_Iter func, void *user_data)
{
    DList *node = head;

    if (func == NULL)
        avr_error ("no iteration func supplied");

    while (node)
    {
        if ((*func) (node->data, user_data))
        {
            /* remove node */
            head = dlist_delete (head, node->data, dlist_iterator_cmp);
        }

        node = node->next;
    }

    return head;
}
