/*
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
 *
 * Copyright (c) 2011
 *
 * ChangeLog
 *
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/arisc/arisc.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include <mach/sys_config.h>
#include <linux/gpio.h>

#define INPUT_DEV_NAME   	   ("sunxi-knob")

#define REPORT_INTERVAL msecs_to_jiffies(80)
static int switch_gpio = -1;

enum {
	DEBUG_INIT       = 1U << 0,
	DEBUG_VOLUME     = 1U << 1,
	DEBUG_BRIGHTNESS = 1U << 2,
	DEBUG_OTHER      = 1U << 3,
};

static u32 debug_mask = 0x0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk("###sunxi-knob###" KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);


#ifdef CONFIG_HAS_EARLYSUSPEND
struct sunxi_knob_data {
	struct early_suspend early_suspend;
};
#endif

static struct input_dev *sunxitb_dev;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct sunxi_knob_data *keyboard_data;
#endif

//停用设备
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sunxi_knob_suspend(struct early_suspend *h)
{
	dprintk(DEBUG_INIT,"[%s] enter standby state: %d. \n", __FUNCTION__, (int)standby_type);

	if (NORMAL_STANDBY == standby_type) {

		;
		/*process for super standby*/
	} else if(SUPER_STANDBY == standby_type) {
		;
	}

	return ;
}

//重新唤醒
static void sunxi_knob_resume(struct early_suspend *h)
{
	dprintk(DEBUG_INIT,"[%s] return from standby state: %d. \n", __FUNCTION__, (int)standby_type);
	/*process for normal standby*/
	if (NORMAL_STANDBY == standby_type) {
		;
		/*process for super standby*/
	} else if(SUPER_STANDBY == standby_type) {
	}

	return ;
}
#else

#endif

static irqreturn_t power_irq_service(int irq, void *dev_id){
	int value ;
	value = !gpio_get_value(switch_gpio);
	printk("power_irq_service power_switch:%d\n", value);
	input_report_key(sunxitb_dev, KEY_POWER, value);
	input_sync(sunxitb_dev);
	return IRQ_HANDLED;
}


static ssize_t knob_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret ;
	ret = !gpio_get_value(switch_gpio);
	dprintk(DEBUG_INIT,"power_on = %d \n",ret);
	return sprintf(buf,"%d\n",ret);
}

static ssize_t knob_state_store(struct device *dev,struct device_attribute *attr,
		const char *buf, size_t size)
{
	return 0;
}

static DEVICE_ATTR(power_switch, S_IRUGO|S_IWUSR|S_IWGRP, knob_state_show, knob_state_store);

static struct attribute *knob_attributes[] = {
	&dev_attr_power_switch.attr,
	NULL
};

static struct attribute_group dev_attr_group = {
	.attrs = knob_attributes,
};

static const struct attribute_group *dev_attr_groups[] = {
	&dev_attr_group,
	NULL,
};

static void knob_release(struct device *dev)
{

}

static struct device knob_dev = {
	.init_name = "knob",
	.release = knob_release,
};

static int __init sunxi_knob_init(void)
{
	int err =0;

	script_item_u val;
	script_item_value_type_e type;
	int irq_num;
	printk("sunxitb_init \n");
	
	sunxitb_dev = input_allocate_device();
	if (!sunxitb_dev) {
		dprintk(DEBUG_INIT,KERN_ERR "sunxitb: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}
	sunxitb_dev->name = INPUT_DEV_NAME;
	sunxitb_dev->phys = "sunxitb/input0";
	sunxitb_dev->id.bustype = BUS_HOST;
	sunxitb_dev->id.vendor = 0x0001;
	sunxitb_dev->id.product = 0x0001;
	sunxitb_dev->id.version = 0x0100;
#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sunxitb_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	dprintk(DEBUG_INIT,"REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	sunxitb_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif
	set_bit(KEY_POWER, sunxitb_dev->keybit);

	err = input_register_device(sunxitb_dev);
	if (err)
		goto fail2;

	type = script_get_item("platform", "power_switch", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		printk("get power_switch gpio failed\n");
	else{
		switch_gpio = val.gpio.gpio;
		printk("gpio num = %d\n",switch_gpio);
		printk("gpio mul_sel = %d\n",val.gpio.mul_sel);
		irq_num = gpio_to_irq(switch_gpio);
		request_irq(irq_num, power_irq_service, 
			IRQF_TRIGGER_RISING| IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, 
			"power switch irq", NULL);
	}

	input_report_key(sunxitb_dev, KEY_POWER, !gpio_get_value(switch_gpio));
	input_sync(sunxitb_dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	dprintk(DEBUG_INIT,"==register_early_suspend =\n");
	keyboard_data = kzalloc(sizeof(*keyboard_data), GFP_KERNEL);
	if (keyboard_data == NULL) {
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	keyboard_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 3;
	keyboard_data->early_suspend.suspend = sunxi_knob_suspend;
	keyboard_data->early_suspend.resume = sunxi_knob_resume;
	register_early_suspend(&keyboard_data->early_suspend);
#endif

	knob_dev.groups = dev_attr_groups;
	err = device_register(&knob_dev);
	if (err) {
		printk("%s register knob driver as misc device error\n",
				__FUNCTION__);
	}

	return 0;
#ifdef CONFIG_HAS_EARLYSUSPEND
err_alloc_data_failed:
#endif
fail2:
	input_free_device(sunxitb_dev);
	free_irq(switch_gpio, NULL);
	gpio_free(switch_gpio);
fail1:
	;
	printk("sunxi_knob_init failed. \n");

	return err;
}

static void __exit sunxi_knob_exit(void)
{
	device_unregister(&knob_dev);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&keyboard_data->early_suspend);
#endif
	input_unregister_device(sunxitb_dev);
	free_irq(switch_gpio, NULL);
	gpio_free(switch_gpio);
}

module_init(sunxi_knob_init);
module_exit(sunxi_knob_exit);


MODULE_AUTHOR("Donghe");
MODULE_DESCRIPTION("sunxi-knob driver");
MODULE_LICENSE("GPL");


