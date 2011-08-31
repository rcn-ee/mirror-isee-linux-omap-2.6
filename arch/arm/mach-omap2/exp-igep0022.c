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

static inline void igep0022_i2c2_init(void)
{
	/*
	 * From TVP5151 datasheet Table 3-8. Reset and Power-Down Modes
	 *   PDN RESETB CONFIGURATION
	 *    0    0    Reserved (unknown state)
	 *    0    1    Powers down the decoder
	 *    1    0    Resets the decoder
	 *    1    1    Normal operation
	 *
	 * If TVP5151_PDN and TPVP5151_RESET is set to 0 the I2C2_SDA line
	 * is forced to low level and all devices connected to I2C2 stop
	 * working, this affects to EEPROM connected to the same bus. By default
	 * we should configure these pins to logical 1 (Normal operation)
	 *
	 * OMAP3530 CBB package can have GPIO126 muxed on 2 pins: mmc1_dat4 and
	 * cam_strobe.
	 */
	omap_mux_init_signal("cam_strobe.gpio_126", 0);
	omap_mux_init_gpio(IGEP2_GPIO_TVP5151_RESET, OMAP_PIN_OUTPUT);

	if ((gpio_request(IGEP2_GPIO_TVP5151_PDN, "TVP5151 PDN") == 0) &&
		(gpio_direction_output(IGEP2_GPIO_TVP5151_PDN, 0) == 0))
		gpio_export(IGEP2_GPIO_TVP5151_PDN, 0);
	else
		pr_warning("IGEP: Could not obtain gpio TVP5151 PDN\n");

	if ((gpio_request(IGEP2_GPIO_TVP5151_RESET, "TVP5151 RESET") == 0)
		&& (gpio_direction_output(IGEP2_GPIO_TVP5151_RESET, 0) == 0)) {
		gpio_export(IGEP2_GPIO_TVP5151_RESET, 0);
		/* Initialize TVP5151 power up sequence */
		udelay(10);
		gpio_set_value(IGEP2_GPIO_TVP5151_PDN, 1);
		udelay(10);
                gpio_set_value(IGEP2_GPIO_TVP5151_RESET, 1);
		udelay(200);
	} else
		pr_warning("IGEP: Could not obtain gpio TVP5151 RESET\n");

	omap_register_i2c_bus(2, 400, NULL, 0);
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux igep0022_mux[] __initdata = {
	/* McSPI 1 */
	OMAP3_MUX(MCSPI1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SIMO, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SOMI, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define igep0022_mux	NULL
#endif

void __init igep0022_init(void)
{
	/* Mux initialitzation for igep0022 */
	omap_mux_write_array(igep0022_mux);

	/* Register I2C2 bus */
	igep0022_i2c2_init();

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

	/* Register OMAP3 camera devices (tvp5151) */
	igep00x0_camera_init();
}
