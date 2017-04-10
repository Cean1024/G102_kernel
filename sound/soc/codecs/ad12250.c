/*
 * ad12250.c  --  ad12250 ALSA Soc Audio driver
 *
 * Copyright(c) 2015-2018 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: guoguo <guoguo@allwinnertech.com>
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <linux/of.h>
#include <sound/tlv.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>
#include "../sunxi/sunxi_daudio.h"

struct ad12250;

struct ad12250_priv {
	struct ad12250 *ad12250;
};


static int init_flag ;

static const struct snd_soc_dapm_widget ad12250_dapm_widgets[] = {

};

/* Target, Path, Source */
static const struct snd_soc_dapm_route ad12250_audio_map[] = {
};

static const struct snd_kcontrol_new ad12250_controls[] = {
};

static int ad12250_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	u16 blen;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	if (init_flag == 0)
	{
		/*
		mask :0 -- bclk
		      1 -- lrck
		      2 -- mclk
		      3 -- global_enable
		*/
		printk("init_flag = %d\n", init_flag);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_BCLK, 0);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_LRCK, 0);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_GEN, 0);
		msleep(10);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_MCLK, 0);		
		msleep(30);
		
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_MCLK, 1);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_GEN, 1);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_BCLK, 1);
		msleep(10);
		daudio_set_clk_onoff(cpu_dai, SUNXI_DAUDIO_LRCK, 1);
		msleep(30);
		
		init_flag = 1;
	}
	printk("params_format(params) = %d", params_format(params));
	switch (params_format(params))
	{
	case SNDRV_PCM_FORMAT_S16_LE:
		blen = 0x0;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		blen = 0x1;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		blen = 0x2;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		blen = 0x3;
		break;
	default:
		dev_err(dai->dev, "Unsupported word length: %u\n",
			params_format(params));
		return -EINVAL;
	}
	return 0;
}

static int ad12250_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}
static int ad12250_set_sysclk(struct snd_soc_dai *dai,
			     int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int ad12250_set_clkdiv(struct snd_soc_dai *dai,
			     int div_id, int div)
{
	return 0;
}

static int ad12250_set_pll(struct snd_soc_dai *dai, int pll_id,
			  int source, unsigned int freq_in,
			  unsigned int freq_out)
{
	return 0;
}


static int ad12250_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	return 0;
}

static const struct snd_soc_dai_ops ad12250_dai_ops = {
	.hw_params = ad12250_hw_params,
	.set_fmt = ad12250_set_fmt,
	.set_sysclk = ad12250_set_sysclk,
	.set_clkdiv = ad12250_set_clkdiv,
	.set_pll = ad12250_set_pll,
	.hw_free = ad12250_hw_free,
};

#define ad12250_FORMATS (SNDRV_PCM_FMTBIT_S16_LE  | SNDRV_PCM_FMTBIT_S16_BE  | \
	 SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S18_3BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE  | SNDRV_PCM_FMTBIT_S24_BE  | \
	 SNDRV_PCM_FMTBIT_S32_LE  | SNDRV_PCM_FMTBIT_S32_BE)

#define ad12250_RATES SNDRV_PCM_RATE_8000_96000
static struct snd_soc_dai_driver ad12250_dai = {
		.name = "ad12250",
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = ad12250_RATES,
			.formats = ad12250_FORMATS,
		},
		.ops = &ad12250_dai_ops,
};

static int ad12250_codec_probe(struct snd_soc_codec *codec)
{
	return 0;
}

static int ad12250_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_ad12250 = {
	.probe = ad12250_codec_probe,
	.remove = ad12250_codec_remove,

	.dapm_widgets = ad12250_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ad12250_dapm_widgets),
	.dapm_routes = ad12250_audio_map,
	.num_dapm_routes = ARRAY_SIZE(ad12250_audio_map),
	.controls = ad12250_controls,
	.num_controls = ARRAY_SIZE(ad12250_controls),
};

static int ad12250_probe(struct platform_device *pdev)
{
	int ret=0;
	
	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_ad12250,
			&ad12250_dai, 1);
	return ret;
}

static int ad12250_remove(struct platform_device *pdev)
{	
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
	
}

static const struct of_device_id AD12250_codec_of_match[] = {
	{ .compatible = "allwinner,ad12250-codec", },
	{},
};

static struct platform_driver ad12250_codec_driver = {
	.driver = {
		.name = "ad12250-codec",
		.owner = THIS_MODULE,
		.of_match_table = AD12250_codec_of_match,
	},
	.probe = ad12250_probe,
	.remove = ad12250_remove,
};

module_platform_driver(ad12250_codec_driver);


MODULE_DESCRIPTION("ASoC ad12250 driver");
MODULE_AUTHOR("guoguo <guoguo@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ad12250-codec");
