#include "delay.h"

__asm void SysCtlDelay(unsigned long ulCount)
{
    subs    r0, #1;
		nop;
    bne     SysCtlDelay;
    bx      lr;
}
