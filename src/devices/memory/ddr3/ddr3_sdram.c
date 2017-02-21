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

#include <drex.h>
#include <ddrphy.h>
#include <ddr3_ac_timing.h>
#include <ddr3_sdram.h>

#define DDR_NEW_LEVELING_TRAINING       (1)

#define DDR_RW_CAL			0

#if defined(CHIPID_NXP4330)
#define DDR_WRITE_LEVELING_EN           (0)
#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_LEVELING_CALIB_EN     (0)     // for Fly-by
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_READ_DQ_MARGIN_VIEW         (0)
#define DDR_WRITE_DQ_MARGIN_VIEW        (0)

#endif  // #if defined(CHIPID_NXP4330)

#if defined(CHIPID_S5P4418)
#define DDR_WRITE_LEVELING_EN           (0)
#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_LEVELING_CALIB_EN     (0)     // for Fly-by
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_READ_DQ_MARGIN_VIEW         (0)
#define DDR_WRITE_DQ_MARGIN_VIEW        (0)
#endif  // #if defined(CHIPID_S5P4418)

#define MEM_CALIBRATION_INFO 		(1)

#define CFG_ODT_ENB                     (1)

#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");

struct s5p4418_drex_sdram_reg *const g_drex_reg =
    (struct s5p4418_drex_sdram_reg * const)PHY_BASEADDR_DREX_MODULE_CH0_APB;
struct s5p4418_ddrphy_reg *const g_ddrphy_reg =
    (struct s5p4418_ddrphy_reg * const)PHY_BASEADDR_DREX_MODULE_CH1_APB;

unsigned int g_DDRLock;
unsigned int g_GateCycle;
unsigned int g_GateCode;
unsigned int g_RDvwmc;
unsigned int g_WRvwmc;

struct dram_device_info g_ddr3_info;

void DMC_Delay(int milisecond)
{
	register volatile int count;

	for (count = 0; count < milisecond; count++) {
		nop();
	}
}

inline void send_directcmd(SDRAM_CMD cmd, U8 chipnum, SDRAM_MODE_REG mrx,
		U16 value)
{
	mmio_write_32((unsigned int *)&g_drex_reg->DIRECTCMD,
			cmd << 24 | chipnum << 20 | mrx << 16 | value);
}

#if (CONFIG_SUSPEND_RESUME == 1)
void enter_self_refresh(void)
{
	union SDRAM_MR MR;
	unsigned int nTemp;
	unsigned int nChips = 0;

#if (DDR3_CS_NUM > 1)
	nChips = 0x3;
#else
	nChips = 0x1;
#endif

	while (mmio_read_32(&g_drex_reg->CHIPSTATUS) & 0xF) {
		nop();
	}

	/* Send PALL command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	DMC_Delay(100);

	/* ODT OFF */
	MR.Reg          = 0;
	MR.MR2.RTT_WR   = 0; 							// 0: disable, 1: RZQ/4 (60ohm), 2: RZQ/2 (120ohm)
//	MR.MR2.RTT_WR   = 2;							// 0: disable, 1: RZQ/4 (60ohm), 2: RZQ/2 (120ohm)
	MR.MR2.SRT      = 0;							// self refresh normal range, if (ASR == 1) SRT = 0;
	MR.MR2.ASR      = 1;							// auto self refresh enable
	MR.MR2.CWL      = (nCWL - 5);

	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif

	MR.Reg          = 0;
	MR.MR1.DLL      = 1;							// 0: Enable, 1 : Disable
	MR.MR1.AL       = MR1_nAL;
	MR.MR1.ODS1     = (CONFIG_DRAM_MR1_ODS >> 1) & 1;
	MR.MR1.ODS0     = (CONFIG_DRAM_MR1_ODS >> 0) & 1;
	MR.MR1.RTT_Nom2 = (CONFIG_DRAM_MR1_RTT_Nom >> 2) & 1;
	MR.MR1.RTT_Nom1 = (CONFIG_DRAM_MR1_RTT_Nom >> 1) & 1;
	MR.MR1.RTT_Nom0 = (CONFIG_DRAM_MR1_RTT_Nom >> 0) & 1;
	MR.MR1.QOff     = 0;
	MR.MR1.WL       = 0;
#if 0
	MR.MR1.TDQS     = (_DDR_BUS_WIDTH >> 3) & 1;
#endif

	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif

	/* Enter self-refresh command */
	send_directcmd(SDRAM_CMD_REFS, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_REFS, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if 1
	do {
		nTemp = ( mmio_read_32(&g_drex_reg->CHIPSTATUS) & nChips );
	} while( nTemp );

	do {
		nTemp = ( (mmio_read_32(&g_drex_reg->CHIPSTATUS) >> 8) & nChips );
	} while( nTemp != nChips );
#else

	// for self-refresh check routine.
	while( 1 ) {
		nTemp = mmio_read_32(&g_drex_reg->CHIPSTATUS);
		if (nTemp)
			MEMMSG("ChipStatus = 0x%04x\r\n", nTemp);
	}
#endif

	mmio_clear_32(&g_drex_reg->CONCONTROL,  (0x1 << 5));			// afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1
	mmio_set_32  (&g_drex_reg->MEMCONTROL,    (0x1 << 0));			// clk_stop_en[0] : Dynamic Clock Control   :: 1'b0  - Always running

	DMC_Delay(1000 * 3);
}

void exit_self_refresh(void)
{
	union SDRAM_MR MR;

	// Step 10    ACK, ACKB on
	mmio_clear_32(&g_drex_reg->MEMCONTROL,  (0x1 << 0));			// clk_stop_en[0] : Dynamic Clock Control   :: 1'b0  - Always running
	DMC_Delay(10);

	// Step 52 Auto refresh counter enable
	mmio_set_32(&g_drex_reg->CONCONTROL,    (0x1 << 5));			// afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1
	DMC_Delay(10);

	/* Send PALL command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	MR.Reg          = 0;
	MR.MR1.DLL      = 0;							// 0: Enable, 1 : Disable
	MR.MR1.AL       = MR1_nAL;
	MR.MR1.ODS1 	= (CONFIG_DRAM_MR1_ODS >> 1) & 1;
	MR.MR1.ODS0 	= (CONFIG_DRAM_MR1_ODS >> 0) & 1;
	MR.MR1.RTT_Nom2 = (CONFIG_DRAM_MR1_RTT_Nom >> 2) & 1;
	MR.MR1.RTT_Nom1 = (CONFIG_DRAM_MR1_RTT_Nom >> 1) & 1;
	MR.MR1.RTT_Nom0 = (CONFIG_DRAM_MR1_RTT_Nom >> 0) & 1;
	MR.MR1.QOff     = 0;
	MR.MR1.WL       = 0;
#if 0
	MR.MR1.TDQS     = (_DDR_BUS_WIDTH>>3) & 1;
#endif

	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif
	/* ODT On */
	MR.Reg          = 0;
	MR.MR2.RTT_WR   = CONFIG_DRAM_MR2_RTT_WR;
	MR.MR2.SRT      = 0;							// self refresh normal range
	MR.MR2.ASR      = 0;							// auto self refresh disable
	MR.MR2.CWL      = (nCWL - 5);

	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR.Reg);
#endif

	/* Exit self-refresh command */
	send_directcmd(SDRAM_CMD_REFSX, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_REFSX, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if 0
	while( mmio_read_32(&g_drex_reg->CHIPSTATUS) & (0xF << 8) ) {
		nop();
	}
#endif

	DMC_Delay(1000 * 2);
}
#endif // #if (CONFIG_SUSPEND_RESUME == 1)

#if (DDR_NEW_LEVELING_TRAINING == 1)
unsigned int GetVWMC_Offset(unsigned int Code, unsigned int Lock_Div4)
{
	U8 VWMC[4];
	int OffSet[4];
	unsigned int i, ret;

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

#if (DDR_GATE_LEVELING_EN == 1)

#if (MEM_CALIBRATION_INFO == 1)
void gate_leveling_information(void)
{
	unsigned int status, reg_value;
	unsigned int gate_cycle[4], gate_code[4];
#if 0
	unsigned int gate_vwmc[4];
#endif
	unsigned int max_slice = 4, slice;
	unsigned int LockValue = g_DDRLock;

	/* DQ Calibration Fail Status */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_FAIL_STATUS);
	status = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);

	if (status == 0) {
		/* Gate Center Cycle */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_CENTER_CYCLE);
		reg_value = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			gate_cycle[slice] = (reg_value >> (slice*8)) & 0xFF;

		/* Gate Code */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_CENTER_CODE);
		reg_value = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			gate_code[slice] = (reg_value >> (slice*8)) & 0xFF;
	#if 0
		/* Gate Vaile Window Margin Center */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_VWMC);
		reg_value = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			gate_vwmc[slice] = (reg_value >> (slice*8)) & 0xFF;
	#endif
	}

	MEMMSG("\r\n####### Gate Leveling - Information #######\r\n");
	MEMMSG("Gate Leveling %s!! \r\n", (status == 0) ? "Success" : "Failed");
	if (status == 0) {
		MEMMSG("Gate Cycle : %d/%d/%d/%d \r\n", gate_cycle[0], gate_cycle[1],
				gate_cycle[2], gate_cycle[3]);
		MEMMSG("Gate Code  : %d/%d/%d/%d \r\n", gate_code[0], gate_code[1],
				gate_code[2], gate_code[3]);
		MEMMSG("Gate Delay %d, %d, %d, %d\r\n",
				(gate_cycle[0])*LockValue + gate_code[0],
				(gate_cycle[1])*LockValue + gate_code[1],
				(gate_cycle[2])*LockValue + gate_code[2],
				(gate_cycle[3])*LockValue + gate_code[3]);
		MEMMSG("###########################################\r\n");
	}
}
#endif

/*************************************************************
 * Must be S5P4418
 * Gate Leveling sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. Send ALL Precharge command.
 * Step 02. Set the Memory in MPR Mode (MR3:A2=1)
 * Step 03. Set the Gate Leveling Mode.
 *	    -> Enable "bylvl_gate_en(=gate_cal_mode)" in PHY_CON2[24]
 *	    -> Enable "ctrl_shgate" in PHY_CON0[8]
 *	    -> Set "ctrl_gateduradj[3:0] (=PHY_CON1[23:20]) (DDR3: 4'b0000")
 * Step 04. Assert "dfi_rdlvl_en" and "dfi_rdlvl_gate_en" after "dfi_rdlvl_resp" is disabled.
 * 	    -> Set the "ctrl_rdlvl_gate_en[0] = 1"
 * Step 05. Waiting for Response.
 *	    -> Wait until "rd_wr_cal_resp"(=PHYT_CON3[26])
 * Step 06. Deassert "dfi_rdlvel_en", "dfi_rdlvl_gate_en" after "dfi_rdlvl_resp" is disabled.
 *	    -> Set the "ctrl_rdlvl_gate_en = 0 (=RDLVL_CONFIG[0])"
 * Step 07. Disable DQS Pull Down Mode.
 *	     -> Set the "ctrl_pulld_dqs[8:0] = 0"
 * Step 08. Disable the Memory in MPR Mode (MR3:A2=0)
 *************************************************************/
int ddr_gate_leveling(void)
{
	union SDRAM_MR MR;

	volatile int cal_count = 0;
#if 0
	volatile int status;
#endif
	volatile int response;
	int temp, ret = 0;

	MEMMSG("\r\n########## Gate Leveling - Start ##########\r\n");

	/* Step 01. Send ALL Precharge command. */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* Step 02. Set the Memory in MPR Mode (MR3:A2=1) */
	MR.Reg          = 0;
	MR.MR3.MPR      = 1;
	MR.MR3.MPR_RF   = 0;
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);

	/* Step 03. Set the Gate Leveling Mode. */
	/* Step 03-1. Enable "bylvl_gate_en(=gate_cal_mode)" in PHY_CON2[24] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2],      (0x1   <<  24));		// rdlvl_gate_en[24] = 1
	/* Step 03-2. Enable "ctrl_shgate" in PHY_CON0[8] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0],      (0x5   <<   6));		// ctrl_shgate[8] = 1, ctrl_atgate[6] = 1
	/* Step 03-3. Set "ctrl_gateduradj[3:0] (=PHY_CON1[23:20]) (DDR3: 4'b0000") */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[1],    (0xF   <<  20));		// ctrl_gateduradj[23:20] = DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9

	/* Step 04. Assert "dfi_rdlvl_en" and "dfi_rdlvl_gate_en" after "dfi_rdlvl_resp" is disabled. */
	/* Step 04-01. Set the "ctrl_rdlvl_gate_en[0] = 1" */
	temp  = mmio_read_32(&g_drex_reg->RDLVL_CONFIG) & ~(0x1 <<0);
	temp |= (0x1 << 0);
	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, temp);				// ctrl_rdlvl_gate_en[0] = 1
//	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));		// ctrl_rdlvl_data_en[0] = 0

	/* Step 05. Wait until "rd_wr_cal_resp" (=PHYSTATUS[14]) */
	for (cal_count = 0; cal_count < 100; cal_count++) {
		response = mmio_read_32(&g_drex_reg->PHYSTATUS);
		if (response & (0x1 << 14))
			break;
		DMC_Delay(100);
	}

	/* Step XX-0. check to success or failed (timeout) */
	if (cal_count >= 100) {
		MEMMSG("GATE: Calibration Responese Checking : fail...!!!\r\n");
		ret = -1;							// Failure Case
		goto gate_err_ret;
	}
	DMC_Delay(0x100);

gate_err_ret:
	/* Step 06. Deassert "dfi_rdlvel_en", "dfi_rdlvl_gate_en" after "dfi_rdlvl_resp" is disabled. */
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 0
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));			// ctrl_rdlvl_data_en[0] = 0

#if (MEM_CALIBRATION_INFO == 1)
	gate_leveling_information();
#endif

#if 0
	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_CENTER_CYCLE);
	g_GateCycle = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_CENTER_CODE);
	g_GateCode = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);


#if (DDR_RESET_GATE_LVL == 1)
	int offset, c_offset;
	int cycle[4], gate_cycle;

	for (cal_count= 0; cal_count < 4; cal_count++)
		cycle[cal_count] = ((g_GateCycle >> (8 * cal_count)) & 0xFF);

	offset = GetVWMC_Offset(g_GateCode, (g_DDRLock >> 2));
#if (DDR_GATE_LVL_COMPENSATION_EN == 1)
	c_offset = GetVWMC_Compensation(offset);
#else
	c_offset = offset;
#endif // #if (DDR_GATE_LVL_COMPENSATION_EN == 1)
	mmio_write_32(&g_ddrphy_reg->PHY_CON[8], c_offset);		// ctrl_offsetc

	gate_cycle = (((U8)cycle[3] << 15) | ((U8)cycle[2] << 10) |
			((U8)cycle[1] << 5) | (U8)cycle[0]);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[3], gate_cycle);	// ctrl_shiftc
#endif // #if (DDR_RESET_GATE_LVL == 1)
#endif

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], 0x0);				// ReadModeCon[7:0] = 0x0

	/* Step 07. Disable DQS Pull Down Mode. */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[14], (0xF << 0));			// ctrl_pulld_dqs[3:0]

	/* Step 08. Disable the Memory in MPR Mode (MR3:A2=0) */
	MR.Reg          = 0;
	MR.MR3.MPR      = 0;
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);

	MEMMSG("\r\n########## Gate Leveling - End ##########\r\n");

	return ret;
}
#endif // #if (DDR_GATE_LEVELING_EN == 1)

#if (DDR_READ_DQ_CALIB_EN == 1)

#if (MEM_CALIBRATION_INFO == 1)
void read_dq_calibration_information(void)
{
	unsigned int DQ_FailStatus, DQ_Calibration;
	unsigned int VWML[4], VWMR[4];
	unsigned int VWMC[4], Deskew[4];
	//	unsigned int RDCenter[8];
	unsigned int max_slice = 4, slice;
#if 0	// Each DQ Line
	unsigned int MaxLane = 4, max_slice = 8, Lane, slice;
#endif
	/* DQ Calibration Fail Status */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_FAIL_STATUS);
	DQ_FailStatus = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);

	if (DQ_FailStatus == 0) {
		/* Vaile Window Margin Left */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_LEFT);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);
		for(slice = 0; slice < max_slice; slice++)
			VWML[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;

		/* Vaile Window Margin Right */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_RIGHT);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);
		for(slice = 0; slice < max_slice; slice++)
			VWMR[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;

		/* Vaile Window Margin Center */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], RD_VWMC);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);
		for(slice = 0; slice < max_slice; slice++)
			VWMC[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;

		/* Vaile Window Margin Deskew */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], RD_DESKEW_CODE);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);
		for(slice = 0; slice < max_slice; slice++)
			Deskew[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;
	}

#if 0	// Each DQ Line
	for(Lane = 0; Lane < MaxLane; Lane) {
		unsigned int Code = 0x1;
		for(slice = 0; slice < max_slice; slice++, Code+=0x10) {
			mmio_write_32(&g_ddrphy_reg->PHY_CON[5], Code);
			RDCenter[slice] = mmio_read_32(g_ddrphy_reg->PHY_CON[18+1]);
		}
	}
#endif
	printf("\r\n#### Read DQ Calibration - Information ####\r\n");

	printf("Read DQ Calibration %s!! \r\n",
			(DQ_FailStatus == 0) ? "Success" : "Failed" );

	if (DQ_FailStatus == 0) {
	#if 0	// Display Type
		for(slice = 0; slice < max_slice; slice++)
			printf("VWML0: %d, VWMC0: %d, VWML0: %d, Deskew0: %d \r\n",
					VWML[slice], VWMC[slice], VWMR[slice], Deskew[slice]);
	#else
		unsigned int Range;
		for(slice = 0; slice < max_slice; slice++) {
			Range = VWMR[slice] - VWML[slice];
			printf("SLICE%d: %d ~ %d ~ %d (Range: %d)(Deskew: %d) \r\n",
					slice, VWML[slice], VWMC[slice], VWMR[slice],
					Range, Deskew[slice]);
		}

	#endif

	#if 0	// Each Byte
		for(Lane = 0; Lane < MaxLane; Lane) {
			printf("Lane Number : %d \r\n", Lane );
			for(slice = 0; slice < max_slice; slice++)
				MEMMSG("DQ%d : %d \r\n", slice, RDCenter[slice]);
		}
	#endif
	}
	printf("\r\n###########################################\r\n");

}
#endif

/*************************************************************
 * Must be S5P4418
 * Read DQ Calibration sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. Send Precharge ALL Command
 * Step 02. Set the Memory in MPR Mode (MR3:A2=1)
 * Step 03. Set Read Leveling Mode.
 * 	     -> Enable "rd_cal_mode" in PHY_CON2[25]
 * Step 04. Start the Read DQ Calibration
 *	     -> Memory Controller should assert "dfi_rdlvl_en" to do read leveling.
 *		-> Set the "ctrl_rdlvl_data_en[1]=1" (Refer Drex 2.0.0)
 * Step 05. Wait for Response.
 *	     -> Wait until "rd_wr_cal_resp"(=PHY_CON3[26]) is set.
 * Step 06. End the Read DQ Calibration
 *	     -> Set "rd_cal_start=0"(=PHY_CON3[19]) after
 	          "rd_wr_cal_resp"(=PHY_CON3[26]) is enabled.
 * Step 07. Disable the Memory in MPR Mode (MR3:A2=0)
 *************************************************************/
int ddr_read_dq_calibration(void)
{
	union SDRAM_MR MR;

	volatile int cal_count = 0;
	volatile int status, response;
	int ret = 0;

	MEMMSG("\r\n########## Read DQ Calibration - Start ##########\r\n");

	/* Step 01. Send Precharge ALL Command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* Step 02. Set the Memory in MPR Mode (MR3:A2=1) */
	MR.Reg          = 0;
	MR.MR3.MPR      = 1;
	MR.MR3.MPR_RF   = 0;
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);

	/* Step 03. Enable "rd_cal_mode" in PHY_CON2[25] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 25));			// rdlvl_en[25] = 1 (=rd_cal_mode)

	/* Step 04. Memory Controller should assert "dfi_rdlvl_en" to do read leveling. (??)*/
//	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 1
	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));			// ctrl_rdlvl_data_en[1]=1

	/* Step 05. Wait until "rd_wr_cal_resp" (=PHYSTATUS[14]) */
	for (cal_count = 0; cal_count < 100; cal_count++) {
		response = mmio_read_32(&g_drex_reg->PHYSTATUS);
		if (response & (0x1 << 14))
			break;
		DMC_Delay(100);
	}

	/* Step XX-0. check to success or failed (timeout) */
	if (cal_count >= 100) {
		MEMMSG("Read: Calibration Responese Checking : fail...!!!\r\n");
		ret = -1;							// Failure Case
		goto rd_err_ret;
	}

	/* Step XX-1. check to success or failed (status) */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_FAIL_STATUS);
	for (cal_count = 0; cal_count < 0x1000; cal_count++) {
		status = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);
		if (status == 0)
			break;
		DMC_Delay(0x100);
	}

	/* Step XX-0. check to success or failed (timeout) */
	if (cal_count >= 100) {
		MEMMSG("Read DQ Calibration Status: 0x%08X \r\n", status);
		ret = -1;							// Failure Case
	}

rd_err_ret:
	/* Step 06. End of Read DQ Calibration */
	/* Step 06-1. memory controller should deassert "dfi_rdlvl_en" and "dfi_rdlvl_gate_en" after */
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 0
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));			// ctrl_rdlvl_data_en[1] = 0

#if (MEM_CALIBRATION_INFO == 1)
	read_dq_calibration_information();
#endif

#if 0
#if (DDR_RESET_READ_DQ == 1)
	unsigned int offset, c_offset;;
	offset = GetVWMC_Offset(g_RDvwmc, (g_DDRLock >> 2));
#if (DDR_READ_DQ_COMPENSATION_EN == 1)
	c_offset = GetVWMC_Compensation(offset);
#else
	c_offset = offset;
#endif // #if (DDR_READ_DQ_COMPENSATION_EN == 1)

	g_RDvwmc = c_offset;

	// Read DQ offset Apply.
	mmio_write_32(&g_ddrphy_reg->PHY_CON[4], c_offset);

	//*** Resync Update READ SDLL Code (ctrl_offsetr) : Make "ctrl_resync" HIGH and LOW
	mmio_set_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0x1 (HIGH)
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0x0 (LOW)
#endif	// #if (DDR_RESET_READ_DQ == 1)


	/*manual*/
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 25));			// rdlvl_en[25] = 0 (=rd_cal_mode)
#endif

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], 0x0);				// ReadModeCon[7:0] = 0x0

	/* Step 07. Disable the Memory in MPR Mode (MR3:A2=0) */
	MR.Reg          = 0;
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR.Reg);

	MEMMSG("\r\n########## Read DQ Calibration - End ##########\r\n");

	return ret;
}
#endif // #if (DDR_READ_DQ_CALIB_EN == 1)


#if (DDR_WRITE_DQ_CALIB_EN == 1)

#if (MEM_CALIBRATION_INFO == 1)
void write_dq_calibration_information(void)
{
	unsigned int DQ_FailStatus, DQ_Calibration;
	unsigned int VWML[4], VWMR[4];
	unsigned int VWMC[4], Deskew[4];
	unsigned int max_slice = 4, slice;
#if 0	// Each DQ Line
	unsigned int MaxLane = 4, max_slice = 8, Lane, slice;
#endif
	/* DQ Calibration Fail Status */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_FAIL_STATUS);
	DQ_FailStatus = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);

	if (DQ_FailStatus == 0) {
		/* Vaile Window Margin Left */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_LEFT);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			VWML[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;

		/* Vaile Window Margin Right */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_RIGHT);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			VWMR[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;

		/* Vaile Window Margin Center */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], WR_VWMC);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			VWMC[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;

		/* Vaile Window Margin Deskew */
		mmio_write_32(&g_ddrphy_reg->PHY_CON[5], WR_DESKEW_CODE);
		DQ_Calibration = mmio_read_32(&g_ddrphy_reg->PHY_CON[19+1]);
		for(slice = 0; slice < max_slice; slice++)
			Deskew[slice] = (DQ_Calibration >> (slice*8)) & 0xFF;
	}

	printf("\r\n### Write DQ Calibration - Information ####\r\n");

	printf("Write DQ Calibration %s!! \r\n",
			(DQ_FailStatus == 0) ? "Success" : "Failed" );

	if (DQ_FailStatus == 0) {
	#if 0	// Display Type
		for(slice = 0; slice < max_slice; slice++)
			printf("VWML0: %d, VWMC0: %d, VWML0: %d, Deskew0: %d \r\n",
					VWML[slice], VWMC[slice], VWMR[slice], Deskew[slice]);
	#else
		unsigned int Range;
		for(slice = 0; slice < max_slice; slice++) {
			Range = VWMR[slice] - VWML[slice];
			printf("SLICE%d: %d ~ %d ~ %d (Range: %d)(Deskew: %d) \r\n",
					slice, VWML[slice], VWMC[slice], VWMR[slice],
					Range, Deskew[slice]);
		#if 0
			if ((VWMC[slice] == 0) || (VWML[slice] == 0)
				|| (VWMR[slice] == 0)) {
				printf("Write Mem Cal Fuck!! \r\n");
				while(1);
			}
		#endif
		}
	#endif
	}
	printf("\r\n###########################################\r\n");
}
#endif

/*************************************************************
 * Must be S5P4418
 * Write DQ Calibration sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. Set Write Latency(=ctrl_wrlat) before Write Latency Calibration.
 * Step 02. Set issue Active command.
 * Step 03. Set the colum address
 * Step 04. Write DQ Calibration (Unit:Byte/Bit), (Pattern)
 *	     - Set "PHY_CON1[15:0]=0x0100" and "byte_rdlvl_en=1(=PHY_CON0[13]).
 *	     - Set "PHY_CON1[15:0]=0xFF00" and "byte_rdlvl_en=0(=PHY_CON0[13]).
 *                for Deskewing.
 * Step 05. Set Write Training Mode.
 *	     -> Set "wr_cal_mode=1"(=PHY_CON2[26]).
 * Step 06. Write DQ Calibration Start
 *	     -> Set "wr_cal_start=1" in PHY_CON2[27] to do Write DQ Calibration
 * Step 07. Waiting for Response
 *	     -> Wait until "rd_wr_cal_resp(=PHY_CON3[26]" is set.
 * Step 08. End the Write DQ Calibration
 *	     -> Set "wr_cal_start=0" in PHY_CON2[27] to do Write DQ Calibration
 	         after wrwr_cal_resp(=PHY_CON3[26]" is set.
 * Step 09. Check to Success or Failed. (timeout & status)
 *************************************************************/
int ddr_write_dq_calibration(void)
{
	volatile int bank = 0, row = 0, column = 0;
	volatile int cal_count = 0;
	volatile int status, response;
	int ret = 0;


	MEMMSG("\r\n########## Write DQ Calibration - Start ##########\r\n");

#if 1
	/* Step XX. Send All Precharge Command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#endif

	/* Step 01. Set Write Latency(=ctrl_wrlat) before Write Latency Calibration.*/
#if 0
	unsigned int DDR_AL, DDR_WL, DDR_RL;

	DDR_AL = 0;
	if (MR1_nAL > 0)
		DDR_AL = nCL - MR1_nAL;

	DDR_WL = (DDR_AL + nCWL);
	DDR_RL = (DDR_AL + nCL);
	mmio_set_32(&g_ddrphy_reg->PHY_CON[26 + 1], (DDR_WL << 16));		// T_wrdata_en, In DDR3
#endif
	/* Step 02. Set issue Active command. */
	mmio_write_32(&g_drex_reg->WRTRA_CONFIG,
			(row  << 16) |						// [31:16] row_addr
			(bank <<  1) |						// [ 3: 1] bank_addr
			(0x1  <<  0));						// [    0] write_training_en = 1
//	mmio_clear_32(&g_drex_reg->WRTRA_CONFIG, (0x1 << 0));		// [   0] write_training_en = 0

	/* Step 03. Set the colum address */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[24 + 1],
			(0x0    << 16) |					// [31:16] ddr3_default
			(column <<  1) |					// [15: 1] ddr3_address
			(0x0    <<  0));					// [    0] ca_swap_mode

#if 0
	/* Step 04-0. Set "PHY_CON1[15:0]=0x0100" and "byte_rdlvl_en=1(=PHY_CON0[13]). */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[1], 0x0100);
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0], (1 << 13));
#else
	/* Step 04-1. Set "PHY_CON1[15:0]=0xFF00" and "byte_rdlvl_en=0(=PHY_CON0[13]). */
	mmio_set_32 (&g_ddrphy_reg->PHY_CON[1], 0xFF00);
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[0], (1 << 13));
#endif
	/* Step 04-2. Enable "p0_cmd_en" in PHY_CON0[14] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0], (0x1 << 14));			// p0_cmd_en[14] = 1

	/* Step 05. Set the Write Training Mode */
	/* Step 05-0. Set the "wr_cal_mode(=wr_cal_mode)" in PHY_CON2[26] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 26));			// wr_deskew_con[26] = 1

	/* Step 06. Start the Write DQ Calibration */
	/* Step 06-0. Set the "wr_cal_start(=wr_deskew_en)" in PHY_CON2[27] to do Write DQ Calibration */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 27));			// wr_deskew_en[27] = 1

	/* Step 07. Wait until "rd_wr_cal_resp" (=PHYSTATUS[14]) */
	for (cal_count = 0; cal_count < 100; cal_count++) {
		response = mmio_read_32(&g_drex_reg->PHYSTATUS);
		if (response & (0x1 << 14))
			break;
		DMC_Delay(100);
	}

	/* Step XX-0. check to success or failed (timeout) */
	if (cal_count >= 100) {
		MEMMSG("Write: Calibration Responese Checking : fail...!!!\r\n");
		ret = -1;							// Failure Case
		goto wr_err_ret;
	}

	/* Step XX-1. check to success or failed (status) */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], VWM_FAIL_STATUS);
	status = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);
	if (status != 0) {
		MEMMSG("Write DQ Calibration Status: 0x%08X \r\n", status);
		ret = -1;
		goto wr_err_ret;
	}
#if (MEM_CALIBRATION_INFO == 1)
	write_dq_calibration_information();
#endif

wr_err_ret:
	mmio_clear_32(&g_drex_reg->WRTRA_CONFIG, (0x1 << 0));			// [   0] write_training_en = 0

	/* Step 08. Clear the "wr_deskew_en" in PHY_CON2[27] */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 27) );			// wr_deskew_en[27] = 0

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], 0x0);				// ReadModeCon[7:0] = 0x0

	MEMMSG("\r\n########## Write DQ Calibration - End ##########\r\n");

	return ret;
}
#endif  // #if (DDR_WRITE_DQ_CALIB_EN == 1)

#endif  // #if (DDR_NEW_LEVELING_TRAINING == 1)

static int resetgen_sequence(void)
{
	int retry = 0x10;

	/* Step 01. Reset (DPHY, DREX, DRAM)  (Min: 10ns, Typ: 200us) */
	do {
		mmio_clear_32(&pReg_RstCon->REGRST[0], (0x7 << 26));		//Reset Pin - High
		DMC_Delay(0x1000); 						// wait 300ms
		mmio_set_32(&pReg_RstCon->REGRST[0], (0x7 << 26));		//Reset Pin - Low
		DMC_Delay(0x1000); 						// wait 300ms
		mmio_clear_32(&pReg_RstCon->REGRST[0], (0x7 << 26));		//Reset Pin - High
		DMC_Delay(0x1000); 						// wait 300ms
		mmio_set_32(&pReg_RstCon->REGRST[0], (0x7 << 26));		//Reset Pin - Low
	//	DMC_Delay(0x10000);						// wait 300ms

#if 0
		mmio_clear_32( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
		DMC_Delay(0x1000);                                          	// wait 300ms
		mmio_set_32  ( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
		DMC_Delay(0x1000);                                          	// wait 300ms
		mmio_clear_32( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
		DMC_Delay(0x1000);                                          	// wait 300ms
		mmio_set_32  ( &pReg_Tieoff->TIEOFFREG[3],  (0x1    <<  31) );
#endif
		DMC_Delay(0x10000); // wait 300ms
		/* Step 01-1. Check the Reset State (Phy Version) */
	} while ((mmio_read_32(&g_ddrphy_reg->PHY_CON[29 + 1]) != 0x5020132) && (retry--));

	MEMMSG("PHY Version: 0x%08X\r\n", mmio_read_32(&g_ddrphy_reg->PHY_CON[29 + 1]));

	if (retry <= 0)
		return -1;

	return 0;
}

int ddr3_initialize(unsigned int is_resume)
{
	union SDRAM_MR MR0, MR1, MR2, MR3;
	unsigned int DDR_WL, DDR_RL;
	unsigned int DDR_AL1, DDR_AL2;
	unsigned int DDR3_LvlTr;
	unsigned int temp;

	is_resume = is_resume;

	MR0.Reg = 0;
	MR1.Reg = 0;
	MR2.Reg = 0;
	MR3.Reg = 0;

	MEMMSG("\r\nDDR3 POR Init Start\r\n");

	/* Step 01. Reset (DPHY, DREX, DRAM)  (Min: 10ns, Typ: 200us) */
	if (resetgen_sequence() < 0) {
		MEMMSG("(DPHY, DREX) Controller Reset Failed! \r\n");
		return -1;
	}

	DDR3_LvlTr = CONFIG_DDR3_LVLTR_EN;

	DDR_AL1 = 0;
	DDR_AL2 = 0;
	if (MR1_nAL > 0) {
		DDR_AL1 = nCL - MR1_nAL;
		DDR_AL2 = nCWL - MR1_nAL;
	}

	DDR_WL = (DDR_AL2 + nCWL);
	DDR_RL = (DDR_AL1 + nCL);

	MR2.Reg          = 0;
	MR2.MR2.RTT_WR   = CONFIG_DRAM_MR2_RTT_WR;
	MR2.MR2.SRT      = 0;							// self refresh normal range
	MR2.MR2.ASR      = 0;							// auto self refresh disable
	MR2.MR2.CWL      = (nCWL - 5);

	MR3.Reg          = 0;
	MR3.MR3.MPR      = 0;
	MR3.MR3.MPR_RF   = 0;

	MR1.Reg          = 0;
	MR1.MR1.DLL      = 0;							// 0: Enable, 1 : Disable

	MR1.MR1.AL       = MR1_nAL;
	MR1.MR1.ODS1 	 = (CONFIG_DRAM_MR1_ODS >> 1) & 1;
	MR1.MR1.ODS0 	 = (CONFIG_DRAM_MR1_ODS >> 0) & 1;
	MR1.MR1.RTT_Nom2 = (CONFIG_DRAM_MR1_RTT_Nom >> 2) & 1;
	MR1.MR1.RTT_Nom1 = (CONFIG_DRAM_MR1_RTT_Nom >> 1) & 1;
	MR1.MR1.RTT_Nom0 = (CONFIG_DRAM_MR1_RTT_Nom >> 0) & 1;
	MR1.MR1.QOff     = 0;
	MR1.MR1.WL       = 0;
#if 0
	MR1.MR1.TDQS    = (_DDR_BUS_WIDTH>>3) & 1;
#endif
	if (nCL > 11)
		temp = ((nCL - 12) << 1) + 1;
	else
		temp = ((nCL - 4) << 1);

	MR0.Reg         = 0;
	MR0.MR0.BL      = 0;
	MR0.MR0.BT      = 1;
	MR0.MR0.CL0     = (temp & 0x1);
	MR0.MR0.CL1     = ((temp>>1) & 0x7);
	MR0.MR0.DLL     = 0;//1;
	MR0.MR0.WR      = MR0_nWR;
	MR0.MR0.PD      = 0;//1;

	/* Step 2. Select Memory type : DDR3 */
		temp = ((0x0    <<  29) |					// [31:29] Reserved - SBZ.
			(0x17   <<  24) |					// [28:24] T_WrWrCmd.
//			(0x0    <<  22) |					// [23:22] Reserved - SBZ.
			(0x0    <<  20) |					// [21:20] ctrl_upd_range.
#if (tWTR == 3)     								// 6 cycles
			(0x7    <<  17) |					// [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#elif (tWTR == 2)								// 4 cycles
			(0x6    <<  17) |					// [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#endif
			(0x0    <<  16) |					// [   16] ctrl_wrlvl_en[16]. Write Leveling Enable. 0:Disable, 1:Enable
//			(0x0    <<  15) |					// [   15] Reserved SBZ.
			(0x0    <<  14) |					// [   14] p0_cmd_en. 0:Issue Phase1 Read command during Read Leveling. 1:Issue Phase0
			(0x0    <<  13) |					// [   13] byte_rdlvl_en. Read Leveling 0:Disable, 1:Enable
			(0x1    <<  11) |					// [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
//			(0x0    <<  10) |					// [   10] Reserved - SBZ.
			(0x1    <<   9) |					// [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
//			(0x1    <<   8) |						// [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
			(0x0    <<   7) |					// [    7] ctrl_ckdis. 0:Clock output Enable, 1:Disable
//			(0x1    <<   6) |						// [    6] ctrl_atgate.
//			(0x1    <<   5) |						// [    5] ctrl_read_disable. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.
			(0x0    <<   4) |					// [    4] ctrl_cmosrcv.
			(0x0    <<   3) |					// [    3] ctrl_read_width.
			(0x0    <<   0));					// [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

	mmio_write_32(&g_ddrphy_reg->PHY_CON[0], temp);

	temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[25 + 1]) & ~(0x3FFF << 16);
	temp |= (0x105E << 16);						// ddr3_cmd= DDR3:0x105E, lpddr2_cmd = LPDDDR2 or LPDDDR3:0x000E
//	temp |= (0x107F <<  0);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[25 + 1], temp);

#if 1
	temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[26 + 1]) & ~(0x3FFF << 0);
	temp |= (0x107F << 16);						// cmd_default= DDR3:0x107F, LPDDDR2 or LPDDDR3:0x000F
	mmio_write_32(&g_ddrphy_reg->PHY_CON[26 + 1], temp);
#endif

	MEMMSG("[DDR] Phy Initialize\r\n");

	/* Step 03. Set Write Latency(WL), Read Latency(RL), Burts Length(BL) */
	/* Step 03-01. Set the RL(Read Latency), , BL(Burst Length) */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[42 + 1],
			(0x8    <<   8) |					// Burst Length(BL)
			(DDR_RL <<   0));					// Read Latency(RL) - 800MHz:0xB, 533MHz:0x5

#if 0
	/* Step 03-02. Set the WL(Write Latency)  */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[26 + 1], (DDR_WL << 16));		// T_wrdata_en, In DDR3
#else
	/* Step 03-02. Set the WL(Write Latency)  */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[26 + 1],
			(DDR_WL <<  16) |					// T_wrdata_en, In DDR3
			(0x1    <<  12) |					// [  :12] RESET
			(0x0    <<   9) |					// [11: 9] BANK
			(0x0    <<   7) |					// [ 8: 7] ODT
			(0x1    <<   6) |					// [  : 6] RAS
			(0x1    <<   5) |					// [  : 5] CAS
			(0x1    <<   4) |					// [  : 4] WEN
#if (DDR3_CS_NUM > 1)
			(0x3    <<   2) |					// [ 3: 2] CKE[1:0]
			(0x3    <<   0));					// [ 1: 0] CS[1:0]
#else
			(0x1    <<   2) |					// [ 3: 2] CKE[1:0]
			(0x1    <<   0));					// [ 1: 0] CS[1:0]
#endif

#endif

	/* Step 04. ZQ Calibration */
#if 0
	mmio_write_32(&g_ddrphy_reg->PHY_CON[39 + 1],				// 100: 48ohm, 101: 40ohm, 110: 34ohm, 111: 30ohm
			(CONFIG_DPHY_DRVDS_BYTE3 <<  25) |			// [27:25] Data slice 3
			(CONFIG_DPHY_DRVDS_BYTE2 <<  22) |			// [24:22] Data slice 2
			(CONFIG_DPHY_DRVDS_BYTE1 <<  19) |			// [21:19] Data slice 1
			(CONFIG_DPHY_DRVDS_BYTE0 <<  16) |			// [18:16] Data slice 0
			(CONFIG_DPHY_DRVDS_CK    <<   9) |			// [11: 9] CK
			(CONFIG_DPHY_DRVDS_CKE   <<   6) |			// [ 8: 6] CKE
			(CONFIG_DPHY_DRVDS_CS    <<   3) |			// [ 5: 3] CS
			(CONFIG_DPHY_DRVDS_CA    <<   0));			// [ 2: 0] CA[9:0], RAS, CAS, WEN, ODT[1:0], RESET, BANK[2:0]
#else
	mmio_write_32(&g_ddrphy_reg->PHY_CON[39 + 1], 0x00);
#endif

	// Driver Strength(zq_mode_dds), zq_clk_div_en[18]=Enable
	mmio_write_32(&g_ddrphy_reg->PHY_CON[16],
			(0x1    <<  27) |					// [   27] zq_clk_en. ZQ I/O clock enable.
			(CONFIG_DPHY_ZQ_DDS <<  24) |				// [26:24] zq_mode_dds, Driver strength selection. 100 : 48ohm, 101 : 40ohm, 110 : 34ohm, 111 : 30ohm
			(CONFIG_DPHY_ZQ_ODT <<  21) |				// [23:21] ODT resistor value. 001 : 120ohm, 010 : 60ohm, 011 : 40ohm, 100 : 30ohm
			(0x0    <<  20) |					// [   20] zq_rgddr3. GDDR3 mode. 0:Enable, 1:Disable
			(0x0    <<  19) |					// [   19] zq_mode_noterm. Termination. 0:Enable, 1:Disable
			(0x1    <<  18) |					// [   18] zq_clk_div_en. Clock Dividing Enable : 0, Disable : 1
			(0x0    <<  15) |					// [17:15] zq_force-impn
			(0x0    <<  12) |					// [14:12] zq_force-impp
			(0x30   <<   4) |					// [11: 4] zq_udt_dly
			(0x1    <<   2) |					// [ 3: 2] zq_manual_mode. 0:Force Calibration, 1:Long cali, 2:Short cali
			(0x0    <<   1) |					// [    1] zq_manual_str. Manual Calibration Stop : 0, Start : 1
			(0x0    <<   0));					// [    0] zq_auto_en. Auto Calibration enable

	mmio_set_32(&g_ddrphy_reg->PHY_CON[16], (0x1 <<  1));			// zq_manual_str[1]. Manual Calibration Start=1
	while((mmio_read_32( &g_ddrphy_reg->PHY_CON[17 + 1] ) & 0x1) == 0);	//- PHY0: wait for zq_done
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[16], (0x1 <<  1));		// zq_manual_str[1]. Manual Calibration Stop : 0, Start : 1

	mmio_clear_32(&g_ddrphy_reg->PHY_CON[16], (0x1 << 18));		// zq_clk_div_en[18]. Clock Dividing Enable : 1, Disable : 0

#if 1
	/* [Drex] Step XX. Set the PHY for dqs pull down mode ??? */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[14],
			(0x0 <<  8) |						// ctrl_pulld_dq[11:8]
			(0xF <<  0));						// ctrl_pulld_dqs[7:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)

	/* [Drex] Step XX. Enable the PhyControl0.mem_term_en, PhyControl0.phy_term_en" */
	/* [Drex] if On Die Termination is requeired */
	mmio_write_32(&g_drex_reg->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1    <<  31) |					// [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1    <<  30) |					// [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0    <<   0) |					// [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x1    <<  29) |					// [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0    <<  24) |					// [28:24] ctrl_pd. Input Gate for Power Down.
//			(0x0    <<   7) |						// [23: 7] Reserved - SBZ
			(0x0    <<   4) |					// [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
//			(0x1    <<   3) |						// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   3) |					// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
//			(0x0    <<   2) |						// [    2] Reserved - SBZ
			(0x0    <<   1));					// [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1
#endif

#if 0
	/* Step 05. Assert "dfi_init_start" from Low to High */
	mmio_write_32(&g_drex_reg->CONCONTROL,
			(0x0   <<  28) |					// [   28] dfi_init_start
			(0xFFF <<  16) |					// [27:16] timeout_level0
//			(0x3    <<  12) |					// [14:12] rd_fetch
//			(0x1   <<  12) |						// [14:12] rd_fetch
//			(0x1   <<   8) |						// [    8] empty
//			(0x1    <<   5) |						// [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0   <<   3) |					// [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0   <<   1));					// [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved
#else

	mmio_clear_32(&g_drex_reg->CONCONTROL,(0x1 <<  5));			// aref_en[5]. 0:Disable, 1:Enable
	/* Step 05. Assert "dfi_init_start" from Low to High */
	mmio_set_32  (&g_drex_reg->CONCONTROL,(0x1 << 28));			// dfi_init_start[28]. DFI PHY initialization start
#endif
	/* Step 06. Wait until "dfi_init_complete" is set. (DLL lock will be processed.) */
	while ((mmio_read_32(&g_drex_reg->PHYSTATUS) & (0x1 << 3)) == 0);	// dfi_init_complete[3]. wait for DFI PHY initialization complete
	mmio_clear_32(&g_drex_reg->CONCONTROL, (0x1 << 28));			// dfi_init_start[28]. DFI PHY initialization clear

	/* Step 07. Set the PHY for dqs pull down mode */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[14],
			(0x0 <<  8) |						// ctrl_pulld_dq[11:8]
			(0xF <<  0));						// ctrl_pulld_dqs[7:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)

	/* [Drex] Step 08 : Update DLL information */
	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

	/* [Drex] Step 09-01. Set the Memory Control(MemControl)  */
	mmio_write_32(&g_drex_reg->MEMCONTROL,
			(0x0    <<  25) |					// [26:25] mrr_byte     : Mode Register Read Byte lane location
			(0x0    <<  24) |					// [   24] pzq_en       : DDR3 periodic ZQ(ZQCS) enable
//			(0x0    <<  23) |					// [   23] reserved     : SBZ
			(0x3    <<  20) |					// [22:20] bl           : Memory Burst Length                       :: 3'h3  - 8
			((DDR3_CS_NUM - 1) << 16) |				// [19:16] num_chip : Number of Memory Chips                :: 4'h0  - 1chips
			(0x2    <<  12) |					// [15:12] mem_width    : Width of Memory Data Bus                  :: 4'h2  - 32bits
			(0x6    <<   8) |					// [11: 8] mem_type     : Type of Memory                            :: 4'h6  - ddr3
			(0x0    <<   6) |					// [ 7: 6] add_lat_pall : Additional Latency for PALL in cclk cycle :: 2'b00 - 0 cycle
			(0x0    <<   5) |					// [    5] dsref_en     : Dynamic Self Refresh                      :: 1'b0  - Disable
			(0x0    <<   4) |					// [    4] tp_en        : Timeout Precharge                         :: 1'b0  - Disable
			(0x0    <<   2) |					// [ 3: 2] dpwrdn_type  : Type of Dynamic Power Down                :: 2'b00 - Active/precharge power down
			(0x0    <<   1) |					// [    1] dpwrdn_en    : Dynamic Power Down                        :: 1'b0  - Disable
			(0x0    <<   0));					// [    0] clk_stop_en  : Dynamic Clock Control                     :: 1'b0  - Always running

	/* [Drex] Step 09-02. Set the (Phy Control and Memory Control) */
	mmio_write_32(&g_drex_reg->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1    <<  31) |					// [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1    <<  30) |					// [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0    <<   0) |					// [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x1    <<  29) |					// [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0    <<  24) |					// [28:24] ctrl_pd. Input Gate for Power Down.
//			(0x0    <<   7) |						// [23: 7] reserved - SBZ
			(0x0    <<   4) |					// [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			(0x0    <<   3) |					// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   1));					// [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1

	mmio_write_32(&g_drex_reg->CONCONTROL,
			(0x0    <<  28) |					// [   28] dfi_init_start
			(0xFFF  <<  16) |					// [27:16] timeout_level0
			(0x1    <<  12) |					// [14:12] rd_fetch
			(0x1    <<   8) |					// [    8] empty
			(0x1    <<   5) |					// [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0    <<   3) |					// [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0    <<   1));					// [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved

	/* [Drex] Step 10. Memory Base Config */
	mmio_write_32(&g_drex_reg->MEMBASECONFIG[0],
			(DDR3_CS0_BASEADDR <<  16) |				// chip_base[26:16]. AXI Base Address. if 0x20 ==> AXI base addr of memory : 0x2000_0000
			(DDR3_CS_MEMMASK   <<   0));				// 256MB:0x7F0, 512MB: 0x7E0, 1GB:0x7C0, 2GB: 0x780, 4GB:0x700

#if (DDR3_CS_NUM > 1)
	mmio_write_32(&g_drex_reg->MEMBASECONFIG[1],
			(DDR3_CS1_BASEADDR <<  16) |				// chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
			(DDR3_CS_MEMMASK   <<   0));				// chip_mask[10:0]. 2048 - chip size
#endif

	/* [Drex] Step 11. Memory Config */
	mmio_write_32(&g_drex_reg->MEMCONFIG[0],
//				(0x0    <<  16) |				// [31:16] Reserved - SBZ
				(0x1 << 12) |					// [15:12] chip_map. Address Mapping Method (AXI to Memory). 0:Linear(Bank, Row, Column, Width), 1:Interleaved(Row, bank, column, width), other : reserved
				(DDR3_COL_NUM  << 8) |				// [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
				(DDR3_ROW_NUM  << 4) |				// [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
				(DDR3_BANK_NUM << 0));				// [ 3: 0] chip_bank. Number of  Bank Address Bit. others:Reserved, 2:4bank, 3:8banks
#if (DDR3_CS_NUM > 1)
	mmio_write_32(&g_drex_reg->MEMCONFIG[1],
//				(0x0  << 16) |					// [31:16] Reserved - SBZ
				(0x1 << 12) |					// [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
				(DDR3_COL_NUM <<  8) |				// [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
				(DDR3_ROW_NUM <<  4) |				// [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
				(DDR3_BANK_NUM << 0));				// [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
#endif

	/* [Drex] Step 12. Precharge Configuration ???? */
	mmio_write_32(&g_drex_reg->PRECHCONFIG, 0xFF000000);			//- precharge policy counter
	mmio_write_32(&g_drex_reg->PWRDNCONFIG, 0xFFFF00FF);			//- low power counter

	/* [Drex] Step 15.  Set the Access(AC) Timing */
	mmio_write_32( &g_drex_reg->TIMINGAREF,
			(tREFI      <<   0));					//- refresh counter, 800MHz : 0x618

	mmio_write_32(&g_drex_reg->TIMINGROW,
			(tRFC       <<  24) |
			(tRRD       <<  20) |
			(tRP        <<  16) |
			(tRCD       <<  12) |
			(tRC        <<   6) |
			(tRAS       <<   0)) ;
	mmio_write_32(&g_drex_reg->TIMINGDATA,
			(tWTR       <<  28) |
			(tWR        <<  24) |
			(tRTP       <<  20) |
			(W2W_C2C    <<  16) |
			(R2R_C2C    <<  15) |
			(tDQSCK     <<  12) |
			(nWL        <<   8) |
			(nRL        <<   0));

	mmio_write_32(&g_drex_reg->TIMINGPOWER,
			(tFAW       <<  26) |
			(tXSR       <<  16) |
			(tXP        <<   8) |
			(tCKE       <<   4) |
			(tMRD       <<   0));

//	mmio_write_32(&g_drex_reg->TIMINGPZQ, 0x00004084);			//- average periodic ZQ interval. Max:0x4084
	mmio_write_32(&g_drex_reg->TIMINGPZQ, tPZQ);				//- average periodic ZQ interval. Max:0x4084

	mmio_write_32(&g_drex_reg->WRLVL_CONFIG[0], (tWLO << 4));		// tWLO[7:4]

	/* Step 18 :  Send NOP command. */
	send_directcmd(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* Step 19 :  Send MR2 command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR2.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR2.Reg);
#endif

	/* Step 20 :  Send MR3 command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR3.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR3.Reg);
#endif

	/* Step 21 :  Send MR1 command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR1.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif

	/* Step 22 :  Send MR0 command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR0, MR0.Reg);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR0, MR0.Reg);
#endif

	/* Step 23 : Send ZQ Init command */
	send_directcmd(SDRAM_CMD_ZQINIT, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_ZQINIT, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	DMC_Delay(100);

#if 1	/* Skip the following steps if Leveling and Training are not required. (Optional features) */
	MEMMSG("\r\n########## Leveling & Training ##########\r\n");

	/* Step 24. Generate "ctrl_gate_p*", ctrl_read_p* */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 <<   6));			// ctrl_atgate=1
#if 0
	/* Step 25.  Issue Phase 0/1 Read Command during read leveling */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 <<  14));		// p0_cmd_en=1
#endif
	/* Step 26.  Initialize related logic before DQ Calibration */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[2], (0x1 <<   6));			// InitDeskewEn=1

	/* Step 27. Byte Leveling enable. */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 <<  13));		// byte_rdlvl_en=1

	/* Step 28. The time to determine the VMP(Vaild Margin Period) Read Training */
	temp  = mmio_read_32( &g_ddrphy_reg->PHY_CON[1]) & ~(0xF <<  16);	// rdlvl_pass_adj=4
	temp |= (0x6 <<  16);
	mmio_write_32( &g_ddrphy_reg->PHY_CON[1], temp);

	/* Step 29-1. Set "ddr3_cmd=14'h105E" as default value (=PHY_CON[25][29:16]) */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[25 + 1], (0x105E << 16));		// ddr3_cmd  = DDR3:0x105E, LPDDDR2 or LPDDDR3:0x000E
#if 0
	/* Step 29-2. Set "lpddr2_cmd=14'h" as default value (=PHY_CON[25][13:0]) */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[25 + 1], (0x000E <<  0));
#endif
#if 1
	/* Step 29-3. Set "cmd_default=14'h107F" as default value (=PHY_CON[25][13:0]) */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[25 + 1], (0x107F << 0)); 		// cmd_default = DDR3:0x107F, LPDDDR2 or LPDDDR3:0x000F
#endif

	/* Step 30. Recommand the "rdlvl_incr_adj=7'h01" for the best margin */
	temp  = mmio_read_32( &g_ddrphy_reg->PHY_CON[2]) & ~(0x7F << 16);	// rdlvl_incr_adj=1
	temp |= (0x1 <<  16);
	mmio_write_32( &g_ddrphy_reg->PHY_CON[2], temp);

	/*
	  * Step 31. Disable "ctrl_dll_on" int PHY_CON[12] before Leveling
	  * Turn on if the signal is High DLL turn on/Low is turen off.
	  */
	do {
		mmio_set_32  (&g_ddrphy_reg->PHY_CON[12], (0x1 << 5));		// ctrl_dll_on[5]=1
		/* Step 31-1. Read "ctrl_lock_value[8:2]" in PHY_CON[13][16:10] */
		do {
			temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[13]);	// read lock value
		} while((temp & 0x7) != 0x7);

		mmio_clear_32(&g_ddrphy_reg->PHY_CON[12], (0x1 << 5));		// ctrl_dll_on[5]=0

		temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[13]);		// read lock value
	} while((temp & 0x7) != 0x7);
	g_DDRLock = (temp >> 8) & 0x1FF;

	/* Step 31-2. Update "ctrl_force[8:0]" in PHY_CON12[15:7] by the value of "ctrl_lock_value[8:0] */
	temp  = mmio_read_32(&g_ddrphy_reg->PHY_CON[12]);
	temp &= ~(0x7F <<  8);
	temp |= ((g_DDRLock >> 2) << 8);					// ctrl_force[14:8]
	mmio_write_32(&g_ddrphy_reg->PHY_CON[12], temp);

#if 0	// deoks
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0],  (0x1 << 5));			// ctrl_read_disable[5]= 1. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.

	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
#endif

	/* Step 32. DDR Controller Calibration*/
	/* temporary code according to suspend/resume policy. */
//	if (is_resume == 0) {
	{
		/*
		   Step 32-2. Gate Leveling
		  * (It should be used only for DDR3 (800Mhz))
		  */
		if (DDR3_LvlTr & LVLTR_GT_LVL) {
			if (ddr_gate_leveling() < 0)
				return -1;
		}

		/* Step 32-3. Read DQ Calibration */
		if (DDR3_LvlTr & LVLTR_RD_CAL) {
			if (ddr_read_dq_calibration() < 0)
				return -1;
		}

		/* Step 32-5. Write DQ Calibration */
		if (DDR3_LvlTr & LVLTR_WR_CAL) {
			if (ddr_write_dq_calibration() < 0)
				return -1;
		}
	}
#endif	// Skip Training

	/* Step 33. DLL turn on */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[12], (0x1 << 5));			// ctrl_dll_on[12]=1
#if 0	// (Optional)
	/* Step 34. Disable "ctrl_atgate" in PHY_CON0[6] */
	/* if controller controls "ctrl_gate_p0/p1 and "ctrl_read_p0/p1" directly */
	mmio_clear_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 << 6));			// ctrl_atgate=0
#endif
	/* Step 35-1. Deskew Code is updated. */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2],  (0x1 << 12));			// DLLDeskewEn[12]=1
#if 0
	/* Step 35-2. Wait for the PhyStatus0.read_level_complete field to change to '1' */
	while(mmio_read_32(&g_drex_reg->PHYSTATUS) & (1 << 14));		//
	/* Step 35-3. Disable Deskew */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[2],  (0x1 << 12));			// DLLDeskewEn[12]=0
#endif
	/* Step 37. Enable and Disable "ctrl_resync"(=PHY_CON[2]" to make sure All SDLL is updated. */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));			// ctrl_resync=1
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));			// ctrl_resync=0

	/* Step 38. Send Precharge ALL command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (DDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if 0	/* Step 39. Set the MemControl0 & PhyControl0 (Optional) */
	/* [Drex] Step 39-1. Set the Memory Control(MemControl)  (Optional) */

	/* [Drex] Step 39-2. Set the (Phy Control and Memory Control) */
	mmio_write_32(&g_drex_reg->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1  <<  31) |						// [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1  <<  30) |						// [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0  <<   0) |						// [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x1  <<  29) |						// [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0  <<  24) |						// [28:24] ctrl_pd. Input Gate for Power Down.
//			(0x0   <<   7) |						// [23: 7] reserved - SBZ
			(0x0  <<   4) |						// [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			(0x0  <<   3) |						// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0  <<   1));						// [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1

	mmio_write_32(&g_drex_reg->CONCONTROL,
			(0x0   << 28) |						// [   28] dfi_init_start
			(0xFFF << 16) |					// [27:16] timeout_level0
			(0x1   << 12) |						// [14:12] rd_fetch
			(0x1   <<  8) |						// [    8] empty
			(0x1   <<  5) |						// [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0   <<  3) |						// [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0   <<  1));						// [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved
#endif

	/* [Drex] Step 40. Set the Controller Control */
	mmio_set_32(&g_drex_reg->CONCONTROL, (0x1 << 5));

	MEMMSG("DLL Lock Value  = %d\r\n", g_DDRLock);

	MEMMSG("GATE CYC    = 0x%08X\r\n", mmio_read_32( &g_ddrphy_reg->PHY_CON[3]));
	MEMMSG("GATE CODE   = 0x%08X\r\n", mmio_read_32( &g_ddrphy_reg->PHY_CON[8]));
	MEMMSG("Read  DQ    = 0x%08X\r\n", mmio_read_32( &g_ddrphy_reg->PHY_CON[4]));
	MEMMSG("Write DQ    = 0x%08X\r\n", mmio_read_32( &g_ddrphy_reg->PHY_CON[6]));

	return 0;
}
