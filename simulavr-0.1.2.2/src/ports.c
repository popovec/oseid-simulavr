/*
 * $Id: ports.c,v 1.17 2004/01/30 07:09:56 troth Exp $
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
 * \file ports.c
 * \brief Module for accessing simulated I/O ports.
 *
 * Defines an abstract Port class as well as subclasses for each individual
 * port.
 *
 * \todo Remove the pins argument and the mask field. That's handled at a
 * higher level so is obsolete here now.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/****************************************************************************\
 *
 * Local static prototypes.
 *
\****************************************************************************/

static Port *port_new (int addr, char *name);
static void port_construct (Port *p, int addr, char *name);
static void port_destroy (void *p);

static uint8_t port_reg_read (VDevice *dev, int addr);
static void port_reg_write (VDevice *dev, int addr, uint8_t val);
static void port_reset (VDevice *dev);

static uint8_t port_read_pin (Port *p, int addr);

static void port_write_port (Port *p, int addr, uint8_t val);

static void port_write_ddr (Port *p, int addr, uint8_t val);

static void port_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                           void *data);

/****************************************************************************\
 *
 * Port(VDevice) : I/O Port registers
 *
\****************************************************************************/

/**
 * \brief Create a new Port instance.
 *
 * This should only be used in DevSuppDefn initializers.
 */

VDevice *
port_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)port_new (addr, name);
}

/**
 * \brief Allocates a new Port object.
 */

static Port *
port_new (int addr, char *name)
{
    Port *p;

    p = avr_new0 (Port, 1);
    port_construct (p, addr, name);
    class_overload_destroy ((AvrClass *)p, port_destroy);

    return p;
}

/**
 * \brief Constructor for the Port object.
 */

static void
port_construct (Port *p, int addr, char *name)
{
    if (p == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)p, port_reg_read, port_reg_write, port_reset,
                    port_add_addr);

    port_add_addr ((VDevice *)p, addr, name, 0, NULL);

    p->ext_rd = NULL;
    p->ext_wr = NULL;

    port_reset ((VDevice *)p);
}

static void
port_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    Port *p = (Port *)vdev;

    if (strncmp ("PORT", name, 4) == 0)
    {
        p->port_addr = addr;
    }

    else if (strncmp ("DDR", name, 3) == 0)
    {
        p->ddr_addr = addr;
    }

    else if (strncmp ("PIN", name, 3) == 0)
    {
        p->pin_addr = addr;
    }

    else
    {
        avr_error ("invalid port register name: '%s' @ 0x%04x", name, addr);
    }
}

static void
port_reset (VDevice *dev)
{
    Port *p = (Port *)dev;

    p->port = 0;
    p->ddr = 0;
    p->pin = 0;

    p->ext_enable = 1;
}

/**
 * \brief Destructor for the Port object
 *
 * This is a virtual method for higher level port implementations and as such
 * should not be used directly.
 */
void
port_destroy (void *p)
{
    if (p == NULL)
        return;

    vdev_destroy (p);
}

/** \brief Disable external port functionality.
 *
 * This is only used when dumping memory to core file. See mem_io_fetch().
 */
void
port_ext_disable (Port *p)
{
    p->ext_enable = 0;
}

/** \brief Enable external port functionality.
 *
 * This is only used when dumping memory to core file. See mem_io_fetch().
 */
void
port_ext_enable (Port *p)
{
    p->ext_enable = 1;
}

/**
 * \brief Attaches read and write functions to a particular port
 *
 * I think I may have this backwards. Having the virtual hardware supply
 * functions for the core to call on every io read/write, might cause missed
 * events (like edge triggered). I'm really not too sure how to handle this.
 *
 * In the future, it might be better to have the core supply a function for
 * the virtual hardware to call when data is written to the device. The device
 * supplied function could then check if an interrupt should be generated or
 * just simply write to the port data register.
 *
 * For now, leave it as is since it's easier to test if you can block when the
 * device is reading from the virtual hardware.
 */
void
port_add_ext_rd_wr (Port *p, PortFP_ExtRd ext_rd, PortFP_ExtWr ext_wr)
{
    p->ext_rd = ext_rd;
    p->ext_wr = ext_wr;
}

static uint8_t
port_read_pin (Port *p, int addr)
{
    uint8_t data;

    /* get the data from the external virtual hardware if connected */
    if (p->ext_rd && p->ext_enable)
        data = p->ext_rd (addr);
    else
        data = 0;

    /*
     * For a pin n to be enabled as input, DDRn == 0,
     * otherwise it will always read 0.
     */
    data &= ~(p->ddr);

    /*
     * Pass data to alternate read so as to check alternate functions of
     * pins for that port.
     */
/*      if (p->alt_rd) */
/*          data = p->alt_rd(p, addr, data); */

    return data;
}

static void
port_write_port (Port *p, int addr, uint8_t val)
{
    /* update port register */
    p->port = val;

    /*
     * Since changing p->port might change what the virtual hardware
     * sees, we need to call ext_wr() to pass change along.
     */
    if (p->ext_wr && p->ext_enable)
        p->ext_wr (addr, (p->port & p->ddr));
}

static void
port_write_ddr (Port *p, int addr, uint8_t val)
{
    /* update ddr register */
    p->ddr = val;

#if 0
    /*
     * Since changing p->ddr might change what the virtual hardware
     * sees, we need to call ext_wr() to pass change allong.
     */
    if (p->ext_wr && p->ext_enable)
        p->ext_wr (addr, (p->port & p->ddr));
#endif
}

static uint8_t
port_reg_read (VDevice *dev, int addr)
{
    Port *p = (Port *)dev;

    if (addr == p->ddr_addr)
        return p->ddr;

    else if (addr == p->pin_addr)
        return port_read_pin (p, addr);

    else if (addr == p->port_addr)
        return p->port;

    else
        avr_error ("Invalid Port Address: 0x%02x", addr);

    return 0;
}

static void
port_reg_write (VDevice *dev, int addr, uint8_t val)
{
    Port *p = (Port *)dev;

    if (addr == p->pin_addr)
    {
        avr_warning ("Attempt to write to readonly PINx register\n");
    }

    else if (addr == p->ddr_addr)
    {
        port_write_ddr ((Port *)p, addr, val);
    }

    else if (addr == p->port_addr)
    {
        port_write_port ((Port *)p, addr, val);
    }

    else
    {
        avr_error ("Invalid Port Address: 0x%02x", addr);
    }
}

