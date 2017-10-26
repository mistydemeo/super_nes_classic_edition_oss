/*
 * arch/arm/mach-sunxi/sun8i.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun8i platform file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/sunxi_timer.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/param.h>
#include <linux/memblock.h>
#include <linux/arisc/arisc.h>
#include <linux/dma-mapping.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>

#include <asm/pmu.h>
#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/arch_timer.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sunxi-chip.h>
#include <mach/gpio.h>

#ifdef CONFIG_SMP
extern struct smp_operations sunxi_smp_ops;
#endif

/* plat memory info, maybe from boot, so we need bkup for future use */
unsigned int mem_start = PLAT_PHYS_OFFSET;
unsigned int mem_size = PLAT_MEM_SIZE;

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
u32 ion_carveout_512m = CONFIG_ION_SUNXI_RESERVE_SIZE_512M << 20;
u32 ion_carveout_1g = CONFIG_ION_SUNXI_RESERVE_SIZE << 20;
u32 ion_cma_512m = CONFIG_ION_SUNXI_RESERVE_SIZE_512M << 20;
u32 ion_cma_1g = CONFIG_ION_SUNXI_RESERVE_SIZE << 20;
struct tag_mem32 ion_mem = { /* the real ion reserve info */
       .size  = CONFIG_ION_SUNXI_RESERVE_SIZE_512M << 20,
       .start = PLAT_PHYS_OFFSET + PLAT_MEM_SIZE - (CONFIG_ION_SUNXI_RESERVE_SIZE_512M << 20),
};

static int __init early_ion_carveout_512m(char *p)
{
       char *endp;

       ion_carveout_512m  = (u32)memparse(p, &endp);
#ifndef CONFIG_CMA
       if (mem_size <= SZ_512M) {
		ion_mem.size = ion_carveout_512m;
		ion_mem.start = mem_start + mem_size - ion_mem.size;
		early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
       }
#endif
       early_printk("ion_carveout_512m reserve: %dm!\n", (int)(ion_carveout_512m>>20));
       return 0;
}
early_param("ion_carveout_512m", early_ion_carveout_512m);

static int __init early_ion_carveout_1g(char *p)
{
       char *endp;

       ion_carveout_1g = (u32)memparse(p, &endp);
#ifndef CONFIG_CMA
       if (mem_size > SZ_512M) {
		ion_mem.size = ion_carveout_1g;
		ion_mem.start = mem_start + mem_size - ion_mem.size;
		early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
       }
#endif
       early_printk("ion_carveout_1g reserve: %dm!\n", (int)(ion_carveout_1g>>20));
       return 0;
}
early_param("ion_carveout_1g", early_ion_carveout_1g);

static int __init early_ion_cma_512m(char *p)
{
       char *endp;

       ion_cma_512m = (u32)memparse(p, &endp);
#ifdef CONFIG_CMA
       if (mem_size <= SZ_512M) {
		ion_mem.size = ion_cma_512m;
		ion_mem.start = mem_start + mem_size - ion_mem.size;
		early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
       }
#endif
       early_printk("ion_cma_512m reserve: %dm!\n", (int)(ion_cma_512m>>20));
       return 0;
}
early_param("ion_cma_512m", early_ion_cma_512m);

static int __init early_ion_cma_1g(char *p)
{
       char *endp;

       ion_cma_1g = (u32)memparse(p, &endp);
#ifdef CONFIG_CMA
       if (mem_size > SZ_512M) {
		ion_mem.size = ion_cma_1g;
		ion_mem.start = mem_start + mem_size - ion_mem.size;
		early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
       }
#endif
       early_printk("ion_cma_1g reserve: %dm!\n", (int)(ion_cma_1g>>20));
       return 0;
}
early_param("ion_cma_1g", early_ion_cma_1g);
#endif

#ifndef CONFIG_OF
#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase        = (void __iomem *)(SUNXI_UART0_VBASE),
		.mapbase        = (resource_size_t)SUNXI_UART0_PBASE,
		.irq            = SUNXI_IRQ_UART0,
		.flags          = UPF_BOOT_AUTOCONF|UPF_IOREMAP,
		.iotype         = UPIO_MEM32,
		.regshift       = 2,
		.uartclk        = 24000000,
	}, {
		.flags          = 0,
	}
 };

static struct platform_device serial_dev = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &serial_platform_data[0],
	}
};
#endif

#if defined(CONFIG_CPU_HAS_PMU)
/* cpu performance support */
static struct resource sunxi_pmu_res = {
#if defined(CONFIG_ARCH_SUN8I) && defined(CONFIG_EVB_PLATFORM)
	.start  = SUNXI_IRQ_PMU0,
	.end    = SUNXI_IRQ_PMU3,
#else
	.start  = SUNXI_IRQ_PMU,
	.end    = SUNXI_IRQ_PMU,
#endif
	.flags  = IORESOURCE_IRQ,
};

static struct platform_device sunxi_pmu_dev = {
	.name   = "arm-pmu",
	.id     = ARM_PMU_DEVICE_CPU,
	.num_resources = 1,
	.resource = &sunxi_pmu_res,
};
#endif
#if defined(CONFIG_KEYBOARD_GPIO_POLLED)
/* Labels according to the SmartQ manual */
static struct gpio_keys_button gpio_buttons[] = {
	{
		.gpio			= GPIOE(11),
		.code			= KEY_UP,
		.desc			= "BlueTooth",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type           = EV_KEY,
	},
	{
		.gpio			= GPIOE(10),
		.code			= KEY_DOWN,
		.desc			= "WiFi",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type           = EV_KEY,
	},
	{
		.gpio			= GPIOE(9),
		.code			= KEY_VOLUMEUP,
		.desc			= "VolumeUp",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type           = EV_KEY,
	},
	{
		.gpio			= GPIOE(8),
		.code			= KEY_VOLUMEDOWN,
		.desc			= "VolumeDown",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type           = EV_KEY,
	},
};

static struct gpio_keys_platform_data gpio_buttons_data  = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
    .poll_interval = 100,
};

static struct platform_device gpio_buttons_device  = {
	.name		= "gpio-keys-polled",
	.id		= 0,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &gpio_buttons_data,
	}
};
#endif

static struct platform_device *sunxi_dev[] __initdata = {
#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
	&serial_dev,
#endif
#if defined(CONFIG_CPU_HAS_PMU)
	&sunxi_pmu_dev,
#endif
#if defined(CONFIG_KEYBOARD_GPIO_POLLED)
	&gpio_buttons_device,
#endif
};
#endif

static void sun8i_restart(char mode, const char *cmd)
{
#ifndef CONFIG_ARCH_SUN8IW8
	writel(0, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_IRQ_EN_REG));
	writel(0x01, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_CFG_REG));
	writel((0x1 << 5) | 0x01, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_MODE_REG));
#else
	writel(0x0, (void __iomem *)(SUNXI_TIMER_VBASE + 0xA0));
	writel(1, (void __iomem *)(SUNXI_TIMER_VBASE + 0xB4));
	writel((0x3 << 4), (void __iomem *)(SUNXI_TIMER_VBASE + 0xB8));
	writel(0x01, (void __iomem *)(SUNXI_TIMER_VBASE + 0xB8));
#endif
	while(1);
}

static struct map_desc sunxi_io_desc[] __initdata = {
	{
		(u32)SUNXI_IO_VBASE,      __phys_to_pfn(SUNXI_IO_PBASE),
		SUNXI_IO_SIZE, MT_DEVICE
	},
	{
		(u32)SUNXI_SRAM_A1_VBASE, __phys_to_pfn(SUNXI_SRAM_A1_PBASE), 
		SUNXI_SRAM_A1_SIZE, MT_MEMORY_ITCM
	},
	{
		(u32)SUNXI_SRAM_A2_VBASE, __phys_to_pfn(SUNXI_SRAM_A2_PBASE),
		SUNXI_SRAM_A2_SIZE, MT_DEVICE_NONSHARED
	},
#ifdef CONFIG_ARCH_SUN8IW3P1
	{
		(u32)SUNXI_SRAM_VE_VBASE, __phys_to_pfn(SUNXI_SRAM_VE_PBASE),
		SUNXI_SRAM_VE_SIZE, MT_DEVICE
	},
#endif
#ifdef CONFIG_SUNXI_HW_READ
	{
		(u32)SUNXI_BROM_VBASE,  __phys_to_pfn(SUNXI_BROM_PBASE),
		SUNXI_BROM_SIZE,    MT_DEVICE
	},
#endif
#if defined(CONFIG_ARCH_SUN8I) && !defined(CONFIG_ARCH_SUN8IW6)
       {(u32)SUNXI_BROM_VBASE,    __phys_to_pfn(SUNXI_BROM_PBASE),    SUNXI_BROM_SIZE,    MT_DEVICE},
#endif
};

static void __init sun8i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
#ifdef CONFIG_EVB_PLATFORM
	struct tag *t;

	for (t = tags; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_MEM && t->u.mem.size) {
			early_printk("[%s]: From boot, get meminfo:\n"
					"\tStart:\t0x%08x\n"
					"\tSize:\t%dMB\n",
					__func__,
					t->u.mem.start,
					t->u.mem.size >> 20);
			mem_start = t->u.mem.start;
			mem_size = t->u.mem.size;
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
			if (mem_size <= SZ_512M)
				ion_mem.size = CONFIG_ION_SUNXI_RESERVE_SIZE_512M << 20;
			else
				ion_mem.size = CONFIG_ION_SUNXI_RESERVE_SIZE << 20;
			ion_mem.start = mem_start + mem_size - ion_mem.size;
#endif
			return;
		}
	}
#endif

	early_printk("[%s] enter\n", __func__);

	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = PLAT_MEM_SIZE;
	meminfo->nr_banks = 1;

	early_printk("nr_banks: %d, bank.start: 0x%08x, bank.size: 0x%08x\n",
			meminfo->nr_banks, meminfo->bank[0].start,
			(unsigned int)meminfo->bank[0].size);
}

void __init sun8i_reserve(void)
{
	/* reserve for sys_config */
	memblock_reserve(SYS_CONFIG_MEMBASE, SYS_CONFIG_MEMSIZE);

	/* reserve for standby */
	memblock_reserve(SUPER_STANDBY_MEM_BASE, SUPER_STANDBY_MEM_SIZE);

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
#ifndef CONFIG_CMA
	memblock_reserve(ion_mem.start, ion_mem.size);
#endif
#endif
}

#ifndef CONFIG_OF
static void __init sun8i_gic_init(void)
{
	gic_init(0, 29, (void __iomem *)SUNXI_GIC_DIST_VBASE, (void __iomem *)SUNXI_GIC_CPU_VBASE);
}
#endif

#if defined(CONFIG_DEBUG_PAGEALLOC) && defined(CONFIG_ARCH_SUN8IW5P1)
/* with CMA, boot log memory may be crashed on booting, so reserve it */
void save_boot_log(void)
{
	u32 width, height, addr, addr_high;
	u32 log_paddr, log_size;

	/* get logo address and size */
	addr = readl(0xf1e60850);
	addr_high = readl(0xf1e60860) & 0xf;
	width = (readl(0xf1e60810) & 0xffff) + 1;
	height = (readl(0xf1e60810) >> 16) + 1;
	addr = (addr >> 3) | (addr_high << 29); /* phy addr */

	log_paddr = addr; /* de and cpu map difference */
	log_size = width * height * 4; /* bytes */

	pr_info("%s: get log with %d, high %d, size 0x%x, pa 0x%x\n", __func__, width, height, log_size, log_paddr);

	memblock_reserve(log_paddr, PAGE_ALIGN(log_size));
}
#endif

void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));

	/* detect sunxi soc ver */
	sunxi_soc_ver_init();

#ifdef CONFIG_DEBUG_PAGEALLOC
	save_boot_log();
#endif
}

static void __init sunxi_dev_init(void)
{
#ifdef CONFIG_OF
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
#else
	platform_add_devices(sunxi_dev, ARRAY_SIZE(sunxi_dev));
#endif
}

extern void __init sunxi_init_clocks(void);
#ifdef CONFIG_ARM_ARCH_TIMER
struct arch_timer sun8i_arch_timer __initdata = {
	.res[0] = {
		.start = 29,
		.end = 29,
		.flags = IORESOURCE_IRQ,
	},
	.res[1] = {
		.start = 30,
		.end = 30,
		.flags = IORESOURCE_IRQ,
	},
};
#endif

extern void sunxi_timer_init(void);
static void __init sun8i_timer_init(void)
{
	sunxi_init_clocks();
#if defined(CONFIG_ARCH_SUN8IW6P1) && ((defined CONFIG_FPGA_V4_PLATFORM) || (defined CONFIG_FPGA_V7_PLATFORM))
	if(!(readl(IO_ADDRESS(SUNXI_TIMESTAMP_CTRL_PBASE)) & 0x01))
		writel(readl(IO_ADDRESS(SUNXI_TIMESTAMP_CTRL_PBASE)) |
				0x01,IO_ADDRESS(SUNXI_TIMESTAMP_CTRL_PBASE));
#endif

#ifdef CONFIG_SUNXI_TIMER
	sunxi_timer_init();
#endif

#ifdef CONFIG_ARM_ARCH_TIMER
	arch_timer_register(&sun8i_arch_timer);
	arch_timer_sched_clock_init();
#endif
}

struct sys_timer sunxi_timer __initdata = {
	.init = sun8i_timer_init,
};

#if defined(CONFIG_SMP) && defined(CONFIG_ARCH_SUN8IW6)
extern bool __init sun8i_smp_init_ops(void);
#endif

void __init sunxi_init_early(void)
{
#ifdef CONFIG_SUNXI_CONSISTENT_DMA_SIZE
	init_consistent_dma_size(CONFIG_SUNXI_CONSISTENT_DMA_SIZE << 20);
#endif
}

MACHINE_START(SUNXI, "sun8i")
	.atag_offset	= 0x100,
	.init_machine	= sunxi_dev_init,
	.init_early     = sunxi_init_early,
	.map_io		= sunxi_map_io,
#ifndef CONFIG_OF
	.init_irq	= sun8i_gic_init,
#endif
	.handle_irq	= gic_handle_irq,
	.restart	= sun8i_restart,
	.timer		= &sunxi_timer,
	.dt_compat	= NULL,
	.reserve	= sun8i_reserve,
	.fixup		= sun8i_fixup,
	.nr_irqs	= NR_IRQS,
#ifdef CONFIG_SMP
	.smp		= smp_ops(sunxi_smp_ops),
#ifdef CONFIG_ARCH_SUN8IW6
	.smp_init	= smp_init_ops(sun8i_smp_init_ops),
#endif
#endif
MACHINE_END
