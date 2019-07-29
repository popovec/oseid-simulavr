/*
 * $Id: demo.c,v 1.6 2003/09/10 04:59:36 troth Exp $
 *
 * Counter from 0xff down to 0x00, output to leds ~cnt.
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "common.h"

int main(void)
{
#if TEST_BIG_STR
#   include "big_str.h"
#endif /* TEST_BIG_STR */

    uint8_t cnt;

    DDRB = 0xff;                /* enable port b for output */

    for ( cnt=0xff; cnt > 0; cnt-- )
        PORTB = cnt;

    return 0;
}
