/******************************************************************************\

          (c) Copyright Explore Semiconductor, Inc. Limited 2005
                           ALL RIGHTS RESERVED 

--------------------------------------------------------------------------------

  File        :  DDC_If.c

  Description :  EP952 DDC Interface

\******************************************************************************/

//#include <string.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()??ï¿½ï¿½|kthread_run()
#include <linux/err.h> //IS_ERR()??ï¿½ï¿½|PTR_ERR()
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include "Edid.h"

#include "DDC_If.h"

#define HDCP_RX_ADDR            0x74     // TV HDCP RX Address
#define HDCP_RX_BKSV_ADDR       0x00     // TV HDCP RX, BKSV Register Address
#define HDCP_RX_RI_ADDR         0x08     // TV HDCP RX, RI Register Address
#define HDCP_RX_AKSV_ADDR       0x10     // TV HDCP RX, AKSV Register Address
#define HDCP_RX_AINFO_ADDR      0x15     // TV HDCP RX, AINFO Register Address
#define HDCP_RX_AN_ADDR         0x18     // TV HDCP RX, AN Register Address
#define HDCP_RX_SHA1_HASH_ADDR  0x20     // TV HDCP RX, SHA-1 Hash Value Start Address
#define HDCP_RX_BCAPS_ADDR      0x40     // TV HDCP RX, BCAPS Register Address
#define HDCP_RX_BSTATUS_ADDR    0x41     // TV HDCP RX, BSTATUS Register Address
#define HDCP_RX_KSV_FIFO_ADDR   0x43     // TV HDCP RX, KSV FIFO Start Address

//--------------------------------------------------------------------------------------------------

unsigned char DDC_Data[EDID_BLOCK_SIZE];

unsigned char TempBit;

// Private Functions
static SMBUS_STATUS DDC_Write(unsigned char IICAddr,unsigned char ByteAddr,unsigned char * Data,unsigned int Size);
static SMBUS_STATUS DDC_Read(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size);
static EDID_STATUS DDC_Read_Block(unsigned char IICAddr, unsigned char *Data, unsigned char block_index);
//==================================================================================================

//--------------------------------------------------------------------------------------------------
//
// Downstream HDCP Control
//

unsigned char Downstream_Rx_read_BKSV(unsigned char *pBKSV)
{
	int i, j;
	SMBUS_STATUS status;

	status = DDC_Read(HDCP_RX_ADDR, HDCP_RX_BKSV_ADDR, pBKSV, 5);
	if(status != SMBUS_STATUS_Success) {
		ERR("BKSV read - DN DDC %d\n", (int)status);
		return 0;
	}

	i = 0;
	j = 0;
	while (i < 5) {
		TempBit = 1;
		while (TempBit) {
			if (pBKSV[i] & TempBit) j++;
			TempBit <<= 1;
		}
		i++;
	}
	if(j != 20) {
		ERR("BKSV read - Key Wrong\n");
		ERR("BKSV=0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\n", (unsigned int)pBKSV[0], (unsigned int)pBKSV[1], (unsigned int)pBKSV[2], (unsigned int)pBKSV[3], (unsigned int)pBKSV[4]);
		return 0;
	}
	return 1;
}

unsigned char Downstream_Rx_BCAPS(void)
{
	DDC_Read(HDCP_RX_ADDR, HDCP_RX_BCAPS_ADDR, DDC_Data, 1);
	return DDC_Data[0];
}

void Downstream_Rx_write_AINFO(char ainfo) 
{
	DDC_Write(HDCP_RX_ADDR, HDCP_RX_AINFO_ADDR, &ainfo, 1);
}

void Downstream_Rx_write_AN(unsigned char *pAN)
{
	DDC_Write(HDCP_RX_ADDR, HDCP_RX_AN_ADDR, pAN, 8);
}

void Downstream_Rx_write_AKSV(unsigned char *pAKSV)
{
	DDC_Write(HDCP_RX_ADDR, HDCP_RX_AKSV_ADDR, pAKSV, 5);
}

unsigned char Downstream_Rx_read_RI(unsigned char *pRI)
{
	SMBUS_STATUS status;
	// Short Read
	status = DDC_Read(HDCP_RX_ADDR, HDCP_RX_RI_ADDR, pRI, 2);
	if(status != SMBUS_STATUS_Success) {
		ERR("Rx Ri read - MCU IIC %d\n", (int)status);
		return 0;
	}
	return 1;
}

void Downstream_Rx_read_BSTATUS(unsigned char *pBSTATUS)
{
	DDC_Read(HDCP_RX_ADDR, HDCP_RX_BSTATUS_ADDR, pBSTATUS, 2);
}

void Downstream_Rx_read_SHA1_HASH(unsigned char *pSHA)
{
	DDC_Read(HDCP_RX_ADDR, HDCP_RX_SHA1_HASH_ADDR, pSHA, 20);
}

// Retrive a 5 byte KSV at "Index" from FIFO
unsigned char Downstream_Rx_read_KSV_FIFO(unsigned char *pBKSV, unsigned char Index, unsigned char DevCount)
{
	int i, j;
	static SMBUS_STATUS status = SMBUS_STATUS_Pending;

	// Try not to re-read the previous KSV
	if(Index == 0) { // Start 
		// Support a max 25 device count because of DDC_Data[] size is 128 byte
		status = DDC_Read(HDCP_RX_ADDR, HDCP_RX_KSV_FIFO_ADDR, DDC_Data, min(DevCount, (unsigned char)25));
	}
	memcpy(pBKSV, DDC_Data+(Index*5), 5);

	if(status != SMBUS_STATUS_Success) {
		ERR("KSV FIFO read - DN DDC %d\n", (int)status);
		return 0;
	}

	i = 0;
	j = 0;
	while (i < 5) {
		TempBit = 1;
		while (TempBit) {
			if (pBKSV[i] & TempBit) j++;
			TempBit <<= 1;
		}
		i++;
	}
	if(j != 20) {
		ERR("KSV FIFO read - Wrong Key\n");
		return 0;
	}	
	return 1;
}

//--------------------------------------------------------------------------------------------------
//
// Downstream EDID Control
//

EDID_STATUS Downstream_Rx_read_EDID(unsigned char *pEDID)
{
	unsigned char ext_count;
	unsigned int block;
	int block_map_found = 0;
	unsigned char block_map[EDID_MAX_BLOCK_COUNT - 2] = {0};
	unsigned char *cur_dest_block, *cur_map_tag;
	EDID_STATUS status;

	// =========================================================
	// I. Read the block 0


	status = DDC_Read_Block(EDID_ADDR, pEDID, 0);
	if(status != EDID_STATUS_Success) {
		return status;
	}

	// =========================================================
	// II. Read other blocks and find Timing Extension Block

	ext_count = pEDID[126];
	pEDID[126] = 0;
	cur_dest_block = pEDID + EDID_BLOCK_SIZE;

	// We look for the CEA timing extension (tag 0x02)
	// If we find a block map, we parse it and use it to avoid reading
	// non-extension blocks
	for (block = 1; block < MIN(ext_count + 1, EDID_MAX_BLOCK_COUNT); block++) {
		if (block_map_found && block_map[block - 2] != EDID_EXT_CEA) {
			if (block == 2) {
				ERR("first non-map extension is not CEA extension according to block map, reading anyway");
			} else {
				continue;
			}
		}

		status = DDC_Read_Block(EDID_ADDR, DDC_Data, block);
		if(status != EDID_STATUS_Success) {
			if (ext_count > 1 && block == 1) {
				ERR("assuming block 1 was a block map, skipping\n");
				continue;
			}
			return status;
		}

		switch (DDC_Data[0]) {
		case EDID_EXT_BLOCK_MAP:
			if (block_map_found) {
				ERR("found block map instead of CEA block at block %u\n", block);
				return EDID_STATUS_InvalidFormat;
			} else if (block != 1) {
				ERR("found block map at block %u, shoud be at block 1, ignoring\n", block);
				continue;
			}

			memcpy(&block_map, &DDC_Data[1], EDID_MAX_BLOCK_COUNT - 2);
			for (cur_map_tag = block_map; cur_map_tag < block_map + EDID_MAX_BLOCK_COUNT - 2; cur_map_tag++) {
				if (*cur_map_tag == 0) break;
			}

			if (cur_map_tag - block_map + 1 != ext_count && ext_count < EDID_MAX_BLOCK_COUNT ||
				ext_count >= EDID_MAX_BLOCK_COUNT && cur_map_tag - block_map + 2 != EDID_MAX_BLOCK_COUNT)
			{
				ERR("mismatch detected between block map and extension flag, ignoring block map\n");
				continue;
			}
			block_map_found = 1;
			break;
		case EDID_EXT_CEA:
			memcpy(cur_dest_block, DDC_Data, EDID_BLOCK_SIZE);
			cur_dest_block += EDID_BLOCK_SIZE;
			pEDID[126]++;
			break;
		default:
			if (block_map_found) {
				ERR("found block type %d instead of CEA block at block %u\n", (int)DDC_Data[0], block);
				return EDID_STATUS_InvalidFormat;
			}
			continue;
		}
	}

	return EDID_STATUS_Success;
}

//==================================================================================================
//
// Private Functions
//
extern s32 ep952_i2c_write(u32 client_addr,u8 * data,int size);
extern s32 ep952_i2c_read(u32 client_addr,u8 sub_addr,u8 * data,int size);
extern s32 ep952_i2c_send(struct i2c_msg *msgs, int msg_count);

static SMBUS_STATUS DDC_Write(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size)
{
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// return 0; for success
	// return 2; for No_ACK
	// return 4; for Arbitration
	/////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned char datas[32] = {0};

	if(Size > 31) {
		ERR("DDC_Write size(%d) > 31\n", Size);
		return 4;
	}
	datas[0] = ByteAddr;
	memcpy((void *)(datas + 1), (void *)Data, Size);

	return ep952_i2c_write(IICAddr, datas, Size + 1);
}

static SMBUS_STATUS DDC_Read(unsigned char IICAddr, unsigned char ByteAddr, unsigned char *Data, unsigned int Size)
{
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// return 0; for success
	// return 2; for No_ACK
	// return 4; for Arbitration
	/////////////////////////////////////////////////////////////////////////////////////////////////

	return ep952_i2c_read(IICAddr, ByteAddr, Data, Size);
}

static EDID_STATUS DDC_Read_Block(unsigned char IICAddr, unsigned char *Data, unsigned char block_index)
{
	unsigned char seg_ptr = block_index / 2;
	unsigned char block_addr = (block_index & 1) << 7;

    struct i2c_msg msgs[] = {
        {
            .addr   = EDID_SEGMENT_PTR,
            .flags  = 0,
            .len    = 1,
            .buf    = &seg_ptr,
        },
        {
            .addr   = IICAddr,
            .flags  = 0,
            .len    = 1,
            .buf    = &block_addr,
        },
        {
            .addr   = IICAddr,
	    	.flags  = I2C_M_RD,
            .len    = EDID_BLOCK_SIZE,
            .buf    = Data,
        },
    };
    SMBUS_STATUS ret;

    if (seg_ptr == 0) {
		ret = ep952_i2c_send(&msgs[1], 2);
	} else {
		ret = ep952_i2c_send(msgs, 3);
	}

	if (ret != SMBUS_STATUS_Success) {
		ERR("failed to read EDID block %d\n", (int)block_index);
		return EDID_STATUS_ReadError;
	}

#if DEBUG
	DBG("EDID block %d:\n", (int)block_index);
	print_hex_dump_bytes("", DUMP_PREFIX_NONE, Data, EDID_BLOCK_SIZE);
	DBG("\n");
#endif

	if (EDID_ValidateBlockChecksum(Data)) {
		ERR("checksum error reading EDID block %d\n", (int)block_index);
		return EDID_STATUS_ChecksumError;
	}

	return EDID_STATUS_Success;
}
