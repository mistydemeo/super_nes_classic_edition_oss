/*************************************************************************/ /*!
@File
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "pvrsrv_device.h"
#include "syscommon.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif

#include <linux/version.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/io.h>
#include <linux/clk/sunxi_name.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/platform.h>

#ifdef CONFIG_SW_POWERNOW	
#include <mach/powernow.h>
#endif /* CONFIG_SW_POWERNOW */

#ifdef CONFIG_CPU_BUDGET_THERMAL
#include <linux/cpu_budget_cooling.h>
#endif

#define AXI_CLK_FREQ 320000000

static RGX_TIMING_INFORMATION	gsRGXTimingInfo;
static RGX_DATA			gsRGXData;
static PVRSRV_DEVICE_CONFIG 	gsDevices[1];
static PVRSRV_SYSTEM_CONFIG 	gsSysConfig;

static PHYS_HEAP_FUNCTIONS	gsPhysHeapFuncs;
#if defined(TDMETACODE)
static PHYS_HEAP_CONFIG		gsPhysHeapConfig[3];
#else
static PHYS_HEAP_CONFIG		gsPhysHeapConfig[1];
#endif

struct clk *gpu_core_clk        = NULL;
struct clk *gpu_mem_clk         = NULL;
struct clk *gpu_axi_clk         = NULL;
struct clk *pll9_clk            = NULL;
struct regulator *rgx_regulator = NULL;
int b_throttle                  = 0;
static unsigned int vol_list[2]=
{
	900000,      /* for performance mode */
	1100000      /* for extremity mode */
};

static unsigned int freq_list[2]=
{
	384000000,   /* for performance mode */
	528000000    /* for extremity mode */
};

static IMG_VOID AssertGpuResetSignal(void)
{
	unsigned int reg;
	reg = readl(SUNXI_CCM_MOD_VBASE + NMI_IRQ_STATUS);
	reg &= ~(1<<9);
	writel(reg, SUNXI_CCM_MOD_VBASE + NMI_IRQ_STATUS);
	reg = readl(SUNXI_CCM_MOD_VBASE + NMI_INIT_CTRL_REG);
	reg &= ~(1<<3);
	writel(reg, SUNXI_CCM_MOD_VBASE + NMI_INIT_CTRL_REG);
}

static IMG_VOID DeAssertGpuResetSignal(void)
{
	unsigned int reg;
	reg = readl(SUNXI_CCM_MOD_VBASE + NMI_INIT_CTRL_REG);
	reg |= 1<<3;
	writel(reg, SUNXI_CCM_MOD_VBASE + NMI_INIT_CTRL_REG);
	reg = readl(SUNXI_CCM_MOD_VBASE + NMI_IRQ_STATUS);
	reg |= 1<<9;
	writel(reg, SUNXI_CCM_MOD_VBASE + NMI_IRQ_STATUS);	
}

static IMG_VOID RgxEnableClock(void)
{
	if(gpu_core_clk->enable_count == 0)
	{
		unsigned int reg;
		
		if(clk_prepare_enable(pll9_clk))
			printk(KERN_ERR "Failed to enable pll9 clock!\n");	
		if(clk_prepare_enable(gpu_core_clk))
			printk(KERN_ERR "Failed to enable core clock!\n");
		if(clk_prepare_enable(gpu_mem_clk))
			printk(KERN_ERR "Failed to enable mem clock!\n");
		if(clk_prepare_enable(gpu_axi_clk))
			printk(KERN_ERR "Failed to enable axi clock!\n");
			
		writel(1 << 8, SUNXI_GPU_CTRL_VBASE + 0x18);
		
		/* enalbe gpu ctrl clk */
		reg = readl(SUNXI_CCM_MOD_VBASE + 0x180);
		reg |= 1<<3;
		writel(reg, SUNXI_CCM_MOD_VBASE + 0x180);
	}
}

static IMG_VOID RgxDisableClock(void)
{				
	if(gpu_core_clk->enable_count == 1)
	{
		unsigned int reg;
				
		/* disalbe gpu ctrl clk */
		reg = readl(SUNXI_CCM_MOD_VBASE + 0x180);
		reg &= ~(1<<3);
		writel(reg, SUNXI_CCM_MOD_VBASE + 0x180);
		
		clk_disable_unprepare(gpu_axi_clk);
		clk_disable_unprepare(gpu_mem_clk);	
		clk_disable_unprepare(gpu_core_clk);
		clk_disable_unprepare(pll9_clk);
	}
}

static IMG_VOID RgxEnablePower(void)
{
	if(!regulator_is_enabled(rgx_regulator))
	{
		regulator_enable(rgx_regulator); 		
	}
}

static IMG_VOID RgxDisablePower(void)
{
	if(regulator_is_enabled(rgx_regulator))
	{
		regulator_disable(rgx_regulator); 		
	}
}

static PVRSRV_ERROR AwPrePowerState(PVRSRV_DEV_POWER_STATE eNewPowerState, PVRSRV_DEV_POWER_STATE eCurrentPowerState, IMG_BOOL bForced)
{
	if(eNewPowerState == PVRSRV_DEV_POWER_STATE_ON)
	{
		RgxEnableClock();
	}
	return PVRSRV_OK;
}

static PVRSRV_ERROR AwPostPowerState(PVRSRV_DEV_POWER_STATE eNewPowerState, PVRSRV_DEV_POWER_STATE eCurrentPowerState, IMG_BOOL bForced)
{
	if(eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF)
	{
		RgxDisableClock();
	}	
	return PVRSRV_OK;
}

PVRSRV_ERROR RgxResume(void)
{
	RgxEnablePower();
	
	mdelay(2);
	
	/* set external isolation invalid */
	writel(0, SUNXI_R_PRCM_VBASE + GPU_PWROFF_GATING);

	RgxEnableClock();
	
	DeAssertGpuResetSignal();
	
	return PVRSRV_OK;
}

PVRSRV_ERROR RgxSuspend(void)
{	
	AssertGpuResetSignal();
	
	/* set external isolation valid */
	writel(1, SUNXI_R_PRCM_VBASE + GPU_PWROFF_GATING);
	
	RgxDisablePower();
	
	return PVRSRV_OK;
}

#ifdef CONFIG_SW_POWERNOW
static int powernow_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
	if(mode == SW_POWERNOW_EXTREMITY && !b_throttle){
		if(regulator_set_voltage(rgx_regulator, vol_list[1], vol_list[1]) != 0)
		{
			printk(KERN_ERR "Failed to set gpu power voltage!\n");
		}
		if(clk_set_rate(pll9_clk, freq_list[1]))
		{
			printk(KERN_ERR "Failed to set gpu pll clock!\n");
		}
	}
	else
	{
		if(clk_set_rate(pll9_clk, freq_list[0]))
		{
			printk(KERN_ERR "Failed to set gpu pll clock!\n");
		}
		if(regulator_set_voltage(rgx_regulator, vol_list[0], vol_list[0]) != 0)
		{
			printk(KERN_ERR "Failed to set gpu power voltage!\n");
		}		
	}
    return retval;
}
 
static struct notifier_block rgx_powernow_notifier = {
.notifier_call = powernow_notifier_call,
};
#endif /* CONFIG_SW_POWERNOW */

#ifdef CONFIG_CPU_BUDGET_THERMAL
static int rgx_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
	if(mode == BUDGET_GPU_THROTTLE)
    {
        b_throttle=1;
        {
            if(clk_set_rate(pll9_clk, freq_list[0]))
            {
                printk(KERN_ERR "Failed to set gpu pll clock!\n");
            }
            if(regulator_set_voltage(rgx_regulator, vol_list[0], vol_list[0]) != 0)
            {
                printk(KERN_ERR "Failed to set gpu power voltage!\n");
            }
        }
    }
    else
        b_throttle=0;
	
	return retval;
}
static struct notifier_block rgx_throttle_notifier = {
.notifier_call = rgx_throttle_notifier_call,
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

static IMG_VOID RgxSunxiInit(void)
{	
	rgx_regulator = regulator_get(NULL, "axp22_dcdc2");
	if (IS_ERR(rgx_regulator)) {
	    printk(KERN_ERR "Failed to get rgx regulator \n");
        rgx_regulator = NULL;
	}
	
	regulator_set_voltage(rgx_regulator, vol_list[0], vol_list[0]);
	
	gpu_core_clk  = clk_get(NULL, GPUCORE_CLK);
	gpu_mem_clk   = clk_get(NULL, GPUMEM_CLK);
	gpu_axi_clk   = clk_get(NULL, GPUAXI_CLK);
	pll9_clk      = clk_get(NULL, PLL9_CLK);
	
	if(clk_set_rate(pll9_clk, freq_list[0]))
		printk(KERN_ERR "Failed to set pll9 clock\n");
		
	if(clk_set_rate(gpu_core_clk, freq_list[0]))
		printk(KERN_ERR "Failed to set gpu core clock\n");
		
	if(clk_set_rate(gpu_mem_clk, freq_list[0]))
		printk(KERN_ERR "Failed to set gpu mem clock\n");
    
    if(clk_set_rate(gpu_axi_clk, AXI_CLK_FREQ))
		printk(KERN_ERR "Failed to set gpu axi clock\n");
		
	RgxResume();
#ifdef CONFIG_SW_POWERNOW	
	register_sw_powernow_notifier(&rgx_powernow_notifier);
#endif /* CONFIG_SW_POWERNOW */	

#ifdef CONFIG_CPU_BUDGET_THERMAL
	register_budget_cooling_notifier(&rgx_throttle_notifier);
#endif /* CONFIG_CPU_BUDGET_THERMAL */

	/* print gpu init information */
	printk(KERN_INFO "=========================================================\n");
	printk(KERN_INFO "               GPU Init Information                      \n");
	printk(KERN_INFO "gpu voltage         : %d mV\n", regulator_get_voltage(rgx_regulator)/1000);
	printk(KERN_INFO "pll9 clock frequency: %ld MHz\n", clk_get_rate(pll9_clk)/1000000);
	printk(KERN_INFO "core clock frequency: %ld MHz\n", clk_get_rate(gpu_core_clk)/1000000);
	printk(KERN_INFO "mem clock frequency : %ld MHz\n", clk_get_rate(gpu_mem_clk)/1000000);
	printk(KERN_INFO "axi clock frequency : %ld MHz\n", clk_get_rate(gpu_axi_clk)/1000000);
	printk(KERN_INFO "=========================================================\n");
}

/*
	CPU to Device physcial address translation
*/
static
IMG_VOID UMAPhysHeapCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
									   IMG_DEV_PHYADDR *psDevPAddr,
									   IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	
	psDevPAddr->uiAddr = psCpuPAddr->uiAddr;
}

/*
	Device to CPU physcial address translation
*/
static
IMG_VOID UMAPhysHeapDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
									   IMG_CPU_PHYADDR *psCpuPAddr,
									   IMG_DEV_PHYADDR *psDevPAddr)				  
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);
	
	psCpuPAddr->uiAddr = psDevPAddr->uiAddr;
}

/*
	SysCreateConfigData
*/
PVRSRV_ERROR SysCreateConfigData(PVRSRV_SYSTEM_CONFIG **ppsSysConfig)
{
	/*
	 * Setup information about physical memory heap(s) we have
	 */
	gsPhysHeapFuncs.pfnCpuPAddrToDevPAddr = UMAPhysHeapCpuPAddrToDevPAddr;
	gsPhysHeapFuncs.pfnDevPAddrToCpuPAddr = UMAPhysHeapDevPAddrToCpuPAddr;

	gsPhysHeapConfig[0].ui32PhysHeapID = 0;
	gsPhysHeapConfig[0].pszPDumpMemspaceName = "SYSMEM";
	gsPhysHeapConfig[0].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[0].psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[0].hPrivData = IMG_NULL;

#if defined(TDMETACODE)
	gsPhysHeapConfig[1].ui32PhysHeapID = 1;
	gsPhysHeapConfig[1].pszPDumpMemspaceName = "TDMETACODEMEM";
	gsPhysHeapConfig[1].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[1].psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[1].hPrivData = IMG_NULL;

	gsPhysHeapConfig[2].ui32PhysHeapID = 2;
	gsPhysHeapConfig[2].pszPDumpMemspaceName = "TDSECUREBUFMEM";
	gsPhysHeapConfig[2].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[2].psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[2].hPrivData = IMG_NULL;
#endif

	gsSysConfig.pasPhysHeaps = &(gsPhysHeapConfig[0]);
	gsSysConfig.ui32PhysHeapCount = IMG_ARR_NUM_ELEMS(gsPhysHeapConfig);

	gsSysConfig.pui32BIFTilingHeapConfigs = gauiBIFTilingHeapXStrides;
	gsSysConfig.ui32BIFTilingHeapCount = IMG_ARR_NUM_ELEMS(gauiBIFTilingHeapXStrides);

	/*
	 * Setup RGX specific timing data
	 */
	gsRGXTimingInfo.ui32CoreClockSpeed        = freq_list[0];
	gsRGXTimingInfo.bEnableActivePM           = IMG_TRUE;
	gsRGXTimingInfo.bEnableRDPowIsland        = IMG_TRUE;
	gsRGXTimingInfo.ui32ActivePMLatencyms     = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/*
	 *Setup RGX specific data
	 */
	gsRGXData.psRGXTimingInfo = &gsRGXTimingInfo;
#if defined(TDMETACODE)
	gsRGXData.bHasTDMetaCodePhysHeap = IMG_TRUE;
	gsRGXData.uiTDMetaCodePhysHeapID = 1;

	gsRGXData.bHasTDSecureBufPhysHeap = IMG_TRUE;
	gsRGXData.uiTDSecureBufPhysHeapID = 2;
#endif

	/*
	 * Setup RGX device
	 */
	gsDevices[0].eDeviceType            = PVRSRV_DEVICE_TYPE_RGX;
	gsDevices[0].pszName                = "RGX";

	/* Device setup information */
	gsDevices[0].sRegsCpuPBase.uiAddr   = SUNXI_GPU_PBASE;
	gsDevices[0].ui32RegsSize           = SUNXI_GPU_SIZE;
	gsDevices[0].ui32IRQ                = SUNXI_IRQ_GPU;
	gsDevices[0].bIRQIsShared           = IMG_FALSE;

	/* Device's physical heap IDs */
	gsDevices[0].aui32PhysHeapID[PVRSRV_DEVICE_PHYS_HEAP_GPU_LOCAL] = 0;
	gsDevices[0].aui32PhysHeapID[PVRSRV_DEVICE_PHYS_HEAP_CPU_LOCAL] = 0;

	/* No power management on no HW system */
	gsDevices[0].pfnPrePowerState       = AwPrePowerState;
	gsDevices[0].pfnPostPowerState      = AwPostPowerState;

	/* No clock frequency either */
	gsDevices[0].pfnClockFreqGet        = IMG_NULL;

	/* No interrupt handled either */
	gsDevices[0].pfnInterruptHandled    = IMG_NULL;

	gsDevices[0].pfnCheckMemAllocSize   = SysCheckMemAllocSize;

	gsDevices[0].hDevData               = &gsRGXData;

	/*
	 * Setup system config
	 */
	gsSysConfig.pszSystemName = RGX_SUNXI_SYSTEM_NAME;
	gsSysConfig.uiDeviceCount = sizeof(gsDevices)/sizeof(gsDevices[0]);
	gsSysConfig.pasDevices = &gsDevices[0];

	/* No power management on no HW system */
	gsSysConfig.pfnSysPrePowerState = IMG_NULL;
	gsSysConfig.pfnSysPostPowerState = IMG_NULL;

	/* no cache snooping */
	gsSysConfig.eCacheSnoopingMode = PVRSRV_SYSTEM_SNOOP_NONE;

	/* Setup other system specific stuff */
#if defined(SUPPORT_ION)
	IonInit(NULL);
#endif

	*ppsSysConfig = &gsSysConfig;
	RgxSunxiInit();
	return PVRSRV_OK;
}

/*
	SysDestroyConfigData
*/
IMG_VOID SysDestroyConfigData(PVRSRV_SYSTEM_CONFIG *psSysConfig)
{
	PVR_UNREFERENCED_PARAMETER(psSysConfig);

#if defined(SUPPORT_ION)
	IonDeinit();
#endif
}

PVRSRV_ERROR SysAcquireSystemData(IMG_HANDLE hSysData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

PVRSRV_ERROR SysReleaseSystemData(IMG_HANDLE hSysData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_SYSTEM_CONFIG *psSysConfig)
{
	PVR_UNREFERENCED_PARAMETER(psSysConfig);

	return PVRSRV_OK;
}

/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/
