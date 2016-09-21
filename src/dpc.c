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

#define DPC_BASEADDR		0xC0102800

#define DPCCTRL0		0x118

static void* dpc_get_baseaddr(unsigned int module)
{
	return (void*)(DPC_BASEADDR + (module * 0x4000));
}


void dpc_set_enable_all(unsigned int module, unsigned int enb)
{
	volatile void *base = dpc_get_baseaddr(module);
	unsigned int value;

	value = mmio_read_32(base + DPCCTRL0);

	value &= ~(1 << 15);
	value |= (U32)enb << 15;

	mmio_write_32((base + DPCCTRL0), value);
}

int dpc_get_pending_all(unsigned int module)
{
	volatile void *base = dpc_get_baseaddr(module);
	return (mmio_read_32(base + DPCCTRL0) >> 10);
}

void dpc_clear_pending_all(unsigned int module)
{
	volatile void* base = (void*)dpc_get_baseaddr(module);
	unsigned int value;

	value = mmio_read_32(base + DPCCTRL0);
	value |= (1 << 10);

	mmio_write_32((base + DPCCTRL0), value);
}

int  dpc_enabled(unsigned int module)
{
	volatile void* base = (void*)dpc_get_baseaddr(module);
	unsigned int value;

	value = mmio_read_32(base + DPCCTRL0);

	return value & (1 << 15);
}
