/*
 * twl-common.c
 *
 * Copyright (C) 2011 Texas Instruments, Inc..
 * Author: Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/i2c.h>
#include <linux/i2c/twl.h>
#include <linux/gpio.h>

#include <plat/i2c.h>

#include "twl-common.h"

static struct i2c_board_info __initdata pmic_i2c_board_info = {
	.addr		= 0x48,
	.flags		= I2C_CLIENT_WAKE,
};

static struct twl4030_usb_data omap3_usb_pdata = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static int omap3_batt_table[] = {
/* 0 C */
30800, 29500, 28300, 27100,
26000, 24900, 23900, 22900, 22000, 21100, 20300, 19400, 18700, 17900,
17200, 16500, 15900, 15300, 14700, 14100, 13600, 13100, 12600, 12100,
11600, 11200, 10800, 10400, 10000, 9630,  9280,  8950,  8620,  8310,
8020,  7730,  7460,  7200,  6950,  6710,  6470,  6250,  6040,  5830,
5640,  5450,  5260,  5090,  4920,  4760,  4600,  4450,  4310,  4170,
4040,  3910,  3790,  3670,  3550
};

static struct twl4030_bci_platform_data omap3_bci_pdata = {
	.battery_tmp_tbl	= omap3_batt_table,
	.tblsize		= ARRAY_SIZE(omap3_batt_table),
};

static struct twl4030_madc_platform_data omap3_madc_pdata = {
	.irq_line	= 1,
};

static struct twl4030_codec_audio_data omap3_audio;

static struct twl4030_codec_data omap3_codec_pdata = {
	.audio_mclk = 26000000,
	.audio = &omap3_audio,
};


void __init omap_pmic_init(int bus, u32 clkrate,
			   const char *pmic_type, int pmic_irq,
			   struct twl4030_platform_data *pmic_data)
{
	strncpy(pmic_i2c_board_info.type, pmic_type,
		sizeof(pmic_i2c_board_info.type));
	pmic_i2c_board_info.irq = pmic_irq;
	pmic_i2c_board_info.platform_data = pmic_data;

	omap_register_i2c_bus(bus, clkrate, &pmic_i2c_board_info, 1);
}

void __init omap3_pmic_get_config(struct twl4030_platform_data *pmic_data,
				  u32 pdata_flags, u32 regulators_flags)
{
	if (!pmic_data->irq_base)
		pmic_data->irq_base = TWL4030_IRQ_BASE;
	if (!pmic_data->irq_end)
		pmic_data->irq_end = TWL4030_IRQ_END;

	/* Common platform data configurations */
	if (pdata_flags & TWL_COMMON_PDATA_USB && !pmic_data->usb)
		pmic_data->usb = &omap3_usb_pdata;

	if (pdata_flags & TWL_COMMON_PDATA_BCI && !pmic_data->bci)
		pmic_data->bci = &omap3_bci_pdata;

	if (pdata_flags & TWL_COMMON_PDATA_MADC && !pmic_data->madc)
		pmic_data->madc = &omap3_madc_pdata;

	if (pdata_flags & TWL_COMMON_PDATA_AUDIO && !pmic_data->codec)
		pmic_data->codec = &omap3_codec_pdata;
}

