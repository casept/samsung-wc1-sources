/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 * Copyright 2010, 2011 David Jander <david@protonic.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#ifdef CONFIG_INPUT_BOOSTER
#include <linux/input/input_booster.h>
#endif
#include <mach/sec_debug.h>
#include <linux/trm.h>
#ifdef CONFIG_DISPLAY_EARLY_DPMS
#include <drm/exynos_drm.h>
#endif
#ifdef CONFIG_SLEEP_MONITOR
#include <linux/power/sleep_monitor.h>
#endif

struct gpio_button_data {
	struct gpio_keys_button *button;
	struct input_dev *input;
	struct timer_list timer;
	struct work_struct work;
	unsigned int timer_debounce;	/* in msecs */
	unsigned int irq;
	spinlock_t lock;
	bool disabled;
	bool key_pressed;
	bool key_state;
	bool wakeup;
	bool isr_status;
#ifdef KEY_BOOSTER
	bool key_dvfs_lock_status;
	bool dvfs_signal;
	struct delayed_work key_work_dvfs_off;
	struct delayed_work key_work_dvfs_chg;
	struct mutex key_dvfs_lock;
	struct pm_qos_request cpu_qos;
	struct pm_qos_request mif_qos;
	struct pm_qos_request int_qos;
#endif
};

struct gpio_keys_drvdata {
	struct input_dev *input;
	struct device *sec_key;
	struct mutex disable_lock;
	unsigned int n_buttons;
	int (*enable)(struct device *dev);
	void (*disable)(struct device *dev);
	bool resume_state;
#ifdef CONFIG_SLEEP_MONITOR
	u32 press_cnt;
#endif
#ifdef CONFIG_SENSORS_HALL
	int gpio_flip_cover;
	int irq_flip_cover;
	bool flip_cover;
	struct delayed_work flip_cover_dwork;
	struct wake_lock flip_wake_lock;
#endif
	struct wake_lock         key_wake_lock;
	struct gpio_button_data data[0];
};

#define WAKELOCK_TIME		HZ/10

static struct gpio_keys_drvdata *global_ddata;

void gpio_keys_send_fake_powerkey(int value)
{
	struct input_dev *input = global_ddata->input;

	input_event(input, EV_KEY, KEY_POWER, value);
	input_sync(input);

	dev_info(&input->dev, "%s: [%s] KEY_POWER\n",
			__func__, value ? "P":"R");
}
EXPORT_SYMBOL(gpio_keys_send_fake_powerkey);

/*
 * SYSFS interface for enabling/disabling keys and switches:
 *
 * There are 4 attributes under /sys/devices/platform/gpio-keys/
 *	keys [ro]              - bitmap of keys (EV_KEY) which can be
 *	                         disabled
 *	switches [ro]          - bitmap of switches (EV_SW) which can be
 *	                         disabled
 *	disabled_keys [rw]     - bitmap of keys currently disabled
 *	disabled_switches [rw] - bitmap of switches currently disabled
 *
 * Userland can change these values and hence disable event generation
 * for each key (or switch). Disabling a key means its interrupt line
 * is disabled.
 *
 * For example, if we have following switches set up as gpio-keys:
 *	SW_DOCK = 5
 *	SW_CAMERA_LENS_COVER = 9
 *	SW_KEYPAD_SLIDE = 10
 *	SW_FRONT_PROXIMITY = 11
 * This is read from switches:
 *	11-9,5
 * Next we want to disable proximity (11) and dock (5), we write:
 *	11,5
 * to file disabled_switches. Now proximity and dock IRQs are disabled.
 * This can be verified by reading the file disabled_switches:
 *	11,5
 * If we now want to enable proximity (11) switch we write:
 *	5
 * to disabled_switches.
 *
 * We can disable only those keys which don't allow sharing the irq.
 */

/**
 * get_n_events_by_type() - returns maximum number of events per @type
 * @type: type of button (%EV_KEY, %EV_SW)
 *
 * Return value of this function can be used to allocate bitmap
 * large enough to hold all bits for given type.
 */
static inline int get_n_events_by_type(int type)
{
	BUG_ON(type != EV_SW && type != EV_KEY);

	return (type == EV_KEY) ? KEY_CNT : SW_CNT;
}

/**
 * gpio_keys_disable_button() - disables given GPIO button
 * @bdata: button data for button to be disabled
 *
 * Disables button pointed by @bdata. This is done by masking
 * IRQ line. After this function is called, button won't generate
 * input events anymore. Note that one can only disable buttons
 * that don't share IRQs.
 *
 * Make sure that @bdata->disable_lock is locked when entering
 * this function to avoid races when concurrent threads are
 * disabling buttons at the same time.
 */
static void gpio_keys_disable_button(struct gpio_button_data *bdata)
{
	if (!bdata->disabled) {
		/*
		 * Disable IRQ and possible debouncing timer.
		 */
		disable_irq(bdata->irq);
		if (bdata->timer_debounce)
			del_timer_sync(&bdata->timer);

		bdata->disabled = true;
	}
}

/**
 * gpio_keys_enable_button() - enables given GPIO button
 * @bdata: button data for button to be disabled
 *
 * Enables given button pointed by @bdata.
 *
 * Make sure that @bdata->disable_lock is locked when entering
 * this function to avoid races with concurrent threads trying
 * to enable the same button at the same time.
 */
static void gpio_keys_enable_button(struct gpio_button_data *bdata)
{
	if (bdata->disabled) {
		enable_irq(bdata->irq);
		bdata->disabled = false;
	}
}

/**
 * gpio_keys_attr_show_helper() - fill in stringified bitmap of buttons
 * @ddata: pointer to drvdata
 * @buf: buffer where stringified bitmap is written
 * @type: button type (%EV_KEY, %EV_SW)
 * @only_disabled: does caller want only those buttons that are
 *                 currently disabled or all buttons that can be
 *                 disabled
 *
 * This function writes buttons that can be disabled to @buf. If
 * @only_disabled is true, then @buf contains only those buttons
 * that are currently disabled. Returns 0 on success or negative
 * errno on failure.
 */
static ssize_t gpio_keys_attr_show_helper(struct gpio_keys_drvdata *ddata,
					  char *buf, unsigned int type,
					  bool only_disabled)
{
	int n_events = get_n_events_by_type(type);
	unsigned long *bits;
	ssize_t ret;
	int i;

	bits = kcalloc(BITS_TO_LONGS(n_events), sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->type != type)
			continue;

		if (only_disabled && !bdata->disabled)
			continue;

		__set_bit(bdata->button->code, bits);
	}

	ret = bitmap_scnlistprintf(buf, PAGE_SIZE - 2, bits, n_events);
	buf[ret++] = '\n';
	buf[ret] = '\0';

	kfree(bits);

	return ret;
}

/**
 * gpio_keys_attr_store_helper() - enable/disable buttons based on given bitmap
 * @ddata: pointer to drvdata
 * @buf: buffer from userspace that contains stringified bitmap
 * @type: button type (%EV_KEY, %EV_SW)
 *
 * This function parses stringified bitmap from @buf and disables/enables
 * GPIO buttons accordingly. Returns 0 on success and negative error
 * on failure.
 */
static ssize_t gpio_keys_attr_store_helper(struct gpio_keys_drvdata *ddata,
					   const char *buf, unsigned int type)
{
	int n_events = get_n_events_by_type(type);
	unsigned long *bits;
	ssize_t error;
	int i;

	bits = kcalloc(BITS_TO_LONGS(n_events), sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	error = bitmap_parselist(buf, bits, n_events);
	if (error)
		goto out;

	/* First validate */
	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->type != type)
			continue;

		if (test_bit(bdata->button->code, bits) &&
		    !bdata->button->can_disable) {
			error = -EINVAL;
			goto out;
		}
	}

	mutex_lock(&ddata->disable_lock);

	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->type != type)
			continue;

		if (test_bit(bdata->button->code, bits))
			gpio_keys_disable_button(bdata);
		else
			gpio_keys_enable_button(bdata);
	}

	mutex_unlock(&ddata->disable_lock);

out:
	kfree(bits);
	return error;
}

#define ATTR_SHOW_FN(name, type, only_disabled)				\
static ssize_t gpio_keys_show_##name(struct device *dev,		\
				     struct device_attribute *attr,	\
				     char *buf)				\
{									\
	struct platform_device *pdev = to_platform_device(dev);		\
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);	\
									\
	return gpio_keys_attr_show_helper(ddata, buf,			\
					  type, only_disabled);		\
}

ATTR_SHOW_FN(keys, EV_KEY, false);
ATTR_SHOW_FN(switches, EV_SW, false);
ATTR_SHOW_FN(disabled_keys, EV_KEY, true);
ATTR_SHOW_FN(disabled_switches, EV_SW, true);

/*
 * ATTRIBUTES:
 *
 * /sys/devices/platform/gpio-keys/keys [ro]
 * /sys/devices/platform/gpio-keys/switches [ro]
 */
static DEVICE_ATTR(keys, S_IRUGO, gpio_keys_show_keys, NULL);
static DEVICE_ATTR(switches, S_IRUGO, gpio_keys_show_switches, NULL);

#define ATTR_STORE_FN(name, type)					\
static ssize_t gpio_keys_store_##name(struct device *dev,		\
				      struct device_attribute *attr,	\
				      const char *buf,			\
				      size_t count)			\
{									\
	struct platform_device *pdev = to_platform_device(dev);		\
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);	\
	ssize_t error;							\
									\
	error = gpio_keys_attr_store_helper(ddata, buf, type);		\
	if (error)							\
		return error;						\
									\
	return count;							\
}

ATTR_STORE_FN(disabled_keys, EV_KEY);
ATTR_STORE_FN(disabled_switches, EV_SW);

/*
 * ATTRIBUTES:
 *
 * /sys/devices/platform/gpio-keys/disabled_keys [rw]
 * /sys/devices/platform/gpio-keys/disables_switches [rw]
 */
static DEVICE_ATTR(disabled_keys, S_IWUSR | S_IRUGO,
		   gpio_keys_show_disabled_keys,
		   gpio_keys_store_disabled_keys);
static DEVICE_ATTR(disabled_switches, S_IWUSR | S_IRUGO,
		   gpio_keys_show_disabled_switches,
		   gpio_keys_store_disabled_switches);

static struct attribute *gpio_keys_attrs[] = {
	&dev_attr_keys.attr,
	&dev_attr_switches.attr,
	&dev_attr_disabled_keys.attr,
	&dev_attr_disabled_switches.attr,
	NULL,
};

static struct attribute_group gpio_keys_attr_group = {
	.attrs = gpio_keys_attrs,
};

static ssize_t key_pressed_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int i;
	int keystate = 0;

	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];
		keystate |= bdata->key_state;
	}

	if (keystate)
		sprintf(buf, "PRESS");
	else
		sprintf(buf, "RELEASE");

	return strlen(buf);
}

/* the volume keys can be the wakeup keys in special case */
static ssize_t wakeup_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int n_events = get_n_events_by_type(EV_KEY);
	unsigned long *bits;
	ssize_t error;
	int i;

	bits = kcalloc(BITS_TO_LONGS(n_events),
		sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	error = bitmap_parselist(buf, bits, n_events);
	if (error)
		goto out;

	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *button = &ddata->data[i];
		if (test_bit(button->button->code, bits))
			button->button->wakeup = 1;
		else
			button->button->wakeup = 0;
	}

out:
	kfree(bits);
	return count;
}

#ifdef CONFIG_SENSORS_HALL
static ssize_t hall_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);

	if (ddata->flip_cover)
		sprintf(buf, "OPEN");
	else
		sprintf(buf, "CLOSE");

	return strlen(buf);
}
#endif

static DEVICE_ATTR(sec_key_pressed, 0664, key_pressed_show, NULL);
static DEVICE_ATTR(wakeup_keys, 0664, NULL, wakeup_enable);
#ifdef CONFIG_SENSORS_HALL
static DEVICE_ATTR(hall_detect, 0664, hall_detect_show, NULL);
#endif

static struct attribute *sec_key_attrs[] = {
	&dev_attr_sec_key_pressed.attr,
	&dev_attr_wakeup_keys.attr,
#ifdef CONFIG_SENSORS_HALL
	&dev_attr_hall_detect.attr,
#endif
	NULL,
};

static struct attribute_group sec_key_attr_group = {
	.attrs = sec_key_attrs,
};

#ifdef KEY_BOOSTER

#define set_qos(req, pm_qos_class, value) { \
	if (pm_qos_request_active(req)) \
		pm_qos_update_request(req, value); \
	else \
		pm_qos_add_request(req, pm_qos_class, value); \
}

#define remove_qos(req) { \
	if (pm_qos_request_active(req)) \
		pm_qos_remove_request(req); \
}

static void gpio_key_change_dvfs_lock(struct work_struct *work)
{
	struct gpio_button_data *bdata =
			container_of(work,
				struct gpio_button_data, key_work_dvfs_chg.work);

	mutex_lock(&bdata->key_dvfs_lock);

	set_qos(&bdata->cpu_qos, PM_QOS_CPU_FREQ_MIN, KEY_BOOSTER_CPU_FREQ1);
	set_qos(&bdata->mif_qos, PM_QOS_BUS_THROUGHPUT, KEY_BOOSTER_MIF_FREQ1);
	set_qos(&bdata->int_qos, PM_QOS_DEVICE_THROUGHPUT, KEY_BOOSTER_INT_FREQ1);

	bdata->key_dvfs_lock_status = true;
	bdata->dvfs_signal = false;
	printk(KERN_DEBUG"keys:DVFS On\n");

	mutex_unlock(&bdata->key_dvfs_lock);
}

static void gpio_key_set_dvfs_off(struct work_struct *work)
{
	struct gpio_button_data *bdata =
				container_of(work,
					struct gpio_button_data, key_work_dvfs_off.work);

	mutex_lock(&bdata->key_dvfs_lock);

	remove_qos(&bdata->cpu_qos);
	remove_qos(&bdata->mif_qos);
	remove_qos(&bdata->int_qos);

	bdata->key_dvfs_lock_status = false;
	bdata->dvfs_signal = false;
	mutex_unlock(&bdata->key_dvfs_lock);

	printk(KERN_DEBUG "keys:DVFS Off\n");
}

static void gpio_key_set_dvfs_lock(struct gpio_button_data *bdata,
					uint32_t on)
{
	mutex_lock(&bdata->key_dvfs_lock);
	if (on == 0) {
		if (bdata->dvfs_signal) {
			cancel_delayed_work(&bdata->key_work_dvfs_chg);
			schedule_delayed_work(&bdata->key_work_dvfs_chg, 0);
			schedule_delayed_work(&bdata->key_work_dvfs_off,
				msecs_to_jiffies(KEY_BOOSTER_OFF_TIME));
		} else if (bdata->key_dvfs_lock_status) {
			schedule_delayed_work(&bdata->key_work_dvfs_off,
				msecs_to_jiffies(KEY_BOOSTER_OFF_TIME));
		}
	} else if (on == 1) {
		cancel_delayed_work(&bdata->key_work_dvfs_off);
		if (!bdata->key_dvfs_lock_status && !bdata->dvfs_signal) {
			schedule_delayed_work(&bdata->key_work_dvfs_chg,
							msecs_to_jiffies(KEY_BOOSTER_ON_TIME));
			bdata->dvfs_signal = true;
		}
	} else if (on == 2) {
		if (bdata->key_dvfs_lock_status) {
			cancel_delayed_work(&bdata->key_work_dvfs_off);
			cancel_delayed_work(&bdata->key_work_dvfs_chg);
			schedule_work(&bdata->key_work_dvfs_off.work);
		}
	}
	mutex_unlock(&bdata->key_dvfs_lock);
}


static int gpio_key_init_dvfs(struct gpio_button_data *bdata)
{
	mutex_init(&bdata->key_dvfs_lock);

	INIT_DELAYED_WORK(&bdata->key_work_dvfs_off, gpio_key_set_dvfs_off);
	INIT_DELAYED_WORK(&bdata->key_work_dvfs_chg, gpio_key_change_dvfs_lock);

	bdata->key_dvfs_lock_status = false;
	return 0;
}
#endif

static void gpio_keys_gpio_report_event(struct gpio_button_data *bdata)
{
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);
	unsigned int type = button->type ?: EV_KEY;
	struct irq_desc *desc = irq_to_desc(gpio_to_irq(button->gpio));
	int state = (gpio_get_value_cansleep(button->gpio) ? 1 : 0) ^ button->active_low;
#ifdef CONFIG_DISPLAY_EARLY_DPMS
	struct display_early_dpms_nb_event event;
#endif

	if (type == EV_ABS) {
		if (state) {
			input_event(input, type, button->code, button->value);
			input_sync(input);
		}
	} else {
		wake_lock_timeout(&ddata->key_wake_lock, WAKELOCK_TIME);

		if (irqd_is_wakeup_set(&desc->irq_data)) {
			if (!state && !bdata->key_state) {
#ifdef CONFIG_SLP_KERNEL_ENG
				pr_info("%s:[%s][%s][A]\n", __func__, !state ? "P":"R",
					button->desc);
#else
				pr_info("gpio_keys:[%s][A]\n", !state ? "P":"R");
#endif
				input_event(input, type, button->code, !state);
				input_sync(input);
			}
		}

		input_event(input, type, button->code, !!state);
		input_sync(input);

#if defined(HARD_KEY_BOOSTER)
		if (((button->code == KEY_POWER) ||(button->code == KEY_BACK))\
		    && (state != 0)) {
			hard_key_booster_turn_on();
		}
#endif

#ifdef KEY_BOOSTER
		if (button->code == KEY_HOMEPAGE)
			gpio_key_set_dvfs_lock(bdata, !!state);
#endif
#ifdef CONFIG_INPUT_BOOSTER
		if (button->code == KEY_HOMEPAGE)
			INPUT_BOOSTER_SEND_EVENT(KEY_HOMEPAGE, !!state);
#endif
	}
#ifdef CONFIG_SLP_KERNEL_ENG
	pr_info("%s:[%s][%s][%s]\n", __func__, !!state ? "P":"R",
		button->desc, bdata->isr_status ? "I":"R");
#else
	pr_info("gpio_keys:[%s][%s]\n", !!state ? "P":"R",
		bdata->isr_status ? "I":"R");
#endif
#ifdef CONFIG_SLEEP_MONITOR
	if ((ddata->press_cnt < 0xffff) && (bdata->isr_status) && (!!state))
		ddata->press_cnt++;
#endif
#ifdef CONFIG_DISPLAY_EARLY_DPMS
	if ((!ddata->resume_state) && (bdata->isr_status)) {
		event.id = DISPLAY_EARLY_DPMS_ID_PRIMARY;
		event.data = (void *)true;
		display_early_dpms_nb_send_event(DISPLAY_EARLY_DPMS_MODE_SET,
			(void *)&event);
	}
#endif
	bdata->isr_status = false;
	bdata->key_state = !!state;
	sec_debug_check_crash_key(button->code, !!state);
}


static void gpio_keys_gpio_work_func(struct work_struct *work)
{
	struct gpio_button_data *bdata =
		container_of(work, struct gpio_button_data, work);

	gpio_keys_gpio_report_event(bdata);
}

static void gpio_keys_gpio_timer(unsigned long _data)
{
	struct gpio_button_data *bdata = (struct gpio_button_data *)_data;

	schedule_work(&bdata->work);
}

static irqreturn_t gpio_keys_gpio_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	const struct gpio_keys_button *button = bdata->button;
	int state;

	BUG_ON(irq != bdata->irq);

	bdata->isr_status = true;
	if (bdata->timer_debounce)
		mod_timer(&bdata->timer,
			jiffies + msecs_to_jiffies(bdata->timer_debounce));
	else
		schedule_work_on(0, &bdata->work);

	if (button->isr_hook) {
		state = (gpio_get_value_cansleep(button->gpio) ? 1 : 0) \
				^ button->active_low;
		button->isr_hook(button->code, state);
	}

	return IRQ_HANDLED;
}

static void gpio_keys_irq_timer(unsigned long _data)
{
	struct gpio_button_data *bdata = (struct gpio_button_data *)_data;
	struct input_dev *input = bdata->input;
	unsigned long flags;

	spin_lock_irqsave(&bdata->lock, flags);
	if (bdata->key_pressed) {
		input_event(input, EV_KEY, bdata->button->code, 0);
		input_sync(input);
		bdata->key_pressed = false;
	}
	spin_unlock_irqrestore(&bdata->lock, flags);
}

static irqreturn_t gpio_keys_irq_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned long flags;
	int state = (gpio_get_value_cansleep(button->gpio) ? 1 : 0) \
		^ button->active_low;

	BUG_ON(irq != bdata->irq);

	spin_lock_irqsave(&bdata->lock, flags);

	if (!bdata->key_pressed) {
		input_event(input, EV_KEY, button->code, 1);
		input_sync(input);

		if (!bdata->timer_debounce) {
			input_event(input, EV_KEY, button->code, 0);
			input_sync(input);
			goto out;
		}

		bdata->key_pressed = true;
	}

	if (bdata->timer_debounce)
		mod_timer(&bdata->timer,
			jiffies + msecs_to_jiffies(bdata->timer_debounce));
out:
	if (button->isr_hook)
		button->isr_hook(button->code, state);

	spin_unlock_irqrestore(&bdata->lock, flags);
	return IRQ_HANDLED;
}

static int __devinit gpio_keys_setup_key(struct platform_device *pdev,
					 struct input_dev *input,
					 struct gpio_button_data *bdata,
					 struct gpio_keys_button *button)
{
	const char *desc = button->desc ? button->desc : "gpio_keys";
	struct device *dev = &pdev->dev;
	irq_handler_t isr;
	unsigned long irqflags;
	int irq, error;

	bdata->input = input;
	bdata->button = button;
	spin_lock_init(&bdata->lock);

	if (gpio_is_valid(button->gpio)) {

		error = gpio_request(button->gpio, desc);
		if (error < 0) {
			dev_err(dev, "Failed to request GPIO %d, error %d\n",
				button->gpio, error);
			return error;
		}

		error = gpio_direction_input(button->gpio);
		if (error < 0) {
			dev_err(dev,
				"Failed to configure direction for GPIO %d, error %d\n",
				button->gpio, error);
			goto fail;
		}

		if (button->debounce_interval) {
			error = gpio_set_debounce(button->gpio,
					button->debounce_interval * 1000);
			/* use timer if gpiolib doesn't provide debounce */
			if (error < 0)
				bdata->timer_debounce =
						button->debounce_interval;
		}

		irq = gpio_to_irq(button->gpio);
		if (irq < 0) {
			error = irq;
			dev_err(dev,
				"Unable to get irq number for GPIO %d, error %d\n",
				button->gpio, error);
			goto fail;
		}
		bdata->irq = irq;

		INIT_WORK(&bdata->work, gpio_keys_gpio_work_func);
		setup_timer(&bdata->timer,
			    gpio_keys_gpio_timer, (unsigned long)bdata);

		isr = gpio_keys_gpio_isr;
		irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	} else {
		if (!button->irq) {
			dev_err(dev, "No IRQ specified\n");
			return -EINVAL;
		}
		bdata->irq = button->irq;

		if (button->type && button->type != EV_KEY) {
			dev_err(dev, "Only EV_KEY allowed for IRQ buttons.\n");
			return -EINVAL;
		}

		bdata->timer_debounce = button->debounce_interval;
		setup_timer(&bdata->timer,
			    gpio_keys_irq_timer, (unsigned long)bdata);

		isr = gpio_keys_irq_isr;
		irqflags = 0;
	}

	input_set_capability(input, button->type ?: EV_KEY, button->code);

	/*
	 * If platform has specified that the button can be disabled,
	 * we don't want it to share the interrupt line.
	 */
	if (!button->can_disable)
		irqflags |= IRQF_SHARED;

#ifndef CONFIG_MACH_WC1
	if (button->wakeup)
		irqflags |= IRQF_NO_SUSPEND;
#endif

	error = request_any_context_irq(bdata->irq, isr, irqflags, desc, bdata);
	if (error < 0) {
		dev_err(dev, "Unable to claim irq %d; error %d\n",
			bdata->irq, error);
		goto fail;
	}

	return 0;

fail:
	if (gpio_is_valid(button->gpio))
		gpio_free(button->gpio);

	return error;
}

#ifdef CONFIG_SENSORS_HALL
#ifdef CONFIG_SEC_FACTORY
static void flip_cover_work(struct work_struct *work)
{
	bool first,second;
	struct gpio_keys_drvdata *ddata =
		container_of(work, struct gpio_keys_drvdata,
				flip_cover_dwork.work);

	first = gpio_get_value(ddata->gpio_flip_cover);

	printk(KERN_DEBUG "keys:%s #1 : %d\n",
		__func__, first);

	msleep(50);

	second = gpio_get_value(ddata->gpio_flip_cover);

	printk(KERN_DEBUG "keys:%s #2 : %d\n",
		__func__, second);

	if(first == second && ddata->flip_cover != first) {
		ddata->flip_cover = first;
		input_report_switch(ddata->input,
			SW_FLIP, ddata->flip_cover);
		input_sync(ddata->input);
	}
}
#else
static void flip_cover_work(struct work_struct *work)
{
	bool first;
	struct gpio_keys_drvdata *ddata =
		container_of(work, struct gpio_keys_drvdata,
				flip_cover_dwork.work);

	first = gpio_get_value(ddata->gpio_flip_cover);

	printk(KERN_DEBUG "keys:%s #1 : %d\n",
		__func__, first);

	if(ddata->flip_cover != first) {
		ddata->flip_cover = first;
		input_report_switch(ddata->input,
			SW_FLIP, ddata->flip_cover);
		input_sync(ddata->input);
	}
}
#endif

static void __flip_cover_detect(struct gpio_keys_drvdata *ddata, bool flip_status)
{
	cancel_delayed_work_sync(&ddata->flip_cover_dwork);
#ifdef CONFIG_SEC_FACTORY
	schedule_delayed_work(&ddata->flip_cover_dwork, HZ / 20);
#else
	if(flip_status)	{
		wake_lock_timeout(&ddata->flip_wake_lock, HZ * 45 / 100);
		schedule_delayed_work(&ddata->flip_cover_dwork, HZ * 4 / 10);
	} else {
		wake_unlock(&ddata->flip_wake_lock);
		schedule_delayed_work(&ddata->flip_cover_dwork, 0);
	}
#endif
}

static irqreturn_t flip_cover_detect(int irq, void *dev_id)
{
	bool flip_status;
	struct gpio_keys_drvdata *ddata = dev_id;

	flip_status = gpio_get_value(ddata->gpio_flip_cover);

	printk(KERN_DEBUG "keys:%s flip_status : %d\n",
		 __func__, flip_status);

	__flip_cover_detect(ddata, flip_status);

	return IRQ_HANDLED;
}
#endif /* CONFIG_SENSORS_HALL */


static int gpio_keys_open(struct input_dev *input)
{
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);
#ifdef CONFIG_SENSORS_HALL
	/* update the current status */
	schedule_delayed_work(&ddata->flip_cover_dwork, HZ / 2);
#endif
	return ddata->enable ? ddata->enable(input->dev.parent) : 0;
}

static void gpio_keys_close(struct input_dev *input)
{
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);
#if defined (KEY_BOOSTER) || defined (CONFIG_INPUT_BOOSTER)
	int i = 0;
#endif

	if (ddata->disable)
		ddata->disable(input->dev.parent);

#ifdef CONFIG_SENSORS_HALL
//	cancel_delayed_work_sync(&ddata->flip_cover_dwork);
#endif
#if defined (KEY_BOOSTER) || defined (CONFIG_INPUT_BOOSTER)
	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->code == KEY_HOMEPAGE) {
#ifdef CONFIG_INPUT_BOOSTER
			INPUT_BOOSTER_SEND_EVENT(KEY_HOMEPAGE,
				BOOSTER_MODE_FORCE_OFF);
#else
			gpio_key_set_dvfs_lock(bdata, 2);
#endif
		}
	}
#endif
}

/*
 * Handlers for alternative sources of platform_data
 */
#ifdef CONFIG_OF
/*
 * Translate OpenFirmware node properties into platform_data
 */
static int gpio_keys_get_devtree_pdata(struct device *dev,
			    struct gpio_keys_platform_data *pdata)
{
	struct device_node *node, *pp;
	int i;
	struct gpio_keys_button *buttons;
	u32 reg;

	node = dev->of_node;
	if (node == NULL)
		return -ENODEV;

	memset(pdata, 0, sizeof *pdata);

	pdata->rep = !!of_get_property(node, "autorepeat", NULL);

	/* First count the subnodes */
	pdata->nbuttons = 0;
	pp = NULL;
	while ((pp = of_get_next_child(node, pp)))
		pdata->nbuttons++;

	if (pdata->nbuttons == 0)
		return -ENODEV;

	buttons = kzalloc(pdata->nbuttons * (sizeof *buttons), GFP_KERNEL);
	if (!buttons)
		return -ENOMEM;

	pp = NULL;
	i = 0;
	while ((pp = of_get_next_child(node, pp))) {
		enum of_gpio_flags flags;

		if (!of_find_property(pp, "gpios", NULL)) {
			pdata->nbuttons--;
			dev_warn(dev, "Found button without gpios\n");
			continue;
		}
		buttons[i].gpio = of_get_gpio_flags(pp, 0, &flags);
		buttons[i].active_low = flags & OF_GPIO_ACTIVE_LOW;

		if (of_property_read_u32(pp, "linux,code", &reg)) {
			dev_err(dev, "Button without keycode: 0x%x\n", buttons[i].gpio);
			goto out_fail;
		}
		buttons[i].code = reg;

		buttons[i].desc = of_get_property(pp, "label", NULL);

		if (of_property_read_u32(pp, "linux,input-type", &reg) == 0)
			buttons[i].type = reg;
		else
			buttons[i].type = EV_KEY;

		buttons[i].wakeup = !!of_get_property(pp, "gpio-key,wakeup", NULL);

		if (of_property_read_u32(pp, "debounce-interval", &reg) == 0)
			buttons[i].debounce_interval = reg;
		else
			buttons[i].debounce_interval = 5;

		i++;
	}

	pdata->buttons = buttons;

	return 0;

out_fail:
	kfree(buttons);
	return -ENODEV;
}

static struct of_device_id gpio_keys_of_match[] = {
	{ .compatible = "gpio-keys", },
	{ },
};
MODULE_DEVICE_TABLE(of, gpio_keys_of_match);

#else

static int gpio_keys_get_devtree_pdata(struct device *dev,
			    struct gpio_keys_platform_data *altp)
{
	return -ENODEV;
}

#define gpio_keys_of_match NULL

#endif

static void gpio_remove_key(struct gpio_button_data *bdata)
{
	free_irq(bdata->irq, bdata);
	if (bdata->timer_debounce)
		del_timer_sync(&bdata->timer);
	cancel_work_sync(&bdata->work);
	if (gpio_is_valid(bdata->button->gpio))
		gpio_free(bdata->button->gpio);
}

#ifdef CONFIG_SENSORS_HALL
static void init_hall_ic_irq(struct input_dev *input)
{
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);

	int ret = 0;
	int irq = gpio_to_irq(ddata->gpio_flip_cover);

	INIT_DELAYED_WORK(&ddata->flip_cover_dwork, flip_cover_work);

	ret =
		request_threaded_irq(
		irq, NULL,
		flip_cover_detect,
		IRQF_DISABLED | IRQF_TRIGGER_RISING |
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"flip_cover", ddata);
	if (ret < 0) {
		printk(KERN_ERR
		"keys: failed to request flip cover irq %d gpio %d\n",
		irq, ddata->gpio_flip_cover);
	} else {
		pr_info("%s : success\n", __func__);
	}
}
#endif

#ifdef CONFIG_SLEEP_MONITOR
#define	PRETTY_MAX	14
#define	STATE_BIT	24
#define	CNT_MARK	0xffff
#define	STATE_MARK	0xff
static int gpio_keys_get_sleep_monitor_cb(void* priv, unsigned int *raw_val, int check_level, int caller_type);

static struct sleep_monitor_ops  gpio_keys_sleep_monitor_ops = {
	 .read_cb_func =  gpio_keys_get_sleep_monitor_cb,
};

static int gpio_keys_get_sleep_monitor_cb(void* priv, unsigned int *raw_val, int check_level, int caller_type)
{
	struct gpio_keys_drvdata *ddata = priv;
	struct input_dev *input = ddata->input;
	int state = DEVICE_UNKNOWN;
	int pretty = 0;

	if ((check_level == SLEEP_MONITOR_CHECK_SOFT) ||\
	    (check_level == SLEEP_MONITOR_CHECK_HARD)) {
		if (ddata->resume_state)
			state = DEVICE_ON_ACTIVE1;
		else
			state = DEVICE_POWER_OFF;
	}

	*raw_val = ((state & STATE_MARK) << STATE_BIT) |\
			(ddata->press_cnt & CNT_MARK);

	if (ddata->press_cnt > PRETTY_MAX)
		pretty = PRETTY_MAX;
	else
		pretty = ddata->press_cnt;

	ddata->press_cnt = 0;

	dev_dbg(&input->dev, "%s: raw_val[0x%08x], check_level[%d], state[%d], pretty[%d]\n",
		__func__, *raw_val, check_level, state, pretty);

	return pretty;
}
#endif

static int __devinit gpio_keys_probe(struct platform_device *pdev)
{
	const struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_drvdata *ddata;
	struct device *dev = &pdev->dev;
	struct gpio_keys_platform_data alt_pdata;
	struct input_dev *input;
	int i, error;
	int wakeup = 0;

	if (!pdata) {
		error = gpio_keys_get_devtree_pdata(dev, &alt_pdata);
		if (error)
			return error;
		pdata = &alt_pdata;
	}

	ddata = kzalloc(sizeof(struct gpio_keys_drvdata) +
			pdata->nbuttons * sizeof(struct gpio_button_data),
			GFP_KERNEL);
	input = input_allocate_device();
	if (!ddata || !input) {
		dev_err(dev, "failed to allocate state\n");
		error = -ENOMEM;
		goto fail1;
	}
	global_ddata = ddata;
	ddata->input = input;
	ddata->n_buttons = pdata->nbuttons;
	ddata->enable = pdata->enable;
	ddata->disable = pdata->disable;
#ifdef CONFIG_SENSORS_HALL
	ddata->gpio_flip_cover = pdata->gpio_flip_cover;
	ddata->irq_flip_cover = gpio_to_irq(ddata->gpio_flip_cover);;

	wake_lock_init(&ddata->flip_wake_lock, WAKE_LOCK_SUSPEND,
		"flip wake lock");
#endif
	mutex_init(&ddata->disable_lock);
	wake_lock_init(&ddata->key_wake_lock,
			WAKE_LOCK_SUSPEND, "gpio-keys_wake_lock");

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = pdata->name ? : pdev->name;
	input->phys = "gpio-keys/input0";
	input->dev.parent = &pdev->dev;
#ifdef CONFIG_SENSORS_HALL
	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_FLIP);
#endif
	input->open = gpio_keys_open;
	input->close = gpio_keys_close;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < pdata->nbuttons; i++) {
		struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_button_data *bdata = &ddata->data[i];

		error = gpio_keys_setup_key(pdev, input, bdata, button);
		if (error)
			goto fail2;

		if (button->wakeup)
			wakeup = 1;

#ifdef KEY_BOOSTER
		if (button->code == KEY_HOMEPAGE) {
			error = gpio_key_init_dvfs(bdata);
			if (error < 0) {
				dev_err(dev, "Fail get dvfs level for touch booster\n");
				goto fail2;
			}
		}
#endif
	}

	error = sysfs_create_group(&pdev->dev.kobj, &gpio_keys_attr_group);
	if (error) {
		dev_err(dev, "Unable to export keys/switches, error: %d\n",
			error);
		goto fail2;
	}

	ddata->sec_key =
	    device_create(sec_class, NULL, 0, ddata, "sec_key");
	if (IS_ERR(ddata->sec_key))
		dev_err(dev, "Failed to create sec_key device\n");

	error = sysfs_create_group(&ddata->sec_key->kobj, &sec_key_attr_group);
	if (error) {
		dev_err(dev, "Failed to create the test sysfs: %d\n",
			error);
		goto fail2;
	}

#ifdef CONFIG_SENSORS_HALL
	init_hall_ic_irq(input);
#endif
	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail3;
	}

	/* get current state of buttons */
	for (i = 0; i < pdata->nbuttons; i++)
		gpio_keys_gpio_report_event(&ddata->data[i]);
	input_sync(input);

	device_init_wakeup(&pdev->dev, wakeup);
	ddata->resume_state = true;

#ifdef CONFIG_DISPLAY_EARLY_DPMS
	device_set_early_complete(&pdev->dev, EARLY_COMP_SLAVE);
#endif
#ifdef CONFIG_SLEEP_MONITOR
	sleep_monitor_register_ops(ddata, &gpio_keys_sleep_monitor_ops,
			SLEEP_MONITOR_KEY);
#endif
	return 0;

 fail3:
	sysfs_remove_group(&pdev->dev.kobj, &gpio_keys_attr_group);
	sysfs_remove_group(&ddata->sec_key->kobj, &sec_key_attr_group);
 fail2:
	while (--i >= 0)
		gpio_remove_key(&ddata->data[i]);

	platform_set_drvdata(pdev, NULL);
	wake_lock_destroy(&ddata->key_wake_lock);
#ifdef CONFIG_SENSORS_HALL
	wake_lock_destroy(&ddata->flip_wake_lock);
#endif
 fail1:
	input_free_device(input);
	kfree(ddata);
	/* If we have no platform_data, we allocated buttons dynamically. */
	if (!pdev->dev.platform_data)
		kfree(pdata->buttons);

	return error;
}

static int __devexit gpio_keys_remove(struct platform_device *pdev)
{
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;
	int i;

#ifdef CONFIG_SLEEP_MONITOR
	sleep_monitor_unregister_ops(SLEEP_MONITOR_KEY);
#endif

	sysfs_remove_group(&pdev->dev.kobj, &gpio_keys_attr_group);

	device_init_wakeup(&pdev->dev, 0);

	for (i = 0; i < ddata->n_buttons; i++)
		gpio_remove_key(&ddata->data[i]);

	input_unregister_device(input);
	wake_lock_destroy(&ddata->key_wake_lock);
#ifdef CONFIG_SENSORS_HALL
	wake_lock_destroy(&ddata->flip_wake_lock);
#endif
	/*
	 * If we had no platform_data, we allocated buttons dynamically, and
	 * must free them here. ddata->data[0].button is the pointer to the
	 * beginning of the allocated array.
	 */
	if (!pdev->dev.platform_data)
		kfree(ddata->data[0].button);

	kfree(ddata);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int gpio_keys_suspend(struct device *dev)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int i;

	if (device_may_wakeup(dev)) {
		for (i = 0; i < ddata->n_buttons; i++) {
			struct gpio_button_data *bdata = &ddata->data[i];
			if (bdata->button->wakeup)
				enable_irq_wake(bdata->irq);
		}
#ifdef CONFIG_SENSORS_HALL
		enable_irq_wake(ddata->irq_flip_cover);
#endif
	}
	ddata->resume_state = false;
	return 0;
}

static int gpio_keys_resume(struct device *dev)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < ddata->n_buttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];
		if (bdata->button->wakeup && device_may_wakeup(dev))
			disable_irq_wake(bdata->irq);

		if (gpio_is_valid(bdata->button->gpio))
			gpio_keys_gpio_report_event(bdata);
	}
	input_sync(ddata->input);
	ddata->resume_state = true;
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(gpio_keys_pm_ops, gpio_keys_suspend, gpio_keys_resume);

static struct platform_driver gpio_keys_device_driver = {
	.probe		= gpio_keys_probe,
	.remove		= __devexit_p(gpio_keys_remove),
	.driver		= {
		.name	= "gpio-keys",
		.owner	= THIS_MODULE,
		.pm	= &gpio_keys_pm_ops,
		.of_match_table = gpio_keys_of_match,
	}
};

static int __init gpio_keys_init(void)
{
	return platform_driver_register(&gpio_keys_device_driver);
}

static void __exit gpio_keys_exit(void)
{
	platform_driver_unregister(&gpio_keys_device_driver);
}

late_initcall(gpio_keys_init);
module_exit(gpio_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for GPIOs");
MODULE_ALIAS("platform:gpio-keys");
