#ifndef VEHICLE_SELECTOR_H
#define VEHICLE_SELECTOR_H

#define MMC

#if defined(MMC)
    #define TARGET_SCI SCI0

#elif defined(SSM)
    #define TARGET_SCI SCI1

#elif defined(NIS)
    #define TARGET_SCI SCI1

#elif defined(MAZ)
    #define TARGET_SCI SCI1
#elif defined(other)
    #define TARGET_SCI SCI1
//#else
//    #error No target specified !

#endif
#endif // VEHICLE_SELECTOR_H
