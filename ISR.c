#include "ISR.h"
#include "SELECTOR-HEADERS/CPU.h"
#include "SELECTOR-HEADERS/VECT_TBL.h"
#include "FLASH.h"
#include "WDT.h"

u32 ivt[IVT_ENTRIES];
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


 inline void set_imask(unsigned long mask)
{
    mask <<= 4;
    mask &= 0xf0;

    asm volatile (
        "stc	sr, r0 \n"
        "or	#0xF0, r0 \n"	// set IIII=b'1111
        "xor	#0xF0, r0 \n"	// equiv. to "R0 = (SR & 0xFFFF FF0F)"
        "or	%0,r0\n"
        "ldc	r0,sr"
            :
            :"r" (mask)
            :"r0"
    );

}

 inline int get_imask()
{
    volatile int val;
    asm volatile (
        "stc         sr,r0\n"
        "shlr2       r0\n"
        "shlr2       r0\n"
        "and         #15,r0\n"
        "mov r0,%0"
            :"=r"(val)
            :
            :"r0"
    );
    return val;
}
 inline void set_vbr(void *vbr)
{
    asm volatile (
        "mov %0, r2\n"
        "ldc r2,vbr"
            :
            :"r"(vbr)
            :"r2"
    );
}
 inline void* get_vbr(void)
{
    void *ptr;
    asm volatile (
        "stc vbr,r2\n"
        "mov.l r2, %0"
            :"=m"(ptr)
            :
            :"r2"
    );
    return ptr;
}



__inline__ unsigned imask_savedisable() {
    unsigned val;

#if 0 //old implem
    volatile unsigned tmp;
    asm volatile (
        "stc   sr,%0\n"
        "mov   %0, %1\n"
        "mov   #0xF0, r0\n"
        "extu.b r0, r0\n"	//r0 = 0000 00F0
        "and   r0, %0\n"	//%0 (val) = 0000 00<I3:I0>0
        "or    r0, %1\n"
        "ldc   %1, sr"
            :"=r"(val),"=r"(tmp)
            :
            :"r0"
    );
#else
    asm volatile (
        "stc	sr, r0 \n"
        "mov r0, %0 \n"		//save orig SR
        "or	#0xF0, r0 \n"
        "ldc	r0, sr \n"	//disable
        "mov	%0, r0 \n"
        "and	#0xF0, r0 \n"	//keep only b'IIII0000 (#imm8 is zero-extended)
        "mov	r0, %0 \n"
            :"=r"(val)
            :
            :"r0"
    );
#endif
    return val;
}

__inline__ void imask_restore(unsigned unshifted_mask) {
#if 0 //old implem ; gcc sometimes uses the same reg for tmp2 and unshifted_mask !?? (fails)
    volatile unsigned tmp2;
    asm volatile (
        "stc   sr,r0\n"
        "mov   #0xff,%0\n"
        "shll8 %0\n"
        "add   #0x0F, %0\n"
        "and   %0,r0\n"
        "or    %1,r0\n"
        "ldc   r0,sr"
            :"=r"(tmp2)
            :"r" (unshifted_mask)
            :"r0"
    );
#endif
    asm volatile (
        "stc	sr, r0 \n"
        "or	#0xF0, r0 \n"	// set IIII=b'1111
        "xor #0xF0, r0 \n"	// equiv. to "R0 = (SR & 0xFFFF FF0F)"
        "or	%0, r0 \n"
        "ldc	r0, sr"
            :
            :"r" (unshifted_mask)
            :"r0"
    );
}


/** init interrupts : set all prios to 0, set vbr */

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

