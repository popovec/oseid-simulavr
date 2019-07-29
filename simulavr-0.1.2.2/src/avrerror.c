/*
 * $Id: avrerror.c,v 1.8 2004/01/30 07:09:56 troth Exp $
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

/**
   \file avrerror.c
   \brief Functions for printing messages, warnings and errors.

   This module provides output printing facilities. */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "avrerror.h"

#if MACRO_DOCUMENTATION

/** \brief Print an ordinary message to stdout. */
#define avr_message(fmt, args...) \
    private_avr_message(__FILE__, __LINE__, fmt, ## args)

/** \brief Print a warning message to stderr. */
#define avr_warning(fmt, args...) \
    private_avr_warning(__FILE__, __LINE__, fmt, ## args)

/** \brief Print an error message to stderr and terminate program. */
#define avr_error(fmt, args...) \
    private_avr_error(__FILE__, __LINE__, fmt, ## args)

#else /* Not Documentation */

#if 1
static char *
strip_dir (char *path)
{
    char *p = path;

    /* Find the end. */

    while (*p++)
        ;

    /* Find the last '/'. */

    while (p != path)
    {
        if (*p == '/')
        {
            p++;
            break;
        }

        p--;
    }

    return p;
}
#else
#  define strip_dir(path) (path)
#endif

#define FLUSH_OUTPUT 1

void
private_avr_message (char *file, int line, char *fmt, ...)
{
    va_list ap;
    char ffmt[128];

    snprintf (ffmt, sizeof (ffmt), "%s:%d: MESSAGE: %s", strip_dir (file),
              line, fmt);
    ffmt[127] = '\0';

    va_start (ap, fmt);
    vfprintf (stdout, ffmt, ap);
    va_end (ap);

#if defined (FLUSH_OUTPUT)
    fflush (stdout);
#endif
}

void
private_avr_warning (char *file, int line, char *fmt, ...)
{
    va_list ap;
    char ffmt[128];

    snprintf (ffmt, sizeof (ffmt), "%s:%d: WARNING: %s", strip_dir (file),
              line, fmt);
    ffmt[127] = '\0';

    va_start (ap, fmt);
    vfprintf (stderr, ffmt, ap);
    va_end (ap);

#if defined (FLUSH_OUTPUT)
    fflush (stderr);
#endif
}

void
private_avr_error (char *file, int line, char *fmt, ...)
{
    va_list ap;
    char ffmt[128];

    snprintf (ffmt, sizeof (ffmt), "\n%s:%d: ERROR: %s\n\n", strip_dir (file),
              line, fmt);
    ffmt[127] = '\0';

    va_start (ap, fmt);
    vfprintf (stderr, ffmt, ap);
    va_end (ap);

#if defined (FLUSH_OUTPUT)
    fflush (stderr);
#endif

    exit (1);                   /* exit instead of abort */
}

#endif /* Not documenation */
