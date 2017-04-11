/*
 * Codec driver for ST STA32x 2.1-channel high-efficiency digital audio system
 *
 * Copyright: 2011 Raumfeld GmbH
 * Author: Johannes Stezenbach <js@sig21.net>
 *
 * based on code from:
 *	wolfson Microelectronics PLC.
 *	  Mark Brown <broonie@opensource.wolfsonmicro.com>
 *	Freescale Semiconductor, Inc.
 *	  Timur Tabi <timur@freescale.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s:%d: " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
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
#include "sta32x.h"


#define STA32X_RATES (SNDRV_PCM_RATE_32000 | \
		      SNDRV_PCM_RATE_44100 | \
		      SNDRV_PCM_RATE_48000 | \
		      SNDRV_PCM_RATE_88200 | \
		      SNDRV_PCM_RATE_96000 | \
		      SNDRV_PCM_RATE_176400 | \
		      SNDRV_PCM_RATE_192000)

#define STA32X_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE  | SNDRV_PCM_FMTBIT_S16_BE  | \
	 SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S18_3BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE  | SNDRV_PCM_FMTBIT_S24_BE  | \
	 SNDRV_PCM_FMTBIT_S32_LE  | SNDRV_PCM_FMTBIT_S32_BE)

/* Power-up register defaults */
static const struct reg_default dsp1_regs[] = {
	{  0x0, 0x63 },
	{  0x1, 0x80 },
	{  0x2, 0x97 },
	{  0x3, 0x40 },
	{  0x4, 0xc2 },
	{  0x5, 0xDC },
	{  0x6, 0x10 },
	{  0x7, 0xFF },
	{  0x8, 0x60 },
	{  0x9, 0x60 },
	{  0xa, 0x60 }, 
	{  0xb, 0x80 },
	{  0xc, 0x00 },
	{  0xe, 0x00 },
	{  0xf, 0x40 },
	{ 0x10, 0x80 }, 
	{ 0x11, 0x77 },
	{ 0x12, 0x6a },
	{ 0x13, 0x69 },
	{ 0x14, 0x6a },
	{ 0x15, 0x69 },
	{ 0x16, 0x00 },
	{ 0x17, 0x00 },
	{ 0x18, 0x00 },
	{ 0x19, 0x00 },
	{ 0x1a, 0x00 },
	{ 0x1b, 0x00 },
	{ 0x1c, 0x00 },
	{ 0x1d, 0x00 },
	{ 0x1e, 0x00 },
	{ 0x1f, 0x00 },
	{ 0x20, 0x00 },
	{ 0x21, 0x00 },
	{ 0x22, 0x00 },
	{ 0x23, 0x00 },
	{ 0x24, 0x00 },
	{ 0x25, 0x00 },
	{ 0x26, 0x00 },
	{ 0x27, 0x1A },
	{ 0x31, 0x80 },
};

/* regulator power supply names */
static const char *dsp1_supply_names[] = {
#ifndef CONFIG_ARCH_SUN8IW10
	"Vdda",	/* analog supply, 3.3VV */
	"Vdd3",	/* digital supply, 3.3V */
	"Vcc"	/* power amp spply, 10V - 36V */
#endif
};

/* codec private data */
struct dsp1_priv {
	struct regmap *regmap;
	struct regulator_bulk_data supplies[ARRAY_SIZE(dsp1_supply_names)];
	struct snd_soc_codec *codec;

	unsigned int mclk;
	unsigned int format;
	struct delayed_work watchdog_work;
	int shutdown;
};

/* MCLK interpolation ratio per fs */
static struct {
	int fs;
	int ir;
} interpolation_ratios[] = {
	{ 32000, 0 },
	{ 44100, 0 },
	{ 48000, 0 },
	{ 88200, 1 },
	{ 96000, 1 },
	{ 176400, 2 },
	{ 192000, 2 },
};

/* MCLK to fs clock ratios */
static struct {
	int ratio;
	int mcs;
} mclk_ratios[3][7] = {
	{ { 768, 0 }, { 512, 1 }, { 384, 2 }, { 256, 3 },
	  { 128, 4 }, { 576, 5 }, { 0, 0 } },
	{ { 384, 0 }, { 256, 1 }, { 192, 2 }, { 128, 3 }, {64, 4 }, { 0, 0 } },
	{ { 192, 0 }, { 128, 1 }, { 96, 2 }, { 64, 3 }, {32, 4 }, { 0, 0 } },
};


static int dsp1_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct dsp1_priv *dsp1 = snd_soc_codec_get_drvdata(codec);
	int i, j, ir, fs;
	unsigned int rates = 0;
	unsigned int rate_min = -1;
	unsigned int rate_max = 0;

	pr_debug("mclk=%u\n", freq);

	return 0;
}


static int dsp1_set_dai_fmt(struct snd_soc_dai *codec_dai,
			      unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct dsp1_priv *dsp1 = snd_soc_codec_get_drvdata(codec);


	return 0;
}

static int dsp1_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct dsp1_priv *dsp1 = snd_soc_codec_get_drvdata(codec);
	unsigned int rate;
	int i, mcs = -1, ir = -1;
	u8 confa, confb;

	rate = params_rate(params);
	printk("rate: %u\n", rate);
	
	return 0;
}

/**
 * dsp1_set_bias_level - DAPM callback
 * @codec: the codec device
 * @level: DAPM power level
 *
 * This is called by ALSA to put the codec into low power mode
 * or to wake it up.  If the codec is powered off completely
 * all registers must be restored after power on.
 */
static int dsp1_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	int ret;
	struct dsp1_priv *dsp1 = snd_soc_codec_get_drvdata(codec);

	printk("level = %d\n", level);

	return 0;
}

static const struct snd_soc_dai_ops dsp1_dai_ops = {
	.hw_params	= dsp1_hw_params,
	.set_sysclk	= dsp1_set_dai_sysclk,
	.set_fmt	= dsp1_set_dai_fmt,
};

static struct snd_soc_dai_driver dsp1_dai = {
	.name = "DSP1",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = STA32X_RATES,
		.formats = STA32X_FORMATS,
	},
	.ops = &dsp1_dai_ops,
};

#ifdef CONFIG_PM
static int dsp1_suspend(struct snd_soc_codec *codec)
{

	return 0;
}

static int dsp1_resume(struct snd_soc_codec *codec)
{
	int ret;

	return 0;
}
#else
#define dsp1_suspend NULL
#define dsp1_resume NULL
#endif

static int dsp1_probe(struct snd_soc_codec *codec)
{
	struct dsp1_priv *dsp1 = snd_soc_codec_get_drvdata(codec);
	int i, ret = 0, thermal = 0;
	printk("[kean]:in %s\n",__func__);


	return 0;
}

static int dsp1_remove(struct snd_soc_codec *codec)
{
	struct dsp1_priv *dsp1 = snd_soc_codec_get_drvdata(codec);

	return 0;
}

static bool dsp1_reg_is_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {

		return 0;
	}
	return 0;
}

static const struct snd_soc_codec_driver dsp1_codec = {
	.probe =		dsp1_probe,
	.remove =		dsp1_remove,
	.suspend =		dsp1_suspend,
	.resume =		dsp1_resume,
	.set_bias_level =	dsp1_set_bias_level,
};

static const struct regmap_config dsp1_regmap = {
	.reg_bits =		8,
	.val_bits =		8,
	.max_register =		STA32X_STATUS,
	.reg_defaults =		dsp1_regs,
	.num_reg_defaults =	ARRAY_SIZE(dsp1_regs),
	.cache_type =		REGCACHE_RBTREE,
	.volatile_reg =		dsp1_reg_is_volatile,
};

static int dsp1_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct dsp1_priv *dsp1;
	int ret, i;
	struct device_node *np = i2c->dev.of_node;
	struct gpio_config config;
	struct gpio_config config_sw;
	printk("[kean]:in %s\n",__func__);
	dsp1 = devm_kzalloc(&i2c->dev, sizeof(struct dsp1_priv),
			      GFP_KERNEL);
	if (!dsp1)
		return -ENOMEM;


	/* regulators */
	for (i = 0; i < ARRAY_SIZE(dsp1->supplies); i++)
		dsp1->supplies[i].supply = dsp1_supply_names[i];

	ret = devm_regulator_bulk_get(&i2c->dev, ARRAY_SIZE(dsp1->supplies),
				      dsp1->supplies);
	if (ret != 0) {
		dev_err(&i2c->dev, "Failed to request supplies: %d\n", ret);
		return ret;
	}

	dsp1->regmap = devm_regmap_init_i2c(i2c, &dsp1_regmap);
	if (IS_ERR(dsp1->regmap)) {
		ret = PTR_ERR(dsp1->regmap);
		dev_err(&i2c->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	i2c_set_clientdata(i2c, dsp1);
	ret = snd_soc_register_codec(&i2c->dev, &dsp1_codec, &dsp1_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);
	printk("[kean]:out %s\n",__func__);
	return ret;
}

static int dsp1_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id dsp1_i2c_id[] = {
	{ "dspcodec1", 0 },
	{ "sta328", 0 },
	{ "sta329", 0 },
	{ "sta369", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, dsp1_i2c_id);

static struct i2c_driver dsp1_i2c_driver = {
	.driver = {
		.name = "dspcodec1",
		.owner = THIS_MODULE,
	},
	.probe =    dsp1_i2c_probe,
	.remove =   dsp1_i2c_remove,
	.id_table = dsp1_i2c_id,
};

module_i2c_driver(dsp1_i2c_driver);

MODULE_DESCRIPTION("ASoC DSP1 driver");
MODULE_AUTHOR("Kean.wu <kean.wu@hansong-china.com>");
MODULE_LICENSE("GPL");
