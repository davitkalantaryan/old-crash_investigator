
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
INCLUDEPATH += "$${repositoryRoot}/contrib/stack_investigator/include"
INCLUDEPATH += "$${repositoryRoot}/src/include"

DESTDIR     = $${artifactRoot}/$${SYSTEM_PATH}/$$CONFIGURATION/test
#TARGET_OLD=$${TARGET}
#TARGET = lib$${TARGET_OLD}_$${CRASH_INVEST_VERSION_ENV}.$${SYS_TARGET_EXT}

#DEFINES += CRASH_INVEST_COMPILING_SHARED_LIB
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
SOURCES += $${repositoryRoot}/src/tests/main_any_test.cpp

HEADERS += $$files($${repositoryRoot}/include/*.h,true)
