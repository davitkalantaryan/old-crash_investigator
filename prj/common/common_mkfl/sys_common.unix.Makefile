

mkfile_path			=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir			=  $(shell dirname $(mkfile_path))
repoRootPathCrIn	:= $(shell curDir=`pwd` && cd $(mkfile_dir)/../../.. && pwd && cd ${curDir})
ifndef repoRootPath
	repoRootPath	:= $(repoRootPathCrIn)
endif

COMMON_FLAGS	+=  -std=c++14

include $(repoRootPathCrIn)/contrib/cpputils/prj/common/common_mkfl/unix.common.tmp.Makefile
