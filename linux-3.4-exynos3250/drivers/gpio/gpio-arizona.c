/*
 * gpiolib support for Wolfson Arizona class devices
 *
 * Copyright 2014 CirrusLogic, Inc.
 * Copyright 2012 Wolfson Microelectronics PLC.
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>

#include <linux/mfd/arizona/core.h>
#include <linux/mfd/arizona/pdata.h>
#include <linux/mfd/arizona/registers.h>

struct arizona_gpio {
	struct arizona *arizona;
	struct gpio_chip gpio_chip;
};

static inline struct arizona_gpio *to_arizona_gpio(struct gpio_chip *chip)
{
	return container_of(chip, struct arizona_gpio, gpio_chip);
}

static int arizona_gpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;

	return regmap_update_bits(arizona->regmap, ARIZONA_GPIO1_CTRL + offset,
				  ARIZONA_GPN_DIR, ARIZONA_GPN_DIR);
}

static int arizona_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;
	unsigned int val;
	int ret;

	ret = regmap_read(arizona->regmap, ARIZONA_GPIO1_CTRL + offset, &val);
	if (ret < 0)
		return ret;

	if (val & ARIZONA_GPN_LVL)
		return 1;
	else
		return 0;
}

static int arizona_gpio_direction_out(struct gpio_chip *chip,
				     unsigned offset, int value)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;

	if (value)
		value = ARIZONA_GPN_LVL;

	return regmap_update_bits(arizona->regmap, ARIZONA_GPIO1_CTRL + offset,
				  ARIZONA_GPN_DIR | ARIZONA_GPN_LVL, value);
}

static void arizona_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;

	if (value)
		value = ARIZONA_GPN_LVL;

	regmap_update_bits(arizona->regmap, ARIZONA_GPIO1_CTRL + offset,
			   ARIZONA_GPN_LVL, value);
}

static int clearwater_gpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;

	offset *= 2;

	return regmap_update_bits(arizona->regmap, CLEARWATER_GPIO1_CTRL_2 + offset,
				  ARIZONA_GPN_DIR, ARIZONA_GPN_DIR);
}

static int clearwater_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;
	unsigned int val;
	int ret;

	offset *= 2;

	ret = regmap_read(arizona->regmap, CLEARWATER_GPIO1_CTRL_1 + offset, &val);
	if (ret < 0)
		return ret;

	if (val & CLEARWATER_GPN_LVL)
		return 1;
	else
		return 0;
}

static int clearwater_gpio_direction_out(struct gpio_chip *chip,
				     unsigned offset, int value)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;
	int ret;

	offset *= 2;

	if (value)
		value = CLEARWATER_GPN_LVL;

	ret = regmap_update_bits(arizona->regmap, CLEARWATER_GPIO1_CTRL_2 + offset,
				  ARIZONA_GPN_DIR, 0);
	if (ret < 0)
		return ret;

	return regmap_update_bits(arizona->regmap, CLEARWATER_GPIO1_CTRL_1 + offset,
				  CLEARWATER_GPN_LVL, value);
}

static void clearwater_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct arizona_gpio *arizona_gpio = to_arizona_gpio(chip);
	struct arizona *arizona = arizona_gpio->arizona;

	offset *= 2;

	if (value)
		value = CLEARWATER_GPN_LVL;

	regmap_update_bits(arizona->regmap, CLEARWATER_GPIO1_CTRL_1 + offset,
			   CLEARWATER_GPN_LVL, value);
}

static struct gpio_chip template_chip = {
	.label			= "arizona",
	.owner			= THIS_MODULE,
	.direction_input	= arizona_gpio_direction_in,
	.get			= arizona_gpio_get,
	.direction_output	= arizona_gpio_direction_out,
	.set			= arizona_gpio_set,
	.can_sleep		= 1,
};

static int __devinit arizona_gpio_probe(struct platform_device *pdev)
{
	struct arizona *arizona = dev_get_drvdata(pdev->dev.parent);
	struct arizona_pdata *pdata = arizona->dev->platform_data;
	struct arizona_gpio *arizona_gpio;
	int ret;

	arizona_gpio = devm_kzalloc(&pdev->dev, sizeof(*arizona_gpio),
				    GFP_KERNEL);
	if (arizona_gpio == NULL)
		return -ENOMEM;

	arizona_gpio->arizona = arizona;
	arizona_gpio->gpio_chip = template_chip;
	arizona_gpio->gpio_chip.dev = &pdev->dev;

	switch (arizona->type) {
	case WM5102:
	case WM8280:
	case WM5110:
	case WM8997:
	case WM8998:
	case WM1814:
		arizona_gpio->gpio_chip.ngpio = 5;
		break;
	case WM8285:
	case WM1840:
		arizona_gpio->gpio_chip.direction_input =
			clearwater_gpio_direction_in;
		arizona_gpio->gpio_chip.get = clearwater_gpio_get;
		arizona_gpio->gpio_chip.direction_output =
			clearwater_gpio_direction_out;
		arizona_gpio->gpio_chip.set = clearwater_gpio_set;

		arizona_gpio->gpio_chip.ngpio = 40;
		break;
	case WM1831:
	case CS47L24:
		arizona_gpio->gpio_chip.ngpio = 2;
		break;
	default:
		dev_err(&pdev->dev, "Unknown chip variant %d\n",
			arizona->type);
		return -EINVAL;
	}

	if (pdata && pdata->gpio_base)
		arizona_gpio->gpio_chip.base = pdata->gpio_base;
	else
		arizona_gpio->gpio_chip.base = -1;

	ret = gpiochip_add(&arizona_gpio->gpio_chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register gpiochip, %d\n",
			ret);
		goto err;
	}

	platform_set_drvdata(pdev, arizona_gpio);

	return ret;

err:
	return ret;
}

static int __devexit arizona_gpio_remove(struct platform_device *pdev)
{
	struct arizona_gpio *arizona_gpio = platform_get_drvdata(pdev);

	return gpiochip_remove(&arizona_gpio->gpio_chip);
}

static struct platform_driver arizona_gpio_driver = {
	.driver.name	= "arizona-gpio",
	.driver.owner	= THIS_MODULE,
	.probe		= arizona_gpio_probe,
	.remove		= __devexit_p(arizona_gpio_remove),
};

module_platform_driver(arizona_gpio_driver);

MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_DESCRIPTION("GPIO interface for Arizona devices");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:arizona-gpio");
