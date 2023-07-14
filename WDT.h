#ifndef WDT_H
#define WDT_H


#include <stdint.h>
#include "ISR.h"
#include "SELECTOR-HEADERS/IODEF.h"

/*** WDT and master clock stuff
 * want to toggle the WDT every X ms (2ms on Nissan)
 */

//#define WDT_PER_MS	2
    /* somehow shc sucks at reducing the following :
     * u16 WDT_MAXCNT = WDT_PER_MS / 1000 * (40*1000*1000ULL / 64))
     */
//#define WDT_MAXCNT WDT_PER_MS * 40*1000*1000UL / 64 / 1000
#define WDT_MAXCNT 4125 //aim for 6.6ms , although it probably works at 2ms anyway


#define trapa(trap_no)	asm ("trapa  %0"::"i"(trap_no))

extern volatile uint16_t *wdt_dr;	//such as &PL.DR.WORD
extern uint16_t wdt_pin;	//mask for PxDR

/** toggle pin. This is called from a periodic interrupt */
void wdt_tog(void);

//implemented in main.c
void wdt_tog(void);



/** WDT toggle interrupt
 *
 * clr timer and flag, easy
 */
void INT_ATU11_IMI1A(void) ISR_attrib;
void INT_ATU11_IMI1A(void) {

    wdt_tog();
    ATU1.TCNTB = 0;
    ATU1.TSRB.BIT.CMF = 0;	//TCNT1B compare match
    return;
}



static void pre_init_wdt(void);

/*
 * Assumes the wdt pin setup has already been done,
 * since host was taking care of it just before.
 * This uses Channel 1 TCNTB CMF interrupt
 */
static void init_wdt(void);


static void manual_wdt(void);

static void waitn_tse(void) {
    u32 start = ATU0.TCNT;
    while ((ATU0.TCNT - start) < TSE)
    {
        manual_wdt();
    }
}


/** force reset by external supervisor and/or internal WDT.
 */
void die(void);

#endif // WDT_H
