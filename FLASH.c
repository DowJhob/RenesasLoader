#include <string.h>	//memcpy
#include "errcodes.h"
#include "FLASH.h"
#include "ISR.h"
#include "WDT.h"


bool reflash_enabled = 0;	//global flag to protect flash, see platf_flash_enable()

volatile u8 *pFLMCR;	//will point to FLMCR1 or FLMCR2 as required


/** Check FWE and FLER bits
 * ret 1 if ok
 */
static bool fwecheck(void) {
    if (!FLASH.FLMCR1.BIT.FWE) return 0;
    if (FLASH.FLMCR2.BIT.FLER) return 0;
    return 1;
}

/** Set SWE bit and wait */
static void sweset(void) {
    CMT1.CMCSR.BIT.CMIE = 0;	// Disable interrupt on 7051 for erase/write
    FLASH.FLMCR1.BIT.SWE1 = 1;
    waitn(TSSWE);
    return;
}

/** Clear SWE bit and wait */
static void sweclear(void) {
    FLASH.FLMCR1.BIT.SWE1 = 0;
    CMT1.CMCSR.BIT.CMIE = 1;	// Re-enable interrupt
    waitn(TCSWE);
}


/*********** Erase ***********/

/** Erase verification
 * Assumes pFLMCR is set, of course
 * ret 1 if ok
 */
static bool ferasevf(unsigned blockno) {
    bool rv = 1;
    volatile u32 *cur, *end;

    cur = (volatile u32 *) fblocks[blockno];
    end = (volatile u32 *) fblocks[blockno + 1];

    for (; cur < end; cur++) {
        *pFLMCR |= FLMCR_EV;
        waitn(TSEV);
        manual_wdt();
        *cur = 0xFFFFFFFF;
        waitn(TSEVR);
        if (*cur != 0xFFFFFFFF) {
            rv = 0;
            break;
        }
    }
    *pFLMCR &= ~FLMCR_EV;
    waitn(TCEV);

    return rv;
}



/* pFLMCR must be set;
 * blockno validated <= 11 of course
 */
static void ferase(unsigned blockno) {
    unsigned bitsel;

    bitsel = 1;
    while (blockno) {
        bitsel <<= 1;
        blockno -= 1;
    }

    FLASH.EBR2.BYTE = 0;	//to ensure we don't have > 1 bit set simultaneously
    FLASH.EBR1.BYTE = bitsel & 0x0F;	//EB0..3
    FLASH.EBR2.BYTE = (bitsel >> 4) & 0xFF;	//EB4..11

    WDT.WRITE.TCSR = WDT_TCSR_STOP;	//this also clears TCNT
    WDT.WRITE.TCSR = WDT_TCSR_ESTART;
    manual_wdt();

    *pFLMCR |= FLMCR_ESU;
    waitn(TSESU);
    *pFLMCR |= FLMCR_E;	//start Erase pulse
    waitn_tse();
    *pFLMCR &= ~FLMCR_E;	//stop pulse
    waitn(TCE);
    *pFLMCR &= ~FLMCR_ESU;
    waitn(TCESU);

    manual_wdt();
    WDT.WRITE.TCSR = WDT_TCSR_STOP;

    FLASH.EBR1.BYTE = 0;
    FLASH.EBR2.BYTE = 0;

}



uint32_t platf_flash_eb(unsigned blockno) {
    unsigned count;

    if (blockno >= BLK_MAX) return PFEB_BADBLOCK;
    if (!reflash_enabled) return 0;

    if (fblocks[blockno] >= FLMCR2_BEGIN) {
        pFLMCR = &FLASH.FLMCR2.BYTE;
    } else {
        pFLMCR = &FLASH.FLMCR1.BYTE;
    }

    if (!fwecheck()) {
        return PF_ERROR;
    }

    sweset();
    WDT.WRITE.TCSR = WDT_TCSR_STOP;
    WDT.WRITE.RSTCSR = WDT_RSTCSR_SETTING;

    /* XXX is this pre-erase verify really necessary ?? DS doesn't say so;
     * FDT example has this, and Nissan kernel doesn't
     */
#if 0
    if (ferasevf(blockno)) {
        //already blank
        sweclear();
        return 0;
    }
#endif


    for (count = 0; count < MAX_ET; count++) {
        ferase(blockno);
        if (ferasevf(blockno)) {
            sweclear();
#if 0
            if (!fwecheck()) {
                return PF_ERROR_AFTERASE;
            }
#endif
            return 0;
        }
    }

    /* haven't managed to get a succesful ferasevf() : badexit */
    sweclear();
    return PFEB_VERIFAIL;

}



/*********** Write ***********/

/** Copy 32-byte chunk + apply write pulse for tsp=500us
 */
static void writepulse(volatile u8 *dest, u8 *src, unsigned tsp) {
    unsigned uim;
    u32 cur;

    //can't use memcpy because these must be byte transfers
    for (cur = 0; cur < MAX_FLASH_BLOCK_SIZE; cur++) {
        dest[cur] = src[cur];
    }

    uim = imask_savedisable();

    WDT.WRITE.TCSR = WDT_TCSR_STOP;
    WDT.WRITE.TCSR = WDT_TCSR_WSTART;
    manual_wdt();

    *pFLMCR |= FLMCR_PSU;
    waitn(TSPSU);		//F-ZTAT has 300 here
    *pFLMCR |= FLMCR_P;
    waitn(tsp);
    *pFLMCR &= ~FLMCR_P;
    waitn(TCP);
    *pFLMCR &= ~FLMCR_PSU;
    waitn(TCPSU);
    WDT.WRITE.TCSR = WDT_TCSR_STOP;

    imask_restore(uim);
}


/** ret 0 if ok, NRC if error
 * assumes params are ok, and that block was already erased
 */
static u32 flash_write(u32 dest, u32 src_unaligned) {
    u8 src[MAX_FLASH_BLOCK_SIZE] __attribute ((aligned (4)));	// aligned copy of desired data
    u8 reprog[MAX_FLASH_BLOCK_SIZE] __attribute ((aligned (4)));	// retry / reprogram data

    unsigned n;
    bool m;
    u32 rv;

    if (dest < FLMCR2_BEGIN) {
        pFLMCR = &FLASH.FLMCR1.BYTE;
    } else {
        pFLMCR = &FLASH.FLMCR2.BYTE;
    }

#if 0
    if (!fwecheck()) {
        return PF_ERROR_B4WRITE;
    }
#endif

    memcpy(src, (void *) src_unaligned, 32);
    memcpy(reprog, (void *) src, 32);

    sweset();
    WDT.WRITE.TCSR = WDT_TCSR_STOP;
    WDT.WRITE.RSTCSR = WDT_RSTCSR_SETTING;

    for (n=1; n < MAX_WT; n++) {
        unsigned cur;

        m = 0;
        manual_wdt();

        //1) write (latch) to flash, with 500us pulse
        writepulse((volatile u8 *)dest, reprog, TSP500);

        manual_wdt();
#if 0
        if (!fwecheck()) {
            return PF_ERROR_AFTWRITE;
        }
#endif
        //2) Program verify
        *pFLMCR |= FLMCR_PV;
        waitn(TSPV);	//F-ZTAT has 10 here

        for (cur = 0; cur < 32; cur += 4) {
            u32 verifdata;
            u32 srcdata;

            //dummy write 0xFFFFFFFF
            *(volatile u32 *) (dest + cur) = (u32) -1;
            waitn(TSPVR);	//F-ZTAT has 5 here

            verifdata = *(volatile u32 *) (dest + cur);
            srcdata = *(u32 *) (src + cur);

            manual_wdt();
#if 0
            if (!fwecheck()) {
                return PF_ERROR_VERIF;
            }
#endif

            if (verifdata != srcdata) {
                //mismatch:
                m = 1;
            }

            if (srcdata & ~verifdata) {
                //wanted '1' bits, but somehow got '0's : serious error
                rv = PFWB_VERIFAIL;
                goto badexit;
            }
            //compute reprogramming data. This fits with my reading of both the DS and the FDT code,
            //but Nissan proceeds differently
            * (u32 *) (reprog + cur) = srcdata | ~verifdata;
        }	//for (program verif)

        *pFLMCR &= ~FLMCR_PV;
        waitn(TCPV);	//F-ZTAT has 5 here

        if (!m) {
            //success
            sweclear();
            return 0;
        }

    }	//for (n < 400)

    //failed, max # of retries
    rv = PFWB_MAXRET;
badexit:
    sweclear();
    return rv;
}


uint32_t platf_flash_wb(uint32_t dest, uint32_t src, uint32_t len) {

    if (dest > FL_MAXROM) return PFWB_OOB;
    if (dest & 0x1F) return PFWB_MISALIGNED;	//dest not aligned on 32B boundary
    if (len & 0x1F) return PFWB_LEN;	//must be multiple of 32B too

    if (!reflash_enabled) return 0;	//pretend success

    while (len) {
        uint32_t rv = 0;

        rv = flash_write(dest, src);

        if (rv) {
            return rv;
        }

        dest += 32;
        src += 32;
        len -= 32;
    }
    return 0;
}


/*********** init, unprotect ***********/


bool platf_flash_init(u8 *err) {
    reflash_enabled = 0;

    //Check FLER
    if (!fwecheck()) {
        *err = PF_ERROR;
        return 0;
    }

    /* suxxess ! */
    return 1;

}


void platf_flash_unprotect(void) {
    reflash_enabled = 1;
}
