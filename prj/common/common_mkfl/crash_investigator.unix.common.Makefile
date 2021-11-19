

mkfile_path		=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir		=  $(shell dirname $(mkfile_path))
repoRootPathDevSheet	:= $(shell curDir=`pwd` && cd $(mkfile_dir)/../../.. && pwd && cd ${curDir})
ifndef repoRootPath
	repoRootPath		:= $(repoRootPathDevSheet)
endif

COMMON_FLAGS	+= -I$(repoRootPath)/contrib/system/include
COMMON_FLAGS	+= -I$(repoRootPath)/contrib/system/src/include
COMMON_FLAGS	+= -std=c++17 -fPIC
EMFLAGS		+= -fexceptions -DDEVSHEET_WASM

include $(repoRootPathDevSheet)/contrib/cpputils/prj/common/common_mkfl/unix.common.Makefile
