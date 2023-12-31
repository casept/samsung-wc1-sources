/*
 * Copyright (c) 2010-2012 Samsung Electronics Co., Ltd.
 *
 * I2C3 GPIO configuration.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

struct platform_device; /* don't need the contents */

#include <linux/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-cfg.h>
#include <plat/cpu.h>

void s3c_i2c3_cfg_gpio(struct platform_device *dev)
{
	if (soc_is_exynos5250())
		s3c_gpio_cfgall_range(EXYNOS5_GPA1(2), 2,
				      S3C_GPIO_SFN(3), S3C_GPIO_PULL_UP);

	else if (soc_is_exynos5260())
		s3c_gpio_cfgall_range(EXYNOS5260_GPB4(6), 2,
				      S3C_GPIO_SFN(2), S3C_GPIO_PULL_UP);

	else if (soc_is_exynos5410())
		s3c_gpio_cfgall_range(EXYNOS5410_GPA1(2), 2,
				      S3C_GPIO_SFN(3), S3C_GPIO_PULL_UP);

	else if (soc_is_exynos5420())
		s3c_gpio_cfgall_range(EXYNOS5420_GPA1(2), 2,
				      S3C_GPIO_SFN(3), S3C_GPIO_PULL_UP);

	else if (soc_is_exynos3250())
		s3c_gpio_cfgall_range(EXYNOS3_GPA1(2), 2,
				      S3C_GPIO_SFN(3), S3C_GPIO_PULL_NONE);

	else	/* EXYNOS4210, EXYNOS4212, and EXYNOS4412 */
		s3c_gpio_cfgall_range(EXYNOS4_GPA1(2), 2,
				      S3C_GPIO_SFN(3), S3C_GPIO_PULL_UP);
}
