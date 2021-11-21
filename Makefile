
mkfile_path		=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir		=  $(shell dirname $(mkfile_path))

all:
	@make -C $(mkfile_dir)/prj/core/core_mkfl -f crash_investigator_malloc.unix.Makefile CPPUTILS_DEBUG=1
	@make -C $(mkfile_dir)/prj/core/core_mkfl -f crash_investigator_new_malloc.unix.Makefile CPPUTILS_DEBUG=1
	@make -C $(mkfile_dir)/prj/core/core_mkfl -f crash_investigator_new.unix.Makefile CPPUTILS_DEBUG=1
	

.PHONY: clean
clean:
	@make -C $(mkfile_dir)/prj/core/core_mkfl -f crash_investigator_malloc.unix.Makefile CPPUTILS_DEBUG=1 clean
	@make -C $(mkfile_dir)/prj/core/core_mkfl -f crash_investigator_new_malloc.unix.Makefile CPPUTILS_DEBUG=1 clean
	@make -C $(mkfile_dir)/prj/core/core_mkfl -f crash_investigator_new.unix.Makefile CPPUTILS_DEBUG=1 clean
