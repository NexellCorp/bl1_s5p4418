/*:wq
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
#if defined(PMIC_ON)
#include <i2c_gpio.h>
#include <nxe2000.h>

#define NXE2000_I2C_GPIO_GRP 			4				// GPIOD
#define NXE2000_I2C_SCL 			14				// SCL : GPIOD14
#define NXE2000_I2C_SDA 			15				// SDA : GPIOD15
#define NXE2000_I2C_SCL_ALT 			0				// SCL : ALT0
#define NXE2000_I2C_SDA_ALT 			0				// SDA : ALT0

/************************************************
  * Raptor Board (PMIC: NXE2000)  - Reference 2016.04.05
  * ARM		: 1.25V
  * CORE	: 1.2V
  * DDR		: 1.5V
  * DDR_IO	: 1.5V
  ************************************************/
void pmic_board_init(void)
{
	char data[4];

	I2C_INIT(NXE2000_I2C_GPIO_GRP, NXE2000_I2C_SCL, NXE2000_I2C_SDA,
			NXE2000_I2C_SCL_ALT, NXE2000_I2C_SDA_ALT);

	// ARM Voltage (Default: 1.25V)
	data[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC1_VOL);
	nxe2000_write(NXE2000_REG_DC1VOL, data, 1);

	// Core Voltage (Default: 1.1V)
	data[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC2_VOL);
	nxe2000_write(NXE2000_REG_DC2VOL, data, 1);

	// DDR I/O Voltage (Default: 1.5V)
	data[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC3_VOL);
	nxe2000_write(NXE2000_REG_DC3VOL, data, 1);

	// DDR Voltage (Default: 1.5V)
	data[0] = nxe2000_get_dcdc_step(NXE2000_DEF_DDC4_VOL);
	nxe2000_write(NXE2000_REG_DC4VOL, data, 1);
}
#endif
