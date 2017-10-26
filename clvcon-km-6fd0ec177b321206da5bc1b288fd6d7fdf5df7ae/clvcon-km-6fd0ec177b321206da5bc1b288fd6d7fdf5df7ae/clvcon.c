/* --------------------------------------------------------------------------
 * @license_begin
 *
 *  Nintendo CLV/Wii Classic/Wii Pro controllers I2C driver
 *  Copyright (C) 2016  Nintendo Co. Ltd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @license_end
 * ----------------------------------------------------------------------- */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/bug.h>

MODULE_AUTHOR("Nintendo Co., Ltd");
MODULE_DESCRIPTION("Nintendo CLV-001/CLV-002/Wii Classic/Wii Pro controllers on I2C");

MODULE_LICENSE("GPL");

#define DRV_NAME "clvcon"

#define CLASSIC_ID             0
#define CONTROLLER_I2C_ADDRESS 0x52
//Rounded up to multiple of 1/HZ
#define POLL_INTERVAL          5

// Confirm bit flips before reporting digital button values
// Beware this implies a one polling interval of input lag
#define FILTER_DIGITAL_BUTTON 0

/* HOME button is to be reported only after these many successful polling
 * positives.
 * Keep it at ~0.1s, adapt whenever POLL_INTERVAL is changed */
#define HOME_BUTTON_THRESHOLD 15

//Delay expressed in polling intervals
#define RETRY_BASE_DELAY 512

#define DATA_FORMAT    3

#define DF3_BTN_R      1
#define DF3_BTN_START  2
#define DF3_BTN_HOME   3
#define DF3_BTN_SELECT 4
#define DF3_BTN_L      5
#define DF3_BTN_DOWN   6
#define DF3_BTN_RIGHT  7

#define DF3_BTN_UP     0
#define DF3_BTN_LEFT   1
#define DF3_BTN_ZR     2
#define DF3_BTN_X      3
#define DF3_BTN_A      4
#define DF3_BTN_Y      5
#define DF3_BTN_B      6
#define DF3_BTN_ZL     7

#define DEAD_ZONE      20
#define DIAG_MAX       40
#define STICK_MAX      72
#define STICK_FUZZ     4

#define TRIGGER_MIN    0
#define TRIGGER_MAX    0xff
#define TRIGGER_FUZZ   4

#define MAX_CON_COUNT  4

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#define CLVCON_DETECT_USE_IRQ 0

#if CLVCON_DETECT_USE_IRQ
#define INVAL_IRQ            -1
#define DETECT_DELAY         (HZ / 10)
#else
#define DETECT_DELAY         (HZ / 5)
#endif

#define DEBOUNCE_VALUE       0x71

static struct delayed_work detect_work;

static DEFINE_MUTEX(con_state_lock);
static DEFINE_MUTEX(detect_task_lock);

#define VERBOSITY        1

#if VERBOSITY > 0
	#define ERR(m, ...) printk(KERN_ERR  "clvcon error: " m "\n", ##__VA_ARGS__)
	#define INF(m, ...) printk(KERN_INFO  "clvcon info: " m "\n", ##__VA_ARGS__)
#else
	#define ERR(m, ...)
	#define INF(m, ...)
#endif

#if VERBOSITY > 1
	#define DBG(m, ...) printk(KERN_DEBUG  "clvcon: " m "\n", ##__VA_ARGS__)
	#if VERBOSITY > 2
		#define FAST_ERR(m, ...) ERR(m, ##__VA_ARGS__)
		#define FAST_DBG(m, ...) DBG(m, ##__VA_ARGS__)
	#else
		#define FAST_ERR(m, ...) trace_printk(KERN_ERR  "clvcon error: " m "\n", ##__VA_ARGS__)
		#define FAST_DBG(m, ...) trace_printk(KERN_DEBUG  "clvcon: " m "\n", ##__VA_ARGS__)
	#endif
#else
	#define DBG(m, ...)
	#define FAST_ERR(m, ...)
	#define FAST_DBG(m, ...)
#endif

enum ControllerState {
	CS_OK,
	CS_FAST_RETRY, // reconnect after typical noise timespan, 2*15ms -> 4 polling periods at 150Hz
	CS_RETRY_1,
	CS_RETRY_2,
	CS_ERR
};

static bool get_bit(u8 data, int bitnum) {
	return (data & (1 << bitnum)) >> bitnum;
}

static const struct i2c_device_id clvcon_idtable[] = {
	{ "classic", CLASSIC_ID },
	{}
};

MODULE_DEVICE_TABLE(i2c, clvcon_idtable);

struct clvcon_button_state
{
	bool report;
	int changed;
};

struct clvcon_info {
	struct input_polled_dev *dev;
	struct i2c_client *client;
	struct i2c_adapter *adapter;
#if CLVCON_DETECT_USE_IRQ
	int irq;
#endif
	int detection_active;
	int gpio;
	int id;
	enum ControllerState state;
	u16 retry_counter;
	int home_counter;
	struct clvcon_button_state s6[8];
	struct clvcon_button_state s7[8];
};

static struct clvcon_info con_info_list[MAX_CON_COUNT];
static int module_params[2 * MAX_CON_COUNT];
static int arr_argc = 0;

#define CON_NAME_PREFIX "Nintendo Clovercon - controller"
const char *controller_names[] = {CON_NAME_PREFIX"1", CON_NAME_PREFIX"2",
                                  CON_NAME_PREFIX"3", CON_NAME_PREFIX"4"};

module_param_array(module_params, int, &arr_argc, 0000);
MODULE_PARM_DESC(module_params, "Input info in the form con0_i2c_bus, con0_detect_gpio, "
	                            "con1_i2c_bus, con1_detect_gpio, ... gpio < 0 means no detection");

#if CLVCON_DETECT_USE_IRQ
struct clvcon_info * clvcon_info_from_irq(int irq) {
	int i;
	for (i = 0; i < MAX_CON_COUNT; i++) {
		if (con_info_list[i].irq == irq) {
			return &con_info_list[i];
		}
	}
	return NULL;
}
#endif

struct clvcon_info * clvcon_info_from_adapter(struct i2c_adapter *adapter) {
	int i;
	for (i = 0; i < MAX_CON_COUNT; i++) {
		if (con_info_list[i].adapter == adapter) {
			return &con_info_list[i];
		}
	}
	return NULL;
}

static int clvcon_write(struct i2c_client *client, u8 *data, size_t count) {
	struct i2c_msg msg = {
		.addr = client->addr,
		.len = count,
		.buf = data
	};
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		//read and write errors are DBG only to account for normal
		//errors coming from disconnects
		DBG("write failed with error %i", ret);
		return ret;
	} else if (ret != 1) {
		DBG("incomplete write, return value: %i", ret);
		return -EIO;
	}

	return 0;
}

static int clvcon_prepare_read(struct i2c_client *client, u8 address) {
	int ret;

	ret = clvcon_write(client, &address, 1);
	if (ret) {
		DBG("prepare_read failed");
		return ret;
	}

	return 0;
}

static int clvcon_read(struct i2c_client *client, u8 address, u8 *values, size_t count) {
	struct i2c_msg data_msg = {
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = count,
		.buf = values
	};
	int ret;

	ret = clvcon_prepare_read(client, address);
	if (ret)
		return ret;

	// Wait > 150µs before prepare_read and actual read
	usleep_range(150, 200);

	ret = i2c_transfer(client->adapter, &data_msg, 1);
	if (ret < 0) {
		DBG("read failed with error %i", ret);
		return ret;
	} else if (ret != 1) {
		DBG("incomplete read, return value: %i", ret);
		return -EIO;
	}

	return 0;
}

static int clvcon_read_controller_info(struct i2c_client *client, u8 *data, size_t len) {
	int ret;

	len = MIN(len, 0xff -0xfa + 1);
	ret = clvcon_read(client, 0xfa, data, len);
	if (ret)
		return ret;

	return len;
	// print_hex_dump(KERN_DEBUG, "Controller info data: " , DUMP_PREFIX_NONE, 16, 256, data, len, false);
}

static int clvcon_setup(struct i2c_client *client) {
	u8 init_data[] = { 0xf0, 0x55, 0xfb, 0x00, 0xfe, DATA_FORMAT };
	static const int CON_INFO_LEN = 6;
	u8 con_info_data[CON_INFO_LEN];
	int ret;

	DBG("clvcon setup");

	ret = clvcon_write(client, &init_data[0], 2);
	if (ret)
		goto err;
	ret = clvcon_write(client, &init_data[2], 2);
	if (ret)
		goto err;
	ret = clvcon_write(client, &init_data[4], 2);
	if (ret)
		goto err;

	ret = clvcon_read_controller_info(client, con_info_data, CON_INFO_LEN);
	if (ret < 0) {
		ERR("error reading controller info");
		goto err;
	} else if (ret != CON_INFO_LEN) {
		ERR("wrong read length %i reading controller info", ret);
		ret = -EIO;
		goto err;
	}
	if (con_info_data[4] != DATA_FORMAT) {
		ERR("failed to set data format, value is %i", (int)con_info_data[4]);
		ret = -EIO;
		goto err;
	}
	if (con_info_data[5] != 1) {
		ERR("unsupported controller id %i", (int)con_info_data[5]);
		ret = -EIO;
		goto err;
	}

	return 0;

err:
	ERR("controller setup failed with error %i", -ret);
	return ret;
}

static void clamp_stick(int *px, int *py) {
	int x_sign = 1;
	int y_sign = 1;
	int x = *px;
	int y = *py;
	int norm;

	if (x < 0) {
		x_sign = -1;
		x = -x;
	}
	if (y < 0) {
		y_sign = -1;
		y = -y;
	}

	x = MAX(0, x - DEAD_ZONE);
	y = MAX(0, y - DEAD_ZONE);

	if (x == 0 && y == 0) {
		goto clamp_end;
	}

	norm = (y <= x) ? (DIAG_MAX * x + (STICK_MAX - DIAG_MAX) * y) :
	                  (DIAG_MAX * y + (STICK_MAX - DIAG_MAX) * x);

	if (DIAG_MAX * STICK_MAX < norm) {
		x = DIAG_MAX * STICK_MAX * x / norm;
		y = DIAG_MAX * STICK_MAX * y / norm;
	}

clamp_end:
	*px = x * x_sign;
	*py = y * y_sign;
}

/* When DC noise is kicking in, we want to avoid the driver to report wrong
 * btn state changes.
 *
 * This is specially important for example to avoid switching to the menui
 * spuriously because of noisy home button events.
 *
 * The following function will filter out as many state changes as the module
 * is configured to before actually reporting the change
 *
 * NB: even the Mini NES controller can report the home button event even
 * though no physical button is present. The MCU is the same as the
 * classic/pro controller.
 */

static inline void clvcon_df3_btn(struct clvcon_button_state* btn, const u8 payload, const int id)
{
	bool v;

	v = !get_bit(payload, id);
	btn += id;

#if FILTER_DIGITAL_BUTTON
	if (btn->report != v) {
		btn->changed++;
		if (btn->changed > 1) {
			btn->report = !btn->report;
			btn->changed = 0;
		}
	} else {
		btn->changed = 0;
	}
#else
        btn->report = v;
#endif
}

/* The Wii could trust the payload 100% because the Wii Remote
 * controller is powered with AA batteries. In the case of
 * CLV, high voltage on DC power supply has shown very noisy
 * payloads coming out of the I²C bus. But we don't have any
 * checksum or parity bits to discard such corrupted payloads.
 *
 * However the payload zero padding should be impacted by the
 * DC noise and some high bits should pop up there too.
 *
 * This function reports a noisy zero padding area
 */
static int inline clvcon_df3_zero_padding_is_noisy(const u8* payload)
{
	int i;
	int up = 0;
	for (i=8; i<21; i++) {
		up += payload[i];
	}
	return up;
}

static inline void clvcon_info_reset_button_states(struct clvcon_info* info) {
	info->home_counter = 0;
	memset(info->s6, 0, sizeof(info->s6));
	memset(info->s7, 0, sizeof(info->s7));
}

static void clvcon_poll(struct input_polled_dev *polled_dev) {
	struct clvcon_info *info = polled_dev->private;
	static const int READ_LEN = 21;
	u8 data[READ_LEN];
	int jx, jy, rx, ry, tl, tr;
	u16 retry_delay = RETRY_BASE_DELAY;
	int ret;

	switch (info->state) {
	case CS_OK:
		ret = clvcon_read(info->client, 0, data, READ_LEN);
		if (ret) {
			DBG("read failed for active controller - possible controller disconnect");
			INF("moving controller %i to error state", info->id);
			/* When DC noise kicks in, reading from I²C becomes unreliable
			 * typical noise bursts have a 15ms duration every 300ms and
			 * reconnecting after ~30ms would be enough. However hardware
			 * team reported the error level in their test environment
			 * would be too high.
			 *
			 * So it's impossible to just use CS_FAST_RETRY as the plan was
			 * Stick to the very conservative value of CS_RETRY_1, leading
			 * to a 850ms reconnection delay. */
			info->state = CS_RETRY_1;
			break;
		}

		//print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 16, 256, data, READ_LEN, false);

		/* Use noise presence in the zero padding as discard criteri
		 * for the data packet */
		if (clvcon_df3_zero_padding_is_noisy(data))
			break;

		// Report analog sticks / buttons
		jx = data[0] - 0x80;
		rx = data[1] - 0x80;
		jy = 0x7fl - data[2];
		ry = 0x7fl - data[3];
		tl = data[4];
		tr = data[5];

		clamp_stick(&jx, &jy);
		clamp_stick(&rx, &ry);

		input_report_abs(polled_dev->input, ABS_X, jx);
		input_report_abs(polled_dev->input, ABS_Y, jy);
		input_report_abs(polled_dev->input, ABS_RX, rx);
		input_report_abs(polled_dev->input, ABS_RY, ry);
		input_report_abs(polled_dev->input, ABS_Z, tl);
		input_report_abs(polled_dev->input, ABS_RZ, tr);

		// Report digital buttons from byte6
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_R);
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_START);
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_HOME);
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_SELECT);
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_L);
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_DOWN);
		clvcon_df3_btn(info->s6, data[6], DF3_BTN_RIGHT);

		if (info->s6[DF3_BTN_HOME].report) {
			info->home_counter++;
			if (info->home_counter>HOME_BUTTON_THRESHOLD) {
				info->home_counter = HOME_BUTTON_THRESHOLD;
			}
		} else {
			info->home_counter = 0;
		}

		input_report_key(polled_dev->input, BTN_TR, info->s6[DF3_BTN_R].report);
		input_report_key(polled_dev->input, BTN_START, info->s6[DF3_BTN_START].report);
		input_report_key(polled_dev->input, BTN_MODE, (info->home_counter>=HOME_BUTTON_THRESHOLD));
		input_report_key(polled_dev->input, BTN_SELECT, info->s6[DF3_BTN_SELECT].report);
		input_report_key(polled_dev->input, BTN_TL, info->s6[DF3_BTN_L].report);
		input_report_key(polled_dev->input, BTN_TRIGGER_HAPPY4, info->s6[DF3_BTN_DOWN].report);
		input_report_key(polled_dev->input, BTN_TRIGGER_HAPPY2, info->s6[DF3_BTN_RIGHT].report);

		// Report digital buttons from byte7
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_UP);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_LEFT);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_ZR);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_X);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_A);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_Y);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_B);
		clvcon_df3_btn(info->s7, data[7], DF3_BTN_ZL);

		input_report_key(polled_dev->input, BTN_TRIGGER_HAPPY3, info->s7[DF3_BTN_UP].report);
		input_report_key(polled_dev->input, BTN_TRIGGER_HAPPY1, info->s7[DF3_BTN_LEFT].report);
		input_report_key(polled_dev->input, BTN_TR2, info->s7[DF3_BTN_ZR].report);
		input_report_key(polled_dev->input, BTN_X, info->s7[DF3_BTN_X].report);
		input_report_key(polled_dev->input, BTN_A, info->s7[DF3_BTN_A].report);
		input_report_key(polled_dev->input, BTN_Y, info->s7[DF3_BTN_Y].report);
		input_report_key(polled_dev->input, BTN_B, info->s7[DF3_BTN_B].report);
		input_report_key(polled_dev->input, BTN_TL2, info->s7[DF3_BTN_ZL].report);

		input_sync(polled_dev->input);

		break;
	case CS_FAST_RETRY:
		retry_delay /= 32;
		//fall-through
	case CS_RETRY_1:
		retry_delay /= 2;
		//fall-through
	case CS_RETRY_2:
		retry_delay /= 2;
		//fall-through
	case CS_ERR:
		info->retry_counter++;
		if (info->retry_counter == retry_delay) {
			DBG("retrying controller setup");
			ret = clvcon_setup(info->client);
			if (ret) {
				info->state = MIN(CS_ERR, info->state + 1);
			} else {
				info->state = CS_OK;
				clvcon_info_reset_button_states(info);
				INF("setup succeeded for controller %i, moving to OK state", info->id);
			}
			info->retry_counter = 0;
		}
		break;
	default:
		info->state = CS_ERR;
	}
}

static void clvcon_open(struct input_polled_dev *polled_dev) {
	struct clvcon_info *info = polled_dev->private;
	if (clvcon_setup(info->client)) {
		info->retry_counter = 0;
		info->state = CS_RETRY_1;
		INF("opened controller %i, controller in error state after failed setup", info->id);
	} else {
		info->state = CS_OK;
		clvcon_info_reset_button_states(info);
		INF("opened controller %i, controller in OK state", info->id);
	}
}

static int clvcon_probe(struct i2c_client *client, const struct i2c_device_id *id) {
	struct clvcon_info *info;
	struct input_polled_dev *polled_dev;
	struct input_dev *input_dev;
	int ret = 0;

	switch (id->driver_data) {
	case CLASSIC_ID:
		DBG("probing classic controller");
		break;
	default:
		ERR("unknown id: %lu\n", id->driver_data);
		return -EINVAL;
	}

	mutex_lock(&con_state_lock);
	info = clvcon_info_from_adapter(client->adapter);
	if (!info) {
		ERR("unkonwn client passed to probe");
		mutex_unlock(&con_state_lock);
		return -EINVAL;
	}
	info->client = client;
	i2c_set_clientdata(client, info);
	mutex_unlock(&con_state_lock);

	polled_dev = input_allocate_polled_device();
	if (!polled_dev) {
		ERR("error allocating polled device");
		return -ENOMEM;
	}

	info->dev = polled_dev;

	polled_dev->poll_interval = POLL_INTERVAL;
	polled_dev->poll = clvcon_poll;
	polled_dev->open = clvcon_open;
	polled_dev->private = info;

	input_dev = polled_dev->input;

	//change controller_names initializer when changing MAX_CON_COUNT
	BUILD_BUG_ON(MAX_CON_COUNT != ARRAY_SIZE(controller_names)); 
	input_dev->name = controller_names[info->id - 1];
	input_dev->phys = DRV_NAME"/clvcon";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	set_bit(EV_ABS, input_dev->evbit);
	set_bit(ABS_X,  input_dev->absbit);
	set_bit(ABS_Y,  input_dev->absbit);
	set_bit(ABS_RX, input_dev->absbit);
	set_bit(ABS_RY, input_dev->absbit);
	/*
	L/R are analog on the classic controller, digital
	on the pro with values 0 - 0xf8
	*/
	set_bit(ABS_Z,  input_dev->absbit);
	set_bit(ABS_RZ, input_dev->absbit);

	set_bit(EV_KEY,     input_dev->evbit);
	set_bit(BTN_X,      input_dev->keybit);
	set_bit(BTN_B,      input_dev->keybit);
	set_bit(BTN_A,      input_dev->keybit);
	set_bit(BTN_Y,      input_dev->keybit);
	set_bit(BTN_TRIGGER_HAPPY3, input_dev->keybit); // up
	set_bit(BTN_TRIGGER_HAPPY4, input_dev->keybit); // down
	set_bit(BTN_TRIGGER_HAPPY2, input_dev->keybit); // right
	set_bit(BTN_TRIGGER_HAPPY1, input_dev->keybit); // left
	set_bit(BTN_TR,     input_dev->keybit);
	set_bit(BTN_TL,     input_dev->keybit);
	set_bit(BTN_TR2,    input_dev->keybit);
	set_bit(BTN_TL2,    input_dev->keybit);
	set_bit(BTN_SELECT, input_dev->keybit);
	set_bit(BTN_START,  input_dev->keybit);
	set_bit(BTN_MODE,   input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X,  -STICK_MAX, STICK_MAX, STICK_FUZZ, 0);
	input_set_abs_params(input_dev, ABS_Y,  -STICK_MAX, STICK_MAX, STICK_FUZZ, 0);
	input_set_abs_params(input_dev, ABS_RX, -STICK_MAX, STICK_MAX, STICK_FUZZ, 0);
	input_set_abs_params(input_dev, ABS_RY, -STICK_MAX, STICK_MAX, STICK_FUZZ, 0);
	input_set_abs_params(input_dev, ABS_Z,  TRIGGER_MIN, TRIGGER_MAX, TRIGGER_FUZZ, 0);
	input_set_abs_params(input_dev, ABS_RZ, TRIGGER_MIN, TRIGGER_MAX, TRIGGER_FUZZ, 0);

	ret = input_register_polled_device(polled_dev);
	if (ret) {
		ERR("registering polled device failed");
		goto err_register_polled_dev;
	}

	INF("probed controller %i", info->id);

	return 0;

err_register_polled_dev:
	input_free_polled_device(polled_dev);

	return ret;
}

static int clvcon_remove(struct i2c_client *client) {
	struct clvcon_info *info;
	struct input_polled_dev *polled_dev;
	
	mutex_lock(&con_state_lock);
	info = i2c_get_clientdata(client);
	polled_dev = info->dev;
	info->dev = NULL;
	mutex_unlock(&con_state_lock);

	input_unregister_polled_device(polled_dev);
	input_free_polled_device(polled_dev);

	INF("removed controller %i", info->id);

	return 0;
}

static struct i2c_driver clvcon_driver = {
	.driver = {
		.name	= "clvcon",
		.owner = THIS_MODULE,
	},

	.id_table	= clvcon_idtable,
	.probe		= clvcon_probe,
	.remove		= clvcon_remove,
};

static struct i2c_board_info clvcon_i2c_board_info = {
	I2C_BOARD_INFO("classic", CONTROLLER_I2C_ADDRESS),
};

/* Must be holding con_state_lock */
int clvcon_add_controller(struct clvcon_info *info) {
	struct i2c_client *client;

	mutex_unlock(&con_state_lock);
	client = i2c_new_device(info->adapter, &clvcon_i2c_board_info);
	mutex_lock(&con_state_lock);
	if (!client) {
		ERR("could not create i2c device");
		return -ENOMEM;
	}

	INF("added device for controller %i", info->id);
	return 0;
}

/* Must be holding con_state_lock */
void clvcon_remove_controller(struct clvcon_info *info) {
	struct i2c_client *client = info->client;

	mutex_unlock(&con_state_lock);
	i2c_unregister_device(client);
	mutex_lock(&con_state_lock);
	info->client = NULL;

	INF("removed device for controller %i", info->id);
}

static void clvcon_remove_controllers(void) {
	int i;

	mutex_lock(&con_state_lock);
	for (i = 0; i < arr_argc / 2; i++) {
		if (!con_info_list[i].client) {
			continue;
		}
		clvcon_remove_controller(&con_info_list[i]);
	}
	mutex_unlock(&con_state_lock);
}

static void clvcon_detect_task(struct work_struct *dummy) {
	struct clvcon_info *info;
	int i;
	int val;

	mutex_lock(&detect_task_lock);
	DBG("detect task running");
	mutex_lock(&con_state_lock);
	for (i = 0; i < MAX_CON_COUNT; i++) {
		info = &con_info_list[i];
		if (!info->detection_active) {
			continue;
		}
		val = gpio_get_value(info->gpio);
		DBG("detect pin value: %i", val);
		if (val && !info->client) {
			DBG("detect task adding controller %i", i);
			clvcon_add_controller(info);
		} else if (!val && info->client) {
			DBG("detect task removing controller %i", i);
			clvcon_remove_controller(info);
		}
	}
	mutex_unlock(&con_state_lock);
	mutex_unlock(&detect_task_lock);
	DBG("detect task done");
}

#if CLVCON_DETECT_USE_IRQ

static irqreturn_t clvcon_detect_interrupt(int irq, void* dummy) {
	struct clvcon_info *info = clvcon_info_from_irq(irq);
	static int initialized = 0;

	if (info == NULL) {
		FAST_ERR("could not find controller info associated with irq %i", irq);
		return IRQ_HANDLED;
	}

	if (initialized == 0) {
		INIT_DELAYED_WORK(&detect_work, clvcon_detect_task);
		initialized = 1;
	} else {
		PREPARE_DELAYED_WORK(&detect_work, clvcon_detect_task);
	}

	schedule_delayed_work(&detect_work, DETECT_DELAY);

	FAST_DBG("interrupt handler on int %i", irq);
	return IRQ_HANDLED;
}

static int clvcon_setup_irq_detect(struct clvcon_info *info) {
	int irq;
	int ret;

	ret = gpio_to_irq(info->gpio);
	if (ret < 0) {
		ERR("gpio to irq failed");
		return ret;
	} else {
		irq = ret;
		DBG("irq for gpio %i: %i", info->gpio, irq);
	}

	mutex_lock(&con_state_lock);
	info->irq = irq;
	info->detection_active = 1;
	mutex_unlock(&con_state_lock);

	ret = request_irq(ret, clvcon_detect_interrupt, IRQ_TYPE_EDGE_BOTH, "clvcon", NULL);
	if (ret) {
		ERR("failed to request irq");
		return ret;
	}

	return 0;
}

static void clvcon_teardown_irq_detect(struct clvcon_info *info) {
	free_irq(info->irq, NULL);
	mutex_lock(&con_state_lock);
	info->detection_active = 0;
	info->irq = INVAL_IRQ;
	mutex_unlock(&con_state_lock);
}

#else //CLVCON_DETECT_USE_IRQ

static void clvcon_detect_timer_task(struct work_struct *dummy) {
	static int initialized = 0;

	if (initialized == 0) {
		INIT_DELAYED_WORK(&detect_work, clvcon_detect_timer_task);
		initialized = 1;
	} else {
		PREPARE_DELAYED_WORK(&detect_work, clvcon_detect_timer_task);
	}

	clvcon_detect_task(NULL);
	schedule_delayed_work(&detect_work, DETECT_DELAY);
}

static int clvcon_setup_timer_detect(struct clvcon_info *info) {
	static int task_running = 0;

	mutex_lock(&con_state_lock);
	info->detection_active = 1;
	mutex_unlock(&con_state_lock);
	
	if (task_running)
		return 0;

	task_running = 1;
	clvcon_detect_timer_task(NULL);

	return 0;
}

static void clvcon_teardown_timer_detect(struct clvcon_info *info) {
	mutex_lock(&con_state_lock);
	info->detection_active = 0;
	mutex_unlock(&con_state_lock);
}

#endif //CLVCON_DETECT_USE_IRQ

static int clvcon_setup_i2c(struct clvcon_info *info, int i2c_bus) {
	struct i2c_adapter *adapter;

	adapter = i2c_get_adapter(i2c_bus);
	if (!adapter) {
		ERR("could not access i2c bus %i", i2c_bus);
		return -EINVAL;
	}

	info->adapter = adapter;

	return 0;
}

static int clvcon_setup_detection(struct clvcon_info *info, int gpio_pin) {
	int ret;

	ret = gpio_request(gpio_pin, "clvcon_detect");
	if (ret) {
		ERR("gpio request failed for pin %i", gpio_pin);
		return ret;
	}

	ret = gpio_direction_input(gpio_pin);
	if (ret) {
		ERR("gpio input direction failed");
		goto err_gpio_cleanup;
	}

	info->gpio = gpio_pin;

	ret = gpio_set_debounce(gpio_pin, DEBOUNCE_VALUE);
	if (ret) {
		ERR("failed to debounce gpio %i", gpio_pin);
		goto err_gpio_cleanup;
	}

#if CLVCON_DETECT_USE_IRQ
	info->irq = INVAL_IRQ;
	ret = clvcon_setup_irq_detect(info);
#else
	ret = clvcon_setup_timer_detect(info);
#endif

	if (ret) {
		ERR("controller detection setup failed");
		goto err_detect_cleanup;
	}

	return 0;

err_detect_cleanup:
#if CLVCON_DETECT_USE_IRQ
	clvcon_teardown_irq_detect(info);
#else
	clvcon_teardown_timer_detect(info);
#endif

err_gpio_cleanup:
	gpio_free(gpio_pin);
	return ret;
}

static void clvcon_teardown_detection(void) {
	int i;
	int gpio;

	cancel_delayed_work_sync(&detect_work);

	for (i = 0; i < MAX_CON_COUNT; i++) {
		if (!con_info_list[i].detection_active) {
			continue;
		}
#if CLVCON_DETECT_USE_IRQ
		clvcon_teardown_irq_detect(&con_info_list[i]);
#else
		clvcon_teardown_timer_detect(&con_info_list[i]);
#endif
		mutex_lock(&con_state_lock);
		con_info_list[i].adapter = NULL;
		gpio = con_info_list[i].gpio;
		mutex_unlock(&con_state_lock);

		DBG("Freeing gpio %i", gpio);
		gpio_free(gpio);
	}
}

static int __init clvcon_init(void) {
	int i2c_bus;
	int gpio_pin;
	int ret;
	int i;

	for (i = 0; i < MAX_CON_COUNT; i++) {
		con_info_list[i].detection_active = 0;
		con_info_list[i].id = i + 1;
	}

	for (i = 0; i < arr_argc / 2; i++) {
		i2c_bus = module_params[2 * i];
		gpio_pin = module_params[2 * i + 1];

		DBG("initializing controller %i on bus %i, gpio %i", i, i2c_bus, gpio_pin);

		ret = clvcon_setup_i2c(&con_info_list[i], i2c_bus);
		if (ret) {
			ERR("failed to init controller %i", i);
			goto err_controller_cleanup;
		}

		if (gpio_pin < 0) {
			mutex_lock(&con_state_lock);
			ret = clvcon_add_controller(&con_info_list[i]);
			mutex_unlock(&con_state_lock);
		} else {
			ret = clvcon_setup_detection(&con_info_list[i], gpio_pin);
			if (ret) {
				ERR("failed to init controller %i", i);
				goto err_controller_cleanup;
			}
		}
	}

	ret = i2c_add_driver(&clvcon_driver);
	if (ret) {
		ERR("failed to add driver");
		goto err_controller_cleanup;
	}

	return 0;

err_controller_cleanup:
	clvcon_teardown_detection();
	clvcon_remove_controllers();
	return ret;
}

module_init(clvcon_init);

static void __exit clvcon_exit(void) {
	DBG("exit");

	clvcon_teardown_detection();
	clvcon_remove_controllers();
	i2c_del_driver(&clvcon_driver);
}

module_exit(clvcon_exit);
