
#include "system_LPC17xx.h"
#include "delay.h"

#define US_TIME    SystemCoreClock / 100000    /* 10 uS */

void _delay_us (int us)
{
    volatile int    i;
    while (us--) {
        for (i = 0; i < US_TIME; i++) 
        {
            ;    /* Burn cycles. */
        }
    }
}

void _delay_ms (int ms)
{
    volatile int i;
    while (ms--) 
   	{
    for (i = 0; i < (US_TIME * 100); i++) 
    {
        ;    /* Burn cycles. */
    }
    }
}