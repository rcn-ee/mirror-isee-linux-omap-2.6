/*
 * Atmel maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

/* Version */
#define MXT_VER_20		20
#define MXT_VER_21		21
#define MXT_VER_22		22

/* Slave addresses */
#define MXT_APP_LOW		0x4a
#define MXT_APP_HIGH		0x4b
/*
 * MXT_BOOT_LOW disagrees with Atmel documentation, but has been
 * updated to support new touch hardware that pairs 0x26 boot with 0x4a app.
 */
#define MXT_BOOT_LOW		0x26
#define MXT_BOOT_HIGH		0x25

/* Firmware */
#define MXT_FW_NAME		"maxtouch.fw"

/* Registers */
#define MXT_INFO		0x00
#define MXT_FAMILY_ID		0x00
#define MXT_VARIANT_ID		0x01
#define MXT_VERSION		0x02
#define MXT_BUILD		0x03
#define MXT_MATRIX_X_SIZE	0x04
#define MXT_MATRIX_Y_SIZE	0x05
#define MXT_OBJECT_NUM		0x06
#define MXT_OBJECT_START	0x07

#define MXT_OBJECT_SIZE		6

/* Object types */
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_GEN_DATASOURCE_T53		53
#define MXT_TOUCH_MULTI_T9		9
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_TOUCH_PROXKEY_T52		52
#define MXT_PROCI_GRIPFACE_T20		20
#define MXT_PROCG_NOISE_T22		22
#define MXT_PROCI_ONETOUCH_T24		24
#define MXT_PROCI_TWOTOUCH_T27		27
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_PALM_T41		41
#define MXT_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCG_NOISESUPPRESSION_T48	48
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_SPT_SELFTEST_T25		25
#define MXT_SPT_CTECONFIG_T28		28
#define MXT_SPT_USERDATA_T38		38
#define MXT_SPT_DIGITIZER_T43		43
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_SPT_CTECONFIG_T46		46

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET	0
#define MXT_COMMAND_BACKUPNV	1
#define MXT_COMMAND_CALIBRATE	2
#define MXT_COMMAND_REPORTALL	3
#define MXT_COMMAND_DIAGNOSTIC	5

/* MXT_GEN_POWER_T7 field */
#define MXT_POWER_IDLEACQINT	0
#define MXT_POWER_ACTVACQINT	1
#define MXT_POWER_ACTV2IDLETO	2

/* MXT_GEN_ACQUIRE_T8 field */
#define MXT_ACQUIRE_CHRGTIME	0
#define MXT_ACQUIRE_TCHDRIFT	2
#define MXT_ACQUIRE_DRIFTST	3
#define MXT_ACQUIRE_TCHAUTOCAL	4
#define MXT_ACQUIRE_SYNC	5
#define MXT_ACQUIRE_ATCHCALST	6
#define MXT_ACQUIRE_ATCHCALSTHR	7

/* MXT_TOUCH_MULTI_T9 field */
#define MXT_TOUCH_CTRL		0
#define MXT_TOUCH_XORIGIN	1
#define MXT_TOUCH_YORIGIN	2
#define MXT_TOUCH_XSIZE		3
#define MXT_TOUCH_YSIZE		4
#define MXT_TOUCH_BLEN		6
#define MXT_TOUCH_TCHTHR	7
#define MXT_TOUCH_TCHDI		8
#define MXT_TOUCH_ORIENT	9
#define MXT_TOUCH_MOVHYSTI	11
#define MXT_TOUCH_MOVHYSTN	12
#define MXT_TOUCH_NUMTOUCH	14
#define MXT_TOUCH_MRGHYST	15
#define MXT_TOUCH_MRGTHR	16
#define MXT_TOUCH_AMPHYST	17
#define MXT_TOUCH_XRANGE_LSB	18
#define MXT_TOUCH_XRANGE_MSB	19
#define MXT_TOUCH_YRANGE_LSB	20
#define MXT_TOUCH_YRANGE_MSB	21
#define MXT_TOUCH_XLOCLIP	22
#define MXT_TOUCH_XHICLIP	23
#define MXT_TOUCH_YLOCLIP	24
#define MXT_TOUCH_YHICLIP	25
#define MXT_TOUCH_XEDGECTRL	26
#define MXT_TOUCH_XEDGEDIST	27
#define MXT_TOUCH_YEDGECTRL	28
#define MXT_TOUCH_YEDGEDIST	29
#define MXT_TOUCH_JUMPLIMIT	30

/* MXT_PROCI_GRIPFACE_T20 field */
#define MXT_GRIPFACE_CTRL	0
#define MXT_GRIPFACE_XLOGRIP	1
#define MXT_GRIPFACE_XHIGRIP	2
#define MXT_GRIPFACE_YLOGRIP	3
#define MXT_GRIPFACE_YHIGRIP	4
#define MXT_GRIPFACE_MAXTCHS	5
#define MXT_GRIPFACE_SZTHR1	7
#define MXT_GRIPFACE_SZTHR2	8
#define MXT_GRIPFACE_SHPTHR1	9
#define MXT_GRIPFACE_SHPTHR2	10
#define MXT_GRIPFACE_SUPEXTTO	11

/* MXT_PROCI_NOISE field */
#define MXT_NOISE_CTRL		0
#define MXT_NOISE_OUTFLEN	1
#define MXT_NOISE_GCAFUL_LSB	3
#define MXT_NOISE_GCAFUL_MSB	4
#define MXT_NOISE_GCAFLL_LSB	5
#define MXT_NOISE_GCAFLL_MSB	6
#define MXT_NOISE_ACTVGCAFVALID	7
#define MXT_NOISE_NOISETHR	8
#define MXT_NOISE_FREQHOPSCALE	10
#define MXT_NOISE_FREQ0		11
#define MXT_NOISE_FREQ1		12
#define MXT_NOISE_FREQ2		13
#define MXT_NOISE_FREQ3		14
#define MXT_NOISE_FREQ4		15
#define MXT_NOISE_IDLEGCAFVALID	16

/* MXT_SPT_COMMSCONFIG_T18 */
#define MXT_COMMS_CTRL		0
#define MXT_COMMS_CMD		1

/* MXT_SPT_CTECONFIG_T28 field */
#define MXT_CTE_CTRL		0
#define MXT_CTE_CMD		1
#define MXT_CTE_MODE		2
#define MXT_CTE_IDLEGCAFDEPTH	3
#define MXT_CTE_ACTVGCAFDEPTH	4
#define MXT_CTE_VOLTAGE		5

#define MXT_VOLTAGE_DEFAULT	2700000
#define MXT_VOLTAGE_STEP	10000

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_BOOT_VALUE		0xa5
#define MXT_BACKUP_VALUE	0x55
#define MXT_BACKUP_TIME		25	/* msec */
#define MXT_RESET_TIME		65	/* msec */
#define MXT_CAL_TIME		25	/* msec */

#define MXT_FWRESET_TIME	500	/* msec */

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK	0x02
#define MXT_FRAME_CRC_FAIL	0x03
#define MXT_FRAME_CRC_PASS	0x04
#define MXT_APP_CRC_FAIL	0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f

/* Touch status */
#define MXT_UNGRIP		(1 << 0)
#define MXT_SUPPRESS		(1 << 1)
#define MXT_AMP			(1 << 2)
#define MXT_VECTOR		(1 << 3)
#define MXT_MOVE		(1 << 4)
#define MXT_RELEASE		(1 << 5)
#define MXT_PRESS		(1 << 6)
#define MXT_DETECT		(1 << 7)

/* Touch orient bits */
#define MXT_XY_SWITCH		(1 << 0)
#define MXT_X_INVERT		(1 << 1)
#define MXT_Y_INVERT		(1 << 2)

/* Touchscreen absolute values */
#define MXT_MAX_AREA		0xff

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size;		/* Size of each instance - 1 */
	u8 instances;		/* Number of instances - 1 */
	u8 num_report_ids;
} __packed;

struct mxt_message {
	u8 reportid;
	u8 message[7];
};

/* Each client has this additional data */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	char phys[64];		/* device physical location */
	const struct mxt_platform_data *pdata;
	struct mxt_object *object_table;
	struct mxt_info info;
	unsigned int irq;
	unsigned int max_x;
	unsigned int max_y;

	u32 info_csum;
	u32 config_csum;

	/* Cached parameters from object table */
	u8 T6_reportid;
	u8 T9_reportid_min;
	u8 T9_reportid_max;

	/* for fw update in bootloader */
	struct completion bl_completion;
};

static void mxt_free_object_table(struct mxt_data *data);
static int mxt_initialize(struct mxt_data *data);
static int mxt_input_dev_create(struct mxt_data *data);
static int mxt_make_highchg(struct mxt_data *data);

static bool mxt_object_readable(unsigned int type)
{
	switch (type) {
	case MXT_GEN_COMMAND_T6:
	case MXT_GEN_POWER_T7:
	case MXT_GEN_ACQUIRE_T8:
	case MXT_GEN_DATASOURCE_T53:
	case MXT_TOUCH_MULTI_T9:
	case MXT_TOUCH_KEYARRAY_T15:
	case MXT_TOUCH_PROXIMITY_T23:
	case MXT_TOUCH_PROXKEY_T52:
	case MXT_PROCI_GRIPFACE_T20:
	case MXT_PROCG_NOISE_T22:
	case MXT_PROCI_ONETOUCH_T24:
	case MXT_PROCI_TWOTOUCH_T27:
	case MXT_PROCI_GRIP_T40:
	case MXT_PROCI_PALM_T41:
	case MXT_PROCI_TOUCHSUPPRESSION_T42:
	case MXT_PROCI_STYLUS_T47:
	case MXT_PROCG_NOISESUPPRESSION_T48:
	case MXT_SPT_COMMSCONFIG_T18:
	case MXT_SPT_GPIOPWM_T19:
	case MXT_SPT_SELFTEST_T25:
	case MXT_SPT_CTECONFIG_T28:
	case MXT_SPT_USERDATA_T38:
	case MXT_SPT_DIGITIZER_T43:
	case MXT_SPT_CTECONFIG_T46:
		return true;
	default:
		return false;
	}
}

static bool mxt_object_writable(unsigned int type)
{
	switch (type) {
	case MXT_GEN_COMMAND_T6:
	case MXT_GEN_POWER_T7:
	case MXT_GEN_ACQUIRE_T8:
	case MXT_TOUCH_MULTI_T9:
	case MXT_TOUCH_KEYARRAY_T15:
	case MXT_TOUCH_PROXIMITY_T23:
	case MXT_TOUCH_PROXKEY_T52:
	case MXT_PROCI_GRIPFACE_T20:
	case MXT_PROCG_NOISE_T22:
	case MXT_PROCI_ONETOUCH_T24:
	case MXT_PROCI_TWOTOUCH_T27:
	case MXT_PROCI_GRIP_T40:
	case MXT_PROCI_PALM_T41:
	case MXT_PROCI_TOUCHSUPPRESSION_T42:
	case MXT_PROCI_STYLUS_T47:
	case MXT_PROCG_NOISESUPPRESSION_T48:
	case MXT_SPT_COMMSCONFIG_T18:
	case MXT_SPT_GPIOPWM_T19:
	case MXT_SPT_SELFTEST_T25:
	case MXT_SPT_CTECONFIG_T28:
	case MXT_SPT_DIGITIZER_T43:
	case MXT_SPT_CTECONFIG_T46:
		return true;
	default:
		return false;
	}
}

static void mxt_dump_message(struct device *dev,
			     struct mxt_message *message)
{
	dev_dbg(dev, "reportid: %u\tmessage: %*ph\n",
		message->reportid, 7, message->message);
}

static bool mxt_in_bootloader(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	return (client->addr == MXT_BOOT_LOW || client->addr == MXT_BOOT_HIGH);
}

static int mxt_i2c_recv(struct i2c_client *client, u8 *buf, size_t count)
{
	int ret;

	ret = i2c_master_recv(client, buf, count);
	if (ret == count) {
		ret = 0;
	} else if (ret != count) {
		ret = (ret < 0) ? ret : -EIO;
		dev_err(&client->dev, "i2c recv failed (%d)\n", ret);
	}

	return ret;
}

static int mxt_i2c_send(struct i2c_client *client, const u8 *buf, size_t count)
{
	int ret;

	ret = i2c_master_send(client, buf, count);
	if (ret == count) {
		ret = 0;
	} else if (ret != count) {
		ret = (ret < 0) ? ret : -EIO;
		dev_err(&client->dev, "i2c send failed (%d)\n", ret);
	}

	return ret;
}

static int mxt_i2c_transfer(struct i2c_client *client, struct i2c_msg *msgs,
		size_t count)
{
	int ret;

	ret = i2c_transfer(client->adapter, msgs, count);
	if (ret == count) {
		ret = 0;
	} else {
		ret = (ret < 0) ? ret : -EIO;
		dev_err(&client->dev, "i2c transfer failed (%d)\n", ret);
	}

	return ret;
}

static int mxt_wait_for_chg(struct mxt_data *data, unsigned int timeout_ms)
{
	struct device *dev = &data->client->dev;
	struct completion *comp = &data->bl_completion;
	unsigned long timeout = msecs_to_jiffies(timeout_ms);
	long ret;

	ret = wait_for_completion_interruptible_timeout(comp, timeout);
	if (ret < 0) {
		dev_err(dev, "Wait for completion interrupted.\n");
		/*
		 * TODO: handle -EINTR better by terminating fw update process
		 * before returning to userspace by writing length 0x000 to
		 * device (iff we are in WAITING_FRAME_DATA state).
		 */
		return -EINTR;
	} else if (ret == 0) {
		dev_err(dev, "Wait for completion timed out.\n");
		return -ETIMEDOUT;
	}
	return 0;
}

static int mxt_check_bootloader(struct mxt_data *data, unsigned int state)
{
	struct i2c_client *client = data->client;
	u8 val;
	int ret;

recheck:
	if (state != MXT_WAITING_BOOTLOAD_CMD) {
		/*
		 * In application update mode, the interrupt
		 * line signals state transitions. We must wait for the
		 * CHG assertion before reading the status byte.
		 * Once the status byte has been read, the line is deasserted.
		 */
		int ret = mxt_wait_for_chg(data, 300);
		if (ret) {
			dev_err(&client->dev, "Update wait error %d\n", ret);
			return ret;
		}
	}

	ret = mxt_i2c_recv(client, &val, 1);
	if (ret)
		return ret;

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
		dev_info(&client->dev, "bootloader version: %d\n",
			 val & MXT_BOOT_STATUS_MASK);
	case MXT_WAITING_FRAME_DATA:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(&client->dev, "Unvalid bootloader mode state\n");
		dev_err(&client->dev, "Invalid bootloader mode state %d, %d\n",
			val, state);
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	return mxt_i2c_send(client, buf, 2);
}

static int mxt_fw_write(struct i2c_client *client,
			     const u8 *data, unsigned int frame_size)
{
	return mxt_i2c_send(client, data, frame_size);
}

static int __mxt_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	return mxt_i2c_transfer(client, xfer, 2);
}

static int mxt_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
	return __mxt_read_reg(client, reg, 1, val);
}

static int __mxt_write_reg(struct i2c_client *client, u16 reg, u16 len,
			   const void *val)
{
	u8 *buf;
	size_t count;
	int ret;

	count = len + 2;
	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	memcpy(&buf[2], val, len);

	ret = mxt_i2c_send(client, buf, count);
	kfree(buf);
	return ret;
}

static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	return __mxt_write_reg(client, reg, 1, &val);
}

static struct mxt_object *
mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type\n");
	return NULL;
}

static int mxt_read_message(struct mxt_data *data,
				 struct mxt_message *message)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, MXT_GEN_MESSAGE_T5);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	return __mxt_read_reg(data->client, reg,
			sizeof(struct mxt_message), message);
}

static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object || offset >= object->size + 1)
		return -EINVAL;

	reg = object->start_address;
	return mxt_write_reg(data->client, reg + offset, val);
}

static void mxt_input_touchevent(struct mxt_data *data,
				      struct mxt_message *message, int id)
{
	struct device *dev = &data->client->dev;
	u8 status = message->message[0];
	struct input_dev *input_dev = data->input_dev;
	int x;
	int y;
	int area;
	int pressure;

	x = (message->message[1] << 4) | ((message->message[3] >> 4) & 0xf);
	y = (message->message[2] << 4) | ((message->message[3] & 0xf));
	if (data->max_x < 1024)
		x = x >> 2;
	if (data->max_y < 1024)
		y = y >> 2;

	area = message->message[4];
	pressure = message->message[5];

	dev_dbg(dev,
		"[%u] %c%c%c%c%c%c%c%c x: %5u y: %5u area: %3u amp: %3u\n",
		id,
		(status & MXT_DETECT) ? 'D' : '.',
		(status & MXT_PRESS) ? 'P' : '.',
		(status & MXT_RELEASE) ? 'R' : '.',
		(status & MXT_MOVE) ? 'M' : '.',
		(status & MXT_VECTOR) ? 'V' : '.',
		(status & MXT_AMP) ? 'A' : '.',
		(status & MXT_SUPPRESS) ? 'S' : '.',
		(status & MXT_UNGRIP) ? 'U' : '.',
		x, y, area, pressure);

	input_mt_slot(input_dev, id);
	input_mt_report_slot_state(input_dev, MT_TOOL_FINGER,
				   status & MXT_DETECT);

	if (status & MXT_DETECT) {
		input_report_abs(input_dev, ABS_MT_POSITION_X, x);
		input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
		input_report_abs(input_dev, ABS_MT_PRESSURE, pressure);
		input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, area);
	}
}

static unsigned mxt_extract_T6_csum(const u8 *csum)
{
	return csum[0] | (csum[1] << 8) | (csum[2] << 16);
}

static bool mxt_is_T9_message(struct mxt_data *data, struct mxt_message *msg)
{
	u8 id = msg->reportid;
	return (id >= data->T9_reportid_min && id <= data->T9_reportid_max);
}

static int mxt_enter_bl(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct device *dev = &client->dev;
	int ret;

	if (mxt_in_bootloader(data))
		return 0;

	disable_irq(data->irq);

	/* Change to the bootloader mode */
	ret = mxt_write_object(data, MXT_GEN_COMMAND_T6,
			       MXT_COMMAND_RESET, MXT_BOOT_VALUE);
	if (ret) {
		enable_irq(data->irq);
		return ret;
	}

	/* Change to slave address of bootloader */
	if (client->addr == MXT_APP_LOW)
		client->addr = MXT_BOOT_LOW;
	else
		client->addr = MXT_BOOT_HIGH;

	/* Free any driver state. It will get reinitialized after fw update. */
	mxt_free_object_table(data);
	if (data->input_dev) {
		input_unregister_device(data->input_dev);
		data->input_dev = NULL;
	}

	INIT_COMPLETION(data->bl_completion);
	enable_irq(data->irq);

	/* Wait for CHG assert to indicate successful reset into bootloader */
	ret = mxt_wait_for_chg(data, MXT_RESET_TIME);
	if (ret) {
		dev_err(dev, "Failed waiting for reset to bootloader.\n");
		if (client->addr == MXT_BOOT_LOW)
			client->addr = MXT_APP_LOW;
		else
			client->addr = MXT_APP_HIGH;
		return ret;
	}
	return 0;
}

static void mxt_exit_bl(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct device *dev = &client->dev;
	int error;

	if (!mxt_in_bootloader(data))
		return;

	/* Wait for reset */
	mxt_wait_for_chg(data, MXT_FWRESET_TIME);

	disable_irq(data->irq);
	if (client->addr == MXT_BOOT_LOW)
		client->addr = MXT_APP_LOW;
	else
		client->addr = MXT_APP_HIGH;

	error = mxt_initialize(data);
	if (error) {
		dev_err(dev, "Failed to initialize on exit bl. error = %d\n",
			error);
		return;
	}

	error = mxt_input_dev_create(data);
	if (error) {
		dev_err(dev, "Create input dev failed after init. error = %d\n",
			error);
		return;
	}

	error = mxt_make_highchg(data);
	if (error)
		dev_err(dev, "Failed to clear CHG after init. error = %d\n",
			error);
	enable_irq(data->irq);
}

static irqreturn_t mxt_interrupt(int irq, void *dev_id)
{
	struct mxt_data *data = dev_id;
	struct mxt_message message;
	const u8 *payload = &message.message[0];
	struct device *dev = &data->client->dev;
	u8 reportid;
	bool update_input = false;

	if (mxt_in_bootloader(data)) {
		/* bootloader state transition completion */
		complete(&data->bl_completion);
		goto end;
	}

	do {
		if (mxt_read_message(data, &message)) {
			dev_err(dev, "Failed to read message\n");
			goto end;
		}

		reportid = message.reportid;

		if (reportid == data->T6_reportid) {
			u8 status = payload[0];
			data->config_csum = mxt_extract_T6_csum(&payload[1]);
			dev_dbg(dev, "Status: %02x Config Checksum: %06x\n",
				status, data->config_csum);
		} else if (mxt_is_T9_message(data, &message)) {
			int id = reportid - data->T9_reportid_min;
			mxt_input_touchevent(data, &message, id);
			update_input = true;
		} else {
			mxt_dump_message(dev, &message);
		}
	} while (reportid != 0xff);

	if (update_input) {
		input_mt_report_pointer_emulation(data->input_dev, false);
		input_sync(data->input_dev);
	}

end:
	return IRQ_HANDLED;
}

static int mxt_check_reg_init(struct mxt_data *data)
{
	const struct mxt_platform_data *pdata = data->pdata;
	struct mxt_object *object;
	struct device *dev = &data->client->dev;
	int index = 0;
	int i, size;
	int ret;

	if (!pdata->config) {
		dev_dbg(dev, "No cfg data defined, skipping reg init\n");
		return 0;
	}

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		if (!mxt_object_writable(object->type))
			continue;

		size = (object->size + 1) * (object->instances + 1);
		if (index + size > pdata->config_length) {
			dev_err(dev, "Not enough config data!\n");
			return -EINVAL;
		}

		ret = __mxt_write_reg(data->client, object->start_address,
				size, &pdata->config[index]);
		if (ret)
			return ret;
		index += size;
	}

	return 0;
}

static int mxt_make_highchg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_message message;
	int count = 10;
	int error;

	/* Read dummy message to make high CHG pin */
	do {
		error = mxt_read_message(data, &message);
		if (error)
			return error;
	} while (message.reportid != 0xff && --count);

	if (!count) {
		dev_err(dev, "CHG pin isn't cleared\n");
		return -EBUSY;
	}

	return 0;
}

static void mxt_handle_pdata(struct mxt_data *data)
{
	const struct mxt_platform_data *pdata = data->pdata;
	u8 voltage;

	/* Set touchscreen lines */
	mxt_write_object(data, MXT_TOUCH_MULTI_T9, MXT_TOUCH_XSIZE,
			pdata->x_line);
	mxt_write_object(data, MXT_TOUCH_MULTI_T9, MXT_TOUCH_YSIZE,
			pdata->y_line);

	/* Set touchscreen orient */
	mxt_write_object(data, MXT_TOUCH_MULTI_T9, MXT_TOUCH_ORIENT,
			pdata->orient);

	/* Set touchscreen burst length */
	mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			MXT_TOUCH_BLEN, pdata->blen);

	/* Set touchscreen threshold */
	mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			MXT_TOUCH_TCHTHR, pdata->threshold);

	/* Set touchscreen resolution */
	mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			MXT_TOUCH_XRANGE_LSB, (pdata->x_size - 1) & 0xff);
	mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			MXT_TOUCH_XRANGE_MSB, (pdata->x_size - 1) >> 8);
	mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			MXT_TOUCH_YRANGE_LSB, (pdata->y_size - 1) & 0xff);
	mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			MXT_TOUCH_YRANGE_MSB, (pdata->y_size - 1) >> 8);

	/* Set touchscreen voltage */
	if (pdata->voltage) {
		if (pdata->voltage < MXT_VOLTAGE_DEFAULT) {
			voltage = (MXT_VOLTAGE_DEFAULT - pdata->voltage) /
				MXT_VOLTAGE_STEP;
			voltage = 0xff - voltage + 1;
		} else
			voltage = (pdata->voltage - MXT_VOLTAGE_DEFAULT) /
				MXT_VOLTAGE_STEP;

		mxt_write_object(data, MXT_SPT_CTECONFIG_T28,
				MXT_CTE_VOLTAGE, voltage);
	}
}

/* Update 24-bit CRC with two new bytes of data */
static u32 crc24_step(u32 crc, u8 byte1, u8 byte2)
{
	const u32 crcpoly = 0x80001b;
	u16 data = byte1 | (byte2 << 8);
	u32 result = data ^ (crc << 1);

	/* XOR result with crcpoly if bit 25 is set (overflow occurred) */
	if (result & 0x01000000)
		result ^= crcpoly;

	return result & 0x00ffffff;
}

static u32 crc24(u32 crc, const u8 *data, size_t len)
{
	size_t i;

	for (i = 0; i < len - 1; i += 2)
		crc = crc24_step(crc, data[i], data[i + 1]);

	/* If there were an odd number of bytes pad with 0 */
	if (i < len)
		crc = crc24_step(crc, data[i], 0);

	return crc;
}

static int mxt_verify_info_block_csum(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct device *dev = &client->dev;
	size_t object_table_size, info_block_size;
	u32 crc = 0;
	u8 *info_block;
	int ret = 0;

	object_table_size = data->info.object_num * MXT_OBJECT_SIZE;
	info_block_size = sizeof(data->info) + object_table_size;
	info_block = kmalloc(info_block_size, GFP_KERNEL);
	if (!info_block)
		return -ENOMEM;

	/*
	 * Information Block CRC is computed over both ID info and Object Table
	 * So concat them in a temporary buffer, before computing CRC.
	 * TODO: refactor how the info block is read from the device such
	 * that it ends up in a single buffer and this copy is not needed.
	 */
	memcpy(info_block, &data->info, sizeof(data->info));
	memcpy(&info_block[sizeof(data->info)], data->object_table,
			object_table_size);

	crc = crc24(crc, info_block, info_block_size);

	if (crc != data->info_csum) {
		dev_err(dev, "Information Block CRC mismatch: %06x != %06x\n",
			data->info_csum, crc);
		ret = -EINVAL;
	}

	kfree(info_block);
	return ret;
}

static int mxt_get_info(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;

	/* Read 7-byte info block starting at address 0 */
	error = __mxt_read_reg(client, MXT_INFO, sizeof(*info), info);
	if (error)
		return error;

	return 0;
}

static int mxt_get_object_table(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct device *dev = &data->client->dev;
	size_t table_size;
	int error;
	int i;
	u8 reportid;
	u8 csum[3];

	table_size = data->info.object_num * sizeof(struct mxt_object);
	error = __mxt_read_reg(client, MXT_OBJECT_START, table_size,
			data->object_table);
	if (error)
		return error;

	/*
	 * Read Information Block checksum from 3 bytes immediately following
	 * info block
	 */
	error = __mxt_read_reg(client, MXT_OBJECT_START + table_size,
			sizeof(csum), csum);
	if (error)
		return error;

	data->info_csum = csum[0] | (csum[1] << 8) | (csum[2] << 16);
	dev_info(dev, "Information Block Checksum = %06x\n", data->info_csum);

	error = mxt_verify_info_block_csum(data);
	if (error)
		return error;

	/* Valid Report IDs start counting from 1 */
	reportid = 1;
	for (i = 0; i < data->info.object_num; i++) {
		struct mxt_object *object = data->object_table + i;
		u8 min_id, max_id;

		le16_to_cpus(&object->start_address);

		if (object->num_report_ids) {
			min_id = reportid;
			reportid += object->num_report_ids *
					(object->instances + 1);
			max_id = reportid - 1;
		} else {
			min_id = 0;
			max_id = 0;
		}

		dev_dbg(&data->client->dev,
			"Type %2d Start %3d Size %3d Instances %2d ReportIDs %3u : %3u\n",
			object->type, object->start_address, object->size + 1,
			object->instances + 1, min_id, max_id);

		switch (object->type) {
		case MXT_GEN_COMMAND_T6:
			data->T6_reportid = min_id;
			break;
		case MXT_TOUCH_MULTI_T9:
			data->T9_reportid_min = min_id;
			data->T9_reportid_max = max_id;
			break;
		}
	}

	return 0;
}

static void mxt_free_object_table(struct mxt_data *data)
{
	kfree(data->object_table);
	data->object_table = NULL;
	data->T6_reportid = 0;
	data->T9_reportid_min = 0;
	data->T9_reportid_max = 0;

}

static int mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;
	u8 val;

	error = mxt_get_info(data);
	if (error)
		return error;

	data->object_table = kcalloc(info->object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
	if (!data->object_table) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Get object table information */
	error = mxt_get_object_table(data);
	if (error)
		goto err_free_object_table;

	/* Check register init values */
	error = mxt_check_reg_init(data);
	if (error)
		goto err_free_object_table;

	mxt_handle_pdata(data);

	/* Backup to memory */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_BACKUPNV,
			MXT_BACKUP_VALUE);
	msleep(MXT_BACKUP_TIME);

	/* Soft reset */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_RESET, 1);
	msleep(MXT_RESET_TIME);

	/* Update matrix size at info struct */
	error = mxt_read_reg(client, MXT_MATRIX_X_SIZE, &val);
	if (error)
		goto err_free_object_table;
	info->matrix_xsize = val;

	error = mxt_read_reg(client, MXT_MATRIX_Y_SIZE, &val);
	if (error)
		goto err_free_object_table;
	info->matrix_ysize = val;

	dev_info(&client->dev,
			"Family ID: %u Variant ID: %u Major.Minor.Build: %u.%u.%02X\n",
			info->family_id, info->variant_id, info->version >> 4,
			info->version & 0xf, info->build);

	dev_info(&client->dev,
			"Matrix X Size: %u Matrix Y Size: %u Object Num: %u\n",
			info->matrix_xsize, info->matrix_ysize,
			info->object_num);

	return 0;

err_free_object_table:
	mxt_free_object_table(data);
	return error;
}

static void mxt_calc_resolution(struct mxt_data *data)
{
	unsigned int max_x = data->pdata->x_size - 1;
	unsigned int max_y = data->pdata->y_size - 1;

	if (data->pdata->orient & MXT_XY_SWITCH) {
		data->max_x = max_y;
		data->max_y = max_x;
	} else {
		data->max_x = max_x;
		data->max_y = max_y;
	}
}

static ssize_t mxt_calibrate_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret;

	disable_irq(data->irq);

	/* Perform touch surface recalibration */
	ret = mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_CALIBRATE, 1);
	if (ret)
		goto out;
	msleep(MXT_CAL_TIME);

out:
	enable_irq(data->irq);
	return ret ?: count;
}

static ssize_t mxt_config_csum_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%06x\n", data->config_csum);
}

/* Firmware Version is returned as Major.Minor.Build */
static ssize_t mxt_fw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_info *info = &data->info;
	return scnprintf(buf, PAGE_SIZE, "%u.%u.%02X\n",
			 info->version >> 4, info->version & 0xf, info->build);
}

/* Hardware Version is returned as FamilyID.VariantID */
static ssize_t mxt_hw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_info *info = &data->info;
	return scnprintf(buf, PAGE_SIZE, "%u.%u\n",
			 info->family_id, info->variant_id);
}

static ssize_t mxt_info_csum_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%06x\n", data->info_csum);
}

static ssize_t mxt_show_instance(char *buf, int count,
				 struct mxt_object *object, int instance,
				 const u8 *val)
{
	int i;

	if (object->instances > 0)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "Instance %u\n", instance);

	for (i = 0; i < object->size + 1; i++)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				"\t[%2u]: %02x (%d)\n", i, val[i], val[i]);
	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

	return count;
}

static ssize_t mxt_object_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	int count = 0;
	int i, j;
	int error;
	u8 *obuf;

	/* Pre-allocate buffer large enough to hold max sized object. */
	obuf = kmalloc(256, GFP_KERNEL);
	if (!obuf)
		return -ENOMEM;

	error = 0;
	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;

		if (!mxt_object_readable(object->type))
			continue;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				"T%u:\n", object->type);

		for (j = 0; j < object->instances + 1; j++) {
			u16 size = object->size + 1;
			u16 addr = object->start_address + j * size;

			error = __mxt_read_reg(data->client, addr, size, obuf);
			if (error)
				goto done;

			count = mxt_show_instance(buf, count, object, j, obuf);
		}
	}

done:
	kfree(obuf);
	return error ?: count;
}

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;

	ret = request_firmware(&fw, fn, dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	}

	ret = mxt_enter_bl(data);
	if (ret) {
		dev_err(dev, "Failed to reset to bootloader.\n");
		goto out;
	}

	ret = mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	ret = mxt_unlock_bootloader(client);
	if (ret)
		goto out;

	while (pos < fw->size) {
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA);
		if (ret)
			goto out;

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;

		/* Write one frame to device */
		ret = mxt_fw_write(client, fw->data + pos, frame_size);
		if (ret)
			goto out;

		ret = mxt_check_bootloader(data, MXT_FRAME_CRC_PASS);
		if (ret)
			goto out;

		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n", pos, fw->size);
	}

	/* Device exits bl mode to app mode only if successful */
	mxt_exit_bl(data);
out:
	release_firmware(fw);

	return ret;
}

static ssize_t mxt_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int error;

	error = mxt_load_fw(dev, MXT_FW_NAME);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		count = error;
	} else {
		dev_dbg(dev, "The firmware update succeeded\n");
	}

	return count;
}

static DEVICE_ATTR(calibrate, S_IWUSR, NULL, mxt_calibrate_store);
static DEVICE_ATTR(config_csum, S_IRUGO, mxt_config_csum_show, NULL);
static DEVICE_ATTR(fw_version, S_IRUGO, mxt_fw_version_show, NULL);
static DEVICE_ATTR(hw_version, S_IRUGO, mxt_hw_version_show, NULL);
static DEVICE_ATTR(info_csum, S_IRUGO, mxt_info_csum_show, NULL);
static DEVICE_ATTR(object, S_IRUGO, mxt_object_show, NULL);
static DEVICE_ATTR(update_fw, S_IWUSR, NULL, mxt_update_fw_store);

static struct attribute *mxt_attrs[] = {
	&dev_attr_calibrate.attr,
	&dev_attr_config_csum.attr,
	&dev_attr_fw_version.attr,
	&dev_attr_hw_version.attr,
	&dev_attr_info_csum.attr,
	&dev_attr_object.attr,
	&dev_attr_update_fw.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

static void mxt_start(struct mxt_data *data)
{
	/* Touch enable */
	mxt_write_object(data,
			MXT_TOUCH_MULTI_T9, MXT_TOUCH_CTRL, 0x83);
}

static void mxt_stop(struct mxt_data *data)
{
	/* Touch disable */
	mxt_write_object(data,
			MXT_TOUCH_MULTI_T9, MXT_TOUCH_CTRL, 0);
}

static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_start(data);

	return 0;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_stop(data);
}

static int mxt_input_dev_create(struct mxt_data *data)
{
	struct input_dev *input_dev;
	int error;
	unsigned int num_mt_slots;

	data->input_dev = input_dev = input_allocate_device();
	if (!input_dev)
		return -ENOMEM;

	input_dev->name = "Atmel maXTouch Touchscreen";
	input_dev->phys = data->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &data->client->dev;
	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);

	/* For single touch */
	input_set_abs_params(input_dev, ABS_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			     0, data->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE,
			     0, 255, 0, 0);

	/* For multi touch */
	num_mt_slots = data->T9_reportid_max - data->T9_reportid_min + 1;
	error = input_mt_init_slots(input_dev, num_mt_slots, 0);
	if (error)
		goto err_free_device;

	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, MXT_MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, data->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
			     0, 255, 0, 0);

	input_set_drvdata(input_dev, data);

	error = input_register_device(input_dev);
	if (error)
		goto err_free_device;

	return 0;

err_free_device:
	input_free_device(data->input_dev);
	data->input_dev = NULL;
	return error;
}

static int __devinit mxt_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	const struct mxt_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	int error;

	if (!pdata)
		return -EINVAL;

	data = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	snprintf(data->phys, sizeof(data->phys), "i2c-%u-%04x/input0",
		 client->adapter->nr, client->addr);

	data->client = client;
	i2c_set_clientdata(client, data);

	data->pdata = pdata;
	data->irq = client->irq;

	init_completion(&data->bl_completion);

	mxt_calc_resolution(data);

	if (mxt_in_bootloader(data)) {
		dev_info(&client->dev, "Device in bootloader at probe\n");
	} else {
		error = mxt_initialize(data);
		if (error)
			goto err_free_mem;

		error = mxt_input_dev_create(data);
		if (error)
			goto err_free_object;
	}

	error = request_threaded_irq(client->irq, NULL, mxt_interrupt,
				     pdata->irqflags | IRQF_ONESHOT,
				     client->name, data);
	if (error) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		if (mxt_in_bootloader(data))
			goto err_free_mem;
		else
			goto err_unregister_device;
	}

	if (!mxt_in_bootloader(data)) {
		error = mxt_make_highchg(data);
		if (error)
			goto err_free_irq;
	}

	error = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (error)
		goto err_free_irq;

	return 0;

err_free_irq:
	free_irq(client->irq, data);
err_unregister_device:
	input_unregister_device(data->input_dev);
err_free_object:
	kfree(data->object_table);
err_free_mem:
	kfree(data);
	return error;
}

static int mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
	free_irq(data->irq, data);
	if (data->input_dev)
		input_unregister_device(data->input_dev);
	kfree(data->object_table);
	kfree(data);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	if (mxt_in_bootloader(data))
		return 0;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_stop(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	if (mxt_in_bootloader(data))
		return 0;

	/* Soft reset */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_RESET, 1);

	msleep(MXT_RESET_TIME);

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_start(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mxt_pm_ops, mxt_suspend, mxt_resume);

static const struct i2c_device_id mxt_id[] = {
	{ "qt602240_ts", 0 },
	{ "atmel_mxt_ts", 0 },
	{ "mXT224", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mxt_id);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "atmel_mxt_ts",
		.owner	= THIS_MODULE,
		.pm	= &mxt_pm_ops,
	},
	.probe		= mxt_probe,
	.remove		= mxt_remove,
	.id_table	= mxt_id,
};

module_i2c_driver(mxt_driver);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Atmel maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");
