TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        FLASH.cpp \
        ISR.cpp \
        SERIAL.cpp \
        WDT.cpp \
        cmd_parser.c \
        crc.c \
        eep_funcs.c \
        main.c

HEADERS += \
    SELECTOR-HEADERS/CPU.h \
    SELECTOR-HEADERS/FLASH_CONST.h \
    SELECTOR-HEADERS/IODEF.h \
    SELECTOR-HEADERS/VECT_TBL.h \
    CPU_SELECTOR.h \
    CPU_STUFF.h \
    FLASH.h \
    ISR.h \
    SERIAL.h \
    SH7052/CPU.h \
    SH7052/FLASH_CONST.h \
    SH7052/VECT_TBL.h \
    VEHICLE_SELECTOR.h \
    WDT.h \
    cmd_parser.h \
    crc.h \
    eep_funcs.h \
    errcodes.h \
    SH7052/IODEF.h \
    iso_cmds.h \
    stypes.h
