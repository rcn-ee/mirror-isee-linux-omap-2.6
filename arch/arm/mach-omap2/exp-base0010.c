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
#include <linux/smsc911x.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/spi/spi.h>
#include <linux/i2c/twl.h>
#include <linux/mfd/twl4030-codec.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/gpmc.h>
#include <plat/mcspi.h>

#include "board-igep00x0.h"
#include "mux.h"

/* SMSC911X Ethernet controller */
#define IGEP3_SMSC911X0_CS       	4
#define IGEP3_SMSC911X0_IRQ		52
#define IGEP3_SMSC911X0_NRESET		42
#define IGEP3_SMSC911X1_CS       	6
#define IGEP3_SMSC911X1_IRQ		41
#define IGEP3_SMSC911X1_NRESET		43
/* SMSC2514B 4-port USB HUB */
#define IGEP3_GPIO_USBHUB_NRESET  	23
/* Display interface */
#define IGEP3_GPIO_DVI_PUP		12
#define IGEP3_GPIO_LCD_EN		186
#define IGEP3_GPIO_LCD_BKL		144
/* TSC2046 Touchscreen controller */
#define IGEP3_GPIO_TSC2046_IRQ		10
/* GE865 Modem */
#define IGEP3_GPIO_MODEM_ON_NOFF	163
#define IGEP3_GPIO_MODEM_NRESET		145
#define IGEP3_GPIO_MODEM_PWRMON		170
/* MCP251X CAN controller */
#define IGEP3_GPIO_MCP251X_IRQ		13
#define IGEP3_GPIO_MCP251X_NRESET	61
/* General Purpose IO */
#define IGEP3_GPIO_OUTPUT1		14
#define IGEP3_GPIO_OUTPUT2		21
#define IGEP3_GPIO_OUTPUT3		17
#define IGEP3_GPIO_INPUT2		18
#define IGEP3_GPIO_INPUT3		168

#if defined(CONFIG_SMSC911X) || defined(CONFIG_SMSC911X_MODULE)

static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_OPEN_DRAIN,
	.flags		= SMSC911X_USE_32BIT | SMSC911X_SAVE_MAC_ADDRESS,
	.phy_interface	= PHY_INTERFACE_MODE_MII,
};

static struct resource smsc911x0_resources[] = {
	{
		.name	= "smsc911x0-memory",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= OMAP_GPIO_IRQ(IGEP3_SMSC911X0_IRQ),
		.end	= OMAP_GPIO_IRQ(IGEP3_SMSC911X0_IRQ),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
	},
};

static struct resource smsc911x1_resources[] = {
	{
		.name	= "smsc911x1-memory",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= OMAP_GPIO_IRQ(IGEP3_SMSC911X1_IRQ),
		.end	= OMAP_GPIO_IRQ(IGEP3_SMSC911X1_IRQ),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
	},
};

static struct platform_device smsc911x0_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smsc911x0_resources),
	.resource	= smsc911x0_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

static struct platform_device smsc911x1_device = {
	.name		= "smsc911x",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(smsc911x1_resources),
	.resource	= smsc911x1_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

static inline void __init base0010_smsc911x_init(void)
{
	/* Set up first smsc911x chip */
	igep00x0_smsc911x_init(&smsc911x0_device, IGEP3_SMSC911X0_CS,
			IGEP3_SMSC911X0_IRQ, IGEP3_SMSC911X0_NRESET);

	/* Set up second smsc911x chip */
	igep00x0_smsc911x_init(&smsc911x1_device, IGEP3_SMSC911X1_CS,
			IGEP3_SMSC911X1_IRQ, IGEP3_SMSC911X1_NRESET);

}
#else
static inline void __init base0010_smsc911x_init(void) { }
#endif

static int base0010_enable_dvi(struct omap_dss_device *dssdev)
{
	gpio_direction_output(IGEP3_GPIO_DVI_PUP, 1);

	return 0;
}

static void base0010_disable_dvi(struct omap_dss_device *dssdev)
{
	gpio_direction_output(IGEP3_GPIO_DVI_PUP, 0);
}

static struct omap_dss_device *base0010_dss_devices[] = {
	&igep00x0_dvi_device,
	&igep00x0_lcd70_device,
};

static struct omap_dss_board_info base0010_dss_data = {
	.num_devices	= ARRAY_SIZE(base0010_dss_devices),
	.devices	= base0010_dss_devices,
	.default_device	= &igep00x0_dvi_device,
};

static struct platform_device base0010_dss_device = {
	.name	= "omapdss",
	.id	= -1,
	.dev	= {
		.platform_data = &base0010_dss_data,
	},
};

static inline void base0010_display_init(void)
{

	if ((gpio_request(IGEP3_GPIO_DVI_PUP, "DVI PUP") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_DVI_PUP, 1) == 0))
		gpio_export(IGEP3_GPIO_DVI_PUP, 0);
	else
		pr_err("IGEP: Could not obtain gpio DVI PUP\n");

	if ((gpio_request(IGEP3_GPIO_LCD_BKL, "LCD BKL") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_LCD_BKL, 1) == 0))
		gpio_export(IGEP3_GPIO_LCD_BKL, 0);
	else
		pr_err("IGEP: Could not obtain gpio LCD BKL\n");

	igep00x0_dvi_device.platform_enable = base0010_enable_dvi;
	igep00x0_dvi_device.platform_disable = base0010_disable_dvi;

	platform_device_register(&base0010_dss_device);
}

static inline void base0010_gpio_init(void)
{
	if ((gpio_request(IGEP3_GPIO_OUTPUT1, "GPIO OUTPUT1") == 0)
		&& (gpio_direction_output(IGEP3_GPIO_OUTPUT1, 0) == 0))
		gpio_export(IGEP3_GPIO_OUTPUT1, 0);
	else
		pr_err("IGEP: Could not obtain gpio OUTPUT1\n");

	if ((gpio_request(IGEP3_GPIO_OUTPUT2, "GPIO OUTPUT2") == 0)
		&& (gpio_direction_output(IGEP3_GPIO_OUTPUT2, 0) == 0))
		gpio_export(IGEP3_GPIO_OUTPUT2, 0);
	else
		pr_err("IGEP: Could not obtain gpio OUTPUT2\n");

	if ((gpio_request(IGEP3_GPIO_OUTPUT3, "GPIO OUTPUT3") == 0)
		&& (gpio_direction_output(IGEP3_GPIO_OUTPUT3, 0) == 0))
		gpio_export(IGEP3_GPIO_OUTPUT3, 0);
	else
		pr_err("IGEP: Could not obtain gpio OUTPUT3\n");

	if (gpio_request(IGEP3_GPIO_INPUT2, "GPIO INPUT2")
	    || gpio_direction_input(IGEP3_GPIO_INPUT2))
		pr_err("IGEP: Could not obtain gpio INPUT2\n");

	if (gpio_request(IGEP3_GPIO_INPUT3, "GPIO INPUT3")
	    || gpio_direction_input(IGEP3_GPIO_INPUT3))
		pr_err("IGEP: Could not obtain gpio INPUT3\n");
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux base0010_mux[] __initdata = {
	/* SMSC911X Ethernet controller */
	OMAP3_MUX(GPMC_NCS4, OMAP_MUX_MODE0),
	OMAP3_MUX(GPMC_NCS1, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(GPMC_A9, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(GPMC_NCS6, OMAP_MUX_MODE0),
	OMAP3_MUX(GPMC_A8, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(GPMC_A10, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* Display Sub System */
	OMAP3_MUX(DSS_PCLK, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_HSYNC, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_VSYNC, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_ACBIAS, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA0, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA1, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA2, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA3, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA4, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA5, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA6, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA7, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA8, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA9, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA10, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA11, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA12, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA13, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA14, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA15, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA16, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA17, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA18, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA19, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA20, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA21, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA22, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(DSS_DATA23, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(ETK_CLK, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(SYS_CLKOUT2, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART2_CTS, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* Serial ports */
	OMAP3_MUX(UART1_TX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART1_RX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(UART1_CTS, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART1_RTS, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART2_TX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(UART2_RX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	/* SMSC2514B 4-port USB HUB */
	OMAP3_MUX(ETK_D9, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* McSPI 1 */
	OMAP3_MUX(MCSPI1_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SIMO, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCSPI1_SOMI, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	/* TSC2046 Touchscreen controller */
	OMAP3_MUX(MCSPI1_CS1, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(SYS_CLKOUT2, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	/* MCP251x CAN controller */
	OMAP3_MUX(MCSPI1_CS0, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(ETK_CTL, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(GPMC_NBE1, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* General Purpose IO */
	OMAP3_MUX(ETK_D0, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(ETK_D3, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(ETK_D7, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(ETK_D4, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(I2C2_SCL, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define base0010_mux	NULL
#endif

void __init base0010_init(struct twl4030_platform_data *pdata)
{
	/* Mux initialitzation for base0010 */
	omap_mux_write_array(base0010_mux);

	/* Add twl4030 platform data */
	pdata->vpll2 = &twl4030_vpll2;

	/* Enable regulator that powers LCD, LCD backlight and Touchscreen */
	if ((gpio_request(IGEP3_GPIO_LCD_EN, "LCD EN") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_LCD_EN, 0) == 0))
		gpio_export(IGEP3_GPIO_LCD_EN, 0);
	else
		pr_err("IGEP: Could not obtain gpio LCD EN\n");

	/* Register I2C3 bus */
	omap_register_i2c_bus(3, 100, NULL, 0);

	/* Add platform devices */

	/* Display initialitzation */
	base0010_display_init();

	/* Ethernet with SMSC9221 LAN Controller */
	base0010_smsc911x_init();

	/* AT24C01 EEPROM with I2C interface */
	igep00x0_at24c01_init(3);

	/* 4-Port USB HUB */
	if ((gpio_request(IGEP3_GPIO_USBHUB_NRESET, "USBHUB NRESET") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_USBHUB_NRESET, 0) == 0))
		gpio_export(IGEP3_GPIO_USBHUB_NRESET, 0);
	else
		pr_warning("IGEP: Could not obtain gpio USBHUB NRESET\n");

	/*
	 * NOTE: Bluetooth UART and PCM voice interface (PCM VSP) is
	 * INCOMPATIBLE with modem (disabled by default)
	 *
	igep00x0_modem_init(IGEP2_GPIO_MODEM_ON_NOFF, IGEP2_GPIO_MODEM_NRESET,
				IGEP2_GPIO_MODEM_PWRMON);
	*/

	/* Touchscreen interface using ADS7846/TSC2046 SPI1.0 */
	omap_mux_init_gpio(IGEP3_GPIO_TSC2046_IRQ, OMAP_PIN_INPUT);
	omap_mux_init_signal("mcspi1_cs1", 0);
	igep00x0_tsc2046_init(1, 0, IGEP3_GPIO_TSC2046_IRQ, 0);

	/* CAN driver for Microchip 251x CAN Controller with SPI Interface */
	omap_mux_init_gpio(IGEP3_GPIO_MCP251X_IRQ, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("mcspi1_cs0", 0);
	igep00x0_mcp251x_init(1, 0, IGEP3_GPIO_MCP251X_IRQ);

	/* General Purpose IO */
	base0010_gpio_init();
}
