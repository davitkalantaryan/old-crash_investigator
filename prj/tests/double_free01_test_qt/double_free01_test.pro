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

win32{
	SOURCES += \
} else {
        GCCPATH = $$system(which g++)
        message("!!!!!!!!!!! GPPPATH=$$GCCPATH")
	SOURCES += \
}

include($${PWD}/../../common/common_qt/sys_common.pri)

#TEMPLATE = lib

QT -= core
QT -= gui
CONFIG -= qt

H_HEADERS01		= $$cpputilsFindFilesRecursive($${PWD}/../../../include, .h)
HPP_HEADERS01	= $$cpputilsFindFilesRecursive($${PWD}/../../../include, .hpp)


INCLUDEPATH += "$${PWD}/../../../include"
INCLUDEPATH += "$${PWD}/../../../contrib/cpputils/include"

SOURCES += \
	"$${PWD}/../../../src/core/crash_investigator_alloc_dealloc.cpp"		\
	"$${PWD}/../../../src/tests/main_double_free01_test.cpp"

HEADERS += $${H_HEADERS01}
HEADERS += $${HPP_HEADERS01}
