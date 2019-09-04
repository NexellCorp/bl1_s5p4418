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
#define __SET_GLOBAL_VARIABLES

#include <sysheader.h>
#include <main.h>
//#include <memtester.h>
extern void DMC_Delay(int milisecond);


void delay_ms(int ms)
{
	int i, k;

	for (i = 0; i < (ms * 1000); i++)
		for (k = 0; k < 1000; k++);
}

void l2cache_set_enb(unsigned int enb)
{
	unsigned int reg;

	/* L2 Cache */
	reg = mmio_read_32(&pReg_Tieoff->TIEOFFREG[0]);
	if (enb)
		reg |= (3UL << 12);
	else
		reg &= ~(3UL << 12);

	mmio_write_32(&pReg_Tieoff->TIEOFFREG[0], reg);
}

static int __init check_bl1_size(void)
{
	if (psbi->load_size <= (SRAM_MAXSIZE - BL1_STACKSIZE)) {
		return 0;
	} else {
		/* Do not exceed the 28K (32K - Stack SIze(3KB). */
		ERROR("BL1 is, must not exceed the maximum size. (28KB <= %d Byte)",
			(psbi->load_size));
		return -1;
	}
}

void __init main(void)
{
	struct sbi_header tbi;
	struct sbi_header *ptbi = &tbi;
	unsigned int serial_ch = CONFIG_S5P_SERIAL_INDEX;
	unsigned int is_resume = 0;

	/* setp 01. set the ema for sram and instruction-cache */
	cache_setup_ema();

	/* step xx. check to binary(bl1) size */
	check_bl1_size();

#if defined(CHIPID_NXP4330)
	/* SD/eMMC Card Detect Ready */
#ifndef QUICKBOOT
	delay_ms(0x100);
#endif

	/* step xx. must be self-loading*/
	nxp4330_self_boot();
#endif

#if 0	/* (early) low level - log message */
	/* step xx. serial console(uartX) initialize. */
	serial_init(serial_ch);
#endif

	mmio_write_32(&pReg_Alive->ALIVEPWRGATEREG, 1);

#if (CONFIG_SUSPEND_RESUME == 1)
	/* step xx. check the suspend, resume */
	is_resume = s5p4418_resume_check();
#endif

	/* step 02. set the pmic(power management ic) */
#if defined(PMIC_ON)
	pmic_initalize();
#endif

#ifndef QUICKBOOT
	DMC_Delay(0xFFFFF);
#endif

#ifdef ZH_HMDRAGON
	gpio_board_init();
#endif

	/* step 03. clock(pll) intialize */
	clock_initialize();

	/* step 04. serial console(uartX) initialize. */
	serial_init(serial_ch);

	/* step xx. enable the l2-cache */
	l2cache_set_enb(CTRUE);

#ifndef QUICKBOOT
	/* step xx. build information. version, build time and date */
	if (build_information() < 0)
		WARN("NSIH Version(or File) Mismatch...!!\r\n");

	/* step xx. display the clock information */
	clock_information();
#endif

#if defined(SECURE_MODE)
	/* step 05. set the secondary-core */
	subcpu_bringup();
#endif

	/* step xx. display the ema(extra margin adjustments) information.  */
#ifndef QUICKBOOT
	ema_information();
#endif

	/* step 06. memory initialize */
	memory_initialize(is_resume);

#ifdef QUICKBOOT
#ifdef PMIC_CANCEL
	printf("\r\nBL1\r\n");
#else
	printf("\r\nBL1 M:%d/P:%d\r\n", (int)(CONFIG_S5P_SDMMC_CLOCK/1000000),CONFIG_S5P_PLL1_FREQ);
#endif
#endif

	/* step 08-1. set the system bus configuration */
#if (CONFIG_BUS_RECONFIG == 1)
	setBusConfig();
#endif

#if (CONFIG_SUSPEND_RESUME == 1)
	/* step 09. s5p4418 resume sequence */
	if (is_resume) {
		NOTICE("(LPDDR3/DDR3) Self-Refresh Exit Done!\r\n0x%08X\r\n",
				mmio_read_32(&pReg_Alive->WAKEUPSTATUS));
		s5p4418_resume();
	}
#endif
	mmio_write_32(&pReg_Alive->ALIVEPWRGATEREG, 0);

	/* step xx. check the nsih header id */
	if (psbi->signature != HEADER_ID)
		ERROR("2nd Boot Header is invalid, Please check it out!\r\n");

	/* step xx. s5p4418 - moudle resetgen (for kernel4.4) */
	common_resetgen();

	/* step xx. check the memory test (optional) */
#if defined(STANDARD_MEMTEST)
	standard_memtester();
#elif defined(SIMPLE_MEMTEST)
	simple_memtest();
#endif
	/* step 10. loads and launches for the next boot-loader. */
	plat_load(ptbi);
}
