/*
 * $Id: display.c,v 1.14 2003/12/01 09:10:14 troth Exp $
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
 * \file display.c
 * \brief Interface for using display coprocesses.
 *
 * Simulavr has the ability to use a coprocess to display register and memory
 * values in near real time.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "display.h"

enum
{
    MAX_BUF = 1024,
};

/* I really don't want to use a global here, but I also don't want to have to
   track the pipe's fd in the core. */

static int global_pipe_fd = -1;

/* Need to store the child's pid so that we can kill and waitpid it when you
   close the display. Otherwise we have problems with zombies. */

static pid_t global_child_pid = -1;

/** \brief Open a display as a coprocess.
    \param prog        The program to use as a display coprocess.
    \param no_xterm    If non-zero, don't run the disply in an xterm.
    \param flash_sz    The size of the flash memory space in bytes.
    \param sram_sz     The size of the sram memory space in bytes.
    \param sram_start  The addr of the first byte of sram (usually 0x60 or
                       0x100).
    \param eeprom_sz   The size of the eeprom memory space in bytes.

    Try to start up a helper program as a child process for displaying
    registers and memory. If the prog argument is NULL, don't start up a
    display.

    Returns an open file descriptor of a pipe used to send data to
    the helper program. 
    
    Returns -1 if something failed. */

int
display_open (char *prog, int no_xterm, int flash_sz, int sram_sz,
              int sram_start, int eeprom_sz)
{
    pid_t pid;
    int pfd[2];                 /* pipe file desc: pfd[0] is read, pfd[1] is
                                   write */
    int res;

    if (prog == NULL)
    {
        prog = getenv ("SIM_DISP_PROG");
        if (prog == NULL)
            return -1;
    }

    /* Open a pipe for writing from the simulator to the display program. 
       We don't want to use popen() since the display program might need to 
       use stdin/stdout for it's own uses. */

    res = pipe (pfd);
    if (res < 0)
    {
        avr_warning ("pipe failed: %s\n", strerror (errno));
        return -1;
    }

    /* Fork off a new process. */

    pid = fork ();
    if (pid < 0)
    {
        avr_warning ("fork failed: %s\n", strerror (errno));
        return -1;
    }
    else if (pid > 0)           /* parent process */
    {
        /* close the read side of the pipe */
        close (pfd[0]);

        /* remember the child's pid */
        global_child_pid = pid;

        global_pipe_fd = pfd[1];
        return global_pipe_fd;
    }
    else                        /* child process */
    {
        char pfd_env[20];
        char fl_sz[20], sr_sz[20], sr_start[20], eep_sz[20];
        char spfd[10];

        /* close the write side of the pipe */
        close (pfd[1]);

        /* setup the args for display program */
        snprintf (fl_sz, sizeof (fl_sz) - 1, "%d", flash_sz);
        snprintf (sr_sz, sizeof (sr_sz) - 1, "%d", sram_sz);
        snprintf (sr_start, sizeof (sr_start) - 1, "%d", sram_start);
        snprintf (eep_sz, sizeof (eep_sz) - 1, "%d", eeprom_sz);
        snprintf (spfd, sizeof (spfd) - 1, "%d", pfd[0]);

        /* set the SIM_PIPE_FD env variable */
        snprintf (pfd_env, sizeof (pfd_env), "SIM_PIPE_FD=%d", pfd[0]);
        putenv (pfd_env);

        /* The user can specify not to use an xterm since some display
           programs might not need (or want) to be run in an xterm. For
           example, a gtk+ program would be able to handle it's own
           windowing. Of course, starting 'prog' up with it's own xterm, will
           not hurt and 'prog' will put stdout/stderr there instead of mixing
           with simulavr's output. The default is to start prog in an
           xterm. */

        if (no_xterm)
        {
            execlp (prog, prog, "--pfd", spfd, fl_sz, sr_sz, sr_start, eep_sz,
                    NULL);
        }
        else
        {
            /* try to start up the display program in it's own xterm */
            execlp ("xterm", "xterm", "-geom", "100x50", "-e", prog, "--pfd",
                    spfd, fl_sz, sr_sz, sr_start, eep_sz, NULL);
        }

        /* if the exec returns, an error occurred */
        avr_warning ("exec failed: %s\n", strerror (errno));
        _exit (1);
    }

    return -1;                  /* should never get here */
}

/** \brief Close a display and send coprocess a quit message. */

void
display_close (void)
{
    if (global_pipe_fd < 0)
        return;

    display_send_msg ("q");
    close (global_pipe_fd);
    global_pipe_fd = -1;

    kill (global_child_pid, SIGINT);
    waitpid (0, NULL, 0);
}

static unsigned char
checksum (char *s)
{
    unsigned char CC = 0;
    while (*s)
    {
        CC += *s;
        s++;
    }

    return CC;
}

/** \brief Encode the message and send to display.
    \param msg   The message string to be sent to the display process.

    Encoding is the same as that used by the gdb remote protocol: '\$...\#CC'
    where '...' is msg, CC is checksum. There is no newline termination for
    encoded messages.

    FIXME: TRoth: This should be a private function. It is only public so that
    dtest.c can be kept simple. dtest.c should be changed to avoid direct use
    of this function. [dtest.c has served it's purpose and will be retired
    soon.] */

void
display_send_msg (char *msg)
{
    int len = strlen (msg) + 4 + 1;
    int res;
    char *enc_msg;              /* the encoded msg */

    enc_msg = avr_new0 (char, len + 1);

    snprintf (enc_msg, len, "$%s#%02x", msg, checksum (msg));
#if defined(DISP_DEBUG_OUTPUT_ON)
    fprintf (stderr, "DISP: %s\n", enc_msg);
#endif

    res = write (global_pipe_fd, enc_msg, len);
    if ((res < 0) && (errno == EINTR))
    {
        /* write() was interrupted, try again and if it still fails, let it be
           fatal. */
        avr_warning ("Interrupted write()\n");
        res = write (global_pipe_fd, enc_msg, len);
    }
    if (res < 0)
        avr_error ("write failed: %s\n", strerror (errno));
    if (res < len)
        avr_error ("incomplete write\n");

    avr_free (enc_msg);
}

static char global_buf[MAX_BUF + 1];

/** \brief Update the time in the display.
    \param clock   The new time in number of clocks. */

void
display_clock (int clock)
{
    if (global_pipe_fd < 0)
        return;

    snprintf (global_buf, MAX_BUF, "n%x", clock);
    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Update the Program Counter in the display.
    \param val   The new value of the program counter. */

void
display_pc (int val)
{
    if (global_pipe_fd < 0)
        return;

    snprintf (global_buf, MAX_BUF, "p%x", val);
    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Update a register in the display.
    \param reg   The register number.
    \param val   The new value of the register. */

void
display_reg (int reg, uint8_t val)
{
    if (global_pipe_fd < 0)
        return;

    snprintf (global_buf, MAX_BUF, "r%x:%02x", reg, val);
    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Update an IO register in the display.
    \param reg   The IO register number.
    \param val   The new value of the register. */

void
display_io_reg (int reg, uint8_t val)
{
    if (global_pipe_fd < 0)
        return;

    snprintf (global_buf, MAX_BUF, "i%x:%02x", reg, val);
    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Specify a name for an IO register.
    \param reg    The IO register number.
    \param name   The symbolic name of the register.

    Names of IO registers may be different from device to device. */

void
display_io_reg_name (int reg, char *name)
{
    if (global_pipe_fd < 0)
        return;

    snprintf (global_buf, MAX_BUF, "I%x:%s", reg, name);
    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Update a block of flash addresses in the display.
    \param addr  Address of beginning of the block.
    \param len   Length of the block (number of words).
    \param vals  Pointer to an array of \a len words.

    The display will update each addr of the block to the coresponding value
    in the \a vals array.

    Each address in the flash references a single 16-bit wide word (or opcode
    or instruction). Therefore, flash addresses are aligned to 16-bit
    boundaries. It is simplest to consider the flash an array of 16-bit values
    indexed by the address. */

void
display_flash (int addr, int len, uint16_t * vals)
{
    int bytes;
    int i;

    if (global_pipe_fd < 0)
        return;

    bytes = snprintf (global_buf, MAX_BUF, "f%x,%x:", addr, len);

    for (i = 0; i < len; i++)
    {
        if (MAX_BUF - bytes < 0)
            avr_error ("buffer overflow");

        bytes +=
            snprintf (global_buf + bytes, MAX_BUF - bytes, "%04x", vals[i]);
    }

    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Update a block of sram addresses in the display.
    \param addr  Address of beginning of the block.
    \param len   Length of the block (number of bytes).
    \param vals  Pointer to an array of \a len bytes.

    The display will update each addr of the block to the coresponding value
    in the \a vals array. */

void
display_sram (int addr, int len, uint8_t * vals)
{
    int bytes;
    int i;

    if (global_pipe_fd < 0)
        return;

    bytes = snprintf (global_buf, MAX_BUF, "s%x,%x:", addr, len);

    for (i = 0; i < len; i++)
    {
        if (MAX_BUF - bytes < 0)
            avr_error ("buffer overflow");

        bytes +=
            snprintf (global_buf + bytes, MAX_BUF - bytes, "%02x", vals[i]);
    }

    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}

/** \brief Update a block of eeprom addresses in the display.
    \param addr  Address of beginning of the block.
    \param len   Length of the block (number of bytes).
    \param vals  Pointer to an array of \a len bytes.

    The display will update each addr of the block to the coresponding value
    in the \a vals array. */

void
display_eeprom (int addr, int len, uint8_t * vals)
{
    int bytes;
    int i;

    if (global_pipe_fd < 0)
        return;

    bytes = snprintf (global_buf, MAX_BUF, "e%x,%x:", addr, len);

    for (i = 0; i < len; i++)
    {
        if (MAX_BUF - bytes < 0)
            avr_error ("buffer overflow");

        bytes +=
            snprintf (global_buf + bytes, MAX_BUF - bytes, "%02x", vals[i]);
    }

    global_buf[MAX_BUF] = '\0';
    display_send_msg (global_buf);
}
