/*
 * Copyright (C) 2012 - ISEE 2007 SL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/mach-types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <media/mt9p031.h>
#include <media/mt9v032.h>
#include <media/mt9v034.h>
#include <media/omap3isp.h>
#include <plat/omap_hwmod.h>

#include "common.h"
#include "devices.h"
#include "mux.h"
#include "../../../drivers/media/video/omap3isp/isp.h"

static bool camr00x0_enable __initdata = false;
static bool mt9v032_type __initdata = false;
static bool mt9p031_type __initdata = false;

#define MT9P031_RESET_GPIO	98
#define MT9P031_STANDBY_GPIO	111

#if defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)

static void camr00x0_set_clock(struct v4l2_subdev *subdev, unsigned int rate)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	isp->platform_cb.set_xclk(isp, rate, ISP_XCLK_A);
}

static int mt9p031_set_xclk(struct v4l2_subdev *subdev, int rate)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	isp->platform_cb.set_xclk(isp, rate, ISP_XCLK_A);
	return 0;
}

static const s64 mt9v034_link_freqs[] = {
	13000000,
	26600000,
	27000000,
	0,
};

static const s64 mt9v032_link_freqs[] = {
	13000000,
	26600000,
	27000000,
	0,
};

static struct mt9v034_platform_data mt9v034_pdata = {
	.clk_pol	= 0,
	.set_clock	= camr00x0_set_clock,
	.link_freqs	= mt9v034_link_freqs,
	.link_def_freq	= 27000000,
};

static struct mt9v032_platform_data mt9v032_pdata = {
	.clk_pol	= 0,
	.set_clock	= camr00x0_set_clock,
	.link_freqs	= mt9v032_link_freqs,
	.link_def_freq	= 26600000,
};

static struct mt9p031_platform_data mt9p031_pdata = {
	.set_xclk	= mt9p031_set_xclk,
	.reset		= MT9P031_RESET_GPIO,
	.ext_freq       = 21000000,
	.target_freq    = 48000000,
};

static struct i2c_board_info mt9v034_i2c_device = {
	I2C_BOARD_INFO("mt9v034", 0xb8 >> 1),
	.platform_data = &mt9v034_pdata,
};

static struct i2c_board_info mt9v032_i2c_device = {
	I2C_BOARD_INFO("mt9v032", 0x5c),
	.platform_data = &mt9v032_pdata,
};

static struct i2c_board_info mt9p031_i2c_device = {
	I2C_BOARD_INFO("mt9p031", 0x5d),
	.platform_data = &mt9p031_pdata,
};

static struct isp_subdev_i2c_board_info mt9v034_board_info[] = {
	{
		.board_info = &mt9v034_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_subdev_i2c_board_info mt9v032_board_info[] = {
	{
		.board_info = &mt9v032_i2c_device,
		.i2c_adapter_id = 3,
	},
	{ NULL, 0, },
};

static struct isp_subdev_i2c_board_info mt9p031_board_info[] = {
	{
		.board_info = &mt9p031_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group camr00x0_subdevs[] = {
	{
		/* default sensor is mt9v034 */
		.subdevs = mt9v034_board_info,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = { .parallel = {
			.data_lane_shift 	= ISP_LANE_SHIFT_0,
			.clk_pol 		= 0,
		} },
	},
	{ NULL, 0 },
};

static struct isp_platform_data camr00x0_isp_platform_data = {
	.subdevs = camr00x0_subdevs,
};

static struct omap_board_mux camr0010_board_mux[] __initdata = {
	OMAP3_MUX(CAM_HS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_VS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_XCLKA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_PCLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_STROBE, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D4, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D5, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D6, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D7, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D8, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D9, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	/* 10 bit cam sensor */
	OMAP3_MUX(CAM_D10, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP3_MUX(CAM_D11, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	/* CAM_STANDBYN (CAM_XCLKB.GPIO111) */
	OMAP3_MUX(CAM_XCLKB, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* CAM_NRESET (CAM_FLD.GPIO_98) */
	OMAP3_MUX(CAM_FLD, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* I2C 2 */
	OMAP3_MUX(I2C2_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(I2C2_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

static struct omap_board_mux camr0020_board_mux[] __initdata = {
	/* 12 bit cam sensor */
	OMAP3_MUX(CAM_D10, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D11, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

static int __init camr00x0_init(void)
{
	int ret;

	/* Check for compatibility */
	if (!camr00x0_enable)
		return 0;

	/* Write camr00x0 mux */
	omap_mux_write_array(omap_mux_get("core"), camr0010_board_mux);

	/* Register I2C bus (if is not registered) */
	if (machine_is_igep0020()) {
		ret = omap_register_i2c_bus(2, 100, NULL, 0);
		if (ret)
			pr_warning("IGEP: Could not register I2C2 bus (%d)\n",
				ret);
	} else if (machine_is_igep0030()) {
		mt9v034_board_info[0].i2c_adapter_id = 3;
		mt9p031_board_info[0].i2c_adapter_id = 3;
	}

	if (mt9v032_type) {
		pr_info("BUDDY: IGEP CAM (mt9v032)\n");
		camr00x0_subdevs[0].subdevs = mt9v032_board_info;
	}
	else if (mt9p031_type) {
		pr_info("BUDDY: IGEP CAM (mt9p031)\n");

		camr00x0_subdevs[0].subdevs = mt9p031_board_info;
		/* 12-bit sensor type */
		omap_mux_write_array(omap_mux_get("core"), camr0020_board_mux);

		if (gpio_request_one(MT9P031_STANDBY_GPIO,
		    GPIOF_OUT_INIT_HIGH, "mt9p031 standby"))
			pr_warning("Could not obtain gpio cam standby\n");

	} else {
		pr_info("BUDDY: IGEP CAM (mt9v034)\n");
	}

	omap3_init_camera(&camr00x0_isp_platform_data);

	return 0;
}

#else
static inline void camr00x0_init(void) {}
#endif

late_initcall(camr00x0_init);

static int __init buddy_early_param(char* str)
{
	char opt[16];

	if (!str)
		return -EINVAL;

	strncpy(opt, str, 16);

	if (!strcmp(opt, "camr00x0"))
		camr00x0_enable = true;

	return 0;
}

static int __init sensor_early_param(char* str)
{
	char opt[16];

	if (!str)
		return -EINVAL;

	strncpy(opt, str, 16);

	if (!strcmp(opt, "mt9v032"))
		mt9v032_type = true;
	else if (!strcmp(opt, "mt9p031"))
		mt9p031_type = true;

	return 0;	
}

early_param("buddy", buddy_early_param);
early_param("camr00x0.sensor", sensor_early_param);

