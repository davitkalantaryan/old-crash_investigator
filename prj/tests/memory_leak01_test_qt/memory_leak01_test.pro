#
# File crash_investigateor.pro
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
# CONFIG += 2test

#QMAKE_CXXFLAGS += "-include types.h"
#DEFINES += u_int=unsigned

#DEFINES += DEBUG_APP

#DEFINES += CRASH_INVEST_DO_NOT_USE_MAL_FREE
DEFINES += use_crash_investigator_analyze_leak_only_new_delete


win32{
	SOURCES += \
} else {
		QMAKE_CXXFLAGS += -std=c++11
        GCCPATH = $$system(which g++)
        message("!!!!!!!!!!! GPPPATH=$$GCCPATH")
	SOURCES += \
}

include("$${PWD}/../../common/common_qt/sys_common.pri")
#include("$${PWD}/../../core/core_qt/crash_investigator.pri")

QT -= core
QT -= gui
CONFIG -= qt


SOURCES += "$${PWD}/../../../src/tests/main_memory_leak01_test.cpp"

INCLUDEPATH += $${PWD}/../../../include
INCLUDEPATH += $${PWD}/../../../contrib/cpputils/include
#SOURCES += "$${PWD}/../../../src/core/analyze/crash_investigator_analyze_leak_only_new_delete.cpp"
#SOURCES += "$${PWD}/../../../contrib/cpputils/src/core/cpputils_hashtbl.cpp"
SOURCES += "$${PWD}/../../../src/core/analyze/crash_investigator_analyze_leak_only_new_delete_std_hash.cpp"

OTHER_FILES +=	\
	"$${PWD}/../../../src/core/analyze/crash_investigator_analyze_leak_only_new_delete.cpp"		\
	"$${PWD}/../../../contrib/cpputils/src/core/cpputils_hashtbl.cpp"				\
	"$${PWD}/../../../src/core/analyze/crash_investigator_analyze_leak_only_new_delete_std_hash.cpp"
