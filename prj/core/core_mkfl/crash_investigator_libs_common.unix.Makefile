
mkfile_path		=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir		=  $(shell dirname $(mkfile_path))

include $(mkfile_dir)/crash_investigator.unix.Makefile

COMMON_FLAGS	+=  -Werror
COMMON_FLAGS	+=  -Wno-attributes
COMMON_FLAGS	+=  -fPIC

LFLAGS			+= -Wl,-E -pie -shared

targetDirPath	= $(repoRootPath)/sys/$(lsbCode)/$(Configuration)/lib
targetFilePath	= $(targetDirPath)/$(targetName)_$(CRASH_INVEST_VERSION_ENV).so
