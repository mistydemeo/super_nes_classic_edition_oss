//*********************************************************************************************************************
//  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
//
//  File name   :	de_peak.c
//
//  Description :	display engine 2.0 peaking basic function definition
//
//  History     :	2014/03/27  vito cheng  v0.1  Initial version
//
//*********************************************************************************************************************

#include "de_peak_type.h"
#include "de_rtmx.h"

//DE configuration
#define VPE_NUM			1	//number of vpe
#define PEAK_CHN_NUM 	1	//number of peak in one vpe

static volatile __peak_reg_t *peak_dev[VPE_NUM][PEAK_CHN_NUM];

//*********************************************************************************************************************
// function       : de_peak_set_reg_base(unsigned int sel, unsigned int chno, unsigned int base)
// description    : set peak reg base
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  base	<reg base>
// return         :
//                  success
//*********************************************************************************************************************
int de_peak_set_reg_base(unsigned int sel, unsigned int chno, unsigned int base)
{
	peak_dev[sel][chno] = (__peak_reg_t *)base;

	return 0;
}

//*********************************************************************************************************************
// function       : de_peak_enable(unsigned int sel, unsigned int chno, unsigned int en)
// description    : enable/disable peak
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  en		<enable: 0-diable; 1-enable>
// return         :
//                  success
//*********************************************************************************************************************
int de_peak_enable(unsigned int sel, unsigned int chno, unsigned int en)
{
	peak_dev[sel][chno]->ctrl.bits.en = en;

	return 0;
}

//*********************************************************************************************************************
// function       : de_peak_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height)
// description    : set peak size
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  width	<input width>
//					height	<input height>
// return         :
//                  success
//*********************************************************************************************************************
int de_peak_set_size(unsigned int sel, unsigned int chno, unsigned int width, unsigned int height)
{
	peak_dev[sel][chno]->size.bits.width = width - 1;
	peak_dev[sel][chno]->size.bits.height = height - 1;

	return 0;
}

//*********************************************************************************************************************
// function       : de_peak_set_window(unsigned int sel, unsigned int chno, unsigned int win_enable, __disp_rect_t window)
// description    : set peak window
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  win_enable	<enable: 0-window mode diable; 1-window mode enable>
//					window	<window rectangle>
// return         :
//                  success
//*********************************************************************************************************************
int de_peak_set_window(unsigned int sel, unsigned int chno, unsigned int win_enable, de_rect window)
{
	peak_dev[sel][chno]->ctrl.bits.win_en = win_enable;

	if(win_enable)
	{
		peak_dev[sel][chno]->win0.bits.win_left = window.x;
		peak_dev[sel][chno]->win0.bits.win_top = window.y;
		peak_dev[sel][chno]->win1.bits.win_right = window.x + window.w - 1;
		peak_dev[sel][chno]->win1.bits.win_bot = window.y + window.h - 1;
	}
	return 0;
}

//*********************************************************************************************************************
// function       : de_peak_set_para(unsigned int sel, unsigned int chno, unsigned int gain)
// description    : set peak para
// parameters     :
//                  sel		<rtmx select>
//                  chno	<overlay select>
//                  gain	<peak gain: normal setting 36-42>
// return         :
//                  success
//*********************************************************************************************************************
int de_peak_set_para(unsigned int sel, unsigned int chno, unsigned int gain)
{
	peak_dev[sel][chno]->gain.bits.gain = gain;
	peak_dev[sel][chno]->filter.bits.filter_sel = 0;
	peak_dev[sel][chno]->filter.bits.hp_ratio = 4;
	peak_dev[sel][chno]->filter.bits.bp0_ratio = 12;

	peak_dev[sel][chno]->filter.bits.bp1_ratio = 0;
	peak_dev[sel][chno]->gainctrl.bits.beta = 0;
	peak_dev[sel][chno]->gainctrl.bits.dif_up = 128;
	peak_dev[sel][chno]->shootctrl.bits.neg_gain = 31;
	peak_dev[sel][chno]->coring.bits.corthr = 4;

	return 0;
}

