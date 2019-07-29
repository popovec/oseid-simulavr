/*
 * $Id: devsupp.h,v 1.7 2004/01/30 07:09:56 troth Exp $
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

#ifndef SIM_DEVSUPP_H
#define SIM_DEVSUPP_H

typedef struct _DevSuppDefn DevSuppDefn;

extern void dev_supp_list_devices (FILE * fp);
extern DevSuppDefn *dev_supp_lookup_device (char *dev_name);
extern void dev_supp_attach_io_regs (AvrCore *core, DevSuppDefn *dev);

extern int dev_supp_get_flash_sz (DevSuppDefn *dev);
extern int dev_supp_get_sram_sz (DevSuppDefn *dev);
extern int dev_supp_get_PC_sz (DevSuppDefn *dev);
extern int dev_supp_get_stack_sz (DevSuppDefn *dev);
extern int dev_supp_get_vtab_idx (DevSuppDefn *dev);

extern int dev_supp_has_ext_io_reg (DevSuppDefn *dev);

#endif /* SIM_DEVSUPP_H */
