/*
 * $Id: ports.h,v 1.7 2004/01/30 07:09:56 troth Exp $
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

#ifndef SIM_PORTS_H
#define SIM_PORTS_H

/****************************************************************************\
 *
 * Port(VDevice) : I/O Port registers
 *
\****************************************************************************/

enum _port_constants
{
    PORT_A = 0,
    PORT_B = 1,
    PORT_C = 2,
    PORT_D = 3,
    PORT_E = 4,
    PORT_F = 5,

    PORT_A_BASE = 0x39,         /* Base Memory address for port */
    PORT_B_BASE = 0x36,         /* Base Memory address for port */
    PORT_C_BASE = 0x33,         /* Base Memory address for port */
    PORT_D_BASE = 0x30,         /* Base Memory address for port */

    /* NOTE: these are only valid addresses for the USB devices. */
    PORT_E_BASE = 0x21,         /* Base Memory address for port */
    PORT_F_BASE = 0x24,         /* Base Memory address for port */

    PORT_PIN = 0,               /* offset to pin io                */
    PORT_DDR = 1,               /* offset to ddr register          */
    PORT_PORT = 2,              /* offset to data register (PORTx) */

    PORT_SIZE = 3,              /* All ports use 3 registers: PINx, DDRx,
                                   PORTx */

    PORT_1_BIT = 1,             /* Some ports are 1 bits wide */
    PORT_2_BIT = 2,             /* Some ports are 2 bits wide */
    PORT_3_BIT = 3,             /* Some ports are 3 bits wide */
    PORT_4_BIT = 4,             /* Some ports are 4 bits wide */
    PORT_5_BIT = 5,             /* Some ports are 5 bits wide */
    PORT_6_BIT = 6,             /* Some ports are 6 bits wide */
    PORT_7_BIT = 7,             /* Some ports are 7 bits wide */
    PORT_8_BIT = 8,             /* Most ports are 8 bits wide */
};

/* Generic I/O Port */

typedef struct _Port Port;

/* Hooks for external device connections */
typedef uint8_t (*PortFP_ExtRd) (int addr);
typedef void (*PortFP_ExtWr) (int addr, uint8_t val);

struct _Port
{
    VDevice parent;

    uint16_t port_addr;         /* port data register address */
    uint16_t ddr_addr;          /* data direction register address */
    uint16_t pin_addr;          /* input register address */

    uint8_t port;               /* port data register */
    uint8_t ddr;                /* data direction register */
    uint8_t pin;                /* input register */

    int ext_enable;             /* allows disabling external read functions */

    PortFP_ExtRd ext_rd;        /* hook to read from external device via
                                   port */
    PortFP_ExtWr ext_wr;        /* hook to write to external device via
                                   port */
};

extern VDevice *port_create (int addr, char *name, int rel_addr, void *data);

extern void port_add_ext_rd_wr (Port *p, PortFP_ExtRd ext_rd,
                                PortFP_ExtWr ext_wr);

extern void port_ext_enable (Port *p);
extern void port_ext_disable (Port *p);

#endif /* SIM_PORTS_H */
