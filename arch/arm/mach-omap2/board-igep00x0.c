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
#include <linux/interrupt.h>

#include <linux/can/platform/mcp251x.h>

#include <linux/i2c/at24.h>
#include <linux/i2c/twl.h>

#include <linux/regulator/machine.h>

#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/gpmc.h>
#include <plat/mcspi.h>
#include <plat/onenand.h>
#include <plat/usb.h>
#include <plat/timer-gp.h>

#include "board-igep00x0.h"
#include "mux.h"
#include "sdram-numonyx-m65kxxxxam.h"

struct omap_dss_device igep00x0_dvi_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "dvi",
	.driver_name		= "generic_panel",
	.phy.dpi.data_lines	= 24,
};

struct omap_dss_device igep00x0_tv_device = {
	.name = "tv",
	.driver_name = "venc",
	.type = OMAP_DISPLAY_TYPE_VENC,
	.phy.venc.type = OMAP_DSS_VENC_TYPE_SVIDEO,
};

/* Powertip 4.3 inch (480 x RGB x 272) TFT with Touch-Panel */
struct omap_dss_device igep00x0_lcd43_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "lcd-43",
	.driver_name		= "ph480272t",
	.phy.dpi.data_lines	= 24,
};

/* Seiko 7.0 inch WVGA (800 x RGB x 480) TFT with Touch-Panel */
struct omap_dss_device igep00x0_lcd70_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "lcd-70",
	.driver_name		= "70wvw1tz3",
	.phy.dpi.data_lines	= 24,
};

struct omap_musb_board_data igep00x0_musb_board_data = {
	.interface_type	= MUSB_INTERFACE_ULPI,
	.mode		= MUSB_OTG,
	.power		= 100,
};

static struct regulator_consumer_supply vmmc1_supply =
	REGULATOR_SUPPLY("vmmc", "mmci-omap-hs.0");

/* VMMC1 for OMAP VDD_MMC1 (i/o) and MMC1 card */
struct regulator_init_data igep00x0_vmmc1_idata = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &vmmc1_supply,
};

void __init igep00x0_pmic_get_config(struct twl4030_platform_data *pmic_data,
				  u32 pdata_flags, u32 regulators_flags)
{
	if (!pmic_data->irq_base)
		pmic_data->irq_base = TWL4030_IRQ_BASE;
	if (!pmic_data->irq_end)
		pmic_data->irq_end = TWL4030_IRQ_END;

	/* Common platform data configurations */

	/* Common regulator configurations */
	if (regulators_flags & TWL_IGEP00X0_REGULATOR_VMMC1 && !pmic_data->vmmc1)
		pmic_data->vmmc1 = &igep00x0_vmmc1_idata;
}

#if defined(CONFIG_MTD_ONENAND_OMAP2) || \
	defined(CONFIG_MTD_ONENAND_OMAP2_MODULE)

#define ONENAND_MAP             0x20000000

/* NAND04GR4E1A ( x2 Flash built-in COMBO POP MEMORY )
 * Since the device is equipped with two DataRAMs, and two-plane NAND
 * Flash memory array, these two component enables simultaneous program
 * of 4KiB. Plane1 has only even blocks such as block0, block2, block4
 * while Plane2 has only odd blocks such as block1, block3, block5.
 * So MTD regards it as 4KiB page size and 256KiB block size 64*(2*2048)
 */

static struct mtd_partition igep00x0_onenand_partitions[] = {
	{
		.name           = "X-Loader",
		.offset         = 0,
		.size           = 2 * (64*(2*2048))
	},
	{
		.name           = "U-Boot",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 6 * (64*(2*2048)),
	},
	{
		.name           = "Environment",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 2 * (64*(2*2048)),
	},
	{
		.name           = "Kernel",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 12 * (64*(2*2048)),
	},
	{
		.name           = "File System",
		.offset         = MTDPART_OFS_APPEND,
		.size           = MTDPART_SIZ_FULL,
	},
};

static struct omap_onenand_platform_data igep00x0_onenand_data = {
	.parts = igep00x0_onenand_partitions,
	.nr_parts = ARRAY_SIZE(igep00x0_onenand_partitions),
	.dma_channel	= -1,	/* disable DMA in OMAP OneNAND driver */
};

static struct platform_device igep00x0_onenand_device = {
	.name		= "omap2-onenand",
	.id		= -1,
	.dev = {
		.platform_data = &igep00x0_onenand_data,
	},
};

void __init igep00x0_flash_init(void)
{
	u8 cs = 0;
	u8 onenandcs = GPMC_CS_NUM + 1;

	for (cs = 0; cs < GPMC_CS_NUM; cs++) {
		u32 ret;
		ret = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG1);

		/* Check if NAND/oneNAND is configured */
		if ((ret & 0xC00) == 0x800)
			/* NAND found */
			pr_err("IGEP: Unsupported NAND found\n");
		else {
			ret = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
			if ((ret & 0x3F) == (ONENAND_MAP >> 24))
				/* ONENAND found */
				onenandcs = cs;
		}
	}

	if (onenandcs > GPMC_CS_NUM) {
		pr_err("IGEP: Unable to find configuration in GPMC\n");
		return;
	}

	igep00x0_onenand_data.cs = onenandcs;

	if (platform_device_register(&igep00x0_onenand_device) < 0)
		pr_err("IGEP: Unable to register OneNAND device\n");
}

#else
void __init igep00x0_flash_init(void) {}
#endif

#if defined(CONFIG_SMSC911X) || \
	defined(CONFIG_SMSC911X_MODULE)

void __init igep00x0_smsc911x_init(struct platform_device *pdev,
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
		pr_err("IGEP: Could not obtain gpio SMSC911X IRQ\n");
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

#else
void __init igep00x0_smsc911x_init(struct platform_device *pdev,
					int cs, int irq_gpio, int nreset) { }
#endif

#if defined(CONFIG_TOUCHSCREEN_ADS7846) || \
	defined(CONFIG_TOUCHSCREEN_ADS7846_MODULE)

static struct omap2_mcspi_device_config tsc2046_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1,	/* 0: slave, 1: master */
};

static struct ads7846_platform_data tsc2046_pdata = {
	.x_max			= 0x0fff,
	.y_max			= 0x0fff,
	.x_plate_ohms		= 180,
	.pressure_max		= 255,
	.debounce_max		= 10,
	.debounce_tol		= 3,
	.debounce_rep		= 1,
	.gpio_pendown		= -EINVAL,
	.keep_vref_on		= 1,
};

static struct spi_board_info tsc2046_spi_board_info __initdata = {
	/*
	 * TSC2046 operates at a max freqency of 2MHz, so
	 * operate slightly below at 1.5MHz
	 */
	.modalias		= "ads7846",
	.bus_num		= -EINVAL,
	.chip_select		= -EINVAL,
	.max_speed_hz		= 1500000,
	.controller_data	= &tsc2046_mcspi_config,
	.irq			= -EINVAL,
	.platform_data		= &tsc2046_pdata,
};

void __init igep00x0_tsc2046_init(int busnum, int cs, int irq,
			int debounce)
{
	struct spi_board_info *spi = &tsc2046_spi_board_info;

	if (debounce)
		gpio_set_debounce(irq, debounce);

	tsc2046_pdata.gpio_pendown = irq;

	spi->bus_num = busnum;
	spi->chip_select = cs;
	spi->irq = OMAP_GPIO_IRQ(irq);

	spi_register_board_info(&tsc2046_spi_board_info, 1);
}

#else
void __init igep00x0_tsc2046_init(int busnum, int gpio_pendown,
			int debounce, struct ads7846_platform_data *pdata) {}
#endif

#if defined(CONFIG_CAN_MCP251X) || \
	defined(CONFIG_CAN_MCP251X_MODULE)

static struct mcp251x_platform_data mcp251x_pdata = {
	.oscillator_frequency	= 20*1000*1000,
	.model			= CAN_MCP251X_MCP2515,
	.irq_flags		= IRQF_TRIGGER_RISING,
};

static struct spi_board_info mcp251x_spi_board_info  __initdata = {
	.modalias	= "mcp2515",
	.bus_num	= -EINVAL,
	.chip_select	= -EINVAL,
	.max_speed_hz	= 10*1000*1000,
	.irq		= -EINVAL,
	.mode		= SPI_MODE_0,
	.platform_data	= &mcp251x_pdata,
};

void __init igep00x0_mcp251x_init(int bus_num, int cs, int irq)
{
	struct spi_board_info *spi = &mcp251x_spi_board_info;

	if ((gpio_request(irq, "MCP251X IRQ") == 0)
		&& (gpio_direction_input(irq) == 0))
		gpio_export(irq, 0);
	else {
		pr_err("IGEP: Could not obtain gpio MCP251X IRQ\n");
		return;
	}

	spi->bus_num = bus_num;
	spi->chip_select = cs;
	spi->irq = OMAP_GPIO_IRQ(irq),

	spi_register_board_info(&mcp251x_spi_board_info, 1);
}

#else
void __init igep00x0_mcp251x_init(int bus_num, int cs, int irq) {}
#endif

#if defined(CONFIG_LIBERTAS_SDIO) || \
	defined(CONFIG_LIBERTAS_SDIO_MODULE)
void __init igep00x0_wifi_bt_init(int npd, int wifi_nreset, int bt_nreset)
{
	omap_mux_init_gpio(npd, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(wifi_nreset, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(bt_nreset, OMAP_PIN_OUTPUT);

	/* Set GPIO's for  W-LAN + Bluetooth combo module */
	if ((gpio_request(npd, "WIFI NPD") == 0) &&
	    (gpio_direction_output(npd, 1) == 0))
		gpio_export(npd, 0);
	else
		pr_warning("IGEP: Could not obtain gpio WIFI NPD\n");

	if ((gpio_request(wifi_nreset, "WIFI NRESET") == 0) &&
	    (gpio_direction_output(wifi_nreset, 1) == 0)) {
		gpio_export(wifi_nreset, 0);
		gpio_set_value(wifi_nreset, 0);
		udelay(10);
		gpio_set_value(wifi_nreset, 1);
	} else
		pr_warning("IGEP: Could not obtain gpio WIFI NRESET\n");

	if ((gpio_request(bt_nreset, "BT NRESET") == 0) &&
	    (gpio_direction_output(bt_nreset, 1) == 0))
		gpio_export(bt_nreset, 0);
	else
		pr_warning("IGEP: Could not obtain gpio BT NRESET\n");
}

#else
void __init igep00x0_wifi_bt_init(int npd, int wifi_nreset, int bt_nreset) { }
#endif

#if defined(CONFIG_EEPROM_AT24) || \
	defined(CONFIG_EEPROM_AT24_MODULE)

static struct at24_platform_data at24c01_pdata __initdata = {
	.byte_len	= SZ_1K / 8,
	.page_size	= 8,
};

static struct i2c_board_info at24c01_i2c_board_info __initdata = {
	I2C_BOARD_INFO("24c01", (0xa0 >> 1)),
	.platform_data = &at24c01_pdata,
};

void __init igep00x0_at24c01_init(int busnum)
{
	i2c_register_board_info(busnum, &at24c01_i2c_board_info, 1);
}

#else
void __init igep00x0_at24c01_init(int busnum) {}
#endif

void __init igep00x0_modem_init(int on, int nreset, int pwrmon)
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
	omap_mux_init_gpio(on, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(nreset, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(pwrmon, OMAP_PIN_INPUT);

	if ((gpio_request(on, "MODEM ON NOFF") == 0)
	    && (gpio_direction_output(on, 0) == 0))
		gpio_export(on, 0);
	else
		pr_warning("IGEP: Could not obtain gpio MODEM ON NOFF\n");

	if ((gpio_request(nreset, "MODEM NRESET") == 0) &&
	    (gpio_direction_output(nreset, 0) == 0))
		gpio_export(nreset, 0);
	else
		pr_warning("IGEP: Could not obtain gpio MODEM NRESET\n");

	if ((gpio_request(pwrmon, "MODEM PWRMON") == 0) &&
	    (gpio_direction_input(pwrmon) == 0))
		gpio_export(pwrmon, 0);
	else
		pr_warning("IGEP: Could not obtain gpio MODEM PWRMON\n");
}

void __init igep00x0_init_irq(void)
{
	omap2_init_common_hw(m65kxxxxam_sdrc_params, m65kxxxxam_sdrc_params);
	omap_init_irq();
#ifdef CONFIG_OMAP_32K_TIMER
	omap2_gp_clockevent_set_gptimer(12);
#endif
	omap_gpio_init();
}


