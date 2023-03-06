
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
        #QMAKE_LFLAGS = -Wl,-E -pie
        #QMAKE_LFLAGS = -Wl,-E -pie -shared
}

include($${PWD}/../../common/common_qt/sys_common.pri)
include($${repositoryRoot}/ENVIRONMENT)

INCLUDEPATH += "$${repositoryRoot}/include"
INCLUDEPATH += "$${cinternalRepoRoot}/include"
INCLUDEPATH += "$${repositoryRoot}/src/include"

DESTDIR     = $${artifactRoot}/$${SYSTEM_PATH}/$$CONFIGURATION/test
#TARGET_OLD=$${TARGET}
#TARGET = lib$${TARGET_OLD}_$${CRASH_INVEST_VERSION_ENV}.$${SYS_TARGET_EXT}

DEFINES += CRASH_INVEST_COMPILING_SHARED_LIB

QT -= gui
QT -= core
QT -= widgets
CONFIG -= qt

SOURCES += $$files($${repositoryRoot}/src/core/linux_simple/*.c*,true)
SOURCES += $${repositoryRoot}/src/tests/main_any_test.cpp

HEADERS += $$files($${repositoryRoot}/include/*.h,true)
