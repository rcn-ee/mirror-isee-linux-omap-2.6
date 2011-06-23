/*
 * Copyright (C) 2011 - ISEE 2007 SL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_MACH_OMAP2_BOARD_IGEP00X0_H
#define __ARCH_ARM_MACH_OMAP2_BOARD_IGEP00X0_H

#include <plat/display.h>

#define IGEP00X0_BUDDY_MAX_STRLEN	16
#define IGEP00X0_BUDDY_NONE		0x01
#define IGEP00X0_BUDDY_IGEP0022		0x01
#define IGEP00X0_BUDDY_BASE0010		0x02

#define IGEP00X0_BUDDY_HWREV_A		(1 << 0)
#define IGEP00X0_BUDDY_HWREV_B		(1 << 1)

#define IGEP00X0_BUDDY_OPT_MODEM	(1 << 0)

#define TWL_IGEP00X0_REGULATOR_VMMC1	(1 << 0)

struct twl4030_platform_data;

struct buddy_platform_data {
	unsigned model;
	unsigned options;
	unsigned revision;
};

extern struct omap_dss_device igep00x0_dvi_device;
extern struct omap_dss_device igep00x0_tv_device;
/* Powertip 4.3 inch (480 x RGB x 272) TFT with Touch-Panel */
extern struct omap_dss_device igep00x0_lcd43_device;
/* Seiko 7.0 inch WVGA (800 x RGB x 480) TFT with Touch-Panel */
extern struct omap_dss_device igep00x0_lcd70_device;

extern struct omap_musb_board_data igep00x0_musb_board_data;

extern struct buddy_platform_data igep00x0_buddy_pdata;

void igep00x0_pmic_get_config(struct twl4030_platform_data *pmic_data,
			   u32 pdata_flags, u32 regulators_flags);

void igep00x0_flash_init(void);

void igep00x0_smsc911x_init(struct platform_device *pdev,
				int cs, int irq_gpio, int nreset);

void igep00x0_tsc2046_init(int busnum, int cs, int irq, int debounce);

void igep00x0_mcp251x_init(int busnum, int cs, int irq, int nreset);

void igep00x0_wifi_bt_init(int npd, int wifi_nreset, int bt_nreset,
			int bt_enable);

void igep00x0_at24c01_init(int busnum);

void igep00x0_modem_init(int on, int nreset, int pwrmon);

void igep00x0_camera_init(void);

void igep00x0_init_irq(void);

#endif /* __ARCH_ARM_MACH_OMAP2_BOARD_IGEP00X0_H */
