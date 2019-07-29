/*
 * $Id: usb.h,v 1.3 2004/03/13 19:55:34 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2003  Keith Gudger
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

#ifndef USB_H
#define USB_H

/****************************************************************************\
 *
 * USBInter_T(VDevice) : USB Interrupt Mask/Flag/Enable Registers
 *
\****************************************************************************/

enum _usb_intr_constants
{
    USB_INTR_BASE = 0x1ff3,     /* base address for vdev */
    USB_INTR_SIZE = 11,         /* UIER, unused, UIAR, UIMSK */

    USB_INTR_ENABLE_ADDR = 0,
    USB_INTR_UNUSED_ADDR = 1,
    USB_INTR_ACK_ADDR = 2,
    USB_INTR_MASK_ADDR = 3,
    USB_INTR_STATUS_ADDR = 4,
    USB_INTR_SPRSMSK_ADDR = 5,
    USB_INTR_SPRSIE_ADDR = 6,
    USB_INTR_SPRSR_ADDR = 7,
    USB_GLB_STATE_ADDR = 8,
    USB_FRM_NUM_L_ADDR = 9,
    USB_FRM_NUM_H_ADDR = 10,
};

typedef enum
{
    bit_FEP0 = 0,               /* Function Endpoint 0 interrupt enable */
    bit_FEP1 = 1,               /* Function Endpoint 1 interrupt enable */
    bit_FEP2 = 2,               /* Function Endpoint 2 interrupt enable */
    bit_HEP0 = 3,               /* Hub      Endpoint 0 interrupt enable */
    bit_FEP3 = 4,               /* Function Endpoint 3 interrupt enable */
} USBMSK_BITS;

typedef enum
{
    mask_FEP0 = 1 << bit_FEP0,
    mask_FEP1 = 1 << bit_FEP1,
    mask_FEP2 = 1 << bit_FEP2,
    mask_HEP0 = 1 << bit_HEP0,
    mask_FEP3 = 1 << bit_FEP3,
} USB_MASKS;

typedef enum
{
    bit_TX_COMPLETE = 0,
    bit_RX_OUT_PACKET = 1,
    bit_RX_SETUP = 2,
    bit_STALL_SENT = 3,
    bit_TX_PACKET_READY = 4,
    bit_FORCE_STALL = 5,
    bit_DATA_END = 6,
    bit_DATA_DIR = 7,
} USBCSR_BITS;

typedef enum
{
    bit_TX_COMPLETE_ACK = 0,
    bit_RX_OUT_PACKET_ACK = 1,
    bit_RX_SETUP_ACK = 2,
    bit_STALL_SENT_ACK = 3,
} USBCAR_BITS;

typedef enum
{
    TX_COMPLETE = 1 << bit_TX_COMPLETE,
    RX_OUT_PACKET = 1 << bit_RX_OUT_PACKET,
    RX_SETUP = 1 << bit_RX_SETUP,
    STALL_SENT = 1 << bit_STALL_SENT,
    TX_PACKET_READY = 1 << bit_TX_PACKET_READY,
    FORCE_STALL = 1 << bit_FORCE_STALL,
    DATA_END = 1 << bit_DATA_END,
    DATA_DIR = 1 << bit_DATA_DIR,
} USBCSR_MASKS;

typedef struct _USBInter_T USBInter_T;

struct _USBInter_T
{
    VDevice parent;

    int uier_addr;              /* USB interrupt enable (mask) register */
    int uiar_addr;              /* USB interrupt ack  register */
    int uimsk_addr;             /* USB interrupt mask register */
    int uisr_addr;              /* USB interrupt flag register */
    int sprsmsk_addr;
    int sprsie_addr;
    int sprsr_addr;
    int glb_state_addr;
    int frm_num_l_addr;
    int frm_num_h_addr;

    uint8_t uier;               /* USB interrupt enable (mask) register */
    uint8_t uiar;               /* USB interrupt ack  register */
    uint8_t uimsk;              /* USB interrupt mask register */
    uint8_t uisr;               /* USB interrupt flag register */
    uint8_t func_mask;          /* mask of available register functions */
    uint8_t sprsmsk;
    uint8_t sprsie;
    uint8_t sprsr;
    uint8_t glb_state;
    uint8_t frm_num_l;
    uint8_t frm_num_h;
    CallBack *intr_cb;          /* callback for checking and raising
                                   interrupts */
};

extern VDevice *usbi_create (int addr, char *name, int rel_addr, void *data);
extern USBInter_T *usb_intr_new (int addr, char *name, uint8_t data);
extern void usb_intr_construct (USBInter_T *usb, int addr, char *name,
                                uint8_t func_mask);
extern void usb_intr_destroy (void *usb);

/****************************************************************************\
 *
 * USB(VDevice) : USB Structure
 *
\****************************************************************************/

enum _usb_constants
{
    USB_BASE = 0x1fa0,          /* base memory address */
    USB_SIZE = 83,              /* TCCR0 and TCNT0 */

    USB_FCAR5_ADDR = 0,         /* offset from base to FCAR Register */
    USB_FCAR4_ADDR = 1,
    USB_FCAR3_ADDR = 2,
    USB_FCAR2_ADDR = 3,
    USB_FCAR1_ADDR = 4,
    USB_FCAR0_ADDR = 5,
    USB_HCAR0_ADDR = 7,
    USB_PSTATE1_ADDR = 8,
    USB_PSTATE2_ADDR = 9,
    USB_PSTATE3_ADDR = 0x0a,
    USB_PSTATE4_ADDR = 0x0b,
    USB_PSTATE5_ADDR = 0x0c,
    USB_PSTATE6_ADDR = 0x0d,
    USB_PSTATE7_ADDR = 0x0e,
    USB_PSTATE8_ADDR = 0x0f,
    USB_HPSCR1_ADDR = 0x10,     /* 0x1fb0 */
    USB_HPSCR2_ADDR = 0x11,
    USB_HPSCR3_ADDR = 0x12,
    USB_HPSCR4_ADDR = 0x13,
    USB_HPSCR5_ADDR = 0x14,
    USB_HPSCR6_ADDR = 0x15,
    USB_HPSCR7_ADDR = 0x16,
    USB_HPSCR8_ADDR = 0x17,
    USB_HPSTAT1_ADDR = 0x18,
    USB_HPSTAT2_ADDR = 0x19,
    USB_HPSTAT3_ADDR = 0x1a,
    USB_HPSTAT4_ADDR = 0x1b,
    USB_HPSTAT5_ADDR = 0x1c,
    USB_HPSTAT6_ADDR = 0x1d,
    USB_HPSTAT7_ADDR = 0x1e,
    USB_HPSTAT8_ADDR = 0x1f,
    USB_HPCON_ADDR = 0x25,
    USB_HSTR_ADDR = 0x27,       /* 0X1FC7 */
    USB_FBYTE_CNT5_ADDR = 0x28,
    USB_FBYTE_CNT4_ADDR = 0x29,
    USB_FBYTE_CNT3_ADDR = 0x2a,
    USB_FBYTE_CNT2_ADDR = 0x2b,
    USB_FBYTE_CNT1_ADDR = 0x2c,
    USB_FBYTE_CNT0_ADDR = 0x2d,
    USB_HBYTE_CNT0_ADDR = 0x2f,
    USB_FDR5_ADDR = 0x30,       /* 0x1fd0 */
    USB_FDR4_ADDR = 0x31,
    USB_FDR3_ADDR = 0x32,
    USB_FDR2_ADDR = 0x33,
    USB_FDR1_ADDR = 0x34,
    USB_FDR0_ADDR = 0x35,
    USB_HDR0_ADDR = 0x37,
    USB_FCSR5_ADDR = 0x38,
    USB_FCSR4_ADDR = 0x39,
    USB_FCSR3_ADDR = 0x3a,
    USB_FCSR2_ADDR = 0x3b,
    USB_FCSR1_ADDR = 0x3c,
    USB_FCSR0_ADDR = 0x3d,
    USB_HCSR0_ADDR = 0x3f,
    USB_FENDP5_CNTR_ADDR = 0x40, /* 0x1fe0 */
    USB_FENDP4_CNTR_ADDR = 0x41,
    USB_FENDP3_CNTR_ADDR = 0x42,
    USB_FENDP2_CNTR_ADDR = 0x43,
    USB_FENDP1_CNTR_ADDR = 0x44,
    USB_FENDP0_CNTR_ADDR = 0x45,
    USB_HENDP1_CNTR_ADDR = 0x46,
    USB_HENDP0_CNTR_ADDR = 0x47,
    USB_FADDR_ADDR = 0x4e,      /* 0x1fee */
    USB_HADDR_ADDR = 0x4f,
    USB_ISCR_ADDR = 0x51,       /* 0x1ff1 */
    USB_UOVCER_ADDR = 0x52,     /* 0x1ff2 */
};

typedef struct _USB USB_T;

struct _USB
{
    VDevice parent;

    uint16_t uovcer_addr;       /* overcurrent register */
    uint16_t haddr_addr;        /* Hub Address register */
    uint16_t faddr_addr;        /* Function Address register */
    uint16_t hstr_addr;         /* Hub Status register */
    uint16_t hpcon_addr;        /* Hub Port Control register */
    uint16_t iscr_addr;         /* interrupt control sense register */
    uint16_t fendp5_cntr_addr;  /* endpoint 5 control register */
    uint16_t fendp4_cntr_addr;  /* endpoint 4 control register */
    uint16_t fendp3_cntr_addr;  /* endpoint 3 control register */
    uint16_t fendp2_cntr_addr;  /* endpoint 2 control register */
    uint16_t fendp1_cntr_addr;  /* endpoint 1 control register */
    uint16_t fendp0_cntr_addr;  /* endpoint 0 control register */
    uint16_t hendp1_cntr_addr;  /* hub endpoint control register */
    uint16_t hendp0_cntr_addr;  /* hub endpoint control register */
    uint16_t fcsr5_addr;        /* endpoint 5 control & status register */
    uint16_t fcsr4_addr;        /* endpoint 4 control & status register */
    uint16_t fcsr3_addr;        /* endpoint 3 control & status register */
    uint16_t fcsr2_addr;        /* endpoint 2 control & status register */
    uint16_t fcsr1_addr;        /* endpoint 1 control & status register */
    uint16_t fcsr0_addr;        /* endpoint 0 control & status register */
    uint16_t hcsr0_addr;        /* hub endpoint control & status register */
    uint16_t fcar5_addr;        /* endpoint 5 control & ack register */
    uint16_t fcar4_addr;        /* endpoint 4 control & ack register */
    uint16_t fcar3_addr;        /* endpoint 3 control & ack register */
    uint16_t fcar2_addr;        /* endpoint 2 control & ack register */
    uint16_t fcar1_addr;        /* endpoint 1 control & ack register */
    uint16_t fcar0_addr;        /* endpoint 0 control & ack register */
    uint16_t hcar0_addr;        /* hub endpoint control & ack register */
    uint16_t hpstat1_addr;      /* hub port 1 status register */
    uint16_t hpstat2_addr;      /* hub port 2 status register */
    uint16_t hpstat3_addr;      /* hub port 3 status register */
    uint16_t hpstat4_addr;      /* hub port 4 status register */
    uint16_t hpstat5_addr;      /* hub port 5 status register */
    uint16_t hpstat6_addr;      /* hub port 6 status register */
    uint16_t hpstat7_addr;      /* hub port 7 status register */
    uint16_t hpstat8_addr;      /* hub port 8 status register */
    uint16_t pstate1_addr;      /* hub port 1 port state register */
    uint16_t pstate2_addr;      /* hub port 2 port state register */
    uint16_t pstate3_addr;      /* hub port 3 port state register */
    uint16_t pstate4_addr;      /* hub port 4 port state register */
    uint16_t pstate5_addr;      /* hub port 5 port state register */
    uint16_t pstate6_addr;      /* hub port 6 port state register */
    uint16_t pstate7_addr;      /* hub port 7 port state register */
    uint16_t pstate8_addr;      /* hub port 8 port state register */
    uint16_t hpscr1_addr;       /* hub port 1 status change register */
    uint16_t hpscr2_addr;       /* hub port 2 status change register */
    uint16_t hpscr3_addr;       /* hub port 3 status change register */
    uint16_t hpscr4_addr;       /* hub port 4 status change register */
    uint16_t hpscr5_addr;       /* hub port 5 status change register */
    uint16_t hpscr6_addr;       /* hub port 6 status change register */
    uint16_t hpscr7_addr;       /* hub port 7 status change register */
    uint16_t hpscr8_addr;       /* hub port 8 status change register */
    uint16_t fdr5_addr;         /* endpoint 5 data register */
    uint16_t fdr4_addr;         /* endpoint 4 data register */
    uint16_t fdr3_addr;         /* endpoint 3 data register */
    uint16_t fdr2_addr;         /* endpoint 2 data register */
    uint16_t fdr1_addr;         /* endpoint 1 data register */
    uint16_t fdr0_addr;         /* endpoint 0 data register */
    uint16_t hdr0_addr;         /* hub endpoint data register */
    uint16_t fbyte_cnt5_addr;   /* byte count register */
    uint16_t fbyte_cnt4_addr;   /* byte count register */
    uint16_t fbyte_cnt3_addr;   /* byte count register */
    uint16_t fbyte_cnt2_addr;   /* byte count register */
    uint16_t fbyte_cnt1_addr;   /* byte count register */
    uint16_t fbyte_cnt0_addr;   /* byte count register */
    uint16_t hbyte_cnt0_addr;   /* byte count register */

    uint8_t uovcer;             /* overcurrent register */
    uint8_t haddr;              /* Hub Address register */
    uint8_t faddr;              /* Function Address register */
    uint8_t hstr;               /* Hub Status register */
    uint8_t hpcon;              /* Hub Port Control register */
    uint8_t iscr;               /* interrupt control sense register */
    uint8_t fendp5_cntr;        /* endpoint 5 control register */
    uint8_t fendp4_cntr;        /* endpoint 4 control register */
    uint8_t fendp3_cntr;        /* endpoint 3 control register */
    uint8_t fendp2_cntr;        /* endpoint 2 control register */
    uint8_t fendp1_cntr;        /* endpoint 1 control register */
    uint8_t fendp0_cntr;        /* endpoint 0 control register */
    uint8_t hendp1_cntr;        /* hub endpoint control register */
    uint8_t hendp0_cntr;        /* hub endpoint control register */
    uint8_t fcsr5;              /* endpoint 5 control & status register */
    uint8_t fcsr4;              /* endpoint 4 control & status register */
    uint8_t fcsr3;              /* endpoint 3 control & status register */
    uint8_t fcsr2;              /* endpoint 2 control & status register */
    uint8_t fcsr1;              /* endpoint 1 control & status register */
    uint8_t fcsr0;              /* endpoint 0 control & status register */
    uint8_t hcsr0;              /* hub endpoint control & status register */
    uint8_t fcar5;              /* endpoint 5 control & ack register */
    uint8_t fcar4;              /* endpoint 4 control & ack register */
    uint8_t fcar3;              /* endpoint 3 control & ack register */
    uint8_t fcar2;              /* endpoint 2 control & ack register */
    uint8_t fcar1;              /* endpoint 1 control & ack register */
    uint8_t fcar0;              /* endpoint 0 control & ack register */
    uint8_t hcar0;              /* hub endpoint control & ack register */
    uint8_t hpstat1;            /* hub port 1 status register */
    uint8_t hpstat2;            /* hub port 2 status register */
    uint8_t hpstat3;            /* hub port 3 status register */
    uint8_t hpstat4;            /* hub port 4 status register */
    uint8_t hpstat5;            /* hub port 5 status register */
    uint8_t hpstat6;            /* hub port 6 status register */
    uint8_t hpstat7;            /* hub port 7 status register */
    uint8_t hpstat8;            /* hub port 8 status register */
    uint8_t pstate1;            /* hub port 1 port state register */
    uint8_t pstate2;            /* hub port 2 port state register */
    uint8_t pstate3;            /* hub port 3 port state register */
    uint8_t pstate4;            /* hub port 4 port state register */
    uint8_t pstate5;            /* hub port 5 port state register */
    uint8_t pstate6;            /* hub port 6 port state register */
    uint8_t pstate7;            /* hub port 7 port state register */
    uint8_t pstate8;            /* hub port 8 port state register */
    uint8_t hpscr1;             /* hub port 1 status change register */
    uint8_t hpscr2;             /* hub port 2 status change register */
    uint8_t hpscr3;             /* hub port 3 status change register */
    uint8_t hpscr4;             /* hub port 4 status change register */
    uint8_t hpscr5;             /* hub port 5 status change register */
    uint8_t hpscr6;             /* hub port 6 status change register */
    uint8_t hpscr7;             /* hub port 7 status change register */
    uint8_t hpscr8;             /* hub port 8 status change register */
    uint8_t fdr5;               /* endpoint 5 data register */
    uint8_t fdr4;               /* endpoint 4 data register */
    uint8_t fdr3;               /* endpoint 3 data register */
    uint8_t fdr2;               /* endpoint 2 data register */
    uint8_t fdr1;               /* endpoint 1 data register */
    uint8_t fdr0;               /* endpoint 0 data register */
    uint8_t hdr0;               /* hub endpoint data register */
    uint8_t fbyte_cnt5;         /* byte count register */
    uint8_t fbyte_cnt4;         /* byte count register */
    uint8_t fbyte_cnt3;         /* byte count register */
    uint8_t fbyte_cnt2;         /* byte count register */
    uint8_t fbyte_cnt1;         /* byte count register */
    uint8_t fbyte_cnt0;         /* byte count register */
    uint8_t hbyte_cnt0;         /* byte count register */
};

extern VDevice *usb_create (int addr, char *name, int rel_addr, void *data);
extern USB_T *usb_new (int addr, char *name);
extern void usb_construct (USB_T *usb, int addr, char *name);
extern void usb_destroy (void *usb);
extern void usb_intr_set_flag (USB_T *usb, uint8_t bitnr);
extern void usb_intr_clear_flag (USB_T *usb, uint8_t bitnr);

#endif /* USB_H */
