#ifndef ISR_H
#define ISR_H


#include "stypes.h"
#include "SELECTOR-HEADERS/FLASH_CONST.h"

#define IVT_ENTRIES 0xA5
#define IVT_DEFAULTENTRY ((u32) &die_trace)
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

#define ISR_attrib __attribute__ ((interrupt_handler))

#define FUNCS_DECL static inline
extern u32 ivt[IVT_ENTRIES];



/** Saves unshifted intmask and sets to 0x0F to disable.
 * Use "imask_restore()" , not set_imask() to re-enable.
 */
extern unsigned imask_savedisable(void) //__attribute__ ((always_inline))
;

/** Restore imask; assumes current imask is 0x0F
 * Only use with value obtained from imask_savedisable()
 */
extern void imask_restore(unsigned unshifted_mask);

extern void set_imask(unsigned long mask) //__attribute__ ((always_inline))
;
extern int get_imask(void) __attribute__ ((always_inline));
extern void set_vbr(void *base) //__attribute__ ((always_inline))
;
extern void *get_vbr(void) __attribute__ ((always_inline));


/** Build an IVT
 */
extern void build_ivt(u32 *dest);

/** init interrupts : set all prios to 0, set vbr */
extern void init_ints(void) ;



// ISR handler that saves the previous PC value at the top of RAM, then dies of external WDT.
extern void die_trace(void) __attribute__ ((noreturn));


#endif // ISR_H
