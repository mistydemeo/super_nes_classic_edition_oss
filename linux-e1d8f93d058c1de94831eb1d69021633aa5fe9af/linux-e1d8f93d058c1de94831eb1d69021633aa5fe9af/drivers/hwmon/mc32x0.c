/*  Date: 2011/4/8 11:00:00
 *  Revision: 2.5
 *
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 *
 * file mc32x0.c
 * brief This file contains all function implementations for the mc32x0 in linux
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>


#include <linux/init-input.h>
#include <mach/hardware.h>
#include <linux/fs.h>

#undef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_PM

#if 0
#define mcprintkreg(x...) printk(x)
#else
#define mcprintkreg(x...)
#endif

#if 0
#define mcprintkfunc(x...) printk(x)
#else
#define mcprintkfunc(x...)
#endif

#if 1
#define GSE_ERR(x...) 	printk(x)
#define GSE_LOG(x...) 	printk(x)
#else
#define GSE_ERR(x...)
#define GSE_LOG(x...)
#endif



#define SENSOR_NAME 			"mc32x0"
#define SENSOR_DATA_SIZE	3
#define AVG_NUM 16
/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x4c,I2C_CLIENT_END};

static __u32 twi_id = 0;
//volatile unsigned char mc32x0_on_off=0;
//static int mc32x0_pin_hd;
#define G_0		ABS_X
#define G_1		ABS_Y
#define G_2		ABS_Z
#define G_0_REVERSE        1
#define G_1_REVERSE        1
#define G_2_REVERSE        1
#define SENSOR_DMARD_IOCTL_BASE 		234

#define IOCTL_SENSOR_SET_DELAY_ACCEL   	_IO(SENSOR_DMARD_IOCTL_BASE, 100)
#define IOCTL_SENSOR_GET_DELAY_ACCEL   	_IO(SENSOR_DMARD_IOCTL_BASE, 101)
#define IOCTL_SENSOR_GET_STATE_ACCEL   	_IO(SENSOR_DMARD_IOCTL_BASE, 102)
#define IOCTL_SENSOR_SET_STATE_ACCEL		_IO(SENSOR_DMARD_IOCTL_BASE, 103)
#define IOCTL_SENSOR_GET_DATA_ACCEL		_IO(SENSOR_DMARD_IOCTL_BASE, 104)

#define IOCTL_MSENSOR_SET_DELAY_MAGNE   	_IO(SENSOR_DMARD_IOCTL_BASE, 200)
#define IOCTL_MSENSOR_GET_DATA_MAGNE		_IO(SENSOR_DMARD_IOCTL_BASE, 201)
#define IOCTL_MSENSOR_GET_STATE_MAGNE   	_IO(SENSOR_DMARD_IOCTL_BASE, 202)
#define IOCTL_MSENSOR_SET_STATE_MAGNE	_IO(SENSOR_DMARD_IOCTL_BASE, 203)

#define IOCTL_SENSOR_GET_NAME   _IO(SENSOR_DMARD_IOCTL_BASE, 301)
#define IOCTL_SENSOR_GET_VENDOR   _IO(SENSOR_DMARD_IOCTL_BASE, 302)

#define IOCTL_SENSOR_GET_CONVERT_PARA   _IO(SENSOR_DMARD_IOCTL_BASE, 401)

#define SENSOR_CALIBRATION   	_IOWR(SENSOR_DMARD_IOCTL_BASE,  402, int[SENSOR_DATA_SIZE])


#define mc32x0_CONVERT_PARAMETER       (1.5f * (9.80665f) / 256.0f)
#define mc32x0_DISPLAY_NAME         "mc32x0"
#define mc32x0_DIPLAY_VENDOR        "domintech"

#define X_OUT 					0x41
#define CONTROL_REGISTER		0x44
#define SW_RESET 				0x53
#define WHO_AM_I 				0x0f
#define WHO_AM_I_VALUE 		0x06

#define MC32X0_AXIS_X		   0
#define MC32X0_AXIS_Y		   1
#define MC32X0_AXIS_Z		   2
#define MC32X0_AXES_NUM 	   3
#define MC32X0_DATA_LEN 	   6

#define MC32X0_XOUT_REG						0x00
#define MC32X0_YOUT_REG						0x01
#define MC32X0_ZOUT_REG						0x02
#define MC32X0_Tilt_Status_REG				0x03
#define MC32X0_Sampling_Rate_Status_REG		0x04
#define MC32X0_Sleep_Count_REG				0x05
#define MC32X0_Interrupt_Enable_REG			0x06
#define MC32X0_Mode_Feature_REG				0x07
#define MC32X0_Sample_Rate_REG				0x08
#define MC32X0_Tap_Detection_Enable_REG		0x09
#define MC32X0_TAP_Dwell_Reject_REG			0x0a
#define MC32X0_DROP_Control_Register_REG	0x0b
#define MC32X0_SHAKE_Debounce_REG			0x0c
#define MC32X0_XOUT_EX_L_REG				0x0d
#define MC32X0_XOUT_EX_H_REG				0x0e
#define MC32X0_YOUT_EX_L_REG				0x0f
#define MC32X0_YOUT_EX_H_REG				0x10
#define MC32X0_ZOUT_EX_L_REG				0x11
#define MC32X0_ZOUT_EX_H_REG				0x12
#define MC32X0_CHIP_ID_REG					0x18
#define MC32X0_RANGE_Control_REG			0x20
#define MC32X0_SHAKE_Threshold_REG			0x2B
#define MC32X0_UD_Z_TH_REG					0x2C
#define MC32X0_UD_X_TH_REG					0x2D
#define MC32X0_RL_Z_TH_REG					0x2E
#define MC32X0_RL_Y_TH_REG					0x2F
#define MC32X0_FB_Z_TH_REG					0x30
#define MC32X0_DROP_Threshold_REG			0x31
#define MC32X0_TAP_Threshold_REG			0x32
#define MC32X0_HIGH_END	0x01
/*******MC3210/20 define this**********/


#define MCUBE_8G_14BIT  0x10

#define DOT_CALI

#define MC32X0_LOW_END 0x02
/*******mc32x0 define this**********/

#define MCUBE_1_5G_8BIT 0x20
//#define MCUBE_1_5G_8BIT_TAP
//#define MCUBE_1_5G_6BIT
#define MC32X0_MODE_DEF 				0x43

#define MC32X0ADDRESS           0x4c

#define mc32x0_I2C_NAME			"mc32x0"
#define A10ASENSOR_DEV_COUNT	        1
#define A10ASENSOR_DURATION_MAX                        200
#define A10ASENSOR_DURATION_MIN                        10
#define A10ASENSOR_DURATION_DEFAULT	        20

#define MAX_RETRY				20
#define INPUT_FUZZ  0
#define INPUT_FLAT  0

#define AUTO_CALIBRATION 0

static unsigned char  McubeID=0;
#ifdef DOT_CALI
#define CALIB_PATH				"/data/data/com.mcube.acc/files/mcube-calib.txt"
#define DATA_PATH			   "/sdcard/mcube-register-map.txt"

typedef struct {
	unsigned short	x;		/*< X axis */
	unsigned short	y;		/*< Y axis */
	unsigned short	z;		/*< Z axis */
} GSENSOR_VECTOR3D;

static GSENSOR_VECTOR3D gsensor_gain;
static struct miscdevice mc32x0_device;

static struct file * fd_file = NULL;

static mm_segment_t oldfs;
//add by Liang for storage offset data
static unsigned char offset_buf[9];
static signed int offset_data[3];
s16 G_RAW_DATA[3];
static signed int gain_data[3];
static signed int enable_RBM_calibration = 0;
#endif

#ifdef DOT_CALI

#if 1
#define GSENSOR						0x95
#define GSENSOR_IOCTL_INIT                              _IO(GSENSOR,  0x01)
#define GSENSOR_IOCTL_READ_CHIPINFO                     _IOR(GSENSOR, 0x02, int)
#define GSENSOR_IOCTL_READ_SENSORDATA                   _IOR(GSENSOR, 0x03, int)
#define GSENSOR_IOCTL_READ_OFFSET			_IOR(GSENSOR, 0x04, GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_GAIN				_IOR(GSENSOR, 0x05, GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_RAW_DATA			_IOR(GSENSOR, 0x06, int)
//#define GSENSOR_IOCTL_SET_CALI				_IOW(GSENSOR, 0x06, SENSOR_DATA)
#define GSENSOR_IOCTL_GET_CALI				_IOW(GSENSOR, 0x07, SENSOR_DATA)
#define GSENSOR_IOCTL_CLR_CALI				_IO(GSENSOR, 0x08)
#define GSENSOR_MCUBE_IOCTL_READ_RBM_DATA		_IOR(GSENSOR, 0x09, SENSOR_DATA)
#define GSENSOR_MCUBE_IOCTL_SET_RBM_MODE		_IO(GSENSOR, 0x0a)
#define GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE		_IO(GSENSOR, 0x0b)
#define GSENSOR_MCUBE_IOCTL_SET_CALI			_IOW(GSENSOR, 0x0c, SENSOR_DATA)
#define GSENSOR_MCUBE_IOCTL_REGISTER_MAP		_IO(GSENSOR, 0x0d)
#define GSENSOR_IOCTL_SET_CALI_MODE   			_IOW(GSENSOR, 0x0e,int)
#else

#define GSENSOR_IOCTL_INIT                              0xa1
#define GSENSOR_IOCTL_READ_CHIPINFO                     0xa2
#define GSENSOR_IOCTL_READ_SENSORDATA                   0xa3
#define GSENSOR_IOCTL_READ_OFFSET                       0xa4
#define GSENSOR_IOCTL_READ_GAIN                         0xa5
#define GSENSOR_IOCTL_READ_RAW_DATA                     0xa6
#define GSENSOR_IOCTL_SET_CALI                          0xa7
#define GSENSOR_IOCTL_GET_CALI                          0xa8
#define GSENSOR_IOCTL_CLR_CALI                          0xa9

#define GSENSOR_MCUBE_IOCTL_READ_RBM_DATA		0xaa
#define GSENSOR_MCUBE_IOCTL_SET_RBM_MODE		0xab
#define GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE		0xac
#define GSENSOR_MCUBE_IOCTL_SET_CALI			0xad
#define GSENSOR_MCUBE_IOCTL_REGISTER_MAP		0xae
#define GSENSOR_IOCTL_SET_CALI_MODE   			0xaf
#endif

typedef struct{
    int x;
	int y;
	int z;
}SENSOR_DATA;

static int load_cali_flg = 0;

#endif

#define MC32X0_WAKE					1
#define MC32X0_SNIFF					2
#define MC32X0_STANDBY					3

struct dev_data {
	struct i2c_client *client;
};
static struct dev_data dev;

/* Addresses to scan */
//static const unsigned short normal_i2c[2] = {MC32X0ADDRESS, I2C_CLIENT_END};
//static __u32 twi_id = 0;

/*
typedef union {
	struct {
		s16	x;
		s16	y;
		s16	z;
	} u;
	s16	v[SENSOR_DATA_SIZE];
} raw_data;
static raw_data offset;
*/

struct acceleration {
	int x;
	int y;
	int z;
};

//void gsensor_write_offset_to_file(void);
//void gsensor_read_offset_from_file(void);
//char OffsetFileName[] = "/data/misc/dmt/offset.txt";
static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
};

static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);


enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};

struct mc32x0_data {
        struct mutex lock;
        struct i2c_client *client;
        struct work_struct  work;
        struct workqueue_struct *mc32x0_wq;
        struct hrtimer timer;
        struct device *device;
        struct input_dev *input_dev;
        int use_count;
        int enabled;
        volatile unsigned int duration;
        int use_irq;
        int irq;
        unsigned long irqflags;
        int gpio;
        unsigned int map[3];
        int inv[3];
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

volatile static short a10asensor_duration = A10ASENSOR_DURATION_DEFAULT;
volatile static short a10asensor_state_flag = 1;

static ssize_t mc32x0_map_show(struct device *dev, struct device_attribute *attr,char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mc32x0_data *data;
	int i;
	data = i2c_get_clientdata(client);
	for (i = 0; i< 3; i++) {
		if(data->inv[i] == 1) {
			switch(data->map[i]) {
				case ABS_X:
					buf[i] = 'x';
					break;
				case ABS_Y:
					buf[i] = 'y';
					break;
				case ABS_Z:
					buf[i] = 'z';
					break;
				default:
					buf[i] = '_';
					break;
			}
		} else {
			switch(data->map[i])
			{
				case ABS_X:
					buf[i] = 'X';
					break;
				case ABS_Y:
					buf[i] = 'Y';
					break;
				case ABS_Z:
					buf[i] = 'Z';
					break;
				default:
					buf[i] = '-';
					break;
			}
		}
	}
	sprintf(buf+3,"\r\n");
	return 5;
}

//Function as i2c_master_send, and return 1 if operation is successful.
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

static bool gsensor_i2c_test(struct i2c_client * client)
{
	int ret, retry;
	uint8_t test_data[1] = { 0 };	//only write a data address.

	for(retry=0; retry < 2; retry++) {
		ret =i2c_write_bytes(client, test_data, 1);	//Test i2c.
		if (ret == 1)
			break;
		msleep(5);
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

	dprintk(DEBUG_INIT, "%s enter \n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;

	if(twi_id == adapter->nr){
                pr_info("%s: addr= %x\n",__func__,client->addr);
                ret = gsensor_i2c_test(client);
                if(!ret){
                        pr_info("%s:I2C connection might be something wrong or maybe the other gsensor equipment! \n",__func__);
            	       return -ENODEV;
                }else{
                         pr_info("I2C connection sucess!\n");
                         strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
                         return 0;
    	        }

	}else{
	         printk("%s   twi_id does not match !!\n",__func__);
		return -ENODEV;
	}
}

int mc32x0_set_image (struct i2c_client *client)
{
	int comres =0;
	unsigned char data;


        data = i2c_smbus_read_byte_data(client, 0x3B);
	//comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x3B, &data, 1 );
	if((data == 0x19)||(data == 0x29))
		McubeID = 0x22;
	else if((data == 0x90)||(data == 0xA8)||(data == 0x88))
		McubeID = 0x11;
	else
		McubeID = 0;

	if(McubeID &MCUBE_8G_14BIT) {
		//#ifdef MCUBE_8G_14BIT
		data = MC32X0_MODE_DEF;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );
                i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );
                i2c_smbus_write_byte_data(client, MC32X0_Sleep_Count_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );
                i2c_smbus_write_byte_data(client, MC32X0_Sample_Rate_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );
                i2c_smbus_write_byte_data(client, MC32X0_Tap_Detection_Enable_REG,data);
		data = 0x3F;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );
                i2c_smbus_write_byte_data(client, MC32X0_RANGE_Control_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_Interrupt_Enable_REG,data);
#ifdef DOT_CALI
                gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 1024;
#endif
	//#endif
	} else if(McubeID &MCUBE_1_5G_8BIT) {
		#ifdef MCUBE_1_5G_8BIT
		data = MC32X0_MODE_DEF;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_Sleep_Count_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_Sample_Rate_REG,data);
		data = 0x02;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_RANGE_Control_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_Tap_Detection_Enable_REG,data);
		data = 0x00;
		//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );
		i2c_smbus_write_byte_data(client, MC32X0_Interrupt_Enable_REG,data);
#ifdef DOT_CALI
		gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 86;
#endif
		#endif
	}

	data = 0x41;
	//comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );
        i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG,data);
	//MC32X0_rbm(0,0);
	return comres;
}

static ssize_t mc32x0_map_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mc32x0_data *data;
	int i;
	data = i2c_get_clientdata(client);

	if(count < 3) return -EINVAL;

	for(i = 0; i< 3; i++) {
		switch(buf[i]) {
			case 'x':
				data->map[i] = ABS_X;
				data->inv[i] = 1;
				break;
			case 'y':
				data->map[i] = ABS_Y;
				data->inv[i] = 1;
				break;
			case 'z':
				data->map[i] = ABS_Z;
				data->inv[i] = 1;
				break;
			case 'X':
				data->map[i] = ABS_X;
				data->inv[i] = -1;
				break;
			case 'Y':
				data->map[i] = ABS_Y;
				data->inv[i] = -1;
				break;
			case 'Z':
				data->map[i] = ABS_Z;
				data->inv[i] = -1;
				break;
			default:
				return -EINVAL;
		}
	}

	return count;
}

static int mc32x0_enable(struct mc32x0_data *data, int enable);

static ssize_t mc32x0_enable_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        struct i2c_client *client = container_of(mc32x0_device.parent, struct i2c_client, dev);

        struct mc32x0_data *mc32x0 = i2c_get_clientdata(client);

        return sprintf(buf, "%d\n", mc32x0->enabled);
}

static ssize_t mc32x0_enable_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
        bool new_enable;

        struct i2c_client *client = container_of(mc32x0_device.parent, struct i2c_client, dev);

        struct mc32x0_data *mc32x0 = i2c_get_clientdata(client);

        if (sysfs_streq(buf, "1"))
                new_enable = true;
        else if (sysfs_streq(buf, "0"))
                new_enable = false;
        else {
                pr_debug("%s: invalid value %d\n", __func__, *buf);
                return -EINVAL;
        }

        mc32x0_enable(mc32x0, new_enable);

        return count;
}

static ssize_t mc32x0_delay_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%d\n", a10asensor_duration);
}

static ssize_t mc32x0_delay_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t count)
{
        unsigned long data;
        int error;

        error = strict_strtoul(buf, 10, &data);
        if (error)
                return error;
        if (data > A10ASENSOR_DURATION_MAX)
                data = A10ASENSOR_DURATION_MAX;
        if (data < A10ASENSOR_DURATION_MIN)
                data = A10ASENSOR_DURATION_MIN;
        a10asensor_duration = data;

        return count;
}

static DEVICE_ATTR(map, 0660, mc32x0_map_show, mc32x0_map_store);
static DEVICE_ATTR(enable, 0660, mc32x0_enable_show, mc32x0_enable_store);
static DEVICE_ATTR(delay, 0660, mc32x0_delay_show, mc32x0_delay_store);

static struct attribute* mc32x0_attrs[] =
{
        &dev_attr_map.attr,
        &dev_attr_enable.attr,
        &dev_attr_delay.attr,
        NULL
};

static const struct attribute_group mc32x0_group =
{
	.attrs = mc32x0_attrs,
};

static int mc32x0_chip_init(struct i2c_client *client)
{

	mc32x0_set_image(client);

	return 0;
}

int mc32x0_set_mode(struct i2c_client *client, unsigned char mode)
{

	int comres=0;
	unsigned char data;

	if (mode<4) {
		data  = 0x40|mode;
	i2c_smbus_write_byte_data(client, MC32X0_Mode_Feature_REG,data);

	}
	return comres;

}



#ifdef DOT_CALI
struct file *openFile(char *path,int flag,int mode)
{
	struct file *fp;

	fp=filp_open(path, flag, mode);
	if (IS_ERR(fp) || !fp->f_op) {
		GSE_LOG("Calibration File filp_open return NULL\n");
		return NULL;
	} else {

		return fp;
	}
}

int readFile(struct file *fp,char *buf,int readlen)
{
	if (fp->f_op && fp->f_op->read)
		return fp->f_op->read(fp,buf,readlen, &fp->f_pos);
	else
		return -1;
}

int writeFile(struct file *fp,char *buf,int writelen)
{
	if (fp->f_op && fp->f_op->write)
		return fp->f_op->write(fp,buf,writelen, &fp->f_pos);
	else
		return -1;
}

int closeFile(struct file *fp)
{
	filp_close(fp,NULL);
	return 0;
}

void initKernelEnv(void)
{
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	printk(KERN_INFO "initKernelEnv\n");
}

 int MC32X0_WriteCalibration(struct i2c_client *client, int dat[MC32X0_AXES_NUM])
{
	int err;
	u8 buf[9];
	s16 tmp, x_gain, y_gain, z_gain ;
	s32 x_off, y_off, z_off;
#if 1  //modify by zwx

	GSE_LOG("UPDATE dat: (%+3d %+3d %+3d)\n",
	dat[MC32X0_AXIS_X], dat[MC32X0_AXIS_Y], dat[MC32X0_AXIS_Z]);

	/*calculate the real offset expected by caller*/
	//cali_temp[MC32X0_AXIS_X] = dat[MC32X0_AXIS_X];
	//cali_temp[MC32X0_AXIS_Y] = dat[MC32X0_AXIS_Y];
	//cali_temp[MC32X0_AXIS_Z] = dat[MC32X0_AXIS_Z];
	//cali[MC32X0_AXIS_Z]= cali[MC32X0_AXIS_Z]-gsensor_gain.z;


#endif
// read register 0x21~0x28
#if 1 //zwx
	//if ((err = mc32x0_read_block(client, 0x21, buf, 3)))
	//if ((err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x21, &buf[0],3)))
	err = i2c_smbus_read_i2c_block_data(client , 0x21 , 3 , &buf[0]);

	//if ((err = mc32x0_read_block(client, 0x24, &buf[3], 3)))
	//if ((err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x24, &buf[3],3)))
	err = i2c_smbus_read_i2c_block_data(client , 0x24 , 3 , &buf[3]);

	//if ((err = mc32x0_read_block(client, 0x27, &buf[6], 3)))
	//if ((err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x27, &buf[6],3)))
	err = i2c_smbus_read_i2c_block_data(client , 0x27 , 3 , &buf[6]);

#else
	buf[0] = 0x21;
	err = mc32x0_rx_data(client, &buf[0], 3);
	buf[3] = 0x24;
	err = mc32x0_rx_data(client, &buf[3], 3);
	buf[6] = 0x27;
	err = mc32x0_rx_data(client, &buf[6], 3);
#endif
#if 1
	// get x,y,z offset
	tmp = ((buf[1] & 0x3f) << 8) + buf[0];
	if (tmp & 0x2000)
		tmp |= 0xc000;
	x_off = tmp;

	tmp = ((buf[3] & 0x3f) << 8) + buf[2];
	if (tmp & 0x2000)
		tmp |= 0xc000;
	y_off = tmp;

	tmp = ((buf[5] & 0x3f) << 8) + buf[4];
	if (tmp & 0x2000)
		tmp |= 0xc000;
	z_off = tmp;

	// get x,y,z gain
	x_gain = ((buf[1] >> 7) << 8) + buf[6];
	y_gain = ((buf[3] >> 7) << 8) + buf[7];
	z_gain = ((buf[5] >> 7) << 8) + buf[8];

	// prepare new offset
	x_off = x_off + 16 * dat[MC32X0_AXIS_X] * 256 * 128 / 3 / gsensor_gain.x / (40 + x_gain);
	y_off = y_off + 16 * dat[MC32X0_AXIS_Y] * 256 * 128 / 3 / gsensor_gain.y / (40 + y_gain);
	z_off = z_off + 16 * dat[MC32X0_AXIS_Z] * 256 * 128 / 3 / gsensor_gain.z / (40 + z_gain);

	//storege the cerrunt offset data with DOT format
	offset_data[0] = x_off;
	offset_data[1] = y_off;
	offset_data[2] = z_off;

	//storege the cerrunt Gain data with GOT format
	gain_data[0] = 256*8*128/3/(40+x_gain);
	gain_data[1] = 256*8*128/3/(40+y_gain);
	gain_data[2] = 256*8*128/3/(40+z_gain);
	printk("%d %d ======================\n\n ",gain_data[0],x_gain);
#endif
	buf[0]=0x43;
	//mc32x0_write_block(client, 0x07, buf, 1);
	//mc32x0_write_reg(client,0x07,0x43);
	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf[0], 1 );
	i2c_smbus_write_byte_data(client, 0x07,buf[0]);
	buf[0] = x_off & 0xff;
	buf[1] = ((x_off >> 8) & 0x3f) | (x_gain & 0x0100 ? 0x80 : 0);
	buf[2] = y_off & 0xff;
	buf[3] = ((y_off >> 8) & 0x3f) | (y_gain & 0x0100 ? 0x80 : 0);
	buf[4] = z_off & 0xff;
	buf[5] = ((z_off >> 8) & 0x3f) | (z_gain & 0x0100 ? 0x80 : 0);


	//mc32x0_write_block(client, 0x21, buf, 6);
        //p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x21, &buf[0], 6 );
        i2c_smbus_write_i2c_block_data(client, 0x21, 2,&buf[0]);
        i2c_smbus_write_i2c_block_data(client, 0x21+2, 2,&buf[2]);
        i2c_smbus_write_i2c_block_data(client, 0x21+4, 2,&buf[4]);


        buf[0]=0x41;
        //mc32x0_write_block(client, 0x07, buf, 1);
        //p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf[0], 1 );
        i2c_smbus_write_byte_data(client, 0x07,buf[0]);
	//mc32x0_write_reg(client,0x07,0x41);

        return err;

}

int mcube_read_cali_file(struct i2c_client *client)
{
	int cali_data[3];
	int err =0;
	char buf[64];
	printk("%s %d\n",__func__,__LINE__);
	initKernelEnv();
	fd_file = openFile(CALIB_PATH,O_RDONLY,0);
	if (fd_file == NULL) {
		GSE_LOG("fail to open\n");
		cali_data[0] = 0;
		cali_data[1] = 0;
		cali_data[2] = 0;
		return 1;
	} else {
		printk("%s %d\n",__func__,__LINE__);
		memset(buf,0,64);
		if ((err = readFile(fd_file,buf,128))>0)
			GSE_LOG("buf:%s\n",buf);
		else
			GSE_LOG("read file error %d\n",err);
		printk("%s %d\n",__func__,__LINE__);

		set_fs(oldfs);
		closeFile(fd_file);

		sscanf(buf, "%d %d %d",&cali_data[MC32X0_AXIS_X], &cali_data[MC32X0_AXIS_Y], &cali_data[MC32X0_AXIS_Z]);
		GSE_LOG("cali_data: %d %d %d\n", cali_data[MC32X0_AXIS_X], cali_data[MC32X0_AXIS_Y], cali_data[MC32X0_AXIS_Z]);

		//cali_data1[MC32X0_AXIS_X] = cali_data[MC32X0_AXIS_X] * gsensor_gain.x / GRAVITY_EARTH_1000;
		//cali_data1[MC32X0_AXIS_Y] = cali_data[MC32X0_AXIS_Y] * gsensor_gain.y / GRAVITY_EARTH_1000;
		//cali_data1[MC32X0_AXIS_Z] = cali_data[MC32X0_AXIS_Z] * gsensor_gain.z / GRAVITY_EARTH_1000;
		//cali_data[MC32X0_AXIS_X]=-cali_data[MC32X0_AXIS_X];
		//cali_data[MC32X0_AXIS_Y]=-cali_data[MC32X0_AXIS_Y];
		//cali_data[MC32X0_AXIS_Z]=-cali_data[MC32X0_AXIS_Z];

		//GSE_LOG("cali_data1: %d %d %d\n", cali_data1[MC32X0_AXIS_X], cali_data1[MC32X0_AXIS_Y], cali_data1[MC32X0_AXIS_Z]);
		printk("%s %d\n",__func__,__LINE__);
		MC32X0_WriteCalibration(client,cali_data);
	}
	return 0;
}


void MC32X0_rbm(struct i2c_client *client, int enable)
{
	//int err;
        char buf1[3];
	if(enable == 1 ){
#if 1
		buf1[0] = 0x43;
		//err = mc32x0_write_block(client, 0x07, buf1, 0x01);
		//err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf1[0], 1 );
		i2c_smbus_write_byte_data(client, 0x07,buf1[0]);
		buf1[0] = 0x02;
		//err = mc32x0_write_block(client, 0x14, buf1, 0x01);
		//err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x14, &buf1[0], 1 );
		i2c_smbus_write_byte_data(client, 0x14,buf1[0]);
		buf1[0] = 0x41;
		//err = mc32x0_write_block(client, 0x07, buf1, 0x01);
		//err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf1[0], 1 );
		i2c_smbus_write_byte_data(client, 0x07,buf1[0]);
#else
		err = mc32x0_write_reg(client,0x07,0x43);
		err = mc32x0_write_reg(client,0x14,0x02);
		err = mc32x0_write_reg(client,0x07,0x41);
#endif
		enable_RBM_calibration =1;

		GSE_LOG("set rbm!!\n");

		msleep(10);
	} else if(enable == 0 ) {
#if 1
		buf1[0] = 0x43;
		//err = mc32x0_write_block(client, 0x07, buf1, 0x01);
		//err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf1[0], 1 );
		i2c_smbus_write_byte_data(client, 0x07,buf1[0]);

		buf1[0] = 0x00;
		//err = mc32x0_write_block(client, 0x14, buf1, 0x01);
		//err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x14, &buf1[0], 1 );
		i2c_smbus_write_byte_data(client, 0x14,buf1[0]);
		buf1[0] = 0x41;
		//err = mc32x0_write_block(client, 0x07, buf1, 0x01);
		//err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf1[0], 1 );
		i2c_smbus_write_byte_data(client, 0x07,buf1[0]);
#else
		err = mc32x0_write_reg(client,0x07,0x43);
		err = mc32x0_write_reg(client,0x14,0x00);
		err = mc32x0_write_reg(client,0x07,0x41);
#endif
		enable_RBM_calibration =0;

		GSE_LOG("clear rbm!!\n");

		msleep(10);
	}
}

/*----------------------------------------------------------------------------*/
 int MC32X0_ReadData_RBM(struct i2c_client *client,int data[MC32X0_AXES_NUM])
{
	//u8 uData;
	u8 addr = 0x0d;
	u8 rbm_buf[MC32X0_DATA_LEN] = {0};
	int err = 0;


	//err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, addr, &rbm_buf[0],6);
	err = i2c_smbus_read_i2c_block_data(client , addr , 6 , rbm_buf);
	//err = mc32x0_read_block(client, addr, rbm_buf, 0x06);

	data[MC32X0_AXIS_X] = (s16)((rbm_buf[0]) | (rbm_buf[1] << 8));
	data[MC32X0_AXIS_Y] = (s16)((rbm_buf[2]) | (rbm_buf[3] << 8));
	data[MC32X0_AXIS_Z] = (s16)((rbm_buf[4]) | (rbm_buf[5] << 8));

	GSE_LOG("rbm_buf<<<<<[%02x %02x %02x %02x %02x %02x]\n",rbm_buf[0], rbm_buf[2], rbm_buf[2], rbm_buf[3], rbm_buf[4], rbm_buf[5]);
	GSE_LOG("RBM<<<<<[%04x %04x %04x]\n", data[MC32X0_AXIS_X], data[MC32X0_AXIS_Y], data[MC32X0_AXIS_Z]);
	GSE_LOG("RBM<<<<<[%04d %04d %04d]\n", data[MC32X0_AXIS_X], data[MC32X0_AXIS_Y], data[MC32X0_AXIS_Z]);
	return err;
}


 int MC32X0_ReadRBMData(struct i2c_client *client, char *buf)
{
	//struct mc32x0_data *mc32x0 = i2c_get_clientdata(client);
	int res = 0;
	int data[3];

	if (!buf){
		return EINVAL;
	}

	mc32x0_set_mode(client,MC32X0_WAKE);
/*
	if(mc32x0->status == mc32x0_CLOSE)
	{
		res = mc32x0_start(client, 0);
		if(res)
		{
			GSE_ERR("Power on mc32x0 error %d!\n", res);
		}
	}
*/
	if((res = MC32X0_ReadData_RBM(client,data))) {
		GSE_ERR("%s I2C error: ret value=%d",__func__, res);
		return EIO;
	} else {
		sprintf(buf, "%04x %04x %04x", data[MC32X0_AXIS_X],
			data[MC32X0_AXIS_Y], data[MC32X0_AXIS_Z]);

	}

	return 0;
}
 int MC32X0_ReadOffset(struct i2c_client *client,s16 ofs[MC32X0_AXES_NUM])
{
	int err;
	u8 off_data[6];


        if(McubeID &MCUBE_8G_14BIT)
	{

		//if ((err = mc32x0_read_block(client, MC32X0_XOUT_EX_L_REG, off_data, MC32X0_DATA_LEN)))
		//if ((err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_EX_L_REG, &off_data[0],MC32X0_DATA_LEN)))
		err = i2c_smbus_read_i2c_block_data(client , MC32X0_XOUT_EX_L_REG , MC32X0_DATA_LEN , off_data);

		ofs[MC32X0_AXIS_X] = ((s16)(off_data[0]))|((s16)(off_data[1])<<8);
		ofs[MC32X0_AXIS_Y] = ((s16)(off_data[2]))|((s16)(off_data[3])<<8);
		ofs[MC32X0_AXIS_Z] = ((s16)(off_data[4]))|((s16)(off_data[5])<<8);
	} else if(McubeID &MCUBE_1_5G_8BIT) {
		//if ((err = mc32x0_read_block(client, 0, off_data, 3)))
		//if ((err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0, &off_data[0],3)))
		err = i2c_smbus_read_i2c_block_data(client , 0 , 3 , off_data);

		ofs[MC32X0_AXIS_X] = (s8)off_data[0];
		ofs[MC32X0_AXIS_Y] = (s8)off_data[1];
		ofs[MC32X0_AXIS_Z] = (s8)off_data[2];
	}

	GSE_LOG("MC32X0_ReadOffset %d %d %d \n",ofs[MC32X0_AXIS_X] ,ofs[MC32X0_AXIS_Y],ofs[MC32X0_AXIS_Z]);

        return 0;
}
/*----------------------------------------------------------------------------*/
 int MC32X0_ResetCalibration(struct i2c_client *client)
{

	u8 buf[6];
	s16 tmp;
	int err;
#if 1   //zwx
	buf[0] = 0x43;
	//if(err = mc32x0_write_block(client, 0x07, buf, 1))
	//if(err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf[0], 1 ))
	if((err = i2c_smbus_write_byte_data(client, 0x07,buf[0]))){
		GSE_ERR("error 0x07: %d\n", err);
	}


	//if(err = mc32x0_write_block(client, 0x21, offset_buf, 6)) // add by liang for writing offset register as OTP value
	//if(err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x21, &offset_buf[0], 6 ))
	if((err = i2c_smbus_write_i2c_block_data(client, 0x21, 6,offset_buf))){
		GSE_ERR("error: %d\n", err);
	}

	buf[0] = 0x41;
	//if(err = mc32x0_write_block(client, 0x07, buf, 1))
	//if(err = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf[0], 1 ))
	if((err = i2c_smbus_write_byte_data(client, 0x07,buf[0]))){
		GSE_ERR("error: %d\n", err);
	}
#else
	mc32x0_write_reg(client,0x07,0x43);

	mc32x0_write_block(client, 0x21, offset_buf, 6);

	mc32x0_write_reg(client,0x07,0x41);
#endif
	msleep(20);

	tmp = ((offset_buf[1] & 0x3f) << 8) + offset_buf[0];  // add by Liang for set offset_buf as OTP value
	if (tmp & 0x2000)
		tmp |= 0xc000;
	offset_data[0] = tmp;

	tmp = ((offset_buf[3] & 0x3f) << 8) + offset_buf[2];  // add by Liang for set offset_buf as OTP value
	if (tmp & 0x2000)
		tmp |= 0xc000;
	offset_data[1] = tmp;

	tmp = ((offset_buf[5] & 0x3f) << 8) + offset_buf[4];  // add by Liang for set offset_buf as OTP value
	if (tmp & 0x2000)
		tmp |= 0xc000;
	offset_data[2] = tmp;

	//memset(mc32x0->cali_sw, 0x00, sizeof(mc32x0->cali_sw));
	return 0;

}
/*----------------------------------------------------------------------------*/
 int MC32X0_ReadCalibration(struct i2c_client *client,int dat[MC32X0_AXES_NUM])
{

        signed short MC_offset[MC32X0_AXES_NUM+1];	/*+1: for 4-byte alignment*/
        int err;
        memset(MC_offset, 0, sizeof(MC_offset));
        if ((err = MC32X0_ReadOffset(client, MC_offset))) {
                GSE_ERR("read offset fail, %d\n", err);
                return err;
        }

        dat[MC32X0_AXIS_X] = MC_offset[MC32X0_AXIS_X];
        dat[MC32X0_AXIS_Y] = MC_offset[MC32X0_AXIS_Y];
        dat[MC32X0_AXIS_Z] = MC_offset[MC32X0_AXIS_Z];
        //modify by zwx
        //GSE_LOG("MC32X0_ReadCalibration %d %d %d \n",dat[mc32x0->cvt.map[MC32X0_AXIS_X]] ,dat[mc32x0->cvt.map[MC32X0_AXIS_Y]],dat[mc32x0->cvt.map[MC32X0_AXIS_Z]]);

        return 0;
}

/*----------------------------------------------------------------------------*/

 int MC32X0_ReadData(struct i2c_client *client, s16 buffer[MC32X0_AXES_NUM])
{
	unsigned char buf[6];
	signed char buf1[6];
	char rbm_buf[6];
	int ret;
	//int err = 0;

	if ( enable_RBM_calibration == 0){
		//err = hwmsen_read_block(client, addr, buf, 0x06);
	} else if (enable_RBM_calibration == 1){
		memset(rbm_buf, 0, 6);
        	//rbm_buf[0] = mc32x0_REG_RBM_DATA;
        	//ret = mc32x0_rx_data(client, &rbm_buf[0], 6);
        	//ret = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x0d, &rbm_buf[0],6);
        	i2c_smbus_read_i2c_block_data(client , 0x0d , 2 , &rbm_buf[0]);
        	i2c_smbus_read_i2c_block_data(client , 0x0d+2 , 2 , &rbm_buf[2]);
        	i2c_smbus_read_i2c_block_data(client , 0x0d+4 , 2 , &rbm_buf[4]);
	}

	if ( enable_RBM_calibration == 0){

		if(McubeID &MC32X0_HIGH_END) {
			#ifdef MC32X0_HIGH_END
			ret = i2c_smbus_read_i2c_block_data(client , MC32X0_XOUT_EX_L_REG , 6 , buf);
			//ret = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_EX_L_REG, &buf[0],6);

			buffer[0] = (signed short)((buf[0])|(buf[1]<<8));
			buffer[1] = (signed short)((buf[2])|(buf[3]<<8));
			buffer[2] = (signed short)((buf[4])|(buf[5]<<8));
			#endif
		} else if(McubeID &MC32X0_LOW_END) {
			#ifdef MC32X0_LOW_END
			ret = i2c_smbus_read_i2c_block_data(client , MC32X0_XOUT_REG , 3 , buf1);
			//ret = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_REG, &buf[0],3);

			buffer[0] = (signed short)buf1[0];
			buffer[1] = (signed short)buf1[1];
			buffer[2] = (signed short)buf1[2];
			#endif
		}

		mcprintkreg("MC32X0_ReadData : %d %d %d \n",buffer[0],buffer[1],buffer[2]);
	} else if (enable_RBM_calibration == 1){
		buffer[MC32X0_AXIS_X] = (s16)((rbm_buf[0]) | (rbm_buf[1] << 8));
		buffer[MC32X0_AXIS_Y] = (s16)((rbm_buf[2]) | (rbm_buf[3] << 8));
		buffer[MC32X0_AXIS_Z] = (s16)((rbm_buf[4]) | (rbm_buf[5] << 8));

		GSE_LOG("%s RBM<<<<<[%08d %08d %08d]\n", __func__,buffer[MC32X0_AXIS_X], buffer[MC32X0_AXIS_Y], buffer[MC32X0_AXIS_Z]);
                if(gain_data[0] == 0){
		      buffer[MC32X0_AXIS_X] = 0;
		      buffer[MC32X0_AXIS_Y] = 0;
		      buffer[MC32X0_AXIS_Z] = 0;
	              return 0;
                }
		buffer[MC32X0_AXIS_X] = (buffer[MC32X0_AXIS_X] + offset_data[0]/2)*gsensor_gain.x/gain_data[0];
		buffer[MC32X0_AXIS_Y] = (buffer[MC32X0_AXIS_Y] + offset_data[1]/2)*gsensor_gain.y/gain_data[1];
		buffer[MC32X0_AXIS_Z] = (buffer[MC32X0_AXIS_Z] + offset_data[2]/2)*gsensor_gain.z/gain_data[2];

		GSE_LOG("%s offset_data <<<<<[%d %d %d]\n", __func__,offset_data[0], offset_data[1], offset_data[2]);

		GSE_LOG("%s gsensor_gain <<<<<[%d %d %d]\n", __func__,gsensor_gain.x, gsensor_gain.y, gsensor_gain.z);

		GSE_LOG("%s gain_data <<<<<[%d %d %d]\n", __func__,gain_data[0], gain_data[1], gain_data[2]);

		GSE_LOG("%s RBM->RAW <<<<<[%d %d %d]\n", __func__,buffer[MC32X0_AXIS_X], buffer[MC32X0_AXIS_Y], buffer[MC32X0_AXIS_Z]);
	}

	return 0;
}

int MC32X0_ReadRawData(struct i2c_client *client,  char * buf)
{


	int res = 0;
	s16 raw_buf[3];

	if (!buf){
		return EINVAL;
	}

	//mc32x0_power_up(mc32x0);
	mc32x0_set_mode(client, MC32X0_WAKE);
	if((res = MC32X0_ReadData(client,&raw_buf[0]))){
	        printk("%s %d\n",__FUNCTION__, __LINE__);
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	} else{

                GSE_LOG("UPDATE dat: (%+3d %+3d %+3d)\n",
                raw_buf[MC32X0_AXIS_X], raw_buf[MC32X0_AXIS_Y], raw_buf[MC32X0_AXIS_Z]);

                G_RAW_DATA[MC32X0_AXIS_X] = raw_buf[0];
                G_RAW_DATA[MC32X0_AXIS_Y] = raw_buf[1];
                G_RAW_DATA[MC32X0_AXIS_Z] = raw_buf[2];
                G_RAW_DATA[MC32X0_AXIS_Z]= G_RAW_DATA[MC32X0_AXIS_Z]+gsensor_gain.z;

                //printk("%s %d\n",__FUNCTION__, __LINE__);
		sprintf(buf, "%04x %04x %04x", G_RAW_DATA[MC32X0_AXIS_X],
			G_RAW_DATA[MC32X0_AXIS_Y], G_RAW_DATA[MC32X0_AXIS_Z]);
		GSE_LOG("G_RAW_DATA: (%+3d %+3d %+3d)\n",
                G_RAW_DATA[MC32X0_AXIS_X], G_RAW_DATA[MC32X0_AXIS_Y], G_RAW_DATA[MC32X0_AXIS_Z]);
	}
	return 0;
}

int mc32x0_reset (struct i2c_client *client)
{

	s16 tmp, x_gain, y_gain, z_gain ;
	s32 x_off, y_off, z_off;
        u8 buf[3];
	  int err;
        //mc32x0_write_reg(client,0x1b,0x6d);
	buf[0]=0x6d;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x1b, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x1b,buf[0]);

	//mc32x0_write_reg(client,0x1b,0x43);
	buf[0]=0x43;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x1b, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x1b,buf[0]);
	msleep(5);

	//mc32x0_write_reg(client,0x07,0x43);
	buf[0]=0x43;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x07, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x07,buf[0]);
	//mc32x0_write_reg(client,0x1C,0x80);
	buf[0]=0x80;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x1C, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x1c,buf[0]);
	//mc32x0_write_reg(client,0x17,0x80);
	buf[0]=0x80;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x17, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x17,buf[0]);
	msleep(5);
	//mc32x0_write_reg(client,0x1C,0x00);
	buf[0]=0x00;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x1C, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x1c,buf[0]);
	//mc32x0_write_reg(client,0x17,0x00);
	buf[0]=0x00;
  	//p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x17, &buf[0], 1 );
  	i2c_smbus_write_byte_data(client, 0x17,buf[0]);
	msleep(5);


/*
	if ((err = mc32x0_read_block(new_client, 0x21, offset_buf, 6))) //add by Liang for storeage OTP offsef register value
	{
		GSE_ERR("error: %d\n", err);
		return err;
	}
*/
         memset(offset_buf, 0, 9);
	//offset_buf[0] = 0x21;
	//err = mc32x0_rx_data(client, offset_buf, 9);
	//err = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x21, &offset_buf[0],9);
	err = i2c_smbus_read_i2c_block_data(client , 0x21 , 9 , offset_buf);


	tmp = ((offset_buf[1] & 0x3f) << 8) + offset_buf[0];
	if (tmp & 0x2000)
		tmp |= 0xc000;
	x_off = tmp;

	tmp = ((offset_buf[3] & 0x3f) << 8) + offset_buf[2];
	if (tmp & 0x2000)
		tmp |= 0xc000;
	y_off = tmp;

	tmp = ((offset_buf[5] & 0x3f) << 8) + offset_buf[4];
	if (tmp & 0x2000)
		tmp |= 0xc000;
	z_off = tmp;

	// get x,y,z gain
	x_gain = ((offset_buf[1] >> 7) << 8) + offset_buf[6];
	y_gain = ((offset_buf[3] >> 7) << 8) + offset_buf[7];
	z_gain = ((offset_buf[5] >> 7) << 8) + offset_buf[8];


	//storege the cerrunt offset data with DOT format
	offset_data[0] = x_off;
	offset_data[1] = y_off;
	offset_data[2] = z_off;

	//storege the cerrunt Gain data with GOT format
	gain_data[0] = 256*8*128/3/(40+x_gain);
	gain_data[1] = 256*8*128/3/(40+y_gain);
	gain_data[2] = 256*8*128/3/(40+z_gain);
	printk("offser gain = %d %d %d %d %d %d======================\n\n ",
		gain_data[0],gain_data[1],gain_data[2],offset_data[0],offset_data[1],offset_data[2]);

	return 0;
}
#endif

int mc32x0_read_accel_xyz(struct i2c_client *client, s16 * acc)
{
	int comres;
#ifdef DOT_CALI
	s16 raw_buf[6];
	comres = MC32X0_ReadData(client,&raw_buf[0]);

	acc[0] = raw_buf[0];
	acc[1] = raw_buf[1];
	acc[2] = raw_buf[2];
#else
	unsigned char raw_buf[6];
	signed char raw_buf1[3];
	if(McubeID &MC32X0_HIGH_END) {
#ifdef MC32X0_HIGH_END
	        comres = i2c_smbus_read_i2c_block_data(client , MC32X0_XOUT_EX_L_REG , 6 , raw_buf);
		//comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_EX_L_REG, &data[0],6);

		acc[0] = (signed short)((raw_buf[0])|(raw_buf[1]<<8));
		acc[1] = (signed short)((raw_buf[2])|(raw_buf[3]<<8));
		acc[2] = (signed short)((raw_buf[4])|(raw_buf[5]<<8));
#endif
	} else if(McubeID &MC32X0_LOW_END) {
#ifdef MC32X0_LOW_END
		comres = i2c_smbus_read_i2c_block_data(client , MC32X0_XOUT_REG , 3 , raw_buf1);
		//comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_REG, &data[0],3);

		acc[0] = (signed short)raw_buf1[0];
		acc[1] = (signed short)raw_buf1[1];
		acc[2] = (signed short)raw_buf1[2];
#endif
	}
#endif
	return comres;
}

static int mc32x0_measure(struct i2c_client *client, struct acceleration *accel)
{

	s16 raw[3];

#ifdef DOT_CALI
	int ret;
#endif

#ifdef DOT_CALI
 	if( load_cali_flg > 0) {
		ret =mcube_read_cali_file(client);
		if(ret == 0)
			load_cali_flg = ret;
		else
			load_cali_flg --;
		GSE_LOG("load_cali %d\n",ret);
	}
#endif
	/* read acceleration data */
	mc32x0_read_accel_xyz(client,&raw[0]);

	accel->x = raw[0] ;
	accel->y = raw[1] ;
	accel->z = raw[2] ;

	return 0;
}

static void mc32x0_work_func(struct work_struct *work)
{
	struct mc32x0_data *data = container_of(work, struct mc32x0_data, work);
	struct acceleration accel = {0};

	mc32x0_measure(data->client, &accel);

	dprintk(DEBUG_DATA_INFO, "mc32x0_measure: acc.x=%d, acc.y=%d, acc.z=%d\n", accel.x,accel.y, accel.z);

	input_report_abs(data->input_dev, data->map[0], accel.x);
	input_report_abs(data->input_dev, data->map[1], accel.y);
	input_report_abs(data->input_dev, data->map[2], accel.z);
	input_sync(data->input_dev);

}

static enum hrtimer_restart mc32x0_timer_func(struct hrtimer *timer)
{
	struct mc32x0_data *data = container_of(timer, struct mc32x0_data, timer);

	queue_work(data->mc32x0_wq, &data->work);
	hrtimer_start(&data->timer, ktime_set(0, a10asensor_duration*1000000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static int mc32x0_enable(struct mc32x0_data *data, int enable)
{
	if(enable){
		msleep(10);
		mc32x0_chip_init(data->client);
		hrtimer_start(&data->timer, ktime_set(0, a10asensor_duration*1000000), HRTIMER_MODE_REL);
                 data->enabled = true;
	}else{
		hrtimer_cancel(&data->timer);
                data->enabled = false;
	}
	return 0;
}

static long mc32x0_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	//int intBuf[SENSOR_DATA_SIZE];
	int ret = 0;
	float convert_para=0.0f;
#ifdef DOT_CALI
	void __user *data1;
	char strbuf[256];
	int cali[3];
	SENSOR_DATA sensor_data;
	struct i2c_client *client = container_of(mc32x0_device.parent, struct i2c_client, dev);
        //struct mc32x0_data* this = (struct mc32x0_data *)i2c_get_clientdata(client);  /* 设备数据实例的指针. */
#endif

	switch (cmd) {
		case IOCTL_SENSOR_SET_DELAY_ACCEL:
			if(copy_from_user((void *)&a10asensor_duration, (void __user *) arg, sizeof(short))!=0){
				printk("copy from error in %s.\n",__func__);
			}

			break;

		case IOCTL_SENSOR_GET_DELAY_ACCEL:
			if(copy_to_user((void __user *) arg, (const void *)&a10asensor_duration, sizeof(short))!=0){
				printk("copy to error in %s.\n",__func__);
			}

			break;

		case IOCTL_SENSOR_GET_STATE_ACCEL:
			if(copy_to_user((void __user *) arg, (const void *)&a10asensor_state_flag, sizeof(short))!=0){
				printk("copy to error in %s.\n",__func__);
			}

			break;

		case IOCTL_SENSOR_SET_STATE_ACCEL:
			if(copy_from_user((void *)&a10asensor_state_flag, (void __user *) arg, sizeof(short))!=0){
				printk("copy from error in %s.\n",__func__);
			}

			break;
		case IOCTL_SENSOR_GET_NAME:
			if(copy_to_user((void __user *) arg,(const void *)mc32x0_DISPLAY_NAME, sizeof(                 mc32x0_DISPLAY_NAME))!=0){
				printk("copy to error in %s.\n",__func__);
			}
			break;

		case IOCTL_SENSOR_GET_VENDOR:
			if(copy_to_user((void __user *) arg,(const void *)mc32x0_DIPLAY_VENDOR, sizeof(mc32x0_DIPLAY_VENDOR))!=0){
				printk("copy to error in %s.\n",__func__);
			}
			break;

		case IOCTL_SENSOR_GET_CONVERT_PARA:
			convert_para = mc32x0_CONVERT_PARAMETER;
			if(copy_to_user((void __user *) arg,(const void *)&convert_para,sizeof(float))!=0){
				printk("copy to error in %s.\n",__func__);
			}



#ifdef DOT_CALI
                case GSENSOR_IOCTL_READ_SENSORDATA:
                case GSENSOR_IOCTL_READ_RAW_DATA:
                        GSE_LOG("fwq GSENSOR_IOCTL_READ_RAW_DATA\n");

                        MC32X0_ReadRawData(client,strbuf);
			if (copy_to_user((void __user *) arg, &strbuf, strlen(strbuf)+1)) {
                                printk("failed to copy sense data to user space.");
                                return -EFAULT;
                        }
                        break;


                case GSENSOR_MCUBE_IOCTL_SET_CALI:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_SET_CALI!!\n");
			data1 = (void __user *)arg;


			//data = (unsigned char*)arg;


			if(data1 == NULL){
				ret = -EINVAL;
				break;
			}
			if(copy_from_user(&sensor_data, data1, sizeof(sensor_data))) {
				ret = -EFAULT;
				break;
			}
			//if(atomic_read(&this->suspend))
			//{
			//	GSE_ERR("Perform calibration in suspend state!!\n");
			//	err = -EINVAL;
			//}
			else
			{
				//this->cali_sw[MC32X0_AXIS_X] += sensor_data.x;
				//this->cali_sw[MC32X0_AXIS_Y] += sensor_data.y;
				//this->cali_sw[MC32X0_AXIS_Z] += sensor_data.z;

				cali[MC32X0_AXIS_X] = sensor_data.x;
				cali[MC32X0_AXIS_Y] = sensor_data.y;
				cali[MC32X0_AXIS_Z] = sensor_data.z;

			  	GSE_LOG("GSENSOR_MCUBE_IOCTL_SET_CALI %d  %d  %d  %d  %d  %d!!\n", cali[MC32X0_AXIS_X], cali[MC32X0_AXIS_Y],cali[MC32X0_AXIS_Z] ,sensor_data.x, sensor_data.y ,sensor_data.z);

				ret = MC32X0_WriteCalibration(client, cali);
			}

			break;

		case GSENSOR_IOCTL_CLR_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
			ret = MC32X0_ResetCalibration(client);
			break;

		case GSENSOR_IOCTL_GET_CALI:
			GSE_LOG("fwq mc32x0 GSENSOR_IOCTL_GET_CALI\n");

			data1 = (unsigned char*)arg;

			if(data1 == NULL){
				ret = -EINVAL;
				break;
			}

			if((ret = MC32X0_ReadCalibration(client,cali))){
				GSE_LOG("fwq mc32x0 MC32X0_ReadCalibration error!!!!\n");
				break;
			}

			sensor_data.x = 0;//this->cali_sw[MC32X0_AXIS_X];
			sensor_data.y = 0;//this->cali_sw[MC32X0_AXIS_Y];
			sensor_data.z = 0;//this->cali_sw[MC32X0_AXIS_Z];
                        //if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))

			if(copy_to_user(data1, &sensor_data, sizeof(sensor_data))){
				ret = -EFAULT;
				break;
			}
			break;
                        // add by liang ****
                        //add in Sensors_io.h
                        //#define GSENSOR_IOCTL_SET_CALI_MODE   _IOW(GSENSOR, 0x0e, int)
		case GSENSOR_IOCTL_SET_CALI_MODE:
			GSE_LOG("fwq mc32x0 GSENSOR_IOCTL_SET_CALI_MODE\n");
			break;

		case GSENSOR_MCUBE_IOCTL_READ_RBM_DATA:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_READ_RBM_DATA\n");
			data1 = (void __user *) arg;
			if(data1 == NULL){
				ret = -EINVAL;
				break;
			}
			MC32X0_ReadRBMData(client,(char *)&strbuf);
			if(copy_to_user(data1, &strbuf, strlen(strbuf)+1)){
				ret = -EFAULT;
				break;
			}
			break;

		case GSENSOR_MCUBE_IOCTL_SET_RBM_MODE:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_SET_RBM_MODE\n");

			MC32X0_rbm(client, 1);

			break;

		case GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE\n");

			MC32X0_rbm(client, 0);

			break;

		case GSENSOR_MCUBE_IOCTL_REGISTER_MAP:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_REGISTER_MAP\n");

			//MC32X0_Read_Reg_Map(client);

			break;
#endif
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}


static int mc32x0_open(struct inode *inode, struct file *filp)
{
	int ret;
	ret = nonseekable_open(inode, filp);
	return ret;
}

static int mc32x0_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations a10asensor_fops =
{
	.owner	= THIS_MODULE,
	.open       	= mc32x0_open,
	.release    	= mc32x0_release,
	.unlocked_ioctl = mc32x0_ioctl,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mc32x0_early_suspend(struct early_suspend *handler)
{
	struct mc32x0_data *data;
	char mc32x0_address;
	char mc32x0_data;

	//printk("mc32x0_early_suspend 2 \n");

	data = container_of(handler, struct mc32x0_data, early_suspend);

	hrtimer_cancel(&data->timer);

	mc32x0_set_mode(data->client,MC32X0_STANDBY);

}

static void mc32x0_early_resume(struct early_suspend *handler)
{
	struct mc32x0_data *data;
	char mc32x0_address;
	char mc32x0_data;

	//printk("mc32x0_early_resume 2\n");

	data = container_of(handler, struct mc32x0_data, early_suspend);

	//Add 20130722
	mc32x0_chip_init(data->client);
	MC32X0_ResetCalibration(data->client);
	mcube_read_cali_file(data->client);
	//before

	mc32x0_set_mode(data->client,MC32X0_WAKE);


	hrtimer_start(&data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
}
#endif

static struct miscdevice mc32x0_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mc32x0",
	.fops = &a10asensor_fops,
};



static int mc32x0_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct mc32x0_data *data;

#ifdef DOT_CALI
	load_cali_flg = 30;
#endif

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	data = kzalloc(sizeof(struct mc32x0_data), GFP_KERNEL);
	if(data == NULL){
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	data->mc32x0_wq = create_singlethread_workqueue("mc32x0_wq");
	if (!data->mc32x0_wq ){
		ret = -ENOMEM;
		goto err_create_workqueue_failed;
	}
	INIT_WORK(&data->work, mc32x0_work_func);
	mutex_init(&data->lock);

	a10asensor_duration = A10ASENSOR_DURATION_DEFAULT;
	a10asensor_state_flag = 1;

	data->input_dev = input_allocate_device();
	if (!data->input_dev) {
		ret = -ENOMEM;
		goto exit_input_dev_alloc_failed;
	}

	data->client = client;
	dev.client=client;

	i2c_set_clientdata(client, data);

#ifdef DOT_CALI
	 mc32x0_reset(client);
#endif

	ret = mc32x0_chip_init(client);
	if (ret < 0) {
		goto err_chip_init_failed;
	}

	set_bit(EV_ABS, data->input_dev->evbit);
	data->map[0] = G_0;
	data->map[1] = G_1;
	data->map[2] = G_2;
        data->inv[0] = G_0_REVERSE;
        data->inv[1] = G_1_REVERSE;
        data->inv[2] = G_2_REVERSE;

	input_set_abs_params(data->input_dev, ABS_X, -32*8, 32*8, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(data->input_dev, ABS_Y, -32*8, 32*8, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(data->input_dev, ABS_Z, -32*8, 32*8, INPUT_FUZZ, INPUT_FLAT);

	data->input_dev->name = "mc32x0";


	ret = input_register_device(data->input_dev);
	if (ret) {
		goto exit_input_register_device_failed;
	}
        mc32x0_device.parent = &client->dev;
	ret = misc_register(&mc32x0_device);
	if (ret) {
		goto exit_misc_device_register_failed;
	}

	ret = sysfs_create_group(&client->dev.kobj, &mc32x0_group);

	if (!data->use_irq){
		hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		data->timer.function = mc32x0_timer_func;
		hrtimer_start(&data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.suspend = mc32x0_early_suspend;
	data->early_suspend.resume = mc32x0_early_resume;
	register_early_suspend(&data->early_suspend);
#endif
	data->enabled = true;
	dprintk(DEBUG_INIT,"mc32x0 probe ok \n");

	return 0;
exit_misc_device_register_failed:
exit_input_register_device_failed:
	input_free_device(data->input_dev);
err_chip_init_failed:
exit_input_dev_alloc_failed:
	destroy_workqueue(data->mc32x0_wq);
err_create_workqueue_failed:
	kfree(data);
err_alloc_data_failed:
err_check_functionality_failed:
	printk("mc32x0 probe failed \n");
	return ret;

}

static int mc32x0_remove(struct i2c_client *client)
{
	struct mc32x0_data *data = i2c_get_clientdata(client);

	hrtimer_cancel(&data->timer);
	input_unregister_device(data->input_dev);
	//gpio_release(mc32x0_pin_hd, 2);
	misc_deregister(&mc32x0_device);
	sysfs_remove_group(&client->dev.kobj, &mc32x0_group);
	kfree(data);
	return 0;
}

static const struct i2c_device_id mc32x0_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mc32x0_id);

static struct i2c_driver mc32x0_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SENSOR_NAME,
	},
	.id_table	= mc32x0_id,
	.probe		= mc32x0_probe,
	.remove		= mc32x0_remove,
	.detect              = gsensor_detect,

	.address_list	= normal_i2c,
};

static int __init mc32x0_init(void)
{
	int ret = -1;
	dprintk(DEBUG_INIT, "======%s=========. \n", __func__);

	if(input_fetch_sysconfig_para(&(gsensor_info.input_type))){
		printk("%s: err.\n", __func__);
		return -1;
	}else
		twi_id = gsensor_info.twi_id;


	dprintk(DEBUG_INIT,"%s: after fetch_sysconfig_para:  normal_i2c: 0x%hx.  \n", \
			__func__, normal_i2c[0]);

	ret = i2c_add_driver(&mc32x0_driver);

        dprintk(DEBUG_INIT, "add mc32x0 i2c driver\n");
	return ret;
}

static void __exit mc32x0_exit(void)
{
	i2c_del_driver(&mc32x0_driver);
}

//*********************************************************************************************************
MODULE_AUTHOR("Long Chen <lchen@mcube-inc.com>");
MODULE_DESCRIPTION("mc32x0 driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(mc32x0_init);
module_exit(mc32x0_exit);