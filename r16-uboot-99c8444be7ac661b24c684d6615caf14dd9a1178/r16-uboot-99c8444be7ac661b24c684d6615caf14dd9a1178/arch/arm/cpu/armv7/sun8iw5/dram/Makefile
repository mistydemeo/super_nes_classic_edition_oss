
##
## Makefile for Sunxi Secure Boot
##



include $(TOPDIR)/config.mk

all:
ifeq ($(notdir $(shell find ./ -name lib-dram)), lib-dram)

	make -C lib-dram
else
	@echo "libdram exist"

endif

ifeq ($(CONFIG_ARCH_R16),y)
	cp ./libdram-riot ./libdram.o
else
	cp ./libdram-pad ./libdram.o
endif
#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################

