/*
 *  drivers/arisc/interfaces/arisc_dvfs.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"
#include <mach/sys_config.h>

typedef struct arisc_freq_voltage
{
	u32 freq;       //cpu frequency
	u32 voltage;    //voltage for the frequency
	u32 axi_div;    //the divide ratio of axi bus
} arisc_freq_voltage_t;

//cpu voltage-freq table
#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1)
static struct arisc_freq_voltage arisc_vf_table[ARISC_DVFS_VF_TABLE_MAX] =
{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
};
#elif defined (CONFIG_ARCH_SUN9IW1P1) || (defined CONFIG_ARCH_SUN8IW6P1)
static struct arisc_freq_voltage arisc_vf_table[2][ARISC_DVFS_VF_TABLE_MAX] =
{
	{
	//freq          //voltage   //axi_div
	{900000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       3}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	{0,             1200,       3}, //end of cpu dvfs table
	},

	{
	//freq          //voltage   //axi_div
	{900000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	{600000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	{420000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	{360000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	{300000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	{240000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	{120000000,     1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	{60000000,      1200,       4}, //cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	{0,             1200,       4}, //end of cpu dvfs table
	},
};
#endif

static int arisc_dvfs_get_cfg(char *main, char *sub, u32 *val)
{
	script_item_u script_val;
	script_item_value_type_e type;
	type = script_get_item(main, sub, &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("arisc dvfs config type err!");
		return -EINVAL;
	}
	*val = script_val.val;
	ARISC_INF("arisc dvfs config [%s] [%s] : %d\n", main, sub, *val);
	return 0;
}


#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1)
int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    result = 0;
	int    vf_table_size = 0;
	char   vf_table_main_key[64];
	char   vf_table_sub_key[64];
	struct arisc_message *pmessage;
	u32    ver;
	u32    is_def_table = 0;
	spinlock_t    dvfs_lock;    /* spinlock for dvfs */
	unsigned long dvfs_flag;

	/* initialize message manager spinlock */
	spin_lock_init(&(dvfs_lock));
	dvfs_flag = 0;

	spin_lock_irqsave(&(dvfs_lock), dvfs_flag);
	ver = readl(IO_ADDRESS(SUNXI_SRAMCTRL_PBASE + 0x24));
	ver |= 0x1 << 15;
	writel(ver, IO_ADDRESS(SUNXI_SRAMCTRL_PBASE + 0x24));
	ver = readl(IO_ADDRESS(SUNXI_SRAMCTRL_PBASE + 0x24)) >> 16;
	spin_unlock_irqrestore(&(dvfs_lock), dvfs_flag);

	if (ver == 0x1661) {
		if (arisc_dvfs_get_cfg("dvfs_table_bak", "LV_count", &vf_table_size)) {
			strcpy(vf_table_main_key, "dvfs_table");
			is_def_table = 1;
		} else {
			strcpy(vf_table_main_key, "dvfs_table_bak");
			is_def_table = 0;
		}
	} else {
		strcpy(vf_table_main_key, "dvfs_table");
		is_def_table = 1;
	}

	/* parse system config v-f table information */
	if (arisc_dvfs_get_cfg(vf_table_main_key, "LV_count", &vf_table_size)) {
		ARISC_WRN("parse system config dvfs_table size fail\n");
	}
	for (index = 0; index < vf_table_size; index++) {
		sprintf(vf_table_sub_key, "LV%d_freq", index + 1);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			arisc_vf_table[index].freq = value;
		}
		sprintf(vf_table_sub_key, "LV%d_volt", index + 1);
		if (arisc_dvfs_get_cfg(vf_table_main_key, vf_table_sub_key, &value) == 0) {
			if ((value < 1100) && (ver == 0x1661) && (is_def_table)) {
				value = 1100;
			}
			arisc_vf_table[index].voltage = value;
		}
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
		/* initialize message
		 *
		 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
		 * | index  |  freq  |voltage |axi_div |  pll  |
		 */
		pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
		pmessage->paras[0]   = index;
		pmessage->paras[1]   = arisc_vf_table[index].freq;
		pmessage->paras[2]   = arisc_vf_table[index].voltage;
		pmessage->paras[3]   = arisc_vf_table[index].axi_div;
		pmessage->paras[4]   = 0;
		pmessage->state      = ARISC_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;

		ARISC_INF("v-f table: index %d freq %d vol %d axi_div %d\n",
		pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);


		/* send request message */
		arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

		//check config fail or not
		if (pmessage->result) {
			ARISC_WRN("config dvfs v-f table [%d] fail\n", index);
			result = -EINVAL;
			break;
		}
	}
	/* free allocated message */
	arisc_message_free(pmessage);

	return result;

}

#elif (defined CONFIG_ARCH_SUN9IW1P1) || (defined CONFIG_ARCH_SUN8IW6P1)
int arisc_dvfs_cfg_vf_table(void)
{
	u32    value = 0;
	int    index = 0;
	int    cluster = 0;
	int    result = 0;
	int    vf_table_size[2] = {0, 0};
	char   vf_table_key[256];
	struct arisc_message *pmessage;

	/* parse system config v-f table information */
	if (arisc_dvfs_get_cfg("dvfs_table", "C0_LV_count", &vf_table_size[0])) {
		ARISC_WRN("parse system config dvfs_table size fail\n");
	}

	if (arisc_dvfs_get_cfg("dvfs_table", "C1_LV_count", &vf_table_size[1])) {
		ARISC_WRN("parse system config dvfs_table size fail\n");
	}

	for (cluster = 0; cluster < 2; cluster++) {
		   for (index = 0; index < vf_table_size[cluster]; index++) {
			sprintf(vf_table_key, "C%d_LV%d_freq", cluster, index + 1);
			if (arisc_dvfs_get_cfg("dvfs_table", vf_table_key, &value) == 0) {
				arisc_vf_table[cluster][index].freq = value;
			}
			sprintf(vf_table_key, "C%d_LV%d_volt", cluster, index + 1);
			if (arisc_dvfs_get_cfg("dvfs_table", vf_table_key, &value) == 0) {
				arisc_vf_table[cluster][index].voltage = value;
			}
		}

	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	for (cluster = 0; cluster < 2; cluster++) {
		for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
			/* initialize message
			 *
			 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
			 * | index  |  freq  |voltage |axi_div |  pll   |
			 */
			pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
			pmessage->paras[0]   = index;
			pmessage->paras[1]   = arisc_vf_table[cluster][index].freq;
			pmessage->paras[2]   = arisc_vf_table[cluster][index].voltage;
			pmessage->paras[3]   = arisc_vf_table[cluster][index].axi_div;
			pmessage->paras[4]   = cluster;
			pmessage->state      = ARISC_MESSAGE_INITIALIZED;
			pmessage->cb.handler = NULL;
			pmessage->cb.arg     = NULL;

			ARISC_INF("v-f table: cluster %d index %d freq %d vol %d axi_div %d\n",
			pmessage->paras[4], pmessage->paras[0], pmessage->paras[1], pmessage->paras[2], pmessage->paras[3]);

			/* send request message */
			arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

			//check config fail or not
			//ARISC_LOG("before pmessage->result:%x\n", pmessage->result);
			if (pmessage->result) {
			//ARISC_WRN("after pmessage->result:%x\n", pmessage->result);
				ARISC_WRN("config dvfs v-f table [%d][%d] fail\n", cluster, index);
				result = -EINVAL;
				break;
			}

		}
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}
#endif


/*
 * set specific pll target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @pll:     which pll will be set
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int arisc_dvfs_set_cpufreq(unsigned int freq, unsigned int pll, unsigned long mode, arisc_cb_t cb, void *cb_arg)
{
	unsigned int          msg_attr = 0;
	struct arisc_message *pmessage;
	int                   result = 0;

	if (mode & ARISC_DVFS_SYN) {
		msg_attr |= ARISC_MESSAGE_ATTR_HARDSYN;
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(msg_attr);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message
	 *
	 * |paras[0]|paras[1]|
	 * |freq    |pll     |
	 */
	pmessage->type       = ARISC_CPUX_DVFS_REQ;
	pmessage->paras[0]   = freq;
	pmessage->paras[1]   = pll;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;

	ARISC_INF("arisc dvfs request : %d\n", freq);
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* dvfs mode : syn or not */
	if (mode & ARISC_DVFS_SYN) {
		result = pmessage->result;
		arisc_message_free(pmessage);
	}

	return result;
}
EXPORT_SYMBOL(arisc_dvfs_set_cpufreq);
