/*
 * LCD panel driver for Powertip PH480272T
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

static struct omap_video_timings ph480272t_timings = {
	.x_res		= 480,
	.y_res		= 272,
	.pixel_clock	= 9000,
	.hsw		= 40,
	.hfp		= 2,
	.hbp		= 2,
	.vsw		= 10,
	.vfp            = 2,
	.vbp            = 2,
};

static int ph480272t_power_on(struct omap_dss_device *dssdev)
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

static void ph480272t_power_off(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);

	/* wait at least 5 vsyncs after disabling the LCD */
	msleep(100);

	omapdss_dpi_display_disable(dssdev);
}

static int ph480272t_probe(struct omap_dss_device *dssdev)
{

	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
		OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_IEO;

	dssdev->panel.acb = 0x0;
	dssdev->panel.timings = ph480272t_timings;

	return 0;
}

static void ph480272t_remove(struct omap_dss_device *dssdev)
{
}

static int ph480272t_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = ph480272t_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void ph480272t_disable(struct omap_dss_device *dssdev)
{
	ph480272t_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int ph480272t_suspend(struct omap_dss_device *dssdev)
{
	ph480272t_power_off(dssdev);
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;
	return 0;
}

static int ph480272t_resume(struct omap_dss_device *dssdev)
{
	int r = 0;

	r = ph480272t_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static struct omap_dss_driver ph480272t_driver = {
	.probe		= ph480272t_probe,
	.remove		= ph480272t_remove,
	.enable		= ph480272t_enable,
	.disable	= ph480272t_disable,
	.suspend	= ph480272t_suspend,
	.resume		= ph480272t_resume,
	.driver         = {
		.name   = "ph480272t",
		.owner  = THIS_MODULE,
	},
};

static int __init ph480272t_init(void)
{
	return omap_dss_register_driver(&ph480272t_driver);
}

static void __exit ph480272t_exit(void)
{
	omap_dss_unregister_driver(&ph480272t_driver);
}

module_init(ph480272t_init);
module_exit(ph480272t_exit);
MODULE_LICENSE("GPL");
