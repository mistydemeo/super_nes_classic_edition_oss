/*
 * @file drivers/misc/dmard10.c
 * @brief DMARD10 g-sensor Linux device driver
 * @author Domintech Technology Co., Ltd (http://www.domintech.com.tw)
 * @version 1.38
 * @date 2012/10/21
 *
 * @section LICENSE
 *
 *  Copyright 2011 Domintech Technology Co., Ltd
 *
 * 	This software is licensed under the terms of the GNU General Public
 * 	License version 2, as published by the Free Software Foundation, and
 * 	may be copied, distributed, and modified under those terms.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 *
 * @DMT Package version D10_Allwinner_driver v1.6
 *
 *
 */
#include "dmard10.h"
#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/wakelock.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/serio.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <linux/miscdevice.h>

#include <linux/platform_device.h>

#include <mach/hardware.h>
#include <mach/sys_config.h> 
#include <linux/workqueue.h>
#include <linux/init-input.h>


static char OffsetFileName[] = "/data/misc/dmt/offset.txt";
static atomic_t delay;
static atomic_t enable;
static struct delayed_work work;
static struct input_dev *input;
static raw_data offset;
static struct dmt_data dev;

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};


static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)
module_param_named(debug_mask,debug_mask,int,S_IRUGO | S_IWUSR | S_IWGRP);

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x18, I2C_CLIENT_END};
static struct sensor_config_info config_info = {
	.input_type = GSENSOR_TYPE,
};


static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;

	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;

	ret=i2c_transfer(client->adapter, &msg,1);
	return ret;
}

static bool i2c_test(struct i2c_client * client)
{
        int ret,retry;
        uint8_t test_data[1] = { 0 };     //only write a data address.

        for(retry=0; retry < 5; retry++)
        {
                ret = i2c_write_bytes(client, test_data, 1);	//Test i2c.
        	if (ret == 1)
        	        break;
        	msleep(10);
        }

        return ret==1 ? true : false;
} 

/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;

	if(config_info.twi_id == adapter->nr){
                dprintk(DEBUG_INIT ,"%s: addr= %x\n",__func__,client->addr);
                ret = i2c_test(client);
                if(!ret){
        		printk("%s:I2C connection might be something wrong \n",__func__);
        		return -ENODEV;
        	}else{
            	    strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
    		    return 0;
	             }

	}else{
		return -ENODEV;
	}
}

/***** I2C I/O function ***********************************************/
static int device_i2c_rxdata( struct i2c_client *client, unsigned char *rxData, int length)
{
	struct i2c_msg msgs[] = 
	{
		{
		        .addr   = client->addr,
		        .flags  = 0,
		        .len    = 1,
		        .buf    = rxData,
		},
		{
		        .addr   = client->addr,
		        .flags  = I2C_M_RD,
		        .len    = length,
		        .buf    = rxData,
		},
	};

	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		dev_err(&client->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}

	return 0;
}

static int device_i2c_txdata( struct i2c_client *client, unsigned char *txData, int length)
{
	struct i2c_msg msg[] = 
	{
		{
		        .addr   = client->addr,
		        .flags  = 0,
		        .len    = length,
		        .buf    = txData,
		},
	};

	if (i2c_transfer(client->adapter, msg, 1) < 0) {
		dev_err(&client->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}

	return 0;
}

static inline void device_i2c_correct_accel_sign(s16 *val)
{
	*val<<= 3;
}

void device_i2c_merge_register_values(struct i2c_client *client, s16 *val, u8 msb, u8 lsb)
{
	*val = (((u16)msb) << 8) | (u16)lsb;
	device_i2c_correct_accel_sign(val);
}

void device_i2c_read_xyz(struct i2c_client *client, s16 *xyz_p)
{
        u8 buffer[11];
        s16 xyzTmp[SENSOR_DATA_SIZE];
        int i, j;

	/* get xyz high/low bytes, 0x12 */
         buffer[0] = REG_STADR;
        device_i2c_rxdata(client, buffer, 10);

	/* merge to 10-bits value */
         for(i = 0; i < SENSOR_DATA_SIZE; ++i){
                 xyz_p[i] = 0;
		device_i2c_merge_register_values(client, (xyzTmp + i), buffer[2*(i+1)+1], buffer[2*(i+1)]);
	/* transfer to the default layout */
		for(j = 0; j < 3; j++)
			xyz_p[i] += sensorlayout[i][j] * xyzTmp[j];
	}
//	dprintk(DEBUG_INIT,"@DMT@ xyz_p: %04d , %04d , %04d\n", xyz_p[0], xyz_p[1], xyz_p[2]);
}

void gsensor_write_offset_to_file(void)
{
	char data[18];
	unsigned int orgfs;
	struct file *fp;

	sprintf(data,"%5d %5d %5d",offset.u.x,offset.u.y,offset.u.z);
	orgfs = get_fs();
	/* Set segment descriptor associated to kernel space */
	set_fs(KERNEL_DS);
	fp = filp_open(OffsetFileName, O_RDWR | O_CREAT, 0777);
	if(IS_ERR(fp)){
		dmtprintk(KERN_INFO "filp_open %s error!!.\n",OffsetFileName);
	}
	else{
		dmtprintk(KERN_INFO "filp_open %s SUCCESS!!.\n",OffsetFileName);
		fp->f_op->write(fp,data,18, &fp->f_pos);
 		filp_close(fp,NULL);
	}
	set_fs(orgfs);
}

void gsensor_read_offset_from_file(void)
{
	unsigned int orgfs;
	char data[18];
	struct file *fp;
	int ux,uy,uz;
	orgfs = get_fs();
	/* Set segment descriptor associated to kernel space */
	set_fs(KERNEL_DS);

	fp = filp_open(OffsetFileName, O_RDWR , 0);
	dmtprintk(KERN_INFO "%s,try file open !\n",__func__);
	if(IS_ERR(fp)){
		dmtprintk(KERN_INFO "%s:Sorry,file open ERROR !\n",__func__);
		offset.u.x=0;offset.u.y=0;offset.u.z=0;
#if AUTO_CALIBRATION
		/* get acceleration average reading */
		gsensor_calibrate();
		gsensor_write_offset_to_file();
#endif
	}
	else{
		dmtprintk("filp_open %s SUCCESS!!.\n",OffsetFileName);
		fp->f_op->read(fp,data,18, &fp->f_pos);
		dmtprintk("filp_read result %s\n",data);
		sscanf(data,"%d %d %d",&ux,&uy,&uz);
		offset.u.x=ux;
		offset.u.y=uy;
		offset.u.z=uz;
		filp_close(fp,NULL);
	}
	set_fs(orgfs);
}
int gsensor_calibrate(void)
{	
	raw_data avg;
	int i, j;
	long xyz_acc[SENSOR_DATA_SIZE];   
  	s16 xyz[SENSOR_DATA_SIZE];

	/* initialize the accumulation buffer */
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i) 
		xyz_acc[i] = 0;

	for(i = 0; i < AVG_NUM; i++) {
		device_i2c_read_xyz(dev.client, (s16 *)&xyz);
		for(j = 0; j < SENSOR_DATA_SIZE; ++j)
			xyz_acc[j] += xyz[j];
  	}
	/* calculate averages */
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i) 
		avg.v[i] = (s16) (xyz_acc[i] / AVG_NUM);

	if(avg.v[2] < 0){
		offset.u.x =  avg.v[0] ;
		offset.u.y =  avg.v[1] ;
		offset.u.z =  avg.v[2] + DEFAULT_SENSITIVITY;
		return CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_POSITIVE;
	}
	else{	
		offset.u.x =  avg.v[0] ;
		offset.u.y =  avg.v[1] ;
		offset.u.z =  avg.v[2] - DEFAULT_SENSITIVITY;
		return CONFIG_GSEN_CALIBRATION_GRAVITY_ON_Z_NEGATIVE;
	}
	return 0;
}

int gsensor_reset(struct i2c_client *client){
	unsigned char buffer[7], buffer2[2];
	/* 1. check D10 , VALUE_STADR = 0x55 , VALUE_STAINT = 0xAA */
	buffer[0] = REG_STADR;
	buffer2[0] = REG_STAINT;

	device_i2c_rxdata(client, buffer, 2);
	device_i2c_rxdata(client, buffer2, 2);

	if( buffer[0] == VALUE_STADR || buffer2[0] == VALUE_STAINT){
		dprintk(DEBUG_INIT ," REG_STADR_VALUE = %d , REG_STAINT_VALUE = %d\n", buffer[0], buffer2[0]);
		dprintk(DEBUG_INIT, " %s DMT_DEVICE_NAME registered I2C driver!\n",__FUNCTION__);
		dev.client = client;
	}
	else{
		dprintk(DEBUG_INIT, " %s gsensor I2C err @@@ REG_STADR_VALUE = %d , REG_STAINT_VALUE = %d \n", 
						__func__, buffer[0], buffer2[0]);
		dev.client = NULL;
		return -1;
	}
	/* 2. Powerdown reset */
	buffer[0] = REG_PD;
	buffer[1] = VALUE_PD_RST;
	device_i2c_txdata(client, buffer, 2);
	/* 3. ACTR => Standby mode => Download OTP to parameter reg => Standby mode => Reset data path => Standby mode */
	buffer[0] = REG_ACTR;
	buffer[1] = MODE_Standby;
	buffer[2] = MODE_ReadOTP;
	buffer[3] = MODE_Standby;
	buffer[4] = MODE_ResetDataPath;
	buffer[5] = MODE_Standby;
	device_i2c_txdata(client, buffer, 6);
	/* 4. OSCA_EN = 1 ,TSTO = b'000(INT1 = normal, TEST0 = normal) */
	buffer[0] = REG_MISC2;
	buffer[1] = VALUE_MISC2_OSCA_EN;
	device_i2c_txdata(client, buffer, 2);
	/* 5. AFEN = 1(AFE will powerdown after ADC) */
	buffer[0] = REG_AFEM;
	buffer[1] = VALUE_AFEM_AFEN_Normal;
	buffer[2] = VALUE_CKSEL_ODR_100_204;
	buffer[3] = VALUE_INTC;	
	buffer[4] = VALUE_TAPNS_Ave_2;
	buffer[5] = 0x00;	// DLYC, no delay timing
	buffer[6] = 0x07;	// INTD=1 (push-pull), INTA=1 (active high), AUTOT=1 (enable T)
	device_i2c_txdata(client, buffer, 7);
	/* 6. write TCGYZ & TCGX */
	buffer[0] = REG_WDAL;	// REG:0x01
	buffer[1] = 0x00;		// set TC of Y,Z gain value
	buffer[2] = 0x00;		// set TC of X gain value
	buffer[3] = 0x03;		// Temperature coefficient of X,Y,Z gain
	device_i2c_txdata(client, buffer, 4);

	buffer[0] = REG_ACTR;			// REG:0x00
	buffer[1] = MODE_Standby;		// Standby
	buffer[2] = MODE_WriteOTPBuf;	// WriteOTPBuf 
	buffer[3] = MODE_Standby;		// Standby
	device_i2c_txdata(client, buffer, 4);	
	//buffer[0] = REG_TCGYZ;
	//device_i2c_rxdata(client, buffer, 2);
	//GSE_LOG(" TCGYZ = %d, TCGX = %d  \n", buffer[0], buffer[1]);

	/* 7. Activation mode */
	buffer[0] = REG_ACTR;
	buffer[1] = MODE_Active;
	device_i2c_txdata(client, buffer, 2);
	return 0;
}

void gsensor_set_offset(int val[3])
{
	int i;
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		offset.v[i] = (s16) val[i];
}



static int device_open(struct inode *inode, struct file *filp)
{
	return 0; 
}

static ssize_t device_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

static ssize_t device_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	s16 xyz[SENSOR_DATA_SIZE];
	int i;

	device_i2c_read_xyz(dev.client, (s16 *)&xyz);
	//offset compensation 
	for(i = 0; i < SENSOR_DATA_SIZE; i++)
		xyz[i] -= offset.v[i];

	if(copy_to_user(buf, &xyz, count))
		return -EFAULT;
	return count;
}


static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)//struct inode *inode, 
{
	int err = 0, ret = 0, i;
	int intBuf[SENSOR_DATA_SIZE];
	s16 xyz[SENSOR_DATA_SIZE];
	//check type and number
	if (_IOC_TYPE(cmd) != IOCTL_MAGIC) return -ENOTTY;

	//check user space pointer is valid
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd)
	{
		case SENSOR_RESET:
			gsensor_reset(dev.client);
			return ret;

		case SENSOR_CALIBRATION:
			gsensor_calibrate();
			dmtprintk("Sensor_calibration:%d %d %d\n",offset.u.x,offset.u.y,offset.u.z);;
			gsensor_write_offset_to_file();

			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = offset.v[i];

			ret = copy_to_user((int *)arg, &intBuf, sizeof(intBuf));
			return ret;

		case SENSOR_GET_OFFSET:
			gsensor_read_offset_from_file();

			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = offset.v[i];

			ret = copy_to_user((int *)arg, &intBuf, sizeof(intBuf));
			return ret;

		case SENSOR_SET_OFFSET:
			ret = copy_from_user(&intBuf, (int *)arg, sizeof(intBuf));
			gsensor_set_offset(intBuf);
			gsensor_write_offset_to_file();
			return ret;

		case SENSOR_READ_ACCEL_XYZ:
			device_i2c_read_xyz(dev.client, (s16 *)&xyz);

			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = xyz[i] - offset.v[i];

		  	ret = copy_to_user((int*)arg, &intBuf, sizeof(intBuf));
			return ret;

		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}

	return 0;
}

static int device_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static int sensor_close_dev(struct i2c_client *client)
{
	char buffer[3];
	buffer[0] = REG_ACTR;
	buffer[1] = MODE_Standby;
	buffer[2] = MODE_Off;
	dprintk(DEBUG_INIT,"%s\n",__FUNCTION__);	
	return device_i2c_txdata(client,buffer, 3);
}

static int device_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	dprintk(DEBUG_SUSPEND,"Gsensor enter 2 level suspend!!\n");
	return sensor_close_dev(client);
}

static void DMT_sysfs_update_active_status(int en)
{
	unsigned long dmt_delay;
	if(en == 1)
	{
		dmt_delay=msecs_to_jiffies(atomic_read(&delay));
		if(dmt_delay<1)
			dmt_delay=1;

		dprintk(DEBUG_INIT,"schedule_delayed_work start with delay time=%lu\n",dmt_delay);
		schedule_delayed_work(&work,dmt_delay);
	}
	else if(en == 0){
	        printk("en = %d \n",en);
		cancel_delayed_work_sync(&work);
	}
}

static bool get_value_as_int(char const *buf, size_t size, int *value)
{
	long tmp;

	if (size == 0)
		return false;

	/* maybe text format value */
	if ((buf[0] == '0') && (size > 1)) {
		if ((buf[1] == 'x') || (buf[1] == 'X')) {
			/* hexadecimal format */
			if (0 != strict_strtol(buf, 16, &tmp))
				return false;
		} else {
			/* octal format */
			if (0 != strict_strtol(buf, 8, &tmp))
				return false;
		}
	} else {
		/* decimal format */
		if (0 != strict_strtol(buf, 10, &tmp))
			return false;
	}
	if (tmp > INT_MAX)
		return false;

	*value = tmp;

	return true;
}
static bool get_value_as_int64(char const *buf, size_t size, long long *value)
{
	long long tmp;

	if (size == 0)
		return false;

	/* maybe text format value */
	if ((buf[0] == '0') && (size > 1)) {
		if ((buf[1] == 'x') || (buf[1] == 'X')) {
			/* hexadecimal format */
			if (0 != strict_strtoll(buf, 16, &tmp))
				return false;
		} else {
			/* octal format */
			if (0 != strict_strtoll(buf, 8, &tmp))
				return false;
		}
	} else {
		/* decimal format */
		if (0 != strict_strtoll(buf, 10, &tmp))
			return false;
	}

	if (tmp > LLONG_MAX)
		return false;

	*value = tmp;

	return true;
}
static ssize_t DMT_enable_acc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char str[2][16]={"ACC enable OFF","ACC enable ON"};
	int flag;
	flag=atomic_read(&enable); 

	return sprintf(buf, "%s\n", str[flag]);
}

static ssize_t DMT_enable_acc_store( struct device *dev, struct device_attribute *attr, 
                                     char const *buf, size_t count)
{
	int en, old_en;
	dprintk(DEBUG_INIT,"%s:buf=%x %x\n",__func__,buf[0],buf[1]);
        if (false == get_value_as_int(buf, count, &en))
		return -EINVAL;
	en = en ? 1 : 0;
	old_en = atomic_read(&enable);
	if(en == old_en) {
	        return count;
	}else {
	        atomic_set(&enable,en);
	        DMT_sysfs_update_active_status(en);
	}
	return 1;
}

static ssize_t DMT_delay_acc_show( struct device *dev, struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%d\n", atomic_read(&delay));
}

static ssize_t DMT_delay_acc_store( struct device *dev, struct device_attribute *attr,
                                   char const *buf, size_t count)
{
	long long data, old_data;
	if (false == get_value_as_int64(buf, count, &data))
		return -EINVAL;

	dmtprintk("Driver attribute set delay =%llu\n",data);
	old_data = atomic_read(&delay);

	if(old_data == data) {
	        return count;
	} else {
	        atomic_set(&delay, (unsigned int) data);
	}

	return count;
}

static DEVICE_ATTR(enable, 0666,
		DMT_enable_acc_show, DMT_enable_acc_store);

static DEVICE_ATTR(delay, 0666,
		DMT_delay_acc_show, DMT_delay_acc_store);

static struct attribute* dmard10_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	NULL
};

static const struct attribute_group dmard10_group =
{
	.attrs = dmard10_attrs,
};

int input_init(void)
{
	int err=0;
	input=input_allocate_device();
	if (!input)
            return -ENOMEM;
	else
	   dmtprintk("input device allocate Success !!\n");
	/* Setup input device */
	set_bit(EV_ABS, input->evbit);
	/* Accelerometer [-78.5, 78.5]m/s2 in Q16*/
	input_set_abs_params(input, ABS_X, -5144576, 5144576, 0, 0);
	input_set_abs_params(input, ABS_Y, -5144576, 5144576, 0, 0);
	input_set_abs_params(input, ABS_Z, -5144576, 5144576, 0, 0);

	/* Set InputDevice Name */
	input->name = INPUT_NAME_ACC;

	/* Register */
	err = input_register_device(input);
	if (err) {
		printk("input_register_device ERROR !!\n");
		input_free_device(input);
		return err;
	}
	dprintk(DEBUG_INIT,"input_register_device SUCCESS %d !! \n",err);
	return err;
}

static void DMT_work_func(struct work_struct *fakework)
{
	int i;
	static int firsttime=0;
	s16 xyz[SENSOR_DATA_SIZE];
	unsigned long t=atomic_read(&delay);
  	unsigned long dmt_delay = msecs_to_jiffies(t);
	if(!firsttime)
	{
		gsensor_read_offset_from_file();	
	 	firsttime=1;
	}
//	dprintk(DEBUG_DATA_INFO,"t=%lu , dmt_delay=%lu\n", t, dmt_delay);

  	device_i2c_read_xyz(dev.client, (s16 *)&xyz);
	PRINT_X_Y_Z(xyz[0], xyz[1], xyz[2]);
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
     		xyz[i] -= offset.v[i];

//	dprintk(DEBUG_DATA_INFO,"xyz[0]:0x%x,xyz[1]:0x%x,xyz[2]:0x%x", xyz[0], xyz[1], xyz[2]);

	input_report_abs(input, ABS_X, xyz[0]);
	input_report_abs(input, ABS_Y, xyz[1]);
	input_report_abs(input, ABS_Z, xyz[2]);
	input_sync(input);

	if(dmt_delay<1)
		dmt_delay=1;
	schedule_delayed_work(&work, dmt_delay);

}
static int __devinit device_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int i;
	int ret = -1;

	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		offset.v[i] = 0;

  	ret = sysfs_create_group(&input->dev.kobj, &dmard10_group);

	gsensor_reset(client);

	return 0;
}

static int __devexit device_i2c_remove(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &dmard10_group);
	//cdev_del(&dev.cdev);
	//unregister_chrdev_region(dev.devno, 1);
	return 0;
}

static int device_i2c_resume(struct i2c_client *client)
{
	dprintk(DEBUG_SUSPEND,"Gsensor 2 level resume!!\n");
	return gsensor_reset(client);
}
struct file_operations dmt_g_sensor_fops = 
{
	.owner          = THIS_MODULE,
	.read           = device_read,
	.write          = device_write,
	.unlocked_ioctl = device_ioctl,
	.open           = device_open,
	.release        = device_close,
};

static const struct i2c_device_id device_i2c_ids[] = 
{
	{DEVICE_I2C_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, device_i2c_ids);

static struct i2c_driver device_i2c_driver = {
	.class  = I2C_CLASS_HWMON,
	.driver	= {
		.owner = THIS_MODULE,
		.name  = DEVICE_I2C_NAME,
		},
	.class          = I2C_CLASS_HWMON,
	.probe          = device_i2c_probe,
	.remove	        = __devexit_p(device_i2c_remove),
	.suspend        = device_i2c_suspend,
	.resume	        = device_i2c_resume,
	.id_table       = device_i2c_ids,
	.address_list	= normal_i2c,
};
static int __init device_init(void)
{
	int err = 0;

	dprintk(DEBUG_INIT,"dmard10 init----!\n");
        if(input_fetch_sysconfig_para(&(config_info.input_type))){
		printk("%s: err.\n", __func__);
		return -1;
	}
	if(config_info.sensor_used == 0){
		printk("*** used set to 0 !\n");
		printk("*** if use sensor,please put the sys_config.fex gsensor_used set to 1. \n");
		return 0;
	}

	INIT_DELAYED_WORK(&work, DMT_work_func);
	dprintk(DEBUG_INIT, "DMT: INIT_DELAYED_WORK\n");

	err=input_init();
	if(err)
		printk("%s:input_init fail, error code= %d\n", __func__, err);

	device_i2c_driver.detect = gsensor_detect;

	return i2c_add_driver(&device_i2c_driver);
}

static void __exit device_exit(void)
{
	dprintk(DEBUG_INIT, "device_exit!\n");
	i2c_del_driver(&device_i2c_driver);
         input_unregister_device(input);
	cancel_delayed_work_sync(&work);
}

//*********************************************************************************************************

MODULE_AUTHOR("DMT_RD");
MODULE_DESCRIPTION("DMT Gsensor Driver");
MODULE_LICENSE("GPL");

module_init(device_init);
module_exit(device_exit);

