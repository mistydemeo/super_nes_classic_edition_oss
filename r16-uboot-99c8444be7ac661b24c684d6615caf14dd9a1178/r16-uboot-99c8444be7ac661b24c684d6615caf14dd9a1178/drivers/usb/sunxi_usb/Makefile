#
# Makefile for SUNXI USB2.0 Dual Role Controller Driver.
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libusb_sunxi_usb.o

ifdef CONFIG_USB_SUNXI_UDC0
COBJS-y = udc/sunxi_udc.o \
                udc/sunxi_udc_board.o \
                udc/sunxi_udc_dma.o
endif

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
