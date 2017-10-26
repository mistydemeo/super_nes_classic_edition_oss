/*
************************************************************************************************************************
*                                                      eNand
*                                   Nand flash driver data struct type define
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*                                                  All Rights Reserved
*
* File Name : nand_type.h
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.19
*
* Description : This file defines the data struct type and return value type for nand flash driver.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.19      0.1          build the file
*
************************************************************************************************************************
*/
#ifndef __NAND_TYPE_H
#define __NAND_TYPE_H

#include "nand_drv_cfg.h"

//==============================================================================
//  define the data structure for physic layer module
//==============================================================================

//define the optional physical operation parameter
struct __OptionalPhyOpPar_t
{
    __u8        MultiPlaneReadCmd[2];               //the command for multi-plane read, the sequence is [0] -ADDR- [0] -ADDR- [1] - DATA
    __u8        MultiPlaneWriteCmd[2];              //the command for multi-plane program, the sequence is 80 -ADDR- DATA - [0] - [1] -ADDR- DATA - 10/15
    __u8        MultiPlaneCopyReadCmd[3];           //the command for multi-plane page copy-back read, the sequence is [0] -ADDR- [1] -ADDR- [2]
    __u8        MultiPlaneCopyWriteCmd[3];          //the command for multi-plane page copy-back program, the sequence is [0] -ADDR- [1] - [2] -ADDR- 10
    __u8        MultiPlaneStatusCmd;                //the command for multi-plane operation status read, the command may be 0x70/0x71/0x78/...
    __u8        InterBnk0StatusCmd;                 //the command for inter-leave bank0 operation status read, the command may be 0xf1/0x78/...
    __u8        InterBnk1StatusCmd;                 //the command for inter-leave bank1 operation status read, the command may be 0xf2/0x78/...
    __u8        BadBlockFlagPosition;               //the flag that marks the position of the bad block flag,0x00-1stpage/ 0x01-1st&2nd page/ 0x02-last page/ 0x03-last 2 page
    __u16       MultiPlaneBlockOffset;              //the value of the block number offset between the left-plane block and the right pane block
};

#if 0
typedef struct
{
    __u32       ChannelCnt;
    __u32        ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
    __u32       ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
    __u32       RbCnt;
    __u32       RbConnectInfo;                      //the connect  information of the all rb  chips are connected
    __u32        RbConnectMode;                     //the rb connect  mode
    __u32        BankCntPerChip;                     //the count of the banks in one nand chip, multiple banks can support Inter-Leave
    __u32        DieCntPerChip;                      //the count of the dies in one nand chip, block management is based on Die
    __u32        PlaneCntPerDie;                     //the count of planes in one die, multiple planes can support multi-plane operation
    __u32        SectorCntPerPage;                   //the count of sectors in one single physic page, one sector is 0.5k
    __u32       PageCntPerPhyBlk;                   //the count of physic pages in one physic block
    __u32       BlkCntPerDie;                       //the count of the physic blocks in one die, include valid block and invalid block
    __u32       OperationOpt;                       //the mask of the operation types which current nand flash can support support
    __u32        FrequencePar;                       //the parameter of the hardware access clock, based on 'MHz'
    __u32        EccMode;                            //the Ecc Mode for the nand flash chip, 0: bch-16, 1:bch-28, 2:bch_32
    __u8        NandChipId[8];                      //the nand chip id of current connecting nand chip
    __u32       ValidBlkRatio;                      //the ratio of the valid physical blocks, based on 1024
    __u32       good_block_ratio;                   //good block ratio get from hwscan
    __u32       ReadRetryType;                      //the read retry type
    __u32       DDRType;
    __u32       Reserved[22];
}boot_nand_para_t;

typedef struct boot_flash_info{
    __u32 chip_cnt;
    __u32 blk_cnt_per_chip;
    __u32 blocksize;
    __u32 pagesize;
    __u32 pagewithbadflag; /*bad block flag was written at the first byte of spare area of this page*/
}boot_flash_info_t;
#endif

//define the nand flash storage system information
struct __NandStorageInfo_t
{
    __u32       ChannelCnt;
    __u32        ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
    __u32       ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
    __u32       RbCnt;
    __u32       RbConnectInfo;                      //the connect  information of the all rb  chips are connected
    __u32        RbConnectMode;                     //the rb connect  mode
    __u32       BankCntPerChip;                     //the count of the banks in one nand chip, multiple banks can support Inter-Leave
    __u32        DieCntPerChip;                      //the count of the dies in one nand chip, block management is based on Die
    __u32        PlaneCntPerDie;                     //the count of planes in one die, multiple planes can support multi-plane operation
    __u32        SectorCntPerPage;                   //the count of sectors in one single physic page, one sector is 0.5k
    __u32       PageCntPerPhyBlk;                   //the count of physic pages in one physic block
    __u32       BlkCntPerDie;                       //the count of the physic blocks in one die, include valid block and invalid block
    __u32       OperationOpt;                       //the mask of the operation types which current nand flash can support support
    __u32        FrequencePar;                       //the parameter of the hardware access clock, based on 'MHz'
    __u32       EccMode;                            //the Ecc Mode for the nand flash chip, 0: bch-16, 1:bch-28, 2:bch_32
    __u8       NandChipId[8];                      //the nand chip id of current connecting nand chip
    __u32       ValidBlkRatio;                         //the ratio of the valid physical blocks, based on 1024
    __u32       ReadRetryType;                      //the read retry type
    __u32       DDRType;
    struct __OptionalPhyOpPar_t OptPhyOpPar;        //the parameters for some optional operation
};

//define the page buffer pool for nand flash driver
struct __NandPageCachePool_t
{
    __u8        *PageCache0;                        //the pointer to the first page size ram buffer
    __u8        *PageCache1;                        //the pointer to the second page size ram buffer
    __u8        *PageCache2;                        //the pointer to the third page size ram buffer
    __u8        *PageCache3;                        //the pointer to the third page size ram buffer
    __u8        *PageCache4;                        //the pointer to the third page size ram buffer
    __u8        *SpareCache;

    __u8        *TmpPageCache;
    __u8        *TmpPageCache1;
};

//define the paramter structure for physic operation function
struct __PhysicOpPara_t
{
    __u32        BankNum;                            //the number of the bank current accessed, bank NO. is different of chip NO.
    __u32       PageNum;                            //the number of the page current accessed, the page is based on single-plane or multi-plane
    __u32       BlkNum;                             //the number of the physic block, the block is based on single-plane or multi-plane
    __u64       SectBitmap;                         //the bitmap of the sector in the page which need access data
    void        *MDataPtr;                          //the pointer to main data buffer, it is the start address of a page size based buffer
    void        *SDataPtr;                          //the pointer to spare data buffer, it will be set to NULL if needn't access spare data
};

struct __NfcInitDdrInfo {
    __u32 en_dqs_c;
    __u32 en_re_c;
    __u32 odt;
    __u32 en_ext_verf;
    __u32 dout_re_warmup_cycle;
    __u32 din_dqs_warmup_cycle;
    __u32 output_driver_strength;
    __u32 rb_pull_down_strength;
} ;

//define the nand flash physical information parameter type, for id table
struct __NandPhyInfoPar_t
{
    __u8        NandID[8];                          //the ID number of the nand flash chip
    __u8        DieCntPerChip;                      //the count of the Die in one nand flash chip
    __u8        SectCntPerPage;                     //the count of the sectors in one single physical page
    __u16       PageCntPerBlk;                      //the count of the pages in one single physical block
    __u16       BlkCntPerDie;                       //the count fo the physical blocks in one nand flash Die
    __u32       OperationOpt;                       //the bitmap that marks which optional operation that the nand flash can support
    __u16       ValidBlkRatio;                      //the valid block ratio, based on 1024 blocks
    __u16       AccessFreq;                         //the highest access frequence of the nand flash chip, based on MHz
    __u16       EccMode;                            //the Ecc Mode for the nand flash chip, 0: bch-16, 1:bch-28, 2:bch_32
    __u32       ReadRetryType;
    __u32       DDRType;
    struct __OptionalPhyOpPar_t *OptionOp;          //the pointer point to the optional operation parameter
    struct __NfcInitDdrInfo *InitDdrInfo;
    __u32       Idnumber;
    __u32       MaxBlkEraseTimes;   
};

//==============================================================================
//  define the data structure for logic management module
//==============================================================================

//define the logical architecture parameter structure
struct __LogicArchitecture_t
{
#if 0
    //__u16       LogicBlkCntPerZone;                 //the counter that marks how many logic blocks in one zone
    __u16       PageCntPerLogicBlk;                 //the counter that marks how many pages in one logical block
    __u8        SectCntPerLogicPage;                //the counter that marks how many  sectors in one logical page
    //__u8        ZoneCntPerDie;                      //the counter that marks how many zones in one die
    //__u16       Reserved;                           //reserved for 32bit align
    __u8       LogicDieCnt;                           //reserved for 32bit align
#else
    __u32       PageCntPerLogicBlk;         //the number of sectors in one logical page
    __u32       SectCntPerLogicPage;        //the number of logical page in one logical block
    __u32       LogicBlkCntPerLogicDie;     //the number of logical block in a logical die
    __u32       LogicDieCnt;                //the number of logical die
    //__u32         ZoneCntPerDie;
#endif
};

//==============================================================================
//  define some constant variable for the nand flash driver used
//==============================================================================

//define the mask for the nand flash optional operation
#define NAND_CACHE_READ          (1<<0)              //nand flash support cache read operation
#define NAND_CACHE_PROGRAM       (1<<1)              //nand flash support page cache program operation
#define NAND_MULTI_READ          (1<<2)              //nand flash support multi-plane page read operation
#define NAND_MULTI_PROGRAM       (1<<3)              //nand flash support multi-plane page program operation
#define NAND_PAGE_COPYBACK       (1<<4)              //nand flash support page copy-back command mode operation
#define NAND_INT_INTERLEAVE      (1<<5)              //nand flash support internal inter-leave operation, it based multi-bank
#define NAND_EXT_INTERLEAVE      (1<<6)              //nand flash support external inter-leave operation, it based multi-chip
#define NAND_RANDOM              (1<<7)              //nand flash support RANDOMIZER
#define NAND_READ_RETRY          (1<<8)              //nand falsh support READ RETRY
#define NAND_READ_UNIQUE_ID      (1<<9)              //nand falsh support READ UNIQUE_ID
#define NAND_PAGE_ADR_NO_SKIP    (1<<10)                //nand falsh page adr no skip is requiered
#define NAND_DIE_SKIP            (1<<11)             //nand flash die adr skip
//#define NAND_LOG_BLOCK_MANAGE   (1<<12)             //nand flash log block type management
#define NAND_FORCE_WRITE_SYNC    (1<<13)             //nand flash need check status after write or erase
//#define NAND_LOG_BLOCK_LSB_TYPE (0xff<<16)          //nand flash log block lsb page type
#define NAND_LSB_PAGE_TYPE       (0xf<<16)          //nand flash lsb page type....

#define NAND_MAX_BLK_ERASE_CNT   (1<<20)            //nand flash support the maximum block erase cnt
#define NAND_READ_RECLAIM        (1<<21)            //nand flash support to read reclaim Operation

#define NAND_TIMING_MODE         (1<<24)            //nand flash support to change timing mode according to ONFI 3.0
#define NAND_DDR2_CFG            (1<<25)            //nand flash support to set ddr2 specific feature according to ONFI 3.0 or Toggle DDR2
#define NAND_IO_DRIVER_STRENGTH  (1<<26)            //nand flash support to set io driver strength according to ONFI 2.x/3.0 or Toggle DDR1/DDR2
#define NAND_VENDOR_SPECIFIC_CFG (1<<27)            //nand flash support to set vendor's specific configuration
#define NAND_ONFI_SYNC_RESET_OP  (1<<28)            //nand flash support onfi's sync reset
#define NAND_TOGGLE_ONLY         (1<<29)            //nand flash support toggle interface only, and do not support switch between legacy and toggle
#define NAND_WITH_TWO_ROW_ADR    (1<<30)             //nand flash has two row address only


#if 0
//define the mask for the nand flash part_type
#define NAND_SECURITY_PARTITION  (1<<0)
#define NAND_FORCE_FLUSH_CACHE   (1<<1)


//define part type of nand
#define NORMAL_TYPE    0
#define LSB_TYPE       1
#endif

//define the mask for the nand flash operation status
#define NAND_OPERATE_FAIL       (1<<0)              //nand flash program/erase failed mask
#define NAND_CACHE_READY        (1<<5)              //nand flash cache program true ready mask
#define NAND_STATUS_READY       (1<<6)              //nand flash ready/busy status mask
#define NAND_WRITE_PROTECT      (1<<7)              //nand flash write protected mask

#if 0
//define the mark for physical page status
#define FREE_PAGE_MARK          0xff                //the page is storing no data, is not used
#define DATA_PAGE_MARK          0x55                //the physical page is used for storing the update data
#define TABLE_PAGE_MARK         0xaa                //the physical page is used for storing page mapping table

#define TABLE_BLK_MARK          0xaa                //the mark for the block mapping table block which is a special type block
#define BOOT_BLK_MARK           0xbb                //the mark for the boot block which is a special type block

//define the count of the physical blocks managed by one zone
#define BLOCK_CNT_OF_ZONE       1024                //one zone is organized based on 1024 blocks
#endif

//define the size of the sector
#define SECTOR_SIZE             512                 //the size of a sector, based on byte


//==============================================================================
//  define the function return value for different modules
//==============================================================================

#define NAND_OP_TRUE            (0)                     //define the successful return value
#define NAND_OP_FALSE           (-1)                    //define the failed return value


//define the return value
#define ECC_LIMIT               10                  //reach the limit of the ability of ECC
#define ERR_MALLOC              11                  //request buffer failed
#define ERR_ECC                 12                  //too much ecc error
#define ERR_NANDFAIL            13                  //nand flash program or erase fail
#define ERR_TIMEOUT             14                  //hardware timeout
#define ERR_PHYSIC              15                  //physical operation module error
#define ERR_SCAN                16                  //scan module error
#define ERR_FORMAT              17                  //format module error
//#define ERR_MAPPING             18                  //mapping module error
//#define ERR_LOGICCTL            19                  //logic control module error
#define ERR_ADDRBEYOND          20                  //the logical sectors need be accessed is beyond the logical disk
#define ERR_INVALIDPHYADDR        21

#endif //ifndef __NAND_TYPE_H

