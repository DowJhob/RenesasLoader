#include "ISR.h"
#include "SELECTOR-HEADERS/CPU.h"
#include "SELECTOR-HEADERS/VECT_TBL.h"
#include "FLASH.h"
#include "WDT.h"

/** Build an IVT
 */
//static void build_ivt(u32 *dest) {
//    unsigned i;

//    // Fill IVT with dummy entries
//    for (i = 0; i < IVT_ENTRIES; i++) {
//        dest[i] = IVT_DEFAULTENTRY;
//    }

//#define WRITEVECT(vectno, ptr) dest[vectno] = (u32) (ptr)

//    WRITEVECT(IVTN_POR_SP, stackinit);
//    WRITEVECT(IVTN_MR_SP, stackinit);
//    WRITEVECT(IVTN_INT_CMT1_CMTI1, &INT_CMT1_CMTI1); // Compare match in lieu of no OCR on ATU

//}

static void build_ivt(u32 *dest) {
    unsigned i;

    // Fill IVT with dummy entries
    for (i = 0; i < IVT_ENTRIES; i++) {
        dest[i] = IVT_DEFAULTENTRY;
    }

#define WRITEVECT(vectno, ptr) dest[vectno] = (u32) (ptr)

    WRITEVECT(IVTN_POR_SP, stackinit);
    WRITEVECT(IVTN_MR_SP, stackinit);
    WRITEVECT(IVTN_INT_ATU11_IMI1A, &INT_ATU11_IMI1A);

}


/** init interrupts : set all prios to 0, set vbr */
//static void init_ints(void) {
//    INTC.IPRA.WORD = 0;
//    INTC.IPRB.WORD = 0;
//    INTC.IPRC.WORD = 0;
//    INTC.IPRD.WORD = 0;
//    INTC.IPRE.WORD = 0;
//    INTC.IPRF.WORD = 0;
//    INTC.IPRG.WORD = 0;
//    INTC.IPRH.WORD = 0;
//    set_vbr((void *) ivt);
//    return;
//}

static void init_ints(void) {
    INTC.IPRA.WORD = 0;
//	INTC.IPRB.WORD = 0;
    INTC.IPRC.WORD = 0;
    INTC.IPRD.WORD = 0;
    INTC.IPRE.WORD = 0;
    INTC.IPRF.WORD = 0;
    INTC.IPRG.WORD = 0;
    INTC.IPRH.WORD = 0;
    INTC.IPRI.WORD = 0;
    INTC.IPRJ.WORD = 0;
    INTC.IPRK.WORD = 0;
    INTC.IPRL.WORD = 0;
    UBC.UBCR.BIT.UBID = 1;	//disable UBC
    set_vbr((void *) ivt);
    return;
}
