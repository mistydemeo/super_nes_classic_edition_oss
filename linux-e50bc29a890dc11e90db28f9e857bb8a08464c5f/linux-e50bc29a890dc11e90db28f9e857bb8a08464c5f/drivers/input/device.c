/*
 *  device.c - Linux kernel modules for  Detection Sensor 
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/device.h>


//#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <mach/gpio.h> 
#include <mach/sys_config.h> 
#include <linux/gpio.h>


#include<linux/fs.h>
#include<linux/string.h>
#include<asm/uaccess.h>

#include<linux/init-input.h>
#include <linux/pinctrl/consumer.h>


#define NOTE_INFO1              ";Behind the equals sign said detected equipment corresponding to the name of the driver\n"
#define NOTE_INFO2              ";Note: don't change the file format!\n"
#define GSENSOR_DEVICE_KEY_NAME "gsensor_module_name"
#define LSENSOR_DEVICE_KEY_NAME "light sensor_module_name"
#define GYR_SENSOR_DEVICE_KEY_NAME "gyr sensor_module_name"
#define CTP_DEVICE_KEY_NAME     "ctp_module_name"
#define FILE_DIR                "data/device.info"

#define STRING_LENGTH           (128)
#define FILE_LENGTH             (1024)
#define NAME_LENGTH             (32)
#define ADDRESS_NUMBER          (5)
#define REG_VALUE_NUMBER        (5)
#define DEFAULT_TOTAL_ROW       (6)

static char gsensor_name[NAME_LENGTH] = {'\0'};
static char ctp_name[NAME_LENGTH] = {'\0'};
static char lsensor_name[NAME_LENGTH] = {'\0'};
static char gyr_sensor_name[NAME_LENGTH] = {'\0'};
static int g_support_number = 0;
static int c_support_number = 0;
static int l_support_number = 0;
static int gyr_support_number = 0;
static int g_device_used = 0;
static int c_device_used = 0;
static int l_device_used = 0;
static int gyr_device_used = 0;
static int write = 0;
static int total_raw = DEFAULT_TOTAL_ROW;
static struct i2c_client *temp_client;
static struct file *filp = NULL;
static __u32 gsensor_twi_id = 0;
static __u32 ctp_twi_id = 0;
static __u32 lsensor_twi_id = 0;
static __u32 gyr_sensor_twi_id = 0;

enum twi_device_type{
        TWI_GSENSOR = 0,
        TWI_CTP,
        TWI_LSENSOR,
        TWI_GYR_SENSOR,
};
 
struct id{
        int gsensor_id;
        int ctp_id;   
        int lsensor_id;
        int gyr_sensor_id;
}write_id = {2,3,4,5};
    
struct device_config_info{
        char str_info[STRING_LENGTH];
        int str_id;
};

static struct device_config_info config_info[STRING_LENGTH];

struct base_info{
        char name[NAME_LENGTH];
        unsigned short i2c_address[ADDRESS_NUMBER];
        unsigned short chip_id_reg;
        unsigned short chip_id_reg_value[REG_VALUE_NUMBER];
};
static struct base_info sensors[] = {
        {    "bma250", {0x18, 0x19, 0x08, 0x38}, 0x00, {0x02,0x03,0xf9}},
        {   "mma8452", {0x1c, 0x1d            }, 0x0d, {0x2A          }},
        {   "mma7660", {0x4c                  }, 0x00, {0x00          }},
        {   "mma865x", {0x1d                  }, 0x0d, {0x4A,0x5A     }},
        {    "afa750", {0x3d                  }, 0x37, {0x3d,0x3c     }},
        {"lis3de_acc", {0x28, 0x29            }, 0x0f, {0x33          }},
        {"lis3dh_acc", {0x18, 0x19            }, 0x0f, {0x33          }},
        {     "kxtik", {0x0f                  }, 0x0f, {0x05,0x08     }},
        {   "dmard10", {0x18                  }, 0x00, {0x00          }},
        {   "dmard06", {0x1c                  }, 0x0f, {0x06          }},
        {   "mxc622x", {0x15                  }, 0x00, {0x00          }},    
        {  "fxos8700", {0x1c, 0x1d, 0x1e, 0x1f}, 0x0d, {0xc7          }},
        {   "lsm303d", {0x1e, 0x1d            }, 0x0f, {0x49          }},    
};

static struct base_info ctps[] = {
        { "ft5x_ts", {      0x38},   0xa3, {0x55,0x08,0x02,0x06,0xa3}},
        {   "gt82x", {      0x5d},  0xf7d, {0x13,0x27,0x28          }},
        { "gslX680", {      0x40},   0x00, {0x00                    }},
        {"gt9xx_ts", {0x14, 0x5d}, 0x8140, {0x39                    }},
        {   "gt811", {      0x5d},  0x715, {0x11                    }},
        {"aw5306_ts",{0x38, 0x39},   0x01, {0xA8                    }},
};

static struct base_info lsensors[] = {
        {"ltr_501als", {0x23   },    0x87, {0x05                    }},
};

static struct base_info gyr_sensors[] = {
        {"l3gd20_gyr", {0x6a, 0x6b}, 0x0F, {0x00D4                  }},
};

enum {
	DEBUG_I2C_DETECT        = 1U << 0,
	DEBUG_INIT              = 1U << 1,
	DEBUG_RW_FILE           = 1U << 2,
	DEBUG_OTHER             = 1U << 3,
};

struct ctp_config_info ctp_config_info = {
	.input_type = CTP_TYPE,
	.device_detect = 1,
	.name = NULL,
};


static char check_addr[15] = {0};

static u32 debug_mask = 0x0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)


static void i2c_devices_events(struct work_struct *work);
struct workqueue_struct *i2c_wq;
static DECLARE_WORK(i2c_work, i2c_devices_events);


long long get_us_time(void) 
{

    struct timeval tv;

    do_gettimeofday(&tv);

    return (long long)tv.tv_sec * 1000000ll + tv.tv_usec;

}
/*Function as i2c_master_send, and return 1 if operation is successful.*/ 
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;//д��Ϣ
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;		
	
	ret=i2c_transfer(client->adapter, &msg, 1);
	return ret;
}

/*Function as i2c_master_receive, and return 2 if operation is successful.*/
static int i2c_read_bytes_addr16(struct i2c_client *client, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	//����д��ַ
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr = client->addr;
	msgs[0].len = 2;		//data address
	msgs[0].buf = buf;
	//��������
	msgs[1].flags = I2C_M_RD;//����Ϣ
	msgs[1].addr = client->addr;
	msgs[1].len = len-2;
	msgs[1].buf = buf+2;
	
	ret=i2c_transfer(client->adapter, msgs, 2);
	return ret;
}

/*
*********************************************************************************************************
*                                   i2c_test
*
*Description: By writing device address, testing equipment, whether or not successful communication
*
*Arguments  :client      Contain equipment address of the i2c_client structure
*
*Return     : result;
*               = true,      Communication success;
*               = false,     Communication failed!
*********************************************************************************************************
*/
static bool i2c_test(struct i2c_client * client)
{
        int ret,retry;
        uint8_t test_data[1] = { 0 };	//only write a data address.
        
        for(retry=0; retry < 2; retry++) {
                ret =i2c_write_bytes(client, test_data, 1);	//Test i2c.
        	if (ret == 1)
        	        break;
        	msleep(1);
        }
        
        return ret==1 ? true : false;
} 

/*
*********************************************************************************************************
*                                   i2c_get_device_number
*
*Description: Through the device name for equipment storage locations
*
*Arguments  :info                   Storage equipment information structure
*            support_number         Storage equipment information structure body size(support equipment number)
*            name                   To find the device name
*Return     : result;
*               = suppotr_number,   Equipment in the position of the array;
*               = -1,               In the array device name not found!
*********************************************************************************************************
*/
static int i2c_get_device_number(struct base_info *info, int support_number, char *name)
{
        int ret = -1;
        
        if(strlen(name)){
                while(support_number--){
                        if (!strncmp(name, info[support_number].name, strlen(info[support_number].name))){
                                dprintk(DEBUG_INIT, "number: %d \n", support_number);
                                return support_number;
                        }
                }
        }
        dprintk(DEBUG_INIT, "-----the name is null !-----\n");
        return ret;
}
/*
*********************************************************************************************************
*                                   get_use_list
*
*Description: Will not need testing equipment of i2c address zero
*
*Arguments  :info                   Storage equipment information structure
*            support_number         Storage equipment information structure body size(support equipment number)
*            main_name              sys_config1.fex Device list the main key name
*********************************************************************************************************
*/
/*static void get_use_list(struct base_info *info, int support_number, char * main_name)
{
        int i = 0;
        script_item_u	val;

        while(support_number--){
                i = 0;
                
                if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(main_name, info[support_number].name, &val)){
	                printk("%s: script_get_item err.support_number = %d. \n", __func__, support_number);
	                continue;
	        }
	        
                 while((info[support_number].i2c_address[i++]) && (i < ADDRESS_NUMBER)){
	                if(val.val == 0){
	                        info[support_number].i2c_address[i-1] = 0x00;      
	                }    
                }
        } 
 }*/

/*
*********************************************************************************************************
*                                   sysconfing_get_para
*
*Description: Read sys_config1. Fex the configuration information
*Return     : sys_config para!
*               
*********************************************************************************************************
*/

static int sysconfing_get_para(char * mname, char * subname)
{
        script_item_u	val;
        
        if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(mname, subname, &val)){
	        printk("%s: type err device_used val.val:%d \n", __func__, val.val);
	        goto script_get_item_err;
	}
        return val.val;
        
script_get_item_err:
        dprintk(DEBUG_INIT, "line:%d:=========script_get_item_err============\n", __LINE__);
	return 0;
}

/*
*********************************************************************************************************
*                                   sensor_fetch_sysconfig_para
*
*Description: Read sys_config.fex the configuration information
*Return     : result;
*               = 0,  Read correct
*               = -1, Read error!
*********************************************************************************************************
*/
static int sensor_fetch_sysconfig_para(void)
{
	int ret = -1;
	int gsensor_used = 0;
		
	dprintk(DEBUG_INIT, "========%s===================\n", __func__);
	 
        gsensor_used = sysconfing_get_para("gsensor_para", "gsensor_used");
        
	if(1 == gsensor_used ){               
        	gsensor_twi_id = sysconfing_get_para("gsensor_para", "gsensor_twi_id");;
        	dprintk(DEBUG_INIT, "%s: gsensor_twi_id is %d. \n", __func__, gsensor_twi_id);
        
	        /*g_device_used = sysconfing_get_para("gsensor_list_para", "gsensor_det_used");*/
	        g_device_used = 1;
	        /*if(g_device_used)
                        get_use_list(sensors,g_support_number, "gsensor_list_para");*/
                ret = 0;
	
	}else{
	        dprintk(DEBUG_INIT, "%s: gsensor_unused. \n",  __func__);
		ret = -1;
	}

	return ret;
}

/*
*********************************************************************************************************
*                                   lsensor_fetch_sysconfig_para
*
*Description: Read sys_config.fex the configuration information
*Return     : result;
*               = 0,  Read correct
*               = -1, Read error!
*********************************************************************************************************
*/
static int lsensor_fetch_sysconfig_para(void)
{
	int ret = -1;
	int lsensor_used = 0;

	dprintk(DEBUG_INIT, "========%s===================\n", __func__);

        lsensor_used = sysconfing_get_para("ls_para", "ls_used");

	if(1 == lsensor_used ){
		lsensor_twi_id = sysconfing_get_para("ls_para", "ls_twi_id");;
		dprintk(DEBUG_INIT, "%s: light sensor_twi_id is %d. \n", __func__, lsensor_twi_id);

	        l_device_used = 0;

                ret = 0;
	}else{
	        dprintk(DEBUG_INIT, "%s: light sensor_unused. \n",  __func__);
		ret = -1;
	}

	return ret;
}

/*
*********************************************************************************************************
*                                   gyr_sensor_fetch_sysconfig_para
*
*Description: Read sys_config.fex the configuration information
*Return     : result;
*               = 0,  Read correct
*               = -1, Read error!
*********************************************************************************************************
*/
static int gyr_sensor_fetch_sysconfig_para(void)
{
	int ret = -1;
	int gyr_sensor_used = 0;

	dprintk(DEBUG_INIT, "========%s===================\n", __func__);

        gyr_sensor_used = sysconfing_get_para("gy_para", "gy_used");

	if(1 == gyr_sensor_used){
		gyr_sensor_twi_id = sysconfing_get_para("gy_para", "gy_twi_id");;
		dprintk(DEBUG_INIT, "%s: gyr sensor_twi_id is %d. \n", __func__, gyr_sensor_twi_id);

	        gyr_device_used = 1;

                ret = 0;
	}else{
	        dprintk(DEBUG_INIT, "%s: gyr sensor_unused. \n",  __func__);
		ret = -1;
	}

	return ret;
}

/*
*********************************************************************************************************
*                                   ctp_fetch_sysconfig_para
*
*Description: Read sys_config.Fex the configuration information
*Return     : result;
*               = 0,  Read correct
*               = -1, Read error!
*********************************************************************************************************
*/
static int ctp_fetch_sysconfig_para(void)
{
     int ret = -1;
     int ctp_used = 0;
	        
     dprintk(DEBUG_INIT, "========%s===================\n", __func__);

	if (input_fetch_sysconfig_para(&(ctp_config_info.input_type))){
		printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
		return 0;
	}else{
		ret = input_init_platform_resource(&(ctp_config_info.input_type));
		if (0 != ret) {
			printk("%s:ctp_ops.init_platform_resource err. \n", __func__);    
		}
    }

	if (ctp_config_info.ctp_used == 0) {
		printk("*** ctp_used set to 0 !\n");
		printk("*** if use ctp,please put the sys_config.fex ctp_used set to 1. \n");
		return 0;
	}
        
    ctp_twi_id = ctp_config_info.twi_id;
    dprintk(DEBUG_INIT,"%s: ctp_twi_id is %d. \n", __func__, ctp_twi_id);
                
	/*c_device_used = sysconfing_get_para("ctp_list_para", "ctp_det_used");*/
    c_device_used = 1;
	/*if(c_device_used)
	 get_use_list(ctps,c_support_number, "ctp_list_para");*/
                
    return ret;
}

static int is_alpha(char chr)
{
        int ret = -1;
        
        ret = ((chr >= 'a') && (chr <= 'z') ) ? 0 : 1;
                
        return ret;
}
/*
*********************************************************************************************************
*                                   get_device_name
*
*Description: For the name of the equipment
*Arguments: buf     Source string
*           name    Used for storage device name of the variable name
*Return     : result;
*               = 0, string error!
*               = 1, Read correct!
*********************************************************************************************************
*/

static int  get_device_name(char *buf, char * name)
{
        int s1 = 0, i = 0;
        int ret = -1;
        char ch = '\"';
        char tmp_name[64];
        char * str1;
        char * str2;
        
        memset(&tmp_name, 0, sizeof(tmp_name));
        if(!strlen (buf)){
                printk("%s:the buf is null!\n", __func__);
                return 0; 
        }
        
        str1 = strchr(buf, ch);
        str2 = strrchr(buf, ch);
        if((str1 == NULL) || (str2 == NULL)) {
                printk("the str1 or str2 is null!\n");
                return 1;
        }
                   
        ret = str1 - buf + 1;  
        s1 =  str2 - buf; 
        dprintk(DEBUG_INIT,"----ret : %d,s1 : %d---\n ", ret, s1);
        
        while(ret != s1)
        {
                tmp_name[i++] = buf[ret++];         
         
        }
        tmp_name[i] = '\0';
        strcpy(name, tmp_name);
        
        dprintk(DEBUG_INIT, "name:%s\n", name);
        return 1;
}
/*
*********************************************************************************************************
*                                   match_device_keyname
*
*Description: To match the device key word
*Arguments: buf            Source string
*           key_name       Equipment key
*           name           Used for storage device name of the variable name
*Return     : result;
*               = 0, error!
*               = 1,correct!
*********************************************************************************************************
*/
static int match_device_keyname(char * src_buf, char * key_name, char * name)
{
        int ret = -1;
        
        if(!(strlen(src_buf) && strlen(key_name))){
                pr_info("%s:the src string is null!\n", __func__);
                return 0;
        }
        
        if(strncmp(src_buf, key_name, strlen(key_name))) {
                dprintk(DEBUG_INIT, "%s:Src_name and key_name don't match!\n", __func__);
                ret = 0;
        }else{
                if(get_device_name(src_buf, name))
                        ret = 1;
                else{ 
                        ret = 0;
                }
        }
        
        return ret;
              
}
/*
*********************************************************************************************************
*                                   match_device_name
*
*Description: According to the source file information find device name
*
*
*********************************************************************************************************
*/
static void match_device_name(void)
{
         int row_number = 0;

         row_number = total_raw;

	 while(row_number--){
	       dprintk(DEBUG_INIT, "config_info[%d].str_info:%s\n", row_number, config_info[row_number].str_info);

	        if(is_alpha(config_info[row_number].str_info[0])){
	                continue;
	        } else if (match_device_keyname(config_info[row_number].str_info,
						GSENSOR_DEVICE_KEY_NAME, gsensor_name)) {
	                write_id.gsensor_id = config_info[row_number].str_id;
	                dprintk(DEBUG_INIT, "gsensor_name:%s,id:%d\n", gsensor_name, write_id.gsensor_id);
	          
	        } else if (match_device_keyname(config_info[row_number].str_info,
						CTP_DEVICE_KEY_NAME, ctp_name)) {
    	                write_id.ctp_id = config_info[row_number].str_id;
    	                dprintk(DEBUG_INIT, "ctp_name:%s,id:%d\n", ctp_name, write_id.ctp_id);                
	        } else if (match_device_keyname(config_info[row_number].str_info,
						LSENSOR_DEVICE_KEY_NAME, lsensor_name)) {
	                write_id.lsensor_id = config_info[row_number].str_id;
	                dprintk(DEBUG_INIT, "lsensor_name:%s,id:%d\n", lsensor_name, write_id.lsensor_id);
	        } else if (match_device_keyname(config_info[row_number].str_info,
						GYR_SENSOR_DEVICE_KEY_NAME, gyr_sensor_name)) {
	                write_id.lsensor_id = config_info[row_number].str_id;
	                dprintk(DEBUG_INIT, "gyr_sensor_name:%s,id:%d\n", gyr_sensor_name, write_id.gyr_sensor_id);
	        }
	 }
}
/*
*********************************************************************************************************
*                                   analytic_device_info
*
*Description: Analysis of the configuration information, to break them down into a line of information 
*             stored in the device_config_info structure body in the variable
*Arguments: src_string     Configuration information source file
*           info           Storage line string structure body variable
*Return     : result;
*               = 1,correct!
*********************************************************************************************************
*/
static int analytic_device_info(char * src_string, struct device_config_info info[])
{
        int ret = -1;
        int i = 0, j = 0, k = 0;
        total_raw = 0;
        
        if(!strlen(src_string) ) {
                dprintk(DEBUG_INIT, "%s: the src string is null !\n", __func__);
                ret = 0;
                return ret;
        }  
               
        while(src_string[i++]){  
                info[k].str_info[j++] = src_string[i-1];
                
                if(src_string[i-1] == '\n'){
                        total_raw++; 
                        info[k].str_info[j] = '\0';
                        info[k].str_id = k;         
                        k += 1;
                        j = 0;
                    
                }   
        } 
        
        ret = 1;
        
        return ret;

}
/*
*********************************************************************************************************
*                                   get_device_info
*
*Description: Reading configuration information
*             
*Arguments: tmp     Store configuration information variable
*Return     : result;
*             = ret,Read results!
*********************************************************************************************************
*/
static int get_device_info(char * tmp)
{
        mm_segment_t old_fs;
        int ret;
        
        filp = filp_open(FILE_DIR,O_RDWR | O_CREAT, 0666);
        if(!filp || IS_ERR(filp)){
                printk("%s:open error ....IS(filp):%ld\n", __func__, IS_ERR(filp));
                return -1;
        } 
        
        old_fs = get_fs();
        set_fs(get_ds());
        filp->f_op->llseek(filp, 0, 0);
        ret = filp->f_op->read(filp, tmp, FILE_LENGTH, &filp->f_pos);
        
        if(ret <= 0){
                printk("%s:read erro or read null !\n", __func__);
        }
        
        set_fs(old_fs);
        filp_close(filp, NULL);
        
        return ret;

}

/*
*********************************************************************************************************
*                                   i2c_update_device_info
*
*Description:Update the device name configuration information
*             
*Arguments: key_name       Equipment key
*           device_name    Equipment name
*           id             Equipment in configuration information of line number
*Return     : result;
*             = true, updated!
*             = false, Wrong information!
*********************************************************************************************************
*/
static bool i2c_update_device_info(char * key_name, char * device_name, int id)
{
        if((key_name == NULL)||(id < 0)){
                dprintk(DEBUG_INIT, "%s:key_name is null or the id is error !\n", __func__);
                return false;
        }
        memset(&config_info[id].str_info, 0, sizeof(config_info[id].str_info));
        sprintf(config_info[id].str_info, "%s=\"%s\"\n", key_name, device_name); 
        return true;
}

/*
*********************************************************************************************************
*                                   write_device_info
*
*Description:Will be updated configuration information to write configuration files
*             
*********************************************************************************************************
*/
static int write_device_info(void)
{
        mm_segment_t old_fs;
        int ret = 0, i =0;
        
        filp = filp_open(FILE_DIR, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(!filp || IS_ERR(filp)){
                printk("%s:open error ....IS(filp):%ld\n", __func__,IS_ERR(filp));
                return -1;
        } 
        
        old_fs = get_fs();
        set_fs(get_ds());
        
        filp->f_op->llseek(filp, 0, 0);
        
        while(i < DEFAULT_TOTAL_ROW ){
                ret = filp->f_op->write(filp, config_info[i].str_info, strlen(config_info[i].str_info), &filp->f_pos);
                i++;
        }
        
        set_fs(old_fs);
        filp_close(filp, NULL);
        return ret;
}
/*
*********************************************************************************************************
*                                   set_device_info
*
*Description:Write equipment information
*             
*********************************************************************************************************
*/
static int set_device_info(void)
{
        int ret = 0;

        dprintk(DEBUG_INIT, "%s:write device info !\n", __func__);
        if(write) {       
                memset(&config_info[0].str_info, 0, sizeof(config_info[0].str_info));
                memset(&config_info[1].str_info, 0, sizeof(config_info[1].str_info));
                strcpy(config_info[0].str_info, NOTE_INFO1);
                strcpy(config_info[1].str_info, NOTE_INFO2);
        }  
        write_device_info();
        
        return ret;
}
/*
*********************************************************************************************************
*                                   chip_id_match_value
*
*Description: The judge read chip id register value whether with equipment chip id the default values are equal
*Arguments: info      Device description information array
*           number    Matching value in the position of the array
*           value     To match the value
*Return     : result;
*               = 0,  Matching failure!
*               = 1, Matching success!
*********************************************************************************************************
*/
static bool chip_id_match_value(struct base_info *info, int number, int value)
{
        int i = 0;
        while(info[number].chip_id_reg_value[i])
        {
                if(info[number].chip_id_reg_value[i] == value){
                        dprintk(DEBUG_I2C_DETECT,"Chip id value equal!\n");
                        return true;
                }
                i++;
        }
        dprintk(DEBUG_I2C_DETECT,"Chip id value does not match!--value:%d--\n",value);
        return false;
}


static int i2c_check_addr(unsigned short address)
{
        int ret = 0;
        
        while((check_addr[ret]) && (ret < 15)) {
                if(check_addr[ret] == address){
                        dprintk(DEBUG_I2C_DETECT, "address:0x%x\n", check_addr[ret]);
                        return 1;
                }
                 ret++;              
        }
        
        return 0;
}

static int i2c_device_get_adapter(enum twi_device_type type)
{
         __u32 twi_id = -1;
        
        struct i2c_adapter *adap ;

        switch(type){
        case TWI_CTP:
                twi_id = ctp_twi_id;
                break;
        case TWI_GSENSOR:
                twi_id = gsensor_twi_id;
                break;
        case TWI_LSENSOR:
                twi_id = lsensor_twi_id;
                break;
        case TWI_GYR_SENSOR:
                twi_id = gyr_sensor_twi_id;
                break;
        }
        
        adap = i2c_get_adapter(twi_id);
        temp_client->adapter = adap;	
        
        return twi_id;
}

/*
*********************************************************************************************************
*                                   i2c_device_i2c_test
*
*Description: Scanning device address, check the I2C communication success
*
*Arguments  :info                   Storage equipment information structure
*            i2c_address_number     The position of structures   
*            type                   Device type
*Return     : result;
*             = 1 ,Successful communication ;
*             = 0 ,Failure communication!
*********************************************************************************************************
*/
static int i2c_device_i2c_test(struct base_info *info, int i2c_address_number, enum twi_device_type type)
{
        int ret = 0, addr_scan = 0;
        int twi_id = -1;
        
        twi_id = i2c_device_get_adapter(type);        
        if (!i2c_check_functionality(temp_client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;
          
        if(twi_id == temp_client->adapter->nr){
                while((info[i2c_address_number].i2c_address[addr_scan++]) && (addr_scan < (ADDRESS_NUMBER+1))){
                        
                        temp_client->addr = info[i2c_address_number].i2c_address[addr_scan - 1];
                        dprintk(DEBUG_I2C_DETECT,"%s: name = %s, addr = 0x%x\n", __func__,
                                info[i2c_address_number].name, temp_client->addr); 
                                
                        
                        if(i2c_check_addr(temp_client->addr)) {
                                ret = 0;
                                continue;
                        }
   
                        ret = i2c_test(temp_client);
                        if(!ret){
                                check_addr[strlen(check_addr)] = temp_client->addr;
                                ret = 0;
                        	continue;
                        }else{         	    
                                dprintk(DEBUG_I2C_DETECT, "I2C connection sucess!\n");
                                break;
                        }  
                }   
        }
        
        return ret;
}
/*
*********************************************************************************************************
*                                   i2c_get_chip_id_value_addr16
*
*Description: By reading chip_id register for 16 bit address, get chip_id value
*
*Arguments  :address     Register address
*Return     : result;
*             Chip_id value
*********************************************************************************************************
*/
static int i2c_get_chip_id_value_addr16(unsigned short address)
{
        int value = -1;
        uint8_t buf[5] = {0};
        int retry = 2;
        
        if(address & 0xff00){
                buf[0] = address >> 8;
                buf[1] = address & 0x00ff;
        }
        while(retry--) {
                value = i2c_read_bytes_addr16(temp_client, buf, 3);
                if(value != 2){
                        msleep(1);
                        printk("%s:read chip id error!", __func__);
                }else{
                        break;
                }
        }
        
        value = buf[2] & 0xffff;
        dprintk(DEBUG_I2C_DETECT, "buf[0]:0x%x, buf[1]:0x%x, buf[2]:0x%x, value:0x%x\n",
                buf[0], buf[1], buf[2], value);
        
        return value;     
}
/*
*********************************************************************************************************
*                                   i2c_get_chip_id_value_addr16
*
*Description: By reading chip_id register for 8 bit address, get chip_id value
*
*Arguments  :address     Register address
*Return     : result;
*             Chip_id value
*********************************************************************************************************
*/
static int i2c_get_chip_id_value_addr8(unsigned short address)
{
        int value = -1;
        int retry = 2;
        
        while(retry--) {
                value = i2c_smbus_read_byte_data(temp_client, address);
                if(value < 0){
                        msleep(1);
                }else { 
                        break;
                }
        }
        value = value & 0xffff;

        return value;
}

/*
*********************************************************************************************************
*                                   chip_id_detect_ctp
*
*Description:Gsensor matching the chip id value
*
*Arguments  :info                   Storage equipment information structure
*            i2c_address_number     The position of structures  
*Return     : result;
*             1 , success
*             0 , failure!
*********************************************************************************************************
*/
static int chip_id_detect(struct base_info *info, int i2c_address_number)
{
        int detect_value = 0;
        int ret = -1, i = 0;
        unsigned short temp_addr;
        
        temp_addr = info[i2c_address_number].chip_id_reg;
        
        while (!((info[i2c_address_number].chip_id_reg_value[i++]) && (i < REG_VALUE_NUMBER))){
                detect_value = 1;
                dprintk(DEBUG_I2C_DETECT,"-----%s:chip_id_reg value:0x%x",
                        __func__, info[i2c_address_number].chip_id_reg_value[i-1]);
                return detect_value;
        }
        
        if(temp_addr & 0xff00){
                ret = i2c_get_chip_id_value_addr16(temp_addr);
        }else{
                ret = i2c_get_chip_id_value_addr8(temp_addr);
        }
        
        dprintk(DEBUG_I2C_DETECT,"-----%s:chip_id_reg:0x%x, chip_id_value = 0x%x-----\n",
                __func__, temp_addr,ret);
                
        detect_value = chip_id_match_value(info, i2c_address_number, ret);
        
        return detect_value;
}
/*
*********************************************************************************************************
*                                   i2c_update_device_name
*
*Description: The name of the update your equipment
*
*Arguments  :name      Equipment name
*            value     The i2c communication results, 0 indicates no find equipment, 1 said have found equipment
*            type      Device type
*********************************************************************************************************
*/
static void i2c_update_device_name(char * name, int value, enum twi_device_type type)
{
  
        switch(type){
        case TWI_GSENSOR: 
                memset(&gsensor_name, 0, sizeof(gsensor_name));
                if(value){
                        strcpy(gsensor_name, name);
                }
                break;
        case TWI_CTP:
                memset(&ctp_name, 0, sizeof(ctp_name));
                if(value){
                        strcpy(ctp_name, name);
                }
                break;
        case TWI_LSENSOR:
                memset(&lsensor_name, 0, sizeof(lsensor_name));
                if(value){
                        strcpy(lsensor_name, name);
                }
                break;
        case TWI_GYR_SENSOR:
                memset(&gyr_sensor_name, 0, sizeof(gyr_sensor_name));
                if(value){
                        strcpy(gyr_sensor_name, name);
                }
                break;
        }
         dprintk(DEBUG_INIT, "%s:gsensor_name:%s, ctp_name:%s, lsensor_name:%s,gyr_sensor_name:%s\n",
                __func__, gsensor_name, ctp_name ,lsensor_name ,gyr_sensor_name);
       
}
/*
*********************************************************************************************************
*                                   i2c_detect_device
*
*Description: Through the i2c address scanning, get support equipment
*Arguments: info              Device description information array
*           support_number    Equipment information describing the size of an array
*           address_number    Would like to start scanning position
*           type              Device type
*Return: result;
*        scan_number      Scanning frequency
*             
*********************************************************************************************************
*/
static int i2c_detect_device(struct base_info *info, int support_number, 
        int address_number, enum twi_device_type type )
{
        int ret = 0;
        int scan_number = 0;
        int report_value = 0;
        int i2c_address_number = 0;
        
        i2c_address_number = address_number;
        
        while(scan_number < (support_number)){
                dprintk(DEBUG_I2C_DETECT, "scan_number:%d, i2c_address_number:%d\n", 
                        scan_number, i2c_address_number);
                scan_number++;
                i2c_address_number = (i2c_address_number == support_number) ? 0 : i2c_address_number;
               
                ret = i2c_device_i2c_test(info, i2c_address_number, type);
                if(!ret){
                        i2c_address_number++; 
        	         continue;
        	     }   
        	
                report_value = chip_id_detect(info, i2c_address_number);

        	         
        	   if(report_value){   	            
        	           break; 
        	    }
                i2c_address_number++; 
         
        }
        
        i2c_update_device_name(info[i2c_address_number].name, report_value, type);
            
        return scan_number;
}

/*
*********************************************************************************************************
*                                   i2c_device_used_gsensor
*
*Description:Gsensor equipment test
*Return: result;
*        flag      Whether to need to update configuration information   
*********************************************************************************************************
*/
static int i2c_device_used_gsensor(void)
{
        int ret = -1, flag = 0;
        int gsensor_scan_number = -1;
                    
        ret = i2c_get_device_number(sensors, g_support_number, gsensor_name);
        
        if(ret != -1){
                gsensor_scan_number = i2c_detect_device(sensors, g_support_number, ret, TWI_GSENSOR);
                flag |= ((gsensor_scan_number != 1) && (strlen(gsensor_name))) ?  0x02: 0;
        }
        else{
                i2c_detect_device(sensors, g_support_number, 0, TWI_GSENSOR);
                flag |= strlen(gsensor_name) ? 0x02 : 0;
        }
        
        if(flag & 0x02){
                i2c_update_device_info(GSENSOR_DEVICE_KEY_NAME, gsensor_name, write_id.gsensor_id);
        }
            
        return flag;
}

/*
*********************************************************************************************************
*                                   i2c_device_used_lsensor
*
*Description:lsensor equipment test
*Return: result;
*        flag      Whether to need to update configuration information
*********************************************************************************************************
*/
static int i2c_device_used_lsensor(void)
{
        int ret = -1, flag = 0;
        int lsensor_scan_number = -1;

        ret = i2c_get_device_number(lsensors, l_support_number, lsensor_name);

        if(ret != -1){
                lsensor_scan_number = i2c_detect_device(lsensors, l_support_number, ret, TWI_LSENSOR);
                flag |= ((lsensor_scan_number != 1) && (strlen(lsensor_name))) ?  0x02: 0;
        }
        else{
                i2c_detect_device(lsensors, l_support_number, 0, TWI_LSENSOR);
                flag |= strlen(lsensor_name) ? 0x02 : 0;
        }

        if(flag & 0x02){
                i2c_update_device_info(LSENSOR_DEVICE_KEY_NAME, lsensor_name, write_id.lsensor_id);
        }

        return flag;
}

/*
*********************************************************************************************************
*                                   i2c_device_used_gyr_sensor
*
*Description:gyr_sensor equipment test
*Return: result;
*        flag      Whether to need to update configuration information
*********************************************************************************************************
*/
static int i2c_device_used_gyr_sensor(void)
{
        int ret = -1, flag = 0;
        int gyr_sensor_scan_number = -1;

        ret = i2c_get_device_number(gyr_sensors, gyr_support_number, gyr_sensor_name);

        if(ret != -1){
                gyr_sensor_scan_number = i2c_detect_device(gyr_sensors, gyr_support_number, ret, TWI_GYR_SENSOR);
                flag |= ((gyr_sensor_scan_number != 1) && (strlen(gyr_sensor_name))) ?  0x02: 0;
        }
        else{
                i2c_detect_device(gyr_sensors, gyr_support_number, 0, TWI_GYR_SENSOR);
                flag |= strlen(gyr_sensor_name) ? 0x02 : 0;
        }

        if(flag & 0x02){
                i2c_update_device_info(GYR_SENSOR_DEVICE_KEY_NAME, gyr_sensor_name, write_id.gyr_sensor_id);
        }

        return flag;
}

/*
*********************************************************************************************************
*                                   i2c_device_used_ctp
*
*Description:ctp equipment test
*Return: result;
*        flag      Whether to need to update configuration information   
*********************************************************************************************************
*/
static int i2c_device_used_ctp(void)
{
        int ret = -1, flag = 0;
        int ctp_scan_number = -1;
       
        ret = i2c_get_device_number(ctps, c_support_number, ctp_name);
        
        if(ret != -1){
                ctp_scan_number = i2c_detect_device(ctps, c_support_number, ret, TWI_CTP);
                flag |= ((ctp_scan_number != 1) && (strlen(ctp_name))) ?  1 : 0;
        }else{
               i2c_detect_device(ctps, c_support_number, 0, TWI_CTP);
               flag |= strlen(ctp_name) ? 1 : 0;
        }
        
        if(flag & 0x01){
                i2c_update_device_info(CTP_DEVICE_KEY_NAME,ctp_name,write_id.ctp_id);
        }

        return flag;
}
//static void i2c_devices_events(struct work_struct *work)
//{
//}
static void i2c_devices_events(struct work_struct *work)
{
        int flag = 0;
        char tmp[FILE_LENGTH];
        int ret = -1;
        
        memset(&tmp, 0, sizeof(tmp));
        
        dprintk(DEBUG_INIT, "g_device_used:%d,c_device_used:%d,l_device_used:%d,gyr_device_used:%d\n",
				g_device_used, c_device_used, l_device_used, gyr_device_used);
        if(c_device_used | g_device_used | l_device_used | gyr_device_used) {
        	ret = get_device_info(tmp);
        	if(ret <= 0){
        	          printk("get twi config erro!\n");
        	          write = 1;
        	}else{
        	        ret = analytic_device_info(tmp, config_info);
        	        if(!ret){
        	                printk("analytic_device_info erro!\n");
        	                write = 1;
        	        }else{
                                match_device_name();
                        }
                }
        }
        
        i2c_update_device_info(GSENSOR_DEVICE_KEY_NAME, gsensor_name, write_id.gsensor_id);
        i2c_update_device_info(CTP_DEVICE_KEY_NAME,ctp_name,write_id.ctp_id);
        i2c_update_device_info(LSENSOR_DEVICE_KEY_NAME, lsensor_name, write_id.lsensor_id);
        i2c_update_device_info(GYR_SENSOR_DEVICE_KEY_NAME, gyr_sensor_name, write_id.gyr_sensor_id);
        
        if(c_device_used){
			    if(ctp_config_info.name == NULL)
					input_wakeup(&(ctp_config_info.input_type), 0, 20); 
				else if(strcmp(ctp_config_info.name,"aw5306_ts")==0)
                	input_wakeup(&(ctp_config_info.input_type), 1, 20);
				else
					input_wakeup(&(ctp_config_info.input_type), 0, 20); 
                msleep(100);
                flag |= i2c_device_used_ctp(); 
        }else{
                if(strlen(ctp_name)) {
                        flag |= 1;
                        memset(&ctp_name, 0, sizeof(ctp_name));
                        i2c_update_device_info(CTP_DEVICE_KEY_NAME, ctp_name, write_id.ctp_id);
                }
        }
        
        if(g_device_used){
	        flag |= i2c_device_used_gsensor();
	}else{
	        if(strlen(gsensor_name)){
                        flag |= 1;
                        memset(&gsensor_name, 0, sizeof(gsensor_name));
                        i2c_update_device_info(GSENSOR_DEVICE_KEY_NAME, gsensor_name, write_id.gsensor_id);
                }
        }

        if(l_device_used){
	        flag |= i2c_device_used_lsensor();
	}else{
	        if(strlen(lsensor_name)){
                        flag |= 1;
                        memset(&lsensor_name, 0, sizeof(lsensor_name));
                        i2c_update_device_info(LSENSOR_DEVICE_KEY_NAME, lsensor_name, write_id.lsensor_id);
                }
        }

        if(gyr_device_used){
	        flag |= i2c_device_used_gyr_sensor();
	}else{
	        if(strlen(gyr_sensor_name)){
                        flag |= 1;
                        memset(&gyr_sensor_name, 0, sizeof(gyr_sensor_name));
                        i2c_update_device_info(GYR_SENSOR_DEVICE_KEY_NAME, gyr_sensor_name,
						write_id.gyr_sensor_id);
                }
        }
        dprintk(DEBUG_INIT,"write:%d,flag:%d",write,flag);
        if(write | flag){
                set_device_info();
        }
        
	kfree(temp_client);
	input_free_platform_resource(&(ctp_config_info.input_type));
        
}
static int i2c_hardware_detect(void)
{
        struct i2c_client *client;
        
        client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client)
                return -ENOMEM;

	temp_client = client;
	
	i2c_wq = create_singlethread_workqueue("i2c_wq");
	if (i2c_wq == NULL) {
		printk("create i2c_wq fail!\n");
		return 0;
	}

	queue_work(i2c_wq, &i2c_work);

	return 1;
    
}

static int __init i2c_hardware_init(void)
{
	int i = 0;
	dprintk(DEBUG_INIT,"======%s=========. \n", __func__);

	write = 0;
	g_support_number = (sizeof(sensors)) / (sizeof(sensors[0]));
	c_support_number = (sizeof(ctps)) / (sizeof(ctps[0]));
	l_support_number = (sizeof(lsensors)) / (sizeof(lsensors[0]));
	gyr_support_number = (sizeof(gyr_sensors)) / (sizeof(gyr_sensors[0]));

        memset(&gsensor_name, 0, sizeof(gsensor_name));
        memset(&ctp_name, 0, sizeof(ctp_name));
        memset(&lsensor_name, 0, sizeof(lsensor_name));
        memset(&gyr_sensor_name, 0, sizeof(gyr_sensor_name));
        
        while(i++ < STRING_LENGTH){
                memset(&config_info[i-1].str_info, 0, sizeof(config_info[i-1].str_info));
        }

	if(sensor_fetch_sysconfig_para()){
		printk("%s: sensor_fetch_sysconfig_para err.\n", __func__);
	}

	if(lsensor_fetch_sysconfig_para()){
		printk("%s: light sensor_fetch_sysconfig_para err.\n", __func__);
	}

	if(gyr_sensor_fetch_sysconfig_para()){
		printk("%s: gyr sensor_fetch_sysconfig_para err.\n", __func__);
	}

	if(ctp_fetch_sysconfig_para()){
		printk("%s: ctp_fetch_sysconfig_para err.\n", __func__);
	} 

        if(!i2c_hardware_detect())
                printk("the client is null!\n");
		
	
	return 0;
}

static void __exit i2c_hardware_exit(void)
{
        printk(" Device driver exit!\n");    
    
}
/*************************************************************************************/
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_AUTHOR("Olina yin");
MODULE_DESCRIPTION("GSENSOR  Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(i2c_hardware_init);
module_exit(i2c_hardware_exit);


