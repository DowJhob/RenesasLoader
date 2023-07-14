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

void build_ivt(u32 *dest) {
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

void init_ints(void) {
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

void die_trace(void) {
    register u32 tmp asm ("r0");
    asm volatile (
        "mov.l	r0, @-r15\n"
        "stc	sr, r0\n"	//critical part is out of the way; disable ints
        "or	#0xF0, r0\n"
        "ldc	r0, sr\n"

    );
    tmp = (RAM_MAX + 1);
    asm volatile (
        "mov.l	r1, @-r15\n"	//save clobbers for full reg dump !
        "mov	r15, r1\n"
        "add	#0x08, r1\n"
        "mov.l	@r1, r1\n"	//get previous PC val off stack
        "mov.l	r1, @-r0\n"	//save at top-of-ram

                        //now, dump all regs
        "mov.l	r15, @-r0\n"
        "mov.l	r14, @-r0\n"
        "mov.l	r13, @-r0\n"
        "mov.l	r12, @-r0\n"
        "mov.l	r11, @-r0\n"
        "mov.l	r10, @-r0\n"
        "mov.l	r9, @-r0\n"
        "mov.l	r8, @-r0\n"
        "mov.l	r7, @-r0\n"
        "mov.l	r6, @-r0\n"
        "mov.l	r5, @-r0\n"
        "mov.l	r4, @-r0\n"
        "mov.l	@r15+, r11\n"	//rescue orig r1 value
        "mov.l	r3, @-r0\n"
        "mov.l	@r15+, r10\n"	//and orig r0
        "mov.l	r2, @-r0\n"
        "mov.l	r11, @-r0\n"
        "mov.l	r10, @-r0\n"
        "0:\n"
        "bra	0b\n"
        "nop\n"

        :
        : "r" (tmp)
        : "r1"
    );
    __builtin_unreachable();
}

