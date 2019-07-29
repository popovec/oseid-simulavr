/*
 * $Id: vdevs.h,v 1.31 2004/01/30 07:09:56 troth Exp $
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

#ifndef SIM_VDEVS_H
#define SIM_VDEVS_H

/****************************************************************************\
 *
 * Virtual Device Definition.
 *
\****************************************************************************/

typedef struct _VDevice VDevice;

typedef VDevice *(*VDevCreate) (int addr, char *name, int rel_addr,
                                void *data);

typedef uint8_t (*VDevFP_Read) (VDevice *dev, int addr);
typedef void (*VDevFP_Write) (VDevice *dev, int addr, uint8_t val);
typedef void (*VDevFP_Reset) (VDevice *dev);
typedef void (*VDevFP_AddAddr) (VDevice *dev, int addr, char *name,
                                int rel_addr, void *data);

struct _VDevice
{
    AvrClass parent;
    AvrClass *core;             /* keep a pointer to the core the device is
                                   attached to */
    VDevFP_Read read;           /* read access for device */
    VDevFP_Write write;         /* write access for device */
    VDevFP_Reset reset;         /* reset function for device */
    VDevFP_AddAddr add_addr;    /* add an address to the list of addresses
                                   handled by this vdev */
};

extern VDevice *vdev_new (char *name, VDevFP_Read rd, VDevFP_Write wr,
                          VDevFP_Reset reset, VDevFP_AddAddr add_addr);
extern void vdev_construct (VDevice *dev, VDevFP_Read rd, VDevFP_Write wr,
                            VDevFP_Reset reset, VDevFP_AddAddr add_addr);
extern void vdev_destroy (void *dev);

extern int vdev_name_cmp (AvrClass *c1, AvrClass *c2);
extern int vdev_addr_cmp (AvrClass *c1, AvrClass *c2);

extern uint8_t vdev_read (VDevice *dev, int addr);
extern void vdev_write (VDevice *dev, int addr, uint8_t val);
extern void vdev_reset (VDevice *dev);

extern void vdev_set_core (VDevice *dev, AvrClass *core);

extern inline AvrClass *vdev_get_core (VDevice *dev)
{
    return dev->core;
}

extern void vdev_add_addr (VDevice *dev, int addr, char *name, int rel_addr,
                           void *data);
extern void vdev_def_AddAddr (VDevice *dev, int addr, char *name, int rel_addr,
                              void *data);

/****************************************************************************\
 *
 * Include Virtual Device Definitions
 *
\****************************************************************************/

#if 1                           /* Make these go away! */
enum __io_reg_constants
{
    IO_REG_ADDR_BEGIN = 0x20,
    IO_REG_ADDR_END = 0x60,
};
#endif

#endif /* SIM_VDEVS_H */
