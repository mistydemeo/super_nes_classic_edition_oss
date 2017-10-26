/*
************************************************************************************************************************
*                                                      eNand
*                                           Nand flash driver scan module
*
*                             Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
*                                                  All Rights Reserved
*
* File Name : nand_id.c
*
* Author : Kevin.z
*
* Version : v0.1
*
* Date : 2008.03.27
*
* Description : This file is a table, that record the physical architecture parameter for
*               every kind of nand flash, and indexed by the nand chip ID.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Kevin.z         2008.03.27      0.1          build the file
* penggang        2009.09.09      0.2          modify the file
*
************************************************************************************************************************
*/

#include "nand_scan.h"

//==============================================================================
// define the optional operation parameter for different kindes of nand flash
//==============================================================================

//the physical architecture parameter for Samsung 2K page SLC nand flash
static struct __OptionalPhyOpPar_t PhysicArchiPara0 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x81},           //multi-plane program command
    {0x00, 0x00, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x81},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0xf1,                   //inter-leave bank0 operation status read command
    0xf2,                   //inter-leave bank1 operation status read command
    0x01,                   //bad block flag position, in the fist 2 page
    1                       //multi-plane block address offset
};

//the physical architecture parameter for Samsung 4K page SLC nand flash
static struct __OptionalPhyOpPar_t PhysicArchiPara1 =
{
    {0x60, 0x30},           //multi-plane read command
    {0x11, 0x81},           //multi-plane program command
    {0x60, 0x60, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x81},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0xf1,                   //inter-leave bank0 operation status read command
    0xf2,                   //inter-leave bank1 operation status read command
    0x00,                   //bad block flag position, in the fist page
    1                       //multi-plane block address offset
};

//the physical architecture parameter for Samsung 2K page MLC nand flash
static struct __OptionalPhyOpPar_t PhysicArchiPara2 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x81},           //multi-plane program command
    {0x00, 0x00, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x81},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0xf1,                   //inter-leave bank0 operation status read command
    0xf2,                   //inter-leave bank1 operation status read command
    0x02,                   //bad block flag position, in the last page
    1                       //multi-plane block address offset
};

//the physical architecture parameter for Samsung 4K page MLC nand flash
static struct __OptionalPhyOpPar_t PhysicArchiPara3 =
{
    {0x60, 0x60},           //multi-plane read command
    {0x11, 0x81},           //multi-plane program command
    {0x60, 0x60, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x81},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0xf1,                   //inter-leave bank0 operation status read command
    0xf2,                   //inter-leave bank1 operation status read command
    0x02,                   //bad block flag position, in the last page
    1                       //multi-plane block address offset
};

//the physical architecture parameter for Micon nand flash
static struct __OptionalPhyOpPar_t PhysicArchiPara4 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x80},           //multi-plane program command
    {0x00, 0x00, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x80},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0x78,                   //inter-leave bank0 operation status read command
    0x78,                   //inter-leave bank1 operation status read command
    0x01,                   //bad block flag position, in the fist 2 page
    1                       //multi-plane block address offset
};

//the physical architecture parameter for Toshiba SLC nand flash
static struct __OptionalPhyOpPar_t PhysicArchiPara5 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x80},           //multi-plane program command
    {0x00, 0x00, 0x30},     //multi-plane page copy-back read command
    {0x8c, 0x11, 0x8c},     //multi-plane page copy-back program command
    0x71,                   //multi-plane operation status read command
    0x70,                   //inter-leave bank0 operation status read command
    0x70,                   //inter-leave bank1 operation status read command
    0x00,                   //bad block flag position, in the fist page
    0                       //multi-plane block address offset
};

//the physical architecture parameter for Toshiba MLC nand flash which multi-plane offset is 1024
static struct __OptionalPhyOpPar_t PhysicArchiPara6 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x80},           //multi-plane program command
    {0x00, 0x00, 0x30},     //multi-plane page copy-back read command
    {0x8c, 0x11, 0x8c},     //multi-plane page copy-back program command
    0x71,                   //multi-plane operation status read command
    0x70,                   //inter-leave bank0 operation status read command
    0x70,                   //inter-leave bank1 operation status read command
    0x00,                   //bad block flag position, in the fist page
    1024                    //multi-plane block address offset
};

//the physical architecture parameter for Toshiba MLC nand flash which multi-plane offset is 2048
static struct __OptionalPhyOpPar_t PhysicArchiPara7 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x80},           //multi-plane program command
    {0x00, 0x00, 0x30},     //multi-plane page copy-back read command
    {0x8c, 0x11, 0x8c},     //multi-plane page copy-back program command
    0x71,                   //multi-plane operation status read command
    0x70,                   //inter-leave bank0 operation status read command
    0x70,                   //inter-leave bank1 operation status read command
    0x00,                   //bad block flag position, in the fist page
    2048                    //multi-plane block address offset
};

static struct __OptionalPhyOpPar_t PhysicArchiPara8 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x80},           //multi-plane program command
    {0x00, 0x00, 0x30},     //multi-plane page copy-back read command
    {0x8c, 0x11, 0x8c},     //multi-plane page copy-back program command
    0x71,                   //multi-plane operation status read command
    0x70,                   //inter-leave bank0 operation status read command
    0x70,                   //inter-leave bank1 operation status read command
    0x02,                   //bad block flag position, in the last page
    1                       //multi-plane block address offset
};

static struct __OptionalPhyOpPar_t PhysicArchiPara9 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x81},           //multi-plane program command
    {0x00, 0x00, 0x30},     //multi-plane page copy-back read command
    {0x8c, 0x11, 0x8c},     //multi-plane page copy-back program command
    0x71,                   //multi-plane operation status read command
    0x70,                   //inter-leave bank0 operation status read command
    0x70,                   //inter-leave bank1 operation status read command
    0x02,                   //bad block flag position, in the last page
    1                       //multi-plane block address offset
};

//the physical architecture parameter for nand flash that do not support multi_plane
//note:need to change multi_plane erase too
static struct __OptionalPhyOpPar_t PhysicArchiPara10 =
{
    {0x00, 0x30},           //multi-plane read command
    {0x10, 0x80},           //multi-plane program command
    {0x00, 0x00, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x80},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0x70,                   //inter-leave bank0 operation status read command
    0x70,                   //inter-leave bank1 operation status read command
    0x02,                   //bad block flag position, in the fist 2 page
    1                       //multi-plane block address offset
};


static struct __OptionalPhyOpPar_t DefualtPhysicArchiPara =
{
    {0x00, 0x30},           //multi-plane read command
    {0x11, 0x81},           //multi-plane program command
    {0x00, 0x00, 0x35},     //multi-plane page copy-back read command
    {0x85, 0x11, 0x81},     //multi-plane page copy-back program command
    0x70,                   //multi-plane operation status read command
    0xf1,                   //inter-leave bank0 operation status read command
    0xf2,                   //inter-leave bank1 operation status read command
    0x00,                   //bad block flag position, in the fist 2 page
    1                       //multi-plane block address offset
};

//==============================================================================
// define the ddr parameter for different kindes of nand flash
//==============================================================================
static struct __NfcInitDdrInfo DefDDRInfo = {
    0, //en_dqs_c;
    0, //en_re_c;
    0, //odt;
    0, //en_ext_verf;
    0, //dout_re_warmup_cycle;
    0, //din_dqs_warmup_cycle;
    0, //output_driver_strength;
    0, //rb_pull_down_strength;
} ;

static struct __NfcInitDdrInfo DDRInfo1 = {
    0, //en_dqs_c;
    0, //en_re_c;
    0, //odt;
    0, //en_ext_verf;
    0, //dout_re_warmup_cycle;
    0, //din_dqs_warmup_cycle;
    2, //output_driver_strength;
    2, //rb_pull_down_strength;
} ;

/*
    LSB Page Type: OpOpt[19:16]  (.OperationOpt)
    0x0 : hynix 26nm, 20nm; micron 256 page;
    0x1 : samsung & toshiba 128 page;
    0x2 : micron 20nm, 256 page; (20nm)
    0x3 : hynix 16nm, 128 page per block
    0x4 : hynix 16nm, 256 page per block
    0x5 : micrion l85a, 512 page per block(20nm)
    0x6 : micrion l95b, 512 page per block(16nm)
    0x6~0xF : Reserved
*/

//==============================================================================
// define the physical architecture parameter for all kinds of nand flash
//==============================================================================

//==============================================================================
//============================ SAMSUNG NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t SamsungNandTbl[] =
{
    //                NAND_CHIP_ID                     DieCnt SecCnt  PagCnt   BlkCnt      OpOpt    DatBlk  Freq   EccMode ReadRetry  DDRType    OperationPar       DDRInfo    
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xec, 0xf1, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000000 ,20000},   // K9F1G08
    { {0xec, 0xf1, 0x00, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000001 ,20000},   // K9F1G08
    { {0xec, 0xda, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     1024,   0x00000000,   896,    15,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000002 ,20000},   // K9K2G08
    { {0xec, 0xda, 0x10, 0x95, 0x44, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x0000008c,   896,    30,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000003 ,60000},   // K9F2G08
    { {0xec, 0xdc, 0xc1, 0x15, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     2048,   0x00000000,   896,    15,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000004 ,20000},   // K9K4G08
    { {0xec, 0xdc, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000008,   896,    15,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000005 ,20000},   // K9F4G08
    { {0xec, 0xd3, 0x51, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000008,   896,    30,     0,       0,        0,     &PhysicArchiPara0, &DefDDRInfo,  0x000006 ,20000},   // K9K8G08
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                
    { {0xec, 0xd3, 0x50, 0xa6, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000018,   896,    30,     0,       0,        0,     &PhysicArchiPara1, &DefDDRInfo,  0x000007 ,10000},   // K9F8G08
    { {0xec, 0xd5, 0x51, 0xa6, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000038,   896,    30,     0,       0,        0,     &PhysicArchiPara1, &DefDDRInfo,  0x000008 ,10000},   // K9KAG08
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------                               
    { {0xec, 0xdc, 0x14, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000008,   896,    20,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x000009 ,10000},   // K9G4G08
    { {0xec, 0xdc, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000008,   896,    30,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x00000a ,10000},   // K9G4G08
    { {0xec, 0xd3, 0x55, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000008,   896,    20,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x00000b ,10000},   // K9L8G08
    { {0xec, 0xd3, 0x55, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000008,   896,    30,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x00000c ,10000},   // K9L8G08
    { {0xec, 0xd3, 0x14, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    20,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x00000d ,10000},   // K9G8G08
    { {0xec, 0xd3, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    30,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x00000e ,10000},   // K9G8G08
    { {0xec, 0xd5, 0x55, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000028,   896,    30,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x00000f ,10000},   // K9LAG08
    { {0xec, 0xd5, 0x55, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000028,   896,    30,     0,       0,        0,     &PhysicArchiPara2, &DefDDRInfo,  0x000010 ,10000},   // K9LAG08
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------                               
    { {0xec, 0xd5, 0x14, 0xb6, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   896,    30,     0,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000011 ,10000},   // K9GAG08
    { {0xec, 0xd7, 0x55, 0xb6, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000028,   896,    30,     0,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000012 ,10000},   // K9LBG08
    { {0xec, 0xd7, 0xd5, 0x29, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000028,   896,    30,     0,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000013 ,10000},   // K9LBG08
    { {0xec, 0xd7, 0x94, 0x72, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000008,   896,    30,     2,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000014 ,5000},   // K9GBG08
    { {0xec, 0xd5, 0x98, 0x71, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     2048,   0x00000008,   896,    30,     3,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000015 ,4000},   // K9AAG08
                                                                                                                                                                         
    { {0xec, 0xd5, 0x94, 0x29, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   896,    30,     0,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000016 ,10000 },   // K9GAG08U0D
    { {0xec, 0xd5, 0x84, 0x72, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     2048,   0x00011000,   896,    24,     2,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000017 ,5000},   // K9GAG08U0E
    { {0xec, 0xd5, 0x94, 0x76, 0x54, 0xff, 0xff, 0xff }, 1,    16,     128,     2048,   0x00011408,   896,    30,     2,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000018 ,5000},   // K9GAG08U0E
    { {0xec, 0xd3, 0x84, 0x72, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     1024,   0x00000000,   896,    24,     2,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x000019 ,5000},   // K9G8G08U0C
    { {0xec, 0xd7, 0x94, 0x76, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00011088,   896,    30,     3,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x00001a ,4000},   // K9GBG08U0A
    { {0xec, 0xd7, 0x94, 0x7A, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00011088,   896,    30,     3,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x00001b ,4000},   // K9GBG08U0A
    { {0xec, 0xde, 0xd5, 0x7A, 0x58, 0xff, 0xff, 0xff }, 2,    16,     128,     4096,   0x00011888,   896,    30,     3,       0,        0,     &PhysicArchiPara3, &DefDDRInfo,  0x00001c ,4000},   // K9LCG08U0A

    { {0xec, 0xd7, 0x94, 0x7A, 0x54, 0xc3, 0xff, 0xff }, 1,    16,     128,     4096,   0x20000088,   896,    60,     1,       0,        3,     &PhysicArchiPara3, &DefDDRInfo,  0x00001d ,8000},   // toogle nand 1.0
    { {0xec, 0xde, 0xa4, 0x7a, 0x68, 0xc4, 0xff, 0xff }, 1,    16,     128,     8192,   0x20000588,   896,    60,     4,   0x200e04,     3,     &PhysicArchiPara3, &DefDDRInfo,  0x00001e ,3500},   // toogle nand 2.0 K9GCGD8U0A
    { {0xec, 0xd7, 0x94, 0x7E, 0x64, 0xc4, 0xff, 0xff }, 1,    16,     128,     4096,   0x20000588,   896,    60,     4,   0x200e04,     3,     &PhysicArchiPara3, &DefDDRInfo,  0x00001f ,3500},   // toogle nand 2.0 K9GBGD8U0B
    { {0xec, 0xd7, 0x94, 0x7e, 0x64, 0x44, 0xff, 0xff }, 1,    16,     128,     4096,   0x00011188,   896,    40,     4,   0x200e04,     0,     &PhysicArchiPara3, &DefDDRInfo,  0x000020 ,3500},   // 21nm sdr K9GBG08U0B
    { {0xec, 0xde, 0xd5, 0x7e, 0x68, 0x44, 0xff, 0xff }, 2,    16,     128,     4096,   0x00011188,   896,    40,     4,   0x200e04,     0,     &PhysicArchiPara3, &DefDDRInfo,  0x000021 ,3500},   // 21nm sdr K9LCG08U0B
    { {0xec, 0xd7, 0x14, 0x76, 0x54, 0xc2, 0xff, 0xff }, 1,    16,     128,     4096,   0x20011088,   896,    30,     3,       0,        3,     &PhysicArchiPara3, &DefDDRInfo,  0x000022 ,4000},   // K9GBGD8U0M
    { {0xec, 0xda, 0x10, 0x95, 0x44, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,     0x000088,   896,    25,     0,       0,        0,     &PhysicArchiPara10,&DefDDRInfo,  0x000023 ,20000},   // K9F2G08U0C 
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,       0,        0,             0,              0,       0xffffff },   // NULL
};


//==============================================================================
//============================= HYNIX NAND FLASH ===============================
//==============================================================================
struct __NandPhyInfoPar_t HynixNandTbl[] =
{
    //                NAND_CHIP_ID                     DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo  
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xad, 0xf1, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010000 ,20000},   // HY27UF081G2M
    { {0xad, 0xf1, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010001 ,20000},   // HY27UF081G2A
    { {0xad, 0xf1, 0x00, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010002 ,20000},   // H27U1G8F2B
    { {0xad, 0xda, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    15,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010003 ,20000},   // HY27UF082G2M
    { {0xad, 0xda, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010004 ,20000},   // HY27UF082G2A
    { {0xad, 0xda, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010005 ,20000},   // HY27UF082G2B
    { {0xad, 0xdc, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 4,     4,      64,     1024,   0x00000000,   896,    15,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010006 ,20000},   // HY27UH084G2M
    { {0xad, 0xdc, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010007 ,20000},   // HY27UF084G2M, HY27UG088G5M
    { {0xad, 0xdc, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000008,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010008 ,20000},   // HY27UF084G2B, HY27UG088G5B
    { {0xad, 0xd3, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 4,     4,      64,     2048,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x010009 ,20000},   // HY27UG084G2M, HY27H088G2M
    { {0xad, 0xd3, 0xc1, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000000,   896,    20,     0,      0,         0,    &PhysicArchiPara0, &DefDDRInfo,  0x01000a ,20000},   // HY27UG088G2M, HY27UH08AG5M
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------                                      
    { {0xad, 0xdc, 0x84, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000000,   896,    12,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x01000b ,10000},   // HY27UT084G2M, HY27UU088G5M
    { {0xad, 0xdc, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000008,   896,    15,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x01000c ,10000},   // HY27U4G8T2BTR
    { {0xad, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000000,   896,    10,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x01000d ,10000},   // HY27UV08AG5M, HY27UW08BGFM
    { {0xad, 0xd3, 0x14, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    12,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x01000e ,10000},   // HY27UT088G2M, HY27UU08AG5M
    { {0xad, 0xd3, 0x14, 0x2d, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    25,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x01000f ,10000},   // HY27UT088G2M, HY27UU08AG5M
    { {0xad, 0xd3, 0x14, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    15,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x010010 ,10000},   // HY27UT088G2M, HY27UU08AG5M
    { {0xad, 0xd5, 0x55, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000008,   896,    15,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x010011 ,10000},   // HY27UV08BG5M, HY27UW08CGFM
    { {0xad, 0xd5, 0x55, 0x2d, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000008,   896,    25,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x010012 ,10000},   // HY27UV08BG5M, HY27UW08CGFM
    { {0xad, 0xd5, 0x55, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     8192,   0x00000008,   896,    30,     0,      0,         0,    &PhysicArchiPara2, &DefDDRInfo,  0x010013 ,10000},   // HY27UV08BG5M, HY27UW08CGFM
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------                                    
    { {0xad, 0xd3, 0x14, 0xb6, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     2048,   0x00000008,   896,    30,     0,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x010014 ,8000},    // H27U8G8T2B
    { {0xad, 0xd5, 0x14, 0xb6, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   896,    30,     0,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x010015 ,8000},    // H27UAG8T2M, H27UBG8U5M
    { {0xad, 0xd7, 0x55, 0xb6, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000008,   896,    30,     0,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x010016 ,8000},    // H27UCG8V5M
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------                                                 
    { {0xad, 0xd5, 0x94, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x010017 ,5000},    // H27UBG8U5A
    { {0xad, 0xd7, 0x95, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000008,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x010018 ,5000},    // H27UCG8V5A
    { {0xad, 0xd5, 0x95, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000008,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x010019 ,5000},    // H27UCG8VFA
    { {0xad, 0xd5, 0x94, 0x9A, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     1024,   0x00001000,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x01001a ,5000},   // H27UAG8T2B
    { {0xad, 0xd7, 0x94, 0x9A, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     2048,   0x00001008,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x01001b ,5000},   // H27UBG8T2A H27UCG8U5(D)A H27UDG8VF(D)A
    { {0xad, 0xde, 0xd5, 0x9A, 0xff, 0xff, 0xff, 0xff }, 2,    16,     256,     2048,   0x00001008,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x01001c ,5000},   // H27UDG8V5A
    { {0xad, 0xd7, 0x94, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00001008,   896,    30,     2,      0,         0,    &PhysicArchiPara3, &DefDDRInfo,  0x01001d ,5000},   // H27UBG8T2M
    { {0xad, 0xde, 0x94, 0xd2, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00001188,   896,    30,     2,  0x000604,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x01001e ,5000},   // H27UCG8T2M
    { {0xad, 0xd7, 0x94, 0xda, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     2048,   0x00001188,   896,    30,     3,  0x010604,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x01001f ,4000},   // H27UBG8T2B
    { {0xad, 0xde, 0x94, 0xda, 0x74, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00003188,   896,    40,     4,  0x020708,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010020 ,2000},   // H27UCG8T2A
    { {0xad, 0xd7, 0x94, 0x91, 0x60, 0xff, 0xff, 0xff }, 1,    16,     256,     2048,   0x00003188,   896,    40,     4,  0x030708,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010021 ,2000},   // H27UBG8T2C
    { {0xad, 0xde, 0x94, 0xeb, 0x74, 0xff, 0xff, 0xff }, 1,    32,     256,     2048,   0x00003188,   896,    40,     4,  0x030708,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010022 ,2000},   // H27UCG8T2B 20nm 8G
    { {0xad, 0xde, 0x14, 0xa7, 0x42, 0x4a, 0xff, 0xff }, 1,    32,     256,     2048,   0x00003188,   896,    40,     5,  0x040704,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010023 ,2000},   // H27UCG8T2ETR 16nm 8G
    { {0xad, 0xd7, 0x14, 0x9e, 0x34, 0x4a, 0xff, 0xff }, 1,    16,     256,     2048,   0x00003188,   896,    40,     5,  0x040704,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010024 ,2000},   // H27UBG8T2DTR 16nm 4G
    { {0xad, 0xde, 0x94, 0xa7, 0x42, 0x48, 0xff, 0xff }, 1,    32,     256,     2048,   0x00003188,   896,    30,     5,  0x040704,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010025 ,2000},   // H27UCG8T2ETR 16nm 8G customer's nand
    { {0xad, 0xde, 0x94, 0x97, 0x44, 0x45, 0xff, 0xff }, 1,    32,     128,     4120,   0x00003188,   896,    30,     6,  0x050704,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010026 ,2000},   // H27UCG8T2DTR 16nm 8G customer's nand,update RR code in boot0
    { {0xad, 0x3a, 0x14, 0xab, 0x42, 0x4a, 0xff, 0xff }, 1,    32,     256,     4184,   0x00003188,   896,    40,     6,  0x040704,      0,    &PhysicArchiPara3, &DefDDRInfo,  0x010027 ,2000},   // H27QDG8T2B8R 16nm 8G  
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------                                 
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,      0,         0,               0,            0,      0xffffff },
};


//==============================================================================
//============================= TOSHIBA NAND FLASH =============================
//==============================================================================
struct __NandPhyInfoPar_t ToshibaNandTbl[] =
{
    //                    NAND_CHIP_ID                 DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo  
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x98, 0xf1, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    20,     0,     0,          0,   &PhysicArchiPara5, &DefDDRInfo,  0x020000 ,20000},   // TC58NVG0S3B
    { {0x98, 0xda, 0xff, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    20,     0,     0,          0,   &PhysicArchiPara5, &DefDDRInfo,  0x020001 ,20000},   // TC58NVG1S3B
    { {0x98, 0xdc, 0x81, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000000,   896,    20,     0,     0,          0,   &PhysicArchiPara5, &DefDDRInfo,  0x020002 ,20000},   // TC58NVG2S3B
    { {0x98, 0xd1, 0x90, 0x15, 0x76, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    25,     0,     0,          0,   &PhysicArchiPara9, &DefDDRInfo,  0x020003 ,20000},   // TC58NVG0S3E
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                  
    { {0x98, 0xda, 0x84, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     1024,   0x00000000,   896,    20,     0,     0,          0,   &PhysicArchiPara6, &DefDDRInfo,  0x020004 ,10000},   // TC58NVG1D4B
    { {0x98, 0xdc, 0x84, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000008,   896,    20,     0,     0,          0,   &PhysicArchiPara6, &DefDDRInfo,  0x020005 ,10000},   // TC58NVG2D4B
    { {0x98, 0xd3, 0x84, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    20,     0,     0,          0,   &PhysicArchiPara7, &DefDDRInfo,  0x020006 ,10000},   // TC58NVG3D4C
    { {0x98, 0xd5, 0x85, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000008,   896,    20,     0,     0,          0,   &PhysicArchiPara7, &DefDDRInfo,  0x020007 ,10000},   // TC58NVG4D4C, TC58NVG5D4C
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                  
    { {0x98, 0xd3, 0x94, 0xba, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     2048,   0x00000008,   896,    20,     0,     0,          0,   &PhysicArchiPara6, &DefDDRInfo,  0x020008 ,10000},   // TC58NVG3D1DTG00
    { {0x98, 0xd7, 0x95, 0xba, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000008,   896,    30,     2,     0,          0,   &PhysicArchiPara7, &DefDDRInfo,  0x020009 ,5000},    // TC58NVG6D1DTG20
    { {0x98, 0xd5, 0x94, 0xba, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   896,    30,     2,     0,          0,   &PhysicArchiPara7, &DefDDRInfo,  0x02000a ,5000},     // TH58NVG5D1DTG20
    { {0x98, 0xd5, 0x94, 0x32, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     2048,   0x00000008,   896,    25,     1,     0,          0,   &PhysicArchiPara8, &DefDDRInfo,  0x02000b ,8000},     // TH58NVG4D2ETA20 TH58NVG4D2FTA20 TH58NVG5D2ETA00
    { {0x98, 0xd7, 0x94, 0x32, 0xff, 0xff, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000008,   896,    25,     2,     0,          0,   &PhysicArchiPara8, &DefDDRInfo,  0x02000c ,5000},     // TH58NVG5D2FTA00 TH58NVG6D2FTA20
    { {0x98, 0xd7, 0x95, 0x32, 0xff, 0xff, 0xff, 0xff }, 2,    16,     128,     4096,   0x00000008,   896,    25,     1,     0,          0,   &PhysicArchiPara8, &DefDDRInfo,  0x02000d ,8000},     // TH58NVG6D2ETA20

    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                            
    { {0x98, 0xde, 0x94, 0x82, 0x76, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00000588,   896,    40,     4, 0x100504,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x02000e ,4000},    // TH58NVG6D2ETA20
    { {0x98, 0xd7, 0x94, 0x32, 0x76, 0x56, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000588,   896,    40,     4, 0x100504,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x02000f ,4000},    // TH58NVG5D2HTA20
    { {0x98, 0xd5, 0x84, 0x32, 0x72, 0x56, 0xff, 0xff }, 1,    16,     128,     2048,   0x00000580,   896,    40,     4, 0x100504,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020010 ,4000},    // TH58NVG4D2HTA20
    { {0x98, 0xde, 0x84, 0x93, 0x72, 0x57, 0xff, 0xff }, 1,    32,     256,     2048,   0x00000580,   864,    40,     4, 0x100504,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020011 ,4000},    // TC58NVG6DCJTA00
    { {0x98, 0xd7, 0x84, 0x93, 0x72, 0x57, 0xff, 0xff }, 1,    32,     256,     1024,   0x00000580,   864,    40,     4, 0x100504,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020012 ,4000},    // TC58TEG5DCJTA00
    { {0x98, 0xde, 0x94, 0x93, 0x76, 0x57, 0xff, 0xff }, 1,    32,     256,     2048,   0x00000580,   864,    40,     4, 0x100504,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020013 ,4000},    // TC58TEG6DDJTA00
    { {0x98, 0xde, 0x84, 0x93, 0x72, 0xd7, 0xff, 0xff }, 1,    32,     256,     2048,   0x20011580,   864,    30,     4, 0x100504,       3,   &PhysicArchiPara9, &DDRInfo1  ,  0x020014 ,4000},    // TC58TEG6DCJTA00
    { {0x98, 0xd7, 0xd4, 0x32, 0x76, 0x55, 0xff, 0xff }, 1,    16,     128,     4096,   0x00000088,   896,    30,     2,     0,          0,   &PhysicArchiPara8, &DefDDRInfo,  0x020015 ,5000},    // TH58NVG6E2FTA00
    { {0x98, 0xde, 0x94, 0x93, 0x76, 0x50, 0xff, 0xff }, 1,    32,     256,     2048,   0x00011588,   840,    40,     4, 0x110705,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020016 ,4000},    // TH58TEG7DDKTA20 or TC58TEG6DDKTA00(A19nm)

    { {0x98, 0xde, 0xa4, 0x82, 0x76, 0xd7, 0xff, 0xff }, 1,    16,     256,     4096,   0x20011588,   864,    30,     4, 0x100504,       3,   &PhysicArchiPara9, &DDRInfo1  ,  0x020017 ,3000},    //TC58NVG6D2JTA00
    { {0x98, 0xd7, 0x84, 0x93, 0x72, 0x50, 0xff, 0xff }, 1,    32,     256,     1024,   0x00011580,   840,    40,     4, 0x110705,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020018 ,3000},    // TC58TEG5DCKTA00(A19nm)
    { {0x98, 0xde, 0x94, 0x93, 0x76, 0x51, 0xff, 0xff }, 1,    32,     256,     2148,   0x00000588,   840,    40,     4, 0x120a05,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x020019 ,3000},    // TC58TEG6DDLTA00(15nm)
    { {0x98, 0x3a, 0x94, 0x93, 0x76, 0x51, 0xff, 0xff }, 1,    32,     256,     4212,   0x00000588,   840,    40,     4, 0x120a05,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x02001a ,3000},    // TC58TFG7DDLTA0D/TH58TFG8DDLTA2D(15nm)
    { {0x98, 0xda, 0x90, 0x15, 0x76, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,      0x00088,   896,    25,     3,        0,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x02001b ,10000},    // TC58NVG1S3HTA00
    { {0x98, 0xd3, 0x91, 0x26, 0x76, 0xff, 0xff, 0xff }, 1,     8,      64,     4096,      0x00088,   896,    30,     3,        0,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x02001c ,10000},    // TC58NVG3S0HTAI0
    { {0x98, 0xdc, 0x90, 0x26, 0x76, 0xff, 0xff, 0xff }, 1,     8,      64,     2048,      0x00088,   896,    30,     3,        0,       0,   &PhysicArchiPara9, &DefDDRInfo,  0x02001d ,10000},    // TC58NVG2S0HTA00  
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,         0,      0,          0,             0,         0xffffff},   // NULL
};


//==============================================================================
//============================= MICON NAND FLASH ===============================
//==============================================================================
struct __NandPhyInfoPar_t MicronNandTbl[] =
{
    //                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo 
    //-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x2c, 0xda, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    25,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030000 ,20000},   // MT29F2G08AAC, JS29F02G08AAN
    { {0x2c, 0xdc, 0xff, 0x15, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     2048,   0x00000000,   896,    25,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030001 ,20000},   // MT29F4G08BAB, MT29F8G08FAB, JS29F04G08BAN, JS29F08G08FAN
    { {0x2c, 0xdc, 0x90, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000008,   896,    25,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030002 ,20000},   // MT29F4G08AAA, MT29F8G08DAA, JS29F04G08AAN
    { {0x2c, 0xd3, 0xd1, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000008,   896,    25,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030003 ,20000},   // MT29F8G08BAB, MT29F16G08FAB, JS29F08G08BAN, JS29F16G08FAN
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                       
    { {0x2c, 0xdc, 0x84, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000000,   896,    20,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030004 ,10000},   // MT29F4G08MAA, MT29F8G08QAA
    { {0x2c, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000000,   896,    20,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030005 ,10000},   // MT29F16GTAA
    { {0x2c, 0xd3, 0x94, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    30,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030006 ,10000},   // MT29F8G08MAA, MT29F16G08QAA, JS29F08G08AAM, JS29F16G08CAM
    { {0x2c, 0xd5, 0x95, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000008,   896,    20,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030007 ,10000},   // MT29F32G08TAA, JS29F32G08FAM
    { {0x2c, 0xd5, 0xd5, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000028,   896,    20,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030008 ,10000},   // MT29F32G08TAA, JS29F32G08FAM
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                       
    { {0x2c, 0xd5, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   896,    30,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030009 ,10000},   // MT29F16G08MAA, MT29F32G08QAA, JS29F32G08AAM, JS29F32G08CAM
    { {0x2c, 0xd5, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000008,   896,    30,     0,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x03000a ,10000},   // MT29F64G08TAA, JS29F64G08FAM
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                       
    { {0x2c, 0xd7, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000208,   896,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x03000b ,5000},    // MT29F32G08CBAAA,MT29F64G08CFAAA
    { {0x2c, 0xd7, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     4096,   0x00000008,   896,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x03000c ,5000},    // MT29F64G08CTAA
    { {0x2c, 0xd9, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 2,     8,     128,     8192,   0x00000008,   896,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x03000d ,5000},    // MT29F128G08,
    { {0x2c, 0x68, 0x04, 0x46, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     4096,   0x00001208,   896,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x03000e ,5000},   // MT29F32G08CBABA
    { {0x2c, 0x88, 0x05, 0xC6, 0xff, 0xff, 0xff, 0xff }, 2,     8,     256,     4096,   0x00001208,   896,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x03000f ,5000},   // MT29F128G08CJABA
    { {0x2c, 0x88, 0x04, 0x4B, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00001208,   896,    40,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030010 ,5000},   // MT29F64G08CBAAA
    { {0x2c, 0x68, 0x04, 0x4A, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     4096,   0x00001208,   896,    40,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030011 ,5000},   // MT29F32G08CBACA
    { {0x2c, 0x48, 0x04, 0x4A, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     2048,   0x00001208,   896,    40,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030012 ,5000},   // MT29F16G08CBACA
    { {0x2c, 0x48, 0x04, 0x46, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     2048,   0x00001208,   896,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x030013 ,5000},   // MT29F16G08CBABA
    { {0x2c, 0x64, 0x44, 0x4B, 0xA9, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00021788,   896,    40,     5,   0x400a01,     0,   &PhysicArchiPara4, &DefDDRInfo,  0x030014 ,4000},   // MT29F64G08CBABA  MT29F128G08CFABA
    { {0x2c, 0x44, 0x44, 0x4B, 0xA9, 0xff, 0xff, 0xff }, 1,    16,     256,     2048,   0x1f021788,   896,    30,     5,   0x400a01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x030015 ,4000},   // MT29F32G08CBADA
    { {0x2c, 0x88, 0x24, 0x4B, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x00001388,   870,    40,     2,   0x400a01,     0,   &PhysicArchiPara4, &DefDDRInfo,  0x030016 ,5000},   // MT29F64G08CBAAA(special for customer's nand)
    { {0x2c, 0x84, 0x64, 0x3c, 0xa5, 0xff, 0xff, 0xff }, 1,    32,     512,     2048,   0x1f051788,   896,    40,     4,   0x400a01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x030017 ,4000},   // MT29F128G08CBCAB(20nm, L85A)
    { {0x2c, 0x84, 0x64, 0x54, 0xa9, 0xff, 0xff, 0xff }, 1,    32,     512,     2048,   0x1f061788,   896,    40,     8,   0x410c01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x030018  ,3500},  // MT29F128G08CBECB(16nm, L95B)
    { {0x2c, 0x84, 0xc5, 0x4b, 0xa9, 0xff, 0xff, 0xff }, 2,    16,     256,     4096,   0x00021788,   896,    40,     5,   0x400a01,     0,   &PhysicArchiPara4, &DefDDRInfo,  0x030019 ,3500},   // MT29F256G08CJABA
    { {0x2c, 0x64, 0x64, 0x3c, 0xa5, 0xff, 0xff, 0xff }, 1,    32,     512,     1024,   0x1f051788,   896,    30,     4,   0x410c01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x03001a ,3500},   // MT29F64G08CBCEB   MT29F64G08CBEEB(20nm, L84D,customer's nand)
    { {0x2c, 0x78, 0x04, 0x4f, 0x89, 0xff, 0xff, 0xff }, 1,    16,     256,     2736,   0x1f021788,   896,    30,     8,   0x400a01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x03001b ,3500},   // FBNL7BT65K3BAAWP-AL
    { {0x2c, 0xa8, 0x05, 0xcb, 0xa9, 0xff, 0xff, 0xff }, 2,    16,     256,     4096,   0x00001388,   896,    30,     2,   0x400a01,     0,   &PhysicArchiPara4, &DefDDRInfo,  0x03001c ,4000},   // MT29F256G08CJAAA(customer's nand)
    { {0x2c, 0x64, 0x64, 0x56, 0xa5, 0x04, 0xff, 0xff }, 1,    24,     512,     1400,   0x1f061788,   896,    40,     8,   0x410c01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x03001d ,3000},  // MT29F64G08CBEFB(customer's nand,16nm, L94C)
    { {0x2c, 0xf1, 0x80, 0x95, 0x04, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    30,     0,       0,        0,   &PhysicArchiPara10,&DefDDRInfo,  0x03001d ,60000},   // MT29F1G08ABAEA
    { {0x2c, 0xda, 0x90, 0x95, 0x04, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x0000008c,   896,    30,     7,       0,        0,   &PhysicArchiPara4, &DefDDRInfo,  0x03001e ,60000},   // MT29F2G08ABAFA
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x0000,        0,      0,     0,     0,          0,            0,            0,        0xffffff },   // NULL
};


//==============================================================================
//============================= INTEL NAND FLASH ===============================
//==============================================================================
struct __NandPhyInfoPar_t IntelNandTbl[] =
{
    //                 NAND_CHIP_ID                    DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo 
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x89, 0xd3, 0x94, 0xa5, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     4096,   0x00000008,   896,    30,     0,     0,        0,   &PhysicArchiPara4, &DefDDRInfo,  0x040000 ,20000},   // 29F08G08AAMB2, 29F16G08CAMB2
    { {0x89, 0xd5, 0xd5, 0xa5, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     4096,   0x00000028,   896,    20,     0,     0,        0,   &PhysicArchiPara4, &DefDDRInfo,  0x040001 ,20000},   // 29F32G08FAMB2
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                                      
    { {0x89, 0xd7, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000008,   864,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x040002 ,5000},    // MLC32GW8IMA,MLC64GW8IMA, 29F32G08AAMD2, 29F64G08CAMD2
    { {0x89, 0xd5, 0x94, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     4096,   0x00000008,   864,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x040003 ,5000},    // 29F32G08CAMC1
    { {0x89, 0xd7, 0xd5, 0x3e, 0xff, 0xff, 0xff, 0xff }, 1,     8,     128,     8192,   0x00000008,   864,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x040004 ,5000},    // 29F64G08FAMC1
    { {0x89, 0x68, 0x04, 0x46, 0xff, 0xff, 0xff, 0xff }, 1,     8,     256,     4096,   0x00000208,   864,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x040005 ,5000},    // 29F32G08AAMDB
    { {0x89, 0x88, 0x24, 0x4B, 0xff, 0xff, 0xff, 0xff }, 1,    16,     256,     4096,   0x1f000208,   864,    40,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x040006 ,5000},    // 29F64G08CBAAA 29F64G083AME1 29F64G08ACME3
    { {0x89, 0xA8, 0x25, 0xCB, 0xff, 0xff, 0xff, 0xff }, 2,    16,     256,     4096,   0x00000208,   864,    30,     2,     0,          0,   &PhysicArchiPara4, &DefDDRInfo,  0x040007 ,5000},    // ?
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------                  
    { {0x89, 0x88, 0x24, 0x4B, 0xa9, 0x84, 0x00, 0x00 }, 1,    16,     256,     4096,   0x1f000788,   864,    40,     4,  0x500701,      0,   &PhysicArchiPara4, &DDRInfo1  ,  0x040008 ,3000},   // JS29F64G08ACMF3 JS29F16B08CCMF3 JS29F32B08JCMF3
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x89, 0x84, 0x64, 0x3c, 0xa5, 0x0C, 0x00, 0x00 }, 1,    32,     512,     2048,   0x1f000788,   896,    40,     4,  0x500701,      0,   &PhysicArchiPara4, &DefDDRInfo  ,  0x040009 ,3000},   // PF2916B08LCMFs 
    { {0x89, 0x84, 0x64, 0x3c, 0xa5, 0x00, 0x00, 0x00 }, 1,    32,     512,     2048,   0x1f000788,   896,    40,     4,  0x500701,      0,   &PhysicArchiPara4, &DefDDRInfo  ,  0x04000a ,3000},   // PF2916B08LCMF2 
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,      0,     0,         0,          0,              0,        0xffffff },   // NULL
};


//==============================================================================
//=============================== ST NAND FLASH ================================
//==============================================================================
struct __NandPhyInfoPar_t StNandTbl[] =
{
    //              NAND_CHIP_ID                       DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo 
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x20, 0xf1, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x050000 ,20000},  // NAND01GW3B
    { {0x20, 0xf1, 0x00, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x050001 ,20000},  // NAND01G001
    { {0x20, 0xda, 0x80, 0x15, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x050002 ,20000},  // NAND02GW3B
    { {0x20, 0xda, 0x10, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x050003 ,20000},  // NAND02GW3B2DN6
    { {0x20, 0xdc, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x050004 ,20000},  // NAND04GW3B
    { {0x20, 0xd3, 0xc1, 0x95, 0xff, 0xff, 0xff, 0xff }, 2,     4,      64,     4096,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x050005 ,20000},  // NAND08GW3B
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------                                
    { {0x20, 0xdc, 0x84, 0x25, 0xff, 0xff, 0xff, 0xff }, 1,     4,     128,     2048,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara2, &DefDDRInfo,  0x050006 ,10000},  // NAND04GW3C
    { {0x20, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 2,     4,     128,     2048,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara2, &DefDDRInfo,  0x050007 ,10000 },  // NAND08GW3C
    { {0x20, 0xd3, 0x85, 0x25, 0xff, 0xff, 0xff, 0xff }, 4,     4,     128,     2048,   0x00000000,   896,    15,    0,       0,         0,   &PhysicArchiPara2, &DefDDRInfo,  0x050008  ,10000},  // NAND16GW3C
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------          
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,    0,       0,         0,           0,            0,         0xffffff },   // NULL
};

//==============================================================================
//============================ SPANSION NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t SpansionNandTbl[] =
{
    //                   NAND_CHIP_ID                  DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo 
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x01, 0xaa, 0x10, 0x00, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00000000,   896,    30,     0,      0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x060000 ,20000},   // S39MS02G
    { {0x01, 0xa1, 0x10, 0x00, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    30,     0,      0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x060001 ,20000},   // S39MS01G
    { {0x01, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x00000000,   896,    30,     0,      0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x060002 ,20000},   // DFT01GR08P1PM0
    { {0x01, 0xda, 0x90, 0x95, 0x46, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,       0x0088,   896,    25,     3,      0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x060003 ,20000},    //S34ML02G2
    { {0x01, 0xdc, 0x90, 0x95, 0x56, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,       0x0088,   896,    30,     3,      0,         0,   &PhysicArchiPara0, &DefDDRInfo,  0x060004 ,60000},    //S34ML04G2
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------           
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,      0,         0,          0,              0,        0xffffff },   // NULL
};

//==============================================================================
//============================ POWER NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t PowerNandTbl[] =
{
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x92, 0xf1, 0x80, 0x95, 0x40, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c,   896,    30,     0,      0,         0,   &PhysicArchiPara10, &DefDDRInfo,  0x070000 ,60000},   // PSU1GA30HT
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------          
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,      0,         0,          0,                0,      0xffffff },   // NULL
};


//==============================================================================
//============================ SANDDISK NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t SandiskNandTbl[] =
{
    //                   NAND_CHIP_ID                 DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo
    //------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0x45, 0xde, 0x94, 0x93, 0xff, 0xff, 0xff, 0xff }, 1,     32,     256,    2048,   0x00000188,   896,    40,     4,      0x301409,  0,   &PhysicArchiPara0, &DefDDRInfo,  0x080000 ,4000},   // SDTNQGAMA-008G
    { {0x45, 0xd7, 0x84, 0x93, 0xff, 0xff, 0xff, 0xff }, 1,     32,     256,    1024,   0x00000180,   896,    40,     4,      0x301409,  0,   &PhysicArchiPara0, &DefDDRInfo,  0x080001 ,4000},   // SDTNQFAMA-004G
    { {0x45, 0xde, 0xa4, 0x82, 0x76, 0xd7, 0xff, 0xff }, 1,     16,     256,    4096,   0x20011188,   896,    30,     4,      0x301409,  3,   &PhysicArchiPara0, &DefDDRInfo,  0x080002 ,4000},   // SDTNQBAMA-008GB
    { {0x45, 0xde, 0x94, 0x93, 0x76, 0xd7, 0xff, 0xff }, 1,     32,     256,    2048,   0x20000188,   896,    30,     4,      0x301409,  3,   &PhysicArchiPara0, &DDRInfo1  ,  0x080003 ,4000},   // SDTNQGAMA-008GP
    { {0x45, 0xde, 0xa4, 0x82, 0x76, 0x56, 0xff, 0xff }, 1,     16,     256,    4096,   0x00001188,   944,    40,     4,      0x310909,  0,   &PhysicArchiPara0, &DefDDRInfo,  0x080004 ,4000},   // SDTNPMAHEM-008G
    { {0x45, 0xde, 0x94, 0x93, 0x76, 0x50, 0xff, 0xff }, 1,     32,     256,    2048,   0x00011188,   944,    40,     4,      0x321f09,  0,   &PhysicArchiPara0, &DefDDRInfo,  0x080005 ,4000},   // SDTNRGAMA-008G
    { {0x45, 0xd7, 0x84, 0x93, 0x72, 0x50, 0xff, 0xff }, 1,     32,     256,    1024,   0x00011180,   944,    40,     4,      0x331f09,  0,   &PhysicArchiPara0, &DefDDRInfo,  0x080006 ,3500},   // SDTNRFAMA-004Gk
    { {0x45, 0xde, 0x84, 0x93, 0x72, 0x57, 0xff, 0xff }, 1,     32,     256,    2048,    0x00000180,  864,    30,     4,      0x301409,  0,   &PhysicArchiPara0, &DefDDRInfo,  0x080007 ,4000},//19nm
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,      0,         0,        0,                 0,       0xffffff },   // NULL
};

//==============================================================================
//============================ SPECTEK NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t SpectekNandTbl[] =
{
    //                   NAND_CHIP_ID                 DieCnt   SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
    { {0xb5, 0x64, 0x64, 0x3c, 0xa5, 0x04, 0xff, 0xff }, 1,     32,      512,     1024,   0x1f051788,   896,    30,     4,   0x410c01,     0,   &PhysicArchiPara4, &DDRInfo1  ,  0x090001 ,3500},   // FBNL84C61KDBABJ4
    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------          
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,     0x00000000,     0,     0,     0,      0,         0,          0,                0,      0xffffff },   // NULL
};

//==============================================================================
//============================ MXIC NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t MacronixNandTbl[] =
{
    //                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       
    //-----------------------------------------------------------------------------------------------------------------------
    { {0xc2, 0xda, 0x90, 0x95, 0x07, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00028c,   944,    30,     2,           0,    0,   &PhysicArchiPara4,  &DefDDRInfo,    0x0a0000 ,40000}, //MX30LF2G28AB
    { {0xc2, 0xda, 0x90, 0x95, 0x06, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x00028c,   944,    30,     0,           0,    0,   &PhysicArchiPara4,  &DefDDRInfo,    0x0a0001 ,40000}, //MX30LF2G18AC
    { {0xc2, 0xf1, 0x80, 0x1d, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c, 944,    25,     0,           0,    0,   &PhysicArchiPara10, &DefDDRInfo,    0x0a0002 ,40000}, //MX30LF1G08AA
    { {0xc2, 0xdc, 0x90, 0x95, 0x57, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00028c,   944,    30,     2,           0,    0,   &PhysicArchiPara4,  &DefDDRInfo,    0x0a0003 ,40000}, //MX30LF4G28AB
    { {0xc2, 0xf1, 0x80, 0x95, 0x02, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c, 944,    30,     0,           0,    0,   &PhysicArchiPara10, &DefDDRInfo,    0x0a0004 ,40000}, //MX30LF1G18AC
    { {0xc2, 0xdc, 0x90, 0x95, 0x56, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x00008c,   944,    30,     0,           0,    0,   &PhysicArchiPara4,  &DefDDRInfo,    0x0a0005 ,40000}, //MX30LF4G18AC
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x000000,     0,     0,     0,           0,    0,  (struct __OptionalPhyOpPar_t *)0, (struct __NfcInitDdrInfo*) 0xffffff}, //default
};

//==============================================================================
//============================ WINBOND NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t WinbondNandTbl[] =
{
    //                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       
    //-----------------------------------------------------------------------------------------------------------------------
    { {0xef, 0xf1, 0x80, 0x95, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c, 944,    25,     0,           0,    0,   &PhysicArchiPara10, &DefDDRInfo,    0x0b0000 ,40000}, //W29N01GV
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x000000,     0,     0,     0,           0,    0,  (struct __OptionalPhyOpPar_t *)0, (struct __NfcInitDdrInfo*) 0xffffff}, //default
};

//==============================================================================
//============================ FIDELIX NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t FidelixNandTbl[] =
{
    //                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       
    //-----------------------------------------------------------------------------------------------------------------------
    { {0xf8, 0xf1, 0x80, 0x91, 0xff, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c, 944,    25,     0,           0,    0,   &PhysicArchiPara10, &DefDDRInfo,    0x0c0000 ,40000}, //FMND1G08U3B
    { {0xf8, 0xda, 0x90, 0x91, 0x46, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x0000008c, 944,    25,     0,           0,    0,   &PhysicArchiPara4,  &DefDDRInfo,    0x0c0001 ,40000}, //FMND2G08U3B
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x000000,     0,     0,     0,           0,    0,  (struct __OptionalPhyOpPar_t *)0, (struct __NfcInitDdrInfo*) 0xffffff}, //default
};

//==============================================================================
//============================ ESMT NAND FLASH ==============================
//==============================================================================
struct __NandPhyInfoPar_t EsmtNandTbl[] =
{
    //                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       
    //-----------------------------------------------------------------------------------------------------------------------
    { {0xc8, 0xd1, 0x80, 0x95, 0x40, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c, 944,    30,     0,           0,    0,   &PhysicArchiPara10,  &DefDDRInfo,   0x0d0000 ,60000}, //F59L1G81MA
    { {0xc8, 0xda, 0x90, 0x95, 0x44, 0xff, 0xff, 0xff }, 1,     4,      64,     2048,   0x0000008c, 944,    30,     0,           0,    0,   &PhysicArchiPara2,   &DefDDRInfo,   0x0d0001 ,60000}, //F59L2G81A
    { {0xc8, 0xdc, 0x90, 0x95, 0x54, 0xff, 0xff, 0xff }, 1,     4,      64,     4096,   0x0000008c, 944,    30,     0,           0,    0,   &PhysicArchiPara2,   &DefDDRInfo,   0x0d0002 ,60000}, //F59L4G81A
    { {0xc8, 0xd1, 0x80, 0x95, 0x42, 0xff, 0xff, 0xff }, 1,     4,      64,     1024,   0x4000008c, 944,    30,     0,           0,    0,   &PhysicArchiPara10,  &DefDDRInfo,   0x0d0003 ,60000}, //PSU1GA30BT
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x000000,     0,     0,     0,           0,    0,  (struct __OptionalPhyOpPar_t *)0, (struct __NfcInitDdrInfo*) 0xffffff}, //default
};

//==============================================================================
//============================= DEFAULT NAND FLASH =============================
//==============================================================================
struct __NandPhyInfoPar_t DefaultNandTbl[] =
{
    //                    NAND_CHIP_ID                DieCnt SecCnt  PagCnt   BlkCnt     OpOpt      DatBlk  Freq   EccMode ReadRetry DDRType   OperationPar       DDRInfo
    //-----------------------------------------------------------------------------------------------------------------------
    { {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, 0,     0,       0,        0,   0x00000000,     0,     0,     0,           0,    0,   &DefualtPhysicArchiPara, &DefDDRInfo, 0xffffff}, //default
};

