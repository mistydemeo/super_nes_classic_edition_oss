#include "nand_drv_cfg.h"
#include "nand_physic.h"
#include "nand_scan.h"
#include "nand_format.h"
#include "nfc.h"
#include "nand_scan.h"
#include "../phy/phy.h"
#include "uboot_head.h"

#ifndef CONFIG_NAND_LOGIC_START_BLOCK
#define CONFIG_NAND_LOGIC_START_BLOCK 128
#endif

//#define NAND_REG_LENGTH     (0xC4>>2)
#define NAND_REG_LENGTH		(0x220>>2)

__u32 MaxBlkEraseTimes = 0;

struct _nand_info aw_nand_info = {0};
struct _uboot_info uboot_info = {0};
__u32 nand_reg_back[2][NAND_REG_LENGTH] = {{0},{0}};

extern struct _boot_info * phyinfo_buf;
extern struct __NandStorageInfo_t  NandStorageInfo;
extern struct __NandPageCachePool_t PageCachePool;


extern  __u32 RetryCount[2][MAX_CHIP_SELECT_CNT];

extern __u32 DieCntOfNand;
extern __u32 SuperBlkCntOfDie;

extern __u32 is_uboot_block(__u32 sys_mode,__u32 block, char* uboot_buf);
extern __s32 _cal_sum_physic_info( __u32 *mem_base, __u32 size );
extern __s32 Set_Hynix_Special_Info(void);

extern __u32 NAND_GetLsbpages(void);
extern __u32 NAND_GetLsbpagetype(void);
extern __s32 NAND_GetLsbblksize(void);
extern __s32 PHY_Readretry_reset(void);
extern __u32 NAND_GetPageSize(void);
extern __u32 Nand_Is_lsb_page(__u32 read_retry_type,__u32 page);

extern int _get_uboot_offset(int sys_mode, void *buf);

__u32 uboot_info_init(void);
struct _nand_info* NandHwInit(void)
//void NandHwInit(void)
{
	__s32 ret = 0;
	__u32 uboot_block_size;
	__u32 uboot_start_block;
	__u32 uboot_next_block;

//	FORMAT_ERR("%s: nand driver physical layer version: %x, %x, %x, %02d:%02d\n", __func__,
//		NAND_DRV_PHYSIC_LAYER_VERSION_0, NAND_DRV_PHYSIC_LAYER_VERSION_1, NAND_DRV_PHYSIC_LAYER_DATE,
//		NAND_DRV_PHYSIC_LAYER_DATE_HOUR, NAND_DRV_PHYSIC_LAYER_MINUTE);

	FORMAT_DBG("%s: Start Nand Hardware initializing .....\n", __func__);
	
//	val[0] = NAND_DRV_PHYSIC_LAYER_VERSION_0;
//	val[1] = NAND_DRV_PHYSIC_LAYER_VERSION_1;
//	val[2] = NAND_DRV_PHYSIC_LAYER_DATE;
//	val[3] = NAND_DRV_PHYSIC_LAYER_TIME;
//	NAND_ShowEnv(0, "physical version", 4, val);
	NAND_Print_Version();

	if( PHY_Init() ) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: PHY_Init() failed!\n", __func__);
		goto ERR_RET;
	}

	if( SCN_AnalyzeNandSystem() ) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: SCN_AnalyzeNandSystem() failed!\n", __func__);
		goto ERR_RET;
	}

	//PHY_ChangeMode(1);

#if 0
	ret = PHY_ScanDDRParam();
	if (ret < 0) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: PHY_ScanDDRParam() failed!\n", __func__);
		goto ERR_RET;
	}
#endif

	FMT_Init();

	MEMSET(&aw_nand_info, 0x0, sizeof(struct _nand_info));

	if (LogicArchiPar.SectCntPerLogicPage < 8) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: super page size is less than 4KB, "
				"then no enough spare data for NFTL!\n", __func__);
		goto ERR_RET;
	}


	aw_nand_info.type = 0;
	aw_nand_info.SectorNumsPerPage = LogicArchiPar.SectCntPerLogicPage;
	aw_nand_info.BytesUserData = 16;
	aw_nand_info.BlkPerChip = LogicArchiPar.LogicBlkCntPerLogicDie;
	aw_nand_info.ChipNum = LogicArchiPar.LogicDieCnt;
	aw_nand_info.PageNumsPerBlk = LogicArchiPar.PageCntPerLogicBlk;
	aw_nand_info.FullBitmap = FULL_BITMAP_OF_SUPER_PAGE;
    aw_nand_info.boot = phyinfo_buf;

	Set_Hynix_Special_Info();

	uboot_block_size = NAND_GetLsbblksize();

	/*small nand:block size < 1MB*/
	if(uboot_block_size <= 0x20000)  //128K
	{
		uboot_start_block = 8;
		uboot_next_block = uboot_start_block + 32;
	}
	else if(uboot_block_size <= 0x40000)  //256k
	{
		uboot_start_block = 8;
		uboot_next_block = uboot_start_block + 24;
	}
	else if(uboot_block_size <= 0x80000)  //512k
	{
		uboot_start_block = 8;
		uboot_next_block = uboot_start_block + 16;
	}
	else if(uboot_block_size <= 0x100000)  //1M
	{
		uboot_start_block = 8;
		uboot_next_block = uboot_start_block + 16;
	}
	else if(uboot_block_size <= 0x200000)  //2M
	{
		uboot_start_block = 4;
		uboot_next_block = uboot_start_block + 10;
	}
	else
	{
		uboot_start_block = 4;
		uboot_next_block = uboot_start_block + 8;
	}

	if(aw_nand_info.boot->uboot_start_block == 0)
	{
	    aw_nand_info.boot->uboot_start_block = uboot_start_block;
	    aw_nand_info.boot->uboot_next_block = uboot_next_block;
	}

	aw_nand_info.boot->logic_start_block_by_single = CONFIG_NAND_LOGIC_START_BLOCK;

#if 0
	if (SUPPORT_USE_MAX_BLK_ERASE_CNT)
		aw_nand_info.MaxBlkEraseTimes = MaxBlkEraseTimes;
	else
		aw_nand_info.MaxBlkEraseTimes = 5000;
#else
	aw_nand_info.MaxBlkEraseTimes = MaxBlkEraseTimes;
#endif
	//if (SUPPORT_READ_RECLAIM)
	if(1)
		aw_nand_info.EnableReadReclaim = 1;
	else 
		aw_nand_info.EnableReadReclaim = 0;


ERR_RET:

	if (ret < 0)
		FORMAT_ERR("%s: End Nand Hardware initializing ..... FAIL!\n", __func__);
	else
		FORMAT_DBG("%s: End Nand Hardware initializing ..... OK!\n", __func__);

    return (ret<0) ? (struct _nand_info*)NULL : &aw_nand_info;
	//return ;
}

__s32 GetUbootStartBlock(void)
{
    return aw_nand_info.boot->uboot_start_block;
}

__s32 GetUbootNextBlock(void)
{
    return aw_nand_info.boot->uboot_next_block;
}

__s32 GetLogicStartBlock(void)
{
    return aw_nand_info.boot->logic_start_block_by_single;
}


__s32 NandHwExit(void)
{
	FMT_Exit();
	PHY_Exit();

	return 0;
}

__s32 NandHwNormalStandby(void)
{
	NAND_PhysicLock();
	return 0;
}

__s32 NandHwNormalResume(void)
{
	NAND_PhysicUnLock();
	return 0;
}



__s32 NandHwSuperStandby(void)
{
	__u32 i,j, nand_ch_cnt;
	__u32 nand_index_bak = NAND_GetCurrentCH();
	__s32 ret = 0;
	__u32 bank = 0;


	 NAND_PhysicLock();

	nand_ch_cnt = NAND_GetChannelCnt();

	 if(nand_index_bak!=0)
	    FORMAT_ERR("[NAND] currnt index: %d\n", nand_index_bak);

	 //sync all chip
	for(j=0;j<nand_ch_cnt;j++)
	{
	    NAND_SetCurrentCH(j);
	    //printk("nand ch %d, chipconnectinfo: 0x%x\n ", j, NAND_GetChipConnect());
	    for(i=0; i<4; i++)
	    {
	        if(NAND_GetChipConnect()&(0x1<<i)) //chip valid
	        {
	            //nand_dbg_err("nand reset ch %d, chip %d!\n",j, i);
	            ret = PHY_SynchBank(bank, 0);
	            bank++;
	            if(ret)
	                FORMAT_ERR("nand reset ch %d, chip %d failed!\n",j, i);
	        }
	        else
	        {
	            //nand_dbg_err("nand skip ch %d, chip %d!\n",j, i);
	        }
	    }
	}
	NAND_SetCurrentCH(nand_index_bak);

	nand_ch_cnt = NAND_GetChannelCnt();

	for(j=0;j<nand_ch_cnt;j++)
	{
		for(i=0; i<(NAND_REG_LENGTH); i++){
			if(j==0)
			{
				nand_reg_back[j][i] = *(volatile __u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04);
				//nand_dbg_err("nand ch %d, reg 0x%x, value: 0x%x\n", j, NAND_GetIOBaseAddrCH0() + i*0x04, *(volatile u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04));
			}
			else if(j==1)
			{
				nand_reg_back[j][i] = *(volatile __u32 *)(NAND_GetIOBaseAddrCH1() + i*0x04);
				//nand_dbg_err("nand ch %d, reg 0x%x, value: 0x%x\n", j, NAND_GetIOBaseAddrCH1() + i*0x04, *(volatile u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04));
			}
		}
	}

	for(j=0;j<nand_ch_cnt;j++)
	{
		NAND_ClkRelease(j);
		NAND_PIORelease(j);
	}

	NAND_ReleaseVoltage();

	return 0;
}

__s32 NandHwSuperResume(void)
{
    __u32 nand_ch_cnt=NAND_GetChannelCnt();
    __u32 nand_index_bak = NAND_GetCurrentCH();
    __u32 bank = 0;
    __u32 i, j;
    __s32 ret = 0;
    NFC_INIT_INFO nand_info;

	if(nand_index_bak!=0)
	    FORMAT_ERR("[NAND] currnt index: %d\n", nand_index_bak);

	NAND_GetVoltage();

	for(j=0;j<nand_ch_cnt;j++)
	{
	    NAND_PIORequest(j);
	    NAND_ClkRequest(j);
	}

	//process for super standby
	//restore reg state
	for(j=0;j<nand_ch_cnt;j++)
	{
	    for(i=0; i<(NAND_REG_LENGTH); i++){
	        if(0x9 == i){
	            continue;
	        }
	        if(j==0)
	        {
	            *(volatile __u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04) = nand_reg_back[j][i];
	        }
	        else if(j==1)
	        {
	            *(volatile __u32 *)(NAND_GetIOBaseAddrCH1() + i*0x04) = nand_reg_back[j][i];
	        }
	    }
	}

	//reset all chip
	for(j=0;j<nand_ch_cnt;j++)
	{
	    NAND_SetCurrentCH(j);
	    //nand_dbg_err("nand ch %d, chipconnectinfo: 0x%x\n ", j, NAND_GetChipConnect());
	    for(i=0; i<4; i++)
	    {
	        if(NAND_GetChipConnect()&(0x1<<i)) //chip valid
	        {
	            //nand_dbg_err("nand reset ch %d, chip %d!\n",j, i);
	            ret = PHY_ResetChip(i);
	            ret |= PHY_SynchBank(bank, 0);
	            bank++;
	            if(ret)
	                FORMAT_ERR("nand reset ch %d, chip %d failed!\n",j, i);
	        }
	        else
	        {
	            //nand_dbg_err("nand skip ch %d, chip %d!\n",j, i);
	        }
	    }

	    {		
			if ((DDR_TYPE == TOG_DDR) || (DDR_TYPE == TOG_DDR2))
			{
				for(NandIndex = 0; NandIndex<CHANNEL_CNT; NandIndex++)
				{
#if 0
					ret = NAND_SetClk(NandIndex, 20, 20*2);
					if (ret) {
						PHY_ERR("NandHwSuperResume: init toggle interface with classic"
							"clk cfg(%d, %d) failed!\n", 20, 20*2);
						return -1;
					}

					nand_info.ddr_type = TOG_DDR;
					nand_info.ddr_edo = 0x2;
					nand_info.ddr_delay = 0x1f;

					NFC_InitDDRParam(0, (nand_info.ddr_edo<<8)|(nand_info.ddr_delay));
					NFC_ChangeInterfaceMode(&nand_info);
#else
					ret = NAND_SetClk(NandIndex, NAND_ACCESS_FREQUENCE, NAND_ACCESS_FREQUENCE*2);
					if (ret) {
						PHY_ERR("NandHwSuperResume: direct resume toggle interface"
							"clk cfg(%d, %d) failed!\n", NAND_ACCESS_FREQUENCE, NAND_ACCESS_FREQUENCE*2);
						return -1;
					}
#endif
					if(NandIndex == (CHANNEL_CNT-1))
						break;
				}
				NandIndex = 0;
			}
			else if ((DDR_TYPE == ONFI_DDR) || (DDR_TYPE == ONFI_DDR2) || (DDR_TYPE == SDR) )
			{
				for(NandIndex = 0; NandIndex<CHANNEL_CNT; NandIndex++)
				{
					ret = NAND_SetClk(NandIndex, 10, 10*2);
					if (ret) {
						PHY_ERR("NandHwSuperResume: init sdr interface with classic"
							"clk cfg(%d, %d) failed!\n", 10, 10*2);
						return -1;
					}

					nand_info.ddr_type = SDR;
					nand_info.serial_access_mode = 0x1;
					nand_info.ddr_edo = 0x1;
					nand_info.ddr_delay = 0;

					NFC_InitDDRParam(0, (0x2<<8)|(0x1f)); //reset to defaul value
					NFC_ChangeInterfaceMode(&nand_info);

					if(NandIndex == (CHANNEL_CNT-1))
						break;
				}
				NandIndex = 0;	
			} 
			else 
			{
				PHY_ERR("NandHwSuperResume: wrong ddr type %d\n", DDR_TYPE);
			}
			
	    	PHY_ChangeMode(DDR_TYPE, (void *)&NandDdrInfo, NAND_ACCESS_FREQUENCE, NAND_ACCESS_FREQUENCE*2);
	    }
	}

	NAND_SetCurrentCH(nand_index_bak);


	PHY_Readretry_reset();

	//process for super standby
	//restore reg state
	for(j=0;j<nand_ch_cnt;j++)
	{
	    for(i=0; i<(NAND_REG_LENGTH); i++){
	        if(0x9 == i){
	            continue;
	        }
	        if(j==0)
	        {
	            *(volatile __u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04) = nand_reg_back[j][i];
	            //nand_dbg_err("nand ch %d, reg 0x%x, value: 0x%x\n", j, NAND_GetIOBaseAddrCH0() + i*0x04, *(volatile u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04));
	        }
	        else if(j==1)
	        {
	            *(volatile __u32 *)(NAND_GetIOBaseAddrCH1() + i*0x04) = nand_reg_back[j][i];
	            //nand_dbg_err("nand ch %d, reg 0x%x, value: 0x%x\n", j, NAND_GetIOBaseAddrCH1() + i*0x04, *(volatile u32 *)(NAND_GetIOBaseAddrCH0() + i*0x04));
	        }
	    }
	}

	NAND_PhysicUnLock();
	return 0;
}


__s32 NandHwShutDown(void)
{
    __u32 nand_ch_cnt=NAND_GetChannelCnt();
    __u32 nand_index_bak = NAND_GetCurrentCH();
    __u32 bank = 0;
    __u32 i, j;
    __s32 ret = 0;

	NAND_PhysicLock();

	if(nand_index_bak!=0)
	    FORMAT_ERR("[NAND] currnt index: %d\n", nand_index_bak);
	
	//sync all chip
	for(j=0;j<nand_ch_cnt;j++)
	{
	    NAND_SetCurrentCH(j);
	    //nand_dbg_err("nand ch %d, chipconnectinfo: 0x%x\n ", j, NAND_GetChipConnect());
	    for(i=0; i<4; i++)
	    {
	        if(NAND_GetChipConnect()&(0x1<<i)) //chip valid
	        {
	            //nand_dbg_err("nand reset ch %d, chip %d!\n",j, i);
	            ret = PHY_SynchBank(bank, 0);
	            bank++;
	            if(ret)
	                FORMAT_ERR("nand reset ch %d, chip %d failed!\n",j, i);
	        }
	        else
	        {
	            //nand_dbg_err("nand skip ch %d, chip %d!\n",j, i);
	        }
	    }
	}
	PHY_Exit();

	return 0;
}

__s32 NandHw_DbgInfo(__u32 type)
{
	//PHY_ERR("%s: nand driver physical layer version: %x, %x, %x, %x\n", __func__,
	//	NAND_DRV_PHYSIC_LAYER_VERSION_0, NAND_DRV_PHYSIC_LAYER_VERSION_1, 
	//	NAND_DRV_PHYSIC_LAYER_DATE, NAND_DRV_PHYSIC_LAYER_TIME);
			
	if (type == 0) {
		/*
			NandStorageInfo
		*/		
		PHY_ERR("\n\n");
	    PHY_ERR(" ==============Nand Architecture Parameter==============\n");
	    PHY_ERR("    Nand Chip ID:         0x%x 0x%x\n",
	        (NandStorageInfo.NandChipId[0] << 0) | (NandStorageInfo.NandChipId[1] << 8)
	        | (NandStorageInfo.NandChipId[2] << 16) | (NandStorageInfo.NandChipId[3] << 24),
	        (NandStorageInfo.NandChipId[4] << 0) | (NandStorageInfo.NandChipId[5] << 8)
	        | (NandStorageInfo.NandChipId[6] << 16) | (NandStorageInfo.NandChipId[7] << 24));
	    PHY_ERR("    Nand Channel Count:   0x%x\n", NandStorageInfo.ChannelCnt);
	    PHY_ERR("    Nand Chip Count:      0x%x\n", NandStorageInfo.ChipCnt);
	    PHY_ERR("    Nand Chip Connect:    0x%x\n", NandStorageInfo.ChipConnectInfo);
		PHY_ERR("    Nand Rb Connect Mode:      0x%x\n", NandStorageInfo.RbConnectMode);
	    PHY_ERR("    Sector Count Of Page: 0x%x\n", NandStorageInfo.SectorCntPerPage);
	    PHY_ERR("    Page Count Of Block:  0x%x\n", NandStorageInfo.PageCntPerPhyBlk);
	    PHY_ERR("    Block Count Of Die:   0x%x\n", NandStorageInfo.BlkCntPerDie);
	    PHY_ERR("    Plane Count Of Die:   0x%x\n", NandStorageInfo.PlaneCntPerDie);
	    PHY_ERR("    Die Count Of Chip:    0x%x\n", NandStorageInfo.DieCntPerChip);
	    PHY_ERR("    Bank Count Of Chip:   0x%x\n", NandStorageInfo.BankCntPerChip);
	    PHY_ERR("    Optional Operation:   0x%x\n", NandStorageInfo.OperationOpt);	    
	    PHY_ERR("    Access Frequency:     0x%x\n", NandStorageInfo.FrequencePar);
	    PHY_ERR("    ECC Mode:             0x%x\n", NandStorageInfo.EccMode);
		PHY_ERR("    Read Retry Type:      0x%x\n", NandStorageInfo.ReadRetryType);
		PHY_ERR("    DDR Type:             0x%x\n", NandStorageInfo.DDRType);
	    PHY_ERR(" =======================================================\n\n");
	
	    //print nand flash optional operation parameter
	    PHY_ERR(" ==============Optional Operaion Parameter==============\n");
	    PHY_ERR("    MultiPlaneReadCmd:      0x%x, 0x%x\n",
	        NandStorageInfo.OptPhyOpPar.MultiPlaneReadCmd[0],NandStorageInfo.OptPhyOpPar.MultiPlaneReadCmd[1]);
	    PHY_ERR("    MultiPlaneWriteCmd:     0x%x, 0x%x\n",
	        NandStorageInfo.OptPhyOpPar.MultiPlaneWriteCmd[0],NandStorageInfo.OptPhyOpPar.MultiPlaneWriteCmd[1]);
	    PHY_ERR("    MultiPlaneCopyReadCmd:  0x%x, 0x%x, 0x%x\n",
	        NandStorageInfo.OptPhyOpPar.MultiPlaneCopyReadCmd[0],NandStorageInfo.OptPhyOpPar.MultiPlaneCopyReadCmd[1],
	        NandStorageInfo.OptPhyOpPar.MultiPlaneCopyReadCmd[2]);
	    PHY_ERR("    MultiPlaneCopyWriteCmd: 0x%x, 0x%x, 0x%x\n",
	        NandStorageInfo.OptPhyOpPar.MultiPlaneCopyWriteCmd[0], NandStorageInfo.OptPhyOpPar.MultiPlaneCopyWriteCmd[1],
	        NandStorageInfo.OptPhyOpPar.MultiPlaneCopyWriteCmd[2]);
	    PHY_ERR("    MultiPlaneStatusCmd:    0x%x\n", NandStorageInfo.OptPhyOpPar.MultiPlaneStatusCmd);
	    PHY_ERR("    InterBnk0StatusCmd:     0x%x\n", NandStorageInfo.OptPhyOpPar.InterBnk0StatusCmd);
	    PHY_ERR("    InterBnk1StatusCmd:     0x%x\n", NandStorageInfo.OptPhyOpPar.InterBnk1StatusCmd);
	    PHY_ERR("    BadBlockFlagPosition:   0x%x\n", NandStorageInfo.OptPhyOpPar.BadBlockFlagPosition);
	    PHY_ERR("    MultiPlaneBlockOffset:  0x%x\n", NandStorageInfo.OptPhyOpPar.MultiPlaneBlockOffset);
	    PHY_ERR(" =======================================================\n");
	    
		PHY_ERR(" ==============Support Optional Operaion================\n");
		if (SUPPORT_CACHE_READ)
			PHY_ERR("    SUPPORT_CACHE_READ\n");
		if (SUPPORT_CACHE_PROGRAM)
			PHY_ERR("    SUPPORT_CACHE_PROGRAM\n");
		if (SUPPORT_MULTI_READ)
			PHY_ERR("    SUPPORT_MULTI_READ\n");
		if (SUPPORT_MULTI_PROGRAM)
			PHY_ERR("    SUPPORT_MULTI_PROGRAM\n");
		if (SUPPORT_PAGE_COPYBACK)
			PHY_ERR("    SUPPORT_PAGE_COPYBACK\n");
		if (SUPPORT_INT_INTERLEAVE)
			PHY_ERR("    SUPPORT_INT_INTERLEAVE\n");
		if (SUPPORT_EXT_INTERLEAVE)
			PHY_ERR("    SUPPORT_EXT_INTERLEAVE\n");
		if (SUPPORT_RANDOM)
			PHY_ERR("    SUPPORT_RANDOM\n");
		if (SUPPORT_READ_RETRY)
			PHY_ERR("    SUPPORT_READ_RETRY\n");
		if (SUPPORT_READ_UNIQUE_ID)
			PHY_ERR("    SUPPORT_READ_UNIQUE_ID\n");
		if (SUPPORT_ALIGN_NAND_BNK)
			PHY_ERR("    SUPPORT_ALIGN_NAND_BNK\n");
		if (SUPPORT_DIE_SKIP)
			PHY_ERR("    SUPPORT_DIE_SKIP\n");
		if (SUPPORT_WRITE_CHECK_SYNC)
			PHY_ERR("    SUPPORT_WRITE_CHECK_SYNC\n");
		if (SUPPORT_USE_MAX_BLK_ERASE_CNT)
			PHY_ERR("    SUPPORT_USE_MAX_BLK_ERASE_CNT\n");		
		if (SUPPORT_READ_RECLAIM)
			PHY_ERR("    SUPPORT_READ_RECLAIM\n");			
		if (SUPPORT_CHANGE_ONFI_TIMING_MODE)
			PHY_ERR("    SUPPORT_CHANGE_ONFI_TIMING_MODE\n");			
		if (SUPPORT_DDR2_SPECIFIC_CFG)
			PHY_ERR("    SUPPORT_DDR2_SPECIFIC_CFG\n");			
		if (SUPPORT_IO_DRIVER_STRENGTH)
			PHY_ERR("    SUPPORT_IO_DRIVER_STRENGTH\n");			
		if (SUPPORT_VENDOR_SPECIFIC_CFG)
			PHY_ERR("    SUPPORT_VENDOR_SPECIFIC_CFG\n");			
		if (SUPPORT_SYNC_RESET_OP)
			PHY_ERR("    SUPPORT_SYNC_RESET_OP\n");		
		if (SUPPORT_TOGGLE_ONLY)
			PHY_ERR("    SUPPORT_TOGGLE_ONLY\n");								
	    PHY_ERR(" =======================================================\n");	
	    
	    PHY_ERR(" ===========Logical Architecture Parameter===========\n");
	    PHY_ERR("    Page Count of Logic Block:  0x%x\n", LogicArchiPar.PageCntPerLogicBlk);
	    PHY_ERR("    Sector Count of Logic Page: 0x%x\n", LogicArchiPar.SectCntPerLogicPage);
	    PHY_ERR("    Block Count of Die:         0x%x\n", SuperBlkCntOfDie);
	    PHY_ERR("    Die Count:                  0x%x\n", LogicArchiPar.LogicDieCnt);
	    PHY_ERR(" ===================================================\n");
    
	} else {
		PHY_ERR("NandHw_DbgInfo, wrong info type, %d\n", type);
		return -1;
	}
	
	return 0;	
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   -1:fail
*Note         :
*****************************************************************************/
int PHY_SimpleBad(struct boot_physical_param *para)
{
    uchar oob_buf[32];

    para->mainbuf = (void *) PageCachePool.TmpPageCache1;
    para->oobbuf = oob_buf;

    para->page = 0;
    PHY_SimpleRead(para);
    if(oob_buf[0] == 0)
    {
        return 1;
    }

    para->page = 1;
    PHY_SimpleRead(para);
    if(oob_buf[0] == 0)
    {
        return 1;
    }

    para->page = PAGE_CNT_OF_PHY_BLK - 2;
    PHY_SimpleRead(para);
    if(oob_buf[0] == 0)
    {
        return 1;
    }

    para->page = PAGE_CNT_OF_PHY_BLK - 1;
    PHY_SimpleRead(para);
    if(oob_buf[0] == 0)
    {
        return 1;
    }

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
#define NAND_BOOT0_BLK_START    0
#define NAND_BOOT0_BLK_CNT		4
#define NAND_UBOOT_BLK_START    (NAND_BOOT0_BLK_START+NAND_BOOT0_BLK_CNT)
#define NAND_UBOOT_BLK_CNT		30

void print_uboot_info(void)
{
	__u32 i;
    struct _uboot_info* uboot = &uboot_info;
    PHY_ERR("uboot->sys_mode %d \n",uboot->sys_mode);
    PHY_ERR("uboot->use_lsb_page %d \n",uboot->use_lsb_page);
    PHY_ERR("uboot->copys %d \n",uboot->copys);
    PHY_ERR("uboot->uboot_len %d \n",uboot->uboot_len);
    PHY_ERR("uboot->total_len %d \n",uboot->total_len);
    PHY_ERR("uboot->uboot_pages %d \n",uboot->uboot_pages);
    PHY_ERR("uboot->total_pages %d \n",uboot->total_pages);
    PHY_ERR("uboot->blocks_per_total %d \n",uboot->blocks_per_total);
    PHY_ERR("uboot->page_offset_for_nand_info %d \n",uboot->page_offset_for_nand_info);
    //PHY_ERR("uboot->uboot_block %d  %d  %d  %d  %d  %d  %d  %d \n",uboot->uboot_block[0],uboot->uboot_block[1],uboot->uboot_block[2],uboot->uboot_block[3],uboot->uboot_block[4],uboot->uboot_block[5],uboot->uboot_block[6],uboot->uboot_block[7]);
	PHY_ERR("uboot->uboot_block\n");
	for(i=0;i<30;i++)
	{
		PHY_ERR(" %d",uboot->uboot_block[i]);
		if((i%10 == 0) && (i != 0))
			PHY_ERR("\n");
	}
	PHY_ERR("\n");
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
__u32 uboot_info_init(void)
{
    int ret,blcok_offset,block_cnt;
    char * uboot_buf;
	__u32 i;
	__u32 jump_size;
	__u32 size_per_page;
	__u32 lsb_pages;
	char  block_flag[40];
	__u8 flag = 0;
	struct _uboot_info* uboot;

	uboot = &uboot_info;
    size_per_page = SECTOR_CNT_OF_SINGLE_PAGE * 512;
    MEMSET(uboot,0,sizeof(struct _uboot_info));

    uboot_buf = MALLOC(size_per_page);
	if(uboot_buf == NULL)
	{
		PHY_ERR("uboot_buf malloc fail\n");
		return 1;
	}

    uboot->sys_mode = NAND_Get_Secure_Mode();
    uboot->use_lsb_page = NAND_GetLsbpagetype();
    blcok_offset = 0;

    for( i = aw_nand_info.boot->uboot_start_block;  i < aw_nand_info.boot->uboot_next_block;  i++ )
    {
        ret = is_uboot_block(uboot->sys_mode,i,uboot_buf);
		if(ret == 2)
		{
			block_flag[i] = 1;
			continue;
		}
		else
		{
			block_flag[i] = 0;
		}
        if(ret == 1)
        {
            //uboot->uboot_block[blcok_offset] = i;
            if(flag == 0)
            {
                uboot->uboot_len = _get_uboot_offset(uboot->sys_mode, uboot_buf);
                uboot->uboot_pages = uboot->uboot_len / size_per_page;
                if(uboot->uboot_len % size_per_page)
                {
                    uboot->uboot_pages++;
                }
                jump_size = size_per_page*uboot->uboot_pages-uboot->uboot_len;

                uboot->total_len = uboot->uboot_len + jump_size + PHY_INFO_SIZE;
                uboot->total_pages = uboot->total_len / size_per_page;
                uboot->page_offset_for_nand_info = uboot->uboot_pages;
                uboot->byte_offset_for_nand_info = size_per_page*uboot->uboot_pages;

                if(uboot->use_lsb_page == 0)
                {
                    uboot->blocks_per_total = uboot->total_pages / PAGE_CNT_OF_PHY_BLK;
					if(uboot->total_pages % PAGE_CNT_OF_PHY_BLK)
                    {
                        uboot->blocks_per_total++;
                    }
                }
                else
                {
                    lsb_pages = NAND_GetLsbpages();
                    uboot->blocks_per_total = uboot->total_pages / lsb_pages;
                    if(uboot->total_pages % lsb_pages)
                    {
                        uboot->blocks_per_total++;
                    }
                }
				flag = 1;
            }
            //blcok_offset++;
            //uboot->copys++;
            //i += uboot->blocks_per_total - 1;
        }
    }

	block_cnt = 0;
	for( i = aw_nand_info.boot->uboot_start_block;  i < aw_nand_info.boot->uboot_next_block;  i++ )
	{
		if(block_flag[i] == 0)
		{
			if(block_cnt == 0)
			{
				uboot->uboot_block[blcok_offset] = i;
			}
			block_cnt++;
			if(block_cnt == uboot->blocks_per_total)
			{
				block_cnt = 0;
				blcok_offset++;
            	uboot->copys++;
			}
		}
	}
	
    print_uboot_info();

    FREE(uboot_buf,size_per_page);
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int nand_read_uboot_one(unsigned char *buf,unsigned char *sbuf,unsigned int counter)
{
    int ret;
    __u32 i,j, k,ecc_limit,ecc_error;
    __u8  oob_buf[32];
    __u32 pages, page_size,next_block;
    struct boot_physical_param  para;
    __u32 many_block[24];

    page_size = NAND_GetPageSize();
    ecc_limit = 0;
    ecc_error = 0;
    if(uboot_info.blocks_per_total == 1)
    {
        para.chip  = 0;
        para.block = uboot_info.uboot_block[counter];
        if(para.block == 0)
        {
            return 0;
        }
        pages = uboot_info.total_pages;

        //PHY_ERR("read page_size : %d\n", page_size);

        for( i=0,k=0;  k < pages;  i++ )
        {
            para.page  = i;
            if((uboot_info.use_lsb_page != 0) && (Nand_Is_lsb_page(0,i) == 0))
            {
                continue;
            }
            para.mainbuf = (void *) (buf + k * page_size);
            if(k==0)
            {
                para.oobbuf = sbuf;
            }
            else
            {
                para.oobbuf = oob_buf;
            }
            //PHY_ERR("read block : %d page : %d buf :0x%x\n", para.block,i,para.mainbuf);
            ret = PHY_SimpleRead(&para);
            if(ret == -ERR_ECC)
            {
                ecc_error = 1;
				break;
            }
            else if(ret == ECC_LIMIT)
            {
                ecc_limit = 1;
            }
			else if((*((__u8 *)para.oobbuf) != 0xff) || (*((__u8 *)para.oobbuf + 1) != 0x0))
			{
                ecc_error = 1;
				break;
            }
            else
            {
                ;
            }
            k++;
        }
    }
    else
    {
        next_block = uboot_info.uboot_block[counter+1];
        many_block[0] = uboot_info.uboot_block[counter];
        if(many_block[0] == 0)
        {
            return 0;
        }

        for(i=0,k=1; k<uboot_info.blocks_per_total; i++)
        {
            para.chip  = 0;
            para.block = many_block[0] + 1 + i;
            ret = PHY_SimpleBad(&para);
            if(ret == 0)
            {
                many_block[k] = para.block;
                if(next_block != 0)
                {
                    if(next_block == para.block)
                    {
                        return  -1;
                    }
                }
                k++;
            }
        }

        ret = is_uboot_block(uboot_info.sys_mode,many_block[0],(char*)buf);
        if(ret != 1)
        {
            return  -1;
        }

        pages = uboot_info.total_pages;
        k = 0;
        for(i=0;i<uboot_info.blocks_per_total;i++)
        {
            para.chip  = 0;
            para.block = many_block[i];

            for(j=0;j<PAGE_CNT_OF_PHY_BLK;j++)
            {
                para.page  = j;
                if((uboot_info.use_lsb_page != 0) && (Nand_Is_lsb_page(0,j) == 0))
                {
                    continue;
                }
                para.mainbuf = (void *) (buf + k * page_size);
                if(k==0)
                {
                    para.oobbuf = sbuf;
                }
                else
                {
                    para.oobbuf = oob_buf;
                }

                ret = PHY_SimpleRead(&para);
                if(ret == -ERR_ECC)
                {
                    ecc_error = 1;
					break;
                }
                else if(ret == ECC_LIMIT)
                {
                    ecc_limit = 1;
                }
				else if((*((__u8 *)para.oobbuf) != 0xff) || (*((__u8 *)para.oobbuf + 1) != 0x0))
				{
	                ecc_error = 1;
					break;
	            }
                else
                {
                    ;
                }
                k++;
                if(k == uboot_info.total_pages)
                {
                    goto read_uboot_one_end;
                }
            }
			if(ecc_error == 1)
				break;
        }
    }
read_uboot_one_end:

    if(ecc_error  != 0)
    {
        PHY_ERR("nand_read_uboot_one ecc_error %d \n",counter);
        return -1;
    }

    if(ecc_limit  != 0)
    {
        PHY_ERR("nand_read_uboot_one ecc_limit %d \n",counter);
        return 1;
    }

    PHY_ERR("nand_read_uboot_one ok %d \n",counter);
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_uboot_one(unsigned char *buf,unsigned char *sbuf,unsigned int counter)
{
    int ret;
    __u32 i,j, k;
    __u32 page_size,next_block;
    struct boot_physical_param  para;
    __u32 many_block[24];

    PHY_ERR("nand_write_uboot_one %d \n",counter);

    page_size = NAND_GetPageSize();
    if(uboot_info.blocks_per_total == 1)
    {
        para.chip  = 0;
        para.block = uboot_info.uboot_block[counter];
        if(para.block == 0)
        {
            return 0;
        }

		if(PHY_SimpleErase(&para ) < 0)
		{
		    PHY_ERR("Fail  in erasing block %d.\n", para.block );
    	}

        for( i=0,k=0;  i < PAGE_CNT_OF_PHY_BLK;  i++ )
        {
            para.page  = i;
            if((uboot_info.use_lsb_page != 0) && (Nand_Is_lsb_page(0,i) == 0))
            {
                continue;
            }
            para.mainbuf = (void *) (buf + k * page_size);
            para.oobbuf = sbuf;
            //PHY_ERR("write block : %d page : %d buf :0x%x\n", para.block,i,para.mainbuf);
            ret = PHY_SimpleWrite(&para);
            if(ret < 0)
            {
                PHY_ERR("Warning.    Fail in writing page %d in block %d.\n", para.page, para.block);
            }
            k++;
        }
    }
    else
    {
        next_block = uboot_info.uboot_block[counter+1];
        many_block[0] = uboot_info.uboot_block[counter];
        for(i=0,k=1; k<uboot_info.blocks_per_total; i++)
        {
            para.chip  = 0;
            para.block = many_block[0] + 1 + i;
            ret = PHY_SimpleBad(&para);
            if(ret == 0)
            {
                many_block[k] = para.block;
                if(next_block != 0)
                {
                    if(next_block == para.block)
                    {
                        return  -1;
                    }
                }
                k++;
            }
        }

		if(many_block[k-1] >= aw_nand_info.boot->uboot_next_block)
		{
			PHY_ERR("the copy has not enough blocks\n");
			goto write_uboot_one_end;
		}

        k = 0;
        for(i=0;i<uboot_info.blocks_per_total;i++)
        {
            para.chip  = 0;
            para.block = many_block[i];
		    if(PHY_SimpleErase(&para ) < 0)
		    {
		        PHY_ERR("Fail  in erasing block %d.\n", para.block );
    	    }

            for(j=0;j<PAGE_CNT_OF_PHY_BLK;j++)
            {
                para.page  = j;
                if((uboot_info.use_lsb_page != 0) && (Nand_Is_lsb_page(0,j) == 0))
                {
                    continue;
                }
                para.mainbuf = (void *) (buf + k * page_size);
                para.oobbuf = sbuf;
                ret = PHY_SimpleWrite(&para);
                if(ret < 0)
                {
                    PHY_ERR("Warning.    Fail in writing page %d in block %d.\n", para.page, para.block);
                }
                k++;
                if(k == uboot_info.total_pages)
                {
                    goto write_uboot_one_end;
                }
            }
        }
    }
write_uboot_one_end:

    return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_clean_uboot_one(unsigned char *buf,unsigned char *sbuf,unsigned int counter)
{
    int ret;
    __u32 i,k;
    __u32 next_block;
    struct boot_physical_param  para;
    __u32 many_block[24];

    PHY_ERR("nand_clean_uboot_one %d \n",counter);

    if(uboot_info.blocks_per_total == 1)
    {
        para.chip  = 0;
        para.block = uboot_info.uboot_block[counter];
        if(para.block == 0)
        {
            return 0;
        }

		if(PHY_SimpleErase(&para ) < 0)
		{
		    PHY_ERR("Fail  in erasing block %d.\n", para.block );
    	}
    }
    else
    {
        next_block = uboot_info.uboot_block[counter+1];
        many_block[0] = uboot_info.uboot_block[counter];
        for(i=0,k=1; k<uboot_info.blocks_per_total; i++)
        {
            para.chip  = 0;
            para.block = many_block[0] + 1 + i;
            ret = PHY_SimpleBad(&para);
            if(ret == 0)
            {
                many_block[k] = para.block;
                if(next_block != 0)
                {
                    if(next_block == para.block)
                    {
                        return  -1;
                    }
                }
                k++;
            }
        }

        k = 0;
        for(i=0;i<uboot_info.blocks_per_total;i++)
        {
            para.chip  = 0;
            para.block = many_block[i];
		    if(PHY_SimpleErase(&para ) < 0)
		    {
		        PHY_ERR("Fail  in erasing block %d.\n", para.block );
    	    }
        }
    }

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_check_uboot(unsigned char *buf,unsigned char *sbuf)
{
    int i;
    int num = 0;
    int counter,first_boot_flag;
    int flag[30];
    int boot_flag[30];
    struct _boot_info * boot;

    counter = uboot_info.copys;

    for(i=0; i<counter; i++)
    {
        boot_flag[i] = -1;
        flag[i] = nand_read_uboot_one(buf,sbuf,i);
        if(flag[i] != -1)
        {
            boot = (struct _boot_info*)(buf + uboot_info.byte_offset_for_nand_info);
            if(boot->magic == PHY_INFO_MAGIC)
            {
                boot_flag[i] = 0;
            }
			else
            {
                PHY_ERR("boot magic error %x\n",boot->magic);
            }
        }
    }

    if(flag[counter-1] == -1)
    {
        for(i=0; i<counter; i++)
        {
            if(flag[i] != -1)
            {
                if(nand_read_uboot_one(buf,sbuf,i) == 0)
                {
                    num = 1;
                    break;
                }
            }
        }
        if(num == 0)
        {
        	PHY_ERR("no uboot ok\n");
            return -1;
        }
    }

    first_boot_flag = 1;
    for(i=0; i<counter; i++)
    {
        if(boot_flag[i] == 0)
        {
            first_boot_flag = 0;
        }
    }
    if(first_boot_flag == 1)
    {
        PHY_ERR("first boot flag\n");
        boot = (struct _boot_info*)(buf + uboot_info.byte_offset_for_nand_info);
        if(boot->magic != PHY_INFO_MAGIC)
        {
            PHY_ERR("recover boot data\n");
            _cal_sum_physic_info((__u32*)aw_nand_info.boot,PHY_INFO_SIZE);
            MEMCPY(boot,aw_nand_info.boot,PHY_INFO_SIZE);
        }
    }

    for(i=0; i<(counter/2); i++)//if error; change
    {
        if((flag[i] == -1) || (first_boot_flag == 1))
        {
            PHY_ERR("recover1  uboot %d.\n", i);
            nand_write_uboot_one(buf,sbuf,i);
        }
		else if((boot_flag[i] < 0) && (first_boot_flag == 0))
        {
            PHY_ERR("recover2  uboot %d.\n", i);
			nand_write_uboot_one(buf,sbuf,i);
        }
        else
        {
            ;
        }
    }

    for(i=(counter/2); i<counter; i++)//if ecc limit ; change
    {
        if((flag[i] != 0) || (first_boot_flag == 1))
        {
            PHY_ERR("recover3  uboot %d.\n", i);
            nand_write_uboot_one(buf,sbuf,i);
        }
		else if((boot_flag[i] < 0) && (first_boot_flag == 0))
        {
            PHY_ERR("recover4  uboot %d.\n", i);
            nand_write_uboot_one(buf,sbuf,i);
        }
        else
        {
            ;
        }
    }

    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int nand_read_nboot_one(unsigned char *buf,unsigned int len,unsigned int counter)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_nboot_one(unsigned char *buf,unsigned int len,unsigned int counter)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
#define NAND_NBOOT_BLK_CNT 2
int nand_check_nboot(unsigned char *buf,unsigned int len)
{
    int i;
    int num = 0;
    int flag[NAND_NBOOT_BLK_CNT];

    for(i=0; i<NAND_NBOOT_BLK_CNT; i++)
    {
        flag[i] = nand_read_nboot_one(buf,len,i);
    }

    if(flag[NAND_NBOOT_BLK_CNT-1] == -1)
    {
        for(i=0; i<NAND_NBOOT_BLK_CNT; i++)
        {
            if(flag[i] != -1)
            {
                if(nand_read_nboot_one(buf,len,i) == 0)
                {
                    num = 1;
                    break;
                }
            }
        }
        if(num == 0)
        {
            return -1;
        }
    }

    for(i=0; i<(NAND_NBOOT_BLK_CNT/2); i++)
    {
        if(flag[i] == -1)
        {
            nand_write_nboot_one(buf,len,i);
        }
    }

    for(i=(NAND_NBOOT_BLK_CNT/2); i<NAND_NBOOT_BLK_CNT; i++)
    {
        if(flag[i] != 0)
        {
            nand_write_nboot_one(buf,len,i);
        }
    }
    return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void check_phyisc(void)
{
    __u32 len;
    char* buf;
    char  sbuf[64];

    PHY_ERR(" check_phyisc start.\n");

    uboot_info_init();

    len = uboot_info.total_len;
	if(len == 0)
    {
        PHY_ERR("error uboot!\n");
        return;
	}
    buf = MALLOC(len);
    if(buf == NULL)
    {
        PHY_ERR("no space.\n");
        return;
    }
	
    nand_check_uboot((uchar*)buf,(uchar*)sbuf);

    PHY_ERR(" check_phyisc end.\n");

    FREE(buf,len);

}
