/*
 * arizona-micsupp.c  --  Microphone supply for Arizona devices
 *
 * Copyright 2012 Wolfson Microelectronics PLC.
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/gpio.h>
#include <linux/slab.h>

#include <linux/mfd/arizona/core.h>
#include <linux/mfd/arizona/pdata.h>
#include <linux/mfd/arizona/registers.h>

#define ARIZONA_MICSUPP_MAX_SELECTOR 0x1f

#define ARIZONA_MICSUPP_RANGE1_MAX_SELECTOR 0x14
#define ARIZONA_MICSUPP_RANGE2_MAX_SELECTOR 0x27

struct arizona_micsupp {
	struct regulator_dev *regulator;
	struct arizona *arizona;

	struct regulator_consumer_supply supply[5];
	struct regulator_init_data init_data;
};

static int arizona_micsupp_sel_to_voltage(unsigned int selector)
{
	if (selector > ARIZONA_MICSUPP_MAX_SELECTOR)
		return -EINVAL;

	if (selector == ARIZONA_MICSUPP_MAX_SELECTOR)
		return 3300000;
	else
		return (selector * 50000) + 1700000;
}

static int arizona_micsupp_ext_sel_to_voltage(unsigned int selector)
{
	if (selector > ARIZONA_MICSUPP_RANGE2_MAX_SELECTOR)
		return -EINVAL;

	if (selector < ARIZONA_MICSUPP_RANGE1_MAX_SELECTOR) {
		return (selector * 25000) + 900000;
	} else {
		selector -= ARIZONA_MICSUPP_RANGE1_MAX_SELECTOR;
		return (selector * 100000) + 1400000;
	}
}

static int arizona_micsupp_reg_list_voltage(struct regulator_dev *rdev,
					    unsigned int selector)
{
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);

	switch (micsupp->arizona->type) {
	case WM8280:
	case WM5110:
	case WM5102:
	case WM8997:
	case WM8998:
	case WM1814:
		return arizona_micsupp_sel_to_voltage(selector);
	default:
		return arizona_micsupp_ext_sel_to_voltage(selector);
	}
}

static int arizona_micsupp_reg_is_enabled(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);

	ret = regmap_read(micsupp->arizona->regmap,
			  ARIZONA_MIC_CHARGE_PUMP_1, &val);
	if (ret != 0)
		return ret;

	return (val & ARIZONA_CPMIC_ENA) != 0;
}

static int arizona_micsupp_reg_enable(struct regulator_dev *rdev)
{
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);

	return regmap_update_bits(micsupp->arizona->regmap,
				  ARIZONA_MIC_CHARGE_PUMP_1,
				  ARIZONA_CPMIC_ENA,
				  ARIZONA_CPMIC_ENA);
}

static int arizona_micsupp_reg_disable(struct regulator_dev *rdev)
{
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);

	return regmap_update_bits(micsupp->arizona->regmap,
				  ARIZONA_MIC_CHARGE_PUMP_1,
				  ARIZONA_CPMIC_ENA, 0);
}

static int arizona_micsupp_reg_get_voltage_sel(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);

	ret = regmap_read(micsupp->arizona->regmap, ARIZONA_LDO2_CONTROL_1, &val);
	if (ret != 0)
		return ret;

	val &= ARIZONA_LDO2_VSEL_MASK;
	val >>= ARIZONA_LDO2_VSEL_SHIFT;

	return val;
}

static int arizona_micsupp_reg_set_voltage_sel(struct regulator_dev *rdev,
					       unsigned sel)
{
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);
	sel <<= ARIZONA_LDO2_VSEL_SHIFT;

	return regmap_update_bits(micsupp->arizona->regmap,
				  ARIZONA_LDO2_CONTROL_1,
				  ARIZONA_LDO2_VSEL_MASK, sel);
}

static int arizona_micsupp_enable_time(struct regulator_dev *rdev)
{
	struct arizona_micsupp *micsupp = rdev_get_drvdata(rdev);
	switch (micsupp->arizona->type) {
	case WM8280:
	case WM5110:
		return 3000;
	default:
		return 6000;
	}
}

static struct regulator_ops arizona_micsupp_ops = {
	.enable = arizona_micsupp_reg_enable,
	.disable = arizona_micsupp_reg_disable,
	.is_enabled = arizona_micsupp_reg_is_enabled,

	.enable_time = arizona_micsupp_enable_time,

	.list_voltage = arizona_micsupp_reg_list_voltage,

	.get_voltage_sel = arizona_micsupp_reg_get_voltage_sel,
	.set_voltage_sel = arizona_micsupp_reg_set_voltage_sel,
};

static struct regulator_desc arizona_micsupp = {
	.name = "MICVDD",
	.type = REGULATOR_VOLTAGE,
	.n_voltages = ARIZONA_MICSUPP_MAX_SELECTOR + 1,
	.ops = &arizona_micsupp_ops,
	.owner = THIS_MODULE,
};

static struct regulator_desc arizona_micsupp_ext = {
	.name = "MICVDD",
	.type = REGULATOR_VOLTAGE,
	.n_voltages = ARIZONA_MICSUPP_RANGE2_MAX_SELECTOR + 1,
	.ops = &arizona_micsupp_ops,
	.owner = THIS_MODULE,
};

static const struct regulator_init_data arizona_micsupp_default = {
	.constraints = {
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_VOLTAGE,
		.min_uV = 1700000,
		.max_uV = 3300000,
	},

	.num_consumer_supplies = 1,
};

static const struct regulator_init_data arizona_micsupp_ext_default = {
	.constraints = {
		.valid_ops_mask = REGULATOR_CHANGE_STATUS |
				REGULATOR_CHANGE_VOLTAGE,
		.min_uV = 900000,
		.max_uV = 3300000,
	},

	.num_consumer_supplies = 1,
};

#ifdef CONFIG_OF
static int arizona_micsupp_of_get_pdata(struct arizona *arizona,
					struct arizona_micsupp *micsupp,
					struct device_node **of_node)
{
	struct arizona_pdata *pdata = &arizona->pdata;
	struct device_node *np;
	struct regulator_init_data *init_data;

	for_each_child_of_node(arizona->dev->of_node, np)
		if (np->name &&
		    (of_node_cmp(np->name, "micvdd") == 0))
			break;

	if (np) {
		*of_node = np;

		init_data = of_get_regulator_init_data(arizona->dev, np);

		if (init_data) {
			init_data->consumer_supplies = &micsupp->supply;
			init_data->num_consumer_supplies = 1;

			pdata->micvdd = init_data;
		}
	}

	return 0;
}
#else
static int arizona_micsupp_of_get_pdata(struct arizona *arizona,
					struct arizona_micsupp *micsupp,
					struct device_node **of_node)
{
	return 0;
}
#endif

static __devinit int arizona_micsupp_probe(struct platform_device *pdev)
{
	struct arizona *arizona = dev_get_drvdata(pdev->dev.parent);
	struct regulator_desc *desc;
	struct arizona_micsupp *micsupp;
	struct regulator_init_data *init_data;
	struct device_node *of_node = NULL;
	int ret, i;

	micsupp = devm_kzalloc(&pdev->dev, sizeof(*micsupp), GFP_KERNEL);
	if (micsupp == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	}

	micsupp->arizona = arizona;

	/*
	 * Since the chip usually supplies itself we provide some
	 * default init_data for it.  This will be overridden with
	 * platform data if provided.
	 */
	switch (arizona->type) {
	case WM5102:
	case WM8997:
	case WM8998:
	case WM1814:
		desc = &arizona_micsupp;
		micsupp->init_data = arizona_micsupp_default;
		break;
	default:
		desc = &arizona_micsupp_ext;
		micsupp->init_data = arizona_micsupp_ext_default;
		break;
	}

	micsupp->init_data.consumer_supplies = micsupp->supply;
	micsupp->init_data.num_consumer_supplies = ARRAY_SIZE(micsupp->supply);
	for (i = 0; i < ARRAY_SIZE(micsupp->supply); i++) {
		micsupp->supply[i].supply = "MICVDD";
	}
	micsupp->supply[0].dev_name = dev_name(arizona->dev);
	micsupp->supply[1].dev_name = "wm5102-codec";
	micsupp->supply[2].dev_name = "florida-codec";
	micsupp->supply[3].dev_name = "wm8997-codec";
	micsupp->supply[4].dev_name = "vegas-codec";

	if (IS_ENABLED(CONFIG_OF)) {
		if (!dev_get_platdata(arizona->dev)) {
			ret = arizona_micsupp_of_get_pdata(arizona, micsupp, &of_node);
			if (ret < 0)
				return ret;
		}
	}

	if (arizona->pdata.micvdd)
		init_data = arizona->pdata.micvdd;
	else
		init_data = &micsupp->init_data;

	/* Default to regulated mode until the API supports bypass */
	regmap_update_bits(arizona->regmap, ARIZONA_MIC_CHARGE_PUMP_1,
			   ARIZONA_CPMIC_BYPASS, 0);

	micsupp->regulator = regulator_register(desc,
						arizona->dev, init_data,
						micsupp, of_node);

	if (IS_ERR(micsupp->regulator)) {
		ret = PTR_ERR(micsupp->regulator);
		dev_err(arizona->dev, "Failed to register mic supply: %d\n",
			ret);
		return ret;
	}

	of_node_put(of_node);

	platform_set_drvdata(pdev, micsupp);

	return 0;
}

static __devexit int arizona_micsupp_remove(struct platform_device *pdev)
{
	struct arizona_micsupp *micsupp = platform_get_drvdata(pdev);

	regulator_unregister(micsupp->regulator);

	return 0;
}

static struct platform_driver arizona_micsupp_driver = {
	.probe = arizona_micsupp_probe,
	.remove = __devexit_p(arizona_micsupp_remove),
	.driver		= {
		.name	= "arizona-micsupp",
		.owner	= THIS_MODULE,
	},
};

module_platform_driver(arizona_micsupp_driver);

/* Module information */
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_DESCRIPTION("Arizona microphone supply driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:arizona-micsupp");
