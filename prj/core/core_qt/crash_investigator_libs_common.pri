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

win32{

    #

} else {

    GCCPATH = $$system(which gcc)
    message("!!!!!!!!!!! GCCPATH=$$GCCPATH")
	QMAKE_LFLAGS = -Wl,-E -pie -shared
	TARGET_EXT = so
	QMAKE_EXTENSION_SHLIB = so
    QMAKE_CXXFLAGS = -fPIC
    QMAKE_CFLAGS = -fPIC
    #QMAKE_LFLAGS = -Wl,-E -pie
    #QMAKE_LFLAGS = -Wl,-E -pie -shared
}

DESTDIR = $${PRJ_PWD}/$${SYSTEM_PATH}/lib$${TARGET_PATH_EXTRA}

QT -= gui
QT -= core
QT -= widgets
CONFIG -= qt
