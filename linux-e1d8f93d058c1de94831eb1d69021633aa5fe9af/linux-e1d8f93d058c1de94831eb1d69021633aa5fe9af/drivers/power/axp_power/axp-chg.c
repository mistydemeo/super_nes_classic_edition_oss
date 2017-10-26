/*
 * Battery charger driver for AW-POWERS
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/power_supply.h>
#include "axp-cfg.h"
#include <linux/mfd/axp-mfd.h>
#ifdef CONFIG_AW_AXP81X
#include "axp81x/axp81x-sply.h"
#include "axp81x/axp81x-common.h"
static const struct axp_config_info *axp_config = &axp81x_config;
#endif

static aw_charge_type axp_usbcurflag = CHARGE_AC;
static aw_charge_type axp_usbvolflag = CHARGE_AC;

int axp_usbvol(aw_charge_type type)
{
	axp_usbvolflag = type;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usbcur(aw_charge_type type)
{
	axp_usbcurflag = type;
	mod_timer(&axp_charger->usb_status_timer,jiffies + msecs_to_jiffies(0));
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur);

int axp_usb_det(void)
{
	uint8_t ret = 0;

	if(axp_charger == NULL || axp_charger->master == NULL)
	{
		return ret;
	}
	axp_read(axp_charger->master,AXP_CHARGE_STATUS,&ret);
	if(ret & 0x10)/*usb or usb adapter can be used*/
		return 1;
	else/*no usb or usb adapter*/
		return 0;
}
EXPORT_SYMBOL_GPL(axp_usb_det);

static void axp_charger_update_usb_state(unsigned long data)
{
	struct axp_charger * charger = (struct axp_charger *)data;

	spin_lock(&charger->charger_lock);
	charger->usb_valid = (((CHARGE_USB_20 == axp_usbcurflag) || (CHARGE_USB_30 == axp_usbcurflag)) &&
				charger->ext_valid);
	charger->usb_adapter_valid = ((0 == charger->ac_valid) && (CHARGE_USB_20 != axp_usbcurflag) &&
					(CHARGE_USB_30 != axp_usbcurflag) && (charger->ext_valid));
	if(charger->in_short) {
		charger->ac_valid = ((charger->usb_adapter_valid == 0) && (charger->usb_valid == 0) &&
					(charger->ext_valid));
	}
	spin_unlock(&charger->charger_lock);

	DBG_PSY_MSG(DEBUG_CHG, "usb_valid=%d ac_valid=%d usb_adapter_valid=%d\n",charger->usb_valid,
				charger->ac_valid, charger->usb_adapter_valid);
	DBG_PSY_MSG(DEBUG_CHG, "usb_det=%d ac_det=%d \n",charger->usb_det,
				charger->ac_det);
	power_supply_changed(&charger->ac);
	power_supply_changed(&charger->usb);
	schedule_delayed_work(&(charger->usbwork), 0);
}

static void axp_usb(struct work_struct *work)
{
	int var;
	uint8_t tmp,val;

	DBG_PSY_MSG(DEBUG_CHG, "[axp_usb]axp_usbcurflag = %d\n",axp_usbcurflag);

	axp_read(axp_charger->master, AXP_CHARGE_STATUS, &val);
	if((val & 0x10) == 0x00){/*usb or usb adapter can not be used*/
		DBG_PSY_MSG(DEBUG_CHG, "USB not insert!\n");
		axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x02);
		axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x01);
	}else if(CHARGE_USB_20 == axp_usbcurflag){
		DBG_PSY_MSG(DEBUG_CHG, "set usbcur_pc %d mA\n",axp_config->pmu_usbcur_pc);
		if(axp_config->pmu_usbcur_pc){
			var = axp_config->pmu_usbcur_pc * 1000;
			if(var >= 900000)
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
			else if (var < 900000){
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x02);
				axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x01);
			}
		}else//not limit
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}else if (CHARGE_USB_30 == axp_usbcurflag){
		axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}else {
		DBG_PSY_MSG(DEBUG_CHG, "set usbcur %d mA\n",axp_config->pmu_usbcur);
		if((axp_config->pmu_usbcur) && (axp_config->pmu_usbcur_limit)){
			var = axp_config->pmu_usbcur * 1000;
			if(var >= 900000)
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
			else if (var < 900000){
				axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x02);
				axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x01);
			}
		}else //not limit
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}

	if(!vbus_curr_limit_debug){ //usb current not limit
		DBG_PSY_MSG(DEBUG_CHG, "vbus_curr_limit_debug = %d\n",vbus_curr_limit_debug);
		axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x03);
	}

	if(CHARGE_USB_20 == axp_usbvolflag){
		DBG_PSY_MSG(DEBUG_CHG, "set usbvol_pc %d mV\n",axp_config->pmu_usbvol_pc);
		if(axp_config->pmu_usbvol_pc){
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
			var = axp_config->pmu_usbvol_pc * 1000;
			if(var >= 4000000 && var <=4700000){
				tmp = (var - 4000000)/100000;
				axp_read(axp_charger->master, AXP_CHARGE_VBUS,&val);
				val &= 0xC7;
				val |= tmp << 3;
				axp_write(axp_charger->master, AXP_CHARGE_VBUS,val);
			}else
				DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol_pc);
		}else
		    axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
	}else if(CHARGE_USB_30 == axp_usbvolflag) {
		axp_read(axp_charger->master, AXP_CHARGE_VBUS,&val);
		val &= 0xC7;
		val |= 7 << 3;
		axp_write(axp_charger->master, AXP_CHARGE_VBUS,val);
	}else {
		DBG_PSY_MSG(DEBUG_CHG, "set usbvol %d mV\n",axp_config->pmu_usbvol);
		if((axp_config->pmu_usbvol) && (axp_config->pmu_usbvol_limit)){
			axp_set_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
			var = axp_config->pmu_usbvol * 1000;
			if(var >= 4000000 && var <=4700000){
				tmp = (var - 4000000)/100000;
				axp_read(axp_charger->master, AXP_CHARGE_VBUS,&val);
				val &= 0xC7;
				val |= tmp << 3;
				axp_write(axp_charger->master, AXP_CHARGE_VBUS,val);
			}else
				DBG_PSY_MSG(DEBUG_CHG, "set usb limit voltage error,%d mV\n",axp_config->pmu_usbvol);
		}else
		    axp_clr_bits(axp_charger->master, AXP_CHARGE_VBUS, 0x40);
	}
}

void axp_usbac_checkst(struct axp_charger *charger)
{
	uint8_t val;

	if (axp_config->pmu_bc_en) {
		if(charger->in_short) {
			axp_read(axp_charger->master, AXP_BC_DET_STATUS, &val);
			spin_lock(&charger->charger_lock);
			if (0x20 == (val&0xe0)) {
				charger->usb_valid = 1;
			} else {
				charger->usb_valid = 0;
			}
			if (0x60 == (val&0xe0)) {
				charger->ac_valid = 1;
			} else {
				charger->ac_valid = 0;
			}
			spin_unlock(&charger->charger_lock);
		}
	}
}

void axp_usbac_in(struct axp_charger *charger)
{
	if (!axp_config->pmu_bc_en) {
		DBG_PSY_MSG(DEBUG_CHG, "axp ac/usb in!\n");
		if(timer_pending(&charger->usb_status_timer))
			del_timer_sync(&charger->usb_status_timer);
		/* must limit the current now,and will again fix it while usb/ac detect finished! */
		if(axp_config->pmu_usbcur_pc){
		        if(axp_config->pmu_usbcur_pc >= 900)
		                axp_clr_bits(charger->master, AXP_CHARGE_VBUS, 0x03);
		        else if (axp_config->pmu_usbcur_pc < 900) {
		                axp_clr_bits(charger->master, AXP_CHARGE_VBUS, 0x02);
		                axp_set_bits(charger->master, AXP_CHARGE_VBUS, 0x01);
		        }
		}else//not limit
		        axp_set_bits(charger->master, AXP_CHARGE_VBUS, 0x03);
		/* this is about 3.5s,while the flag set in usb drivers after usb plugged */
		mod_timer(&charger->usb_status_timer,jiffies + msecs_to_jiffies(5000));
	}
}

void axp_usbac_out(struct axp_charger *charger)
{
	if (!axp_config->pmu_bc_en) {
		DBG_PSY_MSG(DEBUG_CHG, "axp22 ac/usb out!\n");
		if(timer_pending(&charger->usb_status_timer))
			del_timer_sync(&charger->usb_status_timer);
		/* if we plugged usb & ac at the same time,then unpluged ac quickly while the usb driver */
		/* do not finished detecting,the charger type is error!So delay the charger type report 2s */
		mod_timer(&charger->usb_status_timer,jiffies + msecs_to_jiffies(2000));
	}
}

int axp_chg_init(struct axp_charger *charger)
{
	int ret = 0;

	if (axp_config->pmu_bc_en) {
		/* enable BC */
		axp_set_bits(charger->master, AXP_BC_SET, 0x01);
	} else {
		setup_timer(&charger->usb_status_timer, axp_charger_update_usb_state, (unsigned long)charger);
		/* set usb cur-vol limit*/
		INIT_DELAYED_WORK(&(charger->usbwork), axp_usb);
		mod_timer(&charger->usb_status_timer,jiffies + msecs_to_jiffies(10));
	}
	return ret;
}

void axp_chg_exit(struct axp_charger *charger)
{
	if (axp_config->pmu_bc_en) {
		/* enable BC */
		axp_clr_bits(charger->master, AXP_BC_SET, 0x01);
	} else {
		cancel_delayed_work_sync(&(charger->usbwork));
		del_timer_sync(&(charger->usb_status_timer));
	}
	return;
}

