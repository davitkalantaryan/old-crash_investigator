#
# File crash_investigator.pri
# File created : 18 Nov 2021
# Created by : Davit Kalantaryan (davit.kalantaryan@desy.de)
# 


include($${PWD}/../../common/common_qt/sys_common.pri)

CR_INV_HEADERS_H01		= $$cpputilsFindFilesRecursive($${PWD}/../../../include, .h)
CR_INV_HEADERS_HPP01	= $$cpputilsFindFilesRecursive($${PWD}/../../../include, .hpp)

INCLUDEPATH += "$${PWD}/../../../include"
INCLUDEPATH += "$${PWD}/../../../contrib/cpputils/include"

SOURCES += "$${PWD}/../../../src/core/crash_investigator_mallocn_freen.cpp"
SOURCES += "$${PWD}/../../../src/core/crash_investigator_malloc_free.cpp"
SOURCES += "$${PWD}/../../../src/core/crash_investigator_alloc_dealloc.cpp"
SOURCES += "$${PWD}/../../../src/core/crash_investigator_new_delete.cpp"

HEADERS += "$${PWD}/../../../src/core/crash_investigator_alloc_dealloc.hpp"
HEADERS += "$${PWD}/../../../src/core/crash_investigator_mallocn_freen.hpp"

HEADERS += $${CR_INV_HEADERS_H01}
HEADERS += $${CR_INV_HEADERS_HPP01}
