#include "CPU.h"

void init_mclk(void) {
    ATU.TSTR1.BIT.STR0 = 0;	//stop while configging
    ATU0.TIOR.BYTE = 0;	//no input capture
    ATU0.TIER.WORD = 0;	//no interrupts
    ATU0.ITVRR1.BYTE = 0;
    ATU0.ITVRR2A.BYTE = 0;
    ATU0.ITVRR2B.BYTE = 0;	//no ADC intervals
    ATU0.TCNT = 0;
    ATU.TSTR1.BIT.STR0 = 1;	//and GO !
    return;
}
