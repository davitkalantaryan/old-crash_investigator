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

INCLUDEPATH += "$${PWD}/../../../include"
INCLUDEPATH += "$${PWD}/../../../contrib/cpputils/include"
INCLUDEPATH += "$${PWD}/../../../src/include"

SOURCES += "$${PWD}/../../../src/core/crash_investigator_alloc_dealloc.cpp"
SOURCES += "$${PWD}/../../../src/core/crash_investigator_malloc_free.cpp"
SOURCES += "$${PWD}/../../../src/core/crash_investigator_new_delete.cpp"
SOURCES += "$${PWD}/../../../src/cpputilsm/hashitemsbyptr.cpp"
#SOURCES += "$${PWD}/../../../contrib/cpputils/src/core/cpputils_hashtbl.cpp"

HEADERS += "$${PWD}/../../../src/core/crash_investigator_alloc_dealloc.hpp"
HEADERS += "$${PWD}/../../../src/include/cpputilsm/hashitemsbyptr.hpp"
HEADERS += "$${PWD}/../../../src/include/cpputilsm/hashitemsbyptr.impl.hpp"

HEADERS += $${CR_INV_HEADERS_H01}
HEADERS += $${CR_INV_HEADERS_HPP01}

OTHER_FILES +=	\
	$${PWD}/../core_mkfl/crash_investigator.unix.Makefile
