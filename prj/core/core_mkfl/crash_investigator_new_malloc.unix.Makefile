
mkfile_path		=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir		=  $(shell dirname $(mkfile_path))

targetName		=  libcrash_investigator_new_malloc

include $(mkfile_dir)/crash_investigator_libs_common.unix.Makefile


$(targetName): $(CR_INV_OBJECTS)
	mkdir -p $(targetDirPath)
	$(LINK) $(CR_INV_OBJECTS) $(LFLAGS) -o $(targetFilePath)
	
.PHONY: clean
clean:
	@rm -rf $(objectFilesDirPath)
	@rm -f  $(targetFilePath)
	@echo "  " cleaning of $(targetName) !!!
	
