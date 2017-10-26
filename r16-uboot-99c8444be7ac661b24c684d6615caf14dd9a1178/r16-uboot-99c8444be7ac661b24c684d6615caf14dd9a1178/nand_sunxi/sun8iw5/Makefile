#
# (C) Copyright 2000, 2001, 2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)libsunxi-nand.o

COBJS-y	+= src/nftl/nftl_list.o  \
         	src/nftl/nftl_logic.o \
			src/nftl/nftl_gc.o \
         	src/nftl/nftl_cache.o \
         	src/nftl/nftl_sector_cache.o \
         	src/nftl/nftl_common.o \
         	src/nftl/nftl_build.o \
         	src/nftl/nftl_start.o \
         	src/nftl/nftl_interface.o \
         	src/nftl_interface/nftl_init.o \
         	src/nftl_interface/nftl_hw_interface.o \
         	src/nftl_interface/nftl_uboot_interface.o \
         	src/physic/nand_format.o \
			src/physic/nand_id.o \
         	src/physic/nand_phy.o \
         	src/physic/nand_physic_interface.o \
         	src/physic/nand_scan.o \
         	src/physic/nand_simple_r.o \
         	src/physic/nand_simple_w.o \
         	src/physic/nfc_reg.o \
         	src/physic/nfc_r.o \
         	src/physic/nfc_w.o \
         	src/phy_v2/nand_info_init_v2.o \
         	src/phy_v2/build_phy_partition_v2.o \
         	src/phy/build_nand_partition.o \
         	src/phy/build_phy_partition.o \
         	src/phy/hw_phy.o \
         	src/phy/nand_info_init.o \
         	src/phy/phy_input.o    	\

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
