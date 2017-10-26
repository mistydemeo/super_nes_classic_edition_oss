/*
************************************************************************************************************************
*                                                      eNand
*                                           Nand flash driver format module
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*                                                  All Rights Reserved
*
* File Name : nand_format.c
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.28
*
* Description : This file create the logical-physical mapping information on the nand flash.
*               If the mapping information is exsist already, check and repair it;
*               If there is none mapping information, create it.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.28      0.1          build the file
*
************************************************************************************************************************
*/
#include "nand_drv_cfg.h"
#include "nand_type.h"
#include "nand_physic.h"
#include "nand_format.h"
//#include "nand_logic.h"
#include "nfc.h"
#include "nand_physic_interface.h"
#include "../phy/phy.h"

#define DBG_DUMP_DIE_INFO       (1)
#define LOG_BLOCK_ECC_CHECK
//#define NORMAL_LOG_BLOCK_ECC_CHECK
#define LOG_AGE_SEQ_CHECK

extern struct _nand_info aw_nand_info;

struct __LogicArchitecture_t            LogicArchiPar = {0};

static __u32 CurNandOpSta = 0;

//define some local variable
__u32 DieCntOfNand = 0;               //the count of dies in a nand chip
__u32 SuperBlkCntOfDie = 0;    //the count of the super blocks in a Die


__s32 _CalculatePhyOpPar(struct __PhysicOpPara_t *pPhyPar, __u32 nDie, __u32 nBlock, __u32 nPage)
{
	__u32   tmpDieNum, tmpBnkNum, tmpBlkNum, tmpPageNum;

//    FORMAT_DBG("[FORMAT_DBG] Calculate the physical operation parameters.\n"
//               "             ZoneNum:0x%x, BlockNum:0x%x, PageNum: 0x%x\n", nZone, nBlock, nPage);

    //calcualte the Die number by the zone number
    //tmpDieNum = nZone / ZONE_CNT_OF_DIE;
    tmpDieNum = nDie;


    if(SUPPORT_INT_INTERLEAVE && SUPPORT_EXT_INTERLEAVE)
    {
        //nand flash support internal inter-leave and external iner-leave, the block number is
        //same as the virtual block number in the die, the bank number is the virtual page number
        //model the inter-leave bank count, and the page number is the virtual page number
        //divide the inter-leave bank count
        tmpBnkNum = (nPage % INTERLEAVE_BANK_CNT) + (tmpDieNum * INTERLEAVE_BANK_CNT);
        tmpBlkNum = nBlock ;
        tmpPageNum = nPage / INTERLEAVE_BANK_CNT;
    }

    else if(SUPPORT_INT_INTERLEAVE && !SUPPORT_EXT_INTERLEAVE)
    {
        //nand flash support internal inter-leave but don't support external inter-leave, the block
        //number if same as the vitual block number, the bank number is virtual page number model
        //inter-leave count and add the chip bank base, the page number is the virtual page number
        //divide the inter-leave bank count
        tmpBnkNum = (nPage % INTERLEAVE_BANK_CNT) + (tmpDieNum * INTERLEAVE_BANK_CNT);
        tmpBlkNum = nBlock;
        tmpPageNum = nPage / INTERLEAVE_BANK_CNT;
    }

    else if(!SUPPORT_INT_INTERLEAVE && SUPPORT_EXT_INTERLEAVE)
    {
        //nand flash support external inter-leave but don't support internal inter-leave, the block
        //number is virtual block number add the die block base, the bank number is the page number
        //model the inter-leave bank count, the page number is vitual page number divide the inter-leave
        //bank count
        //tmpBnkNum =  (nPage % INTERLEAVE_BANK_CNT) + (tmpDieNum * INTERLEAVE_BANK_CNT);
        tmpBnkNum =  (nPage % INTERLEAVE_BANK_CNT) + (tmpDieNum/DIE_CNT_OF_CHIP )*INTERLEAVE_BANK_CNT;
        tmpBlkNum = nBlock + ((tmpDieNum % DIE_CNT_OF_CHIP) * (BLOCK_CNT_OF_DIE / PLANE_CNT_OF_DIE));
        tmpPageNum = nPage / INTERLEAVE_BANK_CNT;
    }

    else//if(!SUPPORT_INT_INTERLEAVE && !SUPPORT_EXT_INTERLEAVE)
    {
        //nand flash don't internal inter-leave and extern inter-leave either, the bank number is the
        //die number divide the die count of chip, the block number is the virtual block number add
        //the die block base in the chip, the page number is same as the virtual page number
        tmpBnkNum = tmpDieNum / DIE_CNT_OF_CHIP;
        tmpBlkNum = nBlock + (tmpDieNum % DIE_CNT_OF_CHIP) * (BLOCK_CNT_OF_DIE / PLANE_CNT_OF_DIE);
        tmpPageNum = nPage;
    }

    //set the physical operation paramter by the bank number, block number and page number
    pPhyPar->BankNum = tmpBnkNum;
    pPhyPar->PageNum = tmpPageNum;
    pPhyPar->BlkNum = tmpBlkNum;

//    FORMAT_DBG("         Calculate Result: BankNum 0x%x, BlkNum 0x%x, PageNum 0x%x\n", tmpBnkNum, tmpBlkNum, tmpPageNum);

    //calculate physical operation parameter successful
    return 0;
}

void ClearNandStruct( void )
{
    MEMSET(&PageCachePool, 0x00, sizeof(struct __NandPageCachePool_t));
}

__s32 _ReadCheckSpare(__u8 *spare, __u32 die_no, __u32 blk_no, __u32 page_no)
{
    __s32 ret = 0;
    __s32 i;

    if ( SUPPORT_READ_CHECK_SPARE == 1)
    {
		if(spare == NULL)
		{
			return 0;
		}

		if((spare[0] == 0xff ) && (spare[1] == 0xff ) && (spare[2] == 0xff ) && (spare[3] == 0xff )&& (spare[4] == 0xff))
		{
			return 0;
		}

		if ((spare[0] == 0xff ) && (spare[1] == 0xaa ) && (spare[2] == 0xbb ) && (spare[3] == 0xff )) 
		{
			return 0; //read bad block table;
		}

		if (aw_nand_info.partition_nums == 0)
		{
			PHY_DBG("%s: build phy partition(%d,%d,%d)...\n", __func__,die_no, blk_no,page_no);
			return 0;
		}
		
		if ((spare[0] == 0xff ) && (spare[1] == 0x55 ) && (spare[2] == 0x55 ) && (spare[3] == 0x55 )&& (spare[4] == 0x55))
		{
			PHY_DBG("%s: filled invalid page(%d,%d,%d)...\n", __func__,die_no, blk_no,page_no);
			return 0;
		}

		if(page_no == (PAGE_CNT_OF_LOGIC_BLK-1))
		{
			if((spare[1] != 0xaa ) || (spare[2] != 0xaa ) || (spare[3] != 0xff ) || (spare[4] != 0xff ))
			{
				PHY_ERR("%s: read last page spare error %d,%d,%d !\n", __func__, die_no, blk_no,page_no);
				for (i=0; i<16; i++)
				{
					PHY_ERR("%x ", spare[i]);
				}
				PHY_ERR("\n");
				ret = 1;
			}
		}
		else
		{
			if((spare[1] & 0xf0) != 0xc0 )
			{
				PHY_ERR("%s: read page spare error %d,%d,%d !\n",__func__, die_no, blk_no,page_no);
				for (i=0; i<16; i++)
				{
					PHY_ERR("%x ", spare[i]);
				}
				PHY_ERR("\n");
				ret = 1;
			}
		}

		return ret;
    }
    else
    {
    	return 0;
    }
}

__s32 _WaitAllRbReady(void)
{
	__s32 timeout;
	//__s32 bank, chip, rb;
	//__u32 status;
	
	for (NandIndex=0; NandIndex<CHANNEL_CNT; NandIndex++)
	{
		/*wait rb ready*/
		timeout = 0xffff;
		while((timeout--) && (NFC_CheckRbReady(0)));
		if (timeout < 0)
		{
			PHY_ERR("%s: wait rb 0 time out, ch: 0x%x\n", __func__, NandIndex);
			return -ERR_TIMEOUT;
		}

		timeout = 0xffff;
		while((timeout--) && (NFC_CheckRbReady(1)));
		if (timeout < 0)
		{			
			PHY_ERR("%s: wait rb 1 time out, ch: 0x%x\n", __func__, NandIndex);
			return -ERR_TIMEOUT;
		}

#if 0
		for (bank=0; bank<NandStorageInfo.ChipCnt; bank++)
	    {
	    	/*get chip no*/
			chip = _cal_real_chip(bank); //_cal_real_chip(tmpPhyBlk.BankNum);
			if (0xff == chip) {
				PHY_ERR("%s: beyond chip count, %d.\n", __func__, chip);
				return -ERR_INVALIDPHYADDR;
			}

			rb = _cal_real_rb(chip);
			NFC_SelectChip(chip);
			NFC_SelectRb(rb);

			//PHY_DBG("%s: ch: %d  chip: %d  rb: %d\n", __func__, NandIndex, chip, rb);

			timeout = 0xffff;
			/*get status*/
			while(1) {
				status = _read_status(0x70, chip);
				if (status < 0) { /*get status failed*/
					PHY_ERR("%s: get status faield\n", __func__);
					NFC_DeSelectChip(chip);
					NFC_DeSelectRb(rb);
					return status;
				}

				if (status & NAND_STATUS_READY)
					break;

				if (timeout-- == 0)
				{
					PHY_ERR("%s: wait nand status ready timeout, ch: %d  chip: %d, rb: %d\n", __func__, NandIndex, chip, rb);
					NFC_DeSelectChip(chip);
					NFC_DeSelectRb(rb);
					return -ERR_TIMEOUT;
				}
			}

			if (status & NAND_OPERATE_FAIL)
			{				
//				if (op == 1)
//				{
//					/*write*/
//					PHY_ERR("%s: last write operation failed(0x%x) in ch: %d  chip: %d  rb: %d\n", __func__, status, NandIndex, chip, rb);
//					NFC_DeSelectChip(chip);
//					NFC_DeSelectRb(rb);
//					return -1;
//				}
//				else if (op == 2)
//				{
//					/*erase*/
//					PHY_ERR("%s: last erase operation failed(0x%x) in ch: %d  chip: %d  rb: %d\n", __func__, status, NandIndex, chip, rb);
//					NFC_DeSelectChip(chip);
//					NFC_DeSelectRb(rb);
//					return -1;
//				}
				
				PHY_ERR("%s: last operation failed(0x%x) in ch: %d  chip: %d  rb: %d\n", __func__, status, NandIndex, chip, rb);
				NFC_DeSelectChip(chip);
				NFC_DeSelectRb(rb);
				return -1;
			}


			NFC_DeSelectChip(chip);
			NFC_DeSelectRb(rb);
	    }
#endif 
		if(NandIndex == (CHANNEL_CNT-1))
			break;

	}
	NandIndex = 0;
	
	return 0;
}

/*
 * op: 	0: read
 *		1: write
 *		2: erase
 *
 */
__s32 _ChangeNandOpSta(__u32 op)
{
	__s32 ret;

#if 1
	if ((op > 2) && (op != 0xff))
	{
		PHY_ERR("%s: wrong parameter, op: %d\n", __func__, op);
		return -1;
	}

	if (op != CurNandOpSta)
	{
		//PHY_DBG("%s: change nand to status %d...\n", __func__, op);
		ret = _WaitAllRbReady();
		if (ret != 0) 
		{
			PHY_ERR("%s: change nand to status %d failed\n", __func__, op);
			return ret;
		}

		CurNandOpSta = op;
	}
	else
	{
		//PHY_DBG("%s: nand is already in %d status!\n", __func__, op);
	}

#endif

	return 0;
}

/*
************************************************************************************************************************
*                       READ PAGE DATA FROM VIRTUAL BLOCK
*
*Description: Read page data from virtual block, the block is composed by several physical block.
*             It is named super block too.
*
*Arguments  : nDieNum   the number of the DIE, which the page is belonged to;
*             nBlkNum   the number of the virtual block in the die;
*             nPage     the number of the page in the virtual block;
*             Bitmap    the bitmap of the sectors need access in the page;
*             pBuf      the pointer to the page data buffer;
*             pSpare    the pointer to the spare data buffer.
*
*Return     : read result;
*               = 0     read page data successful;
*               < 0     read page data failed.
************************************************************************************************************************
*/
__s32 _VirtualPageRead(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare)
{
    __s32 result;
    __u64 spare_bitmap = 0;
	__s32 i, cnt, err;
    //__u32 *ptmp;
    struct __PhysicOpPara_t tmpPhyPage;
    //unsigned long ts0, te0;

    //ts0 = jiffies();

    if ((nDieNum>=LOGIC_DIE_CNT) || (nBlkNum>=BLK_CNT_OF_LOGIC_DIE) || (nPage>=PAGE_CNT_OF_LOGIC_BLK)) {
    	PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n",
    			__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);
    	return -1;
	}

    if ((pBuf==NULL) && (pSpare==NULL) && (SectBitmap)) {
    	PHY_ERR("%s: Fatal err -1, Wrong input parameter, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%lx\n",
    			__func__, pBuf, pSpare, SectBitmap);
    	return -1;
    }

    if ((pBuf==NULL) && (pSpare==NULL) && (SectBitmap==0)) {
    	PHY_DBG("%s: Warning -0, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%lx\n",
    			__func__, pBuf, pSpare, SectBitmap);
		return 0;
    }

    if (FORMAT_READ_SPARE_DEBUG_ON)
    {
		for (i=0; i<16; i++)
		{
			*(FORMAT_SPARE_BUF + i) = 0x99;
		}
    }

    //calculate the physical operation parameter by the die number, block number and page number
    _CalculatePhyOpPar(&tmpPhyPage, nDieNum, nBlkNum, nPage);

    if ( _ChangeNandOpSta(0) )
    {
    	return -ERR_TIMEOUT;
    }

    //set the sector bitmap in the page, the main data buffer and the spare data buffer
    if ((pBuf==NULL) && pSpare)//if ((SectBitmap==0) && (pBuf==NULL) && pSpare)
    {
    	//1. read spare data only
    	//read spare data in first physical page only
    	spare_bitmap = 0xFF; ///////////FULL_BITMAP_OF_SUPER_PAGE;

    	tmpPhyPage.SectBitmap = spare_bitmap;
    	tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;
    	tmpPhyPage.MDataPtr = FORMAT_PAGE_BUF;

#if(1)
		result = PHY_PageReadSpare(&tmpPhyPage);
#else
		result = PHY_PageRead(&tmpPhyPage);
#endif
    }
    else
    {
    	//2. read full page or several sectors
    	tmpPhyPage.SectBitmap = SectBitmap;
		tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;
		tmpPhyPage.MDataPtr = pBuf;

		result = PHY_PageRead(&tmpPhyPage);
    }

    if(pSpare)
    	MEMCPY(pSpare, tmpPhyPage.SDataPtr, 16); //8 --> 16

	if (FORMAT_READ_SPARE_DEBUG_ON)
	{
	    if ((pSpare) && (nPage == (PAGE_CNT_OF_LOGIC_BLK-1)))
	    {
	    	err = 0;
	    	//check default value
	    	cnt = 0;
			for (i=0; i<5; i++)
			{
				if (*((__u8 *)pSpare + i) == 0x99)
					cnt++;
				else
					break;
			}

			if(*((__u8 *)pSpare + 1) == 0xaa)
			{
			    if(*((__u8 *)pSpare + 2) != 0xaa)
			    {
			        err = 1;
			    }
			    if(*((__u8 *)pSpare + 3) != 0xff)
			    {
			        err = 1;
			    }

			    if(*((__u8 *)pSpare + 4) != 0xff)
			    {
			        err = 1;
			    }
			}
			else
			{
			    err = 1;
			}

			if (err)
			{
				cnt = 0;
				for (i=0; i<16; i++)
				{
					if ( *((__u8 *)pSpare + i) == 0xFF)
						cnt++;
					else
						break;
				}
				if (cnt == 16)
					return 0;

				PHY_ERR("===%s: nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n",__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);

    			for (i=0; i<16; i++)
				{
					PHY_ERR("0x%x ", *((__u8 *)pSpare + i));
				}
				PHY_ERR("\n");
				return 0;
			}
	    }
    }

    //te0 = jiffies();

    //PHY_DBG("%s: %d us\n", __func__, te0-ts0);

	return result;
}

__s32 PHY_WaitAllRbReady(void)
{
	__s32 ret;

	ret = _WaitAllRbReady();
	if (ret != 0)
		return -ERR_TIMEOUT;

	return 0;
}
/*
void _dump_maindata(__u32 buf, __u32 len, __u32 word_step)
{
	__u32 i;
	__u32 *psrc;
	
	psrc = (__u32 *)buf;
	PHY_ERR("dump main data:\n");
	for (i=0; i<len; i+=(word_step))
	{
		if (i%4==0)
			PHY_ERR("\n%04d: ", i);
		PHY_ERR("%08x ", psrc[i]);
	}
	PHY_ERR("\n");
}

void _dump_sparedata(__u32 buf, __u32 len)
{
	__u32 i;
	__u32 *psrc;
	
	psrc = (__u32 *)buf;
	
	PHY_ERR("dump spare data:\n");
	for (i=0; i<len; i++)
	{
		if (i%4==0)
			PHY_ERR("\n%04d: ", i);
		PHY_ERR("%08x ", psrc[i]);
	}
	PHY_ERR("\n");
}
*/
__s32 PHY_VirtualPageRead(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare)
{
	__s32 ret = 0;
	__s32 retry_cnt = 2;
//	__s32 i;
//	__u32 b, r, *psrc, *psrc_spare; 
//	__u8 *tbuf;
//	__u8 *tsbuf;

	while (retry_cnt != 0)
	{
		ret = _VirtualPageRead(nDieNum, nBlkNum, nPage, SectBitmap, pBuf, pSpare);
		if (ret<0)
			break;

		if (_ReadCheckSpare((__u8 *)pSpare, nDieNum, nBlkNum, nPage) == 0)
		{
			break;
		}
		else
		{
			PHY_ERR("%s: NPVP retry_cnt: %d\n", __func__, retry_cnt);
		}
		retry_cnt--;

		if (_WaitAllRbReady() != 0)
			return -ERR_TIMEOUT;
	}

//    if ( pSpare == NULL) 
//        return ret;
//    
//    if ( (*((__u8 *)pSpare + 0) != 0xff) ||  (*((__u8 *)pSpare + 1) != 0xc0) ||  (*((__u8 *)pSpare + 2) !=0x01) ||  (*((__u8 *)pSpare + 3) != 0xa8) ||  (*((__u8 *)pSpare + 4) != 0x00) )
//    {
//        return ret;  
//    }
//
//    PHY_ERR("Physic read addr: die 0x%x, blk 0x%x, page 0x%x, bitmap 0x%x\n", nDieNum, nBlkNum, nPage, (__u32)SectBitmap);
//    PHY_ERR("Physic read addr: buf 0x%x, spare 0x%x\n", (__u32)pBuf, (__u32)pSpare);
//    
//    if ( pBuf == NULL) 
//        return ret;   
//     
//    PHY_ERR("Physic read: ");
//    for (i=0; i<16; i++) 
//    	PHY_ERR("%x ", *((__u8 *)pBuf + 0x1000 + i));
//    PHY_ERR("\n");         
//    	
//    if ((*((__u8 *)pBuf + 0x1000) == 0xff) ||   ( *((__u8 *)pBuf + 0x1000) == 0x41 ))
//        return ret;	
//	            	           	
//    PHY_ERR("---read first time---- \n");      		
//    _dump_maindata((__u32)pBuf,SECTOR_CNT_OF_SUPER_PAGE*512/4, 1);
//    PHY_ERR("---read again---- \n");     	
//        
//
//    lock_nand();
//    PHY_ERR("---while 1---- \n");  
//    
//    for(i=0;i<4;i++)
//    {
//        if(i==0)
//        {
//            tbuf = PageCachePool.PageCache3;
//            tsbuf = PageCachePool.SpareCache3;
//        }
//        
//        if(i==1)
//        {
//            tbuf = PageCachePool.PageCache4;
//            tsbuf = PageCachePool.SpareCache4;
//        }        
//        if(i==2)
//        {
//            tbuf = PageCachePool.PageCache5;
//            tsbuf = PageCachePool.SpareCache5;
//        }        
//        if(i==3)
//        {
//            tbuf = PageCachePool.PageCache6;
//            tsbuf = PageCachePool.SpareCache6;
//        }
//
//        _VirtualPageRead(0, aw_nand_info.no_used_block_addr.Block_NO-1, i, SectBitmap, tbuf, tsbuf);
//        _dump_maindata((__u32)tbuf,SECTOR_CNT_OF_SUPER_PAGE*512/4, 1);
//    }
//    
//    PHY_ERR("---while 2---- \n"); 
//    while(1)
//    {
//        //PHY_ERR("---while 2---- \n");     
//    }


//RET: 

	return ret;
}

/*
************************************************************************************************************************
*                       WRITE PAGE DATA TO VIRTUAL BLOCK
*
*Description: Write page data to virtual block, the block is composed by several physical block.
*             It is named super block too.
*
*Arguments  : nDieNum   the number of the DIE, which the page is belonged to;
*             nBlkNum   the number of the virtual block in the die;
*             nPage     the number of the page in the virtual block;
*             Bitmap    the bitmap of the sectors need access in the page;
*             pBuf      the pointer to the page data buffer;
*             pSpare    the pointer to the spare data buffer.
*
*Return     : write result;
*               = 0     write page data successful;
*               < 0     write page data failed.
************************************************************************************************************************
*/
__s32 PHY_VirtualPageWrite(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare)
{
    __s32 result;
    __s32 i, err; //, cnt;
    //__u32 *ptmp;
    struct __PhysicOpPara_t tmpPhyPage;

    if ((nDieNum>=LOGIC_DIE_CNT) || (nBlkNum>=BLK_CNT_OF_LOGIC_DIE) || (nPage>=PAGE_CNT_OF_LOGIC_BLK)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n",
				__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);
		return -1;
	}

	if ((pBuf==NULL) && (pSpare==NULL) && (SectBitmap)) {
		PHY_ERR("%s: Fatal err -1, Wrong input parameter, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%x\n",
				__func__, pBuf, pSpare, SectBitmap);
		return -1;
	}

	if (SectBitmap != FULL_BITMAP_OF_SUPER_PAGE) {
		PHY_ERR("%s: Fatal err -2, Wrong input parameter, unaligned write page SectBitmap: 0x%llx\n",
				__func__, SectBitmap);
		return -1;
	}

	if ((pBuf==NULL) && (pSpare==NULL) && (SectBitmap==0)) {
    	PHY_DBG("%s: Warning -0, pBuf: 0x%08x  pSpare: 0x%08x  SectBitmap: 0x%lx\n",
    			__func__, pBuf, pSpare, SectBitmap);
		return 0;
    }

	if (FORMAT_WRITE_SPARE_DEBUG_ON)
	{
	    if ((pSpare) && (nPage == (PAGE_CNT_OF_LOGIC_BLK-1)))
	    {
	    	err = 0;
	    	//check default value
	    	//cnt = 0;

			if(*((__u8 *)pSpare + 1) == 0xaa)
			{
			    if(*((__u8 *)pSpare + 2) != 0xaa)
			    {
			        err = 1;
			    }
			    if(*((__u8 *)pSpare + 3) != 0xff)
			    {
			        err = 1;
			    }

			    if(*((__u8 *)pSpare + 4) != 0xff)
			    {
			        err = 1;
			    }
			}
		    else
			{
			    err = 1;
			}

			if (err)
			{
				PHY_ERR("%s: nDieNum: %d/%d  nBlkNum: %d/%d  nPage: %d/%d\n",__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE, nPage, PAGE_CNT_OF_LOGIC_BLK);

				for (i=0; i<16; i++)
				{
					PHY_ERR("0x%x ", *((__u8 *)pSpare + i));
				}
				PHY_ERR("\n");

				//Gavin-20130815, do not return -1
				//return -1;
			}
	    }
    }

    //calculate the physical operation parameter by te die number, block number and page number
    //_CalculatePhyOpPar(&tmpPhyPage, nDieNum * ZONE_CNT_OF_DIE, nBlkNum, nPage);
    _CalculatePhyOpPar(&tmpPhyPage, nDieNum, nBlkNum, nPage);

    //set the sector bitmap in the page, the main data buffer and the spare data buffer
    tmpPhyPage.SectBitmap = SectBitmap;
    tmpPhyPage.MDataPtr = pBuf;
    tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;

    //process spare area data
    if(pSpare)
    {
        MEMCPY(FORMAT_SPARE_BUF, pSpare, 16); //8-->16
    }
    else
    {
        MEMSET(FORMAT_SPARE_BUF, 0xff, 16);
    }

	if ( _ChangeNandOpSta(1) )
    {
    	return -ERR_TIMEOUT;
    }

    result = PHY_PageWrite(&tmpPhyPage);

#if 0
    //physical page write module is successful, synch the operation result to check if write successful true
    result = PHY_SynchBank(tmpPhyPage.BankNum, SYNC_CHIP_MODE);
    if(result < 0)
    {
        //some error happens when synch the write operation, report the error type
        return result;
    }
    else
    {
        return 0;
    }
#endif

    return result;
}

/*
************************************************************************************************************************
*                       ERASE VIRTUAL BLOCK
*
*Description: Erase a virtual blcok.
*
*Arguments  : nDieNum   the number of the DIE, which the block is belonged to;
*             nBlkNum   the number of the virtual block in the die.
*
*Return     : erase result;
*               = 0     virtual block erase successful;
*               < 0     virtual block erase failed.
************************************************************************************************************************
*/
__s32 PHY_VirtualBlockErase(__u32 nDieNum, __u32 nBlkNum)
{
    __s32 i, result = 0;
    struct __PhysicOpPara_t tmpPhyBlk;

    if ((nDieNum>=LOGIC_DIE_CNT) || (nBlkNum>=BLK_CNT_OF_LOGIC_DIE)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d \n",
				__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE);
		return -1;
	}

    if ( _ChangeNandOpSta(2) )
    {
    	return -1;
    }

    //erase every block belonged to different banks
    for(i=0; i<INTERLEAVE_BANK_CNT; i++)
    {
        _CalculatePhyOpPar(&tmpPhyBlk, nDieNum, nBlkNum, i);

        PHY_BlockErase(&tmpPhyBlk);
    }

    //check the result of the block erase
    for(i=0; i<INTERLEAVE_BANK_CNT; i++)
    {
        result = PHY_SynchBank(i, SYNC_CHIP_MODE);
        if(result < 0)
        {
            return -1;
        }
    }

    return 0;
}

__s32 FMT_NandSorting_IsBadblk(__u32 nDieNum, __u32 nBlkNum, __u32 nPage)
{
	__s32 result = 0;
	__u32 bad_flag = 0;
	__u32 tmpBnkNum,tmpPage;
	struct __PhysicOpPara_t tmpPhyPage;

	for(tmpBnkNum=0; tmpBnkNum<INTERLEAVE_BANK_CNT; tmpBnkNum++)
	{
		
		tmpPage = nPage * INTERLEAVE_BANK_CNT + tmpBnkNum;
		_CalculatePhyOpPar(&tmpPhyPage, nDieNum , nBlkNum, tmpPage);
		result = PHY_Nand_Sorting_IsBadblock_superblk(&tmpPhyPage);
		if(result==1)
			bad_flag = 1;
	}
	if(bad_flag == 1)
		return 1;
	
	return result;
}



/*
************************************************************************************************************************
*                       CHECK BAD BLOCK
*
*Description: Check a virtual block is bad or good.
*
*Arguments  : nDieNum   the number of the DIE, which the block is belonged to;
*             nBlkNum   the number of the virtual block in the die.
*
*Return     : erase result;
*               = 0     virtual block is a good block;
*               = 1     virtual block is a bad block.
************************************************************************************************************************
*/
__s32 PHY_VirtualBadBlockCheck(__u32 nDieNum, __u32 nBlkNum)
{
	struct __PhysicOpPara_t tmpPhyPage = {0};
	__u32 tmp_logic_die, tmp_super_blk;
	__u64 spare_bitmap;
	__s32 tmpPageNum[4];
    __u32 tmpBnkNum, tmpPage, tmpBadFlag;
    __s32 i;
    __u8 *temp_mdata_ptr0, *temp_mdata_ptr1;
    __u8 *temp_spare_ptr;

	__s32 result;

	if ( (nDieNum>=LOGIC_DIE_CNT) || (nBlkNum>=BLK_CNT_OF_LOGIC_DIE) )  {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d \n",
				__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE);
		return -1;
	}

	if ( _ChangeNandOpSta(0) )
    {
    	return -1;
    }

	//PHY_DBG("[PHY_DBG]%s: Start checking block %d in die %d ....\n", __func__, nBlkNum, nDieNum);

    tmp_logic_die  = nDieNum;
    tmp_super_blk  = nBlkNum;

    //initiate the number of the pages which need be read, the first page is read always, because the
    //the logical information is stored in the first page, other pages is read for check bad block flag
    tmpPageNum[0] = 0;
    tmpPageNum[1] = -1;
    tmpPageNum[2] = -1;
    tmpPageNum[3] = -1;

    //analyze the number of pages which need be read
	switch(BAD_BLK_FLAG_PST & 0x03)
	{
	  case 0x00:
		  //the bad block flag is in the first page, same as the logical information, just read 1 page is ok
		  break;

	  case 0x01:
		  //the bad block flag is in the first page or the second page, need read the first page and the second page
		  tmpPageNum[1] = 1;
		  break;

	  case 0x02:
		  //the bad block flag is in the last page, need read the first page and the last page
		  tmpPageNum[1] = PAGE_CNT_OF_PHY_BLK - 1;
		  break;

	  case 0x03:
		  //the bad block flag is in the last 2 page, so, need read the first page, the last page and the last-1 page
		  tmpPageNum[1] = PAGE_CNT_OF_PHY_BLK - 1;
		  tmpPageNum[2] = PAGE_CNT_OF_PHY_BLK - 2;
		  break;
	}

	//only read first 4 byte user data(first 4 ecc data block) of each single page in a super page
	if ( SUPPORT_MULTI_PROGRAM && (CHANNEL_CNT==2) ) {
		spare_bitmap = (((__u64)0x3) | (((__u64)0x3)<<SECTOR_CNT_OF_SINGLE_PAGE) | \
				(((__u64)0x3)<<SECTOR_CNT_OF_SINGLE_PAGE*2) | (((__u64)0x3)<<SECTOR_CNT_OF_SINGLE_PAGE*3));
	} else if ( SUPPORT_MULTI_PROGRAM || (CHANNEL_CNT==2) ) {
		spare_bitmap = (((__u64)0x3) | (((__u64)0x3)<<SECTOR_CNT_OF_SINGLE_PAGE));
	} else {
		spare_bitmap = 0x3;
	}

	//initiate the bad block flag
	tmpBadFlag = 0;

	//the super block is composed of several physical blocks in several banks
	for(tmpBnkNum=0; tmpBnkNum<INTERLEAVE_BANK_CNT; tmpBnkNum++)
	{
		for(i=3; i>=0; i--)
		{
			if(tmpPageNum[i] == -1)
			{
				//need not check page
				continue;
			}

			//calculate the number of the page in the super block to get spare data
			tmpPage = tmpPageNum[i] * INTERLEAVE_BANK_CNT + tmpBnkNum;

			//calculate parameter
			_CalculatePhyOpPar(&tmpPhyPage, tmp_logic_die, tmp_super_blk, tmpPage);
	    	tmpPhyPage.SectBitmap = spare_bitmap;
	    	tmpPhyPage.SDataPtr = FORMAT_SPARE_BUF;
	    	tmpPhyPage.MDataPtr = FORMAT_PAGE_BUF;

			temp_mdata_ptr0 = (__u8*)FORMAT_PAGE_BUF;
			temp_mdata_ptr1 = (__u8*)((__u32)FORMAT_PAGE_BUF + SECTOR_CNT_OF_SINGLE_PAGE*512);
			temp_spare_ptr = (__u8*)FORMAT_SPARE_BUF;
			//check if the block is a bad block
			if(NandStorageInfo.NandChipId[0]==0x45) //for sandisk 19nm flash bad blk scan
			{
				//PHY_DBG("[PHY_DBG]%s: read main data and spare(sandisk 19nm): bank %d  blk %d  page %d ....\n", __func__, tmpPhyPage.BankNum, tmpPhyPage.BlkNum, tmpPhyPage.PageNum);
				result = PHY_PageRead(&tmpPhyPage);

				if(result==-ERR_ECC)
				{
					if(((*temp_mdata_ptr0==0x00) && (*(temp_mdata_ptr0+1)==0x00) && (*(temp_mdata_ptr0+2)==0x00) && (*(temp_mdata_ptr0+3)==0x00) && (*(temp_mdata_ptr0+4)==0x00) && (*(temp_mdata_ptr0+5)==0x00))
						||(SUPPORT_MULTI_PROGRAM && ((*temp_mdata_ptr1==0x00) && (*(temp_mdata_ptr1+1)==0x00) && (*(temp_mdata_ptr1+2)==0x00) && (*(temp_mdata_ptr1+3)==0x00) && (*(temp_mdata_ptr1+4)==0x00) && (*(temp_mdata_ptr1+5)==0x00))))
					{
						tmpBadFlag = 1;
					}
					else
					{
						if((*temp_spare_ptr != 0xff))
						{
							tmpBadFlag = 1;
						}
					}
				}
				else
				{
					if((*temp_spare_ptr != 0xff) || (SUPPORT_MULTI_PROGRAM && (*temp_spare_ptr != 0xff)))
					{
						tmpBadFlag = 1;
					}
				}
			}
			else
			{
				//PHY_DBG("[PHY_DBG]%s: read spare: bank %d  blk %d  page %d ....\n", __func__, tmpPhyPage.BankNum, tmpPhyPage.BlkNum, tmpPhyPage.PageNum);
				result = PHY_PageReadSpare(&tmpPhyPage);

				if((*temp_spare_ptr != 0xff))
				{
					//set the bad flag of the physical block
					tmpBadFlag = 1;
				}
			}
		}
	}

    /*check if the block is a bad block*/
    if( tmpBadFlag == 1 ) {
    	/*the physical block is a bad block, set bad block flag in the logical information buffer*/
    	PHY_DBG("[PHY_DBG] Find a bad block (NO. 0x%x) in the Die 0x%x\n", tmp_super_blk, tmp_logic_die);
    	return 1; /*bad block*/
	} else {
		return 0; /*good block*/
	}
}

/*
************************************************************************************************************************
*                       MARK BAD BLOCK
*
*Description: Mark a virtual block as a bad block.
*
*Arguments  : nDieNum   the number of the DIE, which the block is belonged to;
*             nBlkNum   the number of the virtual block in the die.
*
*Return     : erase result;
*               = 0     mark virtual block successfully;
*               < 0     mark virtual block failed.
************************************************************************************************************************
*/
__s32 PHY_VirtualBadBlockMark(__u32 nDieNum, __u32 nBlkNum)
{
    __s32   i;
    __u8 tmpSpare[16];
    __s32 result;

	if ((nDieNum>=LOGIC_DIE_CNT) || (nBlkNum>=BLK_CNT_OF_LOGIC_DIE)) {
		PHY_ERR("%s: Fatal err -0, Wrong input parameter, nDieNum: %d/%d  nBlkNum: %d/%d \n",
				__func__, nDieNum, LOGIC_DIE_CNT, nBlkNum, BLK_CNT_OF_LOGIC_DIE);
		return -1;
	}

    PHY_DBG("[PHY_DBG]%s: Start marking block %d in die %d ....\n", __func__, nBlkNum, nDieNum);

    //set bad block flag to the spare data write to nand flash
    for (i=0; i<16; i++)
    	tmpSpare[i] = 0;

    if (PHY_VirtualBadBlockCheck(nDieNum, nBlkNum)) {
    	PHY_DBG("[PHY_DBG]%s: Block %d in die %d is already a bad block!\n", __func__, nBlkNum, nDieNum);
    	return 0;
    }

    //erase block
    result = PHY_VirtualBlockErase(nDieNum, nBlkNum);
    //if (result<0) {
    //	return result;
    //}

    for(i=0; i<INTERLEAVE_BANK_CNT; i++)
    {
        //write the bad block flag on the first page
    	result = PHY_VirtualPageWrite(nDieNum, nBlkNum, i, FULL_BITMAP_OF_SUPER_PAGE, FORMAT_PAGE_BUF, (void *)tmpSpare);
    	//if (result<0) {
    	//	break;
    	//}

        //write the bad block flag on the last page
        result |= PHY_VirtualPageWrite(nDieNum, nBlkNum, PAGE_CNT_OF_SUPER_BLK - INTERLEAVE_BANK_CNT + i, \
                        FULL_BITMAP_OF_SUPER_PAGE, FORMAT_PAGE_BUF, (void *)tmpSpare);
        //if (result<0) {
        //	break;
        //}
    }

    if (result<0) {
    	PHY_ERR("[PHY_ERR]%s: Mark bad block %d in die %d failed!\n", __func__, nBlkNum, nDieNum);
    } else {
    	if ( PHY_VirtualBadBlockCheck(nDieNum, nBlkNum) )
    		PHY_DBG("[PHY_DBG]%s: Mark bad block %d in die %d ok!\n", __func__, nBlkNum, nDieNum);
    	else
    		PHY_ERR("[PHY_ERR]%s: Mark bad block %d in die %d failed!\n", __func__, nBlkNum, nDieNum);
    }
    return result;
}

__s32 FMT_Init(void)
{
    LogicArchiPar.SectCntPerLogicPage = NandStorageInfo.SectorCntPerPage * NandStorageInfo.PlaneCntPerDie* NandStorageInfo.ChannelCnt;
    LogicArchiPar.PageCntPerLogicBlk = NandStorageInfo.PageCntPerPhyBlk * NandStorageInfo.BankCntPerChip;
    if(SUPPORT_EXT_INTERLEAVE)
    {
       if(NandStorageInfo.ChipCnt >=2)
          LogicArchiPar.PageCntPerLogicBlk *= 2;
    }

    //init some local variable
    DieCntOfNand = NandStorageInfo.DieCntPerChip / NandStorageInfo.BankCntPerChip;
    if(!SUPPORT_EXT_INTERLEAVE)
    {
        DieCntOfNand *= NandStorageInfo.ChipCnt;
    }
    if(SUPPORT_EXT_INTERLEAVE)
    {
       if(NandStorageInfo.ChipCnt >=2)
	   DieCntOfNand *= (NandStorageInfo.ChipCnt/2);
    }
    LogicArchiPar.LogicDieCnt = DieCntOfNand;
    SuperBlkCntOfDie = NandStorageInfo.BlkCntPerDie / NandStorageInfo.PlaneCntPerDie;
    LogicArchiPar.LogicBlkCntPerLogicDie = SuperBlkCntOfDie;

    FORMAT_DBG("\n");
    FORMAT_DBG("[FORMAT_DBG] ===========Logical Architecture Parameter===========\n");
    FORMAT_DBG("[FORMAT_DBG]    Page Count of Logic Block:  0x%x\n", LogicArchiPar.PageCntPerLogicBlk);
    FORMAT_DBG("[FORMAT_DBG]    Sector Count of Logic Page: 0x%x\n", LogicArchiPar.SectCntPerLogicPage);
    FORMAT_DBG("[FORMAT_DBG]    Block Count of Die:         0x%x\n", SuperBlkCntOfDie);
    FORMAT_DBG("[FORMAT_DBG]    Die Count:                  0x%x\n", LogicArchiPar.LogicDieCnt);
    FORMAT_DBG("[FORMAT_DBG] ===================================================\n");

	CurNandOpSta = 0xFF;
	
    return 0;
}

__s32 FMT_Exit(void)
{
	int ret;
	
	ret = _ChangeNandOpSta(0xff);
	if(ret<0)
	{
		FORMAT_ERR("FMT_Exit: _ChangeNandOpSta return 0x%x", ret);
	}
    //release memory resource
    return 0;
}





