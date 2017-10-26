/*
 * codec apa3165 driver
 *
 * Copyright(c) 2015-2018 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: huangxin <huangxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/jack.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>

#define I2C_CHANNEL_NUM 0
#define AUX_SINGLE_TIME 500
/*
* 30min *60s * 1000ms/500ms = 3600
*/
#define TIMER_30M_COUNT	  3600

/*
* 10min * 60s *1000ms/500ms = 1200
*/
#define TIMER_10M_COUNT	  1200

static int shutdown_count  = 0;
static int aux_nosignal_count  = 0;

static bool AUX_SIGNAL_SHUTDOWN = false;
static bool AUX_JACK_DETECT = false;
static bool AUX_SIGNAL_DETECT = false;

static struct i2c_client *apa3165_client = NULL;
static struct snd_soc_codec *g_codec = NULL;
/*for pa gpio ctrl*/
static script_item_u pa_item;
static script_item_u reset_item;
static script_item_u aux_signal_item;
static script_item_u chip_en;
static script_item_u apa3165_power0;
static script_item_u apa3165_power1;

static bool apa3165_linein_en = false;
static bool apa3165_playing_en = false;
struct timer_list aux_signal_timer;
static bool aux_signal = false;
static int apa3165_used 		= 0;
static int aux_signal_time = 600;
struct apa3165_priv {
	int jack_status;
	struct i2c_client *i2c;
	struct snd_soc_codec *codec;
	struct work_struct linein_jack;
	struct snd_soc_jack jack;
};

//PWM MUX register (0x25) default setting
unsigned char mux[] = {
	//MSB to LSB
	0x01, 0x02, 0x13, 0x45,
};

//ch1_bq[0] register (0x29) parameter setting
unsigned char ch1_bq0[] = {
	//MSB to LSB
	0x00, 0x80, 0x00, 0x00,	//byte20~byte17
	0x00, 0x00, 0x00, 0x00,	//byte16~byte13
	0x00, 0x00, 0x00, 0x00,	//byte12~byte9
	0x00, 0x00, 0x00, 0x00,	//byte8~byte5
	0x00, 0x00, 0x00, 0x00,	//byte4~byte1
};

//ch1_bq[1] register (0x2A) parameter setting
unsigned char ch1_bq1[] = {
	//MSB to LSB
	0x00, 0x80, 0x00, 0x00,	//byte20~byte17
	0x00, 0x00, 0x00, 0x00,	//byte16~byte13
	0x00, 0x00, 0x00, 0x00,	//byte12~byte9
	0x00, 0x00, 0x00, 0x00,	//byte8~byte5
	0x00, 0x00, 0x00, 0x00,	//byte4~byte1
};

//DRC function, default setting is diable
unsigned char drc_ctrl[] = {
	0x00, 0x00, 0x00, 0x00,
	//write 0x00, 0x00, 0x00, 0x01 can enable DRC fucntion
};

//DRC atteck time setting
unsigned char drc_att[] = {
	0x00, 0x03, 0x2D, 0x64,
};

//DRC release time setting
unsigned char drc_rls[] = {
	0x00, 0x02, 0xFF, 0xE4,
};

static int apa3165_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	gpio_set_value(pa_item.gpio.gpio, 1);
	apa3165_playing_en = true;
	return 0;
}

static void apa3165_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (apa3165_linein_en != true) {
		gpio_set_value(pa_item.gpio.gpio, 0);
	}
	apa3165_playing_en = false;
}

static int apa3165_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	snd_soc_write(codec, 0x00, 0x6c); //mclk_freq = 512*fs,fs=44.1k/48k;
	snd_soc_write(codec, 0x05, 0x00); //exit shutdown, normal operation

	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			snd_soc_write(codec, 0x04, 0x03); //I2S data format, 16-bit IIS
			break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			snd_soc_write(codec, 0x04, 0x05); //I2S data format, default 24-bit IIS
			break;
		default:
			dev_err(dai->dev, "unsupport: %u\n", params_format(params));
			return -EINVAL;
	}

	return 0;
}

static int apa3165_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

static int apa3165_set_sysclk(struct snd_soc_dai *dai,
			     int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int apa3165_set_clkdiv(struct snd_soc_dai *dai,
			     int div_id, int div)
{
 	return 0;
}

/*
* Identify the jack type as Headset/Headphone/None
*/
static void sunxi_report_jack_type(struct snd_soc_jack *jack, int jack_status)
{
	struct apa3165_priv *apa3165 = container_of(jack, struct apa3165_priv, jack);
	snd_jack_report(apa3165->jack.jack, jack_status);
}

static void aux_signal_events(unsigned long arg)
{
	int ret = 0;
	int reg_val = 0;
	struct apa3165_priv *apa3165 = snd_soc_codec_get_drvdata(g_codec);

	/*
	* ret == 0, means has aux signal;
	* ret == 1, means hasn't aux signal;
	*/
	ret = gpio_get_value(aux_signal_item.gpio.gpio);
	if (!ret) {
		/*aux signal*/
		if (aux_signal == false) {
			aux_nosignal_count 	= 0;
			shutdown_count 		= 0;
			aux_signal 			= true;
			AUX_SIGNAL_DETECT	= true;
			apa3165->jack_status = SND_JACK_LINEIN_SIGNAL;

			gpio_set_value(pa_item.gpio.gpio, 1);
			reg_val = readl((void __iomem *)0xf1c20824);
			reg_val &= 0xffff;
			reg_val |= (0x3333<<16);
			writel(reg_val, (void __iomem *)0xf1c20824);
			sunxi_report_jack_type(&apa3165->jack, apa3165->jack_status);
		}
	} else {
		/*aux signal -> no aux signal*/
		if (aux_signal == true) {
			aux_signal = false;
		}
		/*
		*	no aux signal, aux_nosignal_count++; 10min = 1200 repeat times;
		*/
		if (aux_signal == false) {
			if (aux_nosignal_count >= aux_signal_time*2) {
				if (apa3165_playing_en == false) {
					gpio_set_value(pa_item.gpio.gpio, 0);
				}
				reg_val = readl((void __iomem *)0xf1c20824);
				reg_val &= 0xffff;
				reg_val |= (0x2222<<16);
				writel(reg_val, (void __iomem *)0xf1c20824);
				apa3165->jack_status 	= SND_JACK_LINEIN;
				AUX_SIGNAL_DETECT 		= false;
				sunxi_report_jack_type(&apa3165->jack, apa3165->jack_status);
			}
			aux_nosignal_count++;
		}
		/*
		* no aux signal, and no local voice, shutdown_count++;
		*/
		if ((aux_signal == false)&&(apa3165_playing_en == false)) {
			shutdown_count++;
		}
	}

	if (shutdown_count >= TIMER_30M_COUNT) {
		AUX_SIGNAL_SHUTDOWN = true;
	} else {
		AUX_SIGNAL_SHUTDOWN = false;
	}
	mod_timer(&aux_signal_timer, jiffies + msecs_to_jiffies(AUX_SINGLE_TIME));
}

static void apa3165_linein_work(struct work_struct *work)
{
	struct apa3165_priv *apa3165 = container_of(work, struct apa3165_priv, linein_jack);
    struct snd_soc_codec *codec = apa3165->codec;

	snd_soc_write(codec, 0x00, 0x6c); //mclk_freq = 512*fs,fs=44.1k/48k;
	snd_soc_write(codec, 0x05, 0x00); //exit shutdown, normal operation
	snd_soc_write(codec, 0x04, 0x03); //I2S data format, 16-bit IIS
}

/*
1.plugin linein, enable PA，audio route is:linein->SPK
2.plugout linein, disable PA，audio route is local audio(include wifi audio and bt audio) -> SPK;
*/
void apa3165_linein_play(bool on)
{
	int reg_val = 0;
	struct apa3165_priv *apa3165 = snd_soc_codec_get_drvdata(g_codec);
	schedule_work(&apa3165->linein_jack);

	if (on) {
		apa3165_linein_en = true;
		gpio_set_value(pa_item.gpio.gpio, 1);

		reg_val = readl((void __iomem *)0xf1c20824);
		reg_val &= 0xffff;
		reg_val |= (0x3333<<16);
		writel(reg_val, (void __iomem *)0xf1c20824);

		AUX_JACK_DETECT = true;
		setup_timer(&aux_signal_timer, aux_signal_events, (unsigned long)0);
		mod_timer(&aux_signal_timer, jiffies + HZ);
		apa3165->jack_status = SND_JACK_LINEIN;
	} else {
		apa3165_linein_en = false;
		del_timer(&aux_signal_timer);
		if (apa3165_playing_en != true) {
			gpio_set_value(pa_item.gpio.gpio, 0);
		}
		reg_val = readl((void __iomem *)0xf1c20824);
		reg_val &= 0xffff;
		reg_val |= (0x2222<<16);
		writel(reg_val, (void __iomem *)0xf1c20824);
		AUX_JACK_DETECT 	= false;
		AUX_SIGNAL_DETECT 	= false;
		shutdown_count 		= 0;
		aux_nosignal_count 	= 0;
		aux_signal 	= false;
		apa3165->jack_status = SND_JACK_LINEIN_PLUGOUT;
	}
	sunxi_report_jack_type(&apa3165->jack, apa3165->jack_status);
}
EXPORT_SYMBOL(apa3165_linein_play);

static const struct snd_soc_dai_ops apa3165_dai_ops = {
	.hw_params 	= apa3165_hw_params,
	.set_fmt 	= apa3165_set_fmt,
	.set_sysclk = apa3165_set_sysclk,
	.set_clkdiv = apa3165_set_clkdiv,
	.startup 	= apa3165_startup,
	.shutdown 	= apa3165_shutdown,
};

#define apa3165_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
			SNDRV_PCM_FMTBIT_S24_LE)
#define apa3165_RATES SNDRV_PCM_RATE_8000_96000

static struct snd_soc_dai_driver apa3165_dai0 = {
		.name = "apa3165-pcm0",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = apa3165_RATES,
			.formats = apa3165_FORMATS,
		},
		.ops = &apa3165_dai_ops,
};

static int master_vol = 0;
#ifdef CONFIG_PM
static int apa3165_suspend(struct snd_soc_codec *codec)
{
	master_vol = snd_soc_read(codec, 0x07);

	return 0;
}
 
static int apa3165_resume(struct snd_soc_codec *codec)
{
	int ret = 0;

	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x25, 4,  mux);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x29, 20, ch1_bq0);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x2A, 20, ch1_bq1);

	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x46, 4,  drc_ctrl);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x60, 4,  drc_att);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x61, 4,  drc_rls);
	snd_soc_write(codec, 0x07, master_vol); //master volume

	return 0;
}
#else
#define apa3165_suspend NULL
#define apa3165_resume NULL
#endif
#define MASTER_VOLUME	0x07
#define CHANNEL1_VOLUME	0x08
#define CHANNEL2_VOLUME	0x09

/*
*	max:0x0:24db
*	default:0db:0x30
*	mute:0xff
*/
static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/*name reg shift mask invert*/
	SOC_SINGLE("Master Playback Volume", MASTER_VOLUME, 0x0, 0xff, 0),//0xff is mute,0.5db step
	SOC_SINGLE("CHANNEL1 Playback Volume", CHANNEL1_VOLUME, 0x0, 0xff, 0),//0xff is mute,0.5db step
	SOC_SINGLE("CHANNEL2 Playback Volume", CHANNEL2_VOLUME, 0x0, 0xff, 0),//0xff is mute,0.5db step
};

static int apa3165_probe(struct snd_soc_codec *codec)
{
	struct apa3165_priv *apa3165 = dev_get_drvdata(codec->dev);
	s32 ret = 0;

	ret = snd_soc_codec_set_cache_io(codec, 8, 8, CONFIG_REGMAP_I2C);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}
	apa3165->codec = codec;
	g_codec = codec;
	snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));

	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x25, 4,  mux);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x29, 20, ch1_bq0); 
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x2A, 20, ch1_bq1); 

	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x46, 4,  drc_ctrl);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x60, 4,  drc_att);
	ret = i2c_smbus_write_i2c_block_data(apa3165_client, 0x61, 4,  drc_rls);
	snd_soc_write(codec, 0x07, 0x30); //master volume set to 0dB

	INIT_WORK(&apa3165->linein_jack, apa3165_linein_work);

	ret = snd_soc_jack_new(apa3165->codec, "sunxi linein Jack",
			       SND_JACK_LINEIN_SIGNAL | SND_JACK_LINEIN | SND_JACK_LINEIN_PLUGOUT,
			       &apa3165->jack);
	if (ret) {
		pr_err("%s,l:%d,jack creation failed\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}

static int apa3165_remove(struct snd_soc_codec *codec)
{
	struct apa3165_priv *codecapa3165 = snd_soc_codec_get_drvdata(codec);
 
	kfree(codecapa3165);
 
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_apa3165 = {
	.probe 				= apa3165_probe,
	.remove 			= apa3165_remove,
	.suspend 			= apa3165_suspend,
	.resume 			= apa3165_resume,
	.ignore_pmdown_time = 1,
};

/*****************************************************/
static int __devinit apa3165_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *i2c_id)
{
	int ret = 0;
	int req_status = 0;
	struct apa3165_priv *apa3165;
	script_item_value_type_e  type;

	apa3165_client = i2c;

	apa3165 = devm_kzalloc(&i2c->dev, sizeof(struct apa3165_priv),
			      GFP_KERNEL);
	if (apa3165 == NULL) {
		dev_err(&i2c->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	} else {
		dev_set_drvdata(&i2c->dev, apa3165);
	}
	apa3165->i2c = i2c;

	/*set power*/
	type = script_get_item("apa3165", "chip_en", &chip_en);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[ audio ] err:try to get chip_en failed!\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(chip_en.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("request gpio failed!\n");
	}
	gpio_direction_output(chip_en.gpio.gpio, 1);
	gpio_set_value(chip_en.gpio.gpio, 1);

	 /*set power*/
	type = script_get_item("apa3165", "apa3165_power0", &apa3165_power0);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[ audio ] err:try to get apa3165_power0 failed!\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(apa3165_power0.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("request gpio failed!\n");
	}
	gpio_direction_output(apa3165_power0.gpio.gpio, 1);
	gpio_set_value(apa3165_power0.gpio.gpio, 1);

	/*set power*/
	type = script_get_item("apa3165", "apa3165_power1", &apa3165_power1);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[ audio ] err:try to get apa3165_power1 failed!\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(apa3165_power1.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("request gpio failed!\n");
	}
	gpio_direction_output(apa3165_power1.gpio.gpio, 1);
	gpio_set_value(apa3165_power1.gpio.gpio, 1);

 	/*get the default pa ctl(close)*/
	type = script_get_item("apa3165", "amp_pa_ctrl", &pa_item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[ audio ] err:try to get audio_pa_ctrl failed!\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(pa_item.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("request gpio failed!\n");
	}
	gpio_direction_output(pa_item.gpio.gpio, 1);

	gpio_set_value(pa_item.gpio.gpio, 0);

	/* get config of apa3165 codec reset gpio pin*/
	type = script_get_item("apa3165", "apa3165_reset", &reset_item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("cx_codec apa3165_reset script_get_item return type err\n");
	} else {
		/*request apa3165_reset codec reset control gpio*/
		ret = gpio_request(reset_item.gpio.gpio, NULL);
		if (0 != ret) {
			pr_err("request apa3165_reset gpio failed!\n");
		}else {
			/*
			* config gpio as output.
			*/
			gpio_direction_output(reset_item.gpio.gpio, 1);
			gpio_set_value(reset_item.gpio.gpio, 1);
			// reset the apa3165_reset codec
			gpio_set_value(reset_item.gpio.gpio, 0);
			msleep(20);
			gpio_set_value(reset_item.gpio.gpio, 1);
		}
	}

	type = script_get_item("apa3165", "aux_signal", &aux_signal_item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("[ audio ] err:try to get aux_signal failed!\n");
		return -EFAULT;
	}
	/*request aux_signal_item gpio*/
	req_status = gpio_request((aux_signal_item.gpio.gpio), NULL);
	if (0 != req_status) {
		printk("request aux_signal_item gpio failed!\n");
	}
	gpio_direction_input(aux_signal_item.gpio.gpio);

	if (i2c_id->driver_data == 0) {
		ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_apa3165, &apa3165_dai0, 1);
	}
	else {
		pr_err("The wrong i2c_id number :%ld\n",i2c_id->driver_data);
	}
 
	return ret;
}

static __devexit int apa3165_i2c_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);

	return 0;
}

static struct i2c_board_info apa3165_i2c_board_info[] = {
	{I2C_BOARD_INFO("apa3165_0", 0x1a),	},
};
 
static const struct i2c_device_id apa3165_i2c_id[] = {
	{ "apa3165_0", 0 },
};
MODULE_DEVICE_TABLE(i2c, apa3165_i2c_id);
 
static struct i2c_driver apa3165_i2c_driver = {
	.driver = {
		.name = "apa3165",
		.owner = THIS_MODULE,
	},
	.probe = apa3165_i2c_probe,
	.remove = __devexit_p(apa3165_i2c_remove),
	.id_table = apa3165_i2c_id,
};
 
static int __init apa3165_init(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int i = 0;
 	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("apa3165", "apa3165_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[apa3165] type err!\n");
    } else {
    	apa3165_used = val.val;
    }

	if (apa3165_used) {
		adapter = i2c_get_adapter(I2C_CHANNEL_NUM);
		if (!adapter)
			return -ENODEV;
		for(i = 0; i < sizeof(apa3165_i2c_board_info)/sizeof(apa3165_i2c_board_info[0]);i++) {
			client = NULL;
			client = i2c_new_device(adapter, &apa3165_i2c_board_info[i]);
			if (!client)
				return -ENODEV;
		}
		i2c_put_adapter(adapter);

		type = script_get_item("apa3165", "aux_signal_time", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("[apa3165] aux_signal_time type err!\n");
	    } else {
			aux_signal_time = val.val;
	    }

		return i2c_add_driver(&apa3165_i2c_driver);
	} else {
		return 0;
	}
}
module_init(apa3165_init);
module_param_named(AUX_JACK_DETECT, AUX_JACK_DETECT, bool, S_IRUGO | S_IWUSR);
module_param_named(AUX_SIGNAL_DETECT, AUX_SIGNAL_DETECT, bool, S_IRUGO | S_IWUSR);
module_param_named(AUX_SIGNAL_SHUTDOWN, AUX_SIGNAL_SHUTDOWN, bool, S_IRUGO | S_IWUSR);

static void __exit apa3165_exit(void)
{
	i2c_del_driver(&apa3165_i2c_driver);
}
module_exit(apa3165_exit);

MODULE_DESCRIPTION("ASoC codec apa3165 driver");
MODULE_AUTHOR("huangxin <huangxin@allwinnertech.com>");
MODULE_LICENSE("GPL");
