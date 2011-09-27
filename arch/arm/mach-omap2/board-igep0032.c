/*
 * Copyright (C) 2010-2011 - ISEE 2007 SL
 *
 * Modified from mach-omap2/board-generic.c
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
#include <linux/interrupt.h>
#include <linux/smsc911x.h>
#include <linux/leds.h>

#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/gpmc.h>
#include <plat/usb.h>

#include "board-igep00x0.h"
#include "mux.h"
#include "hsmmc.h"
#include "twl-common.h"

#define GPIO_D442_GREEN		53
#define GPIO_USBH_NRESET	54
#define SMSC911X_CS		5
#define SMSC911X_NRESET		64
#define SMSC911X_IRQ		65

static struct omap2_hsmmc_info mmc[] = {
	[0] = {
		.mmc		= 1,
		.wires		= 4,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{}      /* Terminator */
};

static struct gpio_led gpio_led_data[] = {
	[0] = {
		.name = "d242:green",
		.default_trigger = "default-off",
		.gpio = -EINVAL, /* gets replaced */
		.active_low = true,
	},
	[1] = {
		.name = "d441:green",
		.default_trigger = "default-off",
		.gpio = -EINVAL, /* gets replaced */
		.active_low = true,
	},
	[2] = {
		.name = "d442:green",
		.default_trigger = "default-off",
		.gpio = GPIO_D442_GREEN,
		.active_low = true,
	},
};

static struct gpio_led_platform_data gpio_led_pdata = {
	.leds           = gpio_led_data,
	.num_leds       = ARRAY_SIZE(gpio_led_data),
};

static struct platform_device gpio_led_device = {
	 .name   = "leds-gpio",
	 .id     = -1,
	 .dev    = {
		 .platform_data = &gpio_led_pdata,
	},
};

static int twl4030_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	/* gpio + 0 is "mmc0_cd" (input/IRQ) */
	mmc[0].gpio_cd = gpio + 0;
	omap2_hsmmc_init(mmc);

	/* TWL4030_GPIO_MAX + 1 == ledB (out, active low LED) */
	gpio_led_data[0].gpio = gpio + TWL4030_GPIO_MAX + 1;
	/* TWL4030_GPIO_MAX + 0 == ledA (out, active low LED) */
	gpio_led_data[1].gpio = gpio + TWL4030_GPIO_MAX + 0;

	/* Register led devices */
	platform_device_register(&gpio_led_device);

	return 0;
};

static struct twl4030_gpio_platform_data twl4030_gpio_pdata = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.use_leds	= true,
	.setup		= twl4030_gpio_setup,
};

static struct twl4030_platform_data twl4030_pdata = {
	/* platform_data for children goes here */
	.gpio		= &twl4030_gpio_pdata,
};

static struct ehci_hcd_omap_platform_data ehci_pdata __initdata = {
	.port_mode[0] = EHCI_HCD_OMAP_MODE_UNKNOWN,
	.port_mode[1] = EHCI_HCD_OMAP_MODE_PHY,
	.port_mode[2] = EHCI_HCD_OMAP_MODE_UNKNOWN,

	.phy_reset = true,
	.reset_gpio_port[0] = -EINVAL,
	.reset_gpio_port[1] = GPIO_USBH_NRESET,
	.reset_gpio_port[2] = -EINVAL,
};

static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_OPEN_DRAIN,
	.flags		= SMSC911X_USE_32BIT | SMSC911X_SAVE_MAC_ADDRESS,
	.phy_interface	= PHY_INTERFACE_MODE_MII,
};

static struct resource smsc911x_resources[] = {
	{
		.name	= "smsc911x-memory",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= OMAP_GPIO_IRQ(SMSC911X_IRQ),
		.end	= OMAP_GPIO_IRQ(SMSC911X_IRQ),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data = &smsc911x_config,
	},
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	/* McBSP 2 */
	OMAP3_MUX(MCBSP2_FSX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP2_CLKX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP2_DR, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP2_DX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	/* SMSC911X Ethernet */
	OMAP3_MUX(GPMC_WAIT2, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	OMAP3_MUX(GPMC_WAIT3, OMAP_MUX_MODE4 | OMAP_PIN_INPUT),
	/* GPIO leds */
	OMAP3_MUX(GPMC_NCS2, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* USBH HOST NRESET */
	OMAP3_MUX(GPMC_NCS3, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define board_mux	NULL
#endif

/* Expansion board: BASE0010 */
extern void __init base0010_init(struct twl4030_platform_data *pdata);

static void __init igep0032_init(void)
{
	/* Board initialitzations */
	/* - Mux initialization */
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	/* - Ensure msecure is mux'd to be able to set the RTC. */
	omap_mux_init_signal("sys_drm_msecure", OMAP_PIN_OFF_OUTPUT_HIGH);

	/* - Register serial devices */
	omap_serial_init();

	/* Set up smsc911x chip */
	igep00x0_smsc911x_init(&smsc911x_device, SMSC911X_CS,
		SMSC911X_IRQ, SMSC911X_NRESET);

	/* Expansion board initialitzations */
	/* - BASE0010 Rev. B (adds twl4030_pdata) */
	if (igep00x0_buddy_pdata.model == IGEP00X0_BUDDY_BASE0010) {
		igep00x0_buddy_pdata.revision = IGEP00X0_BUDDY_HWREV_B;
		base0010_init(&twl4030_pdata);
	}

	/* Add twl4030 common data */
	omap3_pmic_get_config(&twl4030_pdata, TWL_COMMON_PDATA_USB |
			TWL_COMMON_PDATA_AUDIO | TWL_COMMON_PDATA_MADC, 0);

	igep00x0_pmic_get_config(&twl4030_pdata, 0,
			TWL_IGEP00X0_REGULATOR_VMMC1 |
			TWL_IGEP00X0_REGULATOR_VIO);

	omap_pmic_init(1, 2600, "twl4030", INT_34XX_SYS_NIRQ, &twl4030_pdata);

	usb_ehci_init(&ehci_pdata);
	usb_musb_init(&igep00x0_musb_board_data);

	/* Common initialitzations */
	/* - Register flash devices */
	igep00x0_flash_init();
}

MACHINE_START(IGEP0032, "IGEP0032 COM")
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_irq	= igep00x0_init_irq,
	.init_machine	= igep0032_init,
	.timer		= &omap_timer,
MACHINE_END
