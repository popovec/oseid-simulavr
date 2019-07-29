/*
 * $Id: gdbserver.c,v 1.50 2004/10/19 18:43:50 zfrdh Exp $
 *
 ****************************************************************************
 *
 * gdbserver.c - Provide interface to a remote debugging target of gdb.
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
 * \file gdbserver.c
 * \brief Provide an interface to gdb's remote serial protocol.
 *
 * This module allows a program to be used by gdb as a remote target. The
 * remote target and gdb communicate via gdb's remote serial protocol. The
 * protocol is documented in the gdb manual and will not be repeated here.
 *
 * Hitting Ctrl-c in gdb can be used to interrupt the remote target while it
 * is processing instructions and return control back to gdb.
 *
 * Issuing a 'signal SIGxxx' command from gdb will send the signal to the
 * remote target via a "continue with signal" packet. The target will process
 * and interpret the signal, but not pass it on to the AVR program running in
 * the target since it really makes no sense to do so. In some circumstances,
 * it may make sense to use the gdb signal mechanism as a way to initiate some
 * sort of external stimulus to be passed on to the virtual hardware system.
 *
 * Signals from gdb which are processed have the following meanings:
 *
 * \li \c SIGHUP Initiate a reset of the target. (Simulates a hardware reset)
 */

#include <config.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "avrerror.h"
#include "avrmalloc.h"
#include "gdb.h"
#include "sig.h"

// dirty access to EERPOM
uint8_t gdb_ee_read(int adr);
void    gdb_ee_write(int adr,uint8_t data);

#define USE_EEPROM_SPACE
/* *INDENT-OFF* */
#ifndef DOXYGEN                 /* have doxygen system ignore this. */
enum
{
    MAX_BUF        = 400,       /* Maximum size of read/write buffers. */
    MAX_READ_RETRY = 10,        /* Maximum number of retries if a read is
                                   incomplete. */

#if defined(USE_EEPROM_SPACE)
    MEM_SPACE_MASK = 0x00ff0000, /* mask to get bits which determine memory
                                    space */
    FLASH_OFFSET   = 0x00000000, /* Data in flash has this offset from gdb */
    SRAM_OFFSET    = 0x00800000, /* Data in sram has this offset from gdb */
    EEPROM_OFFSET  = 0x00810000, /* Data in eeprom has this offset from gdb */
#else
    MEM_SPACE_MASK = 0x00f00000, /* mask to get bits which determine memory
                                    space */
    FLASH_OFFSET   = 0x00000000, /* Data in flash has this offset from gdb */
    SRAM_OFFSET    = 0x00800000, /* Data in sram has this offset from gdb */
#endif

    GDB_BLOCKING_OFF = 0,       /* Signify that a read is non-blocking. */
    GDB_BLOCKING_ON  = 1,       /* Signify that a read will block. */

    GDB_RET_CTRL_C = -2,        /* gdb has sent Ctrl-C to interrupt what is
                                   doing */
    GDB_RET_KILL_REQUEST = -1,  /* gdb has requested that sim be killed */
    GDB_RET_OK = 0,             /* continue normal processing of gdb
                                   requests */

    SPL_ADDR = 0x5d,
    SPH_ADDR = 0x5e,
};
#endif /* not DOXYGEN */
/* *INDENT-ON* */

/* Use HEX_DIGIT as a lookup table to convert a nibble to hex 
   digit. */
static char HEX_DIGIT[] = "0123456789abcdef";

/* There are a couple of nested infinite loops, this allows escaping them
   all. */
static int global_server_quit = 0;

/* Flag if debug messages should be printed out. */
static int global_debug_on;

/* prototypes */

static int gdb_pre_parse_packet (GdbComm_T *comm, int fd, int blocking);

/* Wrap read(2) so we can read a byte without having
   to do a shit load of error checking every time. */

static int
gdb_read_byte (int fd)
{
    char c;
    int res;
    int cnt = MAX_READ_RETRY;

    while (cnt--)
    {
        res = read (fd, &c, 1);
        if (res < 0)
        {
            if (errno == EAGAIN)
                /* fd was set to non-blocking and no data was available */
                return -1;

            avr_error ("read failed: %s", strerror (errno));
        }

        if (res == 0)
        {
            avr_warning ("incomplete read\n");
            continue;
        }

        return c;
    }
    avr_error ("Maximum read reties reached");

    return 0;                   /* make compiler happy */
}

/* Convert a hexidecimal digit to a 4 bit nibble. */

static uint8_t
hex2nib (char hex)
{
    if ((hex >= 'A') && (hex <= 'F'))
        return (10 + (hex - 'A'));

    else if ((hex >= 'a') && (hex <= 'f'))
        return (10 + (hex - 'a'));

    else if ((hex >= '0') && (hex <= '9'))
        return (hex - '0');

    /* Shouldn't get here unless the developer screwed up ;) */
    avr_error ("Invalid hexidecimal digit: 0x%02x", hex);

    return 0;                   /* make compiler happy */
}

/* Wrapper for write(2) which hides all the repetitive error
   checking crap. */

static void
gdb_write (int fd, const void *buf, size_t count)
{
    int res;

    res = write (fd, buf, count);

    /* FIXME: should we try and catch interrupted system calls here? */

    if (res < 0)
        avr_error ("write failed: %s", strerror (errno));

    /* FIXME: if this happens a lot, we could try to resend the
       unsent bytes. */

    if (res != count)
        avr_error ("write only wrote %d of %d bytes", res, count);
}

/* Use a single function for storing/getting the last reply message.
   If reply is NULL, return pointer to the last reply saved.
   Otherwise, make a copy of the buffer pointed to by reply. */

static char *
gdb_last_reply (char *reply)
{
    static char *last_reply = NULL;

    if (reply == NULL)
    {
        if (last_reply == NULL)
            return "";
        else
            return last_reply;
    }

    avr_free (last_reply);
    last_reply = avr_strdup (reply);

    return last_reply;
}

/* Acknowledge a packet from GDB */

static void
gdb_send_ack (int fd)
{
    if (global_debug_on)
        fprintf (stderr, " Ack -> gdb\n");

    gdb_write (fd, "+", 1);
}

/* Send a reply to GDB. */

static void
gdb_send_reply (int fd, char *reply)
{
    int cksum = 0;
    int bytes;

    static char buf[MAX_BUF];

    /* Save the reply to last reply so we can resend if need be. */
    gdb_last_reply (reply);

    if (global_debug_on)
        fprintf (stderr, "Sent: $%s#", reply);

    if (*reply == '\0')
    {
        gdb_write (fd, "$#00", 4);

        if (global_debug_on)
            fprintf (stderr, "%02x\n", cksum & 0xff);
    }
    else
    {
        memset (buf, '\0', sizeof (buf));

        buf[0] = '$';
        bytes = 1;

        while (*reply)
        {
            cksum += (unsigned char)*reply;
            buf[bytes] = *reply;
            bytes++;
            reply++;

            /* must account for "#cc" to be added */
            if (bytes == (MAX_BUF - 3))
            {
                /* FIXME: TRoth 2002/02/18 - splitting reply would be better */
                avr_error ("buffer overflow");
            }
        }

        if (global_debug_on)
            fprintf (stderr, "%02x\n", cksum & 0xff);

        buf[bytes++] = '#';
        buf[bytes++] = HEX_DIGIT[(cksum >> 4) & 0xf];
        buf[bytes++] = HEX_DIGIT[cksum & 0xf];

        gdb_write (fd, buf, bytes);
    }
}

/* GDB needs the 32 8-bit, gpw registers (r00 - r31), the 
   8-bit SREG, the 16-bit SP (stack pointer) and the 32-bit PC
   (program counter). Thus need to send a reply with
   r00, r01, ..., r31, SREG, SPL, SPH, PCL, PCH
   Low bytes before High since AVR is little endian. */

static void
gdb_read_registers (GdbComm_T *comm, int fd)
{
    int i;
    uint32_t val;               /* ensure it's 32 bit value */

    /* (32 gpwr, SREG, SP, PC) * 2 hex bytes + terminator */
    size_t buf_sz = (32 + 1 + 2 + 4) * 2 + 1;
    char *buf;

    buf = avr_new0 (char, buf_sz);

    /* 32 gen purpose working registers */
    for (i = 0; i < 32; i++)
    {
        val = comm->read_reg (comm->user_data, i);
        buf[i * 2] = HEX_DIGIT[(val >> 4) & 0xf];
        buf[i * 2 + 1] = HEX_DIGIT[val & 0xf];
    }

    /* GDB thinks SREG is register number 32 */
    val = comm->read_sreg (comm->user_data);
    buf[i * 2] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 1] = HEX_DIGIT[val & 0xf];
    i++;

    /* GDB thinks SP is register number 33 */
    val = comm->read_sram (comm->user_data, SPL_ADDR);
    buf[i * 2] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 1] = HEX_DIGIT[val & 0xf];
    i++;

    val = comm->read_sram (comm->user_data, SPH_ADDR);
    buf[i * 2] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 1] = HEX_DIGIT[val & 0xf];
    i++;

    /* GDB thinks PC is register number 34.
       GDB stores PC in a 32 bit value (only uses 23 bits though).
       GDB thinks PC is bytes into flash, not words like in simulavr. */

    val = comm->read_pc (comm->user_data) * 2;
    buf[i * 2] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 1] = HEX_DIGIT[val & 0xf];

    val >>= 8;
    buf[i * 2 + 2] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 3] = HEX_DIGIT[val & 0xf];

    val >>= 8;
    buf[i * 2 + 4] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 5] = HEX_DIGIT[val & 0xf];

    val >>= 8;
    buf[i * 2 + 6] = HEX_DIGIT[(val >> 4) & 0xf];
    buf[i * 2 + 7] = HEX_DIGIT[val & 0xf];

    gdb_send_reply (fd, buf);
    avr_free (buf);
}

/* GDB is sending values to be written to the registers. Registers are the
   same and in the same order as described in gdb_read_registers() above. */

static void
gdb_write_registers (GdbComm_T *comm, int fd, char *pkt)
{
    int i;
    uint8_t bval;
    uint32_t val;               /* ensure it's a 32 bit value */

    /* 32 gen purpose working registers */
    for (i = 0; i < 32; i++)
    {
        bval = hex2nib (*pkt++) << 4;
        bval += hex2nib (*pkt++);
        comm->write_reg (comm->user_data, i, bval);
    }

    /* GDB thinks SREG is register number 32 */
    bval = hex2nib (*pkt++) << 4;
    bval += hex2nib (*pkt++);
    comm->write_sreg (comm->user_data, bval);

    /* GDB thinks SP is register number 33 */
    bval = hex2nib (*pkt++) << 4;
    bval += hex2nib (*pkt++);
    comm->write_sram (comm->user_data, SPL_ADDR, bval);

    bval = hex2nib (*pkt++) << 4;
    bval += hex2nib (*pkt++);
    comm->write_sram (comm->user_data, SPH_ADDR, bval);

    /* GDB thinks PC is register number 34.
       GDB stores PC in a 32 bit value (only uses 23 bits though).
       GDB thinks PC is bytes into flash, not words like in simulavr.

       Must cast to uint32_t so as not to get mysterious truncation. */

    val = ((uint32_t) hex2nib (*pkt++)) << 4;
    val += ((uint32_t) hex2nib (*pkt++));

    val += ((uint32_t) hex2nib (*pkt++)) << 12;
    val += ((uint32_t) hex2nib (*pkt++)) << 8;

    val += ((uint32_t) hex2nib (*pkt++)) << 20;
    val += ((uint32_t) hex2nib (*pkt++)) << 16;

    val += ((uint32_t) hex2nib (*pkt++)) << 28;
    val += ((uint32_t) hex2nib (*pkt++)) << 24;
    comm->write_pc (comm->user_data, val / 2);

    gdb_send_reply (fd, "OK");
}

/* Extract a hexidecimal number from the pkt. Keep scanning pkt until stop
   char is reached or size of int is exceeded or a NULL is reached. pkt is
   modified to point to stop char when done.

   Use this function to extract a num with an arbitrary num of hex
   digits. This should _not_ be used to extract n digits from a m len string
   of digits (n <= m). */

static int
gdb_extract_hex_num (char **pkt, char stop)
{
    int i = 0;
    int num = 0;
    char *p = *pkt;
    int max_shifts = sizeof (int) * 2 - 1; /* max number of nibbles to shift
                                              through */

    while ((*p != stop) && (*p != '\0'))
    {
        if (i > max_shifts)
            avr_error ("number too large");

        num = (num << 4)  | hex2nib (*p);
        i++;
        p++;
    }

    *pkt = p;
    return num;
}

/* Read a single register. Packet form: 'pn' where n is a hex number with no
   zero padding. */

static void
gdb_read_register (GdbComm_T *comm, int fd, char *pkt)
{
    int reg;

    char reply[MAX_BUF];

    memset (reply, '\0', sizeof (reply));

    reg = gdb_extract_hex_num (&pkt, '\0');

    if ((reg >= 0) && (reg < 32))
    {                           /* general regs */
        uint8_t val = comm->read_reg (comm->user_data, reg);
        snprintf (reply, sizeof (reply) - 1, "%02x", val);
    }
    else if (reg == 32)         /* sreg */
    {
        uint8_t val = comm->read_sreg (comm->user_data);
        snprintf (reply, sizeof (reply) - 1, "%02x", val);
    }
    else if (reg == 33)         /* SP */
    {
        uint8_t spl, sph;
        spl = comm->read_sram (comm->user_data, SPL_ADDR);
        sph = comm->read_sram (comm->user_data, SPH_ADDR);
        snprintf (reply, sizeof (reply) - 1, "%02x%02x", spl, sph);
    }
    else if (reg == 34)         /* PC */
    {
        int val = comm->read_pc (comm->user_data) * 2;
        snprintf (reply, sizeof (reply) - 1, "%02x%02x" "%02x%02x",
                  val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
                  (val >> 24) & 0xff);
    }
    else
    {
        avr_warning ("Bad register value: %d\n", reg);
        gdb_send_reply (fd, "E00");
        return;
    }
    gdb_send_reply (fd, reply);
}

/* Write a single register. Packet form: 'Pn=r' where n is a hex number with
   no zero padding and r is two hex digits for each byte in register (target
   byte order). */

static void
gdb_write_register (GdbComm_T *comm, int fd, char *pkt)
{
    int reg;
    uint32_t dval, hval;

    reg = gdb_extract_hex_num (&pkt, '=');
    pkt++;                      /* skip over '=' character */

    /* extract the low byte of value from pkt */
    dval = hex2nib (*pkt++) << 4;
    dval += hex2nib (*pkt++);

    if ((reg >= 0) && (reg < 33))
    {
        /* r0 to r31 and SREG */
        if (reg == 32)          /* gdb thinks SREG is register 32 */
        {
            comm->write_sreg (comm->user_data, dval & 0xff);
        }
        else
        {
            comm->write_reg (comm->user_data, reg, dval & 0xff);
        }
    }
    else if (reg == 33)
    {
        /* SP is 2 bytes long so extract upper byte */
        hval = hex2nib (*pkt++) << 4;
        hval += hex2nib (*pkt++);

        comm->write_sram (comm->user_data, SPL_ADDR, dval & 0xff);
        comm->write_sram (comm->user_data, SPH_ADDR, hval & 0xff);
    }
    else if (reg == 34)
    {
        /* GDB thinks PC is register number 34.
           GDB stores PC in a 32 bit value (only uses 23 bits though).
           GDB thinks PC is bytes into flash, not words like in simulavr.

           Must cast to uint32_t so as not to get mysterious truncation. */

        /* we already read the first two nibbles */

        dval += ((uint32_t) hex2nib (*pkt++)) << 12;
        dval += ((uint32_t) hex2nib (*pkt++)) << 8;

        dval += ((uint32_t) hex2nib (*pkt++)) << 20;
        dval += ((uint32_t) hex2nib (*pkt++)) << 16;

        dval += ((uint32_t) hex2nib (*pkt++)) << 28;
        dval += ((uint32_t) hex2nib (*pkt++)) << 24;
        comm->write_pc (comm->user_data, dval / 2);
    }
    else
    {
        avr_warning ("Bad register value: %d\n", reg);
        gdb_send_reply (fd, "E00");
        return;
    }

    gdb_send_reply (fd, "OK");
}

/* Parse the pkt string for the addr and length.
   a_end is first char after addr.
   l_end is first char after len.
   Returns number of characters to advance pkt. */

static int
gdb_get_addr_len (char *pkt, char a_end, char l_end, int *addr, int *len)
{
    char *orig_pkt = pkt;

    *addr = 0;
    *len = 0;

    /* Get the addr from the packet */
    while (*pkt != a_end)
        *addr = (*addr << 4) + hex2nib (*pkt++);
    pkt++;                      /* skip over a_end */

    /* Get the length from the packet */
    while (*pkt != l_end)
        *len = (*len << 4) + hex2nib (*pkt++);
    pkt++;                      /* skip over l_end */

/*      fprintf( stderr, "+++++++++++++ addr = 0x%08x\n", *addr ); */
/*      fprintf( stderr, "+++++++++++++ len  = %d\n", *len ); */

    return (pkt - orig_pkt);
}

static void
gdb_read_memory (GdbComm_T *comm, int fd, char *pkt)
{
    int addr = 0;
    int len = 0;
    uint8_t *buf;
    uint8_t bval;
    uint16_t wval;
    int i;
    int is_odd_addr;

    pkt += gdb_get_addr_len (pkt, ',', '\0', &addr, &len);

    buf = avr_new0 (uint8_t, (len * 2) + 1);

    if (addr >= SRAM_OFFSET && addr < EEPROM_OFFSET)
    {
        /* addressing sram */

        addr -= SRAM_OFFSET;

        /* Return an error to gdb if it tries to read or write any of the 32
           general purpse registers. This allows gdb to know when a zero
           pointer has been dereferenced. */

        /* FIXME: [TRoth 2002/03/31] This isn't working quite the way I
           thought it would so I've removed it for now. */

        /* if ( (addr >= 0) && (addr < 32) ) */
        if (0)
        {
            snprintf ((char*)buf, len * 2, "E%02x", EIO);
        }
        else
        {
            for (i = 0; i < len; i++)
            {
                bval = comm->read_sram (comm->user_data, addr + i);
                buf[i * 2] = HEX_DIGIT[bval >> 4];
                buf[i * 2 + 1] = HEX_DIGIT[bval & 0xf];
            }
        }
    }
    else if (addr < SRAM_OFFSET)
    {
        /* addressing flash */

        is_odd_addr = addr % 2;
        i = 0;

        if (is_odd_addr)
        {
            bval = comm->read_flash (comm->user_data, addr / 2) >> 8;
            buf[i++] = HEX_DIGIT[bval >> 4];
            buf[i++] = HEX_DIGIT[bval & 0xf];
            addr++;
            len--;
        }

        while (len > 1)
        {
            wval = comm->read_flash (comm->user_data, addr / 2);

            bval = wval & 0xff;
            buf[i++] = HEX_DIGIT[bval >> 4];
            buf[i++] = HEX_DIGIT[bval & 0xf];

            bval = wval >> 8;
            buf[i++] = HEX_DIGIT[bval >> 4];
            buf[i++] = HEX_DIGIT[bval & 0xf];

            len -= 2;
            addr += 2;
        }

        if (len == 1)
        {
            bval = comm->read_flash (comm->user_data, addr / 2) & 0xff;
            buf[i++] = HEX_DIGIT[bval >> 4];
            buf[i++] = HEX_DIGIT[bval & 0xf];
        }
    }
#if defined(USE_EEPROM_SPACE)
    else if (addr >= EEPROM_OFFSET)
    {
        /* addressing eeprom */

        addr -= EEPROM_OFFSET;

        for(i=0;i<len;i++){
                bval = gdb_ee_read (addr + i);
                buf[i * 2] = HEX_DIGIT[bval >> 4];
                buf[i * 2 + 1] = HEX_DIGIT[bval & 0xf];
        }
    }
#endif
    else
    {
        /* gdb asked for memory space which doesn't exist */
        avr_warning ("Invalid memory address: 0x%x.\n", addr);
        snprintf ((char*)buf, len * 2, "E%02x", EIO);
    }

    gdb_send_reply (fd, (char*)buf);

    avr_free (buf);
}

static void
gdb_write_memory (GdbComm_T *comm, int fd, char *pkt)
{
    int addr = 0;
    int len = 0;
    uint8_t bval;
    uint16_t wval;
    int is_odd_addr;
    int i;
    char reply[10];

    /* Set the default reply. */
    strncpy (reply, "OK", sizeof (reply));

    pkt += gdb_get_addr_len (pkt, ',', ':', &addr, &len);

    if (addr >= SRAM_OFFSET && addr < EEPROM_OFFSET)
    {
        /* addressing sram */

        addr -= SRAM_OFFSET;

        /* Return error. See gdb_read_memory for reasoning. */
        /* FIXME: [TRoth 2002/03/31] This isn't working quite the way I
           thought it would so I've removed it for now. */
        /* if ( (addr >= 0) && (addr < 32) ) */
        if (0)
        {
            snprintf (reply, sizeof (reply), "E%02x", EIO);
        }
        else
        {
            for (i = addr; i < addr + len; i++)
            {
                bval = hex2nib (*pkt++) << 4;
                bval += hex2nib (*pkt++);
                comm->write_sram (comm->user_data, i, bval);
            }
        }
    }
    else if (addr < SRAM_OFFSET)
    {
        /* addressing flash */

        /* Some targets might not allow writing to flash */

        if (comm->write_flash && comm->write_flash_lo8
            && comm->write_flash_hi8)
        {

            is_odd_addr = addr % 2;

            if (is_odd_addr)
            {
                bval = hex2nib (*pkt++) << 4;
                bval += hex2nib (*pkt++);
                comm->write_flash_hi8 (comm->user_data, addr / 2, bval);
                len--;
                addr++;
            }

            while (len > 1)
            {
                wval = hex2nib (*pkt++) << 4; /* low byte first */
                wval += hex2nib (*pkt++);
                wval += hex2nib (*pkt++) << 12; /* high byte last */
                wval += hex2nib (*pkt++) << 8;
                comm->write_flash (comm->user_data, addr / 2, wval);
                len -= 2;
                addr += 2;
            }

            if (len == 1)
            {
                /* one more byte to write */
                bval = hex2nib (*pkt++) << 4;
                bval += hex2nib (*pkt++);
                comm->write_flash_lo8 (comm->user_data, addr / 2, bval);
            }
        }
        else
        {
            /* target can't write to flash, so complain to gdb */
            avr_warning ("Gdb asked to write to flash and target can't.\n");
            snprintf (reply, sizeof (reply), "E%02x", EIO);
        }
    }
#if defined (USE_EEPROM_SPACE)
    else if (addr >= EEPROM_OFFSET)
    {
        /* addressing eeprom */

        addr -= EEPROM_OFFSET;

        avr_warning ("Gdb asked to write eeprom address = %x len %d\n",addr,len);
            for (i = addr; i < addr + len; i++)
            {
                bval = hex2nib (*pkt++) << 4;
                bval += hex2nib (*pkt++);
                gdb_ee_write (i, bval);
            }
    }
#endif
    else
    {
        /* gdb asked for memory space which doesn't exist */
        avr_warning ("Invalid memory address: 0x%x.\n", addr);
        snprintf (reply, sizeof (reply), "E%02x", EIO);
    }

    gdb_send_reply (fd, reply);
}

/* Format of breakpoint commands (both insert and remove):

   "z<t>,<addr>,<length>"  -  remove break/watch point
   "Z<t>,<add>r,<length>"  -  insert break/watch point

   In both cases t can be the following:
   t = '0'  -  software breakpoint
   t = '1'  -  hardware breakpoint
   t = '2'  -  write watch point
   t = '3'  -  read watch point
   t = '4'  -  access watch point

   addr is address.
   length is in bytes

   For a software breakpoint, length specifies the size of the instruction to
   be patched. For hardware breakpoints and watchpoints, length specifies the
   memory region to be monitored. To avoid potential problems, the operations
   should be implemented in an idempotent way. -- GDB 5.0 manual. */

static void
gdb_break_point (GdbComm_T *comm, int fd, char *pkt)
{
    int addr = 0;
    int len = 0;

    char z = *(pkt - 1);        /* get char parser already looked at */
    char t = *pkt++;
    pkt++;                      /* skip over first ',' */

    gdb_get_addr_len (pkt, ',', '\0', &addr, &len);

    switch (t)
    {
        case '0':              /* software breakpoint */
            /* addr/2 since addr refers to PC */
            if (comm->max_pc
                && ((addr / 2) >= comm->max_pc (comm->user_data)))
            {
                avr_warning ("Attempt to set break at invalid addr\n");
                gdb_send_reply (fd, "E01");
                return;
            }

            if (z == 'z')
                comm->remove_break (comm->user_data, addr / 2);
            else
                comm->insert_break (comm->user_data, addr / 2);
            break;

        case '1':              /* hardware breakpoint */
        case '2':              /* write watchpoint */
        case '3':              /* read watchpoint */
        case '4':              /* access watchpoint */
            gdb_send_reply (fd, "");
            return;             /* unsupported yet */
    }

    gdb_send_reply (fd, "OK");
}

/* Handle an io registers query. Query has two forms:
   "avr.io_reg" and "avr.io_reg:addr,len".

   The "avr.io_reg" has already been stripped off at this point. 

   The first form means, "return the number of io registers for this target
   device." Second form means, "send data len io registers starting with
   register addr." */

static void
gdb_fetch_io_registers (GdbComm_T *comm, int fd, char *pkt)
{
    int addr, len;
    int i;
    uint8_t val;
    char reply[400];
    char reg_name[80];
    int pos = 0;

    if (comm->io_fetch)
    {
        if (pkt[0] == '\0')
        {
            /* gdb is asking how many io registers the device has. */
            gdb_send_reply (fd, "40");
        }

        else if (pkt[0] == ':')
        {
            /* gdb is asking for io registers addr to (addr + len) */

            gdb_get_addr_len (pkt + 1, ',', '\0', &addr, &len);

            memset (reply, '\0', sizeof (reply));

            for (i = 0; i < len; i++)
            {
                comm->io_fetch (comm->user_data, addr + i, &val, reg_name,
                                sizeof (reg_name));
                pos +=
                    snprintf (reply + pos, sizeof (reply) - pos, "%s,%x;",
                              reg_name, val);
            }

            gdb_send_reply (fd, reply); /* do nothing for now */
        }

        else
            gdb_send_reply (fd, "E01"); /* An error occurred */

    }

    else
        gdb_send_reply (fd, ""); /* tell gdb we don't handle info io
                                    command. */
}

/* Dispatch various query request to specific handler functions. If a query is
   not handled, send an empry reply. */

static void
gdb_query_request (GdbComm_T *comm, int fd, char *pkt)
{
    int len;

    switch (*pkt++)
    {
        case 'R':
            len = strlen ("avr.io_reg");
            if (strncmp (pkt, "avr.io_reg", len) == 0)
            {
                gdb_fetch_io_registers (comm, fd, pkt + len);
                return;
            }
    }

    gdb_send_reply (fd, "");
}

/* Continue command format: "c<addr>" or "s<addr>"

   If addr is given, resume at that address, otherwise, resume at current
   address. */

static void
gdb_continue (GdbComm_T *comm, int fd, char *pkt)
{
    char reply[MAX_BUF + 1];
    int res;
    int pc;
    char step = *(pkt - 1);     /* called from 'c' or 's'? */
    int signo = SIGTRAP;

    static int is_running = 0;

    /* This allows gdb_continue to be reentrant while it's running. */
    if (is_running == 1)
    {
        return;
    }
    is_running = 1;

    memset (reply, 0, sizeof (reply));

    if (*pkt != '\0')
    {
        /* NOTE: from what I've read on the gdb lists, gdb never uses the
           "continue at address" functionality. That may change, so let's
           catch that case. */

        /* get addr to resume at */
        avr_error ("attempt to resume at other than current");
    }

    while (1)
    {
        if (signal_has_occurred (SIGINT))
        {
            global_server_quit = 1;
            break;
        }

        res = comm->step (comm->user_data);

        if (res == BREAK_POINT)
        {
            if (comm->disable_breakpts)
                comm->disable_breakpts (comm->user_data);
            break;
        }

        /* check if gdb sent any messages */
        res = gdb_pre_parse_packet (comm, fd, GDB_BLOCKING_OFF);
        if (res < 0)
        {
            if (res == GDB_RET_CTRL_C)
            {
                signo = SIGINT;
            }
            break;
        }

        /* If called from 's' or 'S', only want to step once */
        if ((step == 's') || (step == 'S'))
            break;
    }

    /* If reply hasn't been set, respond as if a breakpoint was hit. */
    if (reply[0] == '\0')
    {
        /* Send gdb SREG, SP, PC */
        int bytes = 0;

        pc = comm->read_pc (comm->user_data) * 2;

        bytes = snprintf (reply, MAX_BUF, "T%02x", signo);

        /* SREG, SP & PC */
        snprintf (reply + bytes, MAX_BUF - bytes,
                  "20:%02x;" "21:%02x%02x;" "22:%02x%02x%02x%02x;",
                  comm->read_sreg (comm->user_data),
                  comm->read_sram (comm->user_data, SPL_ADDR),
                  comm->read_sram (comm->user_data, SPH_ADDR), pc & 0xff,
                  (pc >> 8) & 0xff, (pc >> 16) & 0xff, (pc >> 24) & 0xff);
    }

    gdb_send_reply (fd, reply);

    is_running = 0;
}

/* Continue with signal command format: "C<sig>;<addr>" or "S<sig>;<addr>"
   "<sig>" should always be 2 hex digits, possibly zero padded.
   ";<addr>" part is optional.

   If addr is given, resume at that address, otherwise, resume at current
   address. */

static void
gdb_continue_with_signal (GdbComm_T *comm, int fd, char *pkt)
{
    int signo;
    char step = *(pkt - 1);

    /* strip out the signal part of the packet */

    signo = (hex2nib (*pkt++) << 4);
    signo += (hex2nib (*pkt++) & 0xf);

    if (global_debug_on)
        fprintf (stderr, "GDB sent signal: %d\n", signo);

    /* Process signals send via remote protocol from gdb. Signals really don't
       make sense to the program running in the simulator, so we are using
       them sort of as an 'out of band' data. */

    switch (signo)
    {
        case SIGHUP:
            /* Gdb user issuing the 'signal SIGHUP' command tells sim to reset
               itself. We reply with a SIGTRAP the same as we do when gdb
               makes first connection with simulator. */
            comm->reset (comm->user_data);
            gdb_send_reply (fd, "S05");
            return;
        default:
            /* Gdb user issuing the 'signal <signum>' command where signum is
               >= 80 is interpreted as a request to trigger an interrupt  
               vector. The vector to trigger is signo-80. 
               (there's an offset of 14)  */
            if (signo >= 94)
            {
                if (comm->irq_raise)
                {
                    comm->irq_raise (comm->user_data, signo - 94);
                }
            }
    }

    /* Modify pkt to look like what gdb_continue() expects and send it to
       gdb_continue(): *pkt should now be either '\0' or ';' */

    if (*pkt == '\0')
    {
        *(pkt - 1) = step;
    }
    else if (*pkt == ';')
    {
        *pkt = step;
    }
    else
    {
        avr_warning ("Malformed packet: \"%s\"\n", pkt);
        gdb_send_reply (fd, "");
        return;
    }

    gdb_continue (comm, fd, pkt);
}

/* Parse the packet. Assumes that packet is null terminated.
   Return GDB_RET_KILL_REQUEST if packet is 'kill' command,
   GDB_RET_OK otherwise. */

static int
gdb_parse_packet (GdbComm_T *comm, int fd, char *pkt)
{
    switch (*pkt++)
    {
        case '?':              /* last signal */
            gdb_send_reply (fd, "S05"); /* signal # 5 is SIGTRAP */
            break;

        case 'g':              /* read registers */
            gdb_read_registers (comm, fd);
            break;

        case 'G':              /* write registers */
            gdb_write_registers (comm, fd, pkt);
            break;

        case 'p':              /* read a single register */
            gdb_read_register (comm, fd, pkt);
            break;

        case 'P':              /* write single register */
            gdb_write_register (comm, fd, pkt);
            break;

        case 'm':              /* read memory */
            gdb_read_memory (comm, fd, pkt);
            break;

        case 'M':              /* write memory */
            gdb_write_memory (comm, fd, pkt);
            break;

        case 'k':              /* kill request */
        case 'D':              /* Detach request */
            /* Reset the simulator since there may be another connection
               before the simulator stops running. */

            comm->reset (comm->user_data);
            gdb_send_reply (fd, "OK");
            return GDB_RET_KILL_REQUEST;

        case 'C':              /* continue with signal */
        case 'S':              /* step with signal */
            gdb_continue_with_signal (comm, fd, pkt);
            break;

        case 'c':              /* continue */
        case 's':              /* step */
            gdb_continue (comm, fd, pkt);
            break;

        case 'z':              /* remove break/watch point */
        case 'Z':              /* insert break/watch point */
            gdb_break_point (comm, fd, pkt);
            break;

        case 'q':              /* query requests */
            gdb_query_request (comm, fd, pkt);
            break;

        default:
            gdb_send_reply (fd, "");
    }

    return GDB_RET_OK;
}

static void
gdb_set_blocking_mode (int fd, int mode)
{
    if (mode)
    {
        /* turn non-blocking mode off */
        if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) & ~O_NONBLOCK) < 0)
            avr_warning ("fcntl failed: %s\n", strerror (errno));
    }
    else
    {
        /* turn non-blocking mode on */
        if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK) < 0)
            avr_warning ("fcntl failed: %s\n", strerror (errno));
    }
}

/* Perform pre-packet parsing. This will handle messages from gdb which are
   outside the realm of packets or prepare a packet for parsing.

   Use the static block_on flag to reduce the over head of turning blocking on
   and off every time this function is called. */

static int
gdb_pre_parse_packet (GdbComm_T *comm, int fd, int blocking)
{
    int i, res;
    int c;
    char pkt_buf[MAX_BUF + 1];
    int cksum, pkt_cksum;
    static int block_on = 1;    /* default is blocking mode */

    if (block_on != blocking)
    {
        gdb_set_blocking_mode (fd, blocking);
        block_on = blocking;
    }

    c = gdb_read_byte (fd);

    switch (c)
    {
        case '$':              /* read a packet */
            /* insure that packet is null terminated. */
            memset (pkt_buf, 0, sizeof (pkt_buf));

            /* make sure we block on fd */
            gdb_set_blocking_mode (fd, GDB_BLOCKING_ON);

            pkt_cksum = i = 0;
            c = gdb_read_byte (fd);
            while ((c != '#') && (i < MAX_BUF))
            {
                pkt_buf[i++] = c;
                pkt_cksum += (unsigned char)c;
                c = gdb_read_byte (fd);
            }

            cksum = hex2nib (gdb_read_byte (fd)) << 4;
            cksum |= hex2nib (gdb_read_byte (fd));

            /* FIXME: Should send "-" (Nak) instead of aborting when we get
               checksum errors. Leave this as an error until it is actually
               seen (I've yet to see a bad checksum - TRoth). It's not a
               simple matter of sending (Nak) since you don't want to get into
               an infinite loop of (bad cksum, nak, resend, repeat). */

            if ((pkt_cksum & 0xff) != cksum)
                avr_error ("Bad checksum: sent 0x%x <--> computed 0x%x",
                           cksum, pkt_cksum);

            if (global_debug_on)
                fprintf (stderr, "Recv: \"$%s#%02x\"\n", pkt_buf, cksum);

            /* always acknowledge a well formed packet immediately */
            gdb_send_ack (fd);

            res = gdb_parse_packet (comm, fd, pkt_buf);
            if (res < 0)
                return res;

            break;

        case '-':
            if (global_debug_on)
                fprintf (stderr, " gdb -> Nak\n");
            gdb_send_reply (fd, gdb_last_reply (NULL));
            break;

        case '+':
            if (global_debug_on)
                fprintf (stderr, " gdb -> Ack\n");
            break;

        case 0x03:
            /* Gdb sends this when the user hits C-c. This is gdb's way of
               telling the simulator to interrupt what it is doing and return
               control back to gdb. */
            return GDB_RET_CTRL_C;

        case -1:
            /* fd is non-blocking and no data to read */
            break;

        default:
            avr_warning ("Unknown request from gdb: %c (0x%02x)\n", c, c);
    }

    return GDB_RET_OK;
}

static void
gdb_main_loop (GdbComm_T *comm, int fd)
{
    int res;
    char reply[MAX_BUF];

    while (1)
    {
        res = gdb_pre_parse_packet (comm, fd, GDB_BLOCKING_ON);
        switch (res)
        {
            case GDB_RET_KILL_REQUEST:
                return;

            case GDB_RET_CTRL_C:
                gdb_send_ack (fd);
                snprintf (reply, MAX_BUF, "S%02x", SIGINT);
                gdb_send_reply (fd, reply);
                break;

            default:
                break;
        }
    }
}

/**
 * \brief Start interacting with gdb.
 * \param comm     A previously initialized simulator comm
 * \param port     Port which server will listen for connections on.
 * \param debug_on Turn on gdb debug diagnostic messages.
 * 
 * Start a tcp server socket on localhost listening for connections on the
 * given port. Once a connection is established, enter an infinite loop and
 * process command requests from gdb using the remote serial protocol. Only a
 * single connection is allowed at a time.
 */
void
gdb_interact (GdbComm_T *comm, int port, int debug_on)
{
    struct sockaddr_in address[1];
    int sock, conn, i;
    socklen_t addrLength[1];

    global_debug_on = debug_on;

    if ((sock = socket (PF_INET, SOCK_STREAM, 0)) < 0)
        avr_error ("Can't create socket: %s", strerror (errno));

    /* Let the kernel reuse the socket address. This lets us run
       twice in a row, without waiting for the (ip, port) tuple
       to time out. */
    i = 1;
    setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof (i));

    address->sin_family = AF_INET;
    address->sin_port = htons (port);
    memset (&address->sin_addr, 0, sizeof (address->sin_addr));

    if (bind (sock, (struct sockaddr *)address, sizeof (address)))
        avr_error ("Can not bind socket: %s", strerror (errno));

    signal_watch_start (SIGINT);

    while (global_server_quit == 0)
    {
        if (listen (sock, 1))
        {
            int saved_errno = errno;

            if (signal_has_occurred (SIGINT))
            {
                break;          /* SIGINT will cause listen to be
                                   interrupted */
            }
            avr_error ("Can not listen on socket: %s",
                       strerror (saved_errno));
        }

        fprintf (stderr, "Waiting on port %d for gdb client to connect...\n",
                 port);

        /* accept() needs this set, or it fails (sometimes) */
        addrLength[0] = sizeof (struct sockaddr);

        /* We only want to accept a single connection, thus don't need a
           loop. */
        conn = accept (sock, (struct sockaddr *)address, addrLength);
        if (conn < 0)
        {
            int saved_errno = errno;

            if (signal_has_occurred (SIGINT))
            {
                break;          /* SIGINT will cause accept to be
                                   interrupted */
            }
            avr_error ("Accept connection failed: %s",
                       strerror (saved_errno));
        }

        /* Tell TCP not to delay small packets.  This greatly speeds up
           interactive response. WARNING: If TCP_NODELAY is set on, then gdb
           may timeout in mid-packet if the (gdb)packet is not sent within a
           single (tcp)packet, thus all outgoing (gdb)packets _must_ be sent
           with a single call to write. (see Stevens "Unix Network
           Programming", Vol 1, 2nd Ed, page 202 for more info) */

        i = 1;
        setsockopt (conn, IPPROTO_TCP, TCP_NODELAY, &i, sizeof (i));

        /* If we got this far, we now have a client connected and can start 
           processing. */

        fprintf (stderr, "Connection opened by host %s, port %hd.\n",
                 inet_ntoa (address->sin_addr), ntohs (address->sin_port));

        gdb_main_loop (comm, conn);

        comm->reset (comm->user_data);

        close (conn);

        /* FIXME: How do we correctly break out of this loop? This keeps the
           simulator server up so you don't have to restart it with every gdb
           session. To exit the server loop, you have to hit C-c or send some
           signal which causes the program to terminate, in which case, you
           won't get a dump of the simulator's state. This might actually be
           acceptable behavior. */
        if (signal_has_occurred (SIGINT))
        {
            break;
        }
    }

    signal_watch_stop (SIGINT);

    close (sock);
}
