#ifndef FLASH_CONST_H
#define FLASH_CONST_H

#include "CPU_STUFF.h"


#define MAX_FLASH_BLOCK_SIZE 128     // max programmed bytes at once

#define FL_MAXROM	(256*1024UL - 1UL)

/** Common timing constants */
#define TSSWE	WAITN_CALCN(10)
#define TCSWE	WAITN_CALCN(100)  //Not in Hitachi datasheet, but shouldn't hurt

/** Erase timing constants */
#define TSESU	WAITN_CALCN(200)
#define TSE	    WAITN_TSE_CALCN(5000UL) //need to toggle ext WDT pin during wait
#define TCE	    WAITN_CALCN(10)
#define TCESU	WAITN_CALCN(10)
#define TSEV	WAITN_CALCN(10)	/******** Renesas has 20 for this !?? */
#define TSEVR	WAITN_CALCN(2)
#define TCEV	WAITN_CALCN(5)


/** Write timing constants */
#define TSPSU	WAITN_CALCN(300) //Datasheet has 50, F-ZTAT has 300
#define TSP500	WAITN_CALCN(500)
#define TCP		WAITN_CALCN(10)
#define TCPSU	WAITN_CALCN(10)
#define TSPV	WAITN_CALCN(10) //Datasheet has 4, F-ZTAT has 10
#define TSPVR	WAITN_CALCN(5) //Datasheet has 2, F-ZTAT has 5
#define TCPV	WAITN_CALCN(5) //Datasheet has 4, F-ZTAT has 5


/** FLASH constants */
#define MAX_ET	61		// The number of times of the maximum erase
#define MAX_WT	400		// The number of times of the maximum writing
#define BLK_MAX	12		// EB0..EB11
#define FLMCR2_BEGIN 0x8000 //20000 - 3FFFF controlled by FLMCR2


/** FLMCRx bit defines */
#define FLMCR_FWE	0x80
#define FLMCR_FLER	0x80
#define FLMCR_SWE	0x40
#define FLMCR_ESU	0x20
#define FLMCR_PSU	0x10
#define FLMCR_EV	0x08
#define FLMCR_PV	0x04
#define FLMCR_E	    0x02
#define FLMCR_P	    0x01


const u32 fblocks[] = {
    0x00000000,
    0x00001000,
    0x00002000,
    0x00003000,
    0x00004000,
    0x00005000,
    0x00006000,
    0x00007000,
    0x00008000,
    0x00010000,
    0x00020000,
    0x00030000,
    0x00040000,	//last one just for delimiting the last block
};



#endif // FLASH_CONST_H
