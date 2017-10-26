#ifndef NAND_PHYSIC_INTERFACE_H
#define NAND_PHYSIC_INTERFACE_H

//struct physic_nand_info{
//	unsigned short                  type;
//    unsigned short                  SectorNumsPerPage;
//    unsigned short                  BytesUserData;
//    unsigned short                  PageNumsPerBlk;
//	unsigned short                  BlkPerChip;
//    unsigned short                  ChipNum;
//	__u64                           FullBitmap;
//};

#define PHYSIC_RECV_BLOCK    6

typedef struct 
{
	__u32		ChannelCnt;
	__u32        ChipCnt;                            //the count of the total nand flash chips are currently connecting on the CE pin
    __u32       ChipConnectInfo;                    //chip connect information, bit == 1 means there is a chip connecting on the CE pin
	__u32		RbCnt;
	__u32		RbConnectInfo;						//the connect  information of the all rb  chips are connected
    __u32        RbConnectMode;						//the rb connect  mode
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
	__u32 		good_block_ratio;					//good block ratio get from hwscan
	__u32		ReadRetryType;						//the read retry type
	__u32       DDRType;
	__u32		uboot_start_block;
	__u32		uboot_next_block;
	__u32		logic_start_block_by_single;
	__u32		nand_specialinfo_page;
	__u32		nand_specialinfo_offset;
	__u32		Reserved[17];
}boot_nand_para_t;

typedef struct boot_flash_info{
	__u32 chip_cnt;
	__u32 blk_cnt_per_chip;
	__u32 blocksize;
	__u32 pagesize;
	__u32 pagewithbadflag; /*bad block flag was written at the first byte of spare area of this page*/
}boot_flash_info_t;

struct boot_physical_param {
	__u32   chip; //chip no
	__u32  block; // block no within chip
	__u32  page; // apge no within block
	__u32  sectorbitmap; //done't care
	void   *mainbuf; //data buf
	void   *oobbuf; //oob buf
};

struct boot_ndfc_cfg {
	__u8 page_size_kb;
	__u8 ecc_mode;
	__u8 sequence_mode;
	__u8 res[5];
};

//for simple nand operation (boot0)
extern __s32 PHY_SimpleErase(struct boot_physical_param * eraseop);
extern __s32 PHY_SimpleErase_2CH(struct boot_physical_param * eraseop);
extern __s32 PHY_SimpleErase_CurCH(struct boot_physical_param * eraseop);
extern __s32 PHY_SimpleRead(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_CurCH(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_2CH(struct boot_physical_param *readop);
extern __s32 PHY_SimpleWrite(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_CFG(struct boot_physical_param * writeop, struct boot_ndfc_cfg *ndfc_cfg);
extern __s32 PHY_SimpleWrite_CurCH(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_1K(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_1KCurCH(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_Seq(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleRead_Seq(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_CFG(struct boot_physical_param * readop, struct boot_ndfc_cfg *ndfc_cfg);
extern __s32 PHY_SimpleRead_1K(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_1KCurCH(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_1st1K(struct boot_physical_param * readop);
extern __s32 PHY_SimpleRead_1st1KCurCH(struct boot_physical_param * readop);
extern __s32 PHY_SimpleWrite_Seq_16K(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleRead_Seq_16K(struct boot_physical_param * readop);
extern __s32 PHY_SimpleWrite_0xFF(struct boot_physical_param * writeop);
extern __s32 PHY_SimpleWrite_0xFF_8K(struct boot_physical_param * writeop);
extern __s32  PHY_SimpleCheckBadBlock_Sandisk_2CH(struct boot_physical_param *checkop);


extern __s32 NFC_LSBEnable(__u32 chip, __u32 read_retry_type);
extern __s32 NFC_LSBDisable(__u32 chip, __u32 read_retry_type);
extern __s32 NFC_LSBInit(__u32 read_retry_type);
extern __s32 NFC_LSBExit(__u32 read_retry_type);
extern  __u32 NAND_GetChannelCnt(void);

//for param get&set
extern __u32 NAND_GetFrequencePar(void);
extern __s32 NAND_SetFrequencePar(__u32 FrequencePar);
extern __u32 NAND_GetNandVersion(void);
extern __s32 NAND_GetParam(boot_nand_para_t * nand_param);
extern __s32 NAND_GetFlashInfo(boot_flash_info_t *info);


//general operation based no nand super block
struct _nand_info* NandHwInit(void);
//void NandHwInit(void);
__s32 NandHwExit(void);
__s32 NandHwSuperStandby(void);
__s32 NandHwSuperResume(void);
__s32 NandHwNormalStandby(void);
__s32 NandHwNormalResume(void);
__s32 NandHwShutDown(void);
__s32 NandHw_DbgInfo(__u32 type);

__s32 PHY_WaitAllRbReady(void);
__s32 PHY_VirtualPageRead(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare);
__s32 PHY_VirtualPageWrite(__u32 nDieNum, __u32 nBlkNum, __u32 nPage, __u64 SectBitmap, void *pBuf, void *pSpare);
__s32 PHY_VirtualBlockErase(__u32 nDieNum, __u32 nBlkNum);
__s32 PHY_VirtualBadBlockCheck(__u32 nDieNum, __u32 nBlkNum);
__s32 PHY_VirtualBadBlockMark(__u32 nDieNum, __u32 nBlkNum);

#endif /*NAND_PHYSIC_INTERFACE_H*/
