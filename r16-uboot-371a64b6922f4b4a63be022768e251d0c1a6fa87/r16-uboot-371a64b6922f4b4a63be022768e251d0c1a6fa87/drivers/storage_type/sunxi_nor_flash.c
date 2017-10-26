/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <mmc.h>
#include <sunxi_nand.h>
#include <boot_type.h>
#include "sunxi_flash.h"
#include "sys_config.h"
#include "sys_partition.h"
#ifdef CONFIG_SUNXI_SPINOR
#include <asm/arch/spinor.h>
#endif

int sunxi_flash_init_uboot(int verbose);

static int
sunxi_null_op(unsigned int start_block, unsigned int nblock, void *buffer){
	return 0;
}

//static int
//sunxi_null_erase(int erase, void *mbr_buffer)
//{
//	return 0;
//}

static uint
sunxi_null_size(void){
	return 0;
}

static int
sunxi_null_init(int flag){
	return -1;
}

static int
sunxi_null_exit(int force){
	return -1;
}

static int
sunxi_null_flush(void){
	return 0;
}

//static int
//sunxi_null_datafinish(void){
//	return 0;
//}

/*
************************************************************************************************************
*
*                                             function
*
*
*
*
*
*
*
*
************************************************************************************************************
*/

int (* sunxi_flash_init_pt)(int stage) = sunxi_null_init;
int (* sunxi_flash_read_pt) (uint start_block, uint nblock, void *buffer) = sunxi_null_op;
//int (* sunxi_flash_read_sequence) (uint start_block, uint nblock, void *buffer) = sunxi_null_op;
int (* sunxi_flash_write_pt)(uint start_block, uint nblock, void *buffer) = sunxi_null_op;
uint (* sunxi_flash_size_pt)(void) = sunxi_null_size;
int (* sunxi_flash_exit_pt) (int force) = sunxi_null_exit;
int (* sunxi_flash_flush_pt) (void) = sunxi_null_flush;
int (* sunxi_flash_phyread_pt) (unsigned int start_block, unsigned int nblock, void *buffer) = sunxi_null_op;
int (* sunxi_sprite_read_pt) (uint start_block, uint nblock, void *buffer) = sunxi_null_op;

/*
************************************************************************************************************
*
*                                             function
*
*
*
*
*
*
*
*
************************************************************************************************************
*/
int sunxi_flash_phyread (uint start_block, uint nblock, void *buffer)
{
	return sunxi_flash_phyread_pt(start_block, nblock, buffer);
}


static int
sunxi_flash_spinor_read(unsigned int start_block, unsigned int nblock, void *buffer)
{
	debug("spinor read: start 0x%x, sector 0x%x\n", start_block, nblock);

    return spinor_read(start_block + CONFIG_SPINOR_LOGICAL_OFFSET, nblock, buffer);
}

static int
sunxi_flash_spinor_write(unsigned int start_block, unsigned int nblock, void *buffer)
{
	debug("spinor write: start 0x%x, sector 0x%x\n", start_block, nblock);

	return spinor_write(start_block + CONFIG_SPINOR_LOGICAL_OFFSET, nblock, buffer);
}


static uint
sunxi_flash_spinor_size(void){

	return spinor_size();
}

//static int sunxi_flash_spinor_erase(int erase,void *img_buf)
//{
//	return spinor_erase_all_blocks(erase);
//}

//static int
//sunxi_flash_spinor_init(int stage)
//{
//	return spinor_init(stage);
//}

static int
sunxi_flash_spinor_exit(int force)
{
	return spinor_exit(force);
}

//static int
//sunxi_flash_spinor_datafinish(void)
//{
//	return spinor_datafinish();
//}

static int
sunxi_flash_spinor_flush(void)
{
	return spinor_flush_cache();
}

/*
************************************************************************************************************
*
*											  function
*
*
*
*
*
*
*
*
************************************************************************************************************
*/

int sunxi_flash_read (uint start_block, uint nblock, void *buffer)
{
	debug("sunxi flash read : start %d, sector %d\n", start_block, nblock);
	return sunxi_flash_read_pt(start_block, nblock, buffer);
}

int sunxi_flash_write(uint start_block, uint nblock, void *buffer)
{
	debug("sunxi flash write : start %d, sector %d\n", start_block, nblock);
	return sunxi_flash_write_pt(start_block, nblock, buffer);
}

uint sunxi_flash_size(void)
{
	return sunxi_flash_size_pt();
}

int sunxi_flash_exit(int force)
{
    return sunxi_flash_exit_pt(force);
}

int
sunxi_flash_flush(void)
{
	return sunxi_flash_flush_pt();
}

int sunxi_sprite_read (uint start_block, uint nblock, void *buffer)
{
	return sunxi_sprite_read_pt(start_block, nblock, buffer);
}

int sunxi_sprite_exit(int force)
{
    return 0;
}
/*
************************************************************************************************************
*
*											  function
*
*
*
*
*
*
*
*
************************************************************************************************************
*/
int sunxi_flash_handle_init(void)
{
    int workmode;
    int nand_used = 0;
    int mmc2_used = 0;
    int spi_used = 1;

    workmode = uboot_spare_head.boot_data.work_mode;
#if 1	
    printf("workmode = %d\n", workmode);
    debug("storage type = %d\n", uboot_spare_head.boot_data.storage_type);
#endif

	if(workmode == WORK_MODE_BOOT)
	{
		script_parser_patch("mmc2_para",  "sdc_used", &mmc2_used, 1);
		script_parser_patch("nand0_para",  "nand0_used", &nand_used, 1);
		script_parser_patch("spi0",  "spi_used", &spi_used, 1);
		//sunxi_flash_init_pt  = sunxi_flash_spinor_init;
        sunxi_flash_exit_pt  = sunxi_flash_spinor_exit;
		sunxi_flash_read_pt  = sunxi_flash_spinor_read;
		sunxi_flash_write_pt = sunxi_flash_spinor_write;
		sunxi_flash_size_pt  = sunxi_flash_spinor_size;
		sunxi_flash_flush_pt = sunxi_flash_spinor_flush;

		spinor_init(0);
        sunxi_flash_init_uboot(0);
		tick_printf("sunxi flash init ok\n");
	}
	else if(workmode & WORK_MODE_UPDATE)		/* 升级模式 */
	{
	}
	else   /* undefined mode */
	{
	}

	return 0;
}

static block_dev_desc_t 	sunxi_flash_blk_dev;

block_dev_desc_t *sunxi_flash_get_dev(int dev)
{
	sunxi_flash_blk_dev.dev = dev;
	sunxi_flash_blk_dev.lba = sunxi_partition_get_size(dev);

	return ((block_dev_desc_t *) & sunxi_flash_blk_dev);
}

unsigned long  sunxi_flash_part_read(int dev_num, unsigned long start, lbaint_t blkcnt, void *dst)
{
	uint offset;

	offset = sunxi_partition_get_offset(dev_num);
	if(!offset)
	{
		printf("sunxi flash error: cant get part %d offset\n", dev_num);

		return 0;
	}
	start += offset;
#ifdef DEBUG
    printf("nor try to read from %x, length %x block\n", (int )start, (int )blkcnt);
#endif
#ifdef DEBUG
    int ret;

    ret = sunxi_flash_read((uint)start, (uint )blkcnt, dst);
    printf("read result = %d\n", ret);

	return ret;
#else
    return sunxi_flash_read((uint)start, (uint )blkcnt, dst);
#endif
}

unsigned long  sunxi_flash_part_write(int dev_num, unsigned long start, lbaint_t blkcnt, const void *dst)
{
	uint offset;

	offset = sunxi_partition_get_offset(dev_num);
	if(!offset)
	{
		printf("sunxi flash error: cant get part %d offset\n", dev_num);

		return 0;
	}
	start += offset;
#ifdef DEBUG
    printf("nor try to write from %x, length %x block\n", (int )start, (int )blkcnt);
#endif
#ifdef DEBUG
	int ret;

	ret = sunxi_flash_write((uint)start, (uint )blkcnt, (void *)dst);
	printf("read result = %d\n", ret);

	return ret;
#else
	return sunxi_flash_write((uint)start, (uint )blkcnt, (void *)dst);
#endif
}


int sunxi_flash_init_uboot(int verbose)
{
#ifdef DEBUG
    puts("sunxi flash init uboot\n");
#endif
	sunxi_flash_blk_dev.if_type = IF_TYPE_SUNXI_FLASH;
	sunxi_flash_blk_dev.part_type = PART_TYPE_DOS;
	sunxi_flash_blk_dev.dev = 0;
	sunxi_flash_blk_dev.lun = 0;
	sunxi_flash_blk_dev.type = 0;

	/* FIXME fill in the correct size (is set to 32MByte) */
	sunxi_flash_blk_dev.blksz = 512;
	sunxi_flash_blk_dev.lba = 0;
	sunxi_flash_blk_dev.removable = 0;
	sunxi_flash_blk_dev.block_read = sunxi_flash_part_read;
	sunxi_flash_blk_dev.block_write = sunxi_flash_part_write;

	return 0;
}
