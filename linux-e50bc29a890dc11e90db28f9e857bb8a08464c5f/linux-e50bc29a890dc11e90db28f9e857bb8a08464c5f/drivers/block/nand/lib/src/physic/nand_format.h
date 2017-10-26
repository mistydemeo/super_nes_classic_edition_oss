/*
************************************************************************************************************************
*                                                      eNand
*                                     Nand flash driver format module define
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name : nand_format.h
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.25
*
* Description : This file define the function interface and some data structure export
*               for the format module.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.25      0.1          build the file
*
************************************************************************************************************************
*/
#ifndef __NAND_FORMAT_H__
#define __NAND_FORMAT_H__

#include "nand_type.h"
#include "nand_physic.h"

extern struct __LogicArchitecture_t LogicArchiPar;

//define the page buffer for cache the super page data for read or write
#define FORMAT_PAGE_BUF     (PageCachePool.PageCache0)
#define FORMAT_SPARE_BUF    (PageCachePool.SpareCache)

//==============================================================================
//  define the logical architecture export parameter
//==============================================================================

//define the count of the sectors in the logical page
#define SECTOR_CNT_OF_LOGIC_PAGE    SECTOR_CNT_OF_SUPER_PAGE

//define the full bitmap of sector in the logical page
#define FULL_BITMAP_OF_LOGIC_PAGE   FULL_BITMAP_OF_SUPER_PAGE

//define the count of the pages in a logical block
#define PAGE_CNT_OF_LOGIC_BLK       (LogicArchiPar.PageCntPerLogicBlk) //(NandDriverInfo.LogicalArchitecture->PageCntPerLogicBlk)

//define the count of the pages in a super physical block, size is same as the logical block
#define PAGE_CNT_OF_SUPER_BLK       PAGE_CNT_OF_LOGIC_BLK

//define the total count of inter-leave banks
#define INTERLEAVE_BANK_CNT         (PAGE_CNT_OF_LOGIC_BLK / NandStorageInfo.PageCntPerPhyBlk) //(PAGE_CNT_OF_LOGIC_BLK / NandDriverInfo.NandStorageInfo->PageCntPerPhyBlk)

#define LOGIC_DIE_CNT				(LogicArchiPar.LogicDieCnt)//(NandDriverInfo.LogicalArchitecture->LogicDieCnt)

#define BLK_CNT_OF_LOGIC_DIE		(LogicArchiPar.LogicBlkCntPerLogicDie)//(BLOCK_CNT_OF_ZONE*ZONE_CNT_OF_DIE)

#define LOGIC_PAGE_SIZE				(SECTOR_CNT_OF_SUPER_PAGE*512)

/*
************************************************************************************************************************
*                                   FORMAT NAND FLASH DISK MODULE INIT
*
*Description: Init the nand disk format module, initiate some variables and request resource.
*
*Arguments  : none
*
*Return     : init result;
*               = 0     format module init successful;
*               < 0     format module init failed.
************************************************************************************************************************
*/
__s32 FMT_Init(void);


/*
************************************************************************************************************************
*                                   FORMAT NAND FLASH DISK MODULE EXIT
*
*Description: Exit the nand disk format module, release some resource.
*
*Arguments  : none
*
*Return     : exit result;
*               = 0     format module exit successful;
*               < 0     format module exit failed.
************************************************************************************************************************
*/
__s32 FMT_Exit(void);

void ClearNandStruct( void );

#endif  //ifndef __NAND_FORMAT_H__


