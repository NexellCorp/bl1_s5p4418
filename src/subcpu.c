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
extern void ResetCon(unsigned int devicenum, int en);

#if !defined(SECURE_MODE)
extern void (*g_psci_ep)();
#endif

#define CPU_BRINGUP_CHECK   	(1)
#define CPU_ALIVE_FLAG_ADDR	0xC0010238

CBOOL subcpu_on_start(U32 cpuid)
{
	if ((cpuid > 3) || (cpuid == 0))
		return CFALSE;

#if (CPU_BRINGUP_CHECK == 1)
	// High Vector;
	SetIO32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpuid) << 18));
#else
	// Low Vector;
	ClearIO32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpuid) << 18));
#endif
	/* Reset Assert */
	ResetCon(cpuid, CTRUE);

	/* CPUCLKOFF Set to 1 except CPU0 */
	SetIO32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpuid) << (37 - 32)));

	// Reset Negate
	ResetCon(cpuid, CFALSE);

	/*
	 * CPUCLKOFF Set to 0 except CPU0
	 * supply clock to CPUCLK real startup cpu
	 */
	ClearIO32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpuid) << (37 - 32)));

	return CTRUE;
}

void subcpu_boot(unsigned int cpuid)
{
#if defined(SECURE_MODE)
	volatile U32 *aliveflag
		= (U32 *)CPU_ALIVE_FLAG_ADDR;
	putchar('0' + cpuid);
	*aliveflag = 1;

	subcpu_wfi();
#else
	cpuid = cpuid;
	g_psci_ep();
#endif
}

void subcpu_bringup(void)
{
#if (CPU_BRINGUP_CHECK == 1)
	volatile unsigned int *aliveflag
		= (unsigned int *)CPU_ALIVE_FLAG_ADDR;
	int cpu_number, retry = 0;

	for (cpu_number = 1; cpu_number < 4;) {
		register volatile unsigned int delay;
		*aliveflag = 0;
		delay = 0x10000;
		subcpu_on_start(cpu_number);
		while ((*aliveflag == 0) && (--delay));
		if (delay == 0) {
			if (retry > 3) {
				WARN("maybe cpu %d is dead. -_-;\r\n", cpu_number);
				retry = 0;
				cpu_number++;
			} else {
				WARN("cpu %d is not bringup, retry\r\n", cpu_number);
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
