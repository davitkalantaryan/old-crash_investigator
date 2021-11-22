
mkfile_path		=  $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir		=  $(shell dirname $(mkfile_path))

targetName		=  libcrash_investiator_new

include $(mkfile_dir)/crash_investigator_libs_common.unix.Makefile

COMMON_FLAGS += -DCRASH_INVEST_DO_NOT_USE_NEW_DEL

$(targetName): $(CR_INV_OBJECTS)
	mkdir -p $(targetDirPath)
	$(LINK) $(CR_INV_OBJECTS) $(LFLAGS) -o $(targetFilePath)
	
.PHONY: clean
clean:
	@rm -rf $(objectFilesDirPath)
	@rm -f  $(targetFilePath)
	@echo "  " cleaning of $(targetName) !!!
