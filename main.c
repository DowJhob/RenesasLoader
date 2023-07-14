// #include <stdio.h>
#include "ISR.h"
#include "SERIAL.h"
#include "SELECTOR-HEADERS/CPU.h"
#include "WDT.h"
#include "SELECTOR-HEADERS/IODEF.h"
#include "cmd_parser.h"

void main()
{
    set_imask(0x0F);	// disable interrupts (mask = b'1111)
    pre_init_wdt();

    init_wdt();
    build_ivt(ivt);
    init_ints();
    /* stop all timers */
    ATU.TSTR1.BYTE = 0;
    ATU.TSTR2.BYTE = 0;
    ATU.TSTR3.BYTE = 0;
    init_mclk();
    init_sci();
    init_wdt();

    /* and lower prio mask to let WDT run */
    set_imask(0x07);

    cmd_init(SCI_DEFAULTDIV);
    cmd_loop();
    //we should never get here; if so : die
    die();
}
