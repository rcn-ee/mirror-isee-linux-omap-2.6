/*
 * Copyright (C) 2010 - ISEE 2007 SL
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
#include <linux/spi/ads7846.h>
#include <linux/i2c/twl.h>
#include <linux/i2c/at24.h>
#include <linux/mfd/twl4030-codec.h>
#include <linux/can/platform/mcp251x.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/mcspi.h>

#include "mux.h"

#define IGEP2_GPIO_TSC2046_PENDOWN	155
#define IGEP2_GPIO_MODEM_ON_NOFF	140
#define IGEP2_GPIO_MODEM_NRESET		141
#define IGEP2_GPIO_MODEM_PWRMON		156
#define IGEP2_GPIO_MCP251X_IRQ		157

#if defined(CONFIG_TOUCHSCREEN_ADS7846) || \
	defined(CONFIG_TOUCHSCREEN_ADS7846_MODULE)

static struct omap2_mcspi_device_config tsc2046_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1,	/* 0: slave, 1: master */
};

static int ads7846_get_pendown_state(void)
{
	return !gpio_get_value(IGEP2_GPIO_TSC2046_PENDOWN);
}

static struct ads7846_platform_data tsc2046_pdata __initdata = {
	.x_max			= 0x0fff,
	.y_max			= 0x0fff,
	.x_plate_ohms		= 180,
	.pressure_max		= 255,
	.debounce_max		= 10,
	.debounce_tol		= 3,
	.debounce_rep		= 1,
	.get_pendown_state	= ads7846_get_pendown_state,
	.keep_vref_on		= 1,
	.settle_delay_usecs	= 150,
	.wakeup				= true,
};

static struct spi_board_info tsc2046_spi_board_info[] __initdata = {
	{
		/*
		 * TSC2046 operates at a max freqency of 2MHz, so
		 * operate slightly below at 1.5MHz
		 */
		.modalias		= "ads7846",
		.bus_num		= 1,
		.chip_select		= 1,
		.max_speed_hz		= 1500000,
		.controller_data	= &tsc2046_mcspi_config,
		.irq			= OMAP_GPIO_IRQ(IGEP2_GPIO_TSC2046_PENDOWN),
		.platform_data		= &tsc2046_pdata,
	},
};

static void __init igep0022_tsc2046_init(void)
{
	omap_mux_init_gpio(IGEP2_GPIO_TSC2046_PENDOWN, OMAP_PIN_INPUT);
	omap_mux_init_signal("mcspi1_cs1", 0);

	if ((gpio_request(IGEP2_GPIO_TSC2046_PENDOWN, "TSC2046 IRQ") == 0)
		&& (gpio_direction_input(IGEP2_GPIO_TSC2046_PENDOWN) == 0)) {
		gpio_export(IGEP2_GPIO_TSC2046_PENDOWN, 0);
	} else {
		pr_err("IGEP: Could not obtain gpio GPIO_TSC2046_PENDOWN\n");
        return;
    }

	spi_register_board_info(tsc2046_spi_board_info,
				ARRAY_SIZE(tsc2046_spi_board_info));
}
#else
static inline void igep0022_tsc2046_init(void) {}
#endif

static inline void __init igep0022_modem_init(void)
{
	/*
	 * We need to set the TWL4030 PCM voice interface output pins to high
	 * impedance, otherwise this module forces the level of mcbsp3_fsx and
	 * mcbsp3_clkx pins.
	 *
	 * PCM/codec/TDM/Bluetooth interface modes cannot be changed on the fly, so
	 * we need to set the value 0x04 to TWL4030_REG_VOICE_IF at file
	 * sound/soc/codecs/twl4030.c
	 * -       0x00,  REG_VOICE_IF  (0xF) 
	 * +       0x04,  REG_VOICE_IF  (0xF) 
	 *
	 * NOTE: PCM voice interface (PCM VSP) is INCOMPATIBLE with modem.
	 */

	/*
	 * OMAP3530 CBB package can have UART2 muxed on 2 pins: uart2_* and
	 * mcbsp3_*. The modem uses UART2 on mcbsp3 pins so we need to configure
	 * the MUX properly.
	 */
	omap_mux_init_signal("uart2_rx.safe_mode", 0);
	omap_mux_init_signal("uart2_tx.safe_mode", 0);

	/* Configure pins mcbsp3_fsx and mcbsp3_clkx as UART2 */
	omap_mux_init_signal("mcbsp3_fsx.uart2_rx", OMAP_PIN_INPUT);
	omap_mux_init_signal("mcbsp3_clkx.uart2_tx", OMAP_PIN_OUTPUT);

	/* Modem I/O */
	omap_mux_init_gpio(IGEP2_GPIO_MODEM_ON_NOFF, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(IGEP2_GPIO_MODEM_NRESET, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(IGEP2_GPIO_MODEM_PWRMON, OMAP_PIN_INPUT);

	if ((gpio_request(IGEP2_GPIO_MODEM_ON_NOFF, "GPIO_MODEM_ON_NOFF") == 0) &&
		(gpio_direction_output(IGEP2_GPIO_MODEM_ON_NOFF, 1) == 0)) {
		gpio_export(IGEP2_GPIO_MODEM_ON_NOFF, 0);
		gpio_set_value(IGEP2_GPIO_MODEM_ON_NOFF, 0);
	} else
		pr_warning("IGEP: Could not obtain gpio GPIO_MODEM_ON_NOFF\n");

	if ((gpio_request(IGEP2_GPIO_MODEM_NRESET, "GPIO_MODEM_NRESET") == 0) &&
		(gpio_direction_output(IGEP2_GPIO_MODEM_NRESET, 1) == 0)) {
		gpio_export(IGEP2_GPIO_MODEM_NRESET, 0);
		gpio_set_value(IGEP2_GPIO_MODEM_NRESET, 0);
	} else
		pr_warning("IGEP: Could not obtain gpio GPIO_MODEM_NRESET\n");

	if ((gpio_request(IGEP2_GPIO_MODEM_PWRMON, "GPIO_MODEM_PWRMON") == 0) &&
		(gpio_direction_input(IGEP2_GPIO_MODEM_PWRMON) == 0)) {
		gpio_export(IGEP2_GPIO_MODEM_PWRMON, 0);
	} else
		pr_warning("IGEP: Could not obtain gpio GPIO_MODEM_PWRMON\n");
}

#if defined(CONFIG_EEPROM_AT24) || \
	defined(CONFIG_EEPROM_AT24_MODULE)

static struct at24_platform_data at24c01_pdata __initdata = {
	.byte_len	= SZ_1K / 8,
	.page_size	= 16,
};

static struct i2c_board_info at24c01_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("24c01", 0x50),
		.platform_data = &at24c01_pdata,
	},
};

static inline void igep0022_at24c01_init(void)
{
	omap_register_i2c_bus(2, 400, at24c01_i2c_board_info,
		ARRAY_SIZE(at24c01_i2c_board_info));
}

#else
static inline void igep0022_at24c01_init(void) {}
#endif

#if defined(CONFIG_CAN_MCP251X) || \
	defined(CONFIG_CAN_MCP251X_MODULE)

static struct mcp251x_platform_data mcp251x_pdata __initdata = {
	.oscillator_frequency = 20*1000*1000,
	.model = CAN_MCP251X_MCP2515,
};

static struct spi_board_info mcp251x_spi_board_info[] __initdata = {
	{
		.modalias	= "mcp2515",
		.bus_num	= 1,
		.chip_select	= 0,
		.max_speed_hz	= 1*1000*1000,
		.irq		= OMAP_GPIO_IRQ(IGEP2_GPIO_MCP251X_IRQ),
		.mode		= SPI_MODE_0,
		.platform_data	= &mcp251x_pdata,
	},
};

static inline void igep0022_mcp251x_init(void)
{
	omap_mux_init_gpio(IGEP2_GPIO_MCP251X_IRQ, OMAP_PIN_INPUT_PULLUP);

	if ((gpio_request(IGEP2_GPIO_MCP251X_IRQ, "MCP251X IRQ") == 0)
		&& (gpio_direction_input(IGEP2_GPIO_MCP251X_IRQ) == 0)) {
		gpio_export(IGEP2_GPIO_MCP251X_IRQ, 0);
	} else {
		pr_err("IGEP: Could not obtain gpio MCP251X_IRQ\n");
		return;
	}

	spi_register_board_info(mcp251x_spi_board_info,
				ARRAY_SIZE(mcp251x_spi_board_info));
}

#else
static inline void igep0022_mcp251x_init(void) {}
#endif

void __init igep0022_init(void)
{
	/* TODO: autodetect this board */

	/* Touchscreen interface using ADS7846/TSC2046 */
	igep0022_tsc2046_init();
	/* GPIO's for GE865 modem interface */
	igep0022_modem_init();
	/* AT24C01 EEPROM with I2C interface */
	igep0022_at24c01_init();
	/* CAN bus driver for Microchip 251x CAN Controller with SPI Interface */
	igep0022_mcp251x_init();
}

