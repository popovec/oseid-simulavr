/*
 * $Id: 43usb320.h,v 1.3 2004/03/13 19:55:34 troth Exp $
 *
 ****************************************************************************
 *
 * simulavr - A simulator for the Atmel AVR family of microcontrollers.
 * Copyright (C) 2004  Theodore A. Roth
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

#if defined (IN_DEVSUPP_C)
/* *INDENT-OFF* */

static uint8_t uart_data_320 = UART;

static uint8_t uier_320 = (mask_FEP0 | mask_FEP1 | mask_FEP2 | mask_FEP3 
                           | mask_HEP0); 

static uint8_t timer_mask_320 = (mask_TOIE1 | mask_OCIE1A | mask_OCIE1B
                                 | mask_TICIE1 | mask_TOIE0);

static uint8_t timer1_def_320 = TD_TIMER1;
static uint8_t ocr1a_def_320 = OCR1A_DEF;
static uint8_t ocr1b_def_320 = OCR1B_DEF;

static DevSuppDefn defn_at43usb320 = {
    .name           = "at43usb320",
    .stack_type     = STACK_MEMORY,
    .irq_vect_idx   = VTAB_AT43USB320,

    .size = {
        .pc         = 2,
        .stack      = 0,
        .flash      = 32 * 1024,
        .sram       = 512,
        .eeprom     = 0
    },

    .io_reg = {
        { 
            .addr = 0x29, 
            .name = "UBRR", 
            .vdev_create = uart_int_create,
            .data = &uart_data_320,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x2a, 
            .name = "UCR",
            .ref_addr = 0x29,
            .reset_value = 0x00,
            .rd_mask = 0xfe,
            .wr_mask = 0xfd,
        },
        {   .addr = 0x2b, 
            .name = "USR",
            .ref_addr = 0x29,
            .reset_value = 0x00,
            .rd_mask = 0xf7,
            .wr_mask = 0x40,
        },
        {   .addr = 0x2c, 
            .name = "UDR", 
            .vdev_create = uart_create,
            .related = 0x29,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x2d, 
            .name = "SPCR", 
            .vdev_create = spii_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x2e, 
            .name = "SPSR",
            .ref_addr = 0x2d,
            .reset_value = 0x00,
            .rd_mask = 0xc0,
            .wr_mask = 0xc0,
        },
        {   .addr = 0x2f, 
            .name = "SPDR", 
            .vdev_create = spi_create,
            .related = 0x2d,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x30,
            .name = "PIND",
            .vdev_create = port_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x31,
            .name = "DDRD",
            .ref_addr = 0x30,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x32,
            .name = "PORTD",
            .ref_addr = 0x30,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x33,
            .name = "PINC",
            .vdev_create = port_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x34,
            .name = "DDRC",
            .ref_addr = 0x33,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x35,
            .name = "PORTC",
            .ref_addr = 0x33,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x36,
            .name = "PINB",
            .vdev_create = port_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x37,
            .name = "DDRB",
            .ref_addr = 0x36,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x38,
            .name = "PORTB",
            .ref_addr = 0x36,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x39,
            .name = "PINA",
            .vdev_create = port_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x3a,
            .name = "DDRA",
            .ref_addr = 0x39,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x3b,
            .name = "PORTA",
            .ref_addr = 0x39,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { .addr = 0x41, .name = "WDTCR", },
        { .addr = 0x44, .name = "ICR1L", },
        { .addr = 0x45, .name = "ICR1H", },
        {   .addr = 0x48,
            .name = "OCRBL",
            .vdev_create = ocreg16_create,
            .related = 0x4c,
            .data = &ocr1b_def_320,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x49,
            .name = "OCRBH",
            .ref_addr = 0x48,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x4a,
            .name = "OCRAL",
            .vdev_create = ocreg16_create,
            .related = 0x4c,
            .data = &ocr1a_def_320,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x4b,
            .name = "OCRAH",
            .ref_addr = 0x4a,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x4c,
            .name = "TCNTL",
            .vdev_create = timer16_create,
            .related = 0x58,
            .data = &timer1_def_320,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x4d,
            .name = "TCNTH",
            .ref_addr = 0x4c,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x4e,
            .name = "TCCRB",
            .ref_addr = 0x4c,
            .reset_value = 0,
            .rd_mask = 0xcf,
            .wr_mask = 0xcf,
        },
        {   .addr = 0x4f,
            .name = "TCCRA",
            .ref_addr = 0x4c,
            .reset_value = 0,
            .rd_mask = 0xf3,
            .wr_mask = 0xf3,
        },
        {   .addr = 0x52,
            .name = "TCNT",
            .vdev_create = timer0_create,
            .related = 0x58,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x53,
            .name = "TCCR",
            .ref_addr = 0x52,
            .reset_value = 0,
            .rd_mask = 0x07,
            .wr_mask = 0x07,
        },
        { .addr = 0x55, .name = "MCUCR", },
        {   .addr = 0x58,
            .name = "TIFR",
            .vdev_create = timer_int_create,
            .data = &(timer_mask_320),
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {   .addr = 0x59,
            .name = "TIMSK",
            .ref_addr = 0x58,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { .addr = 0x5a, .name = "GIFR", },
        { .addr = 0x5b, .name = "GIMSK", },
        {
            .addr = 0x5d,
            .name = "SPL", 
            .vdev_create = sp_create,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x5e,
            .name = "SPH",
            .ref_addr = 0x5d,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x5f,
            .name = "SREG",
            .vdev_create = sreg_create,
            .reset_value = 0,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fa3, 
            .name = "FCAR2", 
            .vdev_create = usb_create,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fa4, 
            .name = "FCAR1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fa5, 
            .name = "FCAR0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fa7, 
            .name = "HCAR0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fa8, 
            .name = "PSTATE1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fa9, 
            .name = "PSTATE2", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1faa, 
            .name = "PSTATE3", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fab, 
            .name = "PSTATE4", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fac, 
            .name = "PSTATE5", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb0, 
            .name = "HPSCR1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb1, 
            .name = "HPSCR2", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb2, 
            .name = "HPSCR3", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb2, 
            .name = "HPSCR4", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb2, 
            .name = "HPSCR5", 
            .ref_addr = 0x1fa4,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb8, 
            .name = "HPSTAT1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fb9, 
            .name = "HPSTAT2", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fba, 
            .name = "HPSTAT3", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fbb, 
            .name = "HPSTAT4", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fbc, 
            .name = "HPSTAT5", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fc5, 
            .name = "HPCON", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fc7, 
            .name = "HSTR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fcb, 
            .name = "FBYTE_CNT2", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fcc, 
            .name = "FBYTE_CNT1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fcd, 
            .name = "FBYTE_CNT0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fcf, 
            .name = "HBYTE_CNT0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fd3, 
            .name = "FDR2", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fd4, 
            .name = "FDR1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fd5, 
            .name = "FDR0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fd7, 
            .name = "HDR0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fdb, 
            .name = "FCSR2", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fdc, 
            .name = "FCSR1", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fdd, 
            .name = "FCSR0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fdf, 
            .name = "HCSR0", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fe3, 
            .name = "FENDP2_CNTR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fe4, 
            .name = "FENDP1_CNTR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fe5, 
            .name = "FENDP0_CNTR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fe7, 
            .name = "HENDP0_CNTR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fee, 
            .name = "FADDR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1fef, 
            .name = "HADDR", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ff2, 
            .name = "UOVCER", 
            .ref_addr = 0x1fa3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        {
            .addr = 0x1ff3, 
            .name = "UIER",
            .vdev_create = usbi_create,
            .data = &(uier_320),
            .reset_value = 0x00,
            .rd_mask = 0xc0,
            .wr_mask = 0xc0,
        },
        { 
            .addr = 0x1ff5, 
            .name = "UIAR", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ff6, 
            .name = "UIMSK", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ff7, 
            .name = "UISR", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ff8, 
            .name = "SPRSMSK", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ff9, 
            .name = "SPRSIE", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ffa, 
            .name = "SPRSR", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ffb, 
            .name = "GLB_STATE", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ffc, 
            .name = "FRM_NUM_L", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        { 
            .addr = 0x1ffd, 
            .name = "FRM_NUM_H", 
            .ref_addr = 0x1ff3,
            .reset_value = 0x00,
            .rd_mask = 0xff,
            .wr_mask = 0xff,
        },
        IO_REG_DEFN_TERMINATOR
    }
};

/* *INDENT-ON* */
#endif /* IN_DEVSUPP_C */
