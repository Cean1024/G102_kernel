/*
 * drivers/input/sensor/sunxi_gpadc_test.c
 *
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC test file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include "sunxi_gpadc.h"

struct sunxi_gpadc *sunxi_gpadc_handler;

extern void sunxi_gpadc_sample_rate_set(void __iomem *reg_base, u32 clk_in, u32 round_clk);
extern void sunxi_gpadc_ctrl_set(void __iomem *reg_base, u32 ctrl_para);
extern u32 sunxi_gpadc_ch_select(void __iomem *reg_base, enum gp_channel_id id);
extern u32 sunxi_gpadc_ch_deselect(void __iomem *reg_base, enum gp_channel_id id);
extern u32 sunxi_gpadc_mode_select(void __iomem *reg_base, enum gp_select_mode mode);
extern u32 sunxi_gpadc_ch_cmp(void __iomem *reg_base, enum gp_channel_id id, u32 low_uv, u32 hig_uv);
extern u32 sunxi_gpadc_cmp_select(void __iomem *reg_base, enum gp_channel_id id);
extern void sunxi_gpadc_en(void __iomem *reg_base, bool onoff);
extern u32 sunxi_gpadc_int_cfg(void __iomem *reg_base);
extern void sunxi_gpadc_int_set(void __iomem *reg_base, u32 int_para);
extern void sunxi_gpadc_int_clr(void __iomem *reg_base, u32 int_para);
extern u32 sunxi_gpadc_read_ints(void __iomem *reg_base);
extern void sunxi_gpadc_clr_ints(void __iomem *reg_base, u32 int_para);
extern u32 sunxi_gpadc_read_data(void __iomem *reg_base, enum gp_channel_id id);
extern struct sunxi_gpadc *sunxi_gpadc_get_handler(void);


static irqreturn_t sunxi_gpadc_interrupt(int irqno, void *dev_id)
{
	struct sunxi_gpadc *sunxi_gpadc_handler = (struct sunxi_gpadc *)dev_id;
	u32  reg_val = 0;

	reg_val = sunxi_gpadc_read_ints(sunxi_gpadc_handler->reg_base);
	reg_val &= sunxi_gpadc_int_cfg(sunxi_gpadc_handler->reg_base);

	if (reg_val & GP_CH0_LOW_IRQ_PEND)
		printk("channel 0 low pend\n");

	if (reg_val & GP_CH1_LOW_IRQ_PEND)
		printk("channel 1 low pend\n");

	if (reg_val & GP_CH0_DATA_IRQ_PEND)
		printk("val_ch0 = %d mv\n", sunxi_gpadc_read_data(sunxi_gpadc_handler->reg_base, GP_CH_0)*561);

	if (reg_val & GP_CH1_DATA_IRQ_PEND)
		printk("val_ch1 = %d mv\n", sunxi_gpadc_read_data(sunxi_gpadc_handler->reg_base, GP_CH_1)*561);

	if (reg_val & GP_CH0_HIG_IRQ_PEND)
		printk("channel 0 hight pend\n");

	if (reg_val & GP_CH1_HIG_IRQ_PEND)
		printk("channel 1 hight pend\n");

	sunxi_gpadc_clr_ints(sunxi_gpadc_handler->reg_base, reg_val);

	return IRQ_HANDLED;
}


static int __init gp_test_init(void)
{
	int try_time = 50;
	int val;

	printk("gp_test_init\n");

	sunxi_gpadc_handler = sunxi_gpadc_get_handler();
	if (NULL == sunxi_gpadc_handler) {
		pr_err("sunxi_gpadc_handler is null !\n");
		return -EINVAL;
	}

	sunxi_gpadc_sample_rate_set(sunxi_gpadc_handler->reg_base, OSC_24MHZ, 50000);
	sunxi_gpadc_int_clr(sunxi_gpadc_handler->reg_base, ~0);

	sunxi_gpadc_mode_select(sunxi_gpadc_handler->reg_base, GP_CONTINUOUS_MODE);
	sunxi_gpadc_ch_select(sunxi_gpadc_handler, GP_CH_0);

#if 0
	/* config for channle 1, enable low voltage interrupt */
	sunxi_gpadc_ch_select(sunxi_gpadc_handler, GP_CH_1);
	sunxi_gpadc_ch_cmp(sunxi_gpadc_handler->reg_base, GP_CH_1, 1500000, VOL_RANGE);
	sunxi_gpadc_cmp_select(sunxi_gpadc_handler, GP_CH_1);
	if (request_irq(sunxi_gpadc_handler->irq_num, sunxi_gpadc_interrupt,
		 IRQF_TRIGGER_NONE, "sunxi-gpadc", sunxi_gpadc_handler)) {
		pr_err("sunxi_gpadc request irq failure\n");
		return -1;
	}
	sunxi_gpadc_int_set(sunxi_gpadc_handler->reg_base, GP_CH1_LOW_IRQ_EN);
#endif

	/* enable GPADC */
	sunxi_gpadc_en(sunxi_gpadc_handler, true);

	/* get the data of channel 0 */
	while (try_time--) {
		val = sunxi_gpadc_read_data(sunxi_gpadc_handler->reg_base, GP_CH_0);
		printk("val = 0x%03x ,  vol=%d mv\n", val, val*561);
		mdelay(100);
	}

	return 0;
}

static void __exit gp_test_exit(void)
{
	printk("gp_test_exit\n");
#if 0
	free_irq(sunxi_gpadc_handler->irq_num, sunxi_gpadc_handler);
#endif
	sunxi_gpadc_en(sunxi_gpadc_handler, false);
}

module_init(gp_test_init);
module_exit(gp_test_exit);


MODULE_AUTHOR("Fuzhaoke");
MODULE_DESCRIPTION("sunxi-gpadc driver test");
MODULE_LICENSE("GPL");
