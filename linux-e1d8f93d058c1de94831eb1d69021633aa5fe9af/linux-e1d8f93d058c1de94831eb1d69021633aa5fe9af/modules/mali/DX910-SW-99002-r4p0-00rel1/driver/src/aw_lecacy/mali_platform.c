/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>

#include <linux/err.h>
#include <linux/module.h>  
#include <linux/clk.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include <linux/pm_runtime.h>
#include <linux/clk/sunxi_name.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_SW_POWERNOW
#include <mach/powernow.h>
#define SW_NORMAL SW_POWERNOW_NORMAL 
#define SW_PERFORMANCE SW_POWERNOW_PERFORMANCE
#define SW_EXTREMITY SW_POWERNOW_EXTREMITY
#define SW_SUSPEND SW_POWERNOW_USEREVENT 
#define SW_VIDEOMODE       3
#define SW_LOWPOWER 	SW_POWERNOW_SW_USB
#define SW_MAXPOWER    SW_POWERNOW_MAXPOWER
#else
#include <linux/cpu_budget_cooling.h>
#define SW_EXTREMITY 			0       // extramity
#define SW_PERFORMANCE 		1       // performance
#define SW_NORMAL 				2 
#define SW_VIDEOMODE       3
#define SW_SUSPEND 			4	
#define SW_LOWPOWER 			5 
#define SW_MAXPOWER      6 
#endif



#define DEF_PLL8_CLK   381000000

// use for dcdc2 transition voltage for in/out benchmark mode
#define DCDC2_TRANSITION_VOL		(1200000)


static int mali_clk_div    = 1;
static int mali_clk_flag   = 0;
struct clk *h_mali_clk  = NULL;
struct clk *h_gpu_pll   = NULL;
module_param(mali_clk_div, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(mali_clk_div, "Clock divisor for mali");
struct mutex gpu_mutex;
extern unsigned long totalram_pages;

struct __fb_addr_para {
unsigned int fb_paddr;
unsigned int fb_size;
};

extern void sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);

struct mali_dvfstab
{
    unsigned int vol_max;
    unsigned int freq_max;
    unsigned int freq_min;
    unsigned int mbus_freq;
};
struct mali_dvfstab mali_dvfs_table[] = {
    //extremity
    {1.2*1000*1000, 480*1000*1000, 480*1000*1000, 400*1000*1000},
    //perf
    {1.1*1000*1000, 360*1000*1000, 360*1000*1000, 300*1000*1000},
    //normal
    {1.1*1000*1000, 360*1000*1000, 360*1000*1000, 300*1000*1000},
    //video mode
    {1.1*1000*1000, 252*1000*1000, 252*1000*1000, 300*1000*1000},
    //suspend mode
    {1.0*1000*1000, 240*1000*1000, 240*1000*1000, 300*1000*1000},
    //lowpowr
    {950*1000, 120*1000*1000, 120*1000*1000, 300*1000*1000}
};

unsigned int cur_mode   = SW_PERFORMANCE;
unsigned int user_event = 0;
struct regulator *mali_regulator = NULL;
unsigned int g_mali_freq = 0;


static void mali_platform_device_release(struct device *device);
void mali_gpu_utilization_handler(struct mali_gpu_utilization_data *data);


static void mali_mode_set(unsigned long setmode);
static int mali_powernow_mod_change(unsigned long code, void *cmd);
////////////////////////////////////////////////////////////////////////

typedef enum mali_power_mode_tag
{
	MALI_POWER_MODE_ON,
	MALI_POWER_MODE_LIGHT_SLEEP,
	MALI_POWER_MODE_DEEP_SLEEP,
} mali_power_mode;

typedef enum mali_standby_mode_tag
{
	GPU_SUSPEND = 1,
	GPU_RESUME,
}mali_standby_mode;

static mali_standby_mode gpu_powermode = GPU_RESUME;
static struct resource mali_gpu_resources[]=
{
                                     
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(SUNXI_GPU_PBASE, SUNXI_IRQ_GPUGP, SUNXI_IRQ_GPUGPMMU, \
                                        SUNXI_IRQ_GPUPP0, SUNXI_IRQ_GPUPPMMU0, SUNXI_IRQ_GPUPP1, SUNXI_IRQ_GPUPPMMU1)

};

static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.release = mali_platform_device_release,
    .dev.coherent_dma_mask = DMA_BIT_MASK(32),
};

static struct mali_gpu_device_data mali_gpu_data = 
{
//    .dedicated_mem_start = SW_GPU_MEM_BASE - PLAT_PHYS_OFFSET,
//    .dedicated_mem_size = SW_GPU_MEM_SIZE,
    .shared_mem_size = 512*1024*1024,
//    .fb_start = SW_FB_MEM_BASE,
//    .fb_size = SW_FB_MEM_SIZE,
    .utilization_interval = 2000,
    .utilization_callback = mali_gpu_utilization_handler,
};

static void mali_platform_device_release(struct device *device)
{
    MALI_DEBUG_PRINT(2,("mali_platform_device_release() called\n"));
}

_mali_osk_errcode_t mali_platform_init(void)
{
	

	unsigned long rate = 0;
	unsigned int vol = 0;
	script_item_u   mali_use, clk_drv;
	mali_regulator = regulator_get(NULL, "axp22_dcdc2");
	if (IS_ERR(mali_regulator)) {
	    printk(KERN_ERR "get mali regulator failed\n");
        mali_regulator = NULL;
	}
	
   	//get mali clk
	h_mali_clk = clk_get(NULL, GPU_CLK);
	if(!h_mali_clk || IS_ERR(h_mali_clk)){
		MALI_PRINT(("try to get mali clock failed!\n"));
		return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): get %s handle success!\n", __func__, __LINE__, GPU_CLK);

	/* get gpu pll clock */
#ifdef CONFIG_ARCH_SUN8IW3	
	h_gpu_pll = clk_get(NULL, PLL8_CLK);
#endif /* CONFIG_ARCH_SUN8IW3 */
#ifdef CONFIG_ARCH_SUN8IW5
	h_gpu_pll = clk_get(NULL, PLL_GPU_CLK);
#endif 
	if(!h_gpu_pll || IS_ERR(h_gpu_pll)){
		MALI_PRINT(("try to get ve pll clock failed!\n"));
        return _MALI_OSK_ERR_FAULT;
	} else
		pr_info("%s(%d): get %s handle success!\n", __func__, __LINE__, "pll8");


	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("clock", "pll8", &clk_drv)) {
		pr_info("%s(%d): get clock->pll8 success! pll=%d\n", __func__,
			__LINE__, clk_drv.val);
		if(clk_drv.val > 0)
			rate = clk_drv.val*1000*1000;
	} else
	{
		rate = DEF_PLL8_CLK;
		pr_info("%s(%d): get mali_para->mali_clkdiv failed!\n", __func__, __LINE__);
		printk("MALI:set gpu_pll8 = %d\n",DEF_PLL8_CLK);
	}


	if(clk_set_rate(h_gpu_pll, rate)){
		MALI_PRINT(("try to set gpu pll clock failed!\n"));
		return _MALI_OSK_ERR_FAULT;
	} else
		printk("%s(%d): set pll8 clock rate success!\n", __func__, __LINE__);

	//set mali clock
	rate = clk_get_rate(h_gpu_pll);
	printk("%s(%d): get gpu pll rate %d!\n", __func__, __LINE__, (int)rate);
    
	cur_mode = SW_PERFORMANCE;
	mali_dvfs_table[SW_NORMAL].freq_max = rate;
  mali_dvfs_table[SW_NORMAL].freq_min = rate;
	mali_dvfs_table[SW_PERFORMANCE].freq_max =rate;
  mali_dvfs_table[SW_PERFORMANCE].freq_min = rate;
	if(mali_regulator)
	{
		vol = regulator_get_voltage(mali_regulator);
		mali_dvfs_table[SW_NORMAL].vol_max = vol;
		mali_dvfs_table[SW_PERFORMANCE].vol_max = vol;
		mali_dvfs_table[SW_VIDEOMODE].vol_max = vol;
		printk(KERN_ERR "%s(%d): get gpu pll volt is %d mV!\n", __func__, __LINE__, (int)vol);
    regulator_put(mali_regulator);
    mali_regulator = NULL;
	}
	
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_use)) {
		pr_info("%s(%d): get mali_para->mali_used success! mali_use %d\n", __func__, __LINE__, mali_use.val);
		if(mali_use.val == 1) {
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_clkdiv", &clk_drv)) {
				pr_info("%s(%d): get mali_para->mali_clkdiv success! clk_drv %d\n", __func__,
					__LINE__, clk_drv.val);
				if(clk_drv.val > 0)
					mali_clk_div = clk_drv.val;
				else
					mali_clk_div = 1;

				pr_info("%s(%d): get mali_para->mali_clkdiv success! mali_clk_div %d\n", __func__,
					__LINE__, mali_clk_div);


			} else
			{
				pr_info("%s(%d): get mali_para->mali_clkdiv failed,default div 1!\n", __func__, __LINE__);
				mali_clk_div = 1;
			}
		}
	} else
	{
		pr_info("%s(%d): get mali_para->mali_used failed!default div 1\n", __func__, __LINE__);
		mali_clk_div = 1;
	}
	rate = rate / mali_clk_div; 

	pr_info("%s(%d): mali_clk_div %d\n", __func__, __LINE__, mali_clk_div);
	if(clk_set_rate(h_mali_clk, rate)){
		MALI_PRINT(("try to set mali clock failed!\n"));
		return _MALI_OSK_ERR_FAULT;
	} else
		printk("%s(%d): set mali clock rate success!\n", __func__, __LINE__);
		
	if(mali_clk_flag == 0)//jshwang add 2012-8-23 16:05:50
	{
		printk(KERN_WARNING "enable mali clock\n");
		mali_clk_flag = 1;	
		if(clk_prepare_enable(h_gpu_pll)){
		    MALI_PRINT(("try to enable mali clock failed!\n"));
		}
		mdelay(5);
		if(clk_prepare_enable(h_mali_clk)){
		    MALI_PRINT(("try to enable mali clock failed!\n"));
		}
	} 
  MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
    /*close mali axi/apb clock*/
    if(mali_clk_flag == 1){
        //MALI_PRINT(("disable mali clock\n"));
        mali_clk_flag = 0;
        clk_disable_unprepare(h_mali_clk);
        clk_disable_unprepare(h_gpu_pll);
    }
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{

	unsigned int mvol = 0;
	unsigned int freq = 0;

	if(power_mode == MALI_POWER_MODE_ON){
    if(mali_clk_flag == 0){
        mali_clk_flag = 1;
#if 0
		    mali_regulator = regulator_get(NULL, "axp22_dcdc2");
		    if (IS_ERR(mali_regulator)) {
		        printk(KERN_ERR "get mali regulator failed, %s, %d\n", __func__, __LINE__);
		        mali_regulator = NULL;
		    }
		
				if(mali_regulator)
				{
					mvol = regulator_get_voltage(mali_regulator);
					if(mvol != 0)
					{
						if(mvol < mali_dvfs_table[SW_SUSPEND].vol_max)
							freq = mali_dvfs_table[SW_LOWPOWER].freq_max; 
						else if(mvol < mali_dvfs_table[SW_PERFORMANCE].vol_max)
							freq = mali_dvfs_table[SW_SUSPEND].freq_max; 
						else if(mvol > mali_dvfs_table[SW_PERFORMANCE].vol_max + 300*1000*1000)
							freq = mali_dvfs_table[SW_EXTREMITY].freq_max; 
						else
							freq = mali_dvfs_table[SW_PERFORMANCE].freq_max;
					}
		
				   regulator_put(mali_regulator);
				   mali_regulator = NULL;
		
				}
				if(gpu_powermode == GPU_RESUME)
				{
					if(g_mali_freq != freq  && freq != 0)
					{
					    if(clk_set_rate(h_gpu_pll, freq)){
								printk(KERN_ERR "try to set gpu pll clock failed!\n");
					    } else {
							 	MALI_DEBUG_PRINT(2, ( "%s(%d): set pll8 clock rate success!\n", __func__, __LINE__));
							  printk(KERN_ERR "%s(%d): set pll8 clock rate success,freq = %d Mhz!\n", __func__, __LINE__,freq/1000/1000);
					    }
					    g_mali_freq = freq;
					}
				}
#endif
		    if(clk_prepare_enable(h_gpu_pll)){
	          printk("try to enable mali pll clock failed!\n");
	          return _MALI_OSK_ERR_FAULT;
	      }
		    mdelay(5);//for clock stable
	      if(clk_prepare_enable(h_mali_clk)){
	        	printk("try to enable mali clock failed!\n");;
	        	return _MALI_OSK_ERR_FAULT;
	      }
     	}
    } else if(power_mode == MALI_POWER_MODE_LIGHT_SLEEP){
        //close mali axi/apb clock/
        if(mali_clk_flag == 1){
            mali_clk_flag = 0;
            clk_disable_unprepare(h_mali_clk);
	    			clk_disable_unprepare(h_gpu_pll);
        }
    } else if(power_mode == MALI_POWER_MODE_DEEP_SLEEP){
    	//close mali axi/apb clock
        if(mali_clk_flag == 1){
            mali_clk_flag = 0;
				    clk_disable_unprepare(h_mali_clk);
				    clk_disable_unprepare(h_gpu_pll);
        }
    }
    MALI_SUCCESS;
}

int mali_platform_adjust_gpu_freq(int on)
{
	unsigned int freq = 0;
	mutex_lock(&gpu_mutex);

	if(on &&  (cur_mode != SW_EXTREMITY)) //on extremity mode need't ajust freq
	{
		gpu_powermode = GPU_SUSPEND;
		freq = mali_dvfs_table[SW_LOWPOWER].freq_max; 
		if(g_mali_freq != freq  && freq != 0)
		{

		    mali_dev_pause();
		    if(clk_set_rate(h_gpu_pll, freq)){
					printk(KERN_ERR "try to set gpu pll clock failed!\n");
		    } else {
			 		printk(KERN_ERR "%s(%d): set pll8 clock rate success,freq = %d Mhz!\n", __func__, __LINE__,freq/1000/1000);
		    }
		    g_mali_freq = freq;
		    mali_dev_resume();

		}
	}
	else if(cur_mode != SW_EXTREMITY) //on extremity mode need't ajust freq
	{

		freq = mali_dvfs_table[SW_PERFORMANCE].freq_max; 
		if(g_mali_freq != freq  && freq != 0)
		{

		    mali_dev_pause();
		    if(clk_set_rate(h_gpu_pll, freq)){
					printk(KERN_ERR "try to set gpu pll clock failed!\n");
		    } else {
			 		printk(KERN_ERR "%s(%d): set pll8 clock rate success,freq = %d Mhz!\n", __func__, __LINE__,freq/1000/1000);
		    }
		    g_mali_freq = freq;
		    mali_dev_resume();
		}
		gpu_powermode = GPU_RESUME;
	}
	mutex_unlock(&gpu_mutex);
}


#ifdef CONFIG_SW_POWERNOW
static int powernow_notifier_call(struct notifier_block *this, unsigned long code, void *cmd)
{
    if (cur_mode == code){
        return 0;
    }
     MALI_DEBUG_PRINT(2, ("mali mode change\n\n\n\n\n\n\n"));
    return mali_powernow_mod_change(code, cmd);
}


static struct notifier_block powernow_notifier = {
	.notifier_call = powernow_notifier_call,
};
#else
static int gpu_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd) 
{
	unsigned code = 0;
	if(cmd == 0)
		return 0;
	code = *(unsigned int *)cmd;

    switch (mode) {
	    case BUDGET_GPU_THROTTLE:
	        // performance
	        if (cur_mode == SW_PERFORMANCE){
       			 return 0;
    		}
	        cur_mode = SW_PERFORMANCE;
	        mali_powernow_mod_change(cur_mode, code);
	        break; 
	    case BUDGET_GPU_UNTHROTTLE:
	        if (code == 1) {
	        // extremity
		        if (cur_mode == SW_EXTREMITY){
	       			 return 0;
	    		}
			cur_mode =  SW_EXTREMITY;
	        } else if (code == 0) {
	        // performance
	        	if (cur_mode == SW_PERFORMANCE){
       			 		return 0;
    			}
			cur_mode = SW_PERFORMANCE;
	        }
		 else if (code == 2) {
	        //videomode 
	        	if (cur_mode == SW_VIDEOMODE){
       			 		return 0;
			}
			cur_mode = SW_VIDEOMODE;
	        }
		else
			return 0;

		mali_powernow_mod_change(cur_mode, code);
	        break;
	    default:
	    	break;
    }
    
}

static struct notifier_block gpu_throttle_notifier = { 
    .notifier_call = gpu_throttle_notifier_call,
};
#endif

static int mali_freq_init(void)
{
    script_item_u   mali_use, mali_max_freq, mali_vol;



	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_use)) {
		pr_info("%s(%d): get mali_para->mali_used success! mali_use %d\n", __func__, __LINE__, mali_use.val);
		if(mali_use.val == 1) {
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_extreme_freq", &mali_max_freq)) {
                    if (mali_max_freq.val > 0 ){
                        mali_dvfs_table[SW_EXTREMITY].freq_max = mali_max_freq.val*1000*1000;
                        mali_dvfs_table[SW_EXTREMITY].freq_min = mali_max_freq.val*1000*1000;
                        MALI_DEBUG_PRINT(2, ("mali_extreme_freq:%d Mhz\n", mali_max_freq.val));
                    }    
					else
					{
						printk(KERN_ERR "mali_extreme used default %d Mhz\n",mali_dvfs_table[SW_EXTREMITY].freq_max);
					}
            }
			else
			{
				printk(KERN_ERR "mali_extreme used default %d Mhz\n",mali_dvfs_table[SW_EXTREMITY].freq_max);
			}
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_extreme_vol", &mali_vol)) {
                    if (mali_vol.val > 0 ){
                        mali_dvfs_table[SW_EXTREMITY].vol_max = mali_vol.val*1000;
                        MALI_DEBUG_PRINT(2, ("mali_extreme_vol:%d Mv\n", mali_vol.val));
                    }    
					else
					{
						printk(KERN_ERR "mali_extreme used default %d uV\n",mali_dvfs_table[SW_EXTREMITY].vol_max);
					}
            }
			else
			{
				printk(KERN_ERR "mali_extreme used default %d uV\n",mali_dvfs_table[SW_EXTREMITY].vol_max);
			}
			
		}
	} else
		printk(KERN_ERR "%s(%d): get mali_para->mali_used failed,use default!\n", __func__, __LINE__);
  
#ifdef CONFIG_SW_POWERNOW
    register_sw_powernow_notifier(&powernow_notifier);
#else
    register_budget_cooling_notifier(&gpu_throttle_notifier);
#endif

	return 0;
}

int sun8i_mali_platform_device_register(void)
{
    int err;

    unsigned long mem_sz = 0;
    struct __fb_addr_para fb_addr_para={0};

    mutex_init(&gpu_mutex);	
    sunxi_get_fb_addr_para(&fb_addr_para);
    MALI_DEBUG_PRINT(2,("sun8i__mali_platform_device_register() called\n"));
    printk("mali:fb_start = 0x%x,sz = %x\n",fb_addr_para.fb_paddr,fb_addr_para.fb_size);

    err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
    if (0 == err){
        //mali_gpu_data.dedicated_mem_start = gpu_addr.paddr - PLAT_PHYS_OFFSET;
        //mali_gpu_data.dedicated_mem_size = gpu_addr.size;
        mali_gpu_data.fb_start = fb_addr_para.fb_paddr;
        mali_gpu_data.fb_size = fb_addr_para.fb_size;
	
				mem_sz = (totalram_pages  * PAGE_SIZE )/1024; 
				if(mem_sz > 512*1024)
				{
					mali_gpu_data.shared_mem_size = 1024*1024*1024;
					printk(KERN_ERR "mem %ld kB,mali shared mem is 1G\n",mem_sz);
				}
				else
				{
					mali_gpu_data.shared_mem_size = 512*1024*1024;
					printk(KERN_ERR "mem %ld KB,mali shared mem is 512M\n",mem_sz);
			
				}

        err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
        if(0 == err){
            err = platform_device_register(&mali_gpu_device);
            if (0 == err){
                if(_MALI_OSK_ERR_OK != mali_platform_init())return _MALI_OSK_ERR_FAULT;
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif
                MALI_PRINT(("sun8i_mali_platform_device_register() sucess!!\n"));
				mali_freq_init();
				printk(KERN_ERR "MALI INIT SUCCESS!\n");
                return 0;
            }
        }

        MALI_DEBUG_PRINT(0,("sun8i_mali_platform_device_register() add data failed!\n"));

        platform_device_unregister(&mali_gpu_device);
    }
	
    return err;
}
static int mali_powernow_mod_change(unsigned long code, void *cmd)
{

    switch (code) {
        case SW_EXTREMITY:
        case SW_PERFORMANCE:
        case SW_NORMAL:
        case SW_VIDEOMODE:
            mali_mode_set(code);
            break;
        default:
            MALI_DEBUG_PRINT(2, ("powernow no such mode:%d, plz check!\n",code));
            break;
    }
    return 0;
}




void mali_platform_device_unregister(void)
{
  MALI_DEBUG_PRINT(2, ("mali_platform_device_unregister() called!\n"));
  
  mali_platform_deinit();

#ifdef CONFIG_SW_POWERNOW
	unregister_sw_powernow_notifier(&powernow_notifier);
#endif

  platform_device_unregister(&mali_gpu_device);
	if (mali_regulator) {
		   regulator_put(mali_regulator);
		   mali_regulator = NULL;
	}
}


void mali_gpu_utilization_handler(struct mali_gpu_utilization_data *data)
{
}

static int mali_dvfs_change_status(struct regulator *mreg, unsigned int vol,
                                    struct clk *mali_clk, unsigned int mali_freq, 
                                    struct clk *mbus_clk, unsigned int mbus_freq,unsigned int seq)
{
	int rate = 0, ret = 0;
	unsigned int mvol   = 0;
	
  mutex_lock(&gpu_mutex);	
	mreg = regulator_get(NULL, "axp22_dcdc2");
	if (IS_ERR(mreg)) {
	    printk(KERN_ERR "get mali regulator failed, %s, %d\n", __func__, __LINE__);
        mreg = NULL;
	}
    mali_dev_pause();
	if(seq)
	{
		if (mreg && vol != 0){
			mvol = regulator_get_voltage(mreg);
			if(mvol != vol) {
			ret = regulator_set_voltage(mreg, vol, vol);
				if (ret < 0) {
					MALI_DEBUG_PRINT_ERROR(("warning!!! set gpu volt:%d->%d uV failed! not set clock rate\n",
								mvol, vol));
                    mali_dev_resume();
                    mutex_unlock(&gpu_mutex);	
					return ret;
				}
			}
			mvol = regulator_get_voltage(mreg);
			MALI_DEBUG_PRINT(2, ("set gpu volt:%d uV!\n",mvol));
		}
		if(mali_freq != 0 && mali_clk)
		{
		    if(clk_set_rate(mali_clk, mali_freq)){
				printk(KERN_ERR "try to set gpu pll clock failed!\n");
                mali_dev_resume();
				mutex_unlock(&gpu_mutex);	
				return _MALI_OSK_ERR_FAULT;
			} else
				 MALI_DEBUG_PRINT(2,( "%s(%d): set pll8 clock rate success!\n", __func__, __LINE__));
		}
		//set mali clock
		if(mali_clk)
		{
			rate = clk_get_rate(mali_clk);
			printk( "%s(%d): get gpu pll rate %d!\n", __func__, __LINE__, (int)rate);
		}
	}
	else
	{

		if(mali_freq != 0 && mali_clk)
		{
		    if(clk_set_rate(mali_clk, mali_freq)){
				printk(KERN_ERR "try to set gpu pll clock failed!\n");
				 mali_dev_resume();
				mutex_unlock(&gpu_mutex);	
				return _MALI_OSK_ERR_FAULT;
			} else
				 MALI_DEBUG_PRINT(2, ( "%s(%d): set pll8 clock rate success!\n", __func__, __LINE__));
		}
		//set mali clock
		if(mali_clk)
		{
			rate = clk_get_rate(mali_clk);
			printk( "%s(%d): get gpu pll rate %d!\n", __func__, __LINE__, (int)rate);
		}

		if (mreg && vol != 0){
			mvol = regulator_get_voltage(mreg);
			/*
			 * dcdc2_vol have dithering to 1000mV for 1400mV->1100mV
			 * change to 1400mV->1200mV->30us_delay->1100mV can improve this problem
			 * fix system may dead when in/out benchmark mode several times
			 * */
			if(mvol != vol && mvol > DCDC2_TRANSITION_VOL) {
				regulator_set_voltage(mreg, DCDC2_TRANSITION_VOL, DCDC2_TRANSITION_VOL);
				udelay(30);
			}
			if(mvol != vol) {
				regulator_set_voltage(mreg, vol, vol);
			}
			mvol = regulator_get_voltage(mreg);
			printk( "set gpu volt:%d uV!\n",mvol);
		}

	}
	if (mreg) {
		regulator_put(mreg);
		mreg = NULL;
	}

  mali_dev_resume();
  mutex_unlock(&gpu_mutex);	
  return 0;
}

static void mali_mode_set(unsigned long setmode)
{
	unsigned mode = setmode;//defence complictly
	unsigned seq = 0;
	
	if(mali_dvfs_table[cur_mode].freq_max > mali_dvfs_table[mode].freq_max )
	{
		seq = 1;
	}

	cur_mode = setmode;
     
  //check the mode valid
  if (mode > SW_SUSPEND){
      mode = SW_PERFORMANCE;
      cur_mode = SW_PERFORMANCE;
  }
      mali_dvfs_change_status(mali_regulator, 
                  mali_dvfs_table[cur_mode].vol_max,
                  h_gpu_pll,
                  mali_dvfs_table[cur_mode].freq_max,
                  0,
                 	0,seq);
      user_event =0;
      MALI_DEBUG_PRINT(2,("mali_dvfs_work_handler set freq success, cur mode:%d, cur_freq:%d\n", 
              mode, (int)clk_get_rate(h_mali_clk),  mali_dvfs_table[cur_mode].freq_max));
	
  return;
}

