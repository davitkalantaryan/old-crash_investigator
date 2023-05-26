#
# File crash_investigateor.pro
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
# CONFIG += 2test


TEMPLATE = lib

DEFINES += MEM_HANDLER_MMAP_NEEDED

linux {
        # case of Linux
        message ("!!!!!!!!!! linux")
        GCCPATH = $$system(which gcc)
        message("!!!!!!!!!!! GCCPATH=$$GCCPATH")
        QMAKE_CXXFLAGS += -Wall
        QMAKE_CXXFLAGS += -Werror
        QMAKE_CXXFLAGS += -Wno-attributes
        LIBS += -ldl

        #QMAKE_LFLAGS = -Wl,-E -pie -shared
        #TARGET_EXT = so
        #QMAKE_EXTENSION_SHLIB = so
        #SYS_TARGET_EXT = so
        #QMAKE_CXXFLAGS = -fPIC
        #QMAKE_CFLAGS = -fPIC
}

include($${PWD}/../../common/common_qt/sys_common.pri)
include($${repositoryRoot}/ENVIRONMENT)

VERSION = $$CRASH_INVEST_VERSION_ENV

INCLUDEPATH += "$${repositoryRoot}/include"
INCLUDEPATH += "$${cinternalRepoRoot}/include"
INCLUDEPATH += "$${repositoryRoot}/contrib/stack_investigator/include"


DEFINES += CRASH_INVEST_COMPILING_SHARED_LIB
DEFINES += STACK_INVEST_ANY_ALLOC=MemoryHandlerCLibMalloc
DEFINES += STACK_INVEST_ANY_FREE=MemoryHandlerCLibFree
DEFINES += STACK_INVEST_C_LIB_FREE_NO_CLBK=MemoryHandlerCLibFree

QT -= gui
QT -= core
QT -= widgets
CONFIG -= qt

SOURCES += $$files($${repositoryRoot}/src/core/linux_simple/*.c*,true)
SOURCES += $${cinternalRepoRoot}/src/core/cinternal_core_hash_lhash.c
SOURCES += $${repositoryRoot}/contrib/stack_investigator/src/core/stack_investigator_backtrace_unix.c
SOURCES += $${repositoryRoot}/contrib/stack_investigator/src/core/stack_investigator_backtrace_common.c

HEADERS += $$files($${repositoryRoot}/include/*.h,true)
HEADERS += $$files($${repositoryRoot}/contrib/stack_investigator/include/*.h,true)


OTHER_FILES +=	\
	$${PWD}/../core_mkfl/crash_investigator_malloc.unix.Makefile
