/*
 * $Id: stack.c,v 1.15 2004/01/30 07:09:56 troth Exp $
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

/** \file ee.c
    \brief Module for the definition of the stack. 

    Defines the classes stack, hw_stack, and mem_stack.

    FIXME: Ted, I would really really really love to put in a description of
    what is the difference between these three classes and how they're used,
    but I don't understand it myself. */

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

// OsEID
typedef struct _Oseid Oseid;

struct _Oseid
{
  VDevice parent;
  uint16_t addr;
// display data:
// write 0 to FIFOCTRL (reset FIFO)
// push data to FIFO
// write 1 to FIFOCTRL => FIFO  to stdout

// read stdin:
// write 0 to FIFOCTRL (reset FIFO)
// write 2 to FIFOCTRL (this block  until user type input line)
// pop FIFO - in rx,FIFO wait until enough data is readed

  uint8_t FIFO;
  uint8_t FIFOCTRL;

  uint8_t fifo[256];
  uint8_t flen;
};

static Oseid *oseid_new (int addr, char *name);
static void oseid_construct (Oseid * oseid, int addr, char *name);
static void oseid_destroy (void *sp);
static uint8_t oseid_read (VDevice * dev, int addr);
static void oseid_write (VDevice * dev, int addr, uint8_t val);
static void oseid_reset (VDevice * dev);
static void oseid_add_addr (VDevice * vdev, int addr, char *name,
			    int rel_addr, void *data);

VDevice *
oseid_create (int addr, char *name, int rel_addr, void *data)
{
  return (VDevice *) oseid_new (addr, name);
}

static Oseid *
oseid_new (int addr, char *name)
{
  Oseid *oseid;
  oseid = avr_new (Oseid, 1);
  oseid_construct (oseid, addr, name);
  class_overload_destroy ((AvrClass *) oseid, oseid_destroy);
  return oseid;
}

static void
oseid_construct (Oseid * oseid, int addr, char *name)
{
  if (oseid == NULL)
    avr_error ("passed null ptr");

  vdev_construct ((VDevice *) oseid, oseid_read, oseid_write, oseid_reset,
		  oseid_add_addr);
  oseid_add_addr ((VDevice *) oseid, addr, name, 0, NULL);
  oseid_reset ((VDevice *) oseid);
}

static void
oseid_destroy (void *oseid)
{
  if (oseid == NULL)
    return;
  vdev_destroy (oseid);
}

static uint8_t
oseid_read (VDevice * dev, int addr)
{
  Oseid *oseid = (Oseid *) dev;
  uint8_t len = oseid->flen;

  if (addr == (oseid->addr) + 1)
    {
      avr_message ("input len %d\n", oseid->flen);
      oseid->flen = 0;
      return len;
    }
  if (addr == (oseid->addr) + 0)
    return oseid->fifo[oseid->flen++];

  avr_error ("Bad address: 0x%04x", addr);
  return 0;
}

static void
oseid_write (VDevice * dev, int addr, uint8_t val)
{
  Oseid *oseid = (Oseid *) dev;
  int i;

  if (addr == (oseid->addr) + 1)
    {
      if (val == 0)
	oseid->flen = 0;
      if (val == 1)
	{
	  printf ("< ");
	  for (i = 0; i < oseid->flen; i++)
	    printf ("%02x ", oseid->fifo[i]);
	  printf ("\n");
	  oseid->flen = 0;
	}
      if (val == 2)
	{
	  char buffer[1024];
	  int val;
	  char *pos;
	  printf ("> ");

	  oseid->flen = 0;
	  fflush (stdin);
	  while (buffer != fgets (buffer, 1024, stdin));
	  pos = buffer;

	  while (1 == sscanf (pos, "%2x ", &val))
	    {
	      if (pos > buffer + 1024)
		break;
	      pos += 3;
	      oseid->fifo[oseid->flen] = val;
	      oseid->flen++;
	    }
	}
      if (val == 3)
	{
	  FILE *f;
	  f = fopen ("/dev/urandom", "r");
	  oseid->fifo[0] = fgetc (f);
	  avr_message ("RND data %02x\n", oseid->fifo[0]);
	  fclose (f);
	  oseid->flen = 0;
	}

    }
  else if (addr == (oseid->addr) + 0)
    oseid->fifo[oseid->flen++] = val;
  else
    avr_error ("Bad address: 0x%04x (want %x)", addr, oseid->addr);
}

static void
oseid_reset (VDevice * dev)
{
  Oseid *oseid = (Oseid *) dev;
  memset (oseid->fifo, 0, 256);
  avr_message ("OsEID fifo reset\n");
}

static void
oseid_add_addr (VDevice * vdev, int addr, char *name, int rel_addr,
		void *data)
{
  Oseid *oseid = (Oseid *) vdev;

  if (strncmp ("FIFOCTRL", name, 8) == 0)
    ;
  else if (strncmp ("FIFO", name, 4) == 0)
    oseid->addr = addr;

  else
    avr_error ("Bad address: 0x%04x %s", addr, name);
  avr_message ("setting addres %x\n", oseid->addr);
}


/****************************************************************************\
 *
 * EEprom (VDevice) Definition.
 *
\****************************************************************************/

#ifndef DOXYGEN			/* don't expose to doxygen */
typedef struct _EEprom EEprom;

struct _EEprom
{
  VDevice parent;

  uint16_t addr;

  uint8_t EECR, EEDR, EEARL, EEARH;
  uint8_t mem[4096];		// atmega128, atmega1284
};
#endif
static EEprom *ee_new (int addr, char *name);
static void ee_construct (EEprom * ee, int addr, char *name);
static void ee_destroy (void *sp);
static uint8_t ee_read (VDevice * dev, int addr);
static void ee_write (VDevice * dev, int addr, uint8_t val);
static void ee_reset (VDevice * dev);
static void ee_add_addr (VDevice * vdev, int addr, char *name, int rel_addr,
			 void *data);

// dirty access to eeprom mem  from gdb
uint8_t *ee_mem;
uint8_t gdb_ee_read(int adr){
  if(!ee_mem)
    return 0;
  if(adr < 4096)
    return(ee_mem[adr]);
  return 0;
}
void  gdb_ee_write(int adr,uint8_t data){
  if(!ee_mem)
    return;
   if(adr < 4096)
     ee_mem[adr]=data;
  return;
}

VDevice *
ee_create (int addr, char *name, int rel_addr, void *data)
{
  return (VDevice *) ee_new (addr, name);
}

static EEprom *
ee_new (int addr, char *name)
{
  EEprom *ee;

  ee = avr_new (EEprom, 1);
  ee_construct (ee, addr, name);
  class_overload_destroy ((AvrClass *) ee, ee_destroy);
  ee_mem=ee->mem;
  return ee;
}

static void
ee_construct (EEprom * ee, int addr, char *name)
{
  if (ee == NULL)
    avr_error ("passed null ptr");

  vdev_construct ((VDevice *) ee, ee_read, ee_write, ee_reset, ee_add_addr);
  ee_add_addr ((VDevice *) ee, addr, name, 0, NULL);
  ee_reset ((VDevice *) ee);
}

static void
ee_destroy (void *ee)
{
  if (ee == NULL)
    return;
  vdev_destroy (ee);
}

static uint8_t
ee_read (VDevice * dev, int addr)
{
  EEprom *ee = (EEprom *) dev;

  if (addr == (ee->addr) + 1)
    return ee->EEDR;

  if (addr == (ee->addr) + 2)
    return ee->EEARL;

  if (addr == (ee->addr) + 3)
    return ee->EEARH & 15;

  if (addr == (ee->addr) + 0)
    {
      return 0;
    }

  avr_error ("Bad address: 0x%04x", addr);

  return 0;
}

static void
ee_write (VDevice * dev, int addr, uint8_t val)
{
  EEprom *ee = (EEprom *) dev;

  if (addr == (ee->addr) + 2)
    ee->EEARL = val;
  else if (addr == (ee->addr) + 3)
    ee->EEARH = val;
  else if (addr == (ee->addr) + 1)
    ee->EEDR = val;
  else if (addr == (ee->addr) + 0)
    {
      if (val == 1)
	{
	  ee->EEDR = ee->mem[((ee->EEARH << 8 | ee->EEARL) & 0xfff)];
/*
	  avr_message ("triggered read from  0x%04x (%02x)\n",
		       (ee->EEARH << 8 | ee->EEARL) & 0xfff, ee->EEDR);
*/	  
	  return;
	}
      if (val & 2)
	{

	  avr_message ("triggered write to  0x%04x (%02x)\n",
		       (ee->EEARH << 8 | ee->EEARL) & 0xfff, ee->EEDR);
	  ee->mem[((ee->EEARH << 8 | ee->EEARL) & 0xfff)] = ee->EEDR;
	}
    }
  else
    avr_error ("Bad address: 0x%04x", addr);
}

static void
ee_reset (VDevice * dev)
{
  EEprom *ee = (EEprom *) dev;
  memset (ee->mem, 0xff, 4096);
  avr_message ("EEprom reset\n");
}

static void
ee_add_addr (VDevice * vdev, int addr, char *name, int rel_addr, void *data)
{
  EEprom *ee = (EEprom *) vdev;

  if (strncmp ("EECR", name, 4) == 0)
    ee->addr = addr;

  else if (strncmp ("EEDR", name, 4) == 0)
    ;
  else if (strncmp ("EEARL", name, 5) == 0)
    ;
  else if (strncmp ("EEARH", name, 5) == 0)
    ;
  else
    avr_error ("Bad address: 0x%04x %s", addr, name);
}
