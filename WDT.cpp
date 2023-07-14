#include "WDT.h"
#include "SH7052/IODEF.h"
#include "SH7052/VECT_TBL.h"



/** WDT toggle interrupt
 *
 * clr timer and flag, easy
 */
void INT_CMT1_CMTI1(void) ISR_attrib;

void INT_CMT1_CMTI1(void) {

    wdt_tog();
    CMT1.CMCNT = 0;
    CMT1.CMCSR.BIT.CMF = 0;	//compare match in lieu of no OCR on ATU
    return;
}


/*
 * Assumes the wdt pin setup has already been done,
 * since host was taking care of it just before.
 * This uses Channel 1 TCNTB CMF interrupt
 */
void init_wdt() {
    ATU.TSTR1.BIT.STR12B = 0;	//stop while configging
    ATU1.TIORA.BYTE = 0;
    ATU1.TIORB.BYTE = 0;
    ATU1.TIORC.BYTE = 0;
    ATU1.TIORD.BYTE = 0;
    //ATU1.TSRA.WORD = 0;
    ATU1.TSRB.WORD = 0;

    ATU1.OCR = WDT_MAXCNT;
    ATU1.TCNTB = 0;
    INTC.IPRD.BIT._ATU11 = 0x08;	//medium priority for ATU11_IMI1A / CM1 (same int)
    ATU1.TIERA.WORD = 0;	//disable other IMI1{A,B,C,D} !!
    ATU1.TIERB.BIT.CME = 1;	//TCNTB compare match int

    ATU.PSCR1.BYTE = 0x1F;	//prescaler : 1/32
    ATU1.TCRB.BYTE = 0;		//clksel : 1:1 , from int_clock/2 => 625kHz(1.6us increments)
    ATU.TSTR1.BIT.STR12B = 1;		//start only TCNT1B
    return;
}



//void die() {
//    set_imask(0x0F);
//    set_vbr(0);
//    WDT.WRITE.RSTCSR = WDT_RSTCSR_SETTING;
//    WDT.WRITE.TCSR = (0xA578 | 0);	// clk div2 for ~ 25us overflow
//    while (1) {}
//    return;
//}

/*
 * Some ECUs (VC264) don't seem to reset properly with just the external WDT.
 * But, internal WDT-triggered POR doesn't reset certain periph regs
 * which *may* be causing rare issues (spurious ignition on stopkernel !?).
 * Or were those because I previously wasn't setting vbr=0 ?
 */
void die(void) {
    set_imask(0x0F);
    set_vbr(0);
    trapa(0x3F);
    //unreachable
//	WDT.WRITE.RSTCSR = WDT_RSTCSR_SETTING;
//	WDT.WRITE.TCSR = (0xA578 | 0);	// clk div2 for ~ 12us overflow
    while (1) {}
    return;
}

void manual_wdt() {
    if (CMT1.CMCNT >= (WDT_MAXCNT - 50)) { // -50 so we can hopefully be close enough after waitn calcs
        wdt_tog();
        CMT1.CMCNT = 0;
        CMT1.CMCSR.BIT.CMF = 0;
    }
}

void pre_init_wdt()
{
    wdt_dr = &PB.DR.WORD;  //manually assign these values, not elegant but will do for now
    wdt_pin = 0x8000;

    // set PD8 to bring FWE high to prepare for erasing or flashing
    PD.DR.WORD |= 0x0100;
}
