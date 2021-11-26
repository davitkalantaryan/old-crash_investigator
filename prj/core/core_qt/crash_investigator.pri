#
# File crash_investigator.pri
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# 


include("$${PWD}/../../common/common_qt/sys_common.pri")

QMAKE_CXXFLAGS += -pthread
QMAKE_CFLAGS += -pthread
LIBS += -ldl
LIBS += -pthread


CR_INV_HEADERS_H01		= $$cpputilsFindFilesRecursive($${PWD}/../../../include, .h)
CR_INV_HEADERS_HPP01	= $$cpputilsFindFilesRecursive($${PWD}/../../../include, .hpp)
CR_INV_HEADERS_H02		= $$cpputilsFindFilesRecursive($${PWD}/../../../src/include, .h)
CR_INV_HEADERS_HPP02	= $$cpputilsFindFilesRecursive($${PWD}/../../../src/include, .hpp)

SRC_CORE_BASIC_DIR  = $${PWD}/../../../src/core/basic

INCLUDEPATH += "$${PWD}/../../../include"
INCLUDEPATH += "$${PWD}/../../../contrib/cpputils/include"
INCLUDEPATH += "$${PWD}/../../../src/include"

SOURCES += "$${SRC_CORE_BASIC_DIR}/crash_investigator_alloc_dealloc_unix.cpp"
SOURCES += "$${SRC_CORE_BASIC_DIR}/crash_investigator_alloc_dealloc_analyze.cpp"
SOURCES += "$${SRC_CORE_BASIC_DIR}/crash_investigator_malloc_free.cpp"
SOURCES += "$${SRC_CORE_BASIC_DIR}/crash_investigator_new_delete.cpp"
SOURCES += "$${PWD}/../../../src/core/backtrace/crash_investigator_backtrace_unix.cpp"
SOURCES += "$${PWD}/../../../src/cpputilsm/hashitemsbyptr.cpp"
SOURCES += "$${PWD}/../../../contrib/cpputils/src/core/cpputils_inscopecleaner.cpp"
#SOURCES += "$${PWD}/../../../contrib/cpputils/src/core/cpputils_hashtbl.cpp"

HEADERS += "$${SRC_CORE_BASIC_DIR}/crash_investigator_alloc_dealloc.hpp"

HEADERS += $${CR_INV_HEADERS_H01}
HEADERS += $${CR_INV_HEADERS_HPP01}
HEADERS += $${CR_INV_HEADERS_H02}
HEADERS += $${CR_INV_HEADERS_HPP02}

OTHER_FILES +=	\
	$${PWD}/../core_mkfl/crash_investigator.unix.Makefile
