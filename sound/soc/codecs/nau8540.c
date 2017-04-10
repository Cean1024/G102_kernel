/*
 * nau8540.c  --  NAU85L40 ALSA SoC Audio driver
 *
 * Copyright 2015 Nuvoton Technology Corp.
 *  Author: CFYang3 <CFYang3@nuvoton.com>
 *  Co-author: David Lin <ctlin0@nuvoton.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>

#include "nau8540.h"
#include "../sunxi/sunxi_daudio.h"
struct i2c_client *nau_i2c0;
struct i2c_client *nau_i2c1;
struct nau8540_priv nau8540_i2c0;
struct nau8540_priv nau8540_i2c1;
struct regulator *pa_shdn;
int init_flag = 0;
static const struct reg_default nau8540_reg_defaults[] = {
	{0x01, 0x0000},
	{0x02, 0x0000},
	{0x03, 0x0000},
	{0x04, 0x0001},
	{0x05, 0x3126},
	{0x06, 0x0008},
	{0x07, 0x0010},
	{0x08, 0xC000},
	{0x09, 0x6000},
	{0x0A, 0xF13C},
	{0x10, 0x000B},
	{0x11, 0x0002},
	{0x12, 0x0000},
	{0x13, 0x0000},
	{0x14, 0x0000},
	{0x20, 0x0070},
	{0x21, 0x0000},
	{0x22, 0x0000},
	{0x23, 0x1010},
	{0x24, 0x1010},
	{0x30, 0x0000},
	{0x31, 0x0000},
	{0x32, 0x0000},
	{0x33, 0x0000},
	{0x34, 0x0000},
	{0x35, 0x0000},
	{0x36, 0x0000},
	{0x37, 0x0000},
	{0x38, 0x0000},
	{0x39, 0x0000},
	{0x3A, 0x0002},
	{0x40, 0x0400},
	{0x41, 0x1400},
	{0x42, 0x2400},
	{0x43, 0x0400},
	{0x44, 0x00E4},
	{0x50, 0x0000},
	{0x51, 0x0000},
	{0x52, 0xEFFF},
	{0x5A, 0x0000},
	{0x60, 0x0000},
	{0x61, 0x0000},
	{0x64, 0x0000},
	{0x65, 0x0020},
	{0x66, 0x0000},
	{0x67, 0x0004},
	{0x68, 0x0000},
	{0x69, 0x0000},
	{0x6A, 0x0000},
	{0x6B, 0x0101},
	{0x6C, 0x0101},
	{0x6D, 0x0000},
};

static u16 Set_Codec_Reg_Init[][2]={
	{0x01, 0x000F},
	{0x02, 0x8003},
	{0x03, 0x0040},
	{0x04, 0x0001},
	{0x05, 0x3126},
	{0x06, 0x0008},
	{0x07, 0x0010},
	{0x08, 0xC000},
	{0x09, 0xE000},
	{0x0A, 0xF13C},
	{0x10, 0x000f},
	{0x11, 0x0000},
	{0x12, 0x0000},
	{0x13, 0x0000},
	{0x14, 0xc00f},
	{0x20, 0x0000},
	{0x21, 0x700b},
	{0x22, 0x0022},
	{0x23, 0x1010},
	{0x24, 0x1010},
	{0x2D, 0x1010},
	{0x2E, 0x1010},
	{0x2F, 0x0000},
	{0x30, 0x0000},
	{0x31, 0x0000},
	{0x32, 0x0000},
	{0x33, 0x0000},
	{0x34, 0x0000},
	{0x35, 0x0000},
	{0x36, 0x0000},
	{0x37, 0x0000},
	{0x38, 0x0000},
	{0x39, 0x0000},
	{0x3A, 0x4002},
	{0x40, 0x0438},
	{0x41, 0x1438},
	{0x42, 0x2438},
	{0x43, 0x3438},
	{0x44, 0x80e4},
	{0x48, 0x0000},
	{0x49, 0x0000},
	{0x4A, 0x0000},
	{0x4B, 0x0000},
	{0x4C, 0x0000},
	{0x4D, 0x0000},
	{0x4E, 0x0000},
	{0x4F, 0x0000},
	{0x50, 0x0000},
	{0x51, 0x0000},
	{0x52, 0xEFFF},
	{0x57, 0x0000},
	{0x58, 0x1CF0},
	{0x59, 0x0008},
	{0x60, 0x0060},
	{0x61, 0x0000},
	{0x62, 0x0000},
	{0x63, 0x0000},
	{0x64, 0x0011},
	{0x65, 0x0220},
	{0x66, 0x000F},
	{0x67, 0x0D04},
	{0x68, 0x7000},
	{0x69, 0x0000},
	{0x6A, 0x0000},
	{0x6B, 0x1010},
	{0x6C, 0x1010},
	{0x6D, 0xF000},
};

#define SET_CODEC_REG_INIT_NUM	ARRAY_SIZE(Set_Codec_Reg_Init)

static bool nau8540_volatile(struct device *dev, unsigned int reg)
{ 
	switch (reg) {
	case REG0x00_SW_RESET: 
	case REG0x2D_ALC_GAIN_CH12 ... REG0x2F_ALC_STATUS:
	case REG0x48_P2P_CH1 ... REG0x4F_PEAK_CH4:
	case REG0x58_I2C_DEVICE_ID:
	case REG0x5A_RST:
		return true;
	default:
		return false;
	}
}

static int nau8540_reset(struct snd_soc_codec *codec) 
{
	printk("nau8540_reset in\n");
	return snd_soc_write(codec, REG0x00_SW_RESET, 0);
	printk("nau8540_reset out\n");
}
static const DECLARE_TLV_DB_MINMAX_MUTE(adc_vol_tlv, -12800, 3600);
static const DECLARE_TLV_DB_MINMAX(fepga_gain_tlv, -100, 3600);

static const struct snd_kcontrol_new nau8540_controls[] = {
	SOC_SINGLE_TLV("MIC1 Digital Volume", REG0x40_DIGITAL_GAIN_CH1, 0, 0x7ff, 0, adc_vol_tlv),
	SOC_SINGLE_TLV("MIC2 Digital Volume", REG0x41_DIGITAL_GAIN_CH2, 0, 0x7ff, 0, adc_vol_tlv),
	SOC_SINGLE_TLV("MIC3 Digital Volume", REG0x42_DIGITAL_GAIN_CH3, 0, 0x7ff, 0, adc_vol_tlv),
	SOC_SINGLE_TLV("MIC4 Digital Volume", REG0x43_DIGITAL_GAIN_CH4, 0, 0x7ff, 0, adc_vol_tlv),

	SOC_SINGLE_TLV("Frontend PGA1 Volume", REG0x6B_FEPGA3, 0, 0x3f, 0, fepga_gain_tlv),
	SOC_SINGLE_TLV("Frontend PGA2 Volume", REG0x6B_FEPGA3, 8, 0x3f, 0, fepga_gain_tlv),
	SOC_SINGLE_TLV("Frontend PGA3 Volume", REG0x6C_FEPGA4, 0, 0x3f, 0, fepga_gain_tlv),
	SOC_SINGLE_TLV("Frontend PGA4 Volume", REG0x6C_FEPGA4, 8, 0x3f, 0, fepga_gain_tlv),
};

static const struct snd_soc_dapm_widget nau8540_dapm_widgets[] = {
	
	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("MIC3"),
	SND_SOC_DAPM_INPUT("MIC4"),

	SND_SOC_DAPM_PGA("Frontend PGA1", REG0x6D_PWR, 12, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Frontend PGA2", REG0x6D_PWR, 13, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Frontend PGA3", REG0x6D_PWR, 14, 0, NULL, 0),
	SND_SOC_DAPM_PGA("Frontend PGA4", REG0x6D_PWR, 15, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("MICBIAS1", REG0x67_MIC_BIAS, 10, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("MICBIAS2", REG0x67_MIC_BIAS, 11, 0, NULL, 0),

	SND_SOC_DAPM_ADC("ADC1", NULL, REG0x01_POWER_MANAGEMENT, 0, 0),
	SND_SOC_DAPM_ADC("ADC2", NULL, REG0x01_POWER_MANAGEMENT, 1, 0),
	SND_SOC_DAPM_ADC("ADC3", NULL, REG0x01_POWER_MANAGEMENT, 2, 0),
	SND_SOC_DAPM_ADC("ADC4", NULL, REG0x01_POWER_MANAGEMENT, 3, 0),

	SND_SOC_DAPM_AIF_OUT("AIFTX", "Capture", 0, REG0x11_PCM_CTRL1, 15, 1),
};

static const struct snd_soc_dapm_route nau8540_dapm_routes[] = {

	{"Frontend PGA1", NULL, "MIC1"},
	{"Frontend PGA2", NULL, "MIC2"},
	{"Frontend PGA3", NULL, "MIC3"},
	{"Frontend PGA4", NULL, "MIC4"},

	{"ADC1", NULL, "Frontend PGA1"},
	{"ADC2", NULL, "Frontend PGA2"},
	{"ADC3", NULL, "Frontend PGA3"},
	{"ADC4", NULL, "Frontend PGA4"},

	{"ADC1", NULL, "MICBIAS1"},
	{"ADC2", NULL, "MICBIAS1"},
	{"ADC3", NULL, "MICBIAS2"},
	{"ADC4", NULL, "MICBIAS2"},

	{"AIFTX", NULL, "ADC1"},
	{"AIFTX", NULL, "ADC2"},
	{"AIFTX", NULL, "ADC3"},
	{"AIFTX", NULL, "ADC4"},
};

static int nau8540_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	printk("nau8540_hw_params in\n");
	struct snd_soc_codec *codec = dai->codec;
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);
	u32 reg_value = 0;
	int i;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	if(init_flag == 0)
	{
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_BCLK, 0);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_LRCK, 0);
		msleep(10);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_MCLK, 0);
		for (i = 0; i < SET_CODEC_REG_INIT_NUM; i++ )
		{
			regmap_write(nau8540_i2c0.regmap, Set_Codec_Reg_Init[i][0],
						Set_Codec_Reg_Init[i][1]);
		}
		regmap_write(nau8540_i2c0.regmap, 0x12,0x80);

		for (i = 0; i < SET_CODEC_REG_INIT_NUM; i++ )
		{
			regmap_write(nau8540_i2c1.regmap, Set_Codec_Reg_Init[i][0],
						Set_Codec_Reg_Init[i][1]);
		}
		printk("SUNXI_DAUDIO_MCLK on\n");
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_MCLK, 1);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_BCLK, 1);
		msleep(100);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_LRCK, 1);
		init_flag = 1;
	}

	u16 val_len = 0;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		val_len |= 0x4;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		val_len |= 0x8;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		val_len |= 0xc;
		break;
	default:
		return -EINVAL;
	}
	//iflytek del
	//regmap_update_bits(nau8540->regmap, REG0x10_PCM_CTRL0, 0xc, val_len);
	printk("nau8540_hw_params out\n");
	return 0;
}

static int nau8540_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				 int clk_id, unsigned int freq, int dir)
{
	printk("nau8540_set_dai_sysclk in\n");
	struct snd_soc_codec *codec = codec_dai->codec;
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);

	//dev_err(codec->dev, "MCLK rate %dHz not supported\n", freq);
	printk("nau8540_set_dai_sysclk out\n");
	return 0;
}


static int nau8540_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	printk("nau8540_set_dai_fmt in\n");
	struct snd_soc_codec *codec = codec_dai->codec;
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);
	u16 reg_val_0 = 0, reg_val_1 = 0;
	
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		reg_val_0 |= 0x8;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		dev_alert(codec->dev, "Invalid DAI master/slave interface\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		reg_val_1 |= 0x2;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		reg_val_1 |= 0x1;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		reg_val_1 |= 0x3;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		reg_val_1 |= 0x3;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		reg_val_1 |= 0x80;
		break;
	default:
		return -EINVAL;
	}
	//iflytek del
	//regmap_update_bits(nau8540->regmap, REG0x10_PCM_CTRL0, 0x83, reg_val_1);
	//regmap_update_bits(nau8540->regmap, REG0x11_PCM_CTRL1, 0x8, reg_val_0);
	printk("nau8540_set_dai_fmt out\n");
	return 0;
}

static int nau8540_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	printk("nau8540_set_bias_level in\n");
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);
	int ret;

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
			regcache_sync(nau8540->regmap);
			if (ret) {
				dev_err(codec->dev,
					"Failed to sync cache: %d\n", ret);
				return ret;
			}
		}
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}

	codec->dapm.bias_level = level;
	printk("nau8540_set_bias_level out\n");
	return 0;
}

#define NAU8540_RATES SNDRV_PCM_RATE_8000_48000

#define NAU8540_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops nau8540_dai_ops = {
	.hw_params	= nau8540_hw_params,
	//.set_sysclk	= nau8540_set_dai_sysclk,
	.set_fmt	= nau8540_set_dai_fmt,
};

static struct snd_soc_dai_driver nau8540_dai0 = {
	.name = "nau8540-pcm0",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,	/* Mono modes not yet supported */
		.channels_max = 8,
		.rates = NAU8540_RATES,
		.formats = NAU8540_FORMATS,
	},
	.ops = &nau8540_dai_ops,
};

static struct snd_soc_dai_driver nau8540_dai1 = {
	.name = "nau8540-pcm1",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,	/* Mono modes not yet supported */
		.channels_max = 8,
		.rates = NAU8540_RATES,
		.formats = NAU8540_FORMATS,
	},
	.ops = &nau8540_dai_ops,
};

#ifdef CONFIG_PM
static int nau8540_suspend(struct snd_soc_codec *codec)
{
	printk("nau8540_suspend in\n");
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);

	nau8540_set_bias_level(codec, SND_SOC_BIAS_OFF);
	regcache_cache_only(nau8540->regmap, true);
	
//	if (!IS_ERR(pa_shdn)) {
//		regulator_disable(pa_shdn);
//                        gpio_set_value(pa_shdn, 0);
//	}


	return 0;
}

static int nau8540_resume(struct snd_soc_codec *codec)
{
	printk("nau8540_resume in\n");
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);
	int ret;
//	if (!IS_ERR(pa_shdn)) {
//		ret = regulator_enable(pa_shdn);
//                        gpio_set_value(pa_shdn, 1);
//		if (ret) {
//			pr_err("[%s]: cpvdd:regulator_enable() failed!\n",__func__);
//		}
//	}

	nau8540_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	regcache_cache_only(nau8540->regmap, false);
	printk("nau8540_resume out\n");
	return 0;
}
#else
#define nau8540_suspend NULL
#define nau8540_resume NULL
#endif
static int nau8540_probe(struct snd_soc_codec *codec)
{
	struct nau8540_priv *nau8540 = snd_soc_codec_get_drvdata(codec);
	int ret,i;

	codec->control_data = nau8540->regmap;

	ret = snd_soc_codec_set_cache_io(codec, 8, 16, SND_SOC_REGMAP);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}
	/*iflytek add nau8540 reset*/
	//regmap_write(nau8540_i2c0.regmap, 0,0);
	//regmap_write(nau8540_i2c1.regmap, 0,0);
	//mdelay(200);
	/*iflytek add i2c0 i2c1 init*/
	#if 0
	for (i = 0; i < SET_CODEC_REG_INIT_NUM; i++ )
	{
		regmap_write(nau8540_i2c0.regmap, Set_Codec_Reg_Init[i][0],
					Set_Codec_Reg_Init[i][1]);
	}
	regmap_write(nau8540_i2c0.regmap, 0x12,0x80);
	
	for (i = 0; i < SET_CODEC_REG_INIT_NUM; i++ )
	{
		regmap_write(nau8540_i2c1.regmap, Set_Codec_Reg_Init[i][0],
					Set_Codec_Reg_Init[i][1]);
	}

	nau8540_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	#endif
	return 0;
}

static int nau8540_remove(struct snd_soc_codec *codec)
{
	printk("nau8540_remove in\n");
	nau8540_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_nau8540 = {
	.probe		= nau8540_probe,
	.remove		= nau8540_remove,
	.suspend	= nau8540_suspend,
	.resume		= nau8540_resume,
	.set_bias_level = nau8540_set_bias_level,
	.controls = nau8540_controls,
	.num_controls = ARRAY_SIZE(nau8540_controls),
	.dapm_widgets = nau8540_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(nau8540_dapm_widgets),
	.dapm_routes = nau8540_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(nau8540_dapm_routes),
};

static const struct regmap_config nau8540_regmap_config = {
	.reg_bits = 16,/*iflytek nau8540 reg and value is 16bits*/
	.val_bits = 16,

	.max_register = NAU8540_MAX_REGISTER,
	.volatile_reg = nau8540_volatile,

	.cache_type = REGCACHE_RBTREE,
	.reg_defaults = nau8540_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(nau8540_reg_defaults),
};

static int nau8540_i2c_probe(struct i2c_client *i2c,
				const struct i2c_device_id *id)
{
	struct nau8540_priv *nau8540;
	struct device_node *np = i2c->dev.of_node;
	struct gpio_config config;    
	int ret, i;
	nau8540 = devm_kzalloc(&i2c->dev, sizeof(*nau8540), GFP_KERNEL);
	if (!nau8540)
		return -ENOMEM;

	nau8540->regmap = devm_regmap_init_i2c(i2c, &nau8540_regmap_config);
	if (IS_ERR(nau8540->regmap)) {
		ret = PTR_ERR(nau8540->regmap);
		return ret;
	}
/*
	pa_shdn =  regulator_get(NULL, "pa-shdn"); //HPVCC
	if (IS_ERR(pa_shdn)) {
		pr_err("get audio pa-shdn failed 8540\n");
	} else {
		ret = regulator_set_voltage(pa_shdn , 3300000 , 3300000);
		if (ret) {
			pr_err("[%s]:pa_shdn:regulator_set_volatge failed!\n", __func__);
		}
		ret = regulator_enable(pa_shdn);
		if (ret) {
			pr_err("[%s]: pa_shdn:regulator_enable() failed!\n", __func__);
		}
	}
	*/

        //add by jeffrey
        pa_shdn = of_get_named_gpio_flags(np, "mic_pw_en",
                                                0, (enum of_gpio_flags *)&config);
        if (!gpio_is_valid(pa_shdn)) {
                printk("[jeffrey_nau8540] failed to get pa_shdn gpio from dts,reset:%d\n", pa_shdn);
        } else {
                ret = gpio_request(pa_shdn, "mic_pw_en");
                if (ret) {
                        printk("[jeffrey_nau8540] failed to request mic_pw_en gpio\n");
                } else {
                        gpio_direction_output(pa_shdn, 1);
                        gpio_set_value(pa_shdn, 1);
                        msleep(5);
                }    
        }    
        //---------------end

	i2c_set_clientdata(i2c, nau8540);

	if (id->driver_data == 0) {
		pr_err("iflytek test:i2c0 \n");
		ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_nau8540, &nau8540_dai0, 1);
		nau_i2c0 = i2c;
		nau8540_i2c0.regmap = nau8540->regmap;
	} else if (id->driver_data == 1) {
		pr_err("iflytek test:i2c1 \n");
		ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_nau8540, &nau8540_dai1, 1);
		nau_i2c1 = i2c;
		nau8540_i2c1.regmap = nau8540->regmap;
	} else
		pr_err("The wrong id number :%d\n", id->driver_data);
	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to register nau8540: %d\n", ret);
	}
	
	//ret = snd_soc_register_codec(&i2c->dev,
	//			&soc_codec_dev_nau8540, &nau8540_dai, 1);
	
	return ret;
}

static int nau8540_i2c_remove(struct i2c_client *client)
{
	printk("nau8540_i2c_remove in\n");
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id nau8540_of_match[] = {
	{ .compatible = "nau8540_0",},
	{ .compatible = "nau8540_1",},
	{ }
};
MODULE_DEVICE_TABLE(of, nau8540_of_match);
#endif

static const struct i2c_device_id nau8540_i2c_id[] = {
	{ "nau8540_0", 0 },
	{ "nau8540_1", 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, nau8540_i2c_id);

static struct i2c_driver nau8540_i2c_driver = {
	.driver = {
		.name = "nau8540",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(nau8540_of_match),
	},
	.probe = nau8540_i2c_probe,
	.remove = nau8540_i2c_remove,
	.id_table = nau8540_i2c_id,
};
module_i2c_driver(nau8540_i2c_driver);

MODULE_DESCRIPTION("ASoC NAU85L40 driver");
MODULE_AUTHOR("CFYang3 <CFYang3@nuvoton.com>");
MODULE_LICENSE("GPL");
