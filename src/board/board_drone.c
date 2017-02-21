/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: DeokJin, Lee <truevirtue@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <sysheader.h>
#include <drone.h>
#include <i2c_gpio.h>
#include <axp228.h>

#if defined(DRONE_PMIC)
/************************************************
 * Drone Board (PMIC: AXP228)  - Reference 2016.04.05
 * ARM		: 1.25V (DC2)
 * CORE		: 1.1V (DC3)
 ************************************************/
void pmic_drone(void)
{
	char data[4];

	I2C_INIT(AXP_I2C_GPIO_GRP, AXP_I2C_SCL, AXP_I2C_SDA,
		AXP_I2C_SCL_ALT, AXP_I2C_SDA_ALT);

	axp228_read(0x80, data, 1);
	data[0] = (data[0] & 0x1F) | DCDC_SYS | DCDC_DDR;
	axp228_write(0x80, data, 1);

	/* ARM voltage change */
#if (ARM_VOLTAGE_CONTROL_SKIP == 0)
	data[0] = axp228_get_dcdc_step(
			AXP228_DEF_DDC2_VOL, AXP228_DEF_DDC234_VOL_STEP,
			AXP228_DEF_DDC234_VOL_MIN, AXP228_DEF_DDC24_VOL_MAX);
	axp228_write(AXP228_REG_DC2VOL, data, 1);
#endif

	/* Core Voltage Change */
	data[0] = axp228_get_dcdc_step(
			AXP228_DEF_DDC3_VOL, AXP228_DEF_DDC234_VOL_STEP,
			AXP228_DEF_DDC234_VOL_MIN, AXP228_DEF_DDC3_VOL_MAX);
	axp228_write(AXP228_REG_DC3VOL, data, 1);
#if 0
	/* Set voltage of DCDC4. */
	data[0] = axp228_get_dcdc_step(AXP228_DEF_DDC4_VOL,
		AXP228_DEF_DDC234_VOL_STEP, AXP228_DEF_DDC234_VOL_MIN, AXP228_DEF_DDC24_VOL_MAX);
	axp228_write(AXP228_REG_DC4VOL, data, 1);

	// Set voltage of DCDC5.
	data[0] = axp228_get_dcdc_step(AXP228_DEF_DDC5_VOL,
		AXP228_DEF_DDC5_VOL_STEP, AXP228_DEF_DDC5_VOL_MIN, AXP228_DEF_DDC5_VOL_MAX);
	axp228_write(AXP228_REG_DC5VOL, data, 1);
#endif
	I2C_DEINIT();

	return;
}
#endif
