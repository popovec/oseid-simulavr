/* test deeply nested functions with gdb patch */

#include <avr/io.h>
#include "common.h"

void func4 (void)
{
    static uint8_t val;
    PORTC = val++;
}

void func3 (void)
{
    func4();
}

void func2 (void)
{
    func3();
}

void func1 (void)
{
    func2();
}

int main(void)
{
    DDRC = 0xff;

    while (1)
    {
        func1();
    }

    return 0;
}
