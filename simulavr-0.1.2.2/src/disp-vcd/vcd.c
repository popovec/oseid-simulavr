/*
 * $Id: vcd.c,v 1.4 2004/03/11 19:02:48 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr-vcd - A vcd file writer as display process for simulavr.
 * Copyright (C) 2002  Carsten Beth
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "vcd.h"

#include "config.h"

int vcd_read_config (char *config_file_name);

/* from config_parser.c */

extern int parse_config (FILE * infile);

/* Definitions for debugging (Add -DENABLE_DEBUG=1 to compile command to
   enable debugging output). */

#ifndef ENABLE_DEBUG
#  define ENABLE_DEBUG 0
#endif

#if ENABLE_DEBUG == 1
#  define DEBUG_FKT_CALL( x ) x;
#else
#  define DEBUG_FKT_CALL( x )
#endif

int frequency = 0;

char *vcd_file_name = NULL;
FILE *vcd_file = NULL;

/* Number of clocks (time = 1/f * clk) */

unsigned int clk = 0;

/* Dump tables are used to store the information which signals has to be
   written to the vcd file.  We define one char for each signal. Each Signal
   with a dump table value != 0 will be written. */

char *io_reg_dump_table;
char *reg_dump_table;
char *sram_dump_table;
char sp_dump_table;
char pc_dump_table;

/* A signal is an entry in the vcd file which has to be traced. */

typedef struct t_signal
{
    char *name;                 /* Visible name */
    char *shortcut;             /* Short name which is written in vcd file */
    int addr;                   /* Address of signal */
    unsigned int width;         /* Width of the traced signal */
    t_signal_type type;         /* vcd type of the traced signal */
    struct t_signal *next;      /* Pointer to the next signal */
} t_signal;

/* There are several kinds of signals. */

t_signal *io_reg_signals = NULL;
t_signal *reg_signals = NULL;
t_signal *sram_signals = NULL;
t_signal *other_signals = NULL;

struct
{
    int sram_size;
    int eeprom_size;
} sizes;

t_signal *
vcd_new_signal (t_signal **signals, char *name, char *shortcut, int addr,
                unsigned int width, t_signal_type type)
{
    t_signal *new;

    /* Allocate memory for new signal */
    new = (t_signal *)malloc (sizeof (t_signal));

    /* If there is a name, make a copy */
    if (name)
    {
        new->name = (char *)malloc (strlen (name));
        strcpy (new->name, name);
    }
    else
        name = NULL;

    /* If there is a shortcut, make a copy */
    if (shortcut)
    {
        new->shortcut = (char *)malloc (strlen (shortcut));
        strcpy (new->shortcut, shortcut);
    }
    else
        shortcut = NULL;

    /* Copy the other values */
    new->addr = addr;
    new->width = width;
    new->type = type;

    /* Link to signal list */
    new->next = *signals;
    *signals = new;

    return (new);
}

void
vcd_free_signals (t_signal **signals)
{
    t_signal *p;

    while (*signals)
    {
        /* Remove from list */
        p = *signals;
        *signals = (*signals)->next;

        /* Free memory */
        if (p->name)
            free (p->name);
        if (p->shortcut)
            free (p->shortcut);
        free (p);
    }
}

/* Close a vcd_file */

void
vcd_close (void)
{
    if (vcd_file && vcd_file != stdout)
    {
        fclose (vcd_file);
        vcd_file = NULL;
    }
}

/* Open a vcd_file by name. If name is "-" stdout will be used to write to. */

int
vcd_open (char *vcd_file_name)
{
    if (!vcd_file_name)
    {
        fprintf (stderr, "unvalid VCD file name: (NULL)\n");
        return (0);
    }

    /* Possibly close an opened vcd file */
    if (vcd_file)
        vcd_close ();

    /* Open a VCD file */
    if (strcmp (vcd_file_name, "-") == 0)
        vcd_file = stdout;
    else
    {
        vcd_file = fopen (vcd_file_name, "w");
        if (!vcd_file)
        {
            fprintf (stderr, "open VCD file failed: %s\n", strerror (errno));
            return (0);
        }
    }

    return (1);
}

/* Close files and free memory. */

void
vcd_exit (void)
{
    vcd_close ();

    if (io_reg_dump_table)
        free (io_reg_dump_table);

    if (reg_dump_table)
        free (reg_dump_table);

    if (sram_dump_table)
        free (sram_dump_table);

    vcd_free_signals (&io_reg_signals);
    vcd_free_signals (&reg_signals);
    vcd_free_signals (&sram_signals);
    vcd_free_signals (&other_signals);

    if (vcd_file_name)
        free (vcd_file_name);
}

/* Initialize dump tables, parse config file and open vcd file. */

int
vcd_init (int sram_size, int eeprom_size)
{
    DEBUG_FKT_CALL (fprintf
                    (stderr, "vcd_init( %i, %i )\n", sram_size, eeprom_size));

    atexit (vcd_exit);

    sizes.sram_size = sram_size;
    sizes.eeprom_size = eeprom_size;

    /* Initialize dump tables */
    io_reg_dump_table = (char *)calloc (64, sizeof (char));
    reg_dump_table = (char *)calloc (32, sizeof (char));
    sram_dump_table = (char *)calloc (sram_size, sizeof (char));
    sp_dump_table = 0;
    pc_dump_table = 0;

    if (!io_reg_dump_table || !reg_dump_table || !sram_dump_table)
    {
        fprintf (stderr, "virtual memory exhausted\n");
        exit (1);
    }

    /* Parse configuration file */
    if (!vcd_read_config ("vcd.cfg"))
        return (0);

    /* Open vcd file */
    if (!vcd_open (vcd_file_name))
        return (0);

    return (1);
}

/***************************************************************/

/* Write the current date and time to the vcd file */

int
vcd_write_time (void)
{
    time_t current_time = time (NULL);

    if (!vcd_file)
        return (0);

    fprintf (vcd_file, "$date\n");
    fprintf (vcd_file, "  %s", ctime (&current_time));
    fprintf (vcd_file, "$end\n");
    fprintf (vcd_file, "\n");

    return (1);
}

/* Write the version to the vcd file */

int
vcd_write_version (void)
{
    if (!vcd_file)
        return (0);

    fprintf (vcd_file, "$version\n");
    fprintf (vcd_file, "  simulavr vcd dumper version %s\n", VCD_VERSION);
    fprintf (vcd_file, "$end\n");
    fprintf (vcd_file, "\n");

    return (1);
}

/* Write the timescale to the vcd file. The timescale is always 1 ns. */

int
vcd_write_timescale (void)
{
    if (!vcd_file)
        return (0);

    fprintf (vcd_file, "$timescale\n");
    fprintf (vcd_file, "  1ns\n");
    fprintf (vcd_file, "$end\n");
    fprintf (vcd_file, "\n");

    return (1);
}

int
vcd_bind_io_reg_shortcut (char *io_reg_name, int io_reg_addr)
{
    t_signal *p;
    char name[100];
    char shortcut[100];

    DEBUG_FKT_CALL (fprintf
                    (stderr, "vcd_bind_io_register_shortcut( %s, %i )\n",
                     io_reg_name, io_reg_addr));

    if (!io_reg_signals)
        return (0);

    /* Generate a name and shortcut */
    snprintf (name, 100 - 1, "%x_%s", io_reg_addr, io_reg_name);
    snprintf (shortcut, 100 - 1, "ioreg%x", io_reg_addr);

    /* Count io_reg_signals to search for matching names or addresses */
    for (p = io_reg_signals; p; p = p->next)
    {
        /* Does the addresses or names match? */
        if ((p->addr == io_reg_addr)
            || (p->name && (strcmp (p->name, io_reg_name) == 0)))
        {
            /* Mark register as used */
            io_reg_dump_table[io_reg_addr] = 1;

            /* Change the signals name */
            if (p->name)
                free (p->name);
            p->name = (char *)malloc (strlen (name));
            strcpy (p->name, name);

            /* Copy the address */
            p->addr = io_reg_addr;

            /* Copy the shortcut */
            if (p->shortcut)
                free (p->shortcut);
            p->shortcut = (char *)malloc (strlen (shortcut));
            strcpy (p->shortcut, shortcut);
        }

    }

    return (1);
}

/* Set the controlers clock frequency. */

void
vcd_set_frequency (int f)
{
    DEBUG_FKT_CALL (fprintf (stderr, "vcd_set_frequency( %i )\n", f));

    frequency = f;
}

/* Set the file name which sould be used for the vcd file. */

void
vcd_set_file_name (char *name)
{
    DEBUG_FKT_CALL (fprintf (stderr, "vcd_set_file_name( %s )\n", name));

    if (vcd_file_name)
        free (vcd_file_name);

    vcd_file_name = name;
}

/* Mark a io-register for tracing. This function can be called either with an
   io_reg_name or an io_reg_addr. */

int
vcd_trace_io_reg (char *io_reg_name, int io_reg_addr)
{
    vcd_new_signal (&io_reg_signals, io_reg_name, NULL, io_reg_addr, 8,
                    ST_REGISTER);

    return 0;                   /* FIXME: why wasn't this returning
                                   anything? */
}

/* Mark register reg_num for tracing. */

int
vcd_trace_reg (int reg_num)
{
    char name[100];
    char shortcut[100];

    /* Generate a name and shortcut */
    snprintf (name, 100 - 1, "REG_%02x", reg_num);
    snprintf (shortcut, 100 - 1, "reg%x", reg_num);

    vcd_new_signal (&reg_signals, name, shortcut, -1, 8, ST_REGISTER);

    /* mark register as used */
    reg_dump_table[reg_num] = 1;

    return 0;                   /* FIXME: why wasn't this returning
                                   anything? */
}

/* Mark sram[sram_addr] for tracing. */

int
vcd_trace_sram (int sram_addr)
{
    char name[30];
    char shortcut[30];

    if ((sram_addr < 0) || (sram_addr > sizes.sram_size - 1))
        return (0);

    /* Generate name and shortcut */
    sprintf (name, "SRAM_%02x", sram_addr);
    sprintf (shortcut, "sram%x", sram_addr);

    vcd_new_signal (&sram_signals, name, shortcut, -1, 8, ST_REGISTER);

    /* mark sram cell as used */
    sram_dump_table[sram_addr] = 1;

    return 0;                   /* FIXME: why wasn't this returning
                                   anything? */
}

/* Mark stack pointer for tracing. */

int
vcd_trace_sp (void)
{
    vcd_new_signal (&other_signals, "SP", "sp", -1, 16, ST_REGISTER);

    /* mark sp cell as used */
    sp_dump_table = 1;

    return 0;                   /* FIXME: why wasn't this returning
                                   anything? */
}

/* Mark program counter for tracing. */

int
vcd_trace_pc (void)
{
    vcd_new_signal (&other_signals, "PC", "pc", -1, 16, ST_REGISTER);

    /* mark pc cell as used */
    pc_dump_table = 1;

    return 0;                   /* FIXME: why wasn't this returning
                                   anything? */
}

/* Read an parse the config file. */

int
vcd_read_config (char *config_file_name)
{
    FILE *config_file;

    DEBUG_FKT_CALL (fprintf
                    (stderr, "vcd_read_config( %s )\n", config_file_name));

    config_file = fopen (config_file_name, "r");
    if (!config_file)
    {
        fprintf (stderr, "open config file failed: %s\n", strerror (errno));
        return (0);
    }

    if (!parse_config (config_file))
    {
        fprintf (stderr, "error while reading config file\n");
        fclose (config_file);
        return (0);
    }

    fclose (config_file);
    return (1);
}

/* Write signal declarations to the vcd file. */

void
vcd_write_signals (t_signal *signals, char *module_name)
{
    if (signals)
    {
        fprintf (vcd_file, "$scope module %s $end\n", module_name);

        for (; signals; signals = signals->next)
            if (signals->shortcut)
                fprintf (vcd_file, "$var reg       %i %s    %s   $end\n",
                         signals->width, signals->shortcut, signals->name);

        fprintf (vcd_file, "$upscope $end\n");
        fprintf (vcd_file, "\n");
    }
}

/* Write the header to the vcd file. */

int
vcd_write_header (void)
{
    DEBUG_FKT_CALL (fprintf (stderr, "vcd_write_header()\n"));

    if (!vcd_file)
        return (0);

    /* Rewind file */
    fseek (vcd_file, 0, SEEK_SET);

    vcd_write_time ();
    vcd_write_version ();
    vcd_write_timescale ();

    vcd_write_signals (io_reg_signals, "io_register");
    vcd_write_signals (reg_signals, "register");
    vcd_write_signals (sram_signals, "sram");
    vcd_write_signals (other_signals, "other");

    fprintf (vcd_file, "$enddefinitions $end\n");
    fprintf (vcd_file, "\n");

    /* Go back to the end of the vcd-file */
    fseek (vcd_file, 0, SEEK_END);

    return (1);
}

/* Write a 8 bit binary value to the vcd file. */

void
vcd_write_bit8 (unsigned char val)
{
    int i;

    fputc ('b', vcd_file);      /* write binary signature */
    for (i = 0x80; i > 0; i /= 2) /* shift bit in i for masking */
        fputc (((val & i) != 0) + '0', vcd_file); /* write masked bit */
}

/* Write a 16 bit binary value to the vcd file. */

void
vcd_write_bit16 (unsigned short val)
{
    int i;

    fputc ('b', vcd_file);      /* write binary signature */
    for (i = 0x8000; i > 0; i /= 2) /* shift bit in i for masking */
        fputc (((val & i) != 0) + '0', vcd_file); /* write masked bit */
}

/* Write io-register to the vcd file. */

int
vcd_write_io_reg (int io_reg_addr, unsigned char val)
{
    DEBUG_FKT_CALL (fprintf
                    (stderr, "io_vcd_write_reg( %i, %i )\n", io_reg_addr,
                     val));

    if (!vcd_file)
        return (0);

    if (!io_reg_dump_table[io_reg_addr])
        return (0);

    vcd_write_clock ();
    vcd_write_bit8 (val);
    fprintf (vcd_file, " ioreg%x\n", io_reg_addr); /* write identifier */

    fflush (vcd_file);          /* for debug only */

    return (1);
}

/* Write register to the vcd file. */

int
vcd_write_reg (int reg_num, unsigned char val)
{
    DEBUG_FKT_CALL (fprintf
                    (stderr, "vcd_write_reg( %i, %i )\n", reg_num, val));

    if (!vcd_file)
        return (0);

    if (!reg_dump_table[reg_num])
        return (0);

    vcd_write_clock ();
    vcd_write_bit8 (val);
    fprintf (vcd_file, " reg%x\n", reg_num); /* write identifier */

    fflush (vcd_file);          /* for debug only */

    return (1);
}

/* Write sram to the vcd file. */

int
vcd_write_sram (int sram_addr, unsigned char val)
{
    DEBUG_FKT_CALL (fprintf
                    (stderr, "vcd_write_sram( %i, %i )\n", sram_addr, val));

    if (!vcd_file)
        return (0);

    if ((sram_addr < 0) || (sram_addr > sizes.sram_size - 1))
        return (0);

    if (!sram_dump_table[sram_addr])
        return (0);

    vcd_write_clock ();
    vcd_write_bit8 (val);
    fprintf (vcd_file, " sram%x\n", sram_addr); /* write identifier */

    fflush (vcd_file);          /* for debug only */

    return (1);
}

/* Write stack pointer to the vcd file. */

int
vcd_write_sp (int sp)
{
    static unsigned int written_sp = -1;

    DEBUG_FKT_CALL (fprintf (stderr, "vcd_write_sp( %i )\n", sp));

    if (!vcd_file)
        return (0);

    if (!sp_dump_table || (sp == written_sp))
        return (1);

    written_sp = sp;

    vcd_write_clock ();
    vcd_write_bit16 (sp);
    fprintf (vcd_file, " sp\n"); /* write identifier */

    fflush (vcd_file);          /* for debug only */

    return (1);
}

/* Write program counter to the vcd file. */

int
vcd_write_pc (int pc)
{
    DEBUG_FKT_CALL (fprintf (stderr, "vcd_write_pc( %i )\n", pc));

    if (!vcd_file)
        return (0);

    if (!pc_dump_table)
        return (0);

    vcd_write_clock ();
    vcd_write_bit16 (pc);
    fprintf (vcd_file, " pc\n"); /* write identifier */

    fflush (vcd_file);          /* for debug only */

    return (1);
}

/* Set the current time. */

int
vcd_set_clock (unsigned int c)
{
    clk = c;

    return 0;                   /* FIXME: why wasn't this returning
                                   anything? */
}

/* Write current time to the vcd file. */

int
vcd_write_clock (void)
{
    static unsigned int written_clk = -1;
    static unsigned int factor = 1e9;

    if (!vcd_file)
        return (0);

    /* Have we already written the current time? */
    if (clk == written_clk)
        return (1);
    written_clk = clk;

    /* Write the current time */
    fprintf (vcd_file, "#%i\n", clk * (factor / frequency));

    fflush (vcd_file);          /* for debug only */

    return (1);
}
