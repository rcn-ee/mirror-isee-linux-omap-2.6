/*
 * Copyright (C) 2013, ISEE 2007 SL - http://www.isee.biz/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/ethtool.h>
#include <linux/mfd/tps65910.h>
#include <linux/mfd/ti_tscadc.h>
#include <linux/reboot.h>
#include <linux/pwm/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/rtc/rtc-omap.h>
#include <linux/opp.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>

/* LCD controller is similar to DA850 */
#include <video/da8xx-fb.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/asp.h>

#include <plat/omap_device.h>
#include <plat/omap-pm.h>
#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/usb.h>
#include <plat/mmc.h>
#include <plat/emif.h>
#include <plat/nand.h>
#include <plat/lcdc.h>

#include "board-flash.h"
#include "cpuidle33xx.h"
#include "mux.h"
#include "devices.h"
#include "hsmmc.h"

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#include "common.h"

static struct omap2_hsmmc_info am335x_mmc[] __initdata = {
	{
		.mmc            = 1,
		.caps           = MMC_CAP_4_BIT_DATA,
		.gpio_cd	= GPIO_TO_PIN(3, 14),
		.gpio_wp	= -EINVAL,
		.ocr_mask       = MMC_VDD_32_33 | MMC_VDD_33_34, /* 3V3 */
	},
/* --- Expansion --- */
	{
		.mmc            = 2,
		.caps           = MMC_CAP_4_BIT_DATA,
		.gpio_cd	= GPIO_TO_PIN(3, 15),
		.gpio_wp	= -EINVAL,
		.ocr_mask       = MMC_VDD_32_33 | MMC_VDD_33_34, /* 3V3 */
	},
	{}      /* Terminator */
};


#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	/* I2C0 */
	AM33XX_MUX(I2C0_SDA, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT),
	AM33XX_MUX(I2C0_SCL, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT),
	/* NAND */
	AM33XX_MUX(GPMC_AD0, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD1, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD2, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD3, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD4, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD5, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD6, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_AD7, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_WAIT0, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_WPN, OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_CSN0, OMAP_MUX_MODE0 | AM33XX_PULL_DISA),
	AM33XX_MUX(GPMC_ADVN_ALE,  OMAP_MUX_MODE0 | AM33XX_PULL_DISA),
	AM33XX_MUX(GPMC_OEN_REN, OMAP_MUX_MODE0 | AM33XX_PULL_DISA),
	AM33XX_MUX(GPMC_WEN, OMAP_MUX_MODE0 | AM33XX_PULL_DISA),
	AM33XX_MUX(GPMC_BEN0_CLE, OMAP_MUX_MODE0 | AM33XX_PULL_DISA),
	/* MMC0 */
	AM33XX_MUX(MMC0_DAT3, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MMC0_DAT2, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MMC0_DAT1, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MMC0_DAT0, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MMC0_CLK, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MMC0_CMD, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MCASP0_ACLKX, OMAP_MUX_MODE5 | AM33XX_PIN_INPUT_PULLUP),
	/* Ethernet RMII1 */
	AM33XX_MUX(MII1_CRS, OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MII1_RXERR, OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MII1_TXEN, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(MII1_TXD1, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(MII1_TXD0, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(MII1_RXD1, OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MII1_RXD0, OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MII1_REFCLK, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MDIO_DATA, OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MDIO_CLK, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT_PULLUP),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define	board_mux	NULL
#endif

/* module pin mux structure */
struct pinmux_config {
	const char *string_name; /* signal name format */
	int val; /* Options for the mux register value */
};

/* NAND partitions */
static struct mtd_partition board_nand_partitions[] = {
	{
		.name           = "SPL",
		.offset         = 0,			/* Offset = 0x0 */
		.size           = SZ_512K,
	},
	{
		.name           = "U-Boot",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x80000 */
		.size           = 15 * SZ_128K,
	},
	{
		.name           = "U-Boot Env",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x260000 */
		.size           = 1 * SZ_128K,
	},
	{
		.name           = "Kernel",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x280000 */
		.size           = 40 * SZ_128K,
	},
	{
		.name           = "File System",
		.offset         = MTDPART_OFS_APPEND,   /* Offset = 0x780000 */
		.size           = MTDPART_SIZ_FULL,
	},
};

static struct omap_nand_platform_data board_nand_pdata = {
	.parts		= board_nand_partitions,
	.nr_parts 	= ARRAY_SIZE(board_nand_partitions),
	.cs		= 0,
	.ecc_opt	= OMAP_ECC_BCH8_CODE_HW,
	.elm_used	= true,

};

static struct gpmc_devices_info board_gpmc_devices[2] = {
	{
		.pdata	= &board_nand_pdata,
		.flag	= GPMC_DEVICE_NAND,
	},
	{ NULL, 0 },
};

static void __init board_nand_init(void)
{
	omap_init_gpmc(board_gpmc_devices, sizeof(board_gpmc_devices));
	omap_init_elm();
}

static struct regulator_init_data am335x_dummy = {
	.constraints.always_on	= true,
};

static struct regulator_consumer_supply am335x_vdd1_supply[] = {
	REGULATOR_SUPPLY("vdd_mpu", NULL),
};

static struct regulator_init_data am335x_vdd1 = {
	.constraints = {
		.min_uV			= 600000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE,
		.always_on		= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(am335x_vdd1_supply),
	.consumer_supplies	= am335x_vdd1_supply,
};

static struct regulator_consumer_supply am335x_vdd2_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL),
};

static struct regulator_init_data am335x_vdd2 = {
	.constraints = {
		.min_uV			= 600000,
		.max_uV			= 1500000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE,
		.always_on		= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(am335x_vdd2_supply),
	.consumer_supplies	= am335x_vdd2_supply,
};

/* TPS65217 voltage regulator support */
static struct tps65910_board am335x_tps65910_info = {
	.tps65910_pmic_init_data[TPS65910_REG_VRTC]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VIO]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD1]	= &am335x_vdd1,
	.tps65910_pmic_init_data[TPS65910_REG_VDD2]	= &am335x_vdd2,
	.tps65910_pmic_init_data[TPS65910_REG_VDD3]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG1]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VPLL]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDAC]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX1]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX2]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX33]	= &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VMMC]	= &am335x_dummy,
};

static struct i2c_board_info __initdata board_i2c0_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &am335x_tps65910_info,
	},
	{
		I2C_BOARD_INFO("sgtl5000", 0x0a)
	},
};

/* -------------------------------------------------------------------------- */
/* EXPANSION                                                                  */

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux expansion_mux[] __initdata = {
	/* MMC1 */
	AM33XX_MUX(MII1_RXD2, OMAP_MUX_MODE4 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MII1_RXD3, OMAP_MUX_MODE4 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MII1_RXCLK, OMAP_MUX_MODE4 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MII1_TXCLK, OMAP_MUX_MODE4 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_CSN1, OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(GPMC_CSN2, OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(MCASP0_FSX, OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP),
	/* USB */
	AM33XX_MUX(USB0_DRVVBUS, OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP),
	AM33XX_MUX(USB1_DRVVBUS, OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_A0, OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT),
	/* LCD */
	AM33XX_MUX(LCD_DATA0, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA1, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA2, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA3, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA4, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA5, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA6, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA7, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA8, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA9, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA10, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA11, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA12, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA13, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA14, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(LCD_DATA15, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
					| AM33XX_PULL_DISA),
	AM33XX_MUX(GPMC_AD8, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD9, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD10, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD11, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD12, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD13, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD14, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_AD15, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(LCD_VSYNC, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(LCD_HSYNC, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(LCD_PCLK, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(LCD_AC_BIAS_EN, OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_A3, OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(GPMC_A6, OMAP_MUX_MODE7 | AM33XX_PIN_OUTPUT),
	AM33XX_MUX(MCASP0_ACLKX, OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT),
	/* SGTL5000 */
	AM33XX_MUX(MCASP0_ACLKR, OMAP_MUX_MODE3 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MCASP0_FSR, OMAP_MUX_MODE3 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MCASP0_AXR1, OMAP_MUX_MODE3 | AM33XX_PIN_INPUT_PULLDOWN),
	AM33XX_MUX(MCASP0_AHCLKX, OMAP_MUX_MODE3 | AM33XX_PIN_INPUT_PULLDOWN),
	/* Touch screen */
	AM33XX_MUX(MCASP0_AXR0, OMAP_MUX_MODE7 | AM33XX_PIN_INPUT),
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define	board_mux	NULL
#endif

static struct regulator_consumer_supply audio_consumer_supplies[] = {
	REGULATOR_SUPPLY("VDDA", "1-000a"),
	REGULATOR_SUPPLY("VDDIO", "1-000a"),
};

static struct regulator_init_data exp_vdd_reg_init_data = {
	.constraints	= {
		.name	= "3V3",
		.always_on = 1,
	},
	.consumer_supplies = audio_consumer_supplies,
	.num_consumer_supplies = ARRAY_SIZE(audio_consumer_supplies),
};

static struct fixed_voltage_config exp_vdd_pdata = {
	.supply_name		= "exp-3V3",
	.microvolts		= 3300000,
	.gpio			= -EINVAL,
	.enabled_at_boot	= 1,
	.init_data		= &exp_vdd_reg_init_data,
};

static struct platform_device exp_voltage_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &exp_vdd_pdata,
	},
};

static struct omap_musb_board_data musb_board_data = {
	.interface_type	= MUSB_INTERFACE_ULPI,
	/*
	 * mode[0:3] = USB0PORT's mode
	 * mode[4:7] = USB1PORT's mode
	 * Has USB0 in OTG mode and USB1 in host mode.
	 */
	.mode		= (MUSB_HOST << 4) | MUSB_OTG,
	.power		= 500,
	.instances	= 1,
};

/* LCD backlight platform Data */
#define AM335X_BACKLIGHT_MAX_BRIGHTNESS        100
#define AM335X_BACKLIGHT_DEFAULT_BRIGHTNESS    100
#define AM335X_PWM_PERIOD_NANO_SECONDS        (5000 * 10)

static struct platform_pwm_backlight_data am335x_backlight_data0 = {
	.pwm_id         = "ecap.0",
	.ch             = -1,
	.lth_brightness	= 21,
	.max_brightness = AM335X_BACKLIGHT_MAX_BRIGHTNESS,
	.dft_brightness = AM335X_BACKLIGHT_DEFAULT_BRIGHTNESS,
	.pwm_period_ns  = AM335X_PWM_PERIOD_NANO_SECONDS,
};

/* Setup pwm-backlight */
static struct platform_device am335x_backlight = {
	.name           = "pwm-backlight",
	.id             = -1,
	.dev		= {
		.platform_data = &am335x_backlight_data0,
	},
};

static struct pwmss_platform_data  pwm_pdata[3] = {
	{
		.version = PWM_VERSION_1,
	},
	{
		.version = PWM_VERSION_1,
	},
	{
		.version = PWM_VERSION_1,
	},
};

static int __init backlight_init(void)
{

	am33xx_register_ecap(0, &pwm_pdata[0]);
	platform_device_register(&am335x_backlight);
	return 0;
}
late_initcall(backlight_init);

static const struct display_panel disp_panel = {
	WVGA,
	32,
	32,
	COLOR_ACTIVE,
};

static struct lcd_ctrl_config lcd_cfg = {
	&disp_panel,
	.ac_bias		= 255,
	.ac_bias_intrpt		= 0,
	.dma_burst_sz		= 16,
	.bpp			= 32,
	.fdd			= 0x80,
	.tft_alt_mode		= 0,
	.stn_565_mode		= 0,
	.mono_8bit_mode		= 0,
	.invert_line_clock	= 1,
	.invert_frm_clock	= 1,
	.sync_edge		= 0,
	.sync_ctrl		= 1,
	.raster_order		= 0,
};

struct da8xx_lcdc_platform_data tft_pdata = {
	.manu_name		= "ThreeFive",
	.controller_data	= &lcd_cfg,
	.type			= "TFC_S9700RTWV35TR_01B",
};

static int __init conf_disp_pll(int rate)
{
	struct clk *disp_pll;
	int ret = -EINVAL;

	disp_pll = clk_get(NULL, "dpll_disp_ck");
	if (IS_ERR(disp_pll)) {
		pr_err("Cannot clk_get disp_pll\n");
		goto out;
	}

	ret = clk_set_rate(disp_pll, rate);
	clk_put(disp_pll);
out:
	return ret;
}

static void lcdc_init(void)
{
	struct da8xx_lcdc_platform_data *lcdc_pdata;

	if (conf_disp_pll(300000000)) {
		pr_info("Failed configure display PLL, not attempting to"
				"register LCDC\n");
		return;
	}

	lcdc_pdata = &tft_pdata;

	lcdc_pdata->get_context_loss_count = omap_pm_get_dev_context_loss_count;

	if (am33xx_register_lcdc(lcdc_pdata))
		pr_info("Failed to register LCDC device\n");

	return;
}

static u8 am335x_iis_serializer_direction1[] = {
	INACTIVE_MODE,	INACTIVE_MODE,	TX_MODE,	RX_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,	INACTIVE_MODE,
};

static struct snd_platform_data am335x_evm_snd_data1 = {
	.tx_dma_offset	= 0x46400000,	/* McASP1 */
	.rx_dma_offset	= 0x46400000,
	.op_mode	= DAVINCI_MCASP_IIS_MODE,
	.num_serializer	= ARRAY_SIZE(am335x_iis_serializer_direction1),
	.tdm_slots	= 2,
	.serial_dir	= am335x_iis_serializer_direction1,
	.asp_chan_q	= EVENTQ_2,
	.version	= MCASP_VERSION_3,
	.txnumevt	= 32,
	.rxnumevt	= 32,
	.get_context_loss_count	=
			omap_pm_get_dev_context_loss_count,
};

static void __init expansion_init(void)
{
	int status;
	/* Regulators */
	platform_device_register(&exp_voltage_regulator);

	/* Enable USB HOST VBUS */
	status = gpio_request(GPIO_TO_PIN(3, 13), "usb1_vbus_en");
	if (status < 0)
		pr_err("Failed to request gpio for usb1_vbus_en");
	else
		gpio_direction_output(GPIO_TO_PIN(3, 13), 1);

	/* Enable USB OTG VBUS */
	status = gpio_request(GPIO_TO_PIN(1, 16), "usb0_vbus_en");
	if (status < 0)
		pr_err("Failed to request gpio for usb0_vbus_en");
	else
		gpio_direction_output(GPIO_TO_PIN(1, 16), 1);

	usb_musb_init(&musb_board_data);

	/* LCD */
	status = gpio_request(GPIO_TO_PIN(1, 22), "lcd_reset");
	if (status < 0)
		pr_err("Failed to request gpio lcd_reset");
	else
		gpio_direction_output(GPIO_TO_PIN(1, 22), 1);

	status = gpio_request(GPIO_TO_PIN(1, 19), "lcd_reset");
	if (status < 0)
		pr_err("Failed to request gpio lcd_reset");
	else
		gpio_direction_output(GPIO_TO_PIN(1, 19), 1);

	lcdc_init();

	/* Configure McASP */
	am335x_register_mcasp(&am335x_evm_snd_data1, 1);
}

/* -------------------------------------------------------------------------- */


static void __init igep0033_init(void)
{
	/* Default mux configuration */
	am33xx_mux_init(board_mux);
	/* Serial */
	omap_serial_init();
	/* I2C0 */
	omap_register_i2c_bus(1, 100, board_i2c0_boardinfo,
				ARRAY_SIZE(board_i2c0_boardinfo));
	/* SDRAM */
	omap_sdrc_init(NULL, NULL);
	/* NAND */
	board_nand_init();
	/* SD card */
	omap2_hsmmc_init(am335x_mmc);
	/* Ethernet */
	am33xx_cpsw_init(AM33XX_CPSW_MODE_RMII, NULL, NULL);

	/* Create an alias for icss clock */
	if (clk_add_alias("pruss", NULL, "pruss_uart_gclk", NULL))
		pr_warn("failed to create an alias: icss_uart_gclk --> pruss\n");
	/* Create an alias for gfx/sgx clock */
	if (clk_add_alias("sgx_ck", NULL, "gfx_fclk", NULL))
		pr_warn("failed to create an alias: gfx_fclk --> sgx_ck\n");

	/* Expansion */
	am33xx_mux_init(expansion_mux);
	expansion_init();
}

static void __init board_map_io(void)
{
	omap2_set_globals_am33xx();
	omapam33xx_map_common_io();
}

MACHINE_START(IGEP0033, "IGEP COM AQUILA/CYGNUS")
	/* Maintainer: ISEE 2007 SL */
	.atag_offset	= 0x100,
	.map_io		= board_map_io,
	.init_early	= am33xx_init_early,
	.init_irq	= ti81xx_init_irq,
	.handle_irq     = omap3_intc_handle_irq,
	.timer		= &omap3_am33xx_timer,
	.init_machine	= igep0033_init,
MACHINE_END

