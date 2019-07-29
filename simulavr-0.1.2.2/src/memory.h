/*
 * $Id: memory.h,v 1.5 2004/01/30 07:09:56 troth Exp $
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

#ifndef SIM_MEMORY_H
#define SIM_MEMORY_H

typedef struct _MemoryCell MemoryCell;

struct _MemoryCell {
    char *name;
    VDevice *vdev;
    int flags;
    uint8_t reset_value;
    uint8_t rd_mask;
    uint8_t wr_mask;
};

/****************************************************************************\
 *
 * Memory(AvrClass) Definition.
 *
\****************************************************************************/

typedef struct _Memory Memory;

struct _Memory
{
    AvrClass parent;
    int gpwr_end;               /* Usually 0x1f. */
    int io_reg_end;             /* Usually 0x5f or 0xff. */
    int sram_end;               /* Depends on the device. XRAM starts directly
                                   after. This may be the same as io_reg_end
                                   if the device has no sram. */
    int xram_end;               /* The last address of external memory. If the
                                   device doesn't support the XRAM feature,
                                   this should be set to the same value as
                                   sram_end. */

    MemoryCell *cell;           /* Dynamically allocated to len xram_end+1. */
};

extern Memory *mem_new (int gpwr_end, int io_reg_end, int sram_end,
                        int xram_end);
extern void mem_construct (Memory *mem, int gpwer_end, int io_reg_end,
                           int sram_end, int xram_end);
extern void mem_destroy (void *mem);

extern void mem_attach (Memory *mem, int addr, char *name, VDevice *vdev,
                        int flags, uint8_t reset_value, uint8_t rd_mask,
                        uint8_t wr_mask);

extern VDevice *mem_get_vdevice_by_addr (Memory *mem, int addr);
extern VDevice *mem_get_vdevice_by_name (Memory *mem, char *name);
extern void mem_set_addr_name (Memory *mem, int addr, char *name);

extern uint8_t mem_read (Memory *mem, int addr);
extern void mem_write (Memory *mem, int addr, uint8_t val);
extern void mem_reset (Memory *mem);

extern void mem_io_fetch (Memory *mem, int addr, uint8_t * val, char *buf,
                          int bufsiz);
extern void mem_dump_core (Memory *mem, FILE * f_core);

#endif /* SIM_MEMORY_H */
