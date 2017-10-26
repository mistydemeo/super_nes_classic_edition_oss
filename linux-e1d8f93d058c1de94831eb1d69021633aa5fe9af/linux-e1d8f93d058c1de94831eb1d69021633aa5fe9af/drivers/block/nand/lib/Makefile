
################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################
CC=~/rentiantang/tina/staging_dir/toolchain-arm_cortex-a7+neon_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/bin/arm-openwrt-linux-gcc
AR=~/rentiantang/tina/staging_dir/toolchain-arm_cortex-a7+neon_gcc-4.8-linaro_uClibc-0.9.33.2_eabi/bin/arm-openwrt-linux-ar
LIBPATH=$(shell pwd)
CFLAGS = -g -c -nostdlib -march=armv7-a -marm -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 -msoft-float
#CFLAGS = -g -c -nostdlib -march=armv7-a -marm -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian -O2 --min_array_alignment=4 --no_unaligned_access -fPIC
LIB=$(LIBPATH)/libnand.a
#OBJ=src/format/nand_format.o src/logic/bad_manage.o \
#         	src/logic/logic_ctl.o src/logic/mapping.o \
#			src/logic/mbr.o src/logic/mapping_base.o src/logic/merge.o \
#         	src/logic/read_reclaim.o src/logic/wear_levelling.o src/logic/logic_cache.o \
#         	src/scan/nand_scan.o src/scan/nand_id.o\
#         	src/physic/nand_phy.o src/physic/nand_simple_r.o src/physic/nand_simple_w.o \
#         	src/nfc/nfc_r.o src/nfc/nfc_w.o

OBJ=src/nftl/nftl_list.o  \
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
         	src/phy/build_nand_partition.o \
         	src/phy/build_phy_partition.o \
         	src/phy/hw_phy.o \
         	src/phy/nand_info_init.o \
         	src/phy/phy_input.o    	\
         	src/phy_v2/nand_info_init_v2.o \
         	src/phy_v2/build_phy_partition_v2.o \

install:lib
	cp libnand.a ../libnand_sun8iw5p1

#lib : $(LIB)
#$(OBJ) : $(LIBPATH)/../include/nand_include.h $(LIBPATH)/osal/nand_osal.h


lib : $(LIB)
$(OBJ) : $(LIBPATH)/src/nftl/nftl_type.h $(LIBPATH)/src/nftl/nftl.h $(LIBPATH)/src/nftl/nftl_inc.h $(LIBPATH)/src/nftl_interface/nftl_cfg.h $(LIBPATH)/src/nftl_interface/nftl_interface.h $(LIBPATH)/src/nftl_interface/nftl_blk.h $(LIBPATH)/src/physic/nand_drv_cfg.h $(LIBPATH)/src/physic/nand_drv_cfg.h $(LIBPATH)/src/physic/nand_drv_cfg.h $(LIBPATH)/src/physic/nand_format.h $(LIBPATH)/src/physic/nand_physic.h $(LIBPATH)/src/physic/nand_physic_interface.h $(LIBPATH)/src/physic/nand_scan.h $(LIBPATH)/src/physic/nand_simple.h $(LIBPATH)/src/physic/nand_type.h $(LIBPATH)/src/physic/nfc.h $(LIBPATH)/src/physic/nfc.h $(LIBPATH)/src/physic/nfc.h $(LIBPATH)/src/phy/mbr.h $(LIBPATH)/src/phy/phy.h

$(LIB) : $(OBJ)
	$(AR) -rcs $@ $^
clean:
	-rm $(OBJ) $(LIB)
