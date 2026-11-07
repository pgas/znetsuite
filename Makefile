#
# ZNetSuite - Suite of RomWBW / CP/M Utilities
#

ROMWBW ?= $(abspath ../RomWBW)
export ROMWBW

SUBDIRS = lib ztalk zgmi

all:
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir all ; \
	done

clean:
	@rm -rf build bin

.PHONY: all clean $(SUBDIRS)
