/*
 *      Copyright (C) 2012 Nexell Co., All Rights Reserved
 *      Nexell Co. Proprietary & Confidential
 *
 *      NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *      AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *      BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
 *	FITNESS
 *      FOR A PARTICULAR PURPOSE.
 *
 *      Module          : Memory Controller
 *      File            : init_DDR3.c
 *      Description     :
 *      Author          : Hans
 *      History         : 
 */
#include "sysheader.h"

#include <nx_drex.h>
#include <nx_ddrphy.h>
#include "nx_reg_base.h"
#include "ddr3_sdram.h"

#define DDR_NEW_LEVELING_TRAINING       (1)

#define DDR_RW_CAL      0

#if defined(CHIPID_NXP4330)
#define DDR_WRITE_LEVELING_EN           (0)
#define DDR_CA_CALIB_EN                 (0)     // for LPDDR3
#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_LEVELING_CALIB_EN     (0)     // for Fly-by
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_GATE_LVL_COMPENSATION_EN    (0)     // Do not use. for Test.
#define DDR_READ_DQ_COMPENSATION_EN     (1)
#define DDR_WRITE_DQ_COMPENSATION_EN    (1)


#define DDR_RESET_GATE_LVL              (1)
#define DDR_RESET_READ_DQ               (1)
#define DDR_RESET_WRITE_DQ              (1)

#define DDR_RESET_QOS1                  (1)     // Release version is '1'
#define DDR_READ_DQ_MARGIN_VIEW         (0)
#define DDR_WRITE_DQ_MARGIN_VIEW        (0)

#endif  // #if defined(CHIPID_NXP4330)

#if defined(CHIPID_S5P4418)
#define DDR_WRITE_LEVELING_EN           (0)
#define DDR_CA_CALIB_EN                 (0)     // for LPDDR3
#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_LEVELING_CALIB_EN     (0)     // for Fly-by
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_GATE_LVL_COMPENSATION_EN    (0)     // Do not use. for Test.
#define DDR_READ_DQ_COMPENSATION_EN     (0)
#define DDR_WRITE_DQ_COMPENSATION_EN    (0)


#define DDR_RESET_GATE_LVL              (1)
#define DDR_RESET_READ_DQ               (1)
#define DDR_RESET_WRITE_DQ              (1)

#define DDR_RESET_QOS1                  (1)     // Release version is '1'

#define DDR_READ_DQ_MARGIN_VIEW         (0)
#define DDR_WRITE_DQ_MARGIN_VIEW        (0)
#endif  // #if defined(CHIPID_S5P4418)

#define MEM_CALIBRAION_INFO 		(1)

#define CFG_ODT_ENB                     (1)

#if (CFG_NSIH_EN == 0)
#include "DDR3_K4B8G1646B_MCK0.h"
#endif

#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");

U32 g_DDRLock;
U32 g_GateCycle;
U32 g_GateCode;
U32 g_RDvwmc;
U32 g_WRvwmc;

void DMC_Delay(int milisecond)
{
	register volatile int count;

	for (count = 0; count < milisecond; count++) {
		nop();
	}
}

inline void SendDirectCommand(SDRAM_CMD cmd, U8 chipnum, SDRAM_MODE_REG mrx,
		U16 value)
{
	WriteIO32((U32 *)&pReg_Drex->DIRECTCMD,
			cmd << 24 | chipnum << 20 | mrx << 16 | value);
}

#if (CONFIG_SUSPEND_RESUME == 1)
void enterSelfRefresh(void)
{
	union SDRAM_MR MR;
	U32 nTemp;
	U32 nChips = 0;

#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	nChips = 0x3;
#else
	nChips = 0x1;
#endif
#else
	if (pSBI->DII.ChipNum > 1)
		nChips = 0x3;
	else
		nChips = 0x1;
#endif

	while (ReadIO32(&pReg_Drex->CHIPSTATUS) & 0xF) {
		nop();
	}

	/* Send PALL command */
	SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	DMC_Delay(100);

	// odt off
	MR.Reg          = 0;
	MR.MR2.RTT_WR   = 0;        // 0: disable, 1: RZQ/4 (60ohm), 2: RZQ/2 (120ohm)
	//    MR.MR2.RTT_WR   = 2;        // 0: disable, 1: RZQ/4 (60ohm), 2: RZQ/2 (120ohm)
	MR.MR2.SRT      = 0;        // self refresh normal range, if (ASR == 1) SRT = 0;
	MR.MR2.ASR      = 1;        // auto self refresh enable
#if (CFG_NSIH_EN == 0)
	MR.MR2.CWL      = (nCWL - 5);
#else
	MR.MR2.CWL      = (pSBI->DII.CWL - 5);
#endif

	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif

	MR.Reg          = 0;
	MR.MR1.DLL      = 1;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
	MR.MR1.AL       = MR1_nAL;
#else
	MR.MR1.AL       = pSBI->DII.MR1_AL;
#endif
	MR.MR1.ODS1     = (pSBI->DDR3_DSInfo.MR1_ODS>>1) & 1;
	MR.MR1.ODS0     = (pSBI->DDR3_DSInfo.MR1_ODS>>0) & 1;
	MR.MR1.RTT_Nom2 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>2) & 1;
	MR.MR1.RTT_Nom1 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>1) & 1;
	MR.MR1.RTT_Nom0 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>0) & 1;
	MR.MR1.QOff     = 0;
	MR.MR1.WL       = 0;
#if 0
#if (CFG_NSIH_EN == 0)
	MR.MR1.TDQS     = (_DDR_BUS_WIDTH>>3) & 1;
#else
	MR.MR1.TDQS     = (pSBI->DII.BusWidth>>3) & 1;
#endif
#endif

	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif

	/* Enter self-refresh command */
	SendDirectCommand(SDRAM_CMD_REFS, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_REFS, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_REFS, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if 1
	do {
		nTemp = ( ReadIO32(&pReg_Drex->CHIPSTATUS) & nChips );
	} while( nTemp );

	do {
		nTemp = ( (ReadIO32(&pReg_Drex->CHIPSTATUS) >> 8) & nChips );
	} while( nTemp != nChips );
#else

	// for self-refresh check routine.
	while( 1 ) {
		nTemp = ReadIO32(&pReg_Drex->CHIPSTATUS);
		if (nTemp)
			MEMMSG("ChipStatus = 0x%04x\r\n", nTemp);
	}
#endif


	// Step 52 Auto refresh counter disable
	ClearIO32( &pReg_Drex->CONCONTROL,  (0x1 << 5) );          // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1

	// Step 10  ACK, ACKB off
	SetIO32( &pReg_Drex->MEMCONTROL,    (0x1 << 0) );          // clk_stop_en[0] : Dynamic Clock Control   :: 1'b0  - Always running

	DMC_Delay(1000 * 3);
}

void exitSelfRefresh(void)
{
	union SDRAM_MR MR;

	// Step 10    ACK, ACKB on
	ClearIO32( &pReg_Drex->MEMCONTROL,  (0x1 << 0) );          // clk_stop_en[0] : Dynamic Clock Control   :: 1'b0  - Always running
	DMC_Delay(10);

	// Step 52 Auto refresh counter enable
	SetIO32( &pReg_Drex->CONCONTROL,    (0x1 << 5) );          // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1
	DMC_Delay(10);

	/* Send PALL command */
	SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	MR.Reg          = 0;
	MR.MR1.DLL      = 0;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
	MR.MR1.AL       = MR1_nAL;
#else
	MR.MR1.AL       = pSBI->DII.MR1_AL;
#endif
	MR.MR1.ODS1 	= (pSBI->DDR3_DSInfo.MR1_ODS>>1) & 1;
	MR.MR1.ODS0 	= (pSBI->DDR3_DSInfo.MR1_ODS>>0) & 1;
	MR.MR1.RTT_Nom2 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>2) & 1;
	MR.MR1.RTT_Nom1 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>1) & 1;
	MR.MR1.RTT_Nom0 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>0) & 1;
	MR.MR1.QOff     = 0;
	MR.MR1.WL       = 0;
#if 0
#if (CFG_NSIH_EN == 0)
	MR.MR1.TDQS     = (_DDR_BUS_WIDTH>>3) & 1;
#else
	MR.MR1.TDQS     = (pSBI->DII.BusWidth>>3) & 1;
#endif
#endif

	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif

	// odt on
	MR.Reg          = 0;
	MR.MR2.RTT_WR   = pSBI->DDR3_DSInfo.MR2_RTT_WR;
	MR.MR2.SRT      = 0;        // self refresh normal range
	MR.MR2.ASR      = 0;        // auto self refresh disable
#if (CFG_NSIH_EN == 0)
	MR.MR2.CWL      = (nCWL - 5);
#else
	MR.MR2.CWL      = (pSBI->DII.CWL - 5);
#endif

	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif

	/* Exit self-refresh command */
	SendDirectCommand(SDRAM_CMD_REFSX, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_REFSX, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_REFSX, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if 0
	while( ReadIO32(&pReg_Drex->CHIPSTATUS) & (0xF << 8) ) {
		nop();
	}
#endif

	DMC_Delay(1000 * 2);
}
#endif // #if (CONFIG_SUSPEND_RESUME == 1)


#if (DDR_RW_CAL == 1)
extern    void BurstZero(U32 *WriteAddr, U32 WData);
extern    void BurstWrite(U32 *WriteAddr, U32 WData);
extern    void BurstRead(U32 *ReadAddr, U32 *SaveAddr);

void DDR3_RW_Delay_Calibration(void)
{
	unsigned int rnw, lane, adjusted_dqs_delay=0, bit, pmin, nmin;
	unsigned int *tptr = (unsigned int *)0x40100000;
	int toggle;

	for(rnw = 0; rnw<2; rnw++) {
		MEMMSG("\r\nserching %s delay value......\r\n", rnw?"read":"write");
		bit = 0;
		for(bit = 0; bit<32; bit++) {
			unsigned int readdata[8];
			unsigned int dqs_wdelay, repeatcount;
			unsigned char pwdelay, nwdelay;
			lane = bit>>3;

			if((bit & 7) == 0) {
				pmin = 0x7f;
				nmin = 0x7f;
			}
			MEMMSG("bit:%02d\t", bit);
			pwdelay = 0x80;
			if(rnw ==0)
				WriteIO32(&pReg_DDRPHY->OFFSETW_CON[0], 0x80<<(8*lane));
			else
				WriteIO32(&pReg_DDRPHY->OFFSETR_CON[0], 0x80<<(8*lane));
			SetIO32  ( &pReg_Drex->PHYCONTROL, (0x1 << 3));           // Force DLL Resyncronization
			ClearIO32( &pReg_Drex->PHYCONTROL, (0x1 << 3));           // Force DLL Resyncronization
			DMC_Delay(10000);
			for(dqs_wdelay = 0; dqs_wdelay<=0x7f && pwdelay==0x80; dqs_wdelay++) {
				repeatcount=0;
				if(rnw ==0)
					WriteIO32(&pReg_DDRPHY->OFFSETW_CON[0], dqs_wdelay<<(8*lane));
				else
					WriteIO32(&pReg_DDRPHY->OFFSETR_CON[0], dqs_wdelay<<(8*lane));
				SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1 << 3));       // Force DLL Resyncronization
				ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1 << 3));       // Force DLL Resyncronization
				DMC_Delay(10000);
				while(repeatcount<100) {
					for(toggle=1; toggle>=0; toggle--) {
						if(toggle)
							BurstWrite(tptr, 1<<bit);
						else
							BurstWrite(tptr, ~(1<<bit));
						BurstRead(tptr, readdata);
						if (((readdata[0]>>bit)&0x01) == !toggle &&
								((readdata[1]>>bit)&0x01) == !toggle &&
								((readdata[2]>>bit)&0x01) == !toggle &&
								((readdata[3]>>bit)&0x01) == !toggle &&
								((readdata[4]>>bit)&0x01) ==  toggle &&
								((readdata[5]>>bit)&0x01) == !toggle &&
								((readdata[6]>>bit)&0x01) == !toggle &&
								((readdata[7]>>bit)&0x01) == !toggle)
						{
							repeatcount++;
						}else {
							pwdelay = dqs_wdelay;
							if(pmin > pwdelay)
								pmin = pwdelay;
							MEMMSG("p%d:%02d ", toggle, pwdelay);
							repeatcount = 100;
							toggle = -1;
							break;
						}
					}
				}
			}    // dqs_wdelay
			if(rnw==0)
				WriteIO32(&pReg_DDRPHY->OFFSETW_CON[0], 0<<(8*lane));
			else
				WriteIO32(&pReg_DDRPHY->OFFSETR_CON[0], 0<<(8*lane));
			SetIO32  ( &pReg_Drex->PHYCONTROL, (0x1 << 3));           // Force DLL Resyncronization
			ClearIO32( &pReg_Drex->PHYCONTROL, (0x1 << 3));           // Force DLL Resyncronization
			DMC_Delay(10000);
			nwdelay = 0;
			for(dqs_wdelay = 0x80; dqs_wdelay<=0xFF && nwdelay==0; dqs_wdelay++) {
				repeatcount=0;
				if(rnw == 0)
					WriteIO32(&pReg_DDRPHY->OFFSETW_CON[0], dqs_wdelay<<(8*lane));
				else
					WriteIO32(&pReg_DDRPHY->OFFSETR_CON[0], dqs_wdelay<<(8*lane));
				SetIO32  ( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
				ClearIO32( &pReg_Drex->PHYCONTROL,      (0x1    <<   3));       // Force DLL Resyncronization
				DMC_Delay(10000);
				while(repeatcount<100) {
					for(toggle=1; toggle>=0; toggle--) {
						if(toggle)
							BurstWrite(tptr, 1<<bit);
						else
							BurstWrite(tptr, ~(1<<bit));
						BurstRead(tptr, readdata);
						if( ((readdata[0]>>bit)& 0x01) == !toggle &&
								((readdata[1]>>bit)& 0x01) == !toggle &&
								((readdata[2]>>bit)& 0x01) == !toggle &&
								((readdata[3]>>bit)& 0x01) == !toggle &&
								((readdata[4]>>bit)& 0x01) ==  toggle &&
								((readdata[5]>>bit)& 0x01) == !toggle &&
								((readdata[6]>>bit)& 0x01) == !toggle &&
								((readdata[7]>>bit)& 0x01) == !toggle)
						{
							repeatcount++;
						} else {
							nwdelay = dqs_wdelay & 0x7F;
							if(nmin > nwdelay)
								nmin = nwdelay;
							MEMMSG("n%d:%02d  ", toggle, nwdelay);
							repeatcount = 100;
							toggle = -1;
							break;
						}
					}
				}
			} // dqs_wdelay

			if(pwdelay > nwdelay) { // biased to positive side
				MEMMSG("margin: %2d  adj: %2d\r\n", (pwdelay - nwdelay), (pwdelay - nwdelay)>>1);
			}
			else {     // biased to negative side
				MEMMSG("margin: %2d  adj:-%2d\r\n", (nwdelay - pwdelay), (nwdelay - pwdelay)>>1);
			}
			if((bit & 7)==7) {
				MEMMSG("lane average positive min:%d negative min:%d ", pmin, nmin);
				if(pmin > nmin) { // biased to positive side
					adjusted_dqs_delay |= ((pmin - nmin)>>1) << (8*lane);
					MEMMSG("margin: %2d  adj: %2d\r\n", (pmin - nmin), (pmin - nmin)>>1);
				}
				else { // biased to negative side
					adjusted_dqs_delay |= (((nmin - pmin)>>1) | 0x80) << (8*lane);
					MEMMSG("margin: %2d  adj:-%2d\r\n", (nmin - pmin), (nmin - pmin)>>1);
				}
			}
			if(((bit+1) & 0x7) == 0)
				MEMMSG("\n");
		} // lane

		if(rnw == 0)
			WriteIO32(&pReg_DDRPHY->OFFSETW_CON[0], adjusted_dqs_delay);
		else
			WriteIO32(&pReg_DDRPHY->OFFSETR_CON[0], adjusted_dqs_delay);
		SetIO32  ( &pReg_Drex->PHYCONTROL,   (0x1   <<   3));           // Force DLL Resyncronization
		ClearIO32( &pReg_Drex->PHYCONTROL,   (0x1   <<   3));           // Force DLL Resyncronization
		MEMMSG("\r\n");
		MEMMSG("read  delay value is 0x%08X\r\n", ReadIO32(&pReg_DDRPHY->OFFSETR_CON[0]));
		MEMMSG("write delay value is 0x%08X\r\n", ReadIO32(&pReg_DDRPHY->OFFSETW_CON[0]));
	}
}
#endif

#if (DDR_NEW_LEVELING_TRAINING == 1)

unsigned int GetVWMC_Offset(U32 Code, U32 Lock_Div4)
{
	U8 VWMC[4];
	int OffSet[4];
	U32 i, ret;

	for (i = 0; i < 4; i++)
		VWMC[i] = ((Code >> (8 * i)) & 0xFF);

	for (i = 0; i < 4; i++) {
		OffSet[i] = (int)(VWMC[i] - Lock_Div4);
		if (OffSet[i] < 0) {
			OffSet[i] *= -1;
			OffSet[i] |= 0x80;
		}
	}

	ret = ( ((U8)OffSet[3] << 24) | ((U8)OffSet[2] << 16) |
			((U8)OffSet[1] << 8) | (U8)OffSet[0]);

	return ret;
}

unsigned int GetVWMC_Compensation(unsigned int Code)
{
	unsigned int OffSet[4];
	unsigned int Max, Min, Value;
	int i, inx;
	int ret;

	for(i = 0; i < 4; i++)
		OffSet[i] = (Code >> (i * 8)) & 0xFF;

	//	temp = ( ((U8)OffSet[3] << 24) | ((U8)OffSet[2] << 16) |
	//	((U8)OffSet[1] << 8) | (U8)OffSet[0] );
	//	printf("RD DQ : Org value = 0x%08X\r\n", temp);

	for (inx = 0; inx < 4; inx++) {
		for (i = 1; i < 4; i++) {
			if (OffSet[i - 1] > OffSet[i]) {
				Max = OffSet[i - 1];
				OffSet[i - 1] = OffSet[i];
				OffSet[i] = Max;
			}
		}
	}

#if 0
	for (inx = 0; inx < 4; inx++)
		printf( "Sorted Value[%d] = %d\r\n", inx, offsetr[inx] );
#endif

	if (OffSet[1] > OffSet[2]) {
		Max = OffSet[1];
		Min = OffSet[2];
	} else {
		Max = OffSet[2];
		Min = OffSet[1];
	}

	if ((Max - Min) > 5) {
		Value = Min;
	} else {
		Value = Max;
	}

	for (inx = 0; inx < 4; inx++)
		OffSet[inx] = Value;

	ret = (((U8)OffSet[3] << 24) | ((U8)OffSet[2] << 16) |
			((U8)OffSet[1] << 8) | (U8)OffSet[0]);

	return ret;

}


#if (DDR_WRITE_LEVELING_EN == 1)
void DDR_Write_Leveling(void)
{
#if 0
	MEMMSG("\r\n########## Write Leveling ##########\r\n");

#else
#if defined(MEM_TYPE_DDR3)
	//    union SDRAM_MR MR1;
#endif
	int sdll_shift, dq_shift, find_dq;
	U32 dq_res, dq_sdll, dq_num, i;
	U32 dq_l, dq_r, dq_c;
	U32 temp;

	MEMMSG("\r\n########## Write Leveling - Start ##########\r\n");

	SetIO32( &pReg_DDRPHY->PHY_CON[26+1],       (0x3 <<  7) );          // cmd_default, ODT[8:7]=0x3
	SetIO32( &pReg_DDRPHY->PHY_CON[0],          (0x1 << 16) );          // wrlvl_mode[16]=1

#if 0 //defined(MEM_TYPE_DDR3)
	/* Set MPR mode enable */
	MR1.Reg         = 0;
	MR1.MR1.DLL     = 0;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
	MR1.MR1.AL      = MR1_nAL;
#else
	MR1.MR1.AL      = pSBI->DII.MR1_AL;
#endif
	MR1.MR1.ODS1    = 0;    // 00: RZQ/6, 01 : RZQ/7
	MR1.MR1.ODS0    = 1;
	MR1.MR1.QOff    = 0;
	MR1.MR1.RTT_Nom2    = 0;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8
	MR1.MR1.RTT_Nom1    = 1;
	MR1.MR1.RTT_Nom0    = 0;
	MR1.MR1.WL      = 1;
#if 0
#if (CFG_NSIH_EN == 0)
	MR1.MR1.TDQS    = (_DDR_BUS_WIDTH>>3) & 1;
#else
	MR1.MR1.TDQS    = (pSBI->DII.BusWidth>>3) & 1;
#endif
#endif

	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR1.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif
#endif

	// Send NOP command.
	SendDirectCommand(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* PHY_CON30[30:24] = ctrl_wrlvl_code3, PHY_CON30[23:17] = ctrl_wrlvl_code2, PHY_CON30[14:8] = ctrl_wrlvl_code1, PHY_CON30[6:0] = ctrl_wrlvl_code0 */
	temp = ( (0x8 << 24) | (0x8 << 17) | (0x8 << 8) | (0x8 << 0) );
	WriteIO32( &pReg_DDRPHY->PHY_CON[30+1],     temp );

	for (dq_num = 0; dq_num < 4; dq_num++) {
		dq_l = dq_r = dq_c = 0;
		find_dq     = 0;
		dq_shift    = (dq_num * 8);
		sdll_shift  = dq_shift;

		dq_sdll = ReadIO32( &pReg_DDRPHY->PHY_CON[30+1] ) & ~(0xFF << dq_shift);

		if (dq_num == 2) sdll_shift++;

		for (i = 8; i < 0x39; i++) {
			temp = dq_sdll | (i << sdll_shift);
			WriteIO32( &pReg_DDRPHY->PHY_CON[30+1],     temp );

			// SDLL update.
			SetIO32  ( &pReg_DDRPHY->PHY_CON[30+1],     (0x1 << 16) );          // wrlvl_enable[16]=1, ctrl_wrlvl_resync
			ClearIO32( &pReg_DDRPHY->PHY_CON[30+1],     (0x1 << 16) );          // wrlvl_enable[16]=0, ctrl_wrlvl_resync
			//DMC_Delay(0x10);

			dq_res = ReadIO32( &pReg_Drex->CTRL_IO_RDATA ) & (1 << dq_shift);

			if (find_dq < 0x3) {
				if ( dq_res ) {
					find_dq++;
					if(find_dq == 0x1) {
						dq_l = i;
					}
				} else {
					find_dq = 0x0;                                 //- 첫 번째 PASS로부터 연속 3회 PASS 하지 못하면 연속 3회 PASS가 발생할 때까지 Searching 다시 시작하도록 "find_vmw" = "0"으로 초기화.
				}
			}
			else if( !dq_res ) {
				find_dq = 0x4;
				dq_r = i - 1;
				break;
			}
		}

		dq_r = i - 1;
		dq_c = ((dq_r - dq_l) >> 1) + 0x8;

		dq_sdll |= (dq_c << sdll_shift);
		WriteIO32( &pReg_DDRPHY->PHY_CON[30+1], dq_sdll );
	}

	// SDLL update.
	SetIO32  ( &pReg_DDRPHY->PHY_CON[30+1],     (0x1 << 16) );          // wrlvl_enable[16]=1, ctrl_wrlvl_resync
	ClearIO32( &pReg_DDRPHY->PHY_CON[30+1],     (0x1 << 16) );          // wrlvl_enable[16]=0, ctrl_wrlvl_resync
	DMC_Delay(0x100);

	ClearIO32( &pReg_DDRPHY->PHY_CON[0],        (0x1 << 16) );          // wrlvl_mode[16]=0
	//	ClearIO32( &pReg_DDRPHY->PHY_CON[26+1],     (0x3 << 7) );          // cmd_default, ODT[8:7]=0x0
	MEMMSG("\r\n########## Write Leveling - End ##########\r\n");
#endif
}
#endif

#if (DDR_GATE_LEVELING_EN == 1)

#if (MEM_CALIBRAION_INFO == 1)
void Gate_Leveling_Information(void)
{
	unsigned int GT_FailStatus, DQ_Calibration;
	unsigned int GateCycle[4], GateCode[4], GT_VWMC[4];
	unsigned int MaxSlice = 4, Slice;
	unsigned int LockValue = g_DDRLock;	//

	/* DQ Calibration Fail Status */
	WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_FAIL_STATUS);
	GT_FailStatus = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);

	if (GT_FailStatus == 0) {
		/* Gate Center Cycle */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], GATE_CENTER_CYCLE);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			GateCycle[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Gate Code */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], GATE_CENTER_CODE);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			GateCode[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Gate Vaile Window Margin Center */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], GATE_VWMC);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			GT_VWMC[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;
	}

	printf("\r\n####### Gate Leveling - Information #######\r\n");
	printf("Gate Leveling %s!! \r\n",
			(GT_FailStatus == 0) ? "Success" : "Failed" );
	if (GT_FailStatus == 0) {
		printf("Gate Cycle : %d/%d/%d/%d \r\n", GateCycle[0], GateCycle[1],
				GateCycle[2], GateCycle[3]);
		printf("Gate Code  : %d/%d/%d/%d \r\n", GateCode[0], GateCode[1],
				GateCode[2], GateCode[3]);
		printf("Gate Delay %d, %d, %d, %d\r\n",
				((GateCycle[0]>>0) & 0x7)*LockValue + GT_VWMC[0],
				((GateCycle[1]>>3) & 0x7)*LockValue + GT_VWMC[1],
				((GateCycle[2]>>6) & 0x7)*LockValue + GT_VWMC[2],
				((GateCycle[3]>>9) & 0x7)*LockValue + GT_VWMC[3]);
		printf("###########################################\r\n");
	}
}
#endif

CBOOL DDR_Gate_Leveling(U32 isResume)
{
#if defined(MEM_TYPE_DDR3)
	union SDRAM_MR MR;
#endif
	volatile U32 cal_count = 0;
#if (DDR_RESET_GATE_LVL == 1)
	U32	tGateCycle;
	U8      GateCycle[4];
	U32     i;
#endif
	CBOOL   ret = CTRUE;

	MEMMSG("\r\n########## Gate Leveling - Start ##########\r\n");

	SetIO32  ( &pReg_DDRPHY->PHY_CON[14],   (0xF <<  0) );               // ctrl_pulld_dqs[3:0] = 0
	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1 << 13) );               // byte_rdlvl_en[13]=1

	if (isResume == 0) {
		SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
		SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
		if(pSBI->DII.ChipNum > 1)
			SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if defined(MEM_TYPE_DDR3)
		/* Set MPR mode enable */
		MR.Reg          = 0;
		MR.MR3.MPR      = 1;
		MR.MR3.MPR_RF   = 0;

		SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
		if(pSBI->DII.ChipNum > 1)
			SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#endif
	}


	SetIO32( &pReg_DDRPHY->PHY_CON[2],      (0x1    <<  24) );              // rdlvl_gate_en[24] = 1
	SetIO32( &pReg_DDRPHY->PHY_CON[0],      (0x5    <<   6) );              // ctrl_shgate[8] = 1, ctrl_atgate[6] = 1
#if defined(MEM_TYPE_DDR3)
	ClearIO32( &pReg_DDRPHY->PHY_CON[1],    (0xF    <<  20) );              // ctrl_gateduradj[23:20] = DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
#endif
#if defined(MEM_TYPE_LPDDR23)
	U32     temp;
	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1]) & ~(0xF <<  20);            // ctrl_gateduradj[23:20] = DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
	temp |= (0xB    << 20);
	WriteIO32( &pReg_DDRPHY->PHY_CON[1], temp );
#endif

	/* Update reset DLL */
	WriteIO32(&pReg_Drex->RDLVL_CONFIG, (0x1 << 0));							// ctrl_rdlvl_gate_en[0] = 1

	if (isResume) {
		while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 );      // rdlvl_complete[14] = 1
		WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0 );                            // ctrl_rdlvl_gate_en[0] = 0
	} else {
		while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 ) { 	// rdlvl_complete[14] : Wating until READ leveling is complete
			if (cal_count > 100) { // Failure Case
				WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                 	// ctrl_rdlvl_data_en[0]=0 : Stopping it after completion of READ leveling.

				ret = CFALSE;
				goto gate_err_ret;
			} else {
				DMC_Delay(0x100);
			}

			cal_count++;
		}

		nop();
		WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );		// ctrl_rdlvl_data_en[0]=0 : Stopping it after completion of READ leveling.

		//- Checking the calibration failure status
		//- After setting PHY_CON5(0x14) to "0xC", check whether
		//  PHY_CON21 is zero or nor. If PHY_CON21(0x58) is zero, calibration is normally complete.

		WriteIO32( &pReg_DDRPHY->PHY_CON[5],    VWM_FAIL_STATUS );          // readmodecon[7:0] = 0xC
		cal_count = 0;
		do {
			if (cal_count > 100) {
				MEMMSG("\r\n\nRD VWM_FAIL_STATUS Checking : fail...!!!\r\n");
				ret = CFALSE;                                               // Failure Case
				goto gate_err_ret;
			} else if (cal_count) {
				DMC_Delay(0x100);
			}

			cal_count++;
		} while (ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]) != 0x0);

		//----------------------------------------------------------
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], GATE_CENTER_CYCLE);
		g_GateCycle = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);

		WriteIO32(&pReg_DDRPHY->PHY_CON[5], GATE_CENTER_CODE);
		g_GateCode = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);
		//----------------------------------------------------------
	}

#if (DDR_RESET_GATE_LVL == 1)
	for (i = 0; i < 4; i++)
		GateCycle[i] = ((g_GateCycle >> (8 * i)) & 0xFF);

	unsigned int Offset, C_Offset;
	Offset = GetVWMC_Offset(g_GateCode, (g_DDRLock >> 2));
#if (DDR_GATE_LVL_COMPENSATION_EN == 1)
	C_Offset = GetVWMC_Compensation(Offset);
#else
	C_Offset = Offset;
#endif // #if (DDR_GATE_LVL_COMPENSATION_EN == 1)
	WriteIO32(&pReg_DDRPHY->PHY_CON[8], C_Offset);		// ctrl_offsetc

	tGateCycle = (((U8)GateCycle[3] << 15) | ((U8)GateCycle[2] << 10) |
			((U8)GateCycle[1] << 5) | (U8)GateCycle[0]);
	WriteIO32(&pReg_DDRPHY->PHY_CON[3], tGateCycle);	// ctrl_shiftc
#endif // #if (DDR_RESET_GATE_LVL == 1)

#if (MEM_CALIBRAION_INFO == 1)
	Gate_Leveling_Information();
#endif

gate_err_ret:

	WriteIO32( &pReg_DDRPHY->PHY_CON[5],    0x0 );                          // readmodecon[7:0] = 0x0

	SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization
	ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization

	if (isResume == 0) {
#if defined(MEM_TYPE_DDR3)
		/* Set MPR mode disable */
		MR.Reg          = 0;
		MR.MR3.MPR      = 0;
		MR.MR3.MPR_RF   = 0;

		SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
		if(pSBI->DII.ChipNum > 1)
			SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#endif // #if defined(MEM_TYPE_DDR3)
	}

	MEMMSG("\r\n########## Gate Leveling - End ##########\r\n");

	return ret;
}
#endif // #if (DDR_GATE_LEVELING_EN == 1)

#if (DDR_READ_DQ_CALIB_EN == 1)

#if (MEM_CALIBRAION_INFO == 1)
void Read_DQ_Calibration_Information(void)
{
	unsigned int DQ_FailStatus, DQ_Calibration;
	unsigned int VWML[4], VWMR[4];
	unsigned int VWMC[4], Deskew[4];
	//	unsigned int RDCenter[8];
	unsigned int MaxSlice = 4, Slice;
#if 0	// Each DQ Line
	unsigned int MaxLane = 4, MaxSlice = 8, Lane, Slice;
#endif
	/* DQ Calibration Fail Status */
	WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_FAIL_STATUS);
	DQ_FailStatus = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);

	if (DQ_FailStatus == 0) {
		/* Vaile Window Margin Left */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_LEFT);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			VWML[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Vaile Window Margin Right */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_RIGHT);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			VWMR[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Vaile Window Margin Center */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], RD_VWMC);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			VWMC[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Vaile Window Margin Deskew */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], WR_DESKEW_CODE);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			Deskew[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;
	}

#if 0	// Each DQ Line
	for(Lane = 0; Lane < MaxLane; Lane) {
		unsigned int Code = 0x1;
		for(Slice = 0; Slice < MaxSlice; Slice++, Code+=0x10) {
			WriteIO32(&pReg_DDRPHY->PHY_CON[5], Code);
			RDCenter[Slice] = ReadIO32(pReg_DDRPHY->PHY_CON[18+1]);
		}
	}
#endif
	printf("\r\n#### Read DQ Calibration - Information ####\r\n");

	printf("Read DQ Calibration %s!! \r\n",
			(DQ_FailStatus == 0) ? "Success" : "Failed" );

	if (DQ_FailStatus == 0) {
	#if 0	// Display Type
		for(Slice = 0; Slice < MaxSlice; Slice++)
			printf("VWML0: %d, VWMC0: %d, VWML0: %d, Deskew0: %d \r\n",
					VWML[Slice], VWMC[Slice], VWMR[Slice], Deskew[Slice]);
	#else
		unsigned int Range;
		for(Slice = 0; Slice < MaxSlice; Slice++) {
			Range = VWMR[Slice] - VWML[Slice];
			printf("SLICE%d: %d ~ %d ~ %d (Range: %d)(Deskew: %d) \r\n",
					Slice, VWML[Slice], VWMC[Slice], VWMR[Slice],
					Range, Deskew[Slice]);
		}

	#endif

	#if 0	// Each Byte
		for(Lane = 0; Lane < MaxLane; Lane) {
			printf("Lane Number : %d \r\n", Lane );
			for(Slice = 0; Slice < MaxSlice; Slice++)
				printf("DQ%d : %d \r\n", Slice, RDCenter[Slice]);
		}
	#endif
	}
	printf("\r\n###########################################\r\n");

}
#endif

CBOOL DDR_Read_DQ_Calibration(U32 isResume)
{
#if defined(MEM_TYPE_DDR3)
	union SDRAM_MR MR;
#endif
	volatile U32 cal_count = 0;
	U32     temp;
	CBOOL   ret = CTRUE;

	MEMMSG("\r\n########## Read DQ Calibration - Start ##########\r\n");

	SetIO32  ( &pReg_DDRPHY->PHY_CON[14],   (0xF    <<  0) );               // ctrl_pulld_dqs[3:0] = 0xF, Need for MPR

	while( ReadIO32(&pReg_Drex->CHIPSTATUS) & 0xF ) { //- Until chip_busy_state
		nop();
	}


	if (isResume == 0) {
		SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
		SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
		if(pSBI->DII.ChipNum > 1)
			SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	}

#if defined(MEM_TYPE_DDR3)
	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
	//	temp |= 0xFF00;                                             // rdlvl_rddata_adj[15:0]
	temp |= 0x0100;                                                     // rdlvl_rddata_adj[15:0]
	WriteIO32( &pReg_DDRPHY->PHY_CON[1], temp );

	if (isResume == 0) {
		/* Set MPR mode enable */
		MR.Reg          = 0;
		MR.MR3.MPR      = 1;
		MR.MR3.MPR_RF   = 0;

		SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
		if(pSBI->DII.ChipNum > 1)
			SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
	}

	WriteIO32( &pReg_DDRPHY->PHY_CON[24+1],
			(0x0    << 16) |    // [31:16] ddr3_default
			(0x0    <<  1) |    // [15: 1] ddr3_address
			(0x0    <<  0) );   // [    0] ca_swap_mode
#endif
#if defined(MEM_TYPE_LPDDR23)
#if (DDR_CA_SWAP_MODE == 1)
	WriteIO32( &pReg_DDRPHY->PHY_CON[22+1],     0x00000041 );           	// lpddr2_addr[15:0]=0x041
#endif

	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
	temp |= 0x00FF;                                                     	// rdlvl_rddata_adj[15:0]
	//	temp |= 0x0001;                                                 	// rdlvl_rddata_adj[15:0]
	WriteIO32( &pReg_DDRPHY->PHY_CON[1],    temp );

	SendDirectCommand(SDRAM_CMD_MRR, 0, 32, 0x00);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRR, 1, 32, 0x00);
#endif
#else
	if (pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRR, 1, 32, 0x00);
#endif
#endif // #if defined(MEM_TYPE_LPDDR23)

	SetIO32(&pReg_DDRPHY->PHY_CON[2], (0x1 << 25)); 	// rdlvl_en[25]=1
	WriteIO32( &pReg_Drex->RDLVL_CONFIG, (0x1 << 1) );      // ctrl_rdlvl_data_en[1]=1 : Starting READ leveling

	if (isResume) {
		while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 );  // rdlvl_complete[14] : Wating until READ leveling is complete
		WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                         // ctrl_rdlvl_data_en[1]=0 : Stopping it after completion of READ leveling.
	} else {
		while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1 << 14) ) == 0 )   // rdlvl_complete[14] : Wating until READ leveling is complete
		{
			if (cal_count > 100) { // Failure Case
				WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                 // ctrl_rdlvl_data_en[1]=0 : Stopping it after completion of READ leveling.

				ret = CFALSE;
				goto rd_err_ret;
			} else {
				DMC_Delay(0x100);
			}
			cal_count++;
		}

		nop();
		WriteIO32( &pReg_Drex->RDLVL_CONFIG, 0x0 );                         // ctrl_rdlvl_data_en[1]=0 : Stopping it after completion of READ leveling.

		//- Checking the calibration failure status
		//- After setting PHY_CON5(0x14) to "0xC", check whether
		//	PHY_CON21 is zero or nor. If PHY_CON21(0x58) is zero, calibration is normally complete.

		WriteIO32( &pReg_DDRPHY->PHY_CON[5], VWM_FAIL_STATUS );          // readmodecon[7:0] = 0xC
		cal_count = 0;
		do {
			if (cal_count > 100) {
				MEMMSG("\r\n\nRD VWM_FAIL_STATUS Checking : fail...!!!\r\n");
				ret = CFALSE;                                               // Failure Case
				goto rd_err_ret;
			} else if (cal_count) {
				DMC_Delay(0x100);
			}
			cal_count++;
		} while (ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]) != 0x0);

		//*** Read DQ Calibration Valid Window Margin
		//------------------------------------------------------
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], RD_VWMC);
		g_RDvwmc = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);
		//--------------------------------------------------------
	}

#if (DDR_RESET_READ_DQ == 1)
	unsigned int Offset, C_Offset;;
	Offset = GetVWMC_Offset(g_RDvwmc, (g_DDRLock >> 2));
#if (DDR_READ_DQ_COMPENSATION_EN == 1)
	C_Offset = GetVWMC_Compensation(Offset);
#else
	C_Offset = Offset;
#endif // #if (DDR_READ_DQ_COMPENSATION_EN == 1)

	if (isResume == 1) 
		C_Offset = g_RDvwmc;
	else
		g_RDvwmc = C_Offset;
	// Read DQ Offset Apply.
	WriteIO32(&pReg_DDRPHY->PHY_CON[4], C_Offset);

	//*** Resync Update READ SDLL Code (ctrl_offsetr) : Make "ctrl_resync" HIGH and LOW
	SetIO32(&pReg_DDRPHY->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0x1 (HIGH)
	ClearIO32(&pReg_DDRPHY->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0x0 (LOW)
#endif	// #if (DDR_RESET_READ_DQ == 1)

#if (MEM_CALIBRAION_INFO == 1)
	Read_DQ_Calibration_Information();
#endif

rd_err_ret:

	//*** Set PHY0.CON2.rdlvl_en : setting MUX as "0" to force manually the result value of READ leveling.
	ClearIO32( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  25) );      // rdlvl_en[25]=0
	WriteIO32( &pReg_DDRPHY->PHY_CON[5],    0x0 );                  // readmodecon[7:0] = 0x0

	SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization
	ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization


	if (isResume == 0)
	{
#if defined(MEM_TYPE_DDR3)

		/* Set MPR mode disable */
		MR.Reg          = 0;
		MR.MR3.MPR      = 0;
		MR.MR3.MPR_RF   = 0;

		SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#else
		if(pSBI->DII.ChipNum > 1)
			SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR.Reg);
#endif
#endif
	}
	ClearIO32( &pReg_DDRPHY->PHY_CON[14],   (0xF << 0));              // ctrl_pulld_dqs[3:0]=0

	MEMMSG("\r\n########## Read DQ Calibration - End ##########\r\n");

	return ret;
}
#endif // #if (DDR_READ_DQ_CALIB_EN == 1)

#if (DDR_WRITE_LEVELING_CALIB_EN == 1)
void DDR_Write_Leveling_Calibration(void)
{
	MEMMSG("\r\n########## Write Leveling Calibration - Start ##########\r\n");
}
#endif

#if (DDR_WRITE_DQ_CALIB_EN == 1)

#if (MEM_CALIBRAION_INFO == 1)
void Write_DQ_Calibration_Information(void)
{
	unsigned int DQ_FailStatus, DQ_Calibration;
	unsigned int VWML[4], VWMR[4];
	unsigned int VWMC[4], Deskew[4];
	unsigned int MaxSlice = 4, Slice;
#if 0	// Each DQ Line
	unsigned int MaxLane = 4, MaxSlice = 8, Lane, Slice;
#endif
	/* DQ Calibration Fail Status */
	WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_FAIL_STATUS);
	DQ_FailStatus = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);

	if (DQ_FailStatus == 0) {
		/* Vaile Window Margin Left */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_LEFT);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			VWML[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Vaile Window Margin Right */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], VWM_RIGHT);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			VWMR[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Vaile Window Margin Center */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], WR_VWMC);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			VWMC[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;

		/* Vaile Window Margin Deskew */
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], WR_DESKEW_CODE);
		DQ_Calibration = ReadIO32(&pReg_DDRPHY->PHY_CON[19+1]);
		for(Slice = 0; Slice < MaxSlice; Slice++)
			Deskew[Slice] = (DQ_Calibration >> (Slice*8)) & 0xFF;
	}

	printf("\r\n### Write DQ Calibration - Information ####\r\n");

	printf("Write DQ Calibration %s!! \r\n",
			(DQ_FailStatus == 0) ? "Success" : "Failed" );

	if (DQ_FailStatus == 0) {
	#if 0	// Display Type
		for(Slice = 0; Slice < MaxSlice; Slice++)
			printf("VWML0: %d, VWMC0: %d, VWML0: %d, Deskew0: %d \r\n",
					VWML[Slice], VWMC[Slice], VWMR[Slice], Deskew[Slice]);
	#else
		unsigned int Range;
		for(Slice = 0; Slice < MaxSlice; Slice++) {
			Range = VWMR[Slice] - VWML[Slice];
			printf("SLICE%d: %d ~ %d ~ %d (Range: %d)(Deskew: %d) \r\n",
					Slice, VWML[Slice], VWMC[Slice], VWMR[Slice],
					Range, Deskew[Slice]);
		}
	#endif
	}
	printf("\r\n###########################################\r\n");
}
#endif

CBOOL DDR_Write_DQ_Calibration(U32 isResume)
{
	volatile U32 cal_count = 0;
	U32     temp;
	CBOOL   ret = CTRUE;

	MEMMSG("\r\n########## Write DQ Calibration - Start ##########\r\n");

	ClearIO32( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<  5) );               // ctrl_read_disable[5]=0. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.

#if 1
	SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	DMC_Delay(0x100);
#endif

	while (ReadIO32(&pReg_Drex->CHIPSTATUS) & 0xF) //- Until chip_busy_state
		nop();

	WriteIO32( &pReg_Drex->WRTRA_CONFIG,
			(0x0    << 16) |    // [31:16] row_addr
			(0x0    <<  1) |    // [ 3: 1] bank_addr
			(0x1    <<  0) );   // [    0] write_training_en

#if defined(MEM_TYPE_DDR3)
	WriteIO32( &pReg_DDRPHY->PHY_CON[24+1],
			(0x0    << 16) |    // [31:16] ddr3_default
			(0x0    <<  1) |    // [15: 1] ddr3_address
			(0x0    <<  0) );   // [    0] ca_swap_mode

	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
	if (ReadIO32( &pReg_DDRPHY->PHY_CON[0] ) & (0x1 << 13) )
		temp |= 0x0100;
	else
		temp |= 0xFF00;
	WriteIO32( &pReg_DDRPHY->PHY_CON[1],        temp );
#endif
#if defined(MEM_TYPE_LPDDR23)
	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1] ) & 0xFFFF0000;
	if (ReadIO32( &pReg_DDRPHY->PHY_CON[0] ) & (0x1 << 13) )
		temp |= 0x0001;
	else
		temp |= 0x00FF;
	WriteIO32(&pReg_DDRPHY->PHY_CON[1], temp);
#endif

	SetIO32(&pReg_DDRPHY->PHY_CON[0], (0x1 << 14));   // p0_cmd_en[14] = 1

	SetIO32(&pReg_DDRPHY->PHY_CON[2], (0x1 << 26));   // wr_deskew_con[26] = 1
	SetIO32(&pReg_DDRPHY->PHY_CON[2], (0x1 << 27));   // wr_deskew_en[27] = 1

	if (isResume) {
		while ((ReadIO32(&pReg_Drex->PHYSTATUS) & (0x1 << 14)) == 0); //- wait, rdlvl_complete[14]
		ClearIO32(&pReg_Drex->WRTRA_CONFIG, (0x1 << 0)); // write_training_en[0] = 0

		MEMMSG("\r\n########## Write DQ Calibration - End ##########\r\n");
	} else {
		while ((ReadIO32(&pReg_Drex->PHYSTATUS) & (0x1 << 14)) == 0) //- wait, rdlvl_complete[14]
		{
			if (cal_count > 100) { // Failure Case
				// cal_error = 1;
				ClearIO32(&pReg_Drex->WRTRA_CONFIG,
						(0x1 << 0)); // write_training_en[0]=0 : Stopping it after completion of WRITE leveling.
				MEMMSG("\r\n\nWD DQ CAL Status Checking : fail...!!!\r\n");

				ret = CFALSE;
				goto wr_err_ret;
			} else {
				DMC_Delay(0x100);
				// MEMMSG("r\n\nWD DQ CAL Status Checking :
				// %d\n", cal_count);
			}
			cal_count++;
		}

		nop();
		ClearIO32( &pReg_Drex->WRTRA_CONFIG,    (0x1 << 0) );       // write_training_en[0] = 0

		//- Checking the calibration failure status
		//- After setting PHY_CON5(0x14) to "0xC", check whether PHY_CON21 is zero or nor. 
		//  If PHY_CON21(0x58) is zero, calibration is normally complete.

		WriteIO32( &pReg_DDRPHY->PHY_CON[5], VWM_FAIL_STATUS );      // readmodecon[7:0] = 0xC
		cal_count = 0;
		do {
			if (cal_count > 100) {
				MEMMSG("\r\n\nWR VWM_FAIL_STATUS Checking : fail...!!!\r\n");
				ret = CFALSE;                                           // Failure Case
				goto wr_err_ret;
			} else if (cal_count) {
				DMC_Delay(0x100);
			}

			cal_count++;
		} while (ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]) != 0x0);

		//*** Write DQ Calibration Valid Window Margin
		//--------------------------------------------------------
		//    WriteIO32( &pReg_DDRPHY->PHY_CON[5],    VWM_CENTER);
		WriteIO32(&pReg_DDRPHY->PHY_CON[5], WR_VWMC);
		g_WRvwmc = ReadIO32(&pReg_DDRPHY->PHY_CON[19 + 1]);
		//---------------------------------------------------------
	}

#if (DDR_RESET_WRITE_DQ == 1)
	unsigned int Offset, C_Offset;
	Offset = GetVWMC_Offset(g_WRvwmc, (g_DDRLock >> 2));
#if (DDR_WRITE_DQ_COMPENSATION_EN == 1)
	C_Offset = GetVWMC_Compensation(Offset);
#else
	C_Offset = Offset;
#endif // #if (DDR_WRITE_DQ_COMPENSATION_EN == 1)

	if (isResume == 1)
		C_Offset = g_WRvwmc;
	else
		g_WRvwmc = C_Offset;
	/* Write DQ Offset Apply */
	WriteIO32(&pReg_DDRPHY->PHY_CON[6], C_Offset);

	//*** Resync Update WRITE SDLL Code (ctrl_offsetr) : Make "ctrl_resync" HIGH and LOW
	SetIO32(&pReg_DDRPHY->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0x1 (HIGH)
	ClearIO32(&pReg_DDRPHY->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0x0 (LOW)
#endif	// #if (DDR_RESET_WRITE_DQ == 1)

#if (MEM_CALIBRATION_INFO == 1)
	Write_DQ_Calibration_Information();
#endif

wr_err_ret:

	ClearIO32(&pReg_DDRPHY->PHY_CON[2], (0x3 << 26) );	// wr_deskew_en[27] = 0, wr_deskew_con[26] = 0

	WriteIO32(&pReg_DDRPHY->PHY_CON[5], 0x0);		// readmodecon[7:0] = 0x0

	SetIO32  (&pReg_Drex->PHYCONTROL[0], (0x1 << 3));	// Force DLL Resyncronization
	ClearIO32(&pReg_Drex->PHYCONTROL[0], (0x1 << 3));	// Force DLL Resyncronization

	MEMMSG("\r\n########## Write DQ Calibration - End ##########\r\n");

	return ret;
}
#endif  // #if (DDR_WRITE_DQ_CALIB_EN == 1)
#endif  // #if (DDR_NEW_LEVELING_TRAINING == 1)

/* DDR Lock Value - by.deoks */
#if 0
struct phy_lock_info
{
	U32 val;
	U32 count;
	U32 lock_count[5];
};

U32 g_Lock_Val;

void showLockValue(void)
{
	struct phy_lock_info lock_info[20];
	U32 fFound = 0;
	U32 lock_status, lock_val;
	U32 temp, i, j;

	for (i = 0; i < 20; i++) {
		lock_info[i].val        = 0;
		lock_info[i].count      = 0;

		for (j = 0; j < 5; j++)
			lock_info[i].lock_count[j]  = 0;
	}

	for (i = 0; i < 1000000; i++) {
		temp        = ReadIO32( &pReg_DDRPHY->PHY_CON[13] );
		lock_status = temp & 0x7;
		lock_val    = (temp >> 8) & 0x1FF;         // read lock value

		fFound = 0;

		for (j = 0; lock_info[j].val != 0; j++) {
			if (lock_info[j].val == lock_val) {
				fFound = 1;
				lock_info[j].count++;
				if (lock_status)
					lock_info[j].lock_count[(lock_status>>1)]++;
				else
					lock_info[j].lock_count[4]++;
			}
		}

		if (j == 20)
			break;

		if (fFound == 0) {
			lock_info[j].val   = lock_val;
			lock_info[j].count = 1;
			if (lock_status)
				lock_info[j].lock_count[(lock_status>>1)] = 1;
			else
				lock_info[j].lock_count[4]  = 1;
		}

		DMC_Delay(10);
	}

#if 1 // Descending Sort - by.deoks
	struct phy_lock_info* me 
		= (struct phy_lock_info*)(&lock_info[0]);

	unsigned int array_size = 20, k = 0;
	for (i = 0; i < array_size; i++) {
		for( j = 0; j < array_size-1; j++)
		{
			int swap = 0;
			if( (me[j].val != 0) && (me[j+1].val != 0) )
			{
				if( me[j].val > me[j+1].val )
				{
					swap = me[j].val;
					me[j].val = me[j+1].val;
					me[j+1].val = swap;

					swap = me[j].count;
					me[j].count = me[j+1].count;
					me[j+1].count = swap;

					for (k = 0; k < 4; k++)
					{
						swap = me[j].lock_count[k];
						me[j].lock_count[k] = me[j+1].lock_count[k];
						me[j+1].lock_count[k] = swap;			
					}
				}	
			}
		}
	}
#endif	

	printf("\r\n");
#if 0
	printf("--------------------------------------\r\n");
	printf(" Show lock values : %d\r\n", g_DDRLock );
	printf("--------------------------------------\r\n");
#endif
	printf("lock_val,   hit       bad, not bad,   good, better,   best\r\n");

	for (i = 0; lock_info[i].val; i++) {
		printf("[%6d, %6d] - [%6d", lock_info[i].val, lock_info[i].count, lock_info[i].lock_count[4]);

		for (j = 0; j < 4; j++)
			printf(", %6d", lock_info[i].lock_count[j]);
		printf("]\r\n");
	}
}
#endif

void init_DDR3(U32 isResume)
{
	union SDRAM_MR MR0, MR1, MR2, MR3;
	U32 DDR_AL, DDR_WL, DDR_RL;
	U32 lock_div4;
	U32 temp;

	MR0.Reg = 0;
	MR1.Reg = 0;
	MR2.Reg = 0;
	MR3.Reg = 0;

	MEMMSG("\r\nDDR3 POR Init Start\r\n");

	// Step 1. reset (Min : 10ns, Typ : 200us)
	ClearIO32(&pReg_RstCon->REGRST[0], (0x7 << 26));	//Reset Pin - High
	DMC_Delay(0x1000); 									// wait 300ms
	SetIO32(&pReg_RstCon->REGRST[0], (0x7 << 26));		//Reset Pin - Low
	DMC_Delay(0x1000); 									// wait 300ms
	ClearIO32(&pReg_RstCon->REGRST[0], (0x7 << 26));	//Reset Pin - High
	DMC_Delay(0x1000); 									// wait 300ms
	SetIO32(&pReg_RstCon->REGRST[0], (0x7 << 26));		//Reset Pin - Low
	//    DMC_Delay(0x10000);                                        // wait 300ms

#if 0
	ClearIO32( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
	DMC_Delay(0x1000);                                          // wait 300ms
	SetIO32  ( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
	DMC_Delay(0x1000);                                          // wait 300ms
	ClearIO32( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
	DMC_Delay(0x1000);                                          // wait 300ms
	SetIO32  ( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
#endif
	DMC_Delay(0x10000); // wait 300ms

	//    MEMMSG("PHY Version: %X\r\n",
	//    ReadIO32(&pReg_DDRPHY->VERSION_INFO));

	if (isResume) {
		WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1); // open alive power gate
		WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1); // open alive power gate

		g_GateCycle = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE5);    // read - ctrl_shiftc
		g_GateCode  = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE6);    // read - ctrl_offsetc
		g_RDvwmc    = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE7);    // read - ctrl_offsetr
		g_WRvwmc    = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE8);    // read - ctrl_offsetw

		WriteIO32(&pReg_Alive->ALIVEPWRGATEREG,     0);             // close alive power gate
		WriteIO32(&pReg_Alive->ALIVEPWRGATEREG,     0);             // close alive power gate
	}

	if (!g_GateCycle || !g_RDvwmc || !g_WRvwmc)
		isResume = 0;

#if 0
#if (CFG_NSIH_EN == 0)
	MEMMSG("READDELAY   = 0x%08X\r\n", READDELAY);
	MEMMSG("WRITEDELAY  = 0x%08X\r\n", WRITEDELAY);
#else
	MEMMSG("READDELAY   = 0x%08X\r\n", pSBI->DII.READDELAY);
	MEMMSG("WRITEDELAY  = 0x%08X\r\n", pSBI->DII.WRITEDELAY);
#endif
#endif

#if (CFG_NSIH_EN == 0)
	// pSBI->LvlTr_Mode    = ( LVLTR_WR_LVL | LVLTR_CA_CAL | LVLTR_GT_LVL |
	//							LVLTR_RD_CAL | LVLTR_WR_CAL );
	// pSBI->LvlTr_Mode    = ( LVLTR_GT_LVL | LVLTR_RD_CAL | LVLTR_WR_CAL );
	pSBI->LvlTr_Mode = LVLTR_GT_LVL;
	//	pSBI->LvlTr_Mode    = 0;
#endif

#if (CFG_NSIH_EN == 0)
#if 1   // Common
	pSBI->DDR3_DSInfo.MR2_RTT_WR    = 2;    // RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
	pSBI->DDR3_DSInfo.MR1_ODS       = 1;    // ODS - 00: RZQ/6, 01 : RZQ/7
	pSBI->DDR3_DSInfo.MR1_RTT_Nom   = 2;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8

	pSBI->PHY_DSInfo.DRVDS_Byte3    = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte2    = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte1    = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte0    = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_CK       = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_CKE      = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_CS       = PHY_DRV_STRENGTH_240OHM;
	pSBI->PHY_DSInfo.DRVDS_CA       = PHY_DRV_STRENGTH_240OHM;

	pSBI->PHY_DSInfo.ZQ_DDS         = PHY_DRV_STRENGTH_48OHM;
	pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_120OHM;
#endif

#if 0   // DroneL 720Mhz
	pSBI->DDR3_DSInfo.MR2_RTT_WR    = 1;    // RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
	pSBI->DDR3_DSInfo.MR1_ODS       = 1;    // ODS - 00: RZQ/6, 01 : RZQ/7
	pSBI->DDR3_DSInfo.MR1_RTT_Nom   = 3;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8

	pSBI->PHY_DSInfo.DRVDS_Byte3    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte2    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte1    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte0    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_CK       = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_CKE      = PHY_DRV_STRENGTH_30OHM;
	pSBI->PHY_DSInfo.DRVDS_CS       = PHY_DRV_STRENGTH_30OHM;
	pSBI->PHY_DSInfo.DRVDS_CA       = PHY_DRV_STRENGTH_30OHM;

	pSBI->PHY_DSInfo.ZQ_DDS         = PHY_DRV_STRENGTH_40OHM;
	//    pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_80OHM;
	pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_60OHM;
#endif

#if 0   // DroneL 800Mhz
	pSBI->DDR3_DSInfo.MR2_RTT_WR    = 2;    // RTT_WR - 0: ODT disable, 1: RZQ/4, 2: RZQ/2
	pSBI->DDR3_DSInfo.MR1_ODS       = 1;    // ODS - 00: RZQ/6, 01 : RZQ/7
	pSBI->DDR3_DSInfo.MR1_RTT_Nom   = 3;    // RTT_Nom - 001: RZQ/4, 010: RZQ/2, 011: RZQ/6, 100: RZQ/12, 101: RZQ/8

	pSBI->PHY_DSInfo.DRVDS_Byte3    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte2    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte1    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_Byte0    = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_CK       = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.DRVDS_CKE      = PHY_DRV_STRENGTH_30OHM;
	pSBI->PHY_DSInfo.DRVDS_CS       = PHY_DRV_STRENGTH_30OHM;
	pSBI->PHY_DSInfo.DRVDS_CA       = PHY_DRV_STRENGTH_30OHM;

	pSBI->PHY_DSInfo.ZQ_DDS         = PHY_DRV_STRENGTH_40OHM;
	pSBI->PHY_DSInfo.ZQ_ODT         = PHY_DRV_STRENGTH_120OHM;
#endif
#endif

	DDR_AL = 0;
#if (CFG_NSIH_EN == 0)
	if (MR1_nAL > 0)
		DDR_AL = nCL - MR1_nAL;

	DDR_WL = (DDR_AL + nCWL);
	DDR_RL = (DDR_AL + nCL);
#else
	if (pSBI->DII.MR1_AL > 0)
		DDR_AL = pSBI->DII.CL - pSBI->DII.MR1_AL;

	DDR_WL = (DDR_AL + pSBI->DII.CWL);
	DDR_RL = (DDR_AL + pSBI->DII.CL);
#endif

	MR2.Reg         = 0;
	MR2.MR2.RTT_WR  = pSBI->DDR3_DSInfo.MR2_RTT_WR;
	MR2.MR2.SRT     = 0; // self refresh normal range
	MR2.MR2.ASR     = 0; // auto self refresh disable
#if (CFG_NSIH_EN == 0)
	MR2.MR2.CWL     = (nCWL - 5);
#else
	MR2.MR2.CWL     = (pSBI->DII.CWL - 5);
#endif

	MR3.Reg         = 0;
	MR3.MR3.MPR     = 0;
	MR3.MR3.MPR_RF  = 0;

	MR1.Reg         = 0;
	MR1.MR1.DLL     = 0;    // 0: Enable, 1 : Disable
#if (CFG_NSIH_EN == 0)
	MR1.MR1.AL      = MR1_nAL;
#else
	MR1.MR1.AL      = pSBI->DII.MR1_AL;
#endif
	MR1.MR1.ODS1 	= (pSBI->DDR3_DSInfo.MR1_ODS>>1) & 1;
	MR1.MR1.ODS0 	= (pSBI->DDR3_DSInfo.MR1_ODS>>0) & 1;
	MR1.MR1.RTT_Nom2 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>2) & 1;
	MR1.MR1.RTT_Nom1 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>1) & 1;
	MR1.MR1.RTT_Nom0 = (pSBI->DDR3_DSInfo.MR1_RTT_Nom>>0) & 1;
	MR1.MR1.QOff    = 0;
	MR1.MR1.WL      = 0;
#if 0
#if (CFG_NSIH_EN == 0)
	MR1.MR1.TDQS    = (_DDR_BUS_WIDTH>>3) & 1;
#else
	MR1.MR1.TDQS    = (pSBI->DII.BusWidth>>3) & 1;
#endif
#endif

#if (CFG_NSIH_EN == 0)
	if (nCL > 11)
		temp = ((nCL-12) << 1) + 1;
	else
		temp = ((nCL-4) << 1);
#else
	if (pSBI->DII.CL > 11)
		temp = ((pSBI->DII.CL-12) << 1) + 1;
	else
		temp = ((pSBI->DII.CL-4) << 1);
#endif

	MR0.Reg         = 0;
	MR0.MR0.BL      = 0;
	MR0.MR0.BT      = 1;
	MR0.MR0.CL0     = (temp & 0x1);
	MR0.MR0.CL1     = ((temp>>1) & 0x7);
	MR0.MR0.DLL     = 0;//1;
#if (CFG_NSIH_EN == 0)
	MR0.MR0.WR      = MR0_nWR;
#else
	MR0.MR0.WR      = pSBI->DII.MR0_WR;
#endif
	MR0.MR0.PD      = 0;//1;


	// Step 2. Select Memory type : DDR3
	// Check DDR3 MPR data and match it to PHY_CON[1]??

#if defined(MEM_TYPE_DDR3)
	WriteIO32( &pReg_DDRPHY->PHY_CON[1],
			(0x0    <<  28) |           // [31:28] ctrl_gateadj
			(0x9    <<  24) |           // [27:24] ctrl_readadj
			(0x0    <<  20) |           // [23:20] ctrl_gateduradj  :: DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
			(0x1    <<  16) |           // [19:16] rdlvl_pass_adj
			//        (0x0100 <<   0) );          // [15: 0] rdlvl_rddata_adj :: DDR3 : 0x0100 or 0xFF00
		(0xFF00 <<   0) );          // [15: 0] rdlvl_rddata_adj :: DDR3 : 0x0100 or 0xFF00
#endif
#if defined(MEM_TYPE_LPDDR23)
	WriteIO32( &pReg_DDRPHY->PHY_CON[1],
			(0x0    <<  28) |           // [31:28] ctrl_gateadj
			(0x9    <<  24) |           // [27:24] ctrl_readadj
			(0xB    <<  20) |           // [23:20] ctrl_gateduradj  :: DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
			(0x1    <<  16) |           // [19:16] rdlvl_pass_adj
			(0x0001 <<   0) );          // [15: 0] rdlvl_rddata_adj :: LPDDR3 : 0x0001 or 0x00FF
	//        (0x00FF <<   0) );          // [15: 0] rdlvl_rddata_adj :: LPDDR3 : 0x0001 or 0x00FF
#endif

	WriteIO32( &pReg_DDRPHY->PHY_CON[2],
			(0x0    <<  28) |           // [31:28] ctrl_readduradj
			(0x0    <<  27) |           // [   27] wr_deskew_en
			(0x0    <<  26) |           // [   26] wr_deskew_con
			(0x0    <<  25) |           // [   25] rdlvl_en
			(0x0    <<  24) |           // [   24] rdlvl_gate_en
			(0x0    <<  23) |           // [   23] rdlvl_ca_en
			(0x1    <<  16) |           // [22:16] rdlvl_incr_adj
			(0x0    <<  14) |           // [   14] wrdeskew_clear
			(0x0    <<  13) |           // [   13] rddeskew_clear
			(0x0    <<  12) |           // [   12] dlldeskew_en
			(0x2    <<  10) |           // [11:10] rdlvl_start_adj - Right shift, valid value: 1 or 2
			(0x1    <<   8) |           // [ 9: 8] rdlvl_start_adj - Left shift,  valid value: 0 ~ 2
			(0x0    <<   6) |           // [    6] initdeskewen
			(0x0    <<   0) );          // [ 1: 0] rdlvl_gateadj

	temp = ((0x0    <<  29) |           // [31:29] Reserved - SBZ.
			(0x17   <<  24) |           // [28:24] T_WrWrCmd.
			//		(0x0    <<  22) |           // [23:22] Reserved - SBZ.
			(0x0    <<  20) |           // [21:20] ctrl_upd_range.
#if (CFG_NSIH_EN == 0)
#if (tWTR == 3)     // 6 cycles
			(0x7    <<  17) |           // [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#elif (tWTR == 2)   // 4 cycles
			(0x6    <<  17) |           // [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#endif
#endif
			(0x0    <<  16) |           // [   16] ctrl_wrlvl_en[16]. Write Leveling Enable. 0:Disable, 1:Enable
			//        (0x0    <<  15) |           // [   15] Reserved SBZ.
			(0x0    <<  14) |           // [   14] p0_cmd_en. 0:Issue Phase1 Read command during Read Leveling. 1:Issue Phase0
			(0x0    <<  13) |           // [   13] byte_rdlvl_en. Read Leveling 0:Disable, 1:Enable
#if defined(MEM_TYPE_DDR3)
			(0x1    <<  11) |           // [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
#endif
#if defined(MEM_TYPE_LPDDR23)
			(0x3    <<  11) |           // [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
#endif
			//        (0x0    <<  10) |           // [   10] Reserved - SBZ.
			(0x1    <<   9) |           // [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
			//        (0x1    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
			(0x0    <<   7) |           // [    7] ctrl_ckdis. 0:Clock output Enable, 1:Disable
			//        (0x1    <<   6) |           // [    6] ctrl_atgate.
			//        (0x1    <<   5) |           // [    5] ctrl_read_disable. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
			(0x0    <<   4) |           // [    4] ctrl_cmosrcv.
			(0x0    <<   3) |           // [    3] ctrl_read_width.
			(0x0    <<   0));           // [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

#if (CFG_NSIH_EN == 1)
	if ((pSBI->DII.TIMINGDATA >> 28) == 3)      // 6 cycles
		temp |= (0x7    <<  17);
	else if ((pSBI->DII.TIMINGDATA >> 28) == 2) // 4 cycles
		temp |= (0x6    <<  17);
#endif
	WriteIO32( &pReg_DDRPHY->PHY_CON[0],    temp );

	MEMMSG("phy init\r\n");

	/*  Set ZQ clock div    */
	WriteIO32( &pReg_DDRPHY->PHY_CON[40+1], 0x07);

	/*  Set ZQ Timer    */
	//    WriteIO32( &pReg_DDRPHY->PHY_CON[41+1], 0xF0);

	/* Set WL, RL, BL */
	WriteIO32( &pReg_DDRPHY->PHY_CON[42+1],
			(0x8    <<   8) |       // Burst Length(BL)
			(DDR_RL <<   0));       // Read Latency(RL) - 800MHz:0xB, 533MHz:0x5

	/* Set WL  */
#if defined(MEM_TYPE_DDR3)
	temp = ((0x105E <<  16)| 0x107E);                               // cmd_active= DDR3:0x105E, DDR3:0x107E
	WriteIO32( &pReg_DDRPHY->PHY_CON[25+1], temp);

#if (CFG_NSIH_EN == 0)
	WriteIO32( &pReg_DDRPHY->PHY_CON[26+1],
			(DDR_WL <<  16) |       // T_wrdata_en, In DDR3
			(0x1    <<  12) |       // [  :12] RESET
			(0x0    <<   9) |       // [11: 9] BANK
			(0x0    <<   7) |       // [ 8: 7] ODT
			(0x1    <<   6) |       // [  : 6] RAS
			(0x1    <<   5) |       // [  : 5] CAS
			(0x1    <<   4) |       // [  : 4] WEN
#if (_DDR_CS_NUM > 1)
			(0x3    <<   2) |       // [ 3: 2] CKE[1:0]
			(0x3    <<   0) );      // [ 1: 0] CS[1:0]
#else
	(0x1    <<   2) |       // [ 3: 2] CKE[1:0]
		(0x1    <<   0) );      // [ 1: 0] CS[1:0]
#endif
#else
	if (pSBI->DII.ChipNum > 1) {
		WriteIO32(&pReg_DDRPHY->PHY_CON[26 + 1],
				(DDR_WL << 16) |  // T_wrdata_en, In DDR3
				(0x1 << 12) | // [  :12] RESET
				(0x0 << 9) |  // [11: 9] BANK
				(0x0 << 7) |  // [ 8: 7] ODT
				(0x1 << 6) |  // [  : 6] RAS
				(0x1 << 5) |  // [  : 5] CAS
				(0x1 << 4) |  // [  : 4] WEN
				(0x3 << 2) |  // [ 3: 2] CKE[1:0]
				(0x3 << 0));  // [ 1: 0] CS[1:0]
	} else {
		WriteIO32(&pReg_DDRPHY->PHY_CON[26 + 1],
				(DDR_WL << 16) |  // T_wrdata_en, In DDR3
				(0x1 << 12) | // [  :12] RESET
				(0x0 << 9) |  // [11: 9] BANK
				(0x0 << 7) |  // [ 8: 7] ODT
				(0x1 << 6) |  // [  : 6] RAS
				(0x1 << 5) |  // [  : 5] CAS
				(0x1 << 4) |  // [  : 4] WEN
				(0x1 << 2) |  // [ 3: 2] CKE[1:0]
				(0x1 << 0));  // [ 1: 0] CS[1:0]
	}
#endif // #if (CFG_NSIH_EN == 0)
#endif // #if defined(MEM_TYPE_DDR3)
#if defined(MEM_TYPE_LPDDR23)
	temp = ((0x105E <<  16)| 0x000E);                               // cmd_active= DDR3:0x105E, LPDDDR2 or LPDDDR3:0x000E
	WriteIO32( &pReg_DDRPHY->PHY_CON[25+1], temp);

#if (CFG_NSIH_EN == 0)
	WriteIO32( &pReg_DDRPHY->PHY_CON[26+1],
			((DDR_WL+1) <<  16) |   // T_wrdata_en, In LPDDR3 (WL+1)
#if (_DDR_CS_NUM > 1)
			(0x3    <<   2) |       // [ 3: 2] CKE[1:0]
			(0x3    <<   0) );      // [ 1: 0] CS[1:0]
#else
	(0x1    <<   2) |       // [ 3: 2] CKE[1:0]
		(0x1    <<   0) );      // [ 1: 0] CS[1:0]
#endif
#else
	if (pSBI->DII.ChipNum > 1) {
		WriteIO32(&pReg_DDRPHY->PHY_CON[26 + 1],
				((DDR_WL + 1) << 16) |
				(0x3 << 2) | // [ 3: 2] CKE[1:0]
				(0x3 << 0)); // [ 1: 0] CS[1:0]
	} else {
		WriteIO32(&pReg_DDRPHY->PHY_CON[26 + 1],
				((DDR_WL + 1) << 16) |
				(0x1 << 2) | // [ 3: 2] CKE[1:0]
				(0x1 << 0)); // [ 1: 0] CS[1:0]
	}
#endif // #if (CFG_NSIH_EN == 0)
#endif // #if defined(MEM_TYPE_LPDDR23)

	/* ZQ Calibration */
#if 0
	WriteIO32( &pReg_DDRPHY->PHY_CON[39+1],         // 100: 48ohm, 101: 40ohm, 110: 34ohm, 111: 30ohm
			(pSBI->PHY_DSInfo.DRVDS_Byte3 <<  25) |     // [27:25] Data Slice 3
			(pSBI->PHY_DSInfo.DRVDS_Byte2 <<  22) |     // [24:22] Data Slice 2
			(pSBI->PHY_DSInfo.DRVDS_Byte1 <<  19) |     // [21:19] Data Slice 1
			(pSBI->PHY_DSInfo.DRVDS_Byte0 <<  16) |     // [18:16] Data Slice 0
			(pSBI->PHY_DSInfo.DRVDS_CK    <<   9) |     // [11: 9] CK
			(pSBI->PHY_DSInfo.DRVDS_CKE   <<   6) |     // [ 8: 6] CKE
			(pSBI->PHY_DSInfo.DRVDS_CS    <<   3) |     // [ 5: 3] CS
			(pSBI->PHY_DSInfo.DRVDS_CA    <<   0));     // [ 2: 0] CA[9:0], RAS, CAS, WEN, ODT[1:0], RESET, BANK[2:0]
#else
	WriteIO32( &pReg_DDRPHY->PHY_CON[39+1],     0x00 );
#endif

	// Driver Strength(zq_mode_dds), zq_clk_div_en[18]=Enable
	WriteIO32( &pReg_DDRPHY->PHY_CON[16],
			(0x1    <<  27) |                       // [   27] zq_clk_en. ZQ I/O clock enable.
#if 0
			(PHY_DRV_STRENGTH_48OHM     <<  24) |   // [26:24] zq_mode_dds, Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
			(PHY_DRV_STRENGTH_120OHM    <<  21) |   // [23:21] ODT resistor value. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
#else
			(pSBI->PHY_DSInfo.ZQ_DDS    <<  24) |   // [26:24] zq_mode_dds, Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
			(pSBI->PHY_DSInfo.ZQ_ODT    <<  21) |   // [23:21] ODT resistor value. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
#endif
			(0x0    <<  20) |                       // [   20] zq_rgddr3. GDDR3 mode. 0:Enable, 1:Disable
			(0x0    <<  19) |                       // [   19] zq_mode_noterm. Termination. 0:Enable, 1:Disable
			(0x1    <<  18) |                       // [   18] zq_clk_div_en. Clock Dividing Enable : 0, Disable : 1
			(0x0    <<  15) |                       // [17:15] zq_force-impn
			(0x0    <<  12) |                       // [14:12] zq_force-impp
			(0x30   <<   4) |                       // [11: 4] zq_udt_dly
			(0x1    <<   2) |                       // [ 3: 2] zq_manual_mode. 0:Force Calibration, 1:Long cali, 2:Short cali
			(0x0    <<   1) |                       // [    1] zq_manual_str. Manual Calibration Stop : 0, Start : 1
			(0x0    <<   0));                       // [    0] zq_auto_en. Auto Calibration enable

	SetIO32( &pReg_DDRPHY->PHY_CON[16],     (0x1    <<   1) );          // zq_manual_str[1]. Manual Calibration Start=1
	while( ( ReadIO32( &pReg_DDRPHY->PHY_CON[17+1] ) & 0x1 ) == 0 );    //- PHY0: wait for zq_done
	ClearIO32( &pReg_DDRPHY->PHY_CON[16],   (0x1    <<   1) );          // zq_manual_str[1]. Manual Calibration Stop : 0, Start : 1

	ClearIO32( &pReg_DDRPHY->PHY_CON[16],   (0x1    <<  18) );          // zq_clk_div_en[18]. Clock Dividing Enable : 1, Disable : 0


	// Step 3. Set the PHY for dqs pull down mode
	WriteIO32( &pReg_DDRPHY->PHY_CON[14],
			(0x0    <<   8) |           // ctrl_pulld_dq[11:8]
			(0xF    <<   0));           // ctrl_pulld_dqs[7:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)
	// Step 4. ODT
	WriteIO32( &pReg_Drex->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1    <<  31) |           // [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1    <<  30) |           // [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0    <<   0) |           // [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x1    <<  29) |           // [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0    <<  24) |           // [28:24] ctrl_pd. Input Gate for Power Down.
			//        (0x0    <<   7) |           // [23: 7] Reserved - SBZ
			(0x0    <<   4) |           // [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			//        (0x1    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   2) |           // [    2] Reserved - SBZ
			(0x0    <<   1));           // [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1

#if 1
	WriteIO32( &pReg_Drex->CONCONTROL,
			(0x0    <<  28) |           // [   28] dfi_init_start
			(0xFFF  <<  16) |           // [27:16] timeout_level0
			//        (0x3    <<  12) |           // [14:12] rd_fetch
			(0x1    <<  12) |           // [14:12] rd_fetch
			(0x1    <<   8) |           // [    8] empty
			//        (0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0    <<   3) |           // [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0    <<   1));           // [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved
#else

	temp = (U32)(
			(0x0    <<  28) |           // [   28] dfi_init_start
			(0xFFF  <<  16) |           // [27:16] timeout_level0
			(0x3    <<  12) |           // [14:12] rd_fetch
			(0x1    <<   8) |           // [    8] empty
			(0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0    <<   3) |           // [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0    <<   1));           // [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved

	if (isResume)
		temp &= ~(0x1    <<   5);

	WriteIO32( &pReg_Drex->CONCONTROL,  temp );
#endif

	// Step 5. dfi_init_start : High
	SetIO32( &pReg_Drex->CONCONTROL,    (0x1    <<  28) );          // DFI PHY initialization start
	while( (ReadIO32( &pReg_Drex->PHYSTATUS ) & (0x1<<3) ) == 0);   // wait for DFI PHY initialization complete
	ClearIO32( &pReg_Drex->CONCONTROL,  (0x1    <<  28) );          // DFI PHY initialization clear

	WriteIO32( &pReg_DDRPHY->PHY_CON[12],
			(0x10   <<  24) |           // [30:24] ctrl_start_point
			(0x10   <<  16) |           // [22:16] ctrl_inc
			(0x0    <<   8) |           // [14: 8] ctrl_force
			(0x1    <<   6) |           // [    6] ctrl_start
			(0x1    <<   5) |           // [    5] ctrl_dll_on
			(0xF    <<   1));           // [ 4: 1] ctrl_ref
	DMC_Delay(1);


	// Step 8 : Update DLL information
	SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization
	ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );      // Force DLL Resyncronization


	// Step 11. MemBaseConfig
	WriteIO32( &pReg_Drex->MEMBASECONFIG[0],
			(0x0        <<  16) |                   // chip_base[26:16]. AXI Base Address. if 0x20 ==> AXI base addr of memory : 0x2000_0000
#if (CFG_NSIH_EN == 0)
			(chip_mask  <<   0));                   // 256MB:0x7F0, 512MB: 0x7E0, 1GB:0x7C0, 2GB: 0x780, 4GB:0x700
#else
	(pSBI->DII.ChipMask <<   0));           // chip_mask[10:0]. 1GB:0x7C0, 2GB:0x780
#endif

#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	{
		WriteIO32( &pReg_Drex->MEMBASECONFIG[1],
//			(chip_base1 <<  16) |               // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
			(0x040      <<  16) |               // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
			(chip_mask  <<   0));               // chip_mask[10:0]. 2048 - chip size
	}
#endif
#else
	if(pSBI->DII.ChipNum > 1)
	{
		WriteIO32( &pReg_Drex->MEMBASECONFIG[1],
				(pSBI->DII.ChipBase <<  16) |       // chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
				(pSBI->DII.ChipMask <<   0));       // chip_mask[10:0]. 2048 - chip size
	}
#endif

	// Step 12. MemConfig
	WriteIO32( &pReg_Drex->MEMCONFIG[0],
			//        (0x0    <<  16) |           // [31:16] Reserved - SBZ
			(0x1    <<  12) |           // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0:Linear(Bank, Row, Column, Width), 1:Interleaved(Row, bank, column, width), other : reserved
#if (CFG_NSIH_EN == 0)
			(chip_col   <<   8) |       // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
			(chip_row   <<   4) |       // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
#else
			(pSBI->DII.ChipCol  <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
			(pSBI->DII.ChipRow  <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
#endif
			(0x3    <<   0));           // [ 3: 0] chip_bank. Number of  Bank Address Bit. others:Reserved, 2:4bank, 3:8banks


#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	WriteIO32( &pReg_Drex->MEMCONFIG[1],
//			(0x0    <<  16) |       // [31:16] Reserved - SBZ
			(0x1    <<  12) |       // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
			(chip_col   <<   8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
			(chip_row   <<   4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
			(0x3    <<   0));       // [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
#endif
#else
	if(pSBI->DII.ChipNum > 1) {
		WriteIO32( &pReg_Drex->MEMCONFIG[1],
//			(0x0    <<  16) |       // [31:16] Reserved - SBZ
			(0x1    <<  12) |       // [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
			(pSBI->DII.ChipCol << 8) |   // [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
			(pSBI->DII.ChipRow << 4) |   // [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
			(0x3    <<   0));       // [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
	}
#endif

	// Step 13. Precharge Configuration
	WriteIO32( &pReg_Drex->PRECHCONFIG,     0xFF000000 );           //- precharge policy counter
	WriteIO32( &pReg_Drex->PWRDNCONFIG,     0xFFFF00FF );           //- low power counter

	// Step 14.  AC Timing
#if (CFG_NSIH_EN == 0)
	WriteIO32( &pReg_Drex->TIMINGAREF,
			(tREFI      <<   0));       //- refresh counter, 800MHz : 0x618

	WriteIO32( &pReg_Drex->TIMINGROW,
			(tRFC       <<  24) |
			(tRRD       <<  20) |
			(tRP        <<  16) |
			(tRCD       <<  12) |
			(tRC        <<   6) |
			(tRAS       <<   0)) ;
	WriteIO32( &pReg_Drex->TIMINGDATA,
			(tWTR       <<  28) |
			(tWR        <<  24) |
			(tRTP       <<  20) |
			(W2W_C2C    <<  16) |
			(R2R_C2C    <<  15) |
			(tDQSCK     <<  12) |
			(nWL        <<   8) |
			(nRL        <<   0));

	WriteIO32( &pReg_Drex->TIMINGPOWER,
			(tFAW       <<  26) |
			(tXSR       <<  16) |
			(tXP        <<   8) |
			(tCKE       <<   4) |
			(tMRD       <<   0));

	//    WriteIO32( &pReg_Drex->TIMINGPZQ,   0x00004084 );     //- average periodic ZQ interval. Max:0x4084
	WriteIO32( &pReg_Drex->TIMINGPZQ,   tPZQ );           //- average periodic ZQ interval. Max:0x4084
#else

	WriteIO32( &pReg_Drex->TIMINGAREF,      pSBI->DII.TIMINGAREF );     //- refresh counter, 800MHz : 0x618
	WriteIO32( &pReg_Drex->TIMINGROW,       pSBI->DII.TIMINGROW) ;
	WriteIO32( &pReg_Drex->TIMINGDATA,      pSBI->DII.TIMINGDATA );
	WriteIO32( &pReg_Drex->TIMINGPOWER,     pSBI->DII.TIMINGPOWER );

	//    WriteIO32( &pReg_Drex->TIMINGPZQ,       0x00004084 );               //- average periodic ZQ interval. Max:0x4084
	WriteIO32( &pReg_Drex->TIMINGPZQ,       pSBI->DII.TIMINGPZQ );      //- average periodic ZQ interval. Max:0x4084
#endif

#if 0
#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
#if (CFG_NSIH_EN == 0)
	WriteIO32( &pReg_DDRPHY->PHY_CON[4],    READDELAY);
	WriteIO32( &pReg_DDRPHY->PHY_CON[6],    WRITEDELAY);
#else
	WriteIO32( &pReg_DDRPHY->PHY_CON[4],    pSBI->DII.READDELAY);
	WriteIO32( &pReg_DDRPHY->PHY_CON[6],    pSBI->DII.WRITEDELAY);
#endif
#endif
#if defined(ARCH_NXP5430)
#if (CFG_NSIH_EN == 0)
	WriteIO32( &pReg_DDRPHY->OFFSETR_CON[0], READDELAY);
	WriteIO32( &pReg_DDRPHY->OFFSETW_CON[0], WRITEDELAY);
#else
	WriteIO32( &pReg_DDRPHY->OFFSETR_CON[0], pSBI->DII.READDELAY);
	WriteIO32( &pReg_DDRPHY->OFFSETW_CON[0], pSBI->DII.WRITEDELAY);
#endif
#endif
#endif

#if 0
	WriteIO32( &pReg_DDRPHY->PHY_CON[4],    0x08080808 );
	WriteIO32( &pReg_DDRPHY->PHY_CON[6],    0x08080808 );
#endif

	// Step 52  auto refresh start.
	//    SetIO32( &pReg_Drex->CONCONTROL,        (0x1    <<   5));            // afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1

	// Step 16. Confirm that after RESET# is de-asserted, 500 us have passed before CKE becomes active.
	// Step 17. Confirm that clocks(CK, CK#) need to be started and
	//     stabilized for at least 10 ns or 5 tCK (which is larger) before CKE goes active.

	// Step 18, 19 :  Send NOP command.
	SendDirectCommand(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif


	// Step 20 :  Send MR2 command.
	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR2.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR2.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR2.Reg);
#endif

#if (REG_MSG)
	DebugPutString("\r\n########## pReg_Drex->DIRECTCMD[MR2] \t:");
	DebugPutHex(pReg_Drex->DIRECTCMD);
#endif

	// Step 21 :  Send MR3 command.
	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR3.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR3.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR3.Reg);
#endif



	// Step 22 :  Send MR1 command.
	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR1.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif

#if (REG_MSG)
	DebugPutString("\r\n########## pReg_Drex->DIRECTCMD[MR1] \t:");
	DebugPutHex(pReg_Drex->DIRECTCMD);
#endif


	// Step 23 :  Send MR0 command.
	SendDirectCommand(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR0, MR0.Reg);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR0, MR0.Reg);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR0, MR0.Reg);
#endif
	//    DMC_Delay(100);
#if (REG_MSG)
	DebugPutString("\r\n########## pReg_Drex->DIRECTCMD[MR0] \t:");
	DebugPutHex(pReg_Drex->DIRECTCMD);
#endif


	// Step 24
	//        ClearIO32( &pReg_Drex->DIRECTCMD,           (0x1    <<   8));                   // 	DLL Reset[8]. 0:No, 1:Reset

	// Step 25 :  Send ZQ Init command
	SendDirectCommand(SDRAM_CMD_ZQINIT, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_ZQINIT, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_ZQINIT, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	DMC_Delay(100);


	WriteIO32( &pReg_Drex->MEMCONTROL,
			(0x0    <<  25) |           // [26:25] mrr_byte     : Mode Register Read Byte lane location
			(0x0    <<  24) |           // [   24] pzq_en       : DDR3 periodic ZQ(ZQCS) enable
			//        (0x0    <<  23) |           // [   23] reserved     : SBZ
			(0x3    <<  20) |           // [22:20] bl           : Memory Burst Length                       :: 3'h3  - 8
#if (CFG_NSIH_EN == 0)
			((_DDR_CS_NUM-1)        <<  16) |   // [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
#else
			((pSBI->DII.ChipNum-1)  <<  16) |   // [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
#endif
			(0x2    <<  12) |           // [15:12] mem_width    : Width of Memory Data Bus                  :: 4'h2  - 32bits
			(0x6    <<   8) |           // [11: 8] mem_type     : Type of Memory                            :: 4'h6  - ddr3
			(0x0    <<   6) |           // [ 7: 6] add_lat_pall : Additional Latency for PALL in cclk cycle :: 2'b00 - 0 cycle
			(0x0    <<   5) |           // [    5] dsref_en     : Dynamic Self Refresh                      :: 1'b0  - Disable
			(0x0    <<   4) |           // [    4] tp_en        : Timeout Precharge                         :: 1'b0  - Disable
			(0x0    <<   2) |           // [ 3: 2] dpwrdn_type  : Type of Dynamic Power Down                :: 2'b00 - Active/precharge power down
			(0x0    <<   1) |           // [    1] dpwrdn_en    : Dynamic Power Down                        :: 1'b0  - Disable
			(0x0    <<   0));           // [    0] clk_stop_en  : Dynamic Clock Control                     :: 1'b0  - Always running


#if 1   //(CONFIG_ODTOFF_GATELEVELINGON)
	//    MEMMSG("\r\n########## READ/GATE Level ##########\r\n");

#if (DDR_NEW_LEVELING_TRAINING == 0)
	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<   6) );          // ctrl_atgate=1
	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<  14) );          // p0_cmd_en=1
	SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    <<   6) );          // InitDeskewEn=1
	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<  13) );          // byte_rdlvl_en=1
#else
	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],        (0x1    <<   6) );          // ctrl_atgate=1
	SetIO32  ( &pReg_DDRPHY->PHY_CON[2],        (0x1    <<   6) );          // InitDeskewEn=1
#endif

	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[1]) & ~(0xF <<  16);            // rdlvl_pass_adj=4
	temp |= (0x4 <<  16);
	WriteIO32( &pReg_DDRPHY->PHY_CON[1],        temp);

	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[2]) & ~(0x7F << 16);            // rdlvl_incr_adj=1
	temp |= (0x1 <<  16);
	WriteIO32( &pReg_DDRPHY->PHY_CON[2],        temp);

	/* LOCK VALUE - by.deoks  */
	/*----------------------------------------------------------------------------------------*/
	do {
		SetIO32  ( &pReg_DDRPHY->PHY_CON[12],       (0x1    <<   5) );      // ctrl_dll_on[5]=1
		//showLockValue();	// by.deoks
		do {
			temp = ReadIO32( &pReg_DDRPHY->PHY_CON[13] );                   // read lock value
		} while( (temp & 0x7) != 0x7 );

		ClearIO32( &pReg_DDRPHY->PHY_CON[12],       (0x1    <<   5) );      // ctrl_dll_on[5]=0

		temp = ReadIO32( &pReg_DDRPHY->PHY_CON[13] );                       // read lock value
	} while( (temp & 0x7) != 0x7 );

	g_DDRLock = (temp >> 8) & 0x1FF;
	/*----------------------------------------------------------------------------------------*/
	lock_div4 = (g_DDRLock >> 2);

	temp  = ReadIO32( &pReg_DDRPHY->PHY_CON[12] );
	temp &= ~(0x7F <<  8);
	temp |= (lock_div4 <<  8);                                              // ctrl_force[14:8]
	WriteIO32( &pReg_DDRPHY->PHY_CON[12],   temp );

	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<  5) );               // ctrl_read_disable[5]= 1. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.

#if (DDR_NEW_LEVELING_TRAINING == 0)
	SetIO32  ( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  24) );              // rdlvl_gate_en=1

	SetIO32  ( &pReg_DDRPHY->PHY_CON[0],    (0x1    <<   8) );              // ctrl_shgate=1
	ClearIO32( &pReg_DDRPHY->PHY_CON[1],    (0xF    <<  20) );              // ctrl_gateduradj=0

	WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0x00000001 );                   // ctrl_rdlvl_data_en[1]=1, Gate Traning : Enable
	while( ( ReadIO32( &pReg_Drex->PHYSTATUS ) & 0x4000 ) != 0x4000 );      // Rdlvl_complete_ch0[14]=1

	//    WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0x00000000 );                   //- ctrl_rdlvl_data_en[1]=0, Gate Traning : Disable
	WriteIO32( &pReg_Drex->RDLVL_CONFIG,    0x00000001 );                   // LINARO

	WriteIO32( &pReg_DDRPHY->PHY_CON[14],   0x00000000 );                   // ctrl_pulld_dq[11:8]=0x0, ctrl_pulld_dqs[3:0]=0x0

	SetIO32  ( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<   6) );              // ctrl_start[6]=1
#else   // #if (DDR_NEW_LEVELING_TRAINING == 1)

	SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization
	ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3) );              // Force DLL Resyncronization

#if (CONFIG_SET_MEM_TRANING_FROM_NSIH == 0)
#if (DDR_WRITE_LEVELING_EN == 1)
	DDR_Write_Leveling();
#endif
#if (DDR_GATE_LEVELING_EN == 1)
	DDR_Gate_Leveling(isResume);
#endif
#if (DDR_READ_DQ_CALIB_EN == 1)
	DDR_Read_DQ_Calibration(isResume);
#endif
#if (DDR_WRITE_DQ_CALIB_EN == 1)
	DDR_Write_DQ_Calibration(isResume);
#endif
#else // #if (CONFIG_SET_MEM_TRANING_FROM_NSIH == 0)

//    if (pSBI->LvlTr_Mode & LVLTR_WR_LVL)
//        DDR_Write_Leveling();

#if (DDR_GATE_LEVELING_EN == 1)
	if (pSBI->LvlTr_Mode & LVLTR_GT_LVL)
		DDR_Gate_Leveling(isResume);
#endif

#if (DDR_READ_DQ_CALIB_EN == 1)
	if (pSBI->LvlTr_Mode & LVLTR_RD_CAL)
		DDR_Read_DQ_Calibration(isResume);
#endif

#if (DDR_WRITE_DQ_CALIB_EN == 1)
	if (pSBI->LvlTr_Mode & LVLTR_WR_CAL)
		DDR_Write_DQ_Calibration(isResume);
#endif
#endif  //#if (CONFIG_SET_MEM_TRANING_FROM_NSIH == 0)

	WriteIO32( &pReg_DDRPHY->PHY_CON[14],   0x00000000 );               // ctrl_pulld_dq[11:8]=0x0, ctrl_pulld_dqs[3:0]=0x0
	ClearIO32( &pReg_DDRPHY->PHY_CON[0],    (0x3    <<  13) );          // p0_cmd_en[14]=0, byte_rdlvl_en[13]=0
#endif  // #if (DDR_NEW_LEVELING_TRAINING == 1)

	SetIO32  ( &pReg_DDRPHY->PHY_CON[12],   (0x1    <<   5) );          // ctrl_dll_on[5]=1
	SetIO32  ( &pReg_DDRPHY->PHY_CON[2],    (0x1    <<  12));           // DLLDeskewEn[2]=1

#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)
	SetIO32  ( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24));           // ctrl_resync=1
	ClearIO32( &pReg_DDRPHY->PHY_CON[10],   (0x1    <<  24));           // ctrl_resync=0
#endif
#if defined(ARCH_NXP5430)
	ClearIO32( &pReg_DDRPHY->OFFSETD_CON,   (0x1    <<  28));           // upd_mode=0

	SetIO32  ( &pReg_DDRPHY->OFFSETD_CON,   (0x1    <<  24));           // ctrl_resync=1
	ClearIO32( &pReg_DDRPHY->OFFSETD_CON,   (0x1    <<  24));           // ctrl_resync=0
#endif

#endif // gate leveling

	SetIO32  ( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3));           // Force DLL Resyncronization
	ClearIO32( &pReg_Drex->PHYCONTROL[0],   (0x1    <<   3));           // Force DLL Resyncronization

	temp = (U32)(
			(0x0    <<  29) |           // [31:29] Reserved - SBZ.
			(0x17   <<  24) |           // [28:24] T_WrWrCmd.
			//        (0x0    <<  22) |           // [23:22] Reserved - SBZ.
			(0x0    <<  20) |           // [21:20] ctrl_upd_range.
#if (CFG_NSIH_EN == 0)
#if (tWTR == 3)     // 6 cycles
			(0x7    <<  17) |           // [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#elif (tWTR == 2)   // 4 cycles
			(0x6    <<  17) |           // [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#endif
#endif
			(0x0    <<  16) |           // [   16] ctrl_wrlvl_en[16]. Write Leveling Enable. 0:Disable, 1:Enable
			//        (0x0    <<  15) |           // [   15] Reserved SBZ.
			(0x0    <<  14) |           // [   14] p0_cmd_en. 0:Issue Phase1 Read command during Read Leveling. 1:Issue Phase0
			(0x0    <<  13) |           // [   13] byte_rdlvl_en. Read Leveling 0:Disable, 1:Enable
			(0x1    <<  11) |           // [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
			//        (0x0    <<  10) |           // [   10] Reserved - SBZ.
			(0x1    <<   9) |           // [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
#if (DDR_GATE_LEVELING_EN == 1)
			(0x1    <<   8) |           // [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
#endif
			(0x0    <<   7) |           // [    7] ctrl_ckdis. 0:Clock output Enable, 1:Disable
			(0x1    <<   6) |           // [    6] ctrl_atgate.
			//        (0x1    <<   5) |           // [    5] ctrl_read_disable. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
			(0x0    <<   4) |           // [    4] ctrl_cmosrcv.
			(0x0    <<   3) |           // [    3] ctrl_read_width.
			(0x0    <<   0));           // [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

#if (CFG_NSIH_EN == 1)
	if ((pSBI->DII.TIMINGDATA >> 28) == 3)      // 6 cycles
		temp |= (0x7    <<  17);
	else if ((pSBI->DII.TIMINGDATA >> 28) == 2) // 4 cycles
		temp |= (0x6    <<  17);
#endif

	WriteIO32( &pReg_DDRPHY->PHY_CON[0],    temp );


	/* Send PALL command */
	SendDirectCommand(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (CFG_NSIH_EN == 0)
#if (_DDR_CS_NUM > 1)
	SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
#else
	if(pSBI->DII.ChipNum > 1)
		SendDirectCommand(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	WriteIO32( &pReg_Drex->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1    <<  31) |           // [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1    <<  30) |           // [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0    <<   0) |           // [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x1    <<  29) |           // [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0    <<  24) |           // [28:24] ctrl_pd. Input Gate for Power Down.
			//        (0x0    <<   7) |           // [23: 7] reserved - SBZ
			(0x0    <<   4) |           // [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			(0x0    <<   3) |           // [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   1));           // [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1

	WriteIO32( &pReg_Drex->CONCONTROL,
			(0x0    <<  28) |           // [   28] dfi_init_start
			(0xFFF  <<  16) |           // [27:16] timeout_level0
			(0x1    <<  12) |           // [14:12] rd_fetch
			(0x1    <<   8) |           // [    8] empty
			(0x1    <<   5) |           // [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0    <<   3) |           // [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0    <<   1));           // [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved

#if (DDR_RW_CAL == 1)
	DDR3_RW_Delay_Calibration();
#endif

	printf("\r\n");
#if 0
	printf("Lock value  = %d\r\n",      g_DDRLock );

	printf("GATE CYC    = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[3] ) );
	printf("GATE CODE   = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[8] ) );
#endif
	printf("Read  DQ    = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[4] ) );
	printf("Write DQ    = 0x%08X\r\n",  ReadIO32( &pReg_DDRPHY->PHY_CON[6] ) );
}
