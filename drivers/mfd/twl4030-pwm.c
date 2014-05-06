/*
 * twl4030-pwm.c
 * Copyright (C) 2012 Corscience GmbH & Co. KG
 *               Bernhard Walle <bernhard@bwalle.de>
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
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/slab.h>

#define TWL_INTBR_PMBR1		0xD
#define TWL_INTBR_GPBR1		0xC
#define TWL_LED_PWMON		0x0
#define TWL_LED_PWMOFF		0x1

/* GPBR1 */
#define GPBR1_PWM1_ENABLE	(1<<3)
#define GPBR1_PWM0_ENABLE	(1<<2)
#define GPBR1_PWM1_CLK_ENABLE	(1<<1)
#define GPBR1_PWM0_CLK_ENABLE	(1<<0)

/* PMBR1 */
#define PMBR1_GPIO7_MASK	((1<<5)|(1<<4))
#define PMBR1_GPIO7_GPIO	(0x0<<4)
#define PMBR1_GPIO7_VIBRASYNC	(0x1<<4)
#define PMBR1_GPIO7_NOACTION	(0x2<<4)
#define PMBR1_GPIO7_PWM1	(0x3<<4)

#define PMBR1_GPIO6_MASK	((1<<3)|(1<<2))
#define PMBR1_GPIO6_GPIO	(0x0<<2)
#define PMBR1_GPIO6_PWM0	(0x1<<2)
#define PMBR1_GPIO6_MUTE	(0x2<<2)

/* TWL_LED_PWMON and TWL_LED_PWMOFF must be between 0 and PWM_COUNT_MAX. */
#define PWM_COUNT_MAX		128

/* Opaque data structure */
struct pwm_device {
	const char *label;
	unsigned int pwm_id;
};

/*
 * Converts the pwm_id to the I2C register of the TWL that is used to
 * set the PWM on and off cycle.
 */
static const u8 twl_module_register[] = {
	[0] = TWL4030_MODULE_PWM0,
	[1] = TWL4030_MODULE_PWM1
};

int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	u8 duty_cycle, addr;
	int ret;

	if (pwm == NULL || period_ns == 0 || duty_ns > period_ns)
		return -EINVAL;

	duty_cycle = (duty_ns * PWM_COUNT_MAX) / period_ns;
	addr = twl_module_register[pwm->pwm_id];

	ret = twl_i2c_write_u8(addr, 0, TWL_LED_PWMON);
	if (ret) {
		pr_err("%s: Failed to configure PWM, Error %d\n",
		       pwm->label, ret);
		return ret;
	}

	ret = twl_i2c_write_u8(addr, duty_cycle, TWL_LED_PWMOFF);
	if (ret < 0) {
		pr_err("%s: Failed to configure PWM, Error %d\n",
		       pwm->label, ret);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(pwm_config);

int pwm_enable(struct pwm_device *pwm)
{
	int ret;
	u8 val;

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, TWL_INTBR_GPBR1);
	if (ret < 0) {
		pr_err("%s: failed to read TWL_INTBR_GPBR1 register\n",
			pwm->label);
		return ret;
	}

	if (pwm->pwm_id == 0)
		val |= GPBR1_PWM0_ENABLE | GPBR1_PWM0_CLK_ENABLE;
	else
		val |= GPBR1_PWM1_ENABLE | GPBR1_PWM1_CLK_ENABLE;

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, TWL_INTBR_GPBR1);
	if (ret < 0) {
		pr_err("%s: failed to write TWL_INTBR_GPBR1 register\n",
			pwm->label);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL(pwm_enable);

void pwm_disable(struct pwm_device *pwm)
{
	int ret;
	u8 val;

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, TWL_INTBR_GPBR1);
	if (ret < 0) {
		pr_err("%s: failed to read TWL_INTBR_GPBR1 register\n",
			pwm->label);
		return;
	}

	if (pwm->pwm_id == 0)
		val &= ~(GPBR1_PWM0_ENABLE | GPBR1_PWM0_CLK_ENABLE);
	else
		val &= ~(GPBR1_PWM1_ENABLE | GPBR1_PWM1_CLK_ENABLE);

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, TWL_INTBR_GPBR1);
	if (ret < 0)
		pr_err("%s: failed to write TWL_INTBR_GPBR1 register\n",
			pwm->label);
}
EXPORT_SYMBOL(pwm_disable);

struct pwm_device *pwm_request(int pwm_id, const char *label)
{
	int ret;
	u8 val;
	struct pwm_device *pwm;

	if (!(pwm_id == 0 || pwm_id == 1)) {
		pr_err("%s: Invalid pwm_id, only 0 or 1 allowed\n", label);
		return NULL;
	}

	pwm = kzalloc(sizeof(struct pwm_device), GFP_KERNEL);
	if (!pwm) {
		pr_err("%s: failed to allocate memory\n", label);
		return NULL;
	}

	pwm->label = label;
	pwm->pwm_id = pwm_id;

	/* change pin mux to use the PWM */

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, TWL_INTBR_PMBR1);
	if (ret < 0) {
		pr_err("%s: failed to read TWL_INTBR_PMBR1 register\n",
			pwm->label);
		goto err;
	}

	if (pwm->pwm_id == 0) {
		val &= PMBR1_GPIO6_MASK;
		val |= PMBR1_GPIO6_PWM0;
	} else {
		val &= PMBR1_GPIO7_MASK;
		val |= PMBR1_GPIO7_PWM1;
	}

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, TWL_INTBR_PMBR1);
	if (ret < 0) {
		pr_err("%s: failed to write TWL_INTBR_PMBR1 register\n",
			pwm->label);
		goto err;
	}

	return pwm;

err:
	kfree(pwm);
	return NULL;
}
EXPORT_SYMBOL(pwm_request);

void pwm_free(struct pwm_device *pwm)
{
	int ret;
	u8 val;

	pwm_disable(pwm);

	/* change pin mux to use the GPIOs */

	ret = twl_i2c_read_u8(TWL4030_MODULE_INTBR, &val, TWL_INTBR_PMBR1);
	if (ret < 0) {
		pr_err("%s: failed to read TWL_INTBR_PMBR1 register\n",
			pwm->label);
		goto out;
	}

	if (pwm->pwm_id == 0) {
		val &= PMBR1_GPIO6_MASK;
		val |= PMBR1_GPIO6_GPIO;
	} else {
		val &= PMBR1_GPIO7_MASK;
		val |= PMBR1_GPIO7_GPIO;
	}

	ret = twl_i2c_write_u8(TWL4030_MODULE_INTBR, val, TWL_INTBR_PMBR1);
	if (ret < 0) {
		pr_err("%s: failed to write TWL_INTBR_PMBR1 register\n",
			pwm->label);
	}

out:
	kfree(pwm);
}
EXPORT_SYMBOL(pwm_free);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TWL4030 PWM driver");
MODULE_AUTHOR("Bernhard Walle <bernhard@bwalle.de>");
