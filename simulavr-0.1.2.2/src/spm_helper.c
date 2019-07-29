/*
 * $Id: src/spm_helper.c,v 1.15 2017/06/27 07:09:56 troth Exp $
 *
 ****************************************************************************
 * implementation of SPM insturction (flash write operation)
 * Copyright (C) 2017 Peter Popovec, popovec.peter@gmail.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************
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

#include "display.h"
#include "spm_helper.h"

///////////////////////////////////////////

static SPMdata *spm_new (int addr, char *name);
static void spm_construct (SPMdata * ee, int addr, char *name);
static void spm_destroy (void *sp);
static uint8_t spm_read (VDevice * dev, int addr);
static void spm_write (VDevice * dev, int addr, uint8_t val);
static void spm_reset (VDevice * dev);
static void spm_add_addr (VDevice * vdev, int addr, char *name, int rel_addr,
			  void *data);

VDevice *
spm_create (int addr, char *name, int rel_addr, void *data)
{
  return (VDevice *) spm_new (addr, name);
}

static SPMdata *
spm_new (int addr, char *name)
{
  SPMdata *spm;

  spm = avr_new (SPMdata, 1);
  spm_construct (spm, addr, name);
  class_overload_destroy ((AvrClass *) spm, spm_destroy);

  return spm;
}

static void
spm_construct (SPMdata * spm, int addr, char *name)
{
  if (spm == NULL)
    avr_error ("passed null ptr");

  vdev_construct ((VDevice *) spm, spm_read, spm_write, spm_reset,
		  spm_add_addr);

  spm_add_addr ((VDevice *) spm, addr, name, 0, NULL);

  spm_reset ((VDevice *) spm);
}

static void
spm_destroy (void *spm)
{
  if (spm == NULL)
    return;

  vdev_destroy (spm);
}

static uint8_t
spm_read (VDevice * dev, int addr)
{
  SPMdata *spm = (SPMdata *) dev;
  AvrCore *core = (AvrCore *) vdev_get_core ((VDevice *) dev);
  if (addr == (spm->addr) + 0)
    return core->spmhelper->SPMCSR;
  avr_error ("Bad address: 0x%04x", addr);

  return 0;
}

static void
spm_write (VDevice * dev, int addr, uint8_t val)
{
  SPMdata *spm = (SPMdata *) dev;
  AvrCore *core = (AvrCore *) vdev_get_core ((VDevice *) dev);

  if (val & 0x80)
    avr_error ("SPMCSR does not support SPMIE bit 0x%02x", val);
  if (val & 0x20)
    avr_error ("SPMCSR not used bit 0x%02x", val);
  if (val & 0x08)
    avr_error ("SPMCSR not support BLBSET bit 0x%02x", val);

  switch (val)
    {
    case 0x01:
    case 0x11:
    case 0x09:
    case 0x05:
    case 0x03:
      break;
    default:
      avr_error ("SPMCSR operation without any effect\n");
    }

  if (addr == (spm->addr))
    core->spmhelper->SPMCSR = val;
  else
    avr_error ("Bad address: 0x%04x", addr);
}

static void
spm_reset (VDevice * dev)
{
//  AvrCore *core = (AvrCore *) vdev_get_core ((VDevice *) dev);

//  core->spmhelper->SPMCSR = 0;
}

static void
spm_add_addr (VDevice * vdev, int addr, char *name, int rel_addr, void *data)
{
  SPMdata *spm = (SPMdata *) vdev;

  if (strncmp ("SPMCSR", name, 6) == 0)
    spm->addr = addr;
  else
    avr_error ("Bad address: 0x%04x %s", addr, name);
}


void
spm_run (SPMhelper * spmhelper, int reg0, int reg1, int Z)
{
/*
  avr_message ("running SPM r0=0x%02x r1=0x%02x Z=%d SPMCSR=0x%02x\n", reg0,
	       reg1, Z, spmhelper->SPMCSR);
*/
  if (spmhelper->SPMCSR == 0x11)
    {
      //reenable RWW
      avr_message ("SPM reenable RWW\n");
      memset (spmhelper->page_buffer, 0xff, 256);
      spmhelper->SPMCSR = 0;
      return;
    }
  if (spmhelper->SPMCSR == 0x01)
    {
      // write page buffer
      Z &= 0xfe;
//      avr_message ("SPM write page buffer %d [%02x %02x]\n", Z, reg0, reg1);
      spmhelper->page_buffer[Z] = reg0;
      spmhelper->page_buffer[Z + 1] = reg1;
      spmhelper->SPMCSR = 0;
      return;
    }
  if (spmhelper->SPMCSR == 0x03)
    {
      // page erase
      int i;
      Z >>= 1;
      Z &= 0xff80;
      avr_message ("SPM page erase page %d\n", Z);

      for (i = 0; i < 128; i++)
	flash_write (spmhelper->flash, Z + i, 0xffff);

      spmhelper->SPMCSR = 0;
      return;
    }
  if (spmhelper->SPMCSR == 0x05)
    {
      // page write
      int i, f;
      uint8_t hi, lo;
      Z >>= 1;
      Z &= 0xff80;

      avr_message ("SPM page write %d\n", Z);
      for (i = 0; i < 128; i++)
	{
	  f = flash_read (spmhelper->flash, Z + i);
	  lo = f & 0xff;
	  hi = f >> 8;
	  lo &= spmhelper->page_buffer[i * 2];
	  hi &= spmhelper->page_buffer[i * 2 + 1];
	  
	  flash_write (spmhelper->flash, Z + i, hi << 8 | lo);
	}
      spmhelper->SPMCSR = 0;
      return;
    }
  avr_error ("SPM unknown operation %02x\n", spmhelper->SPMCSR);
}

static void spmhelper_construct (SPMhelper * spmhelper, Flash * flash);
static void spmhelper_destroy (void *spm);

SPMhelper *
spmhelper_new (Flash * flash)
{
  SPMhelper *spmhelper;

  spmhelper = avr_new (SPMhelper, 1);
  spmhelper_construct (spmhelper, flash);
  class_overload_destroy ((AvrClass *) spmhelper, spmhelper_destroy);

  return spmhelper;
}


static void
spmhelper_construct (SPMhelper * spmhelper, Flash * flash)
{
  if (spmhelper == NULL)
    avr_error ("passed null ptr");
  spmhelper->flash = flash;

}

static void
spmhelper_destroy (void *spm)
{
  if (spm == NULL)
    return;

  vdev_destroy (spm);
}
