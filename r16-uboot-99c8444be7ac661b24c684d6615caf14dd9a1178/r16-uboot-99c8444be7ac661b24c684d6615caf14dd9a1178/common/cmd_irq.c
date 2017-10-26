/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <config.h>
#include <command.h>
#include <malloc.h>
#include <pmu.h>
#include <sys_config.h>
#include <asm/arch/intc.h>
#include <asm/arch/timer.h>
#include <smc.h>
#include <securestorage.h>

#ifdef CONFIG_SUNXI_SECURE_SYSTEM
#include <sunxi_efuse.h>
#endif

struct timer_list timer0_t;
struct timer_list timer1_t;
static int timer_test_flag[2];
extern int sprite_led_init(void);
extern int sprite_led_exit(int status);

static  void  timer0_test_func(void *p)
{
	struct timer_list *timer_t;

	timer_t = (struct timer_list *)p;
	debug("timer number = %d\n", timer_t->timer_num);
	printf("this is timer test\n");

	del_timer(timer_t);
	timer_test_flag[0] = 0;

	return;
}

int do_timer_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int  base_count = 1000;

	if(timer_test_flag[0])
	{
		printf("can not test timer 0 now\n");

		return -1;
	}

	if(argc == 2)
	{
		base_count = simple_strtol(argv[1], NULL, 10);
	}
	timer0_t.data = (unsigned long)&timer0_t;
	timer0_t.expires = base_count;
	timer0_t.function = timer0_test_func;

	init_timer(&timer0_t);
	add_timer(&timer0_t);
	timer_test_flag[0] = 1;

	return 0;
}


U_BOOT_CMD(
	timer_test, 2, 0, do_timer_test,
	"do a timer and int test",
	"[delay time]"
);

static  void  timer1_test_func(void *p)
{
	struct timer_list *timer_t;

	timer_t = (struct timer_list *)p;
	debug("timer number = %d\n", timer_t->timer_num);
	printf("this is timer test\n");

	del_timer(timer_t);
	timer_test_flag[1] = 0;

	return;
}


int do_timer_test1(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int  base_count = 1000;

	if(timer_test_flag[1])
	{
		printf("can not test timer 1 now\n");

		return -1;
	}

	if(argc == 2)
	{
		base_count = simple_strtol(argv[1], NULL, 10);
	}
	timer1_t.data = (unsigned long)&timer1_t;
	timer1_t.expires = base_count;
	timer1_t.function = timer1_test_func;

	init_timer(&timer1_t);
	add_timer(&timer1_t);

	timer_test_flag[1] = 1;

	return 0;
}


U_BOOT_CMD(
	timer_test1, 2, 0, do_timer_test1,
	"do a timer and int test",
	"[delay time]"
);

#ifndef CONFIG_SUNXI_SPINOR_PLATFORM
int sunxi_usb_dev_register(uint dev_name);
void sunxi_usb_main_loop(int mode);
int sunxi_card_sprite_main(int workmode, char *name);


DECLARE_GLOBAL_DATA_PTR;


int do_sprite_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#if defined(CONFIG_STORAGE_MEDIA_CARD) || \
    defined(CONFIG_SUNXI_SPRITE_SYSRECOVERY)
	int ret;
#endif

	sunxi_flash_handle_init();

	printf("work mode=0x%x\n", uboot_spare_head.boot_data.work_mode);
	if(uboot_spare_head.boot_data.work_mode == WORK_MODE_USB_PRODUCT)
	{
		printf("run usb efex\n");
		memset((void *)0x17e00, 0x0, 4);
		memset((void *)0x17e04, 0xcc, 252);
		if(sunxi_usb_dev_register(2))
		{
			printf("sunxi usb test: invalid usb device\n");
		}
		sunxi_usb_main_loop(2500);
	}
#ifdef CONFIG_STORAGE_MEDIA_CARD
	else if(uboot_spare_head.boot_data.work_mode == WORK_MODE_CARD_PRODUCT)
	{
		printf("run card sprite\n");
		sprite_led_init();
		ret = sunxi_card_sprite_main(0, NULL);
		sprite_led_exit(ret);
		return ret;
	}
#endif
	else if(uboot_spare_head.boot_data.work_mode == WORK_MODE_USB_DEBUG)
	{
		unsigned int val;

		printf("run usb debug\n");
		if(sunxi_usb_dev_register(2))
		{
			printf("sunxi usb test: invalid usb device\n");
		}

		asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
		val &= ~(1<<2);
		asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR" : : "r" (val) : "cc");

		sunxi_usb_main_loop(0);
	}
#ifdef CONFIG_SUNXI_SPRITE_SYSRECOVERY
	else if(uboot_spare_head.boot_data.work_mode == WORK_MODE_SPRITE_RECOVERY)
	{
		printf("run sprite recovery\n");
		sprite_led_init();
		ret = sprite_form_sysrecovery();
		sprite_led_exit(ret);
		return ret;
	}
#endif
	else
	{
		printf("others\n");
	}

	return 0;
}

U_BOOT_CMD(
	sprite_test, 2, 0, do_sprite_test,
	"do a sprite test",
	"NULL"
);



int do_fastboot_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("run usb fastboot\n");
	if(sunxi_usb_dev_register(3))
	{
		printf("sunxi usb test: invalid usb device\n");
	}
	sunxi_usb_main_loop(0);

	return 0;
}


U_BOOT_CMD(
	fastboot_test, 2, 0, do_fastboot_test,
	"do a sprite test",
	"NULL"
);

int do_mass_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("run usb mass\n");
	if(sunxi_usb_dev_register(1))
	{
		printf("sunxi usb test: invalid usb device\n");
	}
	sunxi_usb_main_loop(0);

	return 0;
}


U_BOOT_CMD(
	mass_test, 2, 0, do_mass_test,
	"do a usb mass test",
	"NULL"
);

int do_efex_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("run usb efex_test\n");
	if(sunxi_usb_dev_register(5))
	{
		printf("sunxi usb test: invalid usb device\n");
	}
	sunxi_usb_main_loop(2500);

	return 0;
}
U_BOOT_CMD(
	efex_test, 2, 0, do_efex_test,
	"do a usb efex test",
	"NULL"
);
#endif

int do_memcpy_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint  size = 64 * 1024 * 1024;

	tick_printf("memcpy test start\n");
	memcpy((void *)MEMCPY_TEST_DST, (void *)MEMCPY_TEST_SRC, size);
	tick_printf("memcpy test end\n");

	return 0;
}


U_BOOT_CMD(
	memcpy_test, 2, 0, do_memcpy_test,
	"do a memcpy test",
	"NULL"
);

int do_delay_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	tick_printf("delay 1000ms\n");
	__msdelay(1000);
	tick_printf("delay test end\n");

	tick_printf("delay 1000ms\n");
	__usdelay(1000 * 1000);
	tick_printf("delay test end\n");

	return 0;
}


U_BOOT_CMD(
	delay_test, 2, 0, do_delay_test,
	"do a delay test",
	"NULL"
);

int do_sysconfig(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *main_key, *sub_key;
	int  buffer[64];
	int  ret;

	memset(buffer, 0, 256);
	if(argc == 2)
	{
		char sub_name[32];
		uint main_hd;
		int  index;

		main_key = argv[1];

		main_hd = script_parser_fetch_subkey_start(main_key);
		if(!main_hd)
		{
			printf("the [%s] cant be found\n", main_key);

			return -1;
		}
		printf("[%s]:\n", main_key);
		index = 0;
		do
		{
			memset(sub_name, 0, 32);
			ret = script_parser_fetch_subkey_next(main_hd, sub_name, buffer, &index);
			if(ret < 0)
			{
				printf("find [%s] to end\n", main_key);

				break;
			}
			printf("--------- %s = 0x%x\n", sub_name, buffer[0]);
		}
		while(1);

		return 0;
	}
	else if(argc == 3)
	{
		main_key = argv[1];
		sub_key = argv[2];

		ret = script_parser_fetch(main_key, sub_key, buffer, 64);
		if(ret < 0)
		{
			printf("the [%s]:%s cant be found\n", main_key, sub_key);

			return -1;
		}
		printf("[%s]:%s = %x\n", main_key, sub_key, buffer[0]);

		return 0;
	}

	printf("the sys_config args is invalid\n");

	return -1;
}

U_BOOT_CMD(
	sys_config, 3, 0, do_sysconfig,
	"show the sys config value",
	"sys_config [main_key]\n"
	"sys_config [main_key] [sub_key]"
);


int do_power_probe(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *power_name, *pmu_type;
	int  power_value;

	if(argc == 3)
	{
		pmu_type   = argv[1];
		power_name = argv[2];

		power_value = axp_probe_supply_status_byname(pmu_type, power_name);
		if(power_value > 0)
		{
			printf("    %s %s output voltage = %d\n", pmu_type, power_name, power_value);
		}
		else
		{
			printf("    probe %s %s output voltage failed\n", pmu_type, power_name);
		}

		return 0;
	}

	cmd_usage(cmdtp);

	return 0;
}

U_BOOT_CMD(
	power_probe, 3, 0, do_power_probe,
	"probe the axp output",
	"usage: power_probe pmu_type axp_name"
);


#ifdef CONFIG_BOOT_A15

#include <asm/arch/cpu_switch.h>

extern void sunxi_set_rtc_flag(u8 flag);

int do_reboot_boot_cpu(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if(argc == 1)
	{
		sunxi_board_restart(0);
	}
	else if(argc == 2)
	{
		char *boot_cpu = argv[1];

		if(!strcmp(boot_cpu, "a7"))
		{
			sunxi_set_rtc_flag(BOOT_A7_FLAG);
			sunxi_board_restart(0);
		}
		else if(!strcmp(boot_cpu, "a15"))
		{
			sunxi_set_rtc_flag(BOOT_A15_FLAG);
			sunxi_board_restart(0);
		}
	}

	cmd_usage(cmdtp);

	return -1;
}

U_BOOT_CMD(
	reboot, 3, 0, do_reboot_boot_cpu,
	"reboot by a7 or a15",
	"usage: reboot [a7] or [a15]"
);
#endif

#ifdef CONFIG_SUNXI_SECURE_SYSTEM
int do_efuse_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *efuse_name;
	char buffer[32];
	int  ret;

	if(argc != 2)
	{
		printf("the efuse item name is empty\n");

		return -1;
	}
	efuse_name = argv[1];
	printf("try to read %s\n", efuse_name);
	memset(buffer, 0, 32);

	ret = sunxi_efuse_read(efuse_name, buffer);
	if(ret)
	{
		printf("read efuse key [%s] failed\n", efuse_name);
	}
	else
	{
		printf("read efuse key [%s] successed\n", efuse_name);
		sunxi_dump(buffer, 32);
	}

	return 0;
}

U_BOOT_CMD(
	efuse_read, 3, 0, do_efuse_read,
	"read efuse key",
	"usage: efuse_read efusename"
);

int do_huk_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char buffer[32];
	int  ret;

	printf("try to create huk\n");
	memset(buffer, 0, 32);
	ret = smc_create_huk(buffer, 32);
	if(ret < 0)
	{
		printf("create huk failed\n");
	}
	else if(ret == 1)
	{
		printf("create huk has been burned\n");

		sunxi_dump(buffer, 32);
	}
	else
	{
		printf("create huk successed\n");

		sunxi_dump(buffer, 32);
	}

	return 0;
}

U_BOOT_CMD(
	huk_test, 3, 0, do_huk_test,
	"create a huk as a test",
	"usage: huk_test"
);

int do_hdcp_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int  ret, data_len;
	char buffer[4096];

	printf("try to test hdcp\n");
	memset(buffer, 0, 4096);
	ret = sunxi_secure_storage_init();
	if(ret)
	{
		printf("sunxi init secure storage failed\n");
	}
	else
	{
		ret = sunxi_secure_storage_read("hdcpkey", buffer, 4096, &data_len);
		if(ret)
		{
			printf("probe hdcp key failed\n");
		}
		else
		{
			printf("source data in secure storage\n");
			sunxi_dump(buffer, (data_len+3)&(~0x3));
			ret = smc_aes_bssk_decrypt_to_keysram(buffer, data_len);
			if(ret)
			{
				printf("push hdcp key failed\n");
			}
			else
			{
				printf("push hdcp key ok\n");
			}
		}
	}

	return 0;
}

U_BOOT_CMD(
	hdcp_test, 3, 0, do_hdcp_test,
	"test the hdcp key",
	"usage: hdcp_test"
);

#endif


#ifdef CONFIG_AES_TEST

#define AES_RSSK_TEST_SOURCE_DATA_LEN  (512)
int do_hdcp_rssk_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8  source[AES_RSSK_TEST_SOURCE_DATA_LEN];
	u8  dest[AES_RSSK_TEST_SOURCE_DATA_LEN];
	u8  cmp[AES_RSSK_TEST_SOURCE_DATA_LEN];
	u8  md5_buf_keysram[16], md5_buf_dram[16];
	u32 data_len = 288, i;
	int ret;

	for(i=0;i<AES_RSSK_TEST_SOURCE_DATA_LEN;i++)
	{
		source[i] = (i&0xff) + 0x80;
	}
	memset(dest, 0, AES_RSSK_TEST_SOURCE_DATA_LEN);
	memset(cmp, 0, AES_RSSK_TEST_SOURCE_DATA_LEN);

	printf("aes rssk test: source data:\n");
	sunxi_dump(source, data_len);
	ret = sunxi_aes_decrypt_rssk_hdcp_to_keysram(source, data_len);
	if(ret)
	{
		printf("%s:fail to calculate hdcp key to keysram\n",__func__);

		return -1;
	}
	ret = sunxi_md5_keysram_calcute((void *)md5_buf_keysram, 16);
	if(ret)
	{
		printf("%s: fail to calculate md5 in keysram\n",__func__);

		return -1;
	}
	printf("aes rssk test: keysram buf md5:\n");
	sunxi_dump(md5_buf_keysram, 16);
	ret = sunxi_aes_decrypt_rssk_hdcp_to_dram(source, data_len, dest);
	if(ret)
	{
		printf("%s: fail to calculate hdcp key for source data\n",__func__ );

		return -1;
	}
	printf("aes rssk test: decrypt data:\n");
	sunxi_dump(dest, data_len);

	ret = sunxi_md5_dram_calcute((void *)dest, data_len, (void *)md5_buf_dram, 16);
	if(ret)
	{
		printf("%s: fail to calculate md5 for source data\n",__func__ );

		return -1;
	}
	printf("aes rssk test: dram buf md5:\n");
	sunxi_dump(md5_buf_dram, 16);

	ret = sunxi_aes_encrypt_rssk_hdcp_to_dram(cmp, dest, data_len);
	if(ret)
	{
		printf("%s: fail to encrypt hdcp rssk for source data\n",__func__ );

		return -1;
	}
	printf("aes rssk test: encrypt data:\n");
	sunxi_dump(cmp, data_len);

	for(i=0;i<16;i++)
	{
		if(md5_buf_keysram[i] != md5_buf_dram[i])
		{
			printf("%s: compare the md5 between keysram and source data failed\n", __func__);

			printf("md5_buf_keysram:\n");
			sunxi_dump(md5_buf_keysram, 16);

			printf("md5_buf_source:\n");
			sunxi_dump(md5_buf_dram, 16);

			return -1;
		}
		else
		{
			printf("%s: compare the md5 between keysram and source data successed\n", __func__);
		}
	}

	return 0;
}
U_BOOT_CMD(
	hdcp_rssk_test, CONFIG_SYS_MAXARGS,1, do_hdcp_rssk_test,
	"test the aes rssk to keysram method",
	"usage: hdcp_rssk_test\n"
	"now there is no any parameters\n"
);


#define AES_TEST_SOURCE_DATA_LEN   (512)
#define AES_TEST_BLOCK_DATA_LEN    (512)
int do_aes_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8  source[AES_TEST_SOURCE_DATA_LEN];
	u8  dest[AES_TEST_SOURCE_DATA_LEN];
	u8  cmp[AES_TEST_SOURCE_DATA_LEN];
	u8  key_buf[256], iv_buf[128];
	u32 key_type=2, aes_mode=0;
	int ret, i;

	if(argc == 1)
	{
		;
	}
	else if(argc == 2)
	{
		key_type = simple_strtol(argv[1], NULL, 10);
	}
	else if(argc == 3)
	{
		key_type = simple_strtol(argv[1], NULL, 10);
		aes_mode = simple_strtol(argv[2], NULL, 10);
	}
	else
	{
		printf("Too much parameters\n");

		goto return_err;
	}

	if(key_type == 0)
	{
		printf("AES KEY: 128bit\n");
	}
	else if(key_type == 1)
	{
		printf("AES KEY: 192bit\n");
	}
	else if(key_type == 2)
	{
		printf("AES KEY: 256bit\n");
	}
	else
	{
		printf("the AES KEY TYPE is invalid\n");

		goto return_err;
	}

	if(aes_mode == 0)
	{
		printf("AES MODE: ECB\n");
	}
	else if(aes_mode == 1)
	{
		printf("AES MODE: CBC\n");
	}
	else
	{
		printf("the AES MODE is invalid\n");

		goto return_err;
	}

	for(i=0;i<AES_TEST_SOURCE_DATA_LEN;i++)
	{
		source[i] = (i&0xff) + 0x80;
	}
	for(i=0;i<256;i++)
	{
		key_buf[i] = (i&0xff) + 0x5A;
	}

	printf("aes test: source data:\n");
	sunxi_dump(source, AES_TEST_SOURCE_DATA_LEN);

	printf("try to encrypt\n");
	memset(dest , 0, AES_TEST_SOURCE_DATA_LEN);

	if(aes_mode == SS_AES_MODE_ECB)
	{
		ret = TNHALCryptoAESEcb(source, AES_TEST_SOURCE_DATA_LEN, dest, key_buf, key_type, SS_DIR_ENCRYPT);
	}
	else
	{
		memset(iv_buf, 0x5A, 128);
		ret = TNHALCryptoAESCbc(source, AES_TEST_SOURCE_DATA_LEN, dest, key_buf, key_type, iv_buf, SS_DIR_ENCRYPT);
	}
	if(ret)
	{
		printf("%s:fail to encrypt\n",__func__);

		goto return_err;
	}
	printf("the encrypt value:\n");
	sunxi_dump(dest, AES_TEST_SOURCE_DATA_LEN);

	printf("try to decrypt\n");
	memset(cmp , 0, AES_TEST_SOURCE_DATA_LEN);

	if(aes_mode == SS_AES_MODE_ECB)
	{
		ret = TNHALCryptoAESEcb(dest, AES_TEST_SOURCE_DATA_LEN, cmp, key_buf, key_type, SS_DIR_DECRYPT);
	}
	else
	{
		memset(iv_buf, 0x5A, 128);
		ret = TNHALCryptoAESCbc(dest, AES_TEST_SOURCE_DATA_LEN, cmp, key_buf, key_type, iv_buf, SS_DIR_DECRYPT);
	}

	if(ret)
	{
		printf("%s:fail to decrypt\n",__func__);

		goto return_err;
	}
	printf("the decrypt value:\n");
	sunxi_dump(cmp, AES_TEST_SOURCE_DATA_LEN);

	for(i=0;i<AES_TEST_SOURCE_DATA_LEN;i++)
	{
		if(source[i] != cmp[i])
		{
			printf("the aes fail: source[%d]:%02x is not equal to cmp[%d]:%02x\n", i, source[i], i, cmp[i]);

			goto return_err;
		}
	}

	printf("aes successed\n");

	return 0;

return_err:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	aes_test, CONFIG_SYS_MAXARGS,1, do_aes_test,
	"test the aes encrypt and decrypt",
	"usage: aes_test [key_type] [aes_mode]\n"
	"key type: 0        , 128BIT\n"
	"          1        , 192BIT\n"
	"          2 default, 256BIT\n"
	"aes mode: 0 default, ECB\n"
	"        : 1        , CBC\n"
);

#define SHAX_TEST_BLOCK_DATA_LEN    (512)
static u8 *__shax_prepare(int argc, char * const argv[], u32 *len)
{
	u8 *data_buf;
	u32 data_len;

	if(argc == 1)
	{
		data_len = 2048;
	}
	else if(argc == 2)
	{
		data_len = simple_strtol(argv[1], NULL, 10);
	}
	else
	{
		printf("Too much parameters\n");

		return NULL;
	}
	data_buf = (u8 *)malloc(data_len + 1);
	if(data_buf == NULL)
	{
		printf("the input data length is invalid\n");

		return NULL;
	}
	memset(data_buf, 'A', data_len);

	printf("the data bytes: %d\n", data_len);
	data_buf[data_len] = '\0';
	//printf("the source data:\n");
	//sunxi_dump(data_buf, data_len);
	*len = data_len;

	return data_buf;

}
int do_md5_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8  *source, *tmp_src;
	u8  md5_buf[64];
	u32 data_len, tmp_data_len;
	int ret;

	source = __shax_prepare(argc, argv, &data_len);
	if(source == NULL)
	{
		printf("The source addr is invalid\n");

		return cmd_usage(cmdtp);
	}
	tmp_src = source;
	tmp_data_len = data_len;
	while(tmp_data_len > SHAX_TEST_BLOCK_DATA_LEN)
	{
		ret = TNHALCryptoMD5(tmp_src, SHAX_TEST_BLOCK_DATA_LEN, 0, md5_buf);
		if(ret)
		{
			printf("%s: fail to calculate md5 for source data\n",__func__ );

			goto __md5_test_err;
		}
		tmp_src  += SHAX_TEST_BLOCK_DATA_LEN;
		tmp_data_len -= SHAX_TEST_BLOCK_DATA_LEN;
	}
	ret = TNHALCryptoMD5(tmp_src, tmp_data_len, 1, md5_buf);
	if(ret)
	{
		printf("%s: fail to calculate md5 for source data\n",__func__ );

		goto __md5_test_err;
	}

	printf("md5 test: dram buf md5:\n");
	sunxi_dump(md5_buf, 16);

	free(source);

	return 0;
__md5_test_err:
	free(source);

	return cmd_usage(cmdtp);

}

U_BOOT_CMD(
	md5, CONFIG_SYS_MAXARGS,1, do_md5_test,
	"test the md5",
	"usage: md [data_len]\n"
	"if null, default value is 512 "
);

int do_sha1_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8  *source, *tmp_src;
	u8  sha1_buf[64];
	u32 data_len, tmp_data_len;
	int ret;

	source = __shax_prepare(argc, argv, &data_len);
	if(source == NULL)
	{
		printf("The source addr is invalid\n");

		return cmd_usage(cmdtp);
	}
	tmp_src = source;
	tmp_data_len = data_len;
	while(tmp_data_len > SHAX_TEST_BLOCK_DATA_LEN)
	{
		ret = TNHALCryptoSHA1(tmp_src, SHAX_TEST_BLOCK_DATA_LEN, 0, sha1_buf);
		if(ret)
		{
			printf("%s: fail to calculate sha1 for source data\n",__func__ );

			goto __sha1_test_err;
		}
		tmp_src  += SHAX_TEST_BLOCK_DATA_LEN;
		tmp_data_len -= SHAX_TEST_BLOCK_DATA_LEN;
	}
	ret = TNHALCryptoSHA1(tmp_src, tmp_data_len, 1, sha1_buf);
	if(ret)
	{
		printf("%s: fail to calculate sha1 for source data\n",__func__ );

		goto __sha1_test_err;
	}

	printf("sha1 test: dram buf sha1:\n");
	sunxi_dump(sha1_buf, 20);
	free(source);

	return 0;
__sha1_test_err:
	free(source);

	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	sha1, CONFIG_SYS_MAXARGS,1, do_sha1_test,
	"test the sha1",
	"usage: sha1 [data_len]\n"
	"if null, default value is 512 "
);

int do_sha256_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8  *source, *tmp_src;
	u8  sha256_buf[64];
	u32 data_len, tmp_data_len;
	int ret;

	source = __shax_prepare(argc, argv, &data_len);
	if(source == NULL)
	{
		printf("The source addr is invalid\n");

		return cmd_usage(cmdtp);
	}
	tmp_src = source;
	tmp_data_len = data_len;
	while(tmp_data_len > SHAX_TEST_BLOCK_DATA_LEN)
	{
		ret = TNHALCryptoSHA256(tmp_src, SHAX_TEST_BLOCK_DATA_LEN, 0, sha256_buf);
		if(ret)
		{
			printf("%s: fail to calculate sha256 for source data\n",__func__ );

			goto __sha256_test_err;
		}
		tmp_src  += SHAX_TEST_BLOCK_DATA_LEN;
		tmp_data_len -= SHAX_TEST_BLOCK_DATA_LEN;
	}
	ret = TNHALCryptoSHA256(tmp_src, tmp_data_len, 1, sha256_buf);
	if(ret)
	{
		printf("%s: fail to calculate sha256 for source data\n",__func__ );

		goto __sha256_test_err;
	}
	printf("sha256 test: dram buf sha256:\n");
	sunxi_dump(sha256_buf, 32);
	free(source);

	return 0;
__sha256_test_err:
	free(source);

	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	sha256, CONFIG_SYS_MAXARGS,1, do_sha256_test,
	"test the sha256",
	"usage: sha256 [data_len]\n"
	"if null, default value is 512 "
);

int do_sha512_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8  *source, *tmp_src;
	u8  sha512_buf[64];
	u32 data_len, tmp_data_len;
	int ret;

	source = __shax_prepare(argc, argv, &data_len);
	if(source == NULL)
	{
		printf("The source addr is invalid\n");

		return cmd_usage(cmdtp);
	}
	tmp_src = source;
	tmp_data_len = data_len;
	while(tmp_data_len > SHAX_TEST_BLOCK_DATA_LEN)
	{
		ret = TNHALCryptoSHA512(tmp_src, SHAX_TEST_BLOCK_DATA_LEN, 0, sha512_buf);
		if(ret)
		{
			printf("%s: fail to calculate sha512 for source data\n",__func__ );

			goto __sha512_test_err;
		}
		tmp_src      += SHAX_TEST_BLOCK_DATA_LEN;
		tmp_data_len -= SHAX_TEST_BLOCK_DATA_LEN;
	}
	ret = TNHALCryptoSHA512(tmp_src, tmp_data_len, 1, sha512_buf);
	if(ret)
	{
		printf("%s: fail to calculate sha512 for source data\n",__func__ );

		goto __sha512_test_err;
	}

	printf("sha512 test: dram buf sha512:\n");
	sunxi_dump(sha512_buf, 64);
	free(source);

	return 0;
__sha512_test_err:
	free(source);

	return cmd_usage(cmdtp);
}

U_BOOT_CMD(
	sha512, CONFIG_SYS_MAXARGS,1, do_sha512_test,
	"test the sha512",
	"usage: sha512 [data_len]\n"
	"if null, default value is 512 "
);
#endif

