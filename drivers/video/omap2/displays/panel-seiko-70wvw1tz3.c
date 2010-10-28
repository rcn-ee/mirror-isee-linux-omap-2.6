/*
 * LCD panel driver for Seiko 70wvw1tz3Z3
 *
 * Copyright (C) 2010, ISEE 2007 SL
 * Author: Enric Balletbo i Serra <eballetbo@iseebcn.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>

#include <plat/display.h>

static struct omap_video_timings s70wvw1tz3_timings = {
	.x_res = 800,
	.y_res = 480,
	.pixel_clock	= 33000,
	.hsw		= 128,
	.hfp		= 10,
	.hbp		= 10,
	.vsw		= 2,
	.vfp		= 4,
	.vbp		= 11,
};

static int s70wvw1tz3_panel_power_on(struct omap_dss_device *dssdev)
{
	int r;

	r = omapdss_dpi_display_enable(dssdev);
	if (r)
		goto err0;

	/* wait couple of vsyncs until enabling the LCD */
	msleep(50);

	if (dssdev->platform_enable) {
		r = dssdev->platform_enable(dssdev);
		if (r)
			goto err1;
	}

	return 0;
err1:
	omapdss_dpi_display_disable(dssdev);
err0:
	return r;
}

static void s70wvw1tz3_panel_power_off(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	/* wait at least 5 vsyncs after disabling the LCD */
	msleep(100);

	omapdss_dpi_display_disable(dssdev);
}

static int s70wvw1tz3_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
		OMAP_DSS_LCD_IHS;

	dssdev->panel.acb = 0x0;
	dssdev->panel.timings = s70wvw1tz3_timings;

	return 0;
}

static void s70wvw1tz3_panel_remove(struct omap_dss_device *dssdev)
{
}

static int s70wvw1tz3_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = s70wvw1tz3_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void s70wvw1tz3_panel_disable(struct omap_dss_device *dssdev)
{
	s70wvw1tz3_panel_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int s70wvw1tz3_panel_suspend(struct omap_dss_device *dssdev)
{
	s70wvw1tz3_panel_power_off(dssdev);
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;

	return 0;
}

static int s70wvw1tz3_panel_resume(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = s70wvw1tz3_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static struct omap_dss_driver s70wvw1tz3_driver = {
	.probe		= s70wvw1tz3_panel_probe,
	.remove		= s70wvw1tz3_panel_remove,

	.enable		= s70wvw1tz3_panel_enable,
	.disable	= s70wvw1tz3_panel_disable,
	.suspend	= s70wvw1tz3_panel_suspend,
	.resume		= s70wvw1tz3_panel_resume,

	.driver         = {
		.name   = "70wvw1tz3",
		.owner  = THIS_MODULE,
	},
};

static int __init s70wvw1tz3_init(void)
{
	return omap_dss_register_driver(&s70wvw1tz3_driver);
}

static void __exit s70wvw1tz3_exit(void)
{
	omap_dss_unregister_driver(&s70wvw1tz3_driver);
}

module_init(s70wvw1tz3_init);
module_exit(s70wvw1tz3_exit);
MODULE_LICENSE("GPL");
