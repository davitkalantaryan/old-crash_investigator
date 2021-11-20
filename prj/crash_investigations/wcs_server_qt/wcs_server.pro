#
# File wcs_server.pro
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

include("$${PWD}/../../common/common_qt/doocs_server_common.pri")
include("$${PWD}/../../core/crash_investigator_qt/crash_investigator.pri")

QMAKE_CXXFLAGS += -pthread
QMAKE_CFLAGS += -pthread
LIBS += -pthread


SPS_NEXUS_DIR	= $${PWD}/../../../.codes_to_investigate/SPS
INCLUDEPATH += "$${SPS_NEXUS_DIR}/contrib/tinyfsm/include"
INCLUDEPATH += "$${SPS_NEXUS_DIR}/src/common"
INCLUDEPATH += "$${SPS_NEXUS_DIR}/src/wcs_server"
INCLUDEPATH += $$MYDOOCS/include/doocs/doocs


CC_SOURCES01	= $$cpputilsFindFilesRecursive($${SPS_NEXUS_DIR}/src/wcs_server, .cc)
CPP_SOURCES01	= $$cpputilsFindFilesRecursive($${SPS_NEXUS_DIR}/src/wcs_server, .cpp)
#CC_SOURCES02	= $$cpputilsFindFilesRecursive($${SPS_NEXUS_DIR}/src/common, .cc)
#CPP_SOURCES02	= $$cpputilsFindFilesRecursive($${SPS_NEXUS_DIR}/src/common, .cpp)
H_HEADERS01	= $$cpputilsFindFilesRecursive($${SPS_NEXUS_DIR}/src/wcs_server, .h)
H_HEADERS02	= $$cpputilsFindFilesRecursive($${SPS_NEXUS_DIR}/src/common, .h)



SOURCES += \
	"$${SPS_NEXUS_DIR}/src/wcs_server/wcs_rpc_server.cc"		\
	"$${SPS_NEXUS_DIR}/src/wcs_server/WCS.cc"					\
	"$${SPS_NEXUS_DIR}/src/wcs_server/WCSTDS.cc"				\
	"$${SPS_NEXUS_DIR}/src/wcs_server/WCSBooster.cc"			\
	"$${SPS_NEXUS_DIR}/src/wcs_server/WCSBoosterLaser.cc"		\
	"$${SPS_NEXUS_DIR}/src/wcs_server/WCSGun.cc"				\
	"$${SPS_NEXUS_DIR}/src/wcs_server/external.cc"				\
	"$${SPS_NEXUS_DIR}/src/wcs_server/wcsfsm.cc"				\
	"$${SPS_NEXUS_DIR}/src/wcs_server/udpspsext.cc"				\
	"$${SPS_NEXUS_DIR}/src/wcs_server/wcs_types.cc"				\
	"$${SPS_NEXUS_DIR}/src/wcs_server/D_button_sps.cc"			\
	"$${SPS_NEXUS_DIR}/src/wcs_server/wcs_fsm.cc"				\
	\
	"$${SPS_NEXUS_DIR}/src/common/D_cmd_sps.cc"					\
	"$${SPS_NEXUS_DIR}/src/common/udp_peer.cpp"

HEADERS += $${H_HEADERS01}
HEADERS += $${H_HEADERS02}
