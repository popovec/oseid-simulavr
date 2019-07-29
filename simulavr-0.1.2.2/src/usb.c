/*
 * $Id: usb.c,v 1.3 2004/03/13 19:55:34 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2003, 2004  Keith Gudger
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
 * \file usb.c
 * \brief Module to simulate the AVR's USB module.
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
#include "usb.h"

#include "intvects.h"

void usb_port_wr (char *name, uint8_t val);
uint8_t usb_port_rd (char *name);

/*****************************************************************************\
 *
 * USB Interrupts 
 *
\*****************************************************************************/

static void usbi_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                           void *data);
static uint8_t usb_intr_read (VDevice *dev, int addr);
static void usb_intr_write (VDevice *dev, int addr, uint8_t val);
static void usb_intr_reset (VDevice *dev);
static char *usb_intr_reg_name (VDevice *dev, int addr);

/** \brief Allocate a new USB interrupt */

/*  return (VDevice *)usb_intr_new (addr, name, (uint8_t) *data_ptr);
  }*/

VDevice *
usbi_create (int addr, char *name, int rel_addr, void *data)
{
    uint8_t *data_ptr = (uint8_t *) data;
    if (data)
        return (VDevice *)usb_intr_new (addr, name, (uint8_t) * data_ptr);
    else
        avr_error ("Attempted USB interrupt create with NULL data pointer");
    return 0;
}

USBInter_T *
usb_intr_new (int addr, char *name, uint8_t func_mask)
{
    USBInter_T *usb;

    usb = avr_new (USBInter_T, 1);
    usb_intr_construct (usb, addr, name, func_mask);
    class_overload_destroy ((AvrClass *)usb, usb_intr_destroy);

    return usb;
}

/** \brief Constructor for usb interrupt object. */

void
usb_intr_construct (USBInter_T *usb, int addr, char *name, uint8_t func_mask)
{
    if (usb == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)usb, usb_intr_read, usb_intr_write,
                    usb_intr_reset, usbi_add_addr);

    usb->func_mask = func_mask;
    usbi_add_addr ((VDevice *)usb, addr, name, 0, NULL);
    usb_intr_reset ((VDevice *)usb);
}

static void
usbi_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    USBInter_T *usb = (USBInter_T *)vdev;

    if (strncmp ("UIER", name, 4) == 0)
    {
        usb->uier_addr = addr;
    }

    else if (strncmp ("UIAR", name, 4) == 0)
    {
        usb->uiar_addr = addr;
    }

    else if (strncmp ("UIMSK", name, 5) == 0)
    {
        usb->uimsk_addr = addr;
    }

    else if (strncmp ("UISR", name, 4) == 0)
    {
        usb->uisr_addr = addr;
    }

    else if (strncmp ("SPRSMSK", name, 7) == 0)
    {
        usb->sprsmsk_addr = addr;
    }

    else if (strncmp ("SPRSIE", name, 6) == 0)
    {
        usb->sprsie_addr = addr;
    }

    else if (strncmp ("SPRSR", name, 5) == 0)
    {
        usb->sprsr_addr = addr;
    }

    else if (strncmp ("GLB_STATE", name, 9) == 0)
    {
        usb->glb_state_addr = addr;
    }

    else if (strncmp ("FRM_NUM_L", name, 9) == 0)
    {
        usb->frm_num_l_addr = addr;
    }

    else if (strncmp ("FRM_NUM_H", name, 9) == 0)
    {
        usb->frm_num_h_addr = addr;
    }

    else
    {
        avr_error ("invalid USB Int register name: '%s' @ 0x%04x", name,
                   addr);
    }
}

/** \brief Destructor for usb interrupt object. */

void
usb_intr_destroy (void *usb)
{
    if (usb == NULL)
        return;

    vdev_destroy (usb);
}

static uint8_t
usb_intr_read (VDevice *dev, int addr)
{
    USBInter_T *usb = (USBInter_T *)dev;

    if (addr == usb->uier_addr)
        return (usb->uier);
    else if (addr == usb->uimsk_addr)
        return (usb->uimsk);
    else if (addr == usb->uisr_addr)
        return (usb->uisr =
                usb_port_rd (usb_intr_reg_name ((VDevice *)usb, addr)));
    else if (addr == usb->sprsie_addr)
        return (usb->sprsie);
    else if (addr == usb->sprsr_addr)
        return (usb->sprsr =
                usb_port_rd (usb_intr_reg_name ((VDevice *)usb, addr)));
    else if (addr == usb->glb_state_addr)
        return (usb->glb_state);
    else if (addr == usb->frm_num_l_addr)
        return (usb->frm_num_l);
    else if (addr == usb->frm_num_h_addr)
        return (usb->frm_num_h);
    else if (addr == usb->sprsmsk_addr)
        return (usb->sprsmsk);
    else
        avr_error ("Bad address: 0x%04x", addr);

    return 0;                   /* will never get here */
}

static void
usb_intr_write (VDevice *dev, int addr, uint8_t val)
{
    USBInter_T *usb = (USBInter_T *)dev;

    if (addr == usb->uier_addr)
        (usb->uier = val);
    else if (addr == usb->uimsk_addr)
        (usb->uimsk = val);
    else if (addr == usb->sprsmsk_addr)
        (usb->sprsmsk = val);
    else if (addr == usb->sprsie_addr)
        (usb->sprsie = val);
    else if (addr == usb->uiar_addr)
        (usb->uiar = val);
    else if (addr == usb->glb_state_addr)
        (usb->glb_state = val);
    else if (addr == usb->frm_num_l_addr)
        (usb->frm_num_l = val);
    else if (addr == usb->frm_num_h_addr)
        (usb->frm_num_h = val);
    else
        avr_error ("Bad address: 0x%04x", addr);
}

static void
usb_intr_reset (VDevice *dev)
{
    USBInter_T *usb = (USBInter_T *)dev;

    usb->sprsr = 0;
    usb->uisr = 0;
}

static char *
usb_intr_reg_name (VDevice *dev, int addr)
{
    USBInter_T *usb = (USBInter_T *)dev;

    if (addr == usb->uier_addr)
        return ("UIER");
    else if (addr == usb->uimsk_addr)
        return ("UIMSK");
    else if (addr == usb->uisr_addr)
        return ("UISR");
    else if (addr == usb->sprsie_addr)
        return ("SPRSIE");
    else if (addr == usb->sprsr_addr)
        return ("SPRSR");
    else if (addr == usb->glb_state_addr)
        return ("GLB_STATE");
    else if (addr == usb->frm_num_l_addr)
        return ("FRM_NUM_L");
    else if (addr == usb->frm_num_h_addr)
        return ("FRM_NUM_H");
    else if (addr == usb->sprsmsk_addr)
        return ("SPRSMSK");
    else if (addr == usb->uiar_addr)
        return ("UIAR");
    else
        avr_error ("Bad address: 0x%04x", addr);

    return NULL;                /* will never get here */
}

/*****************************************************************************\
 *
 * USB  
 *
\*****************************************************************************/

static void usb_add_addr (VDevice *vdev, int addr, char *name, int rel_addr,
                          void *data);
static uint8_t usb_read (VDevice *dev, int addr);
static void usb_write (VDevice *dev, int addr, uint8_t val);
static void usb_reset (VDevice *dev);
static char *usb_reg_name (VDevice *dev, int addr);

/** \brief Allocate a new USB structure. */

VDevice *
usb_create (int addr, char *name, int rel_addr, void *data)
{
    return (VDevice *)usb_new (addr, name);
}

USB_T *
usb_new (int addr, char *name)
{
    USB_T *usb;

    usb = avr_new (USB_T, 1);
    usb_construct (usb, addr, name);
    class_overload_destroy ((AvrClass *)usb, usb_destroy);

    return usb;
}

/** \brief Constructor for new USB object. */

void
usb_construct (USB_T *usb, int addr, char *name)
{
    if (usb == NULL)
        avr_error ("passed null ptr");

    vdev_construct ((VDevice *)usb, usb_read, usb_write, usb_reset,
                    usb_add_addr);

    usb_add_addr ((VDevice *)usb, addr, name, 0, NULL);
    usb_reset ((VDevice *)usb);
}

static void
usb_add_addr (VDevice *vdev, int addr, char *name, int rel_addr, void *data)
{
    USB_T *usb = (USB_T *)vdev;

    if (strncmp ("FCAR5", name, 5) == 0)
        usb->fcar5_addr = addr;
    else if (strncmp ("FCAR4", name, 5) == 0)
        usb->fcar4_addr = addr;
    else if (strncmp ("FCAR3", name, 5) == 0)
        usb->fcar3_addr = addr;
    else if (strncmp ("FCAR2", name, 5) == 0)
        usb->fcar2_addr = addr;
    else if (strncmp ("FCAR1", name, 5) == 0)
        usb->fcar1_addr = addr;
    else if (strncmp ("FCAR0", name, 5) == 0)
        usb->fcar0_addr = addr;
    else if (strncmp ("HCAR0", name, 5) == 0)
        usb->hcar0_addr = addr;
    else if (strncmp ("PSTATE1", name, 7) == 0)
        usb->pstate1_addr = addr;
    else if (strncmp ("PSTATE2", name, 7) == 0)
        usb->pstate2_addr = addr;
    else if (strncmp ("PSTATE3", name, 7) == 0)
        usb->pstate3_addr = addr;
    else if (strncmp ("PSTATE4", name, 7) == 0)
        usb->pstate4_addr = addr;
    else if (strncmp ("PSTATE5", name, 7) == 0)
        usb->pstate5_addr = addr;
    else if (strncmp ("PSTATE6", name, 7) == 0)
        usb->pstate6_addr = addr;
    else if (strncmp ("PSTATE7", name, 7) == 0)
        usb->pstate7_addr = addr;
    else if (strncmp ("PSTATE8", name, 7) == 0)
        usb->pstate8_addr = addr;
    else if (strncmp ("HPSCR1", name, 6) == 0)
        usb->hpscr1_addr = addr;
    else if (strncmp ("HPSCR2", name, 6) == 0)
        usb->hpscr2_addr = addr;
    else if (strncmp ("HPSCR3", name, 6) == 0)
        usb->hpscr3_addr = addr;
    else if (strncmp ("HPSCR4", name, 6) == 0)
        usb->hpscr4_addr = addr;
    else if (strncmp ("HPSCR5", name, 6) == 0)
        usb->hpscr5_addr = addr;
    else if (strncmp ("HPSCR6", name, 6) == 0)
        usb->hpscr6_addr = addr;
    else if (strncmp ("HPSCR7", name, 6) == 0)
        usb->hpscr7_addr = addr;
    else if (strncmp ("HPSCR8", name, 6) == 0)
        usb->hpscr8_addr = addr;
    else if (strncmp ("HPSTAT1", name, 7) == 0)
        usb->hpstat1_addr = addr;
    else if (strncmp ("HPSTAT2", name, 7) == 0)
        usb->hpstat2_addr = addr;
    else if (strncmp ("HPSTAT3", name, 7) == 0)
        usb->hpstat3_addr = addr;
    else if (strncmp ("HPSTAT4", name, 7) == 0)
        usb->hpstat4_addr = addr;
    else if (strncmp ("HPSTAT5", name, 7) == 0)
        usb->hpstat5_addr = addr;
    else if (strncmp ("HPSTAT6", name, 7) == 0)
        usb->hpstat6_addr = addr;
    else if (strncmp ("HPSTAT7", name, 7) == 0)
        usb->hpstat7_addr = addr;
    else if (strncmp ("HPSTAT8", name, 7) == 0)
        usb->hpstat8_addr = addr;
    else if (strncmp ("HPCON", name, 5) == 0)
        usb->hpcon_addr = addr;
    else if (strncmp ("HSTR", name, 4) == 0)
        usb->hstr_addr = addr;
    else if (strncmp ("FBYTE_CNT5", name, 10) == 0)
        usb->fbyte_cnt5_addr = addr;
    else if (strncmp ("FBYTE_CNT4", name, 10) == 0)
        usb->fbyte_cnt4_addr = addr;
    else if (strncmp ("FBYTE_CNT3", name, 10) == 0)
        usb->fbyte_cnt3_addr = addr;
    else if (strncmp ("FBYTE_CNT2", name, 10) == 0)
        usb->fbyte_cnt2_addr = addr;
    else if (strncmp ("FBYTE_CNT1", name, 10) == 0)
        usb->fbyte_cnt1_addr = addr;
    else if (strncmp ("FBYTE_CNT0", name, 10) == 0)
        usb->fbyte_cnt0_addr = addr;
    else if (strncmp ("HBYTE_CNT0", name, 10) == 0)
        usb->hbyte_cnt0_addr = addr;
    else if (strncmp ("FDR5", name, 4) == 0)
        usb->fdr5_addr = addr;
    else if (strncmp ("FDR4", name, 4) == 0)
        usb->fdr4_addr = addr;
    else if (strncmp ("FDR3", name, 4) == 0)
        usb->fdr3_addr = addr;
    else if (strncmp ("FDR2", name, 4) == 0)
        usb->fdr2_addr = addr;
    else if (strncmp ("FDR1", name, 4) == 0)
        usb->fdr1_addr = addr;
    else if (strncmp ("FDR0", name, 4) == 0)
        usb->fdr0_addr = addr;
    else if (strncmp ("HDR0", name, 4) == 0)
        usb->hdr0_addr = addr;
    else if (strncmp ("FCSR5", name, 5) == 0)
        usb->fcsr5_addr = addr;
    else if (strncmp ("FCSR4", name, 5) == 0)
        usb->fcsr4_addr = addr;
    else if (strncmp ("FCSR3", name, 5) == 0)
        usb->fcsr3_addr = addr;
    else if (strncmp ("FCSR2", name, 5) == 0)
        usb->fcsr2_addr = addr;
    else if (strncmp ("FCSR1", name, 5) == 0)
        usb->fcsr1_addr = addr;
    else if (strncmp ("FCSR0", name, 5) == 0)
        usb->fcsr0_addr = addr;
    else if (strncmp ("HCSR0", name, 5) == 0)
        usb->hcsr0_addr = addr;
    else if (strncmp ("FENDP5_CNTR", name, 11) == 0)
        usb->fendp5_cntr_addr = addr;
    else if (strncmp ("FENDP4_CNTR", name, 11) == 0)
        usb->fendp4_cntr_addr = addr;
    else if (strncmp ("FENDP3_CNTR", name, 11) == 0)
        usb->fendp3_cntr_addr = addr;
    else if (strncmp ("FENDP2_CNTR", name, 11) == 0)
        usb->fendp2_cntr_addr = addr;
    else if (strncmp ("FENDP1_CNTR", name, 11) == 0)
        usb->fendp1_cntr_addr = addr;
    else if (strncmp ("FENDP0_CNTR", name, 11) == 0)
        usb->fendp0_cntr_addr = addr;
    else if (strncmp ("HENDP1_CNTR", name, 11) == 0)
        usb->hendp1_cntr_addr = addr;
    else if (strncmp ("HENDP0_CNTR", name, 11) == 0)
        usb->hendp0_cntr_addr = addr;
    else if (strncmp ("FADDR", name, 5) == 0)
        usb->faddr_addr = addr;
    else if (strncmp ("HADDR", name, 5) == 0)
        usb->haddr_addr = addr;
    else if (strncmp ("ISCR", name, 4) == 0)
        usb->iscr_addr = addr;
    else if (strncmp ("UOVCER", name, 6) == 0)
        usb->uovcer_addr = addr;
    else
    {
        avr_error ("invalid USB Int register name: '%s' @ 0x%04x", name,
                   addr);
    }
}

/** \brief Destructor for USB object. */

void
usb_destroy (void *usb)
{
    if (usb == NULL)
        return;

    vdev_destroy (usb);
}

static uint8_t
usb_read (VDevice *dev, int addr)
{
    USB_T *usb = (USB_T *)dev;

    if (addr == usb->uovcer_addr)
        return usb->uovcer;
    else if (addr == usb->haddr_addr)
        return usb->haddr;
    else if (addr == usb->faddr_addr)
        return usb->faddr;
    else if (addr == usb->hstr_addr)
        return usb->hstr;
    else if (addr == usb->hpcon_addr)
        return usb->hpcon;
    else if (addr == usb->iscr_addr)
        return usb->iscr;
    else if (addr == usb->fendp5_cntr_addr)
        return usb->fendp5_cntr;
    else if (addr == usb->fendp4_cntr_addr)
        return usb->fendp4_cntr;
    else if (addr == usb->fendp3_cntr_addr)
        return usb->fendp3_cntr;
    else if (addr == usb->fendp2_cntr_addr)
        return usb->fendp2_cntr;
    else if (addr == usb->fendp1_cntr_addr)
        return usb->fendp1_cntr;
    else if (addr == usb->fendp0_cntr_addr)
        return usb->fendp0_cntr;
    else if (addr == usb->hendp1_cntr_addr)
        return usb->hendp1_cntr;
    else if (addr == usb->hendp0_cntr_addr)
        return usb->hendp0_cntr;
    else if (addr == usb->fcsr5_addr)
        return usb->fcsr5 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fcsr4_addr)
        return usb->fcsr4 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fcsr3_addr)
        return usb->fcsr3 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fcsr2_addr)
        return usb->fcsr2 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fcsr1_addr)
        return usb->fcsr1 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fcsr0_addr)
    {
        usb->fcsr0 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
        if (usb->fcsr0 & RX_SETUP)
            usb->fbyte_cnt0 = 10;
        return usb->fcsr0;
    }
    else if (addr == usb->hcsr0_addr)
    {
        usb->hcsr0 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
        if (usb->hcsr0 & RX_SETUP)
            usb->hbyte_cnt0 = 10;
        return usb->hcsr0;
    }
    else if (addr == usb->fcar5_addr)
        return usb->fcar5;
    else if (addr == usb->fcar4_addr)
        return usb->fcar4;
    else if (addr == usb->fcar3_addr)
        return usb->fcar3;
    else if (addr == usb->fcar2_addr)
        return usb->fcar2;
    else if (addr == usb->fcar1_addr)
        return usb->fcar1;
    else if (addr == usb->fcar0_addr)
        return usb->fcar0;
    else if (addr == usb->hcar0_addr)
        return usb->hcar0;
    else if (addr == usb->hpstat1_addr)
        return usb->hpstat1;
    else if (addr == usb->hpstat2_addr)
        return usb->hpstat2;
    else if (addr == usb->hpstat3_addr)
        return usb->hpstat3;
    else if (addr == usb->hpstat4_addr)
        return usb->hpstat4;
    else if (addr == usb->hpstat5_addr)
        return usb->hpstat5;
    else if (addr == usb->hpstat6_addr)
        return usb->hpstat6;
    else if (addr == usb->hpstat7_addr)
        return usb->hpstat7;
    else if (addr == usb->hpstat8_addr)
        return usb->hpstat8;
    else if (addr == usb->pstate1_addr)
        return usb->pstate1;
    else if (addr == usb->pstate2_addr)
        return usb->pstate2;
    else if (addr == usb->pstate3_addr)
        return usb->pstate3;
    else if (addr == usb->pstate4_addr)
        return usb->pstate4;
    else if (addr == usb->pstate5_addr)
        return usb->pstate5;
    else if (addr == usb->pstate6_addr)
        return usb->pstate6;
    else if (addr == usb->pstate7_addr)
        return usb->pstate7;
    else if (addr == usb->pstate8_addr)
        return usb->pstate8;
    else if (addr == usb->hpscr1_addr)
        return usb->hpscr1;
    else if (addr == usb->hpscr2_addr)
        return usb->hpscr2;
    else if (addr == usb->hpscr3_addr)
        return usb->hpscr3;
    else if (addr == usb->hpscr4_addr)
        return usb->hpscr4;
    else if (addr == usb->hpscr5_addr)
        return usb->hpscr5;
    else if (addr == usb->hpscr6_addr)
        return usb->hpscr6;
    else if (addr == usb->hpscr7_addr)
        return usb->hpscr7;
    else if (addr == usb->hpscr8_addr)
        return usb->hpscr8;
    else if (addr == usb->fbyte_cnt5_addr)
        return usb->fbyte_cnt5;
    else if (addr == usb->fbyte_cnt4_addr)
        return usb->fbyte_cnt4;
    else if (addr == usb->fbyte_cnt3_addr)
        return usb->fbyte_cnt3;
    else if (addr == usb->fbyte_cnt2_addr)
        return usb->fbyte_cnt2;
    else if (addr == usb->fbyte_cnt1_addr)
        return usb->fbyte_cnt1;
    else if (addr == usb->fbyte_cnt0_addr)
        return usb->fbyte_cnt0;
    else if (addr == usb->hbyte_cnt0_addr)
        return usb->hbyte_cnt0;
    else if (addr == usb->fdr5_addr)
        return usb->fdr5 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fdr4_addr)
        return usb->fdr4 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fdr3_addr)
        return usb->fdr3 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fdr2_addr)
        return usb->fdr2 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fdr1_addr)
        return usb->fdr1 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->fdr0_addr)
        return usb->fdr0 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else if (addr == usb->hdr0_addr)
        return usb->hdr0 = usb_port_rd (usb_reg_name ((VDevice *)usb, addr));
    else
        avr_error ("Bad address: 0x%04x", addr);
    return 0;                   /* will never get here */
}

static void
usb_write (VDevice *dev, int addr, uint8_t val)
{
    USB_T *usb = (USB_T *)dev;

    if (addr == usb->uovcer_addr)
        (usb->uovcer = val);
    else if (addr == usb->haddr_addr)
        (usb->haddr = val);
    else if (addr == usb->faddr_addr)
        (usb->faddr = val);
    else if (addr == usb->hstr_addr)
        (usb->hstr = val);
    else if (addr == usb->hpcon_addr)
        (usb->hpcon = val);
    else if (addr == usb->fendp5_cntr_addr)
        usb->fendp5_cntr = val;
    else if (addr == usb->fendp4_cntr_addr)
        usb->fendp4_cntr = val;
    else if (addr == usb->fendp3_cntr_addr)
        usb->fendp3_cntr = val;
    else if (addr == usb->fendp2_cntr_addr)
        usb->fendp2_cntr = val;
    else if (addr == usb->fendp1_cntr_addr)
        usb->fendp1_cntr = val;
    else if (addr == usb->fendp0_cntr_addr)
        usb->fendp0_cntr = val;
    else if (addr == usb->hendp1_cntr_addr)
        usb->hendp1_cntr = val;
    else if (addr == usb->hendp0_cntr_addr)
        usb->hendp0_cntr = val;
    else if (addr == usb->fcar5_addr)
    {
        usb->fcar5 = val;
        usb->fcsr5 &= ~val;
        (usb->fbyte_cnt5) = 0;
    }
    else if (addr == usb->fcar4_addr)
    {
        usb->fcar4 = val;
        usb->fcsr4 &= ~val;
        (usb->fbyte_cnt4) = 0;
    }
    else if (addr == usb->fcar3_addr)
    {
        usb->fcar3 = val;
        usb->fcsr3 &= ~val;
        (usb->fbyte_cnt3) = 0;
    }
    else if (addr == usb->fcar2_addr)
    {
        usb->fcar2 = val;
        usb->fcsr2 &= ~val;
        (usb->fbyte_cnt2) = 0;
    }
    else if (addr == usb->fcar1_addr)
    {
        usb->fcar1 = val;
        usb->fcsr1 &= ~val;
        (usb->fbyte_cnt1) = 0;
    }
    else if (addr == usb->fcar0_addr)
    {
        usb->fcar0 = val;
        usb->fcsr0 &= ~val;
        (usb->fbyte_cnt0) = 0;
    }
    else if (addr == usb->hcar0_addr)
    {
        usb->hcar0 = val;
        usb->hcsr0 &= ~val;
        (usb->hbyte_cnt0) = 0;
    }
    else if (addr == usb->fdr5_addr)
    {
        usb->fdr5 = val;
        (usb->fbyte_cnt5)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else if (addr == usb->fdr4_addr)
    {
        usb->fdr4 = val;
        (usb->fbyte_cnt4)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else if (addr == usb->fdr3_addr)
    {
        usb->fdr3 = val;
        (usb->fbyte_cnt3)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else if (addr == usb->fdr2_addr)
    {
        usb->fdr2 = val;
        (usb->fbyte_cnt2)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else if (addr == usb->fdr1_addr)
    {
        usb->fdr1 = val;
        (usb->fbyte_cnt1)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else if (addr == usb->fdr0_addr)
    {
        usb->fdr0 = val;
        (usb->fbyte_cnt0)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else if (addr == usb->hdr0_addr)
    {
        usb->hdr0 = val;
        (usb->hbyte_cnt0)++;
        usb_port_wr (usb_reg_name ((VDevice *)usb, addr), val);
    }
    else
        avr_error ("Bad address: 0x%04x", addr);
}

static void
usb_reset (VDevice *dev)
{
    USB_T *usb = (USB_T *)dev;

    usb->haddr = 0;
    usb->faddr = 0;

    usb->hstr = 0;
    usb->hpcon = 0;

    usb->uovcer = 0;
}

static char *
usb_reg_name (VDevice *dev, int addr)
{
    USB_T *usb = (USB_T *)dev;

    if (addr == usb->fcar5_addr)
        return "FCAR5";
    else if (addr == usb->fcar4_addr)
        return "FCAR4";
    else if (addr == usb->fcar3_addr)
        return "FCAR3";
    else if (addr == usb->fcar2_addr)
        return "FCAR2";
    else if (addr == usb->fcar1_addr)
        return "FCAR1";
    else if (addr == usb->fcar0_addr)
        return "FCAR0";
    else if (addr == usb->hcar0_addr)
        return "HCAR0";
    else if (addr == usb->pstate1_addr)
        return "PSTATE1";
    else if (addr == usb->pstate2_addr)
        return "PSTATE2";
    else if (addr == usb->pstate3_addr)
        return "PSTATE3";
    else if (addr == usb->pstate4_addr)
        return "PSTATE4";
    else if (addr == usb->pstate5_addr)
        return "PSTATE5";
    else if (addr == usb->pstate6_addr)
        return "PSTATE6";
    else if (addr == usb->pstate7_addr)
        return "PSTATE7";
    else if (addr == usb->pstate8_addr)
        return "PSTATE8";
    else if (addr == usb->hpscr1_addr)
        return "HPSCR1";
    else if (addr == usb->hpscr2_addr)
        return "HPSCR2";
    else if (addr == usb->hpscr3_addr)
        return "HPSCR3";
    else if (addr == usb->hpscr4_addr)
        return "HPSCR4";
    else if (addr == usb->hpscr5_addr)
        return "HPSCR5";
    else if (addr == usb->hpscr6_addr)
        return "HPSCR6";
    else if (addr == usb->hpscr7_addr)
        return "HPSCR7";
    else if (addr == usb->hpscr8_addr)
        return "HPSCR8";
    else if (addr == usb->hpstat1_addr)
        return "HPSTAT1";
    else if (addr == usb->hpstat2_addr)
        return "HPSTAT2";
    else if (addr == usb->hpstat3_addr)
        return "HPSTAT3";
    else if (addr == usb->hpstat4_addr)
        return "HPSTAT4";
    else if (addr == usb->hpstat5_addr)
        return "HPSTAT5";
    else if (addr == usb->hpstat6_addr)
        return "HPSTAT6";
    else if (addr == usb->hpstat7_addr)
        return "HPSTAT7";
    else if (addr == usb->hpstat8_addr)
        return "HPSTAT8";
    else if (addr == usb->hpcon_addr)
        return "HPCON";
    else if (addr == usb->hstr_addr)
        return "HSTR";
    else if (addr == usb->fbyte_cnt5_addr)
        return "FBYTE_CNT5";
    else if (addr == usb->fbyte_cnt4_addr)
        return "FBYTE_CNT4";
    else if (addr == usb->fbyte_cnt3_addr)
        return "FBYTE_CNT3";
    else if (addr == usb->fbyte_cnt2_addr)
        return "FBYTE_CNT2";
    else if (addr == usb->fbyte_cnt1_addr)
        return "FBYTE_CNT1";
    else if (addr == usb->fbyte_cnt0_addr)
        return "FBYTE_CNT0";
    else if (addr == usb->hbyte_cnt0_addr)
        return "HBYTE_CNT0";
    else if (addr == usb->fdr5_addr)
        return "FDR5";
    else if (addr == usb->fdr4_addr)
        return "FDR4";
    else if (addr == usb->fdr3_addr)
        return "FDR3";
    else if (addr == usb->fdr2_addr)
        return "FDR2";
    else if (addr == usb->fdr1_addr)
        return "FDR1";
    else if (addr == usb->fdr0_addr)
        return "FDR0";
    else if (addr == usb->hdr0_addr)
        return "HDR0";
    else if (addr == usb->fcsr5_addr)
        return "FCSR5";
    else if (addr == usb->fcsr4_addr)
        return "FCSR4";
    else if (addr == usb->fcsr3_addr)
        return "FCSR3";
    else if (addr == usb->fcsr2_addr)
        return "FCSR2";
    else if (addr == usb->fcsr1_addr)
        return "FCSR1";
    else if (addr == usb->fcsr0_addr)
        return "FCSR0";
    else if (addr == usb->hcsr0_addr)
        return "HCSR0";
    else if (addr == usb->fendp5_cntr_addr)
        return "FENDP5_CNTR";
    else if (addr == usb->fendp4_cntr_addr)
        return "FENDP4_CNTR";
    else if (addr == usb->fendp3_cntr_addr)
        return "FENDP3_CNTR";
    else if (addr == usb->fendp2_cntr_addr)
        return "FENDP2_CNTR";
    else if (addr == usb->fendp1_cntr_addr)
        return "FENDP1_CNTR";
    else if (addr == usb->fendp0_cntr_addr)
        return "FENDP0_CNTR";
    else if (addr == usb->hendp1_cntr_addr)
        return "HENDP1_CNTR";
    else if (addr == usb->hendp0_cntr_addr)
        return "HENDP0_CNTR";
    else if (addr == usb->faddr_addr)
        return "FADDR";
    else if (addr == usb->haddr_addr)
        return "HADDR";
    else if (addr == usb->iscr_addr)
        return "ISCR";
    else if (addr == usb->uovcer_addr)
        return "UOVCER";
    else
        avr_error ("Bad address: 0x%04x", addr);
    return NULL;
}

uint8_t
usb_port_rd (char *name)
{
    int data;
    char line[80];

    while (1)
    {
        fprintf (stderr, "\nEnter a byte of hex data to read into %s: ",
                 name);

        /* try to read in a line of input */
        if (fgets (line, sizeof (line), stdin) == NULL)
            continue;

        /* try to parse the line for a byte of data */
        if (sscanf (line, "%x\n", &data) != 1)
            continue;

        break;
    }
    return (uint8_t) (data & 0xff);
}

void
usb_port_wr (char *name, uint8_t val)
{
    fprintf (stderr, "wrote 0x%02x to %s\n", val, name);
}
