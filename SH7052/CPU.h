#ifndef CPU_H
#define CPU_H

#include "stypes.h"
#include "IODEF.h"

/********** Timing defs
*/

//20MHz clock. Some critical timing depends on this being true,
//WDT stuff in particular isn't macro-fied
#define CPUFREQ	(40)
#define ATUPRESCALER (32)

#define WDT_RSTCSR_SETTING 0x5A4F	//reset if TCNT overflows
#define WDT_TCSR_ESTART (0xA578 | 0x06)	//write value to start with 1:4096 div (52.4 ms @ 20MHz), for erase runaway
#define WDT_TCSR_WSTART (0xA578 | 0x05)	//write value to start with 1:1024 div (13.1 ms @ 20MHz), for write runaway
#define WDT_TCSR_STOP 0xA558	//write value to stop WDT count

#define WAITN_TCYCLE 4		/* clock cycles per loop, see asm */



extern u32 stackinit[];	/* ptr set at linkage */




/** init free-running main counter, to use for timestamps etc
 * Get current timestamp with MCLK_TS

 * This uses the 32-bit Channel 0. It usees the same prescaler (#1)
 * as Channel 1, so 1/32 => 625kHz (t=1.6us). This gives a measurable
 * span of ~6872 seconds for intervals. Plenty enough for iso14230
 */

extern void init_mclk(void);

#endif // CPU_H
