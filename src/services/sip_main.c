/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sangjong, Han <hans@nexell.co.kr>
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
#include <gic.h>

/* External function */
extern void s5p4418_bclk_dfs(unsigned int pll_data);

/* Macro for Secure Write/Read  */
#define S5PXX18_REG_WRITE		0x82000000
#define S5PXX18_REG_READ		0x82000001

#define S5PXX18_REG_BCLK		0x82000009

/*******************************************************************************
 * Registers to access in a secure mode write function.
 ******************************************************************************/
static int secure_write(void* hwreg, int value)
{
	set_secure_mode();
	WriteIO32(hwreg, value);
	set_nonsecure_mode();

	return 0;
}

/*******************************************************************************
 * Registers to access in a secure mode read function.
 ******************************************************************************/
static int secure_read(void* hwreg)
{
	int value = 0;

	set_secure_mode();
	value = ReadIO32(hwreg);
	set_nonsecure_mode();

	return value;
}

/*******************************************************************************
 * For implementing the functions defiend by the user, the SIP interface the
 * main function
 ******************************************************************************/
int sip_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3)
{
	r3 = r3;

	switch (smc_fid) {
	case S5PXX18_REG_WRITE:
		return secure_write((void*)r1, (int)r2);

	case S5PXX18_REG_READ:
		return secure_read((void*)r1);

	case S5PXX18_REG_BCLK:
		s5p4418_bclk_dfs(r1);
		break;

	default:
		WARN("Unimplemented SIP Service Call: 0x%x \n", smc_fid);
		break;
	}

	return 0;
}
