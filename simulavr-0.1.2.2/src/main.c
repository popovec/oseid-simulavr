/*
 * $Id: main.c,v 1.40 2004/04/17 00:03:51 troth Exp $
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "avrclass.h"
#include "utils.h"
#include "callback.h"
#include "op_names.h"

#include "storage.h"
#include "flash.h"

#include "vdevs.h"
#include "memory.h"
#include "stack.h"
#include "register.h"
#include "sram.h"
#include "eeprom.h"
#include "timers.h"
#include "ports.h"

#include "avrcore.h"

#include "devsupp.h"
#include "display.h"

#include "gdb.h"
#include "gnu_getopt.h"

/****************************************************************************\
 *
 * global variables (keep them to a minimum)
 *
\****************************************************************************/

static char *global_device_type = NULL;

static int global_eeprom_image_type = FFMT_BIN;
static char *global_eeprom_image_file = NULL;

static int global_flash_image_type = FFMT_BIN;
static char *global_flash_image_file = NULL;

static int global_gdbserver_mode = 0;
static int global_gdbserver_port = 1212; /* default port number */
static int global_gdb_debug = 0;

static char *global_disp_prog = NULL;
static int global_disp_without_xterm = 0;

static int global_dump_core = 0;

static int global_clock_freq = 8000000; /* Default is 8 MHz. */

/* If the user needs more than LEN_BREAK_LIST on the command line, they've got
   bigger problems. */

#define LEN_BREAK_LIST  50
static int global_break_count = 0;
static int global_break_list[LEN_BREAK_LIST];

static AvrCore *global_core = NULL;

/* *INDENT-OFF* */
static GdbComm_T global_gdb_comm[1] = {{
    .user_data = NULL,          /* user_data: will be global_core later */
    
    .read_reg = (CommFuncReadReg) avr_core_gpwr_get,
    .write_reg = (CommFuncWriteReg) avr_core_gpwr_set,
    
    .read_sreg = (CommFuncReadSREG) avr_core_sreg_get,
    .write_sreg = (CommFuncWriteSREG) avr_core_sreg_set,
    
    .read_pc = (CommFuncReadPC) avr_core_PC_get,
    .write_pc = (CommFuncWritePC) avr_core_PC_set,
    .max_pc = (CommFuncMaxPC) avr_core_PC_max,
    
    .read_sram = (CommFuncReadSRAM) avr_core_mem_read,
    .write_sram = (CommFuncWriteSRAM) avr_core_mem_write,
    
    .read_flash = (CommFuncReadFlash) avr_core_flash_read,
    .write_flash = (CommFuncWriteFlash) avr_core_flash_write,
    .write_flash_lo8 = (CommFuncWriteFlashLo8) avr_core_flash_write_lo8,
    .write_flash_hi8 = (CommFuncWriteFlashHi8) avr_core_flash_write_hi8,
    
    .insert_break = (CommFuncInsertBreak) avr_core_insert_breakpoint,
    .remove_break = (CommFuncRemoveBreak) avr_core_remove_breakpoint,
    .enable_breakpts = (CommFuncEnableBrkpts) avr_core_enable_breakpoints,
    .disable_breakpts = (CommFuncDisableBrkpts) avr_core_disable_breakpoints,
    
    .step = (CommFuncStep) avr_core_step,
    .reset = (CommFuncReset) avr_core_reset,
    
    .io_fetch = (CommFuncIORegFetch) avr_core_io_fetch,
    
    .irq_raise = (CommFuncIrqRaise) avr_core_irq_raise,
}};

static char *usage_fmt_str =
"\nUsage: %s [OPTIONS]... [flash_image]\n" "\n"
"Simulate an avr device. The optional flash_image file is loaded\n"
"into the flash program memory space of the device.\n" "\n" "Options:\n"
"  -h, --help                : Show this message\n"
"  -D, --debug               : Debug instruction output\n"
"  -v, --version             : Print out the version number and exit\n"
"  -g, --gdbserver           : Run as a gdbserver process\n"
"  -G, --gdb-debug           : Print out debug messages for gdbserver\n"
"  -p, --port <port>         : Listen for gdb connection on TCP port\n"
"  -d, --device <dev>        : Specify device type\n"
"  -e, --eeprom-image <img>  : Specify an eeprom image file\n"
"  -E, --eeprom-type <type>  : Specify the type of the eeprom image file\n"
"  -F, --flash-type <type>   : Specify the type of the flash image file\n"
"  -L, --list-devices        : Print supported devices to stdout and exit\n"
"  -P, --disp-prog <prog>    : Display register and memory info with prog\n"
"  -X, --without-xterm       : Don't start disp prog in an xterm\n"
"  -C, --core-dump           : Dump a core memory image to file on exit\n"
"  -c, --clock-freq <freq>   : Set the simulated mcu clock freqency (in Hz)\n"
"  -B, --breakpoint <addr>   : Set a breakpoint (address is a byte address)\n"
"\n" "If the image file types for eeprom or flash images are not given,\n"
"the default file type is binary.\n" "\n"
"If you wish to run the simulator in gdbserver mode, you do not\n"
"have to specify a flash-image file since the program can be loaded\n"
"from gdb via the `load` command.\n" "\n"
"If '--port' option is given, and '--gdbserver' is not, port is ignored\n"
"\n" "If running in gdbserver mode and port is not specified, a default\n"
"port of 1212 is used.\n" "\n"
"If using the '--breakpoint' option, note the simulator will terminate when\n"
"the address is hit if you are not running in gdbserver mode. This feature\n"
"not intended for use in gdbserver mode. It is really intended for testing\n"
"the simulator itself, but may be useful for testing avr programs too.\n"
"\n" "Currently available device types:\n";

/* *INDENT-ON* */

/*
 * Print usage message.
 */
static void
usage (char *prog)
{
    fprintf (stdout, usage_fmt_str, prog);
    dev_supp_list_devices (stdout);
    fprintf (stdout, "\n");

    exit (1);
}

/* *INDENT-OFF* */
static struct option long_opts[] = {
    /* name,             has_arg, flag,   val */
    { "help",            0,       0,     'h' },
    { "debug",           0,       0,     'D' },
    { "version",         0,       0,     'v' },
    { "gdbserver",       0,       0,     'g' },
    { "gdb-debug",       0,       0,     'G' },
    { "port",            1,       0,     'p' },
    { "device",          1,       0,     'd' },
    { "eeprom-type",     1,       0,     'E' },
    { "eeprom-image",    1,       0,     'e' },
    { "flash-type",      1,       0,     'F' },
    { "list-devices",    0,       0,     'L' },
    { "disp-prog",       1,       0,     'P' },
    { "without-xterm",   1,       0,     'X' },
    { "core-dump",       0,       0,     'C' },
    { "clock-freq",      1,       0,     'c' },
    { "breakpoint",      1,       0,     'B' },
    { NULL,              0,       0,      0  }
};
/* *INDENT-ON* */

/*
 * Parse the command line arguments.
 */
static void
parse_cmd_line (int argc, char **argv)
{
    int c;
    char *prog = argv[0];
    char *basename;
    int option_index;
    char dummy_char;
    int break_addr;

    opterr = 0;                 /* disable default error message */

    while (1)
    {
        c = getopt_long (argc, argv, "hgGvDLd:e:E:F:p:P:XCc:B:", long_opts,
                         &option_index);
        if (c == -1)
            break;              /* no more options */

        switch (c)
        {
            case 'h':
            case '?':
                usage (prog);
            case 'g':
                global_gdbserver_mode = 1;
                break;
            case 'G':
                global_gdb_debug = 1;
                break;
            case 'p':
                global_gdbserver_port = atoi (optarg);
                break;
            case 'v':
                printf ("\n%s version %s\n", PACKAGE, VERSION);
                printf ("Copyright 2001, 2002, 2003, 2004"
                        "  Theodore A. Roth.\n");
                printf ("\n%s is free software, covered by the GNU General "
                        "Public License,\n", PACKAGE);
                printf ("and you are welcome to change it and/or distribute "
                        "copies of it under\n");
                printf ("the conditions of the GNU General Public License."
                        "\n\n");
                exit (0);
            case 'D':
                global_debug_inst_output = 1;
                break;
            case 'd':
                global_device_type = optarg;
                break;
            case 'e':
                global_eeprom_image_file = avr_strdup (optarg);
                break;
            case 'E':
                break;
                global_eeprom_image_type = str2ffmt (optarg);
            case 'F':
                global_flash_image_type = str2ffmt (optarg);
                break;
            case 'L':
                dev_supp_list_devices (stdout);
                exit (0);
            case 'P':
                global_disp_prog = avr_strdup (optarg);
                break;
            case 'X':
                global_disp_without_xterm = 1;
                break;
            case 'C':
                global_dump_core = 1;
                break;
            case 'c':
                if (sscanf (optarg, "%d%c", &global_clock_freq, &dummy_char)
                    != 1)
                {
                    avr_error ("Invalid clock value: %s", optarg);
                }
                avr_warning ("Clock frequency option is not yet "
                             "implemented.\n");
                break;
            case 'B':
                if (sscanf (optarg, "%i%c", &break_addr, &dummy_char) != 1)
                {
                    avr_error ("Ignoring invalid break addres: %s", optarg);
                }

                if (global_break_count < LEN_BREAK_LIST)
                {
                    global_break_list[global_break_count] = break_addr;
                    global_break_count++;
                }
                else
                {
                    avr_warning ("Too many break points: igoring %s\n",
                                 optarg);
                }

                break;
            default:
                avr_error ("getop() did something screwey");
        }
    }

    if ((optind + 1) == argc)
        global_flash_image_file = argv[optind];
    else if (optind != argc)
        usage (prog);

    /* FIXME: Issue a warning and bail out if user selects a file format type
       we haven't implemented yet. */

    if ((global_eeprom_image_type != FFMT_BIN)
        || (global_flash_image_type != FFMT_BIN))
    {
        fprintf (stderr,
                 "Only the bin file format is currently "
                 "implemented. Sorry.\n");
        exit (1);
    }

    /* If user didn't specify a device type, see if it can be gleaned from the
       name of the program. */

    if (global_device_type == NULL)
    {
        /* find the last '/' in dev_name */
        basename = strrchr (prog, '/');
        if (basename == NULL)
            /* no slash in dev_name */
            global_device_type = prog;
        else
            global_device_type = ++basename;
    }
}

uint8_t
ext_port_rd (int addr)
{
    int data;
    char line[80];

    while (1)
    {
        fprintf (stderr, "\nEnter a byte of data to read into 0x%04x: ",
                 addr);

        /* try to read in a line of input */
        if (fgets (line, sizeof (line), stdin) == NULL)
            continue;

        /* try to parse the line for a byte of data */
        if (sscanf (line, "%i\n", &data) != 1)
            continue;

        break;
    }
    return (uint8_t) (data & 0xff);
}

void
ext_port_wr (int addr, uint8_t val)
{
    fprintf (stderr, "writing 0x%02x to 0x%04x\n", val, addr);
    fflush (stderr);
}

/* This is called whenever the program terminates via a call to exit(). */

void
atexit_cleanup (void)
{
    FILE *dump;

    if (global_dump_core)
    {
        if ((dump = fopen ("core_avr_dump.core", "w")) == NULL)
        {
            /* can't call avr_error here since it could have called us */
            fprintf (stderr, "fopen failed: core_avr_dump.core: %s\n",
                     strerror (errno));
        }
        else
        {
            avr_core_dump_core (global_core, dump);
            fclose (dump);
        }
    }

    class_unref ((AvrClass *)global_core);
}

/*
 * Symlinks should be created for each supported device to the
 * simulavr program.
 */
int
main (int argc, char **argv)
{
    int i;
    int flash_sz = 0, sram_sz = 0, eeprom_sz = 0;
    int sram_start = 0;

    parse_cmd_line (argc, argv);

    global_core = avr_core_new (global_device_type);
    if (global_core == NULL)
    {
        avr_warning ("Device not supported: %s\n", global_device_type);
        exit (1);
    }

    avr_message ("Simulating clock frequency of %d Hz\n", global_clock_freq);

    avr_core_get_sizes (global_core, &flash_sz, &sram_sz, &sram_start,
                        &eeprom_sz);
    display_open (global_disp_prog, global_disp_without_xterm, flash_sz,
                  sram_sz, sram_start, eeprom_sz);
    avr_core_io_display_names (global_core);

    /* Send initial clock cycles to display */
    display_clock (0);

    /* install my_atexit to be called when exit() is called */
    if (atexit (atexit_cleanup))
        avr_error ("Failed to install exit handler");

#if 0
    /* Add external device hooks to ports */
    avr_core_add_ext_rd_wr (global_core, PORT_B_BASE, ext_port_rd,
                            ext_port_wr);
    avr_core_add_ext_rd_wr (global_core, PORT_C_BASE, ext_port_rd,
                            ext_port_wr);
    avr_core_add_ext_rd_wr (global_core, PORT_D_BASE, ext_port_rd,
                            ext_port_wr);
#endif

    /* Load program into flash */
    if (global_flash_image_file)
        avr_core_load_program (global_core, global_flash_image_file,
                               global_flash_image_type);

    /* Load eeprom data image into eeprom */
    if (global_eeprom_image_file)
        avr_core_load_eeprom (global_core, global_eeprom_image_file,
                              global_eeprom_image_type);

    for (i = 0; i < global_break_count; i++)
    {
        /* Note that we interpret the break address from the user as a byte
           address instead of a word address. This makes it easier on the user
           since binutils, gcc and gdb all work in terms of byte addresses. */

        avr_message ("Setting breakpoint at 0x%x.\n", global_break_list[i]);
        avr_core_insert_breakpoint (global_core, global_break_list[i] / 2);
    }

    if (global_gdbserver_mode == 1)
    {
        global_gdb_comm->user_data = global_core;
        gdb_interact (global_gdb_comm, global_gdbserver_port,
                      global_gdb_debug);
    }
    else
    {
        if (global_flash_image_file)
            /* Run the program */
            avr_core_run (global_core);
        else
            fprintf (stderr, "No program was specified to be run.\n");
    }

    display_close ();           /* close down the display coprocess */

    exit (0);
    return 0;
}
