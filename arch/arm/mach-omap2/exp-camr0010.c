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
#include <media/mt9v034.h>
#include <plat/omap_hwmod.h>
#include <plat/common.h>

#include <linux/i2c.h>
#include "devices.h"
#include "mux.h"
#include "../../../drivers/media/video/isp/isp.h"

static bool camr0010_enable __initdata = false;

#if defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)

static void mt9v034_set_clock(struct v4l2_subdev *subdev, unsigned int rate)
{
	struct isp_device *isp = v4l2_dev_to_isp_device(subdev->v4l2_dev);

	isp->platform_cb.set_xclk(isp, rate, ISP_XCLK_A);
}

static const s64 mt9v034_link_freqs[] = {
	13000000,
	26600000,
	27000000,
	0,
};

static struct mt9v034_platform_data cam0010_platform_data = {
	.clk_pol	= 0,
	.set_clock	= mt9v034_set_clock,
	.link_freqs	= mt9v034_link_freqs,
	.link_def_freq	= 13000000,
};

static struct i2c_board_info cam0010_camera_i2c_device = {
	I2C_BOARD_INFO("mt9v034", 0xb8 >> 1),
	.platform_data = &cam0010_platform_data,
};

static struct isp_subdev_i2c_board_info cam0010_camera_subdevs[] = {
	{
		.board_info = &cam0010_camera_i2c_device,
		.i2c_adapter_id = 3,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group camera_subdevs[] = {
	{
		.subdevs = cam0010_camera_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = { .parallel = {
			.data_lane_shift 	= 0, /* ISP_LANE_SHIFT_0 */
			.clk_pol 		= 0,
		} },
	},
	{ NULL, 0 },
};

static struct isp_platform_data camr0010_isp_platform_data = {
	.subdevs = camera_subdevs,
};

static struct omap_board_mux camr0010_board_mux[] __initdata = {
	OMAP3_MUX(CAM_HS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_VS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_XCLKA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_PCLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_FLD, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
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
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

static struct omap_board_mux i2c2_board_mux[] __initdata = {
	/* I2C 2 */
	OMAP3_MUX(I2C2_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(I2C2_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

int __init camr0010_init(void)
{
	struct omap_hwmod *oh;

	camr0010_enable = true;

	/* Check for compatibility */
	if (!camr0010_enable)
		return 0;

	pr_info("IGEP: CAMR0010\n");

	/* Write camera mux */
	omap_mux_write_array(omap_mux_get("core"), camr0010_board_mux);

	if (machine_is_igep0020()) {
		/* Write camera mux */
		omap_mux_write_array(omap_mux_get("core"), i2c2_board_mux);
		/* For IGEP0020 the i2c adapter is the number 2 */
		cam0010_camera_subdevs[0].i2c_adapter_id = 2;
		omap_register_i2c_bus(2, 100, NULL, 0);
	} else if (machine_is_igep0030())
		omap_register_i2c_bus(3, 100, NULL, 0);

	/* Register I2C bus (if is not registered) 
	TODO: seems doesn't work
	oh = omap_hwmod_lookup("i2c3");
	if (!oh)
		omap_register_i2c_bus(3, 100, NULL, 0);
	*/

	omap3_init_camera(&camr0010_isp_platform_data);

	return 0;
}

#else
inline void camr0010_init(void) {}
#endif

late_initcall(camr0010_init);

static int __init buddy_early_param(char* options)
{
	camr0010_enable = true;

	return 0;	

}
early_param("buddy", buddy_early_param);

