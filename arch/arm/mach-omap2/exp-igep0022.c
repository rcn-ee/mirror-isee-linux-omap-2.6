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
#include <plat/display.h>
#include <plat/mcspi.h>

#include "mux.h"

#define IGEP2_GPIO_TSC2046_PDN		155
#define IGEP2_GPIO_MODEM_ON_NOFF	141
#define IGEP2_GPIO_MODEM_NRESET		140
#define IGEP2_GPIO_MODEM_PWRMON		156
#define IGEP2_GPIO_MCP251X_IRQ		157
#define IGEP2_GPIO_TVP5151_PDN		126
#define IGEP2_GPIO_TVP5151_RESET	167

#if defined(CONFIG_TOUCHSCREEN_ADS7846) || \
	defined(CONFIG_TOUCHSCREEN_ADS7846_MODULE)

static struct omap2_mcspi_device_config tsc2046_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1,	/* 0: slave, 1: master */
};

static int ads7846_get_pendown_state(void)
{
	return !gpio_get_value(IGEP2_GPIO_TSC2046_PDN);
}

static struct ads7846_platform_data tsc2046_pdata = {
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
	.wakeup			= true,
};

static struct spi_board_info tsc2046_spi_board_info[] = {
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
		.irq			= OMAP_GPIO_IRQ(IGEP2_GPIO_TSC2046_PDN),
		.platform_data		= &tsc2046_pdata,
	},
};

static void __init igep0022_tsc2046_init(void)
{
	omap_mux_init_gpio(IGEP2_GPIO_TSC2046_PDN, OMAP_PIN_INPUT);
	omap_mux_init_signal("mcspi1_cs1", 0);

	if ((gpio_request(IGEP2_GPIO_TSC2046_PDN, "TSC2046 IRQ") == 0)
		&& (gpio_direction_input(IGEP2_GPIO_TSC2046_PDN) == 0)) {
		gpio_export(IGEP2_GPIO_TSC2046_PDN, 0);
	} else {
		pr_err("IGEP: Could not obtain gpio GPIO_TSC2046_PDN\n");
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
	 * PCM/codec/TDM/Bluetooth interface modes cannot be changed on the fly,
	 * so we need to set the value 0x04 to TWL4030_REG_VOICE_IF at file
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

	if ((gpio_request(IGEP2_GPIO_MODEM_ON_NOFF, "GPIO_MODEM_ON_NOFF") == 0)
		&& (gpio_direction_output(IGEP2_GPIO_MODEM_ON_NOFF, 1) == 0)) {
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
	.page_size	= 8,
};

static struct i2c_board_info at24c01_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("24c01", (0xa0 >> 1)),
		.platform_data = &at24c01_pdata,
	},
};

static inline void igep0022_at24c01_init(void)
{
	i2c_register_board_info(2, at24c01_i2c_board_info,
				ARRAY_SIZE(at24c01_i2c_board_info));
}

#else
static inline void igep0022_at24c01_init(void) {}
#endif

#if defined(CONFIG_CAN_MCP251X) || \
	defined(CONFIG_CAN_MCP251X_MODULE)

static struct mcp251x_platform_data mcp251x_pdata = {
	.oscillator_frequency	= 20*1000*1000,
	.model			= CAN_MCP251X_MCP2515,
	.irq_flags		= IRQF_TRIGGER_RISING,
};

static struct spi_board_info mcp251x_spi_board_info[] = {
	{
		.modalias	= "mcp2515",
		.bus_num	= 1,
		.chip_select	= 0,
		.max_speed_hz	= 10*1000*1000,
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

#if defined(CONFIG_VIDEO_TVP5150) || \
	defined(CONFIG_VIDEO_TVP5150_MODULE)

static struct i2c_board_info tvp5151_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("tvp5150", (0xb8 >> 1)),
	},
};

static inline void igep0022_tvp5151_init(void)
{
	i2c_register_board_info(2, tvp5151_i2c_board_info,
				ARRAY_SIZE(tvp5151_i2c_board_info));
}

#else
static inline void igep0022_tvp5151_init(void) {}
#endif

static inline void igep2_i2c2_init(void)
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

	if ((gpio_request(IGEP2_GPIO_TVP5151_PDN, "GPIO_TVP5151_PDN") == 0) &&
		(gpio_direction_output(IGEP2_GPIO_TVP5151_PDN, 1) == 0)) {
		gpio_export(IGEP2_GPIO_TVP5151_PDN, 0);
		gpio_set_value(IGEP2_GPIO_TVP5151_PDN, 1);
	} else
		pr_warning("IGEP: Could not obtain gpio GPIO_TVP5151_PDN\n");

	if ((gpio_request(IGEP2_GPIO_TVP5151_RESET, "GPIO_TVP5151_RESET") == 0)
		&& (gpio_direction_output(IGEP2_GPIO_TVP5151_RESET, 1) == 0)) {
		gpio_export(IGEP2_GPIO_TVP5151_RESET, 0);
		gpio_set_value(IGEP2_GPIO_TVP5151_RESET, 1);
	} else
		pr_warning("IGEP: Could not obtain gpio GPIO_TVP5151_RESET\n");

	omap_register_i2c_bus(2, 400, NULL, 0);
}

void __init igep0022_init(void)
{
	/* Register I2C2 bus */
	igep2_i2c2_init();

	/*
	 * To use UART1 as RS232 port instead of RS485 we need configure
	 * UART1_RTS and UART1_CTS pins in safe mode. To set the RS485
	 * comment next to lines.
	 */
	omap_mux_init_signal("uart1_rts.safe_mode", 0);
	omap_mux_init_signal("uart1_cts.safe_mode", 0);

	/* AT24C01 EEPROM with I2C interface */
	igep0022_at24c01_init();

	/* Touchscreen interface using ADS7846/TSC2046 */
	igep0022_tsc2046_init();

	/* GPIO's for GE865 modem interface
	igep0022_modem_init();*/

	/* CAN driver for Microchip 251x CAN Controller with SPI Interface */
	igep0022_mcp251x_init();

	/* TVP5151 video decoder driver with I2C interface */
	igep0022_tvp5151_init();
}
