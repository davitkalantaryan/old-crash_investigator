#
# File doocs_client_common.pri
# File created : 12 Feb 2017
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# This file can be used to produce Makefile for daqadcreceiver application
# for PITZ
#


message("!!! doocs_client_common.pri:")
include($${PWD}/sys_common.pri)

MYDOOCS = /afs/ifh.de/group/pitz/doocs
LIBS += -L$$MYDOOCS/system_arch/$$CODENAME/lib
INCLUDEPATH += $$MYDOOCS/include/doocs

#MYDOOCS = /export/doocs
#LIBS += -L/export/doocs/lib
#INCLUDEPATH += /export/doocs/lib/include

win32 {
    contains(QMAKE_TARGET.arch, x86_64) {
        ## Windows x64 (64bit) specific build here

    } else {
        ## Windows x86 (32bit) specific build here
    }

} else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-attributes
    QMAKE_CXXFLAGS += -std=c++14
}

CONFIG += c++14

DEFINES += LINUX

LIBS += -lDOOCSapi
LIBS += -lgul14
LIBS += -ltinemt
LIBS += -lldap
LIBS += -lrt
