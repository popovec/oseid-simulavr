; -*-Mode:asm; -*-
;
; 8515def.inc
;
; This file was based on the Atmel include file and is for use
; with avr-as.
;
;***** I/O Register Definitions
.equ    SREG,    0x3f
.equ    SPH,     0x3e
.equ    SPL,     0x3d
.equ    GIMSK,   0x3b
.equ    GIFR,    0x3a
.equ    TIMSK,   0x39
.equ    TIFR,    0x38
.equ    MCUCR,   0x35

.equ    TCCR0,   0x33
.equ    TCNT0,   0x32
.equ    OCR0,    0x31

.equ    TCCR1A,  0x2f
.equ    TCCR1B,  0x2e
.equ    TCNT1H,  0x2d
.equ    TCNT1L,  0x2c
.equ    OCR1AH,  0x2b
.equ    OCR1AL,  0x2a
.equ    OCR1BH,  0x29
.equ    OCR1BL,  0x28
.equ    ICR1H,   0x25
.equ    ICR1L,   0x24

.equ    WDTCR,   0x21
.equ    EEARH,   0x1f
.equ    EEARL,   0x1e
.equ    EEDR,    0x1d
.equ    EECR,    0x1c

.equ    PORTA,   0x1b
.equ    DDRA,    0x1a
.equ    PINA,    0x19
.equ    PORTB,   0x18
.equ    DDRB,    0x17
.equ    PINB,    0x16
.equ    PORTC,   0x15
.equ    DDRC,    0x14
.equ    PINC,    0x13
.equ    PORTD,   0x12
.equ    DDRD,    0x11
.equ    PIND,    0x10

.equ    SPDR,    0x0f
.equ    SPSR,    0x0e
.equ    SPCR,    0x0d
.equ    UDR,     0x0c
.equ    USR,     0x0b
.equ    UCR,     0x0a
.equ    UBRR,    0x09
.equ    ACSR,    0x08


;***** Bit Definitions
.equ    INT1,    7
.equ    INT0,    6

.equ    INTF1,   7
.equ    INTF0,   6

.equ    TOIE1,   7
.equ    OCIE1A,  6
.equ    OCIE1B,  5
.equ    TICIE1,  3
.equ    TOIE0,   1

.equ    TOV1,    7
.equ    OCF1A,   6
.equ    OCF1B,   5
.equ    ICF1,    3
.equ    TOV0,    1

.equ    SRE,     7
.equ    SRW,     6
.equ    SE,      5
.equ    SM,      4
.equ    ISC11,   3
.equ    ISC10,   2
.equ    ISC01,   1
.equ    ISC00,   0

.equ    CS02,    2
.equ    CS01,    1
.equ    CS00,    0

.equ    COM1A1,  7
.equ    COM1A0,  6
.equ    COM1B1,  5
.equ    COM1B0,  4
.equ    PWM11,   1
.equ    PWM10,   0

.equ    ICNC1,   7
.equ    ICES1,   6
.equ    CTC1,    3
.equ    CS12,    2
.equ    CS11,    1
.equ    CS10,    0

.equ    WDDE,    4
.equ    WDE,     3
.equ    WDP2,    2
.equ    WDP1,    1
.equ    WDP0,    0

.equ    EEMWE,   2
.equ    EEWE,    1
.equ    EERE,    0

.equ    PA7,     7
.equ    PA6,     6
.equ    PA5,     5
.equ    PA4,     4
.equ    PA3,     3
.equ    PA2,     2
.equ    PA1,     1
.equ    PA0,     0

.equ    DDA7,    7
.equ    DDA6,    6
.equ    DDA5,    5
.equ    DDA4,    4
.equ    DDA3,    3
.equ    DDA2,    2
.equ    DDA1,    1
.equ    DDA0,    0

.equ    PINA7,   7
.equ    PINA6,   6
.equ    PINA5,   5
.equ    PINA4,   4
.equ    PINA3,   3
.equ    PINA2,   2
.equ    PINA1,   1
.equ    PINA0,   0

.equ    PB7,     7
.equ    PB6,     6
.equ    PB5,     5
.equ    PB4,     4
.equ    PB3,     3
.equ    PB2,     2
.equ    PB1,     1
.equ    PB0,     0

.equ    DDB7,    7
.equ    DDB6,    6
.equ    DDB5,    5
.equ    DDB4,    4
.equ    DDB3,    3
.equ    DDB2,    2
.equ    DDB1,    1
.equ    DDB0,    0

.equ    PINB7,   7
.equ    PINB6,   6
.equ    PINB5,   5
.equ    PINB4,   4
.equ    PINB3,   3
.equ    PINB2,   2
.equ    PINB1,   1
.equ    PINB0,   0

.equ    PC7,     7
.equ    PC6,     6
.equ    PC5,     5
.equ    PC4,     4
.equ    PC3,     3
.equ    PC2,     2
.equ    PC1,     1
.equ    PC0,     0

.equ    DDC7,    7
.equ    DDC6,    6
.equ    DDC5,    5
.equ    DDC4,    4
.equ    DDC3,    3
.equ    DDC2,    2
.equ    DDC1,    1
.equ    DDC0,    0

.equ    PINC7,   7
.equ    PINC6,   6
.equ    PINC5,   5
.equ    PINC4,   4
.equ    PINC3,   3
.equ    PINC2,   2
.equ    PINC1,   1
.equ    PINC0,   0

.equ    PD7,     7
.equ    PD6,     6
.equ    PD5,     5
.equ    PD4,     4
.equ    PD3,     3
.equ    PD2,     2
.equ    PD1,     1
.equ    PD0,     0

.equ    DDD7,    7
.equ    DDD6,    6
.equ    DDD5,    5
.equ    DDD4,    4
.equ    DDD3,    3
.equ    DDD2,    2
.equ    DDD1,    1
.equ    DDD0,    0

.equ    PIND7,   7
.equ    PIND6,   6
.equ    PIND5,   5
.equ    PIND4,   4
.equ    PIND3,   3
.equ    PIND2,   2
.equ    PIND1,   1
.equ    PIND0,   0

.equ    SPIE,    7
.equ    SPE,     6
.equ    DORD,    5
.equ    MSTR,    4
.equ    CPOL,    3
.equ    CPHA,    2
.equ    SPR1,    1
.equ    SPR0,    0

.equ    SPIF,    7
.equ    WCOL,    6

.equ    RXC,     7
.equ    TXC,     6
.equ    UDRE,    5
.equ    FE,      4
.equ    OR,      3

.equ    RXCIE,   7
.equ    TXCIE,   6
.equ    UDRIE,   5
.equ    RXEN,    4
.equ    TXEN,    3
.equ    CHR9,    2
.equ    RXB8,    1
.equ    TXB8,    0

.equ    ACD,     7
.equ    ACO,     5
.equ    ACI,     4
.equ    ACIE,    3
.equ    ACIC,    2
.equ    ACIS1,   1
.equ    ACIS0,   0

.equ    XL,      26
.equ    XH,      27
.equ    YL,      28
.equ    YH,      29
.equ    ZL,      30
.equ    ZH,      31

.equ    RAMEND,  0x25F   ;Last On-Chip SRAM Location
.equ    XRAMEND, 0xFFFF
.equ    E2END,   0x1FF
.equ    FLASHEND,0xFFF


.equ    INT0addr,0x001   ;External Interrupt0 Vector Address
.equ    INT1addr,0x002   ;External Interrupt1 Vector Address
.equ    ICP1addr,0x003   ;Input Capture1 Interrupt Vector Address
.equ    OC1Aaddr,0x004   ;Output Compare1A Interrupt Vector Address
.equ    OC1Baddr,0x005   ;Output Compare1B Interrupt Vector Address
.equ    OVF1addr,0x006   ;Overflow1 Interrupt Vector Address
.equ    OVF0addr,0x007   ;Overflow0 Interrupt Vector Address
.equ    SPIaddr, 0x008   ;SPI Interrupt Vector Address
.equ    URXCaddr,0x009   ;UART Receive Complete Interrupt Vector Address
.equ    UDREaddr,0x00a   ;UART Data Register Empty Interrupt Vector Address
.equ    UTXCaddr,0x00b   ;UART Transmit Complete Interrupt Vector Address
.equ    ACIaddr, 0x00c   ;Analog Comparator Interrupt Vector Address
