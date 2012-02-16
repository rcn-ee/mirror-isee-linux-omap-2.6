/*
 * Copyright (C) 2010-2011 - ISEE 2007 SL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>

#include <linux/spi/spi.h>
#include <linux/i2c/twl.h>
#include <linux/mfd/twl4030-codec.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/mcspi.h>

#include "board-igep00x0.h"
#include "mux.h"

#define IGEP2_GPIO_TSC2046_IRQ		155
#define IGEP2_GPIO_MODEM_ON_NOFF	141
#define IGEP2_GPIO_MODEM_NRESET		140
#define IGEP2_GPIO_MODEM_PWRMON		156
#define IGEP2_GPIO_MCP251X_IRQ		157
#define IGEP2_GPIO_MCP251X_NRESET	136
#define IGEP2_GPIO_TVP5151_PDN		126
#define IGEP2_GPIO_TVP5151_RESET	167

#if defined(CONFIG_VIDEO_OMAP3) || \
	defined(CONFIG_VIDEO_OMAP3_MODULE)

static struct i2c_board_info igep0022_camera_i2c_devices[] = {
	{
		I2C_BOARD_INFO("tvp5150", (0xb8 >> 1)),
	},
};

static struct isp_subdev_i2c_board_info igep0022_camera_primary_subdevs[] = {
	{
		.board_info = &igep0022_camera_i2c_devices[0],
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct isp_v4l2_subdevs_group igep0022_camera_subdevs[] = {
	{
		.subdevs = igep0022_camera_primary_subdevs,
		.interface = ISP_INTERFACE_PARALLEL,
		.bus = { .parallel = {
				.width			= 8,
				.data_lane_shift	= 0,
				.clk_pol		= 0,
				.hdpol                  = 0,
				.vdpol                  = 0,
				.fldmode                = 1,
				.bridge		= ISPCTRL_PAR_BRIDGE_DISABLE,
				.is_bt656               = 1,
		} },
	},
	{ NULL, 0, },
};

static struct isp_platform_data isp_pdata = {
	.subdevs = igep0022_camera_subdevs,
};
#endif

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux igep0022_mux[] __initdata = {
	/* McSPI 1 */
	OMAP3_MUX(MCSPI1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SIMO, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SOMI, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	/* OMAP3 ISP */
	OMAP3_MUX(CAM_STROBE, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(CAM_FLD, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_HS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_VS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_XCLKA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_PCLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
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
	OMAP3_MUX(CAM_D10, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_D11, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(CAM_WEN, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define igep0022_mux	NULL
#endif

void __init igep0022_init(void)
{
	mux_partition = omap_mux_get("core");

	/* Mux initialitzation for igep0022 */
	omap_mux_write_array(mux_partition, igep0022_mux);

	/* Register I2C2 bus */
	omap_register_i2c_bus(2, 400, NULL, 0);

	/* AT24C01 EEPROM with I2C interface */
	igep00x0_at24c01_init(2);

	/*
	 * NOTE: Bluetooth UART and PCM voice interface (PCM VSP) is
	 * INCOMPATIBLE with modem (disabled by default, enable with
	 * buddy.modem=yes in kernel command line
	 */
	if (igep00x0_buddy_pdata.options & IGEP00X0_BUDDY_OPT_MODEM)
		igep00x0_modem_init(IGEP2_GPIO_MODEM_ON_NOFF,
			IGEP2_GPIO_MODEM_NRESET, IGEP2_GPIO_MODEM_PWRMON);

	/* Touchscreen interface using ADS7846/TSC2046 SPI1.1 */
	omap_mux_init_gpio(IGEP2_GPIO_TSC2046_IRQ, OMAP_PIN_INPUT);
	omap_mux_init_signal("mcspi1_cs1", 0);
	igep00x0_tsc2046_init(1, 1, IGEP2_GPIO_TSC2046_IRQ, 0);

	/* CAN driver for Microchip 251x CAN Controller with SPI Interface */
	omap_mux_init_gpio(IGEP2_GPIO_MCP251X_NRESET, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_gpio(IGEP2_GPIO_MCP251X_IRQ, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("mcspi1_cs0", 0);
	igep00x0_mcp251x_init(1, 0, IGEP2_GPIO_MCP251X_IRQ,
				IGEP2_GPIO_MCP251X_NRESET);

#if defined(CONFIG_VIDEO_OMAP3) || \
	defined(CONFIG_VIDEO_OMAP3_MODULE)
	/* Register OMAP3 camera devices (tvp5151) */
	igep00x0_camera_init(&isp_pdata, IGEP2_GPIO_TVP5151_RESET,
			     IGEP2_GPIO_TVP5151_PDN);
#endif
}
