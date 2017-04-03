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

#define TIEOFFREG26	0x1068
#define TIEOFFREG29	0x1070

void __init tieoff_set_secure(void)
{
	void *base = (void *)0xC0010000;

	mmio_set_32((base + TIEOFFREG26), 0xFFFFFFFF);
//	mmio_set_32((base + TIEOFFREG29), 0x1FF);		//VIP, DISP, SCALER
}

