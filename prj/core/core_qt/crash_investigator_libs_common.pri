#
# File crash_investigateor.pro
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
# CONFIG += 2test

#TEMPLATE = lib

include($${PWD}/../../common/common_qt/sys_common.pri)
include("$${PWD}/crash_investigator.pri")


macx {
        # case of MAC
        message ("!!!!!!!!!! mac")
        QMAKE_CXXFLAGS += -Wall
        QMAKE_CXXFLAGS += -Werror
} else:win32 {
        # case of windows
        QMAKE_CXXFLAGS += /FI"devsheet/core/devsheet_disable_warnings.h"
        QMAKE_CXXFLAGS += /Wall /WX
        #QMAKE_CXXFLAGS += /showIncludes
        contains(QMAKE_TARGET.arch, x86_64) {
                message ("!!!!!!!!!! windows 64")
        } else {
                message ("!!!!!!!!!! windows 32")
        }
} else:linux {
        # case of Linux
        message ("!!!!!!!!!! linux")
        GCCPATH = $$system(which gcc)
        message("!!!!!!!!!!! GCCPATH=$$GCCPATH")
        QMAKE_CXXFLAGS += -Wall
        QMAKE_CXXFLAGS += -Werror
        QMAKE_CXXFLAGS += -Wno-attributes

        QMAKE_LFLAGS = -Wl,-E -pie -shared
        TARGET_EXT = so
        QMAKE_EXTENSION_SHLIB = so
        QMAKE_CXXFLAGS = -fPIC
        QMAKE_CFLAGS = -fPIC
        #QMAKE_LFLAGS = -Wl,-E -pie
        #QMAKE_LFLAGS = -Wl,-E -pie -shared
} else:android {
        message ("!!!!!!!!!! android")
        QMAKE_CXXFLAGS += -Werror
} else:ios {
        message ("!!!!!!!!!! ios")
        QMAKE_CXXFLAGS += -Wall
        QMAKE_CXXFLAGS += -Werror
} else {
        # WASM
        message ("!!!!!!!!!! wasm")
        DEFINES += DEVSHEET_WASM
        #DEFINES += USE_DLOPEN_FROM_WASM
        QMAKE_CXXFLAGS += -Wall
        QMAKE_CXXFLAGS += -Werror
        QMAKE_CXXFLAGS += -fexceptions
        #QMAKE_CXXFLAGS += -s DISABLE_EXCEPTION_CATCHING=0 -s ASYNCIFY -O3
        QMAKE_CXXFLAGS += -s DISABLE_EXCEPTION_CATCHING=0 -O3 $$(EXTRA_WASM_FLAGS)

        # trying dynamic linking
        #QMAKE_CXXFLAGS += -s MAIN_MODULE=1 -s EXPORT_ALL=1 -fPIC
        #QMAKE_CXXFLAGS += -s MAIN_MODULE=1
        #QMAKE_LFLAGS += -s MAIN_MODULE=1
}



DESTDIR = $${PRJ_PWD}/$${SYSTEM_PATH}/lib$${TARGET_PATH_EXTRA}

QT -= gui
QT -= core
QT -= widgets
CONFIG -= qt

OTHER_FILES +=	\
	$${PWD}/../core_mkfl/crash_investigator_libs_common.unix.Makefile
