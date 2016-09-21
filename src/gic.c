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
#include "sysheader.h"
#include "gic.h"

/*******************************************************************************
 * cpu-interface s/w interface accessors for reading entire registers
 ******************************************************************************/

unsigned char* gicc_get_baseaddr(void)
{
	return ((unsigned char*)GICC_BASEADDR);
}

unsigned int gicc_get_iar(void* base)
{
	return (mmio_read_32((base + GIC_CPUIF_IAR)) & 0xFFFF);
}

/*******************************************************************************
 * cpu-interface s/w interface accessors for writing entire registers
 ******************************************************************************/
void gicc_set_ctrl(void* base, int val)
{
	mmio_set_32((base + GIC_CPUIF_CTRL), val);
}

void gicc_set_eoir(void* base, int val)
{
	int eoir = val & 0xFFFF;
	mmio_write_32((base + GIC_CPUIF_EOIR), eoir);
}

/*******************************************************************************
 * Distributor interface accessors for reading entire registers
 ******************************************************************************/
unsigned char* gicd_get_baseaddr(void)
{
	return ((unsigned char*)GICD_BASEADDR);
}

/*******************************************************************************
 * Distributor interface accessors for writing entire registers
 ******************************************************************************/
void gicd_set_enable(void* base, int val)
{
	mmio_set_32((base + GIC_DIST_SENABLE), val);
}

void gicd_set_group(void* base, int val)
{
	mmio_write_32((base + GIC_DIST_GROUP), val);
}

void gicd_set_sgir(void* base, int val)
{
	mmio_write_32((base + GIC_DIST_SGIR), val);
}

/*******************************************************************************
 * Setup the ARM GIC CPU and Distributor interfaces.
******************************************************************************/
#if 0
void gic_disp_init(void)
{
	unsigned int dist_base = gic_disp_get_baseaddr();
	unsigned int cpu_base = gic_cpuif_get_baseaddr();

	int nonsecure = 1, secure = 1;
	int i;

	/* CPU Interface Enable */
	WriteIO32((cpu_base + GIC_CPUIF_CTRL), (3 << 0));

	/*
	 * Whether or not to activate the interrupt
	 * occurs GROUP0/GROUP1 on the GIC.
	 */
	WriteIO32((dist_base + GIC_DISP_CTRL), (nonsecure << 1) | (secure << 0));

	/*
	 * secure/non-secure used to determine
	 * the priority of the interrupt.
	 */
	WriteIO32((cpu_base + GIC_CPUIF_PRIORTY), 0xFFFFFFFF);

	/*
	 * GIC set the path that is connected to the
	 * internal IP interrupts, generated. (0: Secure, 1: Non-Secure)
	 */
	for (i = 0; i <= 0xC; i+=4)
		WriteIO32((dist_base + GIC_DISP_GROUP + i), 0xFFFFFFFF);

}
#endif
