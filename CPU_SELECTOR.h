#ifndef CPU_SELECTOR_H
#define CPU_SELECTOR_H


#define SH7052

#if defined(SH7058)
    #include "reg_defines/7058.h"
    #define RAM_MIN	0xFFFF0000
    #define RAM_MAX 	0xFFFFBFFF

#elif defined(SH7055_18)
    #include "reg_defines/7055_180nm.h"
    #define RAM_MIN	0xFFFF6000
    #define RAM_MAX	0xFFFFDFFF

#elif defined(SH7055_35)
    #include "reg_defines/7055_350nm.h"
    #define RAM_MIN	0xFFFF6000
    #define RAM_MAX	0xFFFFDFFF

#elif defined(SH7052)
    // #define ECU             "SH7052"
    #define FLASH_CONST     "SH7052/FLASH_CONST.h"
    #define CPU             "SH7052/CPU.h"
    #define IODEF           "SH7052/IODEF.h"
    #define VECT_TBL        "SH7052/VECT_TBL.h"
    #define VEHICLE_DEPENDS "SH7052/VEHICLE_DEPENDS.h"
    #define RAM_MIN         0xFFFF8000
    #define RAM_MAX         0xFFFFAFFF

#elif defined(SH7051)
    #include "reg_defines/7051.h"
    #define RAM_MIN	0xFFFFD800
    #define RAM_MAX	0xFFFFFFFF

//#else
//    #error No target specified !
#endif


#endif // CPU_SELECTOR_H
