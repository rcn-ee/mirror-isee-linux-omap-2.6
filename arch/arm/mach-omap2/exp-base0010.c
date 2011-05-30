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

#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <linux/i2c/twl.h>
#include <linux/i2c/at24.h>
#include <linux/mfd/twl4030-codec.h>
#include <linux/can/platform/mcp251x.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/gpmc.h>
#include <plat/mcspi.h>

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
#define IGEP3_GPIO_LCD_PUP		186
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

#include <linux/smsc911x.h>

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

static void __init smsc911x_init(struct platform_device *pdev,
					int cs, int irq_gpio, int nreset)
{
	unsigned long cs_mem_base;

	/* Set smsc911x chip */
	if (gpmc_cs_request(cs, SZ_16M, &cs_mem_base) < 0) {
		pr_err("IGEP: Failed request for GPMC mem for smsc911x-%d\n",
				pdev->id);
		gpmc_cs_free(cs);
		return;
	}

	pdev->resource[0].start = cs_mem_base + 0x0;
	pdev->resource[0].end   = cs_mem_base + 0xff;

	if (gpio_request(irq_gpio, "SMSC911X IRQ")
	    || gpio_direction_input(irq_gpio)) {
		pr_err("IGEP: Could not obtain gpio MCP251X_IRQ\n");
		return;
	}

	if ((gpio_request(nreset, "SMSC911X NRESET") == 0) &&
	    (gpio_direction_output(nreset, 1) == 0))
		gpio_export(nreset, 0);
	else {
		pr_err("IGEP: Could not obtain gpio NRESET for smsc911x-%d\n",
				pdev->id);
		return;
	}

	platform_device_register(pdev);
}

static inline void __init base0010_smsc911x_init(void)
{
	/* Set up first smsc911x chip */
	smsc911x_init(&smsc911x0_device, IGEP3_SMSC911X0_CS,
			IGEP3_SMSC911X0_IRQ, IGEP3_SMSC911X0_NRESET);

	/* Set up second smsc911x chip */
	smsc911x_init(&smsc911x1_device, IGEP3_SMSC911X1_CS,
			IGEP3_SMSC911X1_IRQ, IGEP3_SMSC911X1_NRESET);

}
#else
static inline void __init base0010_smsc911x_init(void) { }
#endif

static int enable_dvi(struct omap_dss_device *dssdev)
{
	gpio_direction_output(IGEP3_GPIO_DVI_PUP, 1);

	return 0;
}

static void disable_dvi(struct omap_dss_device *dssdev)
{
	gpio_direction_output(IGEP3_GPIO_DVI_PUP, 0);
}

static int enable_lcd(struct omap_dss_device *dssdev)
{
	gpio_direction_output(IGEP3_GPIO_LCD_PUP, 0);

	return 0;
}

static void disable_lcd(struct omap_dss_device *dssdev)
{
	gpio_direction_output(IGEP3_GPIO_LCD_PUP, 1);
}


static struct omap_dss_device base0010_dvi_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "dvi",
	.driver_name		= "generic_panel",
	.phy.dpi.data_lines	= 24,
	.platform_enable	= enable_dvi,
	.platform_disable	= disable_dvi,
};

/* Seiko 7.0 inch WVGA (800 x RGB x 480) TFT with Touch-Panel */
static struct omap_dss_device base0010_lcd70_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "lcd-70",
	.driver_name		= "70wvw1tz3",
	.phy.dpi.data_lines	= 24,
	.platform_enable	= enable_lcd,
	.platform_disable	= disable_lcd,
};

static struct omap_dss_device *base0010_dss_devices[] = {
	&base0010_dvi_device,
	&base0010_lcd70_device,
};

static struct omap_dss_board_info base0010_dss_data = {
	.num_devices	= ARRAY_SIZE(base0010_dss_devices),
	.devices	= base0010_dss_devices,
	.default_device	= &base0010_dvi_device,
};

static struct platform_device base0010_dss_device = {
	.name	= "omapdss",
	.id	= -1,
	.dev	= {
		.platform_data = &base0010_dss_data,
	},
};

static struct regulator_consumer_supply base0010_vpll2_supply = {
	.supply	= "vdds_dsi",
	.dev	= &base0010_dss_device.dev,
};

static struct regulator_init_data base0010_vpll2 = {
	.constraints = {
		.name			= "VDVI",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &base0010_vpll2_supply,
};

static void __init base0010_display_init(void)
{

	if ((gpio_request(IGEP3_GPIO_DVI_PUP, "DVI PUP") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_DVI_PUP, 1) == 0))
		gpio_export(IGEP3_GPIO_DVI_PUP, 0);
	else
		pr_err("IGEP: Could not obtain gpio DVI PUP\n");

	if ((gpio_request(IGEP3_GPIO_LCD_PUP, "LCD PUP") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_LCD_PUP, 1) == 0))
		gpio_export(IGEP3_GPIO_LCD_PUP, 0);
	else
		pr_err("IGEP: Could not obtain gpio LCD PUP\n");

	if ((gpio_request(IGEP3_GPIO_LCD_BKL, "LCD BKL") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_LCD_BKL, 1) == 0))
		gpio_export(IGEP3_GPIO_LCD_BKL, 0);
	else
		pr_err("IGEP: Could not obtain gpio LCD BKL\n");
}

#if defined(CONFIG_TOUCHSCREEN_ADS7846) || \
	defined(CONFIG_TOUCHSCREEN_ADS7846_MODULE)

static struct omap2_mcspi_device_config tsc2046_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1,	/* 0: slave, 1: master */
};

static int ads7846_get_pendown_state(void)
{
	return !gpio_get_value(IGEP3_GPIO_TSC2046_IRQ);
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
		.irq			= OMAP_GPIO_IRQ(IGEP3_GPIO_TSC2046_IRQ),
		.platform_data		= &tsc2046_pdata,
	},
};

static void __init base0010_tsc2046_init(void)
{
	omap_mux_init_gpio(IGEP3_GPIO_TSC2046_IRQ, OMAP_PIN_INPUT);
	omap_mux_init_signal("mcspi1_cs1", 0);

	if ((gpio_request(IGEP3_GPIO_TSC2046_IRQ, "TSC2046 IRQ") == 0)
		&& (gpio_direction_input(IGEP3_GPIO_TSC2046_IRQ) == 0))
		gpio_export(IGEP3_GPIO_TSC2046_IRQ, 0);
	else {
		pr_err("IGEP: Could not obtain gpio TSC2046 IRQ\n");
		return;
	}

	spi_register_board_info(tsc2046_spi_board_info,
				ARRAY_SIZE(tsc2046_spi_board_info));
}

#else
static inline void base0010_tsc2046_init(void) {}
#endif

static inline void __init base0010_modem_init(void)
{
	omap_mux_init_signal("uart2_rx.uart2_rx", OMAP_PIN_INPUT);
	omap_mux_init_signal("uart2_tx.uart2_tx", OMAP_PIN_OUTPUT);

	/* Modem I/O */
	omap_mux_init_gpio(IGEP3_GPIO_MODEM_ON_NOFF, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(IGEP3_GPIO_MODEM_NRESET, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(IGEP3_GPIO_MODEM_PWRMON, OMAP_PIN_INPUT);

	if ((gpio_request(IGEP3_GPIO_MODEM_ON_NOFF, "MODEM ON NOFF") == 0)
	    && (gpio_direction_output(IGEP3_GPIO_MODEM_ON_NOFF, 1) == 0)) {
		gpio_export(IGEP3_GPIO_MODEM_ON_NOFF, 0);
		gpio_set_value(IGEP3_GPIO_MODEM_ON_NOFF, 0);
	} else
		pr_warning("IGEP: Could not obtain gpio MODEM ON NOFF\n");

	if ((gpio_request(IGEP3_GPIO_MODEM_NRESET, "MODEM NRESET") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_MODEM_NRESET, 1) == 0)) {
		gpio_export(IGEP3_GPIO_MODEM_NRESET, 0);
		gpio_set_value(IGEP3_GPIO_MODEM_NRESET, 0);
	} else
		pr_warning("IGEP: Could not obtain gpio MODEM NRESET\n");

	if ((gpio_request(IGEP3_GPIO_MODEM_PWRMON, "MODEM PWRMON") == 0) &&
	    (gpio_direction_input(IGEP3_GPIO_MODEM_PWRMON) == 0))
		gpio_export(IGEP3_GPIO_MODEM_PWRMON, 0);
	else
		pr_warning("IGEP: Could not obtain gpio MODEM PWRMON\n");
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

static inline void base0010_at24c01_init(void)
{
	i2c_register_board_info(3, at24c01_i2c_board_info,
				ARRAY_SIZE(at24c01_i2c_board_info));
}

#else
static inline void base0010_at24c01_init(void) {}
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
		.irq		= OMAP_GPIO_IRQ(IGEP3_GPIO_MCP251X_IRQ),
		.mode		= SPI_MODE_0,
		.platform_data	= &mcp251x_pdata,
	},
};

static inline void base0010_mcp251x_init(void)
{
	if ((gpio_request(IGEP3_GPIO_MCP251X_NRESET, "MCP251X NRESET") == 0)
		&& (gpio_direction_output(IGEP3_GPIO_MCP251X_NRESET, 0) == 0))
		gpio_export(IGEP3_GPIO_MCP251X_NRESET, 0);
	else {
		pr_err("IGEP: Could not obtain gpio MCP251X NRESET\n");
		return;
	}

	if (gpio_request(IGEP3_GPIO_MCP251X_IRQ, "MCP251X IRQ")
	    || gpio_direction_input(IGEP3_GPIO_MCP251X_IRQ)) {
		pr_err("IGEP: Could not obtain gpio MCP251X IRQ\n");
		return;
	}

	spi_register_board_info(mcp251x_spi_board_info,
				ARRAY_SIZE(mcp251x_spi_board_info));
}

#else
static inline void base0010_mcp251x_init(void) {}
#endif

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

static struct platform_device *base0010_devices[] __initdata = {
	&base0010_dss_device,
};

void __init base0010_init(struct twl4030_platform_data *pdata)
{
	/* Mux initialitzation for base0010 */
	omap_mux_write_array(base0010_mux);

	/* Add twl4030 platform data */
	pdata->vpll2 = &base0010_vpll2;

	/* Register I2C3 bus */
	omap_register_i2c_bus(3, 100, NULL, 0);

	/* Touchscreen interface using ADS7846/TSC2046 */
	base0010_tsc2046_init();

	/* Add platform devices */
	platform_add_devices(base0010_devices, ARRAY_SIZE(base0010_devices));

	/* Display initialitzation */
	base0010_display_init();

	/* Ethernet with SMSC9221 LAN Controller */
	base0010_smsc911x_init();

	/* AT24C01 EEPROM with I2C interface */
	base0010_at24c01_init();

	/* 4-Port USB HUB */
	if ((gpio_request(IGEP3_GPIO_USBHUB_NRESET, "USBHUB NRESET") == 0) &&
	    (gpio_direction_output(IGEP3_GPIO_USBHUB_NRESET, 0) == 0))
		gpio_export(IGEP3_GPIO_USBHUB_NRESET, 0);
	else
		pr_warning("IGEP: Could not obtain gpio for USBHUB NRESET\n");

	/* GPIO's for GE865 modem interface */
	base0010_modem_init();

	/* CAN driver for Microchip 251x CAN Controller with SPI Interface */
	base0010_mcp251x_init();

	/* General Purpose IO */
	base0010_gpio_init();
}
