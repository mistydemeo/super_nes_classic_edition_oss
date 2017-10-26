//*********************************************************************************************************************
//  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
//
//  File name   :	de_fcc.c
//
//  Description :	display engine 2.0 fcc base functions implement
//
//  History     :	2014/03/28  iptang  v0.1  Initial version
//
//*********************************************************************************************************************

#include "de_fcc_type.h"
#include "de_rtmx.h"
#include "de_vep_table.h"
#include "de_vep.h"

#define FCC_OFST 0xAA000

static volatile __fcc_reg_t *fcc_dev[DEVICE_NUM][VEP_NUM];
static de_reg_blocks fcc_para_block[DEVICE_NUM][VEP_NUM];
static de_reg_blocks fcc_csc_block[DEVICE_NUM][VEP_NUM];
static bool enhance_enable[DEVICE_NUM][VEP_NUM] = {{0}};

int de_fcc_init(unsigned int sel, unsigned int reg_base)
{
	int j, chno = 0;
	unsigned int fcc_base;
	void *memory;
	int num_chns;

	num_chns = de_feat_get_num_chns(sel);

	chno = de_feat_is_support_vep(sel);
	__inf("sel%d has %d ch support vep\n", sel, chno);

	for(j=0;j<chno;j++)
	{
		fcc_base = reg_base + (sel+1)*0x00100000 + FCC_OFST + j*0x10000;	//FIXME  display path offset should be defined
		__inf("sel %d, fcc_base[%d]=0x%x\n", sel, j, fcc_base);

		memory = disp_sys_malloc(sizeof(__fcc_reg_t));
		if(NULL == memory) {
			__wrn("malloc vep fcc[%d][%d] memory fail! size=0x%x\n", sel, j, sizeof(__fcc_reg_t));
			return -1;
		}

		fcc_para_block[sel][j].off		= fcc_base;
		fcc_para_block[sel][j].val		= memory;
		fcc_para_block[sel][j].size		= 0x48;
		fcc_para_block[sel][j].dirty 	= 0;

		fcc_csc_block[sel][j].off		= fcc_base + 0x50;
		fcc_csc_block[sel][j].val		= memory + 0x50;
		fcc_csc_block[sel][j].size		= 0x40;
		fcc_csc_block[sel][j].dirty 	= 0;

		de_fcc_set_reg_base(sel, j, memory);
	}

	return 0;
}

int de_fcc_update_regs(unsigned int sel)
{
	int i,chno;
#if 0
	chno=3;	//ui scaler FIXME
	if(sel)
	{
		chno=1;
	}
#else
	chno = de_feat_get_num_vi_chns(sel);//?
#endif

	for(i=0;i<chno;i++)
	{
		if(fcc_para_block[sel][i].dirty == 0x1){
			memcpy((void *)fcc_para_block[sel][i].off,fcc_para_block[sel][i].val,fcc_para_block[sel][i].size);}
		if(fcc_csc_block[sel][i].dirty == 0x1){
			memcpy((void *)fcc_csc_block[sel][i].off,fcc_csc_block[sel][i].val,fcc_csc_block[sel][i].size);}

	}

	return 0;
}

//*********************************************************************************************************************
// function       : de_fcc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
// description    : set fcc reg base
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  base	<reg base>
// return         :
//                  success
//*********************************************************************************************************************
int de_fcc_set_reg_base(unsigned int sel, unsigned int chno, void *base)
{
	fcc_dev[sel][chno] = (__fcc_reg_t *)base;

	return 0;
}

//*********************************************************************************************************************
// function       : de_fcc_enable(unsigned int sel, unsigned int chno, unsigned int en)
// description    : enable/disable fcc
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  en		<enable: 0-diable; 1-enable>
// return         :
//                  success
//*********************************************************************************************************************
int de_fcc_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	fcc_dev[sel][chno]->fcc_ctl.bits.en = en;
	enhance_enable[sel][chno] = en;

	fcc_para_block[sel][chno].dirty	= 1;

	return 0;
}

//*********************************************************************************************************************
// function       : de_fcc_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height)
// description    : set fcc size
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  width	<input width>
//					height	<input height>
// return         :
//                  success
//*********************************************************************************************************************
int de_fcc_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height)
{
	fcc_dev[sel][chno]->fcc_size.bits.width = width==0?0:width-1;
	fcc_dev[sel][chno]->fcc_size.bits.height = height==0?0:height-1;

	fcc_para_block[sel][chno].dirty	= 1;

	return 0;
}

//*********************************************************************************************************************
// function       : de_fcc_set_window(unsigned int sel, unsigned int chno, unsigned int win_en, de_rect window)
// description    : set fcc window
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  win_en	<enable: 0-window mode diable; 1-window mode enable>
//					window	<window rectangle>
// return         :
//                  success
//*********************************************************************************************************************
int de_fcc_set_window(unsigned int sel, unsigned int chno, unsigned int win_en, de_rect window)
{
	fcc_dev[sel][chno]->fcc_ctl.bits.win_en = win_en&0x1;

	if(win_en)
	{
		fcc_dev[sel][chno]->fcc_win0.bits.left	= window.x;
		fcc_dev[sel][chno]->fcc_win0.bits.top	= window.y;
		fcc_dev[sel][chno]->fcc_win1.bits.right = window.x + window.w - 1;
		fcc_dev[sel][chno]->fcc_win1.bits.bot	= window.y + window.h - 1;
	}

	fcc_para_block[sel][chno].dirty	= 1;

	return 0;
}

//*********************************************************************************************************************
// function       : de_fcc_set_para(unsigned int sel, unsigned int chno, unsigned int mode)
// description    : set fcc para
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  mode	<fcc mode: select mode table>
// return         :
//                  success
//*********************************************************************************************************************
int de_fcc_set_para(unsigned int sel, unsigned int chno, unsigned int mode)
{
	memcpy((void*)fcc_dev[sel][chno]->fcc_range, (void*)&fcc_range_gain[0],sizeof(int)*14);

	fcc_para_block[sel][chno].dirty	= 1;

	return 0;
}

//*********************************************************************************************************************
// function       : de_csc_enable(unsigned int sel, unsigned int chno, unsigned int en)
// description    : enable/disable csc
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  en		<enable: 0-diable; 1-enable>
//                  mode	<mode: 0-bt601; 1-bt709; 2-ycc;>
// return         :
//                  success
//*********************************************************************************************************************
int de_fcc_csc_set(unsigned int sel, unsigned int chno, unsigned int en, unsigned int mode)
{
    __inf("sel %d, chno%d, en %d, mode %d\n", sel, chno, en, mode);
	fcc_dev[sel][chno]->fcc_csc_ctl.bits.bypass = en;

	switch(mode)
	{
		case DE_BT601:
		{
			memcpy((void*)fcc_dev[sel][chno]->fcc_csc_coff0, (void*)&y2r[0], sizeof(int)*12);break;
		}
		case DE_BT709:
		{
			memcpy((void*)fcc_dev[sel][chno]->fcc_csc_coff0, (void*)&y2r[12], sizeof(int)*12);break;
		}
		case DE_YCC:
		{
			memcpy((void*)fcc_dev[sel][chno]->fcc_csc_coff0, (void*)&y2r[24], sizeof(int)*12);break;
		}
		default:
			break;
	}

	fcc_csc_block[sel][chno].dirty 	= 1;

	return 0;
}

