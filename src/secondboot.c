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
#define __SET_GLOBAL_VARIABLES

#include "sysheader.h"
#include "memtester.h"

extern void DMC_Delay(int milisecond);
extern void ResetCon(U32 devicenum, CBOOL en);

extern void subcpu_bringup(void);

#if (CONFIG_SUSPEND_RESUME == 1)
extern void enterSelfRefresh(void);
extern void exitSelfRefresh(void);
#endif

#if (CONFIG_BUS_RECONFIG == 1)
extern void setBusConfig(void);
#endif

extern CBOOL iUSBBOOT(struct NX_SecondBootInfo * pTBI);
extern CBOOL iSDXCBOOT(struct NX_SecondBootInfo * pTBI);
extern void initClock(void);
#ifdef MEM_TYPE_DDR3
extern void init_DDR3(U32);
#endif
#ifdef MEM_TYPE_LPDDR23
extern void init_LPDDR3(U32);
#endif

#if defined( INITPMIC_YES )
extern void initPMIC(void);
#endif
extern void buildinfo(void);
extern void printClkInfo(void);

extern void setEMA(void);
extern void s5p4418_resume(void);

extern int CRC_Check(void* buf, unsigned int size, unsigned int ref_crc);

void delay_ms(int ms)
{
	int i, k;

	for (i = 0; i < (ms * 1000); i++)
		for (k = 0; k < 1000; k++);
}

void enableL2Cache(unsigned int enb)
{
	unsigned int reg;

	// L2 Cache
	reg = ReadIO32(&pReg_Tieoff->TIEOFFREG[0]);
	if (enb)
		reg |= (3UL << 12);
	else
		reg &= ~(3UL << 12);

	WriteIO32(&pReg_Tieoff->TIEOFFREG[0], reg);
}

static int __init check_bl1_size(void)
{
	if (pSBI->LOADSIZE <= (SRAM_MAXSIZE - BL1_STACKSIZE)) {
		return 0;
	} else{
		/* Do not exceed the 28K (32K - Stack SIze(3KB). */
		ERROR("BL1 is, must not exceed the maximum size. (28KB <= %d Byte)",
			(pSBI->LOADSIZE));
		return -1;
	}
}

#ifdef CHIPID_NXP4330
extern unsigned int __init sdmmc_self_boot(void);

/*
 * NXP4330 a part to overcome the limitations
 * on the size used in the SRAM Romboot(rev2).
 */
static int __init nxp4330_self_boot(void)
{
	int boot_option  = pSBI->DBI.SDMMCBI.LoadDevice;
	unsigned int fix_bl1_size = (16 * 1024);
	int ret = 0;

	/* Make sure than the size loaded in Romboot, built size is large. */
	if (pSBI->LOADSIZE > fix_bl1_size) {
		/* Check to boot type. */
		switch(boot_option) {
			case BOOT_FROM_SDMMC:
				ret = sdmmc_self_boot();
				break;
		}
	}

	return ret;
}
#endif

void __init BootMain(void)
{
	struct NX_SecondBootInfo TBI;
	struct NX_SecondBootInfo *pTBI = &TBI; // third boot info
	int ret = CFALSE;
#if (CONFIG_SUSPEND_RESUME == 1)
	U32 signature;
#endif
	U32 is_resume = 0;
	U32 debug_ch = 0;

#if defined(AVN) || defined(NAVI) || defined(RAPTOR)
	debug_ch = 3;
#endif

	/* check to binary(bl1) size */
	check_bl1_size();

#ifdef CHIPID_NXP4330
	/* must be self loading*/
	nxp4330_self_boot();
#endif

#if 0 // Low Level Debug Message
	/*  Low Level Debug Message */
	DebugInit(debug_ch);
#endif

#if (CONFIG_SUSPEND_RESUME == 1)
	WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1);
	signature = ReadIO32(&pReg_Alive->ALIVESCRATCHREADREG);
	if ((SUSPEND_SIGNATURE == signature) &&
			ReadIO32(&pReg_Alive->WAKEUPSTATUS)) {	
		is_resume = 1;
	}
#endif  //#if (CONFIG_SUSPEND_RESUME == 1)

#if defined(INITPMIC_YES)
	/* Set the PMIC */
	initPMIC();
#endif
	DMC_Delay(0xFFFFF);

	/* S5PXX18 Clock initialize */
	initClock();

	/* Console initialize */
	DebugInit(debug_ch);

	/* Build information */
	buildinfo();

	/* Set the EMA */
	setEMA();

	/*  */
	enableL2Cache(CTRUE);

#if 0 // Clock Information Display.
	/* Clock Information */
	printClkInfo();
#endif
	device_reset();

#if defined(SECURE_MODE)
	/* Secondary cpu bring up */
	subcpu_bringup();
#endif

#if (CONFIG_SUSPEND_RESUME == 1)
#ifdef MEM_TYPE_DDR3
	init_DDR3(is_resume);
#endif
#ifdef MEM_TYPE_LPDDR23
	init_LPDDR3(is_resume);
#endif

	/* Exit to Self Refresh  */
	if (is_resume) 
		exitSelfRefresh();

	SYSMSG("DDR3/LPDDR3 Init Done!\r\n");

#if (CONFIG_BUS_RECONFIG == 1)
	setBusConfig();
#endif

	if (is_resume) {
		printf(" DDR3 SelfRefresh exit Done!\r\n0x%08X\r\n",
				ReadIO32(&pReg_Alive->WAKEUPSTATUS));
		s5p4418_resume();
	}
	WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 0);
#else // #if (CONFIG_SUSPEND_RESUME == 1)

	/* (DDR3/LPDDR3) SDRAM initialize */
#ifdef MEM_TYPE_DDR3
	init_DDR3(is_resume);
#endif
#ifdef MEM_TYPE_LPDDR23
	init_LPDDR3(is_resume);
#endif
	SYSMSG("DDR3/LPDDR3 Init Done!\r\n");

#if (CONFIG_BUS_RECONFIG == 1)
	setBusConfig();
#endif
#endif // #if (CONFIG_SUSPEND_RESUME == 1)

	if (pSBI->SIGNATURE != HEADER_ID)
		printf("2nd Boot Header is invalid, Please check it out!\r\n");

	/* Check the data loaded to the SDRAM as a CRC */
#if defined(STANDARD_MEMTEST)
	memtester_main((U32)0x40000000UL, (U32)0x50000000UL, 0x10);
#elif defined(SIMPLE_MEMTEST)
	simple_memtest((U32*)0x40000000UL, (U32*)0x60000000UL);
#endif

#if defined(LOAD_FROM_USB)
	printf( "Loading from usb...\r\n" );
	ret = iUSBBOOT(pTBI);            // for USB boot
#endif

	switch (pSBI->DBI.SPIBI.LoadDevice) {
#if defined(SUPPORT_USB_BOOT)
		case BOOT_FROM_USB:
			printf("Loading from usb...\r\n");
			ret = iUSBBOOT(pTBI);	// for USB boot
			break;
#endif

#if defined(SUPPORT_SDMMC_BOOT)
		case BOOT_FROM_SDMMC:
			printf("Loading from sdmmc...\r\n");
			ret = iSDXCBOOT(pTBI);	// for SD boot
			break;
#endif
	}

#ifdef CRC_CHECK_ON
	/* Check the data loaded to the SDRAM CRC. */
	ret = CRC_Check((void*)pTBI->LOADADDR, (unsigned int)pTBI->LOADSIZE
			,(unsigned int)pTBI->DBI.SDMMCBI.CRC32);
#endif
	if (ret) {
		void (*pLaunch)(U32, U32) = (void (*)(U32, U32))pTBI->LAUNCHADDR;
		printf(" Image Loading Done!\r\n");
		printf("Launch to 0x%08X\r\n", (U32)pLaunch);
		while (!DebugIsUartTxDone());
		pLaunch(0, 4330);
	}

	printf("Image Loading Failure Try to USB boot\r\n");
	while (!DebugIsUartTxDone());
}
