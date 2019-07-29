/*
 * $Id: disp.c,v 1.18 2003/12/01 05:48:34 troth Exp $
 *
 ****************************************************************************
 *
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
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "gnu_getopt.h"

#ifdef HAS_NCURSES
#  include <ncurses.h>
#else
#  include <curses.h>
#endif

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

WINDOW *win_reg;
WINDOW *win_io_reg;
WINDOW *win_sram;
WINDOW *win_main;

WINDOW *cur_win;

struct timespec nap = { 0, 1000000 }; /* 1 ms */

int sig_int = 0;
int sig_winch = 0;

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

char *io_reg_names[MAX_IO_REG];

void sram_destroy (void);
void disp_move_cursor (int addr);
void disp_delete_windows (void);

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
sram_init (int size, int start)
{
    sram->size = size + start;
    sram->curr = -1;
    sram->vis_lines = 0;
    sram->vis_beg = 0;
    sram->vis_end = 0;

    sram->data =
        (unsigned char *)malloc (sizeof (unsigned char) * sram->size);
    if (sram->data == NULL)
        abort ();

    memset (sram->data, '\0', sram->size); /* zero out the array */

    atexit (sram_destroy);
}

void
sram_vis_setup (int lines)
{
    /* we'll always show SRAM_COLS addresses per line */

    sram->vis_lines = lines;
    sram->vis_beg = 0;
    sram->vis_end = SRAM_COLS * lines;
    if (sram->vis_end > sram->size)
        sram->vis_end = sram->size;
}

void
sram_destroy (void)
{
    if (sram->data)
        free (sram->data);
}

void
disp_sig_handler (int sig)
{
    switch (sig)
    {
        case SIGINT:
            sig_int++;
            break;
        case SIGWINCH:
            sig_winch++;
            break;
    }
}

/* Wrap read(2) so we can read a byte without having
   to do a shit load of error checking every time. */

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
            standout ();
            mvwprintw (win_main, LINES - 1, 4, " incomplete read      ");
            standend ();
            continue;
        }

        return c;
    }
    fprintf (stderr, "Maximum read reties reached");
    exit (1);

    return 0;                   /* make compiler happy */
}

void
disp_draw_borders (void)
{
    /* Draw borders for all windows on main on main */
    wstandout (win_main);

    wborder (win_main, '|', '|', '-', '-', '-', '-', '-', '-');

    wmove (win_main, IO_REG_LABEL_LINE, 0);
    whline (win_main, '-', COLS);

    wmove (win_main, SRAM_LABEL_LINE, 0);
    whline (win_main, '-', COLS);

    mvwprintw (win_main, REG_LABEL_LINE, 3, " Registers ");
    mvwprintw (win_main, IO_REG_LABEL_LINE, 3, " IO Registers ");
    mvwprintw (win_main, SRAM_LABEL_LINE, 3, " SRAM  [ 0x0000 ] ");

    wstandend (win_main);
}

void
disp_print_reg (int reg, unsigned char val)
{
    int row, col;

    row = (reg % REG_ROWS) + REG_ROW_OFFSET;
    col = ((COLS - 4) / REG_COLS) * (reg / REG_ROWS) + 7;

    mvwprintw (win_reg, row, col, "%02x", val);
}

void
disp_print_pc (int val)
{
    int w = (COLS - 4) / 4;
    mvwprintw (win_reg, 0, REG_OFFSET, "0x%08x", val);
    mvwprintw (win_reg, 0, REG_OFFSET + w, "0x%08x", val * 2);
}

void
disp_print_sp (int val)
{
    mvwprintw (win_reg, 0, REG_OFFSET + ((COLS - 4) / 4) * 2, "0x%08x", val);
}

/* Print the X, Y and Z address registers */

void
disp_print_addr_reg (int reg, unsigned int val)
{
    int c = REG_OFFSET + ((COLS - 4) / 4 * (reg / 2 - 13));
    mvwprintw (win_reg, 1, c, "0x%08x", val);
}

/* Print the floating point registers. These are pseudo registers where 
     fp0 -> r17::r14
     fp1 -> r21::r18
     fp2 -> r25::r22 */

void
disp_print_float_reg (int fp_reg, float val)
{
    mvwprintw (win_reg, fp_reg, REG_OFFSET + ((COLS - 4) / 4) * 3,
               "                 ");
    mvwprintw (win_reg, fp_reg, REG_OFFSET + ((COLS - 4) / 4) * 3, "%+e",
               (double)val);
}

void
disp_print_sreg (unsigned char val)
{
    int i;

    mvwprintw (win_reg, 2, 7, "%02x", val);

    for (i = 0; i < 8; i++)
        mvwprintw (win_reg, 2, 14 + (i * 4), "%d", val >> (7 - i) & 0x1);
}

void
disp_reg_labels (void)
{
    int i, j, w;

    /* Print PC, PC*2, SP, ??? */
    w = (COLS - 4) / 4;
    mvwprintw (win_reg, 0, 2, "  PC =");
    mvwprintw (win_reg, 0, 2 + w, "PC*2 =");
    mvwprintw (win_reg, 0, 2 + w * 2, "  SP =");

    /* Print X, Y, Z */
    mvwprintw (win_reg, 1, 2, "   X =");
    mvwprintw (win_reg, 1, 2 + w, "   Y =");
    mvwprintw (win_reg, 1, 2 + w * 2, "   Z =");

    /* Floating point registers */
    mvwprintw (win_reg, 0, 2 + w * 3 - 3, "r17::14 =");
    mvwprintw (win_reg, 1, 2 + w * 3 - 3, "r21::18 =");
    mvwprintw (win_reg, 2, 2 + w * 3 - 3, "r25::22 =");

    /* Print SREG */
    mvwprintw (win_reg, 2, 2, "SREG=00   I=0 T=0 H=0 S=0 V=0 N=0 Z=0 C=0");

    w = (COLS - 4) / REG_COLS;
    /* Print the rNN registers */
    for (i = 0; i < REG_ROWS; i++)
    {
        for (j = 0; j < REG_COLS; j++)
        {
            mvwprintw (win_reg, i + REG_ROW_OFFSET, 3 + w * j, "r%02d=",
                       j * REG_ROWS + i);
        }
    }

    for (i = 0; i < MAX_REG; i++)
        disp_print_reg (i, 0);

    disp_print_addr_reg (26, 0);
    disp_print_addr_reg (28, 0);
    disp_print_addr_reg (30, 0);

    disp_print_float_reg (0, 0.0);
    disp_print_float_reg (1, 0.0);
    disp_print_float_reg (2, 0.0);

    disp_print_pc (0);
    disp_print_sp (0);
}

void
disp_print_io_reg (int reg, unsigned char val)
{
    int row, col;

    row = (reg % IO_REG_LINES);
    col = ((COLS - 4) / IO_REG_COLS) * (reg / IO_REG_LINES) + 7;

    mvwprintw (win_io_reg, row, col, "%02x", val);
}

void
disp_print_io_reg_name (int reg, char *long_name)
{
    int w, wn, r, c;
    char name[50];

    w = (COLS - 4) / IO_REG_COLS;
    wn = w - 11;                /* "[nn]=vv  " */

    r = reg % IO_REG_LINES;
    c = 11 + (reg / IO_REG_LINES) * w;

    strncpy (io_reg_names[reg], long_name, MAX_IO_REG_BUF);
    strncpy (name, long_name, wn);
    mvwprintw (win_io_reg, r, c, "%-*s", wn, name);
}

/* Print the IO Registers */

void
disp_io_reg_labels (void)
{
    int i, j, w;
    char name[50];
    int wn;

    memset (name, '\0', 50);

    w = (COLS - 4) / IO_REG_COLS;
    wn = w - 11;                /* "[nn]=vv  " */

    for (i = 0; i < IO_REG_LINES; i++)
    {
        for (j = 0; j < IO_REG_COLS; j++)
        {
            int reg = j * IO_REG_LINES + i;
            strncpy (name, io_reg_names[reg], wn);
            mvwprintw (win_io_reg, i, 2 + w * j, "[%02x]=    %-*s", reg, wn,
                       name);
        }
    }

    for (i = 0; i < MAX_IO_REG; i++)
        disp_print_io_reg (i, 0);
}

void
disp_update_sram (void)
{
    int i, j, w;
    int addr;

    werase (win_sram);

    w = (COLS - 2 - SRAM_COL_OFFSET) / SRAM_COLS; /* 2 for borders, 9 for
                                                     addr */

    for (i = 0; i < sram->vis_lines; i++)
    {
        addr = (sram->vis_beg / SRAM_COLS + i) * SRAM_COLS;
        if (addr >= sram->size)
            break;

        mvwprintw (win_sram, i, 0, "0x%04x :", addr);

        for (j = 0; j < SRAM_COLS; j++)
        {
            if (addr + j >= sram->size)
                break;

            mvwprintw (win_sram, i, SRAM_COL_OFFSET + j * w, "%02x",
                       sram->data[addr + j]);
        }
    }
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
        if (((addr + i) >= 0) && ((addr + i) < sram->size))
            sram->data[addr + i] = datum;
    }

    /* Update the screen if any address in range [addr, addr+len) is in the
       range of [sram->vis_beg, sram->vis_end). */

    if ((addr < sram->vis_end) && ((addr + len) > sram->vis_beg))
        disp_update_sram ();
}

/* Move the cursor to another sram address and update the current address in
   the border. Scroll the SRAM window if needed. */

void
disp_move_cursor (int addr)
{
    int r, c, w;

    if (addr < 0)
        return;

    if (addr >= sram->size)
        return;

    sram->curr = addr;
    standout ();
    mvwprintw (win_main, SRAM_LABEL_LINE, SRAM_LABEL_ADDR_COL, "%04x", addr);
    standend ();

    if (addr < sram->vis_beg)
    {
        /* need to scroll up */
        sram->vis_beg = (addr / SRAM_COLS) * SRAM_COLS;
        sram->vis_end = sram->vis_beg + (SRAM_COLS * sram->vis_lines);
        if (sram->vis_end > sram->size)
            sram->vis_end = sram->size;
        disp_update_sram ();
    }

    if (addr >= sram->vis_end)
    {
        /* need to scroll down */
        sram->vis_beg = (addr / SRAM_COLS - sram->vis_lines + 1) * SRAM_COLS;
        sram->vis_end = sram->vis_beg + (SRAM_COLS * sram->vis_lines);
        if (sram->vis_end > sram->size)
            sram->vis_end = sram->size;
        disp_update_sram ();
    }

    /* move the cursor */
    w = (COLS - 2 - SRAM_COL_OFFSET) / SRAM_COLS; /* 2 for borders, 9 for
                                                     addr */
    r = (addr - sram->vis_beg) / SRAM_COLS;
    c = SRAM_COL_OFFSET + ((addr % SRAM_COLS) * w);
    wmove (win_sram, r, c);
}

void
disp_cycle_cur_win (void)
{
    /* This does nothing at the moment */
/*      if (cur_win == win_sram) */
/*          cur_win = win_eeprom; */
/*      else if (cur_win == win_eeprom) */
/*          cur_win = win_sram; */
/*      else */
/*      { */
/*          fprintf(stderr, "bad win for cycling\n"); */
/*          abort(); */
/*      } */
/*      wmove(cur_win, 0, 0); */
}

void
disp_initialize (void)
{
    int lines, cols, line, col;

    static int is_scr_initialized = 0;

    /* When the xterm is resized, we will call disp_initialize() again to
       re-draw the display. */

    if (is_scr_initialized == 0)
    {
        win_main = initscr ();
        is_scr_initialized = 1;
    }
    else
    {
        disp_delete_windows ();
    }

    /* Setup the register window */
    line = col = REG_LABEL_LINE + 1;
    lines = REG_LINES;
    cols = COLS - 2;            /* use entire width of main less borders */
    win_reg = subwin (win_main, lines, cols, line, col);

    /* Setup io register window */
    line = IO_REG_LABEL_LINE + 1;
    lines = IO_REG_LINES;
    win_io_reg = subwin (win_main, lines, cols, line, col);

    /* Setup sram window */
    line = SRAM_LABEL_LINE + 1;
    lines = LINES - line - 1;   /* use rest of screen */
    sram_vis_setup (lines);
    win_sram = subwin (win_main, lines, cols, line, col);

    disp_draw_borders ();
    disp_reg_labels ();
    disp_io_reg_labels ();

    disp_update_sram ();

    cur_win = win_sram;

    wrefresh (win_main);

    keypad (win_main, TRUE);
    noecho ();

    nodelay (win_main, TRUE);

    /* I used this for debugging, might need it again. */
    if (0)
    {
        struct winsize size;

        if (ioctl (STDOUT_FILENO, TIOCGWINSZ, (char *)&size) >= 0)
            mvwprintw (win_main, 0, 20, "rows:%d(%d)  cols:%d(%d)",
                       size.ws_row, LINES, size.ws_col, COLS);
    }
}

/* The terminal has been resized, redraw the display. */

static void
disp_change_size (void)
{
    struct winsize size[1];

    if (ioctl (STDIN_FILENO, TIOCGWINSZ, (char *)size) >= 0)
    {
        resizeterm (size->ws_row, size->ws_col);

#if 0
        mvwprintw (win_main, 0, 20, "rows:%d(%d)  cols:%d(%d)", size->ws_row,
                   LINES, size->ws_col, COLS);
#endif

        werase (win_main);
        disp_initialize ();

        disp_move_cursor (sram->curr);

        wrefresh (win_main);
        wrefresh (win_reg);
        wrefresh (win_io_reg);
        wrefresh (win_sram);
    }
}

void
disp_delete_windows (void)
{
    delwin (win_reg);
    delwin (win_io_reg);
    delwin (win_sram);
}

void
disp_cleanup (void)
{
    disp_delete_windows ();

    endwin ();
}

void
disp_usage (char *prog)
{
    fprintf (stderr,
             "Usage: %s [-p |--pfd=nn] flash_size sram_size "
             "sram_start eeprom_size\n", prog);
    exit (1);
}

#define GOTO_INIT_STR  " Goto Addr -> [           ] "
#define GOTO_WIPE_STR  "-----------------------------"
#define GOTO_FMT_STR   " Goto Addr -> [ %10s ] "
#define MAX_ADDR_STR   10

/* See if the user has hit any keys and process them.
   Returns, 1 if user wants to quit and 0 otherwise. */

int
disp_check_for_user_input (void)
{
    int c;
    int new_addr;
    char *endptr;

    static int goto_mode = 0;
    static char addr_str[MAX_ADDR_STR + 1];
    static int len = 0;

    if (goto_mode)
    {
        switch (c = getch ())
        {
            case KEY_ENTER:
            case '\n':
                new_addr = strtol (addr_str, &endptr, 0);
                disp_move_cursor (new_addr);
                /* fall through to exit mode */
            case '\033':       /* ESC */
                len = 0;
                memset (addr_str, '\0', sizeof (addr_str));
                goto_mode = 0;
                standout ();
                mvwprintw (win_main, SRAM_LABEL_LINE, SRAM_GOTO_ADDR_COL,
                           "%s", GOTO_WIPE_STR);
                standend ();
                break;
            case KEY_BACKSPACE:
                if (len > 0)
                {
                    addr_str[--len] = '\0';
                    standout ();
                    mvwprintw (win_main, SRAM_LABEL_LINE, SRAM_GOTO_ADDR_COL,
                               GOTO_FMT_STR, addr_str);
                    standend ();
                }
                break;
            default:
                if (isxdigit (c) || (c == 'x') || (c == 'X'))
                {
                    if (len < (sizeof (addr_str) - 1))
                    {
                        addr_str[len++] = c;
                        standout ();
                        mvwprintw (win_main, SRAM_LABEL_LINE,
                                   SRAM_GOTO_ADDR_COL, GOTO_FMT_STR,
                                   addr_str);
                        standend ();
                    }
                }
        }
    }
    else
    {
        switch (c = getch ())
        {
            case ERR:
                break;          /* no key has been pressed */
            case '\t':
                disp_cycle_cur_win ();
                break;
            case 'k':
            case KEY_UP:
                disp_move_cursor (sram->curr - SRAM_COLS);
                break;
            case 'j':
            case KEY_DOWN:
                disp_move_cursor (sram->curr + SRAM_COLS);
                break;
            case 'h':
            case KEY_LEFT:
                disp_move_cursor (sram->curr - 1);
                break;
            case 'l':
            case KEY_RIGHT:
                disp_move_cursor (sram->curr + 1);
                break;
            case 'g':
                len = 0;
                memset (addr_str, '\0', sizeof (addr_str));
                standout ();
                mvwprintw (win_main, SRAM_LABEL_LINE, SRAM_GOTO_ADDR_COL,
                           GOTO_FMT_STR, " ");
                standend ();
                goto_mode = 1;
                break;
            case KEY_HOME:
                disp_move_cursor (0);
                break;
            case KEY_END:
                disp_move_cursor (sram->size - 1);
                break;
            case KEY_PPAGE:
                new_addr = sram->curr - (sram->vis_end - sram->vis_beg);
                if (new_addr < 0)
                {
                    new_addr = 0;
                }
                disp_move_cursor (new_addr);
                break;
            case KEY_NPAGE:
                new_addr = sram->curr + (sram->vis_end - sram->vis_beg);
                if (new_addr >= sram->size)
                {
                    new_addr = sram->size - 1;
                }
                disp_move_cursor (new_addr);
                break;
            case 'q':
            case 'Q':
                return 1;
        }
    }

    wrefresh (win_main);
    wrefresh (win_reg);
    wrefresh (win_io_reg);
    wrefresh (win_sram);

    return 0;
}

/* Parse an input packet which has already been decoded. */

int
disp_parse_packet (char *pkt)
{
    int val;
    unsigned int bval;
    char name[50];

    switch (*pkt++)
    {
        case 'q':              /* quit */
            return 1;

        case 'p':              /* set program counter */
            if (sscanf (pkt, "%x", &val) == 1)
                disp_print_pc (val);
            break;

        case 'r':              /* set register */
            if (sscanf (pkt, "%x:%x", &val, &bval) == 2)
            {
                if ((val >= 14) && (val <= 25))
                {
                    int fp_reg = (val - 14) / 4;
                    int fp_byte = (val - 14) % 4;

                    fp_regs[fp_reg].ival[fp_byte] = (bval & 0xff);

                    disp_print_float_reg (fp_reg, fp_regs[fp_reg].fval);
                }
                if ((val >= 26) && (val <= 31))
                {
                    unsigned int areg;
                    switch (val)
                    {
                        case 26:
                            areg = xreg = (xreg & 0xff00) | (bval & 0xff);
                            break;
                        case 27:
                            areg = xreg =
                                (xreg & 0xff) | ((bval << 8) & 0xff00);
                            break;
                        case 28:
                            areg = yreg = (yreg & 0xff00) | (bval & 0xff);
                            break;
                        case 29:
                            areg = yreg =
                                (yreg & 0xff) | ((bval << 8) & 0xff00);
                            break;
                        case 30:
                            areg = zreg = (zreg & 0xff00) | (bval & 0xff);
                            break;
                        case 31:
                            areg = zreg =
                                (zreg & 0xff) | ((bval << 8) & 0xff00);
                            break;
						default:
							areg = 0;
                    }
                    disp_print_addr_reg (val, areg);
                }
                disp_print_reg (val, (unsigned char)bval);
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
                    disp_print_sp (spreg);
                }
                else if (val == 0x3e) /* SPH */
                {
                    spreg = (spreg & 0xff) | ((bval << 8) & 0xff00);
                    disp_print_sp (spreg);
                }
                else if (val == 0x3f) /* SREG */
                {
                    disp_print_sreg (bval);
                }
                disp_print_io_reg (val, (unsigned char)bval);
            }
            break;

        case 'I':              /* set io register name */
            if (sscanf (pkt, "%x:%s", &val, name) == 2)
                disp_print_io_reg_name (val, name);
            break;

        case 'e':              /* set eeprom value: not implemented yet */
            break;

        case 'f':              /* set flash value: not implemented yet */
            break;
    }

    disp_move_cursor (sram->curr);

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
    int i;
    int max_fd;
    int rv;
    int updated;
    fd_set watchset, inset;
    struct timeval to, otime, tdiff;

    for (i = 0; i < MAX_IO_REG; i++)
    {
        io_reg_names[i] =
            (char *)malloc ((MAX_IO_REG_BUF + 1) * sizeof (char));
        memset (io_reg_names[i], '\0', MAX_IO_REG_BUF + 1);
        strncpy (io_reg_names[i], "Reserved", MAX_IO_REG_BUF);
    }

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

    sram_init (args->sram_size, args->sram_start);

    signal (SIGINT, disp_sig_handler);
    signal (SIGWINCH, disp_sig_handler);

    /* setup the select() for multiplexing */

    FD_ZERO (&watchset);
    if (args->pipe_fd >= 0)
        FD_SET (args->pipe_fd, &watchset);
    FD_SET (STDIN_FILENO, &watchset);

    max_fd = STDIN_FILENO > args->pipe_fd ? STDIN_FILENO : args->pipe_fd;

    disp_initialize ();
    atexit (disp_cleanup);

    disp_move_cursor (0);

    wrefresh (win_main);
    wrefresh (win_reg);
    wrefresh (win_io_reg);
    wrefresh (win_sram);

    timerclear (&otime);
    updated = 0;

    /* main loop */
    while (FD_ISSET (STDIN_FILENO, &watchset)
           || ((args->pipe_fd >= 0) && FD_ISSET (args->pipe_fd, &watchset)))
    {
        if (sig_int)
        {
            sig_int = 0;
            break;
        }

        if (sig_winch)
        {
            sig_winch = 0;
            disp_change_size ();
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
            if (disp_check_for_user_input () && (args->pipe_fd == -1))
            {
                /* Only enable quiting if not using the pipe so that the user
                   can't quit the display if it's run as a sub-process. */
                break;
            }
        }

        if ((args->pipe_fd >= 0) && FD_ISSET (args->pipe_fd, &inset))
        {
            updated++;
            if (disp_check_update_pipe (args->pipe_fd))
                break;
        }

        gettimeofday (&to, 0);
        timersub (&to, &otime, &tdiff);

        if (updated && (tdiff.tv_sec > 0 || tdiff.tv_usec > 100000))
        {
            wrefresh (win_main);
            wrefresh (win_reg);
            wrefresh (win_io_reg);
            wrefresh (win_sram);
            updated = 0;
            otime = to;
        }
    }

    return 0;
}
