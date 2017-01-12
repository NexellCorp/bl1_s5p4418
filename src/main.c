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
	if (pSBI->LOADSIZE <= (SRAM_MAXSIZE - BL1_STACKSIZE)) {
		return 0;
	} else {
		/* Do not exceed the 28K (32K - Stack SIze(3KB). */
		ERROR("BL1 is, must not exceed the maximum size. (28KB <= %d Byte)",
			(pSBI->LOADSIZE));
		return -1;
	}
}

void __init main(void)
{
	struct NX_SecondBootInfo TBI;
	struct NX_SecondBootInfo *pTBI = &TBI; // third boot info
	unsigned int serial_ch = 0;
	unsigned int is_resume = 0;
	int ret = 0;

#if defined(AVN) || defined(NAVI) || defined(RAPTOR) || defined(SMART_VOICE)
	serial_ch = 3;
#endif

	/* setp 01. set the ema for sram and instruction-cache */
	cache_setup_ema();

	/* step xx. check to binary(bl1) size */
	check_bl1_size();

#ifdef CHIPID_NXP4330
	/* step xx. must be self loading*/
	nxp4330_self_boot();
#endif

#if 0	/* (early) low level - log message */
	/* step xx. serial console(uartX) initialize. */
	serial_init(serial_ch);
#endif

	/* step xx. check the suspend, resume */
	is_resume = s5p4418_resume_check();

	/* step 02. set the pmic(power management ic) */
#if defined(INITPMIC_YES)
	initPMIC();
#endif
	DMC_Delay(0xFFFFF);

	/* step 03. clock(pll) intialize */
	initClock();

	/* step 04. serial console(uartX) initialize. */
	serial_init(serial_ch);

	/* step xx. build information. version, build time and date */
	if (build_information() < 0)
		WARN("NSIH Version(or File) Mismatch...!!\r\n");	

	/* step xx. display the clock information */
	printClkInfo();

#if defined(SECURE_MODE)
	/* step 05. set the secondary-core */
	subcpu_bringup();
#endif

	/* step 06. (ddr3/lpddr3) sdram memory initialize */
#ifdef MEM_TYPE_DDR3
	init_DDR3(is_resume);
#endif
#ifdef MEM_TYPE_LPDDR23
	init_LPDDR3(is_resume);
#endif
	NOTICE("(LPDDR3/DDR3) Initialize Done!\r\n");

	/* step xx. display the ema(extra margin adjustments) information.  */
	ema_information();

	/* step 07-1. exit the (sdram) self-refresh  */
	if (is_resume) 
		exitSelfRefresh();

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
	if (pSBI->SIGNATURE != HEADER_ID)
		ERROR("2nd Boot Header is invalid, Please check it out!\r\n");

	/* step xx. enable the l2-cache */
	l2cache_set_enb(CTRUE);

	/* step xx. s5p4418 - device reset (temporary) */
	device_reset();

	/* step xx. check the memory test (optional) */
#if defined(STANDARD_MEMTEST)
	memtester_main((unsigned int)0x40000000UL, (unsigned int)0x50000000UL, 0x10);
#elif defined(SIMPLE_MEMTEST)
	simple_memtest((unsigned int*)0x40000000UL, (unsigned int*)0x60000000UL);
#endif

	/* 
	  * step 10-1. check the (thirdboot) boot mode
	  * step 10-2. loading the next file (thirdboot)
	  */
	switch (pSBI->DBI.SPIBI.LoadDevice) {
#if defined(SUPPORT_USB_BOOT)
		case BOOT_FROM_USB:
			SYSMSG("Loading from usb...\r\n");
			ret = iUSBBOOT(pTBI);	// for USB boot
			break;
#endif

#if defined(SUPPORT_SDMMC_BOOT)
		case BOOT_FROM_SDMMC:
			SYSMSG("Loading from sdmmc...\r\n");
			ret = iSDXCBOOT(pTBI);	// for SD boot
			break;
#endif
	}

#ifdef CRC_CHECK_ON
	/* step xx. check the memory crc check (optional) */
	ret = crc_check((void*)pTBI->LOADADDR, (unsigned int)pTBI->LOADSIZE
			,(unsigned int)pTBI->DBI.SDMMCBI.CRC32);
#endif
	/* step 11. jump the next bootloader (thirdboot) */
	if (ret) {
		void (*pLaunch)(unsigned int, unsigned int)
			= (void (*)(unsigned int, unsigned int))pTBI->LAUNCHADDR;
		SYSMSG("Image Loading Done!\r\n");
		SYSMSG("Launch to 0x%08X\r\n", (unsigned int)pLaunch);
		while (!serial_done());
		pLaunch(0, 4330);
	}

	ERROR("Image Loading Failure Try to USB boot\r\n");
	while (!serial_done());
}

