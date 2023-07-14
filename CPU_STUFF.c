#include "CPU_STUFF.h"

void waitn(unsigned loops) {
    u32 tmp;
    asm volatile ("0: dt %0":"=r"(tmp):"0"(loops):"cc");
    asm volatile ("bf 0b");
}