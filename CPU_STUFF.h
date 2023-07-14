#ifndef CPU_STUFF_H
#define CPU_STUFF_H

#include "stypes.h"
#include "SELECTOR-HEADERS/IODEF.h"
#include "SELECTOR-HEADERS/CPU.h"




#define MCLK_GETMS(x) ((x) * 16 / 10000)	/* convert ticks to milliseconds */
#define MCLK_GETTS(x) ((x) * 10000 / 16) /* convert millisec to ticks */
#define MCLK_MAXSPAN	10000	/* arbitrary limit (in ms) for time spans measured by MCLK */
/** Get current timestamp from free-running counter */
//uint32_t get_mclk_ts(void);
#define get_mclk_ts(x) (ATU0.TCNT)

#define WAITN_CALCN(usec) (((usec) * CPUFREQ / WAITN_TCYCLE) + 1)
#define WAITN_TSE_CALCN(usec) (((usec) * CPUFREQ / ATUPRESCALER))

///** Common timing constants */
//#define TSSWE	WAITN_CALCN(10)
//#define TCSWE	WAITN_CALCN(100)  //Not in Hitachi datasheet, but shouldn't hurt


static void waitn(unsigned loops) {
    u32 tmp;
    asm volatile ("0: dt %0":"=r"(tmp):"0"(loops):"cc");
    asm volatile ("bf 0b");
}



#endif // CPU_STUFF_H
