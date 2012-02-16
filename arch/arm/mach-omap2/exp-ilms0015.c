/*
 * Copyright (C) 2011 - ISEE 2007 SL
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
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/lis3lv02d.h>

#include <linux/spi/spi.h>

#include <asm/mach-types.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/mcspi.h>

#include "board-igep00x0.h"
#include "devices.h"
#include "mux.h"

#if defined(CONFIG_SENSORS_LIS3_SPI) || defined(CONFIG_SENSORS_LIS3_SPI_MODULE)

static struct lis3lv02d_platform_data lis3lv02d_pdata = {
};

static struct spi_board_info lis3lv02d_spi_board_info  __initdata = {
	.modalias	= "lis3lv02d_spi",
	.bus_num	= -EINVAL,
	.chip_select	= -EINVAL,
	.max_speed_hz	= 1*1000*1000,
	.irq		= -EINVAL,
	.mode		= SPI_MODE_0,
	.platform_data	= &lis3lv02d_pdata,
};

inline void __init ilms0015_lis3lv02d_init(int bus_num, int cs, int irq)
{
	struct spi_board_info *spi = &lis3lv02d_spi_board_info;

	if ((gpio_request(irq, "LIS3LV02D IRQ") == 0)
		&& (gpio_direction_input(irq) == 0))
		gpio_export(irq, 0);
	else {
		pr_err("IGEP: Could not obtain gpio LIS3LV02D IRQ\n");
		return;
	}

	spi->bus_num = bus_num;
	spi->chip_select = cs;
	spi->irq = OMAP_GPIO_IRQ(irq),

	spi_register_board_info(&lis3lv02d_spi_board_info, 1);
}
#else
inline void __init ilms0015_lis3lv02d_init(int bus_num, int cs,	int irq) { }
#endif

inline void __init ilms0015_gpio_init(void)
{
	int gpio;

	/*
	 * By default configure DSS bus as GPIO inputs
	 *   DSS_PCLK        -> GPIO66
	 *   DSS_HSYNC       -> GPIO67
	 *   DSS_VSYNC       -> GPIO68
	 *   DSS_ACBIAS      -> GPIO69
	 *   DSS_DATA[0..23] -> GPIO[70..93]
	 */
	for (gpio = 66; gpio < 94; gpio++) {
		if ((gpio_request(gpio, "GPIO") == 0)
			&& (gpio_direction_input(gpio) == 0))
			gpio_export(gpio, 1);
		else
			pr_err("IGEP: Could not obtain GPIO\n");
	}
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux ilms0015_mux[] __initdata = {
	OMAP3_MUX(MCSPI1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SIMO, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SOMI, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_CS0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP3_MUX(MCSPI1_CS1, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_CS2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN),
	OMAP3_MUX(I2C3_SCL, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(I2C3_SDA, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(UART2_TX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART2_RX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(UART2_RTS, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART2_CTS, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(UART3_TX_IRTX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART3_RX_IRRX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(UART3_RTS_SD, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART3_CTS_RCTX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_PCLK, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_HSYNC, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_VSYNC, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_ACBIAS, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA0, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA1, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA2, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA3, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA4, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA5, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA6, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA7, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA8, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA9, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA10, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA11, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA12, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA13, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA14, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA15, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA16, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA17, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA18, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA19, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA20, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA21, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA22, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(DSS_DATA23, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define ilms0015_mux	NULL
#endif

void __init ilms0015_init(void)
{
	mux_partition = omap_mux_get("core");

	/* Mux initialitzation for ilms0015 */
	omap_mux_write_array(mux_partition, ilms0015_mux);

	/* 3-axis accelerometer */
	ilms0015_lis3lv02d_init(1, 2, 174);
	/* Export some GPIO */
	ilms0015_gpio_init();
}
