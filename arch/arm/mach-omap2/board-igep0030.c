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
#include <linux/leds.h>

#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>
#include <linux/mmc/host.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/gpmc.h>
#include <plat/usb.h>
#include <plat/onenand.h>

#include "board-igep00x0.h"
#include "mux.h"
#include "hsmmc.h"
#include "twl-common.h"

#include <linux/wl12xx.h>

#define GPIO_LED_D210_RED	16
#define GPIO_LED_D440_GREENRG	168
#define GPIO_WIFI_NPD		138
#define GPIO_WIFI_NRESET	139
#define GPIO_BT_NRESET		137

#define IGEP3_RD_GPIO_LED_D440_GREEN	54
#define IGEP3_RD_GPIO_LED_D440_RED	53
#define IGEP3_RD_GPIO_USBH_NRESET	183
#define IGEP3_RE_GPIO_USBH_NRESET	54

#define IGEP3_RG_GPIO_WL_EN  139
#define IGEP3_RG_GPIO_BT_EN  137
#define IGEP3_RG_GPIO_W_IRQ  136

/*
 * IGEP3 Hardware Revision
 *
 * Revision D is only assembled with OMAP35x
 * Revision E & G is only assembled with DM37xx
 *
 */

/*
 * IGEP3 Hardware Revision Table
 *
 *  --------------------------------------------------------------------------
 * | Id. | Hw Rev.            | HW0 (16) | WIFI_NPD | WIFI_NRESET | BT_NRESET |
 *  --------------------------------------------------------------------------
 * |  0  | A/B/C/D            |   LOW    |  gpio138 |   gpio139  |  gpio137   |
 * |     |                    ------------------------------------------------
 * |     |                    | HW1 (126)| HW2 (127)| HW3 (128)  | HW4 (129)  |
 * |     |                    ------------------------------------------------
 * |     |                    |   LOW     |   NC    |   NC       |   NC       |
 *  --------------------------------------------------------------------------
 * | Id. | Hw Rev.            | HW0 (16) | WIFI_NPD | WIFI_NRESET | BT_NRESET |
 *  --------------------------------------------------------------------------
 * |  1  | E                  |   LOW    |  gpio138 |   gpio139  |  gpio137   |
 * |     |                    ------------------------------------------------
 * |     |                    | HW1 (126)| HW2 (127)| HW3 (128)  | HW4 (129)  |
 * |     |                    ------------------------------------------------
 * |     |                    |   NC     |   LOW    |   NC       |   NC       |
 *  --------------------------------------------------------------------------
 * | Id. | Hw Rev.            | HW0 (16) | WIFI_IRQ | WIFI_NRESET | BT_NRESET |
 *  --------------------------------------------------------------------------
 * |  1  | G                  |   LOW    |  gpio136 |   gpio139  |  gpio137   |
 * |     |                    ------------------------------------------------
 * |     |                    | HW1 (126)| HW2 (127)| HW3 (128)  | HW4 (129)  |
 * |     |                    ------------------------------------------------
 * |     |                    |   NC     |   NC     |   NC       |   LOW      |
 *  --------------------------------------------------------------------------
 */

#define IGEP3_BOARD_HWREV_D	0xD
#define IGEP3_BOARD_HWREV_E	0xE

static u8 hwrev;

static void __init igep0030_get_revision(void)
{
	if (cpu_is_omap3630()) {
		pr_info("IGEP: Hardware Rev. E (or G)\n");
		hwrev = IGEP3_BOARD_HWREV_E;
	} else {
		pr_info("IGEP: Hardware Rev. D\n");
		hwrev = IGEP3_BOARD_HWREV_D;
	}
}

static struct omap2_hsmmc_info mmc[] = {
	[0] = {
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
#if defined(CONFIG_LIBERTAS_SDIO) || defined(CONFIG_LIBERTAS_SDIO_MODULE)
	[1] = {
		.mmc		= 2,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
#endif

#if defined(CONFIG_WL12XX_PLATFORM_DATA) && !defined(CONFIG_LIBERTAS_SDIO_MODULE)
	[1] = {
		.mmc            = 2,
		.caps           = MMC_CAP_4_BIT_DATA | MMC_CAP_POWER_OFF_CARD,
		.gpio_cd        = -EINVAL,
		.gpio_wp        = -EINVAL,
		.ocr_mask       = MMC_VDD_165_195,
		.nonremovable   = true,
	},
#endif
	{}      /* Terminator */
};

#if defined(CONFIG_WL12XX_PLATFORM_DATA) && !defined(CONFIG_LIBERTAS_SDIO_MODULE)
static struct wl12xx_platform_data wl12xx __initdata;
static void __init __used legacy_init_wl12xx(unsigned ref_clock,
                                            unsigned tcxo_clock,
                                            int gpio){
	int res;

	wl12xx.board_ref_clock = ref_clock;
	wl12xx.board_tcxo_clock = tcxo_clock;
	wl12xx.irq = gpio_to_irq(gpio);

	res = wl12xx_set_platform_data(&wl12xx);

	if (res) {
		pr_err("error setting wl12xx data: %d\n", res);
		return;
	}
}

static void __init ti_wl12xx_wlan_init(void)
{
	struct device *dev;
	struct omap_mmc_platform_data *pdata;

	/* Set up the WLAN_EN and WLAN_IRQ muxes */
	omap_mux_init_gpio(IGEP3_RG_GPIO_WL_EN, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(IGEP3_RG_GPIO_W_IRQ, OMAP_PIN_INPUT);

	/*
	* The WLAN_EN gpio has to be toggled without using a fixed regulator,
	* as the omap_hsmmc does not enable/disable regulators on the TI814X.
	*/

	if ((gpio_request(IGEP3_RG_GPIO_WL_EN, "WL EN") == 0) &&
		 (gpio_direction_output(IGEP3_RG_GPIO_WL_EN, 1) == 0)){
		gpio_export(IGEP3_RG_GPIO_WL_EN, 0);
		gpio_set_value(IGEP3_RG_GPIO_WL_EN, 0);
		udelay(10);
		gpio_set_value(IGEP3_RG_GPIO_WL_EN, 1);
	} else
		pr_warning("IGEP3: Could not obtain gpio WL EN\n");

	omap_mux_init_gpio(IGEP3_RG_GPIO_BT_EN, OMAP_PIN_OUTPUT);

	if ((gpio_request(IGEP3_RG_GPIO_BT_EN, "BT EN") == 0) &&
		 (gpio_direction_output(IGEP3_RG_GPIO_BT_EN, 1) == 0)) {
		gpio_export(IGEP3_RG_GPIO_BT_EN, 0);
		gpio_set_value(IGEP3_RG_GPIO_BT_EN, 0);
		udelay(10);
		gpio_set_value(IGEP3_RG_GPIO_BT_EN, 1);
	} else
		pr_warning("IGEP3: Could not obtain gpio BT EN\n");

	omap_mux_init_gpio(IGEP3_RG_GPIO_W_IRQ, OMAP_PIN_INPUT);

	if ((gpio_request_one(IGEP3_RG_GPIO_W_IRQ, GPIOF_IN, "W IRQ") == 0) &&
		 (gpio_direction_output(IGEP3_RG_GPIO_W_IRQ, 0) == 0)) {
		gpio_export(IGEP3_RG_GPIO_W_IRQ, 0);
		gpio_set_value(IGEP3_RG_GPIO_W_IRQ, 1);
		udelay(10);
		gpio_set_value(IGEP3_RG_GPIO_W_IRQ, 0);
		legacy_init_wl12xx(WL12XX_REFCLOCK_38_XTAL, WL12XX_TCXOCLOCK_26,
		 IGEP3_RG_GPIO_W_IRQ);
	} else
		pr_warning("IGEP3: Could not obtain gpio W IRQ\n");

	/*
	* Set our set_power callback function which will be called from
	* set_ios. This is requireq since, unlike other omap2+ platforms, a
	* no-op set_power function is registered. Thus, we cannot use a fixed
	* regulator, as it will never be toggled.
	* Moreover, even if this was not the case, we're on mmc0, for which
	* omap_hsmmc' set_power functions do not toggle any regulators.
	*/
	dev = mmc[1].dev;
	if (!dev) {
		pr_err("wl12xx mmc device initialization failed\n");
		return;
	}
	pdata = dev->platform_data;
	if (!pdata) {
		pr_err("Platform data of wl12xx device not set\n");
		return;
	}
}

static void __init ti_wl12xx_init(void)
{
	ti_wl12xx_wlan_init();
}

#else /* CONFIG_WL12XX_PLATFORM_DATA */

static void __init ti_wl12xx_init(void) { }

#endif

static struct gpio_led gpio_led_data[] = {
	[0] = {
		.name = "d440:red",
		.gpio = -EINVAL, /* gets replaced */
		.default_trigger = "default-off",
		.active_low = true,
	},
	[1] = {
		.name = "d440:green",
		.gpio = -EINVAL, /* gets replaced */
		.default_trigger = "default-off",
		.active_low = true,
	},
	[2] = {
		.name = "d210:red",
		.gpio = GPIO_LED_D210_RED,
		.default_trigger = "default-off",
		.active_low = true,
	},
	[3] = {
		.name = "d210:green",
		.default_trigger = "default-on",
		.gpio = -EINVAL, /* gets replaced */
		.active_low = true,
	},
	[4] = {
		.name = "d440:greenrg",
		.default_trigger = "default-off",
		.gpio = GPIO_LED_D440_GREENRG,
		.active_low = false,
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

	/* REVISIT: need ehci-omap hooks for external VBUS
	 * power switch and overcurrent detect
	 */
	gpio_request(gpio + 1, "EHCI NOC");
	gpio_direction_input(gpio + 1);

	if (hwrev == IGEP3_BOARD_HWREV_D) {
		gpio_led_data[0].gpio = IGEP3_RD_GPIO_LED_D440_RED;
		gpio_led_data[1].gpio = IGEP3_RD_GPIO_LED_D440_GREEN;
	} else {
		/* Hardware Rev. E (or G)*/
		/* TWL4030_GPIO_MAX + 0 == ledA (out, active low LED) */
		gpio_led_data[0].gpio = gpio + TWL4030_GPIO_MAX + 0;
		/* gpio + 13 == ledsync (out, active low LED) */
		gpio_led_data[1].gpio = gpio + 13;
	}

	/* TWL4030_GPIO_MAX + 1 == ledB (out, active low LED) */
	gpio_led_data[3].gpio = gpio + TWL4030_GPIO_MAX + 1;

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
	.reset_gpio_port[1] = -EINVAL, /* gets replaced */
	.reset_gpio_port[2] = -EINVAL,
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	OMAP3_MUX(I2C2_SDA, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
	/* McBSP 2 */
	OMAP3_MUX(MCBSP2_FSX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP2_CLKX, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP2_DR, OMAP_MUX_MODE0 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP2_DX, OMAP_MUX_MODE0 | OMAP_PIN_OUTPUT),
	/* Serial ports */
	OMAP3_MUX(UART2_RX, OMAP_MUX_MODE7 | OMAP_PIN_INPUT),
	OMAP3_MUX(UART2_TX, OMAP_MUX_MODE7 | OMAP_PIN_INPUT),
	OMAP3_MUX(MCBSP3_CLKX, OMAP_MUX_MODE1 | OMAP_PIN_OUTPUT),
	OMAP3_MUX(MCBSP3_FSX, OMAP_MUX_MODE1 | OMAP_PIN_INPUT),
	/* GPIO leds */
	OMAP3_MUX(GPMC_NCS2, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(GPMC_NCS3, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(ETK_D2, OMAP_MUX_MODE4 | OMAP_PIN_INPUT_PULLUP),
	OMAP3_MUX(I2C2_SCL, OMAP_MUX_MODE4 | OMAP_PIN_OUTPUT),
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
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define board_mux	NULL
#endif

/* Expansion board: BASE0010 */
extern void __init base0010_init(struct twl4030_platform_data *pdata);
/* Expansion board: ILMS0015 */
extern void __init ilms0015_init(void);

static void __init igep0030_init(void)
{
	int opt;

	/* Board initialitzations */
	/* - Mux initialization */
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	/* - Ensure msecure is mux'd to be able to set the RTC. */
	omap_mux_init_signal("sys_drm_msecure", OMAP_PIN_OFF_OUTPUT_HIGH);
	/* - Get IGEP0030 Hardware Revision */
	igep0030_get_revision();
	/* - Register serial devices */
	omap_serial_init();

	/* Expansion board initialitzations */
	/* - BASE0010 (adds twl4030_pdata)  */
	/* - ILMS0015 */
	if (igep00x0_buddy_pdata.model == IGEP00X0_BUDDY_BASE0010)
		base0010_init(&twl4030_pdata);
	else if (igep00x0_buddy_pdata.model == IGEP00X0_BUDDY_ILMS0015)
		ilms0015_init();

	/* Add twl4030 common data */
	omap3_pmic_get_config(&twl4030_pdata, TWL_COMMON_PDATA_USB |
			TWL_COMMON_PDATA_AUDIO | TWL_COMMON_PDATA_MADC,
			TWL_COMMON_REGULATOR_VPLL2);

	igep00x0_pmic_get_config(&twl4030_pdata, 0,
			TWL_IGEP00X0_REGULATOR_VMMC1 |
			TWL_IGEP00X0_REGULATOR_VIO);

	omap_pmic_init(1, 2600, "twl4030", INT_34XX_SYS_NIRQ, &twl4030_pdata);

	/* - USB OTG & USB HOST */
	if (hwrev == IGEP3_BOARD_HWREV_D)
		ehci_pdata.reset_gpio_port[1] = IGEP3_RD_GPIO_USBH_NRESET;
	else
		/* Hardware Rev. E (or G) */
		ehci_pdata.reset_gpio_port[1] = IGEP3_RE_GPIO_USBH_NRESET;

	usb_ehci_init(&ehci_pdata);
	usb_musb_init(&igep00x0_musb_board_data);

	platform_device_register(&igep00x0_vdd33_device);

	/* Common initialitzations */
	/* - Register flash devices */
	igep00x0_flash_init();

	#if defined(CONFIG_WL12XX_PLATFORM_DATA) && !defined(CONFIG_LIBERTAS_SDIO_MODULE)
	/*
	* Wilink-8 TI WiFi moldule
	*
	* NOTE: RfKill must be dissabled in .confing
	*/
	ti_wl12xx_init();
	#else
	/*
	 * WLAN-BT combo module from MuRata with SDIO interface.
	 *
	 * NOTE: If we have an expansion board with modem enabled we need to
	 * disable the bluetooth interface as is INCOMPATIBLE
	 */
	opt = igep00x0_buddy_pdata.options & IGEP00X0_BUDDY_OPT_MODEM;
	igep00x0_wifi_bt_init(GPIO_WIFI_NPD, GPIO_WIFI_NRESET,
			 GPIO_BT_NRESET, !opt);
	#endif
}

MACHINE_START(IGEP0030, "IGEP0030 COM")
	.boot_params	= 0x80000100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_irq	= igep00x0_init_irq,
	.init_machine	= igep0030_init,
	.timer		= &omap_timer,
MACHINE_END
