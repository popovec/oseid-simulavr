/*
 * $Id: disp.c,v 1.3 2003/12/01 05:48:35 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr-vcd - A vcd file writer as display process for simulavr.
 * Copyright (C) 2002  Carsten Beth
 *
 * Derivated from:
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2001, 2002  Theodore A. Roth
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

/* WARNING: This code is a hack and needs major improvements. */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "gnu_getopt.h"
#include "vcd.h"

/* *INDENT-OFF* */
enum _constants {
    REG_OFFSET          = 9,
    REG_LABEL_LINE      = 0,
    REG_ROWS            = 4,    /* 4 for Rnn */
    REG_ROW_OFFSET      = 3,    /* 1 for PC, SP, etc; 1 fir X,Y,Z, 1 for
                                   SREG */
    REG_LINES           = REG_ROWS+REG_ROW_OFFSET,
    REG_COLS            = 8,
    IO_REG_LABEL_LINE   = REG_LABEL_LINE + REG_LINES + 1,
    IO_REG_LINES        = 16,   /* 16 lines for io registers */
    IO_REG_COLS         = 4,    /* data displayed with # of cols */
    SRAM_LABEL_LINE     = IO_REG_LABEL_LINE + IO_REG_LINES + 1,
    SRAM_LABEL_ADDR_COL = 14,
    SRAM_GOTO_ADDR_COL  = 40,
    SRAM_COLS           = 16,
    SRAM_COL_OFFSET     = 9,

    MAX_BUF             = 1024,
    MAX_READ_RETRY      = 10,
    MAX_REG             = 32,
    MAX_IO_REG          = 64,
    MAX_IO_REG_BUF      = 64,
};
/* *INDENT-ON* */

struct timespec nap = { 0, 1000000 }; /* 1 ms */

int sig_int = 0;

unsigned int xreg = 0;
unsigned int yreg = 0;
unsigned int zreg = 0;
unsigned int spreg = 0;

union fp_reg
{
    float fval;
    unsigned char ival[4];
};

union fp_reg fp_regs[3];

struct SRAM
{
    int size;                   /* size of array */
    int curr;                   /* curr cursor position */
    int vis_lines;              /* how many lines can we show */
    int vis_beg;                /* first visible address */
    int vis_end;                /* first non-visible address after last
                                   visible */
    unsigned char *data;        /* the data array */
};

struct SRAM sram[1];            /* global sram storage */

/* Convert a hexidecimal digit to a 4 bit nibble. */

int
hex2nib (char hex)
{
    if ((hex >= 'A') && (hex <= 'F'))
        return (10 + (hex - 'A'));

    else if ((hex >= 'a') && (hex <= 'f'))
        return (10 + (hex - 'a'));

    else if ((hex >= '0') && (hex <= '9'))
        return (hex - '0');

    /* Shouldn't get here unless the developer screwed up ;) */
    fprintf (stderr, "Invalid hexidecimal digit: 0x%02x", hex);
    exit (1);

    return 0;                   /* make compiler happy */
}

void
disp_sig_handler (int sig)
{
    switch (sig)
    {
        case SIGINT:
            sig_int++;
            break;
    }
}

/* Wrap read(2) so we can read a byte without having to do a shit load of
   error checking every time. */

char
disp_read_byte (int fd)
{
    char c;
    int res;
    int cnt = MAX_READ_RETRY;

    while (cnt--)
    {
        res = read (fd, &c, 1);
        if (res < 0)
        {
            fprintf (stderr, "read failed: %s", strerror (errno));
            exit (1);
        }

        if (res == 0)
        {
            continue;
        }

        return c;
    }
    fprintf (stderr, "Maximum read reties reached");
    exit (1);

    return 0;                   /* make compiler happy */
}

void
disp_set_sram_values (char *pkt)
{
    unsigned int addr, len;
    unsigned char datum;
    int i;

    if (sscanf (pkt, "%x,%x:", &addr, &len) != 2)
        return;

    while (*pkt != ':')
        pkt++;
    pkt++;                      /* skip over ':' */

    for (i = 0; i < len; i++)
    {
        datum = hex2nib (*pkt++) << 4;
        datum += hex2nib (*pkt++) & 0xf;
        if ((addr + i) >= 0)
            vcd_write_sram (addr + i, datum);
    }
}

/* Move the cursor to another sram address and update the current address in
   the border. Scroll the SRAM window if needed. */

void
disp_usage (char *prog)
{
    fprintf (stderr,
             "Usage: %s [-p |--pfd=nn] flash_size sram_size "
             "sram_start eeprom_size\n", prog);
    exit (1);
}

/* Parse an input packet which has already been decoded. */

int
disp_parse_packet (char *pkt)
{
    int val;
    unsigned int bval;
    char name[50];
    static int first_clock_occured = 0;

    switch (*pkt++)
    {
        case 'q':              /* quit */
            return 1;

        case 'p':              /* set program counter */
            if (sscanf (pkt, "%x", &val) == 1)
                vcd_write_pc (val);
            break;

        case 'n':              /* set time */
            if (sscanf (pkt, "%x", &bval) == 1)
                vcd_set_clock (bval);
            /* if the first clock occures write the vcd header */
            if (!first_clock_occured)
            {
                first_clock_occured = 1;
                vcd_write_header ();
            }
            break;

        case 'r':              /* set register */
            if (sscanf (pkt, "%x:%x", &val, &bval) == 2)
            {
                if ((val >= 14) && (val <= 25))
                {
                    int fp_reg = (val - 14) / 4;
                    int fp_byte = (val - 14) % 4;

                    fp_regs[fp_reg].ival[fp_byte] = (bval & 0xff);

                }
                if ((val >= 26) && (val <= 31))
                {
                    switch (val)
                    {
                        case 26:
                            xreg = (xreg & 0xff00) | (bval & 0xff);
                            break;
                        case 27:
                            xreg =
                                (xreg & 0xff) | ((bval << 8) & 0xff00);
                            break;
                        case 28:
                            yreg = (yreg & 0xff00) | (bval & 0xff);
                            break;
                        case 29:
                            yreg =
                                (yreg & 0xff) | ((bval << 8) & 0xff00);
                            break;
                        case 30:
                            zreg = (zreg & 0xff00) | (bval & 0xff);
                            break;
                        case 31:
                            zreg =
                                (zreg & 0xff) | ((bval << 8) & 0xff00);
                            break;
                    }
                }
                vcd_write_reg (val, bval); /* write (num, value) to vcd-file */
            }
            break;

        case 's':              /* set sram value */
            disp_set_sram_values (pkt);
            break;

        case 'i':              /* set io register value */
            if (sscanf (pkt, "%x:%x", &val, &bval) == 2)
            {
                if (val == 0x3d) /* SPL */
                {
                    spreg = (spreg & 0xff00) | (bval & 0xff);
                    vcd_write_sp (spreg);
                }
                else if (val == 0x3e) /* SPH */
                {
                    spreg = (spreg & 0xff) | ((bval << 8) & 0xff00);
                    vcd_write_sp (spreg);
                }
                else if (val == 0x3f) /* SREG */
                {
                }
                vcd_write_io_reg (val, bval); /* write (adress, value) to
                                                 vcd-file */
            }
            break;

        case 'I':              /* set io register name */
            if (sscanf (pkt, "%x:%s", &val, name) == 2)
                vcd_bind_io_reg_shortcut (name, val);
            break;

        case 'e':              /* set eeprom value: not implemented yet */
            break;

        case 'f':              /* set flash value: not implemented yet */
            break;
    }

    return 0;
}

/* Read an incoming packet. Turns off non-bolcking mode while reading the
   packet data from the pipe and turns it on again when done. */

int
disp_read_packet (int fd)
{
    int i;
    char c;
    char pkt_buf[MAX_BUF + 1];
    int cksum, pkt_cksum;
    int result = 0;

    /* turn non-blocking mode off */
    if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) & ~O_NONBLOCK) < 0)
    {
        fprintf (stderr, "fcntl failed: %s\n", strerror (errno));
        exit (1);
    }

    /* insure that packet is null terminated. */
    memset (pkt_buf, 0, sizeof (pkt_buf));

    pkt_cksum = i = 0;
    c = disp_read_byte (fd);
    while ((c != '#') && (i < MAX_BUF))
    {
        pkt_buf[i++] = c;
        pkt_cksum += (unsigned char)c;
        c = disp_read_byte (fd);
    }

    cksum = hex2nib (disp_read_byte (fd)) << 4;
    cksum |= hex2nib (disp_read_byte (fd));

    if ((pkt_cksum & 0xff) != cksum)
    {
        fprintf (stderr, "Bad checksum: sent 0x%x <--> computed 0x%x", cksum,
                 pkt_cksum);
        sleep (10);
        exit (1);
    }

    /* parse the packet */
    result = disp_parse_packet (pkt_buf);

    /* turn non-blocking mode back on */
    if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK) < 0)
    {
        fprintf (stderr, "fcntl failed: %s\n", strerror (errno));
        exit (1);
    }

    return result;
}

/* See if the parent process (usually simulavr) has written an update request
   to the update pipe. Parent may ask us to quit. Return 1 if we should quit, 
   0 otherwise. */

int
disp_check_update_pipe (int pfd)
{
    int res;
    char c;

    if (pfd < 0)
        return 0;               /* no pipe to read from */

    while (1)
    {
        res = read (pfd, &c, 1);
        if (res < 0)
        {
            if (errno == EAGAIN)
                return 0;       /* no data available */

            fprintf (stderr, "read failed: %s\n", strerror (errno));
            exit (1);
        }
        if (res == 0)
            return 0;

        /* got some data */

        switch (c)
        {
            case '$':
                return disp_read_packet (pfd);
        }
    }

    return 0;
}

/* *INDENT-OFF* */
static struct option long_opts[] = {
    /* name,         has_arg, flag,   val */
    { "help",        0,       0,     'h' },
    { "pfd",         1,       0,     'p' },
    { 0,             0,       0,      0  }
};
/* *INDENT-ON* */

struct ARGS
{
    int flash_size;             /* not used, yet */
    int sram_size;
    int sram_start;
    int eeprom_size;            /* not used, yet */
    int pipe_fd;
};

void
disp_parse_cmd_line (int argc, char **argv, struct ARGS *args)
{
    int c;
    char *prog = argv[0];
    int option_index;
    opterr = 0;                 /* disable default error message */

    /* init the args */
    args->sram_size = 0;
    args->sram_start = 0;
    args->pipe_fd = -1;

    while (1)
    {
        c = getopt_long (argc, argv, "hp:", long_opts, &option_index);
        if (c == -1)
            break;              /* no more options */

        switch (c)
        {
            case 'h':
            case '?':
                disp_usage (prog);
            case 'p':
                args->pipe_fd = atoi (optarg);
                break;
            default:
                fprintf (stderr, "getop() did something screwey");
                exit (1);
        }
    }

    if ((optind + 4) == argc)
    {
        args->flash_size = atoi (argv[optind + 0]);
        args->sram_size = atoi (argv[optind + 1]);
        args->sram_start = atoi (argv[optind + 2]);
        args->eeprom_size = atoi (argv[optind + 3]);
    }
    else
        disp_usage (prog);
}

int
main (int argc, char **argv)
{
    struct ARGS args[1];
    int max_fd;
    int rv;
    fd_set watchset, inset;
    struct timeval to;

    disp_parse_cmd_line (argc, argv, args);

    if (args->pipe_fd >= 0)
    {
        /* make pipe_fd non-blocking */
        if (fcntl
            (args->pipe_fd, F_SETFL,
             fcntl (args->pipe_fd, F_GETFL, 0) | O_NONBLOCK) < 0)
        {
            fprintf (stderr, "fcntl failed: %s\n", strerror (errno));
            exit (1);
        }
    }

    signal (SIGINT, disp_sig_handler);
    signal (SIGWINCH, disp_sig_handler);

    /* setup the select() for multiplexing */

    FD_ZERO (&watchset);
    if (args->pipe_fd >= 0)
        FD_SET (args->pipe_fd, &watchset);
    FD_SET (STDIN_FILENO, &watchset);

    max_fd = STDIN_FILENO > args->pipe_fd ? STDIN_FILENO : args->pipe_fd;

    vcd_init (args->sram_size, args->eeprom_size);

    /* main loop */
    while (FD_ISSET (STDIN_FILENO, &watchset)
           || ((args->pipe_fd >= 0) && FD_ISSET (args->pipe_fd, &watchset)))
    {
        if (sig_int)
        {
            sig_int = 0;
            break;
        }

        to.tv_sec = 0;
        to.tv_usec = 100000;
        inset = watchset;       /* copy watch since select() updates it */
        if ((rv = select (max_fd + 1, &inset, NULL, NULL, &to)) < 0)
        {
            if (errno == EINTR)
                continue;

            fprintf (stderr, "select failed: %s\n", strerror (errno));
            exit (1);
        }

        if (FD_ISSET (STDIN_FILENO, &inset))
        {
            if (args->pipe_fd == -1)
            {
                /* Only enable quiting if not using the pipe so that the user
                   can't quit the display if it's run as a sub-process. */
                break;
            }
        }

        if ((args->pipe_fd >= 0) && FD_ISSET (args->pipe_fd, &inset))
        {
            if (disp_check_update_pipe (args->pipe_fd))
                break;
        }

    }

    return 0;
}
