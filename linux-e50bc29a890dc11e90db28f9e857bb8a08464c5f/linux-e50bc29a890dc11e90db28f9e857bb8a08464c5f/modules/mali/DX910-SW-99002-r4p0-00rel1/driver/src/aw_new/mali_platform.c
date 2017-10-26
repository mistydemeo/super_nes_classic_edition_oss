#include "mali_kernel_common.h"
#include "mali_osk.h"

#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/module.h>  
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk-private.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>

#ifdef CONFIG_CPU_BUDGET_THERMAL
#include <linux/cpu_budget_cooling.h>
static int Is_powernow = 0;
#endif

static int mali_clk_div                  = 1;
static struct clk *h_mali_clk            = NULL;
static struct clk *h_gpu_pll             = NULL;
static struct regulator *mali_regulator  = NULL;
extern unsigned long totalram_pages;

struct __fb_addr_para {
unsigned int fb_paddr;
unsigned int fb_size;
};

extern void sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);

static unsigned int vol_list[2]=
{
	1100000,  /* for normal mode */
	1400000   /* for extremity mode */
};

static unsigned int freq_list[2]=
{
	408000000, /* for normal mode */
	600000000  /* for extremity mode */
};

typedef enum _mali_power_mode
{
	NORMAL,
	EXTREMITY,	
} mali_power_mode;

static struct mali_gpu_device_data mali_gpu_data;

static struct resource mali_gpu_resources[]=
{                                    
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(SUNXI_GPU_PBASE, SUNXI_IRQ_GPUGP, SUNXI_IRQ_GPUGPMMU, \
                                        SUNXI_IRQ_GPUPP0, SUNXI_IRQ_GPUPPMMU0, SUNXI_IRQ_GPUPP1, SUNXI_IRQ_GPUPPMMU1)
};

static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.coherent_dma_mask = DMA_BIT_MASK(32),
};

/*
***************************************************************
 @Function	  :enable_gpu_clk

 @Description :Enable gpu related clocks

 @Input		  :None

 @Return	  :None
***************************************************************
*/
void enable_gpu_clk(void)
{
	if(h_mali_clk->enable_count == 0)
	{
		if(clk_prepare_enable(h_gpu_pll))
		{
			printk(KERN_ERR "failed to enable gpu pll!\n");
		}	
		if(clk_prepare_enable(h_mali_clk))
		{
			printk(KERN_ERR "failed to enable mali clock!\n");
		}
	}
}

/*
***************************************************************
 @Function	  :disable_gpu_clk

 @Description :Disable gpu related clocks

 @Input		  :None

 @Return	  :None
***************************************************************
*/
void disable_gpu_clk(void)
{
	if(h_mali_clk->enable_count == 1)
	{
		clk_disable_unprepare(h_mali_clk);
		clk_disable_unprepare(h_gpu_pll);
	}
}

#ifdef CONFIG_CPU_BUDGET_THERMAL
/*
***************************************************************
 @Function	  :set_gpu_voltage

 @Description :Set gpu voltage

 @Input		  :vol

 @Return	  :vol
***************************************************************
*/
static int set_gpu_voltage(int vol)
{
	int cur_vol;
	cur_vol = regulator_get_voltage(mali_regulator);
	if(regulator_set_voltage(mali_regulator, (cur_vol+vol)/2, vol_list[1]) != 0)
	{
		printk(KERN_ERR "Failed to set gpu voltage!\n");
		return -1;
	}
	udelay(30);
	if(regulator_set_voltage(mali_regulator, vol, vol_list[1]) != 0)
	{
		printk(KERN_ERR "Failed to set gpu voltage!\n");
		return -2;
	}
	return vol;
}

/*
***************************************************************
 @Function	  :set_gpu_freq

 @Description :Set gpu frequency

 @Input		  :freq

 @Return	  :freq
***************************************************************
*/
static int set_gpu_freq(int freq)
{
	if(clk_set_rate(h_gpu_pll, freq))
	{
		printk(KERN_ERR "Failed to set gpu pll clock!\n");
		return -1;
	}
	return freq;
}

/*
***************************************************************
 @Function	  :mali_change_mode

 @Description :Change the mode of gpu

 @Input		  :mode

 @Return	  :mode
***************************************************************
*/
mali_power_mode mali_change_mode(mali_power_mode mode)
{
	int cur_vol;
	cur_vol = regulator_get_voltage(mali_regulator);
	if(vol_list[mode] > cur_vol)
	{
		set_gpu_voltage(vol_list[mode]);
		set_gpu_freq(freq_list[mode]);
	}
	else if(vol_list[mode] < cur_vol)
	{
		set_gpu_freq(freq_list[mode]);
		set_gpu_voltage(vol_list[mode]);
	}
	return mode;
}

/*
***************************************************************
 @Function	  :mali_throttle_notifier_call

 @Description :The callback of throttle notifier

 @Input		  :nfb, mode, cmd

 @Return	  :retval
***************************************************************
*/
static int mali_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
	mali_power_mode mode;
	if(mode == BUDGET_GPU_THROTTLE)
    {
        b_throttle=1;
		mode = NORMAL;
        Is_powernow = 0;
    }
    else
	{
        b_throttle=0;
        if(cmd && (*(int *)cmd) == 1 && !Is_powernow)
		{
			mode = EXTREMITY;
            Is_powernow = 1;
        }else if(cmd && (*(int *)cmd) == 0 && Is_powernow)
		{
            mode = NORMAL;
            Is_powernow = 0;
        }
    }
	mali_change_mode(mode);
	return retval;
}
static struct notifier_block mali_throttle_notifier = {
.notifier_call = mali_throttle_notifier_call,
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

/*
***************************************************************
 @Function	  :mali_platform_init

 @Description :Init gpu related clocks

 @Input		  :None

 @Return	  :_MALI_OSK_ERR_OK or error code
***************************************************************
*/
_mali_osk_errcode_t mali_platform_init(void)
{
	unsigned long mali_freq = freq_list[0];
	script_item_u mali_used, mali_max_freq, mali_max_vol, mali_clk_freq, mali_clkdiv;
	
	/* get gpu voltage */
	mali_regulator = regulator_get(NULL, "axp22_dcdc2");
	if (IS_ERR(mali_regulator)) {
	    printk(KERN_ERR "Failed to get mali regulator!\n");
        mali_regulator = NULL;
	}
	
	/* get mali clock */
	h_mali_clk = clk_get(NULL, GPU_CLK);
	if(!h_mali_clk || IS_ERR(h_mali_clk))
	{
		printk(KERN_ERR "Failed to get mali clock!\n");
        return _MALI_OSK_ERR_FAULT;
	} 
	
	/* get gpu pll clock */
#ifdef CONFIG_ARCH_SUN8IW3	
	h_gpu_pll = clk_get(NULL, PLL8_CLK);
#endif /* CONFIG_ARCH_SUN8IW3 */
#ifdef CONFIG_ARCH_SUN8IW5
	h_gpu_pll = clk_get(NULL, PLL_GPU_CLK);
#endif /* CONFIG_ARCH_SUN8IW5 */

	if(!h_gpu_pll || IS_ERR(h_gpu_pll))
	{
		printk(KERN_ERR "Failed to get gpu pll clock!\n");
		return _MALI_OSK_ERR_FAULT;
	} 
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_used))
	{		
		if(mali_used.val == 1)
		{
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_clkdiv", &mali_clkdiv))
			{
                if (mali_clkdiv.val > 0)
				{
                    mali_clk_div = mali_clkdiv.val;
                } 
			}
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_extreme_freq", &mali_max_freq)) 
			{
                if (mali_max_freq.val > 0 )
				{
                    freq_list[1] = mali_max_freq.val*1000*1000;
                }    
            }
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_extreme_vol", &mali_max_vol))
			{
                if (mali_max_vol.val > 0)
				{
                    vol_list[1] = mali_max_vol.val*1000;    
                } 
			}
		}
	} 			
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("clock", "pll8", &mali_clk_freq)) 
	{
		if(mali_clk_freq.val > 0)
		{
			mali_freq = mali_clk_freq.val*1000*1000;
		}
	}
	
	if(regulator_set_voltage(mali_regulator, vol_list[0], vol_list[1]) != 0)
	{
		printk(KERN_ERR "Failed to set gpu power voltage!\n");
	}
	
	if(clk_set_rate(h_gpu_pll, mali_freq))
	{
		printk(KERN_ERR "Failed to set gpu pll clock!\n");
		return _MALI_OSK_ERR_FAULT;
	}
	
	mali_freq /= mali_clk_div; 

	if(clk_set_rate(h_mali_clk, mali_freq))
	{
		printk(KERN_ERR "Failed to set mali clock!\n");
		return _MALI_OSK_ERR_FAULT;
	}
	
	enable_gpu_clk();

    MALI_SUCCESS;
}

/*
***************************************************************
 @Function	  :sunxi_mali_platform_device_register

 @Description :Register mali platform device

 @Input		  :None

 @Return	  :0 or error code
***************************************************************
*/
int sunxi_mali_platform_device_register(void)
{
    int err;
    unsigned long mem_size = 0;
    struct __fb_addr_para fb_addr_para={0};

    sunxi_get_fb_addr_para(&fb_addr_para);

    err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
    if (0 == err){
        mali_gpu_data.fb_start = fb_addr_para.fb_paddr;
        mali_gpu_data.fb_size = fb_addr_para.fb_size;	
		mem_size = (totalram_pages  * PAGE_SIZE )/1024; /* KB */
	
		if(mem_size > 512*1024)
		{
			mali_gpu_data.shared_mem_size = 1024*1024*1024;
		}
		else
		{
			mali_gpu_data.shared_mem_size = 512*1024*1024;
		}

        err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
        if(0 == err)
		{
            err = platform_device_register(&mali_gpu_device);
            if (0 == err){
                if(_MALI_OSK_ERR_OK != mali_platform_init())
				{
					return _MALI_OSK_ERR_FAULT;
				}
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif /* CONFIG_PM_RUNTIME */
				/* print mali gpu information */
				printk(KERN_INFO "=========================================================\n");
				printk(KERN_INFO "       Mali GPU Information         \n");
				printk(KERN_INFO "voltage             : %d mV\n", regulator_get_voltage(mali_regulator)/1000);
				printk(KERN_INFO "initial frequency   : %ld MHz\n", clk_get_rate(h_mali_clk)/(1000*1000));
				printk(KERN_INFO "frame buffer address: 0x%lx - 0x%lx\n", mali_gpu_data.fb_start, mali_gpu_data.fb_start + mali_gpu_data.shared_mem_size);
				printk(KERN_INFO "frame buffer size   : %ld MB\n", mali_gpu_data.shared_mem_size/(1024*1024));
				printk(KERN_INFO "=========================================================\n");
                return 0;
            }
        }

        platform_device_unregister(&mali_gpu_device);
    }

#ifdef CONFIG_CPU_BUDGET_THERMAL
	register_budget_cooling_notifier(&mali_throttle_notifier);
#endif /* CONFIG_CPU_BUDGET_THERMAL */
	
    return err;
}

/*
***************************************************************
 @Function	  :mali_platform_device_unregister

 @Description :Unregister mali platform device

 @Input		  :None

 @Return	  :None
***************************************************************
*/
void mali_platform_device_unregister(void)
{
    platform_device_unregister(&mali_gpu_device);
	if (mali_regulator) {
		   regulator_put(mali_regulator);
		   mali_regulator = NULL;
	}
	disable_gpu_clk();
}

