#ifndef FLASH_H
#define FLASH_H


#include <stdbool.h>
#include "stypes.h"
#include "SELECTOR-HEADERS/FLASH_CONST.h"




static bool reflash_enabled = 0;	//global flag to protect flash, see platf_flash_enable()

static volatile u8 *pFLMCR;	//will point to FLMCR1 or FLMCR2 as required




/** Ret 1 if ok
 *
 * sets *err to a negative response code if failed
 */
bool platf_flash_init(uint8_t *err);


/** Enable modification (erase/write) to flash.
 *
 * If this is not called after platf_flash_init(), the actual calls to erase / write flash are skipped
 */
void platf_flash_unprotect(void);

/** Erase block, see definition of blocks in DS.
 *
 * ret 0 if ok
 */
uint32_t platf_flash_eb(unsigned blockno);

/** Write block of data. len must be multiple of SIDFL_WB_DLEN
 *
 *
 * @return 0 if ok , response code ( > 0x80) if failed.
 *
 * Note : the implementation must not assume that the src address will be 4-byte aligned !
 */
uint32_t platf_flash_wb(uint32_t dest, uint32_t src, uint32_t len);


#endif // FLASH_H
