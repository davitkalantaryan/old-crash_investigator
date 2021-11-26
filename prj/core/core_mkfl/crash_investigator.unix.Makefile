

mkfile_path		=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir		=  $(shell dirname $(mkfile_path))

include $(mkfile_dir)/../../common/common_mkfl/sys_common.unix.Makefile

COMMON_FLAGS	+=  -pthread
COMMON_FLAGS	+=  -I"$(repoRootPath)/include"
COMMON_FLAGS	+=  -I"$(repoRootPath)/contrib/cpputils/include"
COMMON_FLAGS	+=  -I"$(repoRootPath)/src/include"

LFLAGS			+= -pthread
LFLAGS			+= -ldl

objectFilesDirPath	= $(repoRootPath)/sys/$(lsbCode)/$(Configuration)/.objects/$(targetName)
# SRC_CORE_BASIC_DIR      = $(repoRootPath)/src/core/basic

CR_INV_OBJECTS = \
        $(objectFilesDirPath)/core/basic/crash_investigator_alloc_dealloc_analyze.cpp.o \
        $(objectFilesDirPath)/core/basic/crash_investigator_alloc_dealloc_unix.cpp.o \
        $(objectFilesDirPath)/core/basic/crash_investigator_malloc_free.cpp.o	\
        $(objectFilesDirPath)/core/basic/crash_investigator_new_delete.cpp.o
