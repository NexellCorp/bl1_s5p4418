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

extern void subcpu_wfi(void);
#if !defined(SECURE_MODE)
extern void (*g_psci_ep)();
#endif

#define CPU_BRINGUP_CHECK   	(1)
#define CPU_ALIVE_FLAG_ADDR	0xC0010238

int subcpu_on_start(unsigned int cpu_id)
{
	if ((cpu_id > 3) || (cpu_id == 0))
		return CFALSE;

#if (CPU_BRINGUP_CHECK == 1)
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpu_id) << 18));	// High Vector
#else
	mmio_clear_32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpu_id) << 18));	// Low Vector
#endif

	reset_con(cpu_id, CTRUE);						// Reset Assert

	/* CPUCLKOFF Set to 1 except CPU0 */
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));

	reset_con(cpu_id, CFALSE);						// Reset DeAssert

	/*
	 * CPUCLKOFF Set to 0 except CPU0
	 * supply clock to CPUCLK real startup cpu
	 */
	mmio_clear_32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));

	return CTRUE;
}

void subcpu_boot(unsigned int cpu_id)
{
#if defined(SECURE_MODE)
	volatile unsigned int *flag
		= (unsigned int *)CPU_ALIVE_FLAG_ADDR;
	putchar('0' + cpu_id);
	*flag = 1;

	subcpu_wfi();
#else
	cpu_id = cpu_id;
	g_psci_ep();
#endif
}

void subcpu_bringup(void)
{
#if (CPU_BRINGUP_CHECK == 1)
	volatile unsigned int *flag
		= (unsigned int *)CPU_ALIVE_FLAG_ADDR;
	int cpu_number, retry = 0;

	for (cpu_number = 1; cpu_number < 4;) {
		register volatile unsigned int delay;
		*flag = 0;
		delay = 0x10000;
		subcpu_on_start(cpu_number);
		while ((*flag == 0) && (--delay));
		if (delay == 0) {
			if (retry > 3) {
				WARN("Maybe Core%d is Dead. \r\n", cpu_number);
				retry = 0;
				cpu_number++;
			} else {
				WARN("Core%d is not Bringup, Retry%d! \r\n",
					cpu_number, retry);
				retry++;
			}
		} else {
			retry = 0;
			cpu_number++;
		}
	}
#else
	subcpu_on_start(1);
	subcpu_on_start(2);
	subcpu_on_start(3);
#endif
}
