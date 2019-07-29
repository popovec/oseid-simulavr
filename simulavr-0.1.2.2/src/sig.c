/*
 * $Id: sig.c,v 1.6 2003/12/01 09:10:16 troth Exp $
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
 * \file sig.c
 * \brief Public interface to signal handlers.
 *
 * This module provides a way for the simulator to process signals generated
 * by the native host system. Note that these signals in this context have
 * nothing to do with signals or interrupts as far as a program running in the
 * simulator is concerned. */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "avrerror.h"
#include "sig.h"

static volatile int global_got_sigint = 0;

/*
 * Private.
 *
 * Handler for SIGINT signals. 
 */
static void
signal_handle_sigint (int signo)
{
    global_got_sigint = 1;
}

/**
 * \brief Start watching for the occurrance of the given signal.
 *
 * This function will install a signal handler which will set a flag when the
 * signal occurs. Once the watch has been started, periodically call
 * signal_has_occurred() to check if the signal was raised. 
 */
void
signal_watch_start (int signo)
{
    struct sigaction act, oact;

    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;

    switch (signo)
    {
        case SIGINT:
            global_got_sigint = 0;
            act.sa_handler = signal_handle_sigint;
            break;
        default:
            avr_warning ("Invalid signal: %d\n", signo);
            return;
    }

    if (sigaction (signo, &act, &oact) < 0)
        avr_warning ("Failed to install signal handler: sig=%d: %s\n", signo,
                     strerror (errno));
}

/**
 * \brief Stop watching signal.
 *
 * Restores the default signal handler for the given signal and resets the
 * signal flag. 
 */
void
signal_watch_stop (int signo)
{
    struct sigaction act, oact;

    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = SIG_DFL;

    signal_reset (signo);

    if (sigaction (signo, &act, &oact) < 0)
        avr_warning ("Failed to restore default signal handler: sig=%d: %s\n",
                     signo, strerror (errno));
}

/**
 * \brief Check to see if a signal has occurred.
 *
 * \return Non-zero if signal has occurred. The flag will always be reset
 * automatically. 
 */
int
signal_has_occurred (int signo)
{
    int res = 0;

    switch (signo)
    {
        case SIGINT:
            res = global_got_sigint;
            global_got_sigint = 0;
            break;
        default:
            avr_warning ("Invalid signal: %d", signo);
    }

    return res;
}

/**
 * \brief Clear the flag which indicates that a signal has ocurred.
 *
 * Use signal_reset to manually reset (i.e. clear) the flag.
 */
void
signal_reset (int signo)
{
    signal_has_occurred (signo);
}
