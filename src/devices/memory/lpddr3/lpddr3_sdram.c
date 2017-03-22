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
#include <lpddr3_ac_timing.h>
#include <lpddr3_sdram.h>

#define DDR_NEW_LEVELING_TRAINING       (1)

#if defined(CHIPID_NXP4330)
#define DDR_CA_CALIBRATION_EN           (1)     // for LPDDR3
#define DDR_CA_SWAP_MODE                (0)
#define DDR_CA_AUTO_CALIB               (1)     // for LPDDR3

#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_GATE_LVL_COMPENSATION_EN    (0)     // Do not use. for Test.
#define DDR_READ_DQ_COMPENSATION_EN     (1)
#define DDR_WRITE_DQ_COMPENSATION_EN    (1)

#define DDR_RESET_GATE_LVL              (1)
#define DDR_RESET_READ_DQ               (1)
#define DDR_RESET_WRITE_DQ              (1)
#endif  // #if defined(CHIPID_NXP4330)

#if defined(CHIPID_S5P4418)
#define DDR_CA_CALIBRATION_EN           (1)     // for LPDDR3
#define DDR_CA_AUTO_CALIB               (1)     // for LPDDR3

#define DDR_GATE_LEVELING_EN            (1)     // for DDR3, great then 667MHz
#define DDR_READ_DQ_CALIB_EN            (1)
#define DDR_WRITE_DQ_CALIB_EN           (1)

#define DDR_GATE_LVL_COMPENSATION_EN    (0)     // Do not use. for Test.
#define DDR_READ_DQ_COMPENSATION_EN     (1)
#define DDR_WRITE_DQ_COMPENSATION_EN    (1)

#define DDR_RESET_GATE_LVL              (1)
#define DDR_RESET_READ_DQ               (1)
#define DDR_RESET_WRITE_DQ              (1)
#endif  // #if defined(CHIPID_S5P4418)

#define CFG_DDR_LOW_FREQ                (0)
#define CFG_ODT_ENB                     (1)

#define MEM_CALIBRATION_INFO		(1)

#define nop() __asm__ __volatile__("mov\tr0,r0\t@ nop\n\t");

struct s5p4418_drex_sdram_reg *const g_drex_reg =
    (struct s5p4418_drex_sdram_reg * const)PHY_BASEADDR_DREX_MODULE_CH0_APB;
struct s5p4418_ddrphy_reg *const g_ddrphy_reg =
    (struct s5p4418_ddrphy_reg * const)PHY_BASEADDR_DREX_MODULE_CH1_APB;

struct dram_device_info g_ddr3_info;

unsigned int g_ddr_lockvalue;
unsigned int g_GateCycle;
unsigned int g_GateCode;
unsigned int g_RDvwmc;
unsigned int g_WRvwmc;

void DMC_Delay(int milisecond)
{
	register volatile int count;

	for (count = 0; count < milisecond; count++) {
		nop();
	}
}

inline void send_directcmd(SDRAM_CMD cmd, unsigned char chip_num,
	SDRAM_MODE_REG mrx, unsigned short value)
{
	mmio_write_32(&g_drex_reg->DIRECTCMD,
			((cmd << 24) | ((chip_num & 1) << 20) |
			((mrx >> 6) & 0x3) | (((mrx >> 3) & 0x7) << 16) |
			((mrx & 0x7) << 10) | ((value & 0xFF) << 2)));
}

#if (CONFIG_SUSPEND_RESUME == 1)
void enter_self_refresh(void)
{
	union SDRAM_MR MR;
	unsigned int value;
	unsigned int chip_num = 0;

#if (LPDDR3_CS_NUM > 1)
	chip_num = 0x3;
#else
	chip_num = 0x1;
#endif

	/* step 01. check the memory controller's busy state */
	while (mmio_read_32(&g_drex_reg->CHIPSTATUS) & 0xF) {
		nop();
	}
	DMC_Delay(10000);

	/* step 02. send precharge all Command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	DMC_Delay(10000);

#if 0	/* step xx. PASR bank Command (optional) */
	unsigned short MR16 = 0xFF << 0;
	unsigned short MR17 = 0xFF << 0;

	// Send MR16 PASR_Bank Command.
	send_directcmd(SDRAM_CMD_MRS, 0, 16, MR16);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, 16, MR16);
#endif

	// Send MR17 PASR_Seg Command.
	send_directcmd(SDRAM_CMD_MRS, 0, 17, MR17);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, 17, MR17);
#endif
	DMC_Delay(10000);
#endif

	/* step 03. sdram(device) odt on/off */
	MR.LP_MR11.DQ_ODT = 0;							// 0: Disable, 1: RZQ/4, 2:RZQ/2, 3:RZQ/1
	MR.LP_MR11.PD_CON = 0;							// 0: ODT Disable by during power down. 1: ODT Enable  by during power down. MR11
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR11, MR.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR11, MR.Reg);
#endif
	DMC_Delay(1000 * 3);

	/* step 04. enter self-refresh Command */
	send_directcmd(SDRAM_CMD_REFS, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_REFS, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* step xx. check the memory controller's busy state */
	do {
		value = (mmio_read_32(&g_drex_reg->CHIPSTATUS) & chip_num);
	} while (value);

	/* step xx. check the memory controller's busy state */
	do {
		value = ((mmio_read_32(&g_drex_reg->CHIPSTATUS) >> 8) & chip_num);
	} while (value != chip_num);

	/* step xx. disable the auto refresh counter */
	mmio_clear_32(&g_drex_reg->CONCONTROL, (0x1 << 5));			// afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1

	/* step xx. dynamic clock controll on */
	mmio_set_32(&g_drex_reg->MEMCONTROL, (0x1 << 0));			// clk_stop_en[0] : Dynamic Clock control   :: 1'b0  - Always running
}

void exit_self_refresh(void)
{
	union SDRAM_MR MR;

	/* step xx. dynamic clock controll off*/
	mmio_clear_32(&g_drex_reg->MEMCONTROL, (0x1 << 0));			// clk_stop_en[0] : Dynamic clock Control   :: 1'b0  - Always running
	DMC_Delay(10);

	/* step xx. enable the auto refresh counter */
	mmio_set_32(&g_drex_reg->CONCONTROL,(0x1<< 5));				// afre_en[5]. Auto Refresh Counter. Disable:0, Enable:1
	DMC_Delay(10);

	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR.Reg);
#endif

	/* step 01. exit the self-refresh */
	send_directcmd(SDRAM_CMD_REFSX, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_REFSX, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* step 02. odt on/off */
	MR.LP_MR11.DQ_ODT = 0;
	MR.LP_MR11.PD_CON = 0;
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR11, MR.Reg);
#endif

	/* step 03. send the precharge all Command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	DMC_Delay(1000 * 2);
}
#endif // #if (CONFIG_SUSPEND_RESUME == 1)

#if (DDR_NEW_LEVELING_TRAINING == 1)

#if (DDR_CA_CALIBRATION_EN == 1)

#if 0
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
#endif

int ddr_ca_calibration(void)
{
	int ret = FALSE;
#if (DDR_CA_AUTO_CALIB == 1)
	unsigned int lock_div4 = (g_ddr_lockvalue >> 2);
	unsigned int offsetd;
	unsigned int vwml, vwmr, vwmc;
	unsigned int temp, mr41, mr48;
	int find_vmw;
	int code;

	code = 0x8;								// CMD SDLL Code default value "ctrl_offsetd"=0x8
	find_vmw = 0;
	vwml = vwmr = vwmc = 0;

	MEMMSG("\r\n########## CA Calibration - Start ##########\r\n");

#if (DDR_CA_SWAP_MODE == 1)
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[3], (0x1 << 26));			// drex_ca_swap[26]=1
	mmio_set_32(&g_ddrphy_reg->PHY_CON[24 + 1], (0x1 << 0));		// ca_swap_mode[0]=1
#endif

	mmio_set_32(&g_ddrphy_reg->PHY_CON[0], (0x1 << 16));			// ctrl_wrlvl_en(wrlvl_mode)[16]="1" (Enable)
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 23));			// rdlvl_ca_en(ca_cal_mode)[23]="1" (Enable)

	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif
	while (1) {
		mmio_write_32(&g_ddrphy_reg->PHY_CON[10], code);

		mmio_set_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync[24]=1
		mmio_clear_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));	// ctrl_resync[24]=0
		DMC_Delay(0x80);

		send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR41, 0xA4);	//- CH0 : Send MR41 to start CA calibration for LPDDR3 : MA=0x29 OP=0xA4, 0x50690
#if (LPDDR3_CS_NUM > 1)
		send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR41, 0xA4);	//- CH1 : Send MR41 to start CA calibration for LPDDR3 : MA=0x29 OP=0xA4, 0x50690
#endif

		mmio_set_32(&g_drex_reg->CACAL_CONFIG[0], (0x1 << 0));		// deassert_cke[0]=1 : CKE pin is "LOW"
		temp = ((0x3FF << 12) |					// dfi_address_p0[24]=0x3FF
			(tADR   <<  4) |
			(0x1   <<  0));						// deassert_cke[0]=1 : CKE pin is "LOW"
		mmio_write_32(&g_drex_reg->CACAL_CONFIG[0], temp);

		mmio_write_32(&g_drex_reg->CACAL_CONFIG[1], 0x00000001);	// cacal_csn(dfi_csn_p0)[0]=1 : generate one pulse
		/*
		  * CSn(Low and High), cacal_csn field need not to return to "0"
		  * and whenever this field is written in "1", one pulse is genrerated.
		  */
//		DMC_Delay(0x80);

		mr41 = mmio_read_32(&g_drex_reg->CTRL_IO_RDATA) & MASK_MR41;

		mmio_clear_32(&g_drex_reg->CACAL_CONFIG[0], (0x1 << 0));	// deassert_cke[0]=0 : CKE pin is "HIGH" - Normal operation
		DMC_Delay(0x80);

		send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR48, 0xC0);	// CH0 : Send MR48 to start  CA calibration for LPDDR3  : MA=0x30 OP=0xC0, 0x60300
#if (LPDDR3_CS_NUM > 1)
		send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR48, 0xC0);	//CH1 : Send MR48 to start  CA calibration for LPDDR3  : MA=0x30 OP=0xC0, 0x60300
#endif
		mmio_set_32(&g_drex_reg->CACAL_CONFIG[0], (0x1 << 0));		// deassert_cke[0]=1 : CKE pin is "LOW"
		mmio_write_32(&g_drex_reg->CACAL_CONFIG[1], 0x00000001);	// cacal_csn(dfi_csn_p0)[0]=1 : generate one pulse

		/*
		  * CSn(Low and High), cacal_csn field need not to return to "0"
		  * and whenever this field is written in "1", one pulse is genrerated.
		  */
		mr48 = mmio_read_32(&g_drex_reg->CTRL_IO_RDATA) & MASK_MR48;

		mmio_clear_32(&g_drex_reg->CACAL_CONFIG[0], (0x1 << 0));	// deassert_cke[0]=0 : CKE pin is "HIGH" - Normal operation

		/*
		  * If you can not pass 3 consecutive times from the 1st PASS,
		  * initialize it to "find_vmw" = "0" to start searching again until 3
		  * consecutive PASS occur.
		  */
		if (find_vmw < 0x3) {
			if ((mr41 == RESP_MR41) && (mr48 == RESP_MR48)) {
				find_vmw++;
				if (find_vmw == 0x1)
					vwml = code;
				printf("+ %d\r\n", code);
			} else {
				find_vmw = 0x0;
				printf("- %d\r\n", code);
			}
		} else if ((mr41 != RESP_MR41) || (mr48 != RESP_MR48)) {
			find_vmw = 0x4;
			vwmr = code - 1;
			printf("-- %d\r\n", code);
			MEMMSG("MR41 = 0x%08X, MR48 = 0x%08X\r\n", mr41, mr48);
			break;
		}

		code++;

		if (code == 256) {
			printf("[Error] CA Calibration : code %d\r\n", code);
			while (1)
				;
			goto ca_error_ret;
		}
	}

	lock_div4 = (g_ddr_lockvalue >> 2);
	vwmc = (vwml + vwmr) >> 1;

	/*
	  * (g_ddr_lockvalue >> 2) means "T/4", lock value means the number of delay
	  * cell for one period.
	  */
	code = (int)(vwmc - lock_div4);
	offsetd = (vwmc & 0xFF);
	ret = TRUE;

ca_error_ret:

#if (MEM_CALIBRATION_INFO == 1)
	if (ret != TRUE) {
		printf("\r\n CA Calibration Failed!! \r\n");
	} else {
		MEMMSG("\r\n#### CA Calibration - Information #####\r\n");
		printf("CA Calibration Success!! \r\n");
		MEMMSG("CA Vaild Margin Left  : %d(%d) \r\n", vwml, vwml - lock_div4);
		MEMMSG("CA Vaild Margin Center: %d(%d) \r\n", vwmc, offsetd);
		MEMMSG("CA Vaild Margin Right : %d(%d) \r\n", vwmr, vwmr - lock_div4);
		MEMMSG("###########################################\r\n");
	}
#endif

	if (ret == FALSE)
		mmio_write_32(&g_ddrphy_reg->PHY_CON[10], 0x08);
	else
		mmio_write_32(&g_ddrphy_reg->PHY_CON[10], offsetd);

	mmio_set_32  (&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync[24]=0x1 (HIGH)
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync[24]=0x0 (LOW)

	mmio_clear_32(&g_ddrphy_reg->PHY_CON[0], (0x1 << 16));			// ctrl_wrlvl_en(wrlvl_mode)[16]="0"(Disable)

	// Exiting Calibration Mode of LPDDR3 using MR42
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR42, 0xA8);		// CH0 : Send MR42 to exit from A calibration mode for  LPDDR3, MA=0x2A OP=0xA8,0x50AA0
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR42, 0xA8);		// CH0 : Send MR42 to exit from A calibration mode for  LPDDR3, MA=0x2A OP=0xA8,0x50AA0
#endif

#else

	MEMMSG("\r\n########## CA Calibration - Start ##########\r\n");

	mmio_write_32(&g_ddrphy_reg->PHY_CON[10], 0x37);			// Set CA delay time. - Miware value OK

	mmio_set_32  (&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync[24]=0x1 (HIGH)
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync[24]=0x0 (LOW)
	DMC_Delay(0x80);
#endif // #if (DDR_CA_AUTO_CALIB == 1)

	/* [Drex] Step XX. Set the PHY for dqs pull down mode ??? */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[14],
			(0x0 <<  8) |						// ctrl_pulld_dq[11:8]
			(0xF <<  0));						// ctrl_pulld_dqs[7:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)

	MEMMSG("\r\n########## CA Calibration - End ##########\r\n");

	return ret;
}
#endif

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
	unsigned int LockValue = g_ddr_lockvalue;

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
	volatile int cal_count = 0;
#if 0
	volatile int status;
#endif
	volatile int value;
	volatile int response;
	int ret = 0;

	MEMMSG("\r\n########## Gate Leveling - Start ##########\r\n");

	/* Step 01. Send ALL Precharge command. */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* Step 02. Set the Memory in DQ Pattern A (MR32) */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR32, 0x00);

	/* Step 03. Set the Gate Leveling Mode. */
	/* Step 03-1. Enable "bylvl_gate_en(=gate_cal_mode)" in PHY_CON2[24] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2],      (0x1   <<  24));		// rdlvl_gate_en[24] = 1
	/* Step 03-2. Enable "ctrl_shgate" in PHY_CON0[8] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0],      (0x5   <<   6));		// ctrl_shgate[8] = 1, ctrl_atgate[6] = 1
	/* Step 03-3. Set "ctrl_gateduradj[3:0] (=PHY_CON1[23:20]) (DDR3: 4'b1011") */
	value = mmio_read_32(&g_ddrphy_reg->PHY_CON[1]) & ~(0xF << 20);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[1], (value | (0xB << 20)));	// ctrl_gateduradj[23:20] = DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9

	/* Step 04. Assert "dfi_rdlvl_en" and "dfi_rdlvl_gate_en" after "dfi_rdlvl_resp" is disabled. */
	/* Step 04-01. Set the "ctrl_rdlvl_gate_en[0] = 1" */
	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 1
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

gate_err_ret:
	/* Step 06. Deassert "dfi_rdlvel_en", "dfi_rdlvl_gate_en" after "dfi_rdlvl_resp" is disabled. */
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 0
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));			// ctrl_rdlvl_data_en[0] = 0

#if (MEM_CALIBRATION_INFO == 1)
	gate_leveling_information();
#endif

#if 0
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[2], (0x1   <<  24));		// rdlvl_gate_en[24] = 0

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_CENTER_CYCLE);
	g_GateCycle = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], GATE_CENTER_CODE);
	g_GateCode = mmio_read_32(&g_ddrphy_reg->PHY_CON[19 + 1]);


#if (DDR_RESET_GATE_LVL == 1)
	int offset, c_offset;
	int cycle[4], gate_cycle;

	for (cal_count= 0; cal_count < 4; cal_count++)
		cycle[cal_count] = ((g_GateCycle >> (8 * cal_count)) & 0xFF);

	offset = GetVWMC_Offset(g_GateCode, (g_ddr_lockvalue >> 2));
#if (DDR_GATE_LVL_COMPENSATION_EN == 1)
	c_offset = GetVWMC_Compensation(offset);
#else
	c_offset = offset;
#endif // #if (DDR_GATE_LVL_COMPENSATION_EN == 1)
	mmio_write_32(&g_ddrphy_reg->PHY_CON[8], c_offset);			// ctrl_offsetc

	gate_cycle = (((U8)cycle[3] << 15) | ((U8)cycle[2] << 10) |
			((U8)cycle[1] << 5) | (U8)cycle[0]);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[3], gate_cycle);			// ctrl_shiftc
#endif // #if (DDR_RESET_GATE_LVL == 1)
#endif

	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], 0x0);				// ReadModeCon[7:0] = 0x0

	/* Step 07. Disable DQS Pull Down Mode. */
//	mmio_clear_32(&g_ddrphy_reg->PHY_CON[14], (0xF << 0));			// ctrl_pulld_dqs[3:0]

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
			printf("SLICE%2d: %2d ~ %2d ~ %2d (Range: %2d)(Deskew: %2d) \r\n",
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
	printf("###########################################\r\n");

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
	volatile int cal_count = 0;
	volatile int status, response;
	int ret = 0, temp =0;

	MEMMSG("\r\n########## Read DQ Calibration - Start ##########\r\n");

	/* Step 01. Send Precharge ALL Command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	/* Step 02. Set the Memory in MPR Mode (MR32) */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR32, 0x00);

#if 1
#if 1
	/* Step 03-1. Set "PHY_CON1[15:0]=0x00FF" and "byte_rdlvl_en=1(=PHY_CON0[13]). */
//	temp = mmio_read_32(g_ddrphy_reg->PHY_CON[1]) & ~(0xFFFF << 0);
//	temp |= (0x00FF << 0);
//	mmio_write_32(&g_ddrphy_reg->PHY_CON[1], temp);
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[1], 0xFFFF);
	mmio_set_32(&g_ddrphy_reg->PHY_CON[1], 0x00FF);
	temp = mmio_read_32(g_ddrphy_reg->PHY_CON[1]);
	printf("PHY_CON[1] : 0x%08X \r\n", temp);
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0], (1 << 13));
#else
	/* Step 03-1. Set "PHY_CON1[15:0]=0x0001" and "byte_rdlvl_en=1(=PHY_CON0[13]). */
	temp = mmio_read_32(g_ddrphy_reg->PHY_CON[1]) & ~(0xFFFF << 0);
	temp |= (0x0001 << 0);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[1], temp);
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0], (1 << 13));
#endif
#endif

	/* Step 04. Enable "rd_cal_mode" in PHY_CON2[25] */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 25));			// rdlvl_en[25] = 1 (=rd_cal_mode)

	/* Step 05. Memory Controller should assert "dfi_rdlvl_en" to do read leveling. (??)*/
//	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 1
	mmio_write_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));			// ctrl_rdlvl_data_en[1]=1

	/* Step 06. Wait until "rd_wr_cal_resp" (=PHYSTATUS[14]) */
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
	/* Step 07. End of Read DQ Calibration */
	/* Step 07-1. memory controller should deassert "dfi_rdlvl_en" and "dfi_rdlvl_gate_en" after */
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 0));			// ctrl_rdlvl_gate_en[0] = 0
	mmio_clear_32(&g_drex_reg->RDLVL_CONFIG, (0x1 << 1));			// ctrl_rdlvl_data_en[1] = 0

#if (MEM_CALIBRATION_INFO == 1)
	read_dq_calibration_information();
#endif

#if 0
#if (DDR_RESET_READ_DQ == 1)
	unsigned int offset, c_offset;;
	offset = GetVWMC_Offset(g_RDvwmc, (g_ddr_lockvalue >> 2));
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

	/* [Drex] Step 08 : Update DLL information */
	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

//	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], 0x0);				// ReadModeCon[7:0] = 0x0

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
			printf("SLICE%2d: %2d ~ %2d ~ %2d (Range: %2d)(Deskew: %2d) \r\n",
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
	printf("###########################################\r\n");
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
	int ret = 0, temp = 0;

	MEMMSG("\r\n########## Write DQ Calibration - Start ##########\r\n");

#if 1
	/* Step XX. Send All Precharge Command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
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

#if 1
#if 1
	/* Step 04-0. Set "PHY_CON1[15:0]=0x0100" and "byte_rdlvl_en=1(=PHY_CON0[13]). */
	temp = mmio_read_32(g_ddrphy_reg->PHY_CON[1]) & ~(0xFFFF << 0);
	temp |= (0x0001 <<0);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[1], temp);
	mmio_set_32(&g_ddrphy_reg->PHY_CON[0], (1 << 13));
#else
	/* Step 04-1. Set "PHY_CON1[15:0]=0xFF00" and "byte_rdlvl_en=0(=PHY_CON0[13]). */
	temp = mmio_read_32(g_ddrphy_reg->PHY_CON[1]) & ~(0xFFFF << 0);
	temp |= (0x00FF << 0);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[1], temp);
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[0], (1 << 13));
#endif
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
//	mmio_clear_32(&g_drex_reg->WRTRA_CONFIG, (0x1 << 0));			// [   0] write_training_en = 0

	/* Step 08. Clear the "wr_deskew_en" in PHY_CON2[27] */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[2], (0x1 << 27) );		// wr_deskew_en[27] = 0

//	mmio_write_32(&g_ddrphy_reg->PHY_CON[5], 0x0);				// ReadModeCon[7:0] = 0x0

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

#if (CFG_DDR_LOW_FREQ == 1)
void low_frequency_first(void)
{
	// 5. set CA0 ~ CA9 deskew code to 7h'60
	mmio_write_32(&g_ddrphy_reg->PHY_CON[31 + 1], 0x0C183060);		// PHY_CON31 DeSkew Code for CA
	mmio_write_32(&g_ddrphy_reg->PHY_CON[32 + 1], 0x60C18306);		// PHY_CON32 DeSkew Code for CA
	mmio_write_32(&g_ddrphy_reg->PHY_CON[33 + 1], 0x00000030);		// PHY_CON33 DeSkew Code for CA

	// Step 16: ctrl_offsetr0~3 = 0x7F, ctrl_offsetw0~3 = 0x7F
	mmio_write_32(&g_ddrphy_reg->PHY_CON[4], 0x7F7F7F7F);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[6], 0x7F7F7F7F);

	// Step 17: ctrl_offsetd[7:0] = 0x7F
	mmio_write_32(&g_ddrphy_reg->PHY_CON[10], (0x7F << 0));

	// Step 18: ctrl_force[14:8] = 0x7F
	mmio_set_32(&g_ddrphy_reg->PHY_CON[12], (0x7F << 8));

	// Step 19: ctrl_dll_on[5] = 0
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[12], (0x1 << 5));

	// Step 20: Wait 10MPCLK
	DMC_Delay(100);
}

void low_frequency_second(void)
{
#if 0
	// set CA0 ~ CA9 deskew code to 7h'00
	mmio_write_32(&g_ddrphy_reg->PHY_CON[31 + 1], 0x00000000);		// PHY_CON31 DeSkew Code for CA
	mmio_write_32(&g_ddrphy_reg->PHY_CON[32 + 1], 0x00000000);		// PHY_CON32 DeSkew Code for CA
	mmio_write_32(&g_ddrphy_reg->PHY_CON[33 + 1], 0x00000000);		// PHY_CON33 DeSkew Code for CA

	// Step 30: ctrl_offsetr0~3 = 0x00, ctrl_offsetw0~3 = 0x00
	mmio_write_32(&g_ddrphy_reg->PHY_CON[4],    0x00000000);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[6],    0x00000000);

	// Step 31: ctrl_offsetd[7:0] = 0x00
	mmio_write_32(&g_ddrphy_reg->PHY_CON[10],   0x00000000);
#else
	// set CA0 ~ CA9 deskew code to 7h'08
	mmio_write_32(&g_ddrphy_reg->PHY_CON[31 + 1], 0x81020408);		// PHY_CON31 DeSkew Code for CA
	mmio_write_32(&g_ddrphy_reg->PHY_CON[32 + 1], 0x08102040);		// PHY_CON32 DeSkew Code for CA
	mmio_write_32(&g_ddrphy_reg->PHY_CON[33 + 1], 0x00000004);		// PHY_CON33 DeSkew Code for CA

	// Step 30: ctrl_offsetr0~3 = 0x08, ctrl_offsetw0~3 = 0x08
	mmio_write_32(&g_ddrphy_reg->PHY_CON[4], 0x08080808);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[6], 0x08080808);

	// Step 31: ctrl_offsetd[7:0] = 0x08
	mmio_write_32(&g_ddrphy_reg->PHY_CON[10], 0x00000008);
#endif
}
#endif

int lpddr3_initialize(unsigned int is_resume)
{
	union SDRAM_MR MR1, MR2, MR3, MR11;
	unsigned int DDR_WL, DDR_RL;
	unsigned int DDR3_LvlTr;
	unsigned int temp;

	is_resume = is_resume;

	MR1.Reg = 0;
	MR2.Reg = 0;
	MR3.Reg = 0;
	MR11.Reg = 0;

	MEMMSG("\r\n[LPDDR3] POR Init Start\r\n");

	/* Step 01. Reset (DPHY, DREX, DRAM)  (Min: 10ns, Typ: 200us) */
	if (resetgen_sequence() < 0) {
		MEMMSG("(DPHY, DREX) Controller Reset Failed! \r\n");
		return -1;
	}

	DDR3_LvlTr = CONFIG_LPDDR3_LVLTR_EN;

	MR1.LP_MR1.BL = 3;

	MR2.LP_MR2.WL_SEL = 0;
	MR2.LP_MR2.WR_LVL = 0;

	DDR_WL = nWL;
	DDR_RL = nRL;

	if (MR1_nWR > 9) {
		MR1.LP_MR1.WR = (MR1_nWR - 10) & 0x7;
		MR2.LP_MR2.WRE = 1;
	} else {
		MR1.LP_MR1.WR = (MR1_nWR - 2) & 0x7;
		MR2.LP_MR2.WRE = 0;
	}

	if (MR1_AL < 6)
		MR2.LP_MR2.RL_WL = 4;
	else
		MR2.LP_MR2.RL_WL = (MR1_AL - 2);

	MR3.LP_MR3.DS = MR3_DS;

#if (CFG_ODT_ENB == 1)
	MR11.LP_MR11.DQ_ODT = MR11_DQ_ODT;						// DQ ODT - 0: Disable, 1: Rzq/4, 2: Rzq/2, 3: Rzq/1
#endif
	MR11.LP_MR11.PD_CON = MR11_PD_CTL;

	/* Step 2. Select Memory type : LPDDR3 */
#if (DDR_CA_SWAP_MODE == 1)
	mmio_write_32(&g_ddrphy_reg->PHY_CON[22 + 1], 0x00000041);		// lpddr2_addr[19:0] = 0x041
#else
	mmio_write_32(&g_ddrphy_reg->PHY_CON[22 + 1], 0x00000208);		// lpddr2_addr[19:0] = 0x208
#endif

#if 1	// Old Code - unknown (by.deoks)
	mmio_write_32(&g_ddrphy_reg->PHY_CON[1],
	    		(0x0 << 28) |						// [31:28] ctrl_gateadj
			(0x9 << 24) |						// [27:24] ctrl_readadj
			(0xB << 20) |						// [23:20] ctrl_gateduradj  :: DDR3: 0x0, LPDDR3: 0xB, LPDDR2: 0x9
			(0x1 << 16) |						// [19:16] rdlvl_pass_adj
//			(0x0001 << 0));					// [15: 0] rdlvl_rddata_adj :: LPDDR3 : 0x0001  or 0x00FF
			(0x00FF << 0));						// [15: 0] rdlvl_rddata_adj :: LPDDR3 : 0x0001 or 0x00FF

	mmio_write_32(&g_ddrphy_reg->PHY_CON[2],
			(0x0 << 28) |						// [31:28] ctrl_readduradj
			(0x0 << 27) |						// [   27] wr_deskew_en
			(0x0 << 26) |						// [   26] wr_deskew_con
			(0x0 << 25) |						// [   25] rdlvl_en
			(0x0 << 24) |						// [   24] rdlvl_gate_en
			(0x0 << 23) |						// [   23] rdlvl_ca_en
			(0x1 << 16) |						// [22:16] rdlvl_incr_adj
			(0x0 << 14) |						// [   14] wrdeskew_clear
			(0x0 << 13) |						// [   13] rddeskew_clear
			(0x0 << 12) |						// [   12] dlldeskew_en
			(0x2 << 10) |						// [11:10] rdlvl_start_adj - Right shift, valid value: 1 or 2
			(0x1 <<  8) |						// [ 9: 8] rdlvl_start_adj - Left shift  valid value: 0 ~ 2
			(0x0 <<  6) |						// [    6] initdeskewen
			(0x0 <<  0));						// [ 1: 0] rdlvl_gateadj
#endif	// Old Code - unkndown

		temp = ((0x0  << 29) |						// [31:29] Reserved - SBZ.
			(0x17 << 24) |						// [28:24] T_WrWrCmd.
//			(0x0  <<  22) |						// [23:22] Reserved - SBZ.
			(0x0  << 20) |						// [21:20] ctrl_upd_range.
#if (tWTR == 3)									// 6 cycles
	     		(0x7  << 17) |						// [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#elif (tWTR == 2)								// 4 cycles
	     		(0x6  << 17) |						// [19:17] T_RwRdCmd. 6:tWTR=4cycle, 7:tWTR=6cycle
#endif
			(0x0  << 16) |						// [   16] ctrl_wrlvl_en[16]. Write Leveling Enable. 0:Disable, 1:Enable
//			(0x0  <<  15) |						// [   15] Reserved SBZ.
			(0x0  << 14) |						// [   14] p0_cmd_en. 0:Issue Phase1 Read command during Read Leveling. 1:Issue Phase0
			(0x0  << 13) |						// [   13] byte_rdlvl_en. Read Leveling 0:Disable, 1:Enable
/*LPDDR3*/    		(0x3  << 11) |						// [12:11] ctrl_ddr_mode. 0:DDR2&LPDDR1, 1:DDR3, 2:LPDDR2, 3:LPDDR3
//			(0x0  << 10) |						// [   10] Reserved - SBZ.
			(0x1  <<  9) |						// [    9] ctrl_dfdqs. 0:Single-ended DQS, 1:Differential DQS
			(0x0  <<  8) |						// [    8] ctrl_shgate. 0:Gate signal length=burst length/2+N, 1:Gate signal length=burst length/2-1
			(0x0  <<  7) |						// [    7] ctrl_ckdis. 0:Clock output Enable, 1:Disable
//			(0x1  <<  6) |						// [    6] ctrl_atgate.
			(0x0  <<  4) |						// [    4] ctrl_cmosrcv.
			(0x0  <<  3) |						// [    3] ctrl_read_width.
			(0x0  <<  0));						// [ 2: 0] ctrl_fnc_fb. 000:Normal operation.

	mmio_write_32(&g_ddrphy_reg->PHY_CON[0], temp);	

	temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[25 + 1])
		& ~((0x3FFF << 16) | (0x3FFF << 0));
	temp |= (0x105E << 16);						// ddr3_cmd= DDR3:0x105E
	temp |= (0x000E <<  0);						// lpddr2_cmd = LPDDDR2 or LPDDDR3:0x000E
	mmio_write_32(&g_ddrphy_reg->PHY_CON[25 + 1], temp);


	MEMMSG("[DDR] Phy Initialize\r\n");

	/* Step 03. Set Write Latency(WL), Read Latency(RL), Burts Length(BL) */
	/* Step 03-01. Set the RL(Read Latency), , BL(Burst Length) */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[42 + 1],
			(0x8    <<   8) |					// Burst Length(BL)
			(DDR_RL <<   0));					// Read Latency(RL) - 800MHz:0xB, 533MHz:0x5

	/* Step 03-02. Set the WL(Write Latency)  */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[26 + 1],
			(((DDR_WL + 1) <<  16) | 0x000F));			// T_wrdata_en, In LPDDR3

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
/* LPDDR3 */		(0x1    <<  19) |					// [   19] zq_mode_noterm. Termination. 0:Enable, 1:Disable
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

//	mmio_clear_32(&g_ddrphy_reg->PHY_CON[16], (0x1 << 18));		// zq_clk_div_en[18]. Clock Dividing Enable : 1, Disable : 0

#if 1
	/* [Drex] Step XX. Set the PHY for dqs pull down mode ??? */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[14],
			(0x0 <<  8) |						// ctrl_pulld_dq[11:8]
			(0x0 <<  0));						// ctrl_pulld_dqs[7:0].  No Gate leveling : 0xF, Use Gate leveling : 0x0(X)

	/* Step 05. Assert "dfi_init_start" from Low to High */
	mmio_write_32(&g_drex_reg->CONCONTROL,
			(0x0   << 28) |						// [   28] dfi_init_start
			(0xFFF << 16) |					// [27:16] timeout_level0
			(0x1   << 12) |						// [14:12] rd_fetch
			(0x1   <<  8) |						// [    8] empty
//			(0x1   <<  5) |						// [    5] aref_en - Auto Refresh Counter. Disable:0, Enable:1
			(0x0   <<  3) |						// [    3] io_pd_con - I/O Powerdown Control in Low Power Mode(through LPI)
			(0x0   <<  1));						// [ 2: 1] clk_ratio. Clock ratio of Bus clock to Memory clock. 0x0 = 1:1, 0x1~0x3 = Reserved
	mmio_set_32  (&g_drex_reg->CONCONTROL,(0x1 << 28));			// dfi_init_start[28]. DFI PHY initialization start

	/* Step 06. Wait until "dfi_init_complete" is set. (DLL lock will be processed.) */
	while ((mmio_read_32(&g_drex_reg->PHYSTATUS) & (0x1 << 3)) == 0);	// dfi_init_complete[3]. wait for DFI PHY initialization complete
	mmio_clear_32(&g_drex_reg->CONCONTROL, (0x1 << 28));			// dfi_init_start[28]. DFI PHY initialization clear
#if 0	// Old Code -unknown
	/* [Drex] Step XX. Enable the PhyControl0.mem_term_en, PhyControl0.phy_term_en" */
	/* [Drex] if On Die Termination is requeired */
	mmio_write_32(&g_drex_reg->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1    <<  31) |					// [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1    <<  30) |					// [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0    <<   0) |					// [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
/* LPDDR3*/		(0x0    <<  29) |					// [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0    <<  24) |					// [28:24] ctrl_pd. Input Gate for Power Down.
//			(0x0    <<   7) |						// [23: 7] Reserved - SBZ
			(0x0    <<   4) |					// [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			(0x0    <<   3) |					// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
//			(0x1    <<   3) |						// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   3) |					// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   2) |					// [    2] Reserved - SBZ
			(0x0    <<   1));					// [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1
#endif
#endif
	/* [Drex] Step 08 : Update DLL information */
	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

#if 1	//
	/* [Drex] Step 09-01. Set the Memory Control(MemControl)  */
	mmio_write_32(&g_drex_reg->MEMCONTROL,
			(0x0    <<  25) |					// [26:25] mrr_byte     : Mode Register Read Byte lane location
			(0x0    <<  24) |					// [   24] pzq_en       : DDR3 periodic ZQ(ZQCS) enable
//			(0x0    <<  23) |					// [   23] reserved     : SBZ
			(0x3    <<  20) |					// [22:20] bl           : Memory Burst Length :: 3'h3  - 8
			((LPDDR3_CS_NUM - 1) << 16) |				// [19:16] num_chip : Number of Memory Chips :: 4'h0  - 1chips
			(0x2    <<  12) |					// [15:12] mem_width    : Width of Memory Data Bus :: 4'h2  - 32bits
/* LPDDR3 */		(0x7    <<   8) |					// [11: 8] mem_type     : Type of Memory :: 4'h6  - ddr3
			(0x0    <<   6) |					// [ 7: 6] add_lat_pall : Additional Latency for PALL in cclk cycle :: 2'b00 - 0 cycle
			(0x0    <<   5) |					// [    5] dsref_en     : Dynamic Self Refresh :: 1'b0  - Disable
			(0x0    <<   4) |					// [    4] tp_en        : Timeout Precharge :: 1'b0  - Disable
			(0x0    <<   2) |					// [ 3: 2] dpwrdn_type  : Type of Dynamic Power Down :: 2'b00 - Active/precharge power down
			(0x0    <<   1) |					// [    1] dpwrdn_en    : Dynamic Power Down :: 1'b0  - Disable
			(0x0    <<   0));					// [    0] clk_stop_en  : Dynamic Clock Control  :: 1'b0  - Always running

	/* [Drex] Step 09-02. Set the (Phy Control and Memory Control) */
	mmio_write_32(&g_drex_reg->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1    <<  31) |					// [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1    <<  30) |					// [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0    <<   0) |					// [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x0    <<  29) |					// [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0    <<  24) |					// [28:24] ctrl_pd. Input Gate for Power Down.
//			(0x0    <<   7) |						// [23: 7] reserved - SBZ
			(0x0    <<   4) |					// [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			(0x0    <<   3) |					// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0    <<   1));					// [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1
#endif

	/* [Drex] Step 10. Memory Base Config */
	mmio_write_32(&g_drex_reg->MEMBASECONFIG[0],
			(LPDDR3_CS0_BASEADDR <<  16) |				// chip_base[26:16]. AXI Base Address. if 0x20 ==> AXI base addr of memory : 0x2000_0000
			(LPDDR3_CS_PERMASK   <<   0));				// 256MB:0x7F0, 512MB: 0x7E0, 1GB:0x7C0, 2GB: 0x780, 4GB:0x700

#if (LPDDR3_CS_NUM > 1)
	mmio_write_32(&g_drex_reg->MEMBASECONFIG[1],
			(LPDDR3_CS1_BASEADDR <<  16) |				// chip_base[26:16]. AXI Base Address. if 0x40 ==> AXI base addr of memory : 0x4000_0000, 16MB unit
			(LPDDR3_CS_PERMASK   <<   0));				// chip_mask[10:0]. 2048 - chip size
#endif

	/* [Drex] Step 11. Memory Config */
	mmio_write_32(&g_drex_reg->MEMCONFIG[0],
//				(0x0    <<  16) |				// [31:16] Reserved - SBZ
				(0x1 << 12) |					// [15:12] chip_map. Address Mapping Method (AXI to Memory). 0:Linear(Bank, Row, Column, Width), 1:Interleaved(Row, bank, column, width), other : reserved
				(LPDDR3_COL_NUM << 8) |				// [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
				(LPDDR3_ROW_NUM << 4) |				// [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
				(LPDDR3_BANK_NUM << 0));			// [ 3: 0] chip_bank. Number of  Bank Address Bit. others:Reserved, 2:4bank, 3:8banks
#if (LPDDR3_CS_NUM > 1)
	mmio_write_32(&g_drex_reg->MEMCONFIG[1],
//				(0x0  << 16) |					// [31:16] Reserved - SBZ
				(0x1 << 12) |					// [15:12] chip_map. Address Mapping Method (AXI to Memory). 0 : Linear(Bank, Row, Column, Width), 1 : Interleaved(Row, bank, column, width), other : reserved
				(LPDDR3_COL_NUM <<  8) |			// [11: 8] chip_col. Number of Column Address Bit. others:Reserved, 2:9bit, 3:10bit,
				(LPDDR3_ROW_NUM <<  4) |			// [ 7: 4] chip_row. Number of  Row Address Bit. others:Reserved, 0:12bit, 1:13bit, 2:14bit, 3:15bit, 4:16bit
				(LPDDR3_BANK_NUM << 0));			// [ 3: 0] chip_bank. Number of  Row Address Bit. others:Reserved, 2:4bank, 3:8banks
#endif

	/* [Drex] Step 12. Precharge Configuration ???? */
	mmio_write_32(&g_drex_reg->PRECHCONFIG, 0xFF000000);			//- precharge policy counter
	mmio_write_32(&g_drex_reg->PWRDNCONFIG, 0xFFFF00FF);			//- low power counter

	/* [Drex] Step 15.  Set the Access(AC) Timing */
	mmio_write_32(&g_drex_reg->TIMINGAREF,
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

	// step xx. wating for 20ns
	DMC_Delay(0xFFFF);

	/* [Drex]  Step XX. Set the Phy in "ctrl_offsetr 0 ~ 3" of value '0x7F' */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[4], 0x7F7F7F7F);

	/* [Drex]  Step XX. Set the Phy in "ctrl_offsetw 0 ~ 3" of value '0x7F'*/
	mmio_write_32(&g_ddrphy_reg->PHY_CON[6], 0x7F7F7F7F);

	/* [Drex] Step XX. Set the Phy in "ctrl_force" value to '0x7F' */
	temp  = mmio_read_32(&g_ddrphy_reg->PHY_CON[12]);
	temp &= ~(0x7F <<  8);
	temp |= ((0x7F >> 2) << 8);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[12], temp);

	/* [Drex] Step XX. Set the Phy in "ctrl_dll_on" to low */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[12], (1UL << 5));

	/* [Drex] Step XX. Wait for 10 PCLK Cycle */
	DMC_Delay(0xFFFF);

	/* [Drex] Step XX. Update DLL information */
	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

	DMC_Delay(0xFFF);

	/* Step 18 :  Send NOP Command. */
	send_directcmd(SDRAM_CMD_NOP, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_NOP, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

	// Step XX : Wait for minimum 200us
	DMC_Delay(100);

	// Step 19 : Send MR63 (Reset) Command.
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR63, 0);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR63, 0);
#endif

	// Step xx : Wait for minimum 10us
	int i;
	for (i = 0; i < 20000; i++) {
		send_directcmd(SDRAM_CMD_MRR, 0, 0, 0);				// 0x9 = MRR (mode register reading), MR0_Device Information
		if (mmio_read_32(&g_drex_reg->MRSTATUS) & (1 << 0))		// OP0 = DAI (Device Auto-Initialization Status)
			break;
	}
#if (LPDDR3_CS_NUM > 1)
	for (i = 0; i < 20000; i++) {
		send_directcmd(SDRAM_CMD_MRR, 1, 0, 0);				// 0x9 = MRR (mode register reading), MR0_Device Information
		if (mmio_read_32(&g_drex_reg->MRSTATUS) & (1 << 0))		// OP0 = DAI (Device Auto-Initialization Status)
			break;
	}
#endif

#if 1	// Old Code - unknown (by.deoks)
	// Step 20. Send MR10 Command - DRAM ZQ calibration
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR10, 0xFF);		// 0x0 = MRS/EMRS (mode register setting),  MR10_Calibration, 0xFF: Calibration command after initialization
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR10, 0xFF);
#endif
#endif
	// Step XX. Wait for minimum 1us (tZQINIT).
	DMC_Delay(267); // MIN 1us

	/* Step 19 :  Send MR2 Command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR2, MR2.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR2, MR2.Reg);
#endif

	/* Step 20 :  Send MR1 Command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR1, MR1.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR1, MR1.Reg);
#endif

	/* Step 21 :  Send MR3 Command. - I/O Configuration :: Drive Strength */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR3, MR3.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR3, MR3.Reg);
#endif

	/* Step 22 :  Send MR11 Command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR11, MR11.Reg);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR11, MR11.Reg);
#endif

	/* Step 23 :  Send MR16 (PASR) Bank Command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR16, 0xFF);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR16, 0xFF);
#endif

	/* Step 24 :  Send MR17 (PASR) Bank Command. */
	send_directcmd(SDRAM_CMD_MRS, 0, SDRAM_MODE_REG_MR17, 0xFF);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_MRS, 1, SDRAM_MODE_REG_MR17, 0xFF);
#endif
	DMC_Delay(100);

	/* [Drex]  Step XX. Set the Phy in "ctrl_offsetr 0 ~ 3" of value '0' */
	mmio_write_32(&g_ddrphy_reg->PHY_CON[4], 0x88888888);

	/* [Drex]  Step XX. Set the Phy in "ctrl_offsetw 0 ~ 3" of value '0'*/
	mmio_write_32(&g_ddrphy_reg->PHY_CON[6], 0x88888888);

	/* [Drex]  Step XX. Set the Phy in "ctrl_offsetd 0 ~ 3" of value '0'*/
	mmio_write_32(&g_ddrphy_reg->PHY_CON[10], 0x0);

	/* [Drex] Step XX. Enable the Phy in "ctrl_dll_on" */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[12], (1UL << 5));

	/* [Drex] Step XX. Wait for 10 PCLK Cycle */
	DMC_Delay(0xFFF);

	/* [Drex] Step XX. Set the Phy in "ctrl_start" value to '0' */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[12], (1UL << 6));
	/* [Drex] Step XX. Set the Phy in "ctrl_start" value to '1' */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[12], (1UL << 6));

	/* [Drex] Step XX. Wait until "dfi_init_complete" is set. (DLL lock will be processed.) */
	while ((mmio_read_32(&g_drex_reg->PHYSTATUS) & (0x1 << 3)) == 0);	// dfi_init_complete[3]. wait for DFI PHY initialization complete
	mmio_clear_32(&g_drex_reg->CONCONTROL, (0x1 << 28));			// dfi_init_start[28]. DFI PHY initialization clear

	/* [Drex] Step XX. Update DLL information */
	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

#if 1	/* Skip the following steps if Leveling and Training are not required. (Optional features) */
	MEMMSG("\r\n########## Leveling & Training ##########\r\n");

	/* Step 24. Generate "ctrl_gate_p*", ctrl_read_p* */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 <<   6));			// ctrl_atgate=1

	/* Step 25.  Issue Phase 0/1 Read Command during read leveling */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 <<  14));		// p0_cmd_en=1

	/* Step 26.  Initialize related logic before DQ Calibration */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[2], (0x1 <<   6));			// InitDeskewEn=1

	/* Step 27. Byte Leveling enable. */
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[0], (0x1 <<  13));		// byte_rdlvl_en=1

	/* Step 28. The time to determine the VMP(Vaild Margin Period) Read Training */
	temp  = mmio_read_32( &g_ddrphy_reg->PHY_CON[1]) & ~(0xF <<  16);	// rdlvl_pass_adj=4
	temp |= (0x4 <<  16);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[1], temp);

	temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[2]) & ~(0x7F << 16);	// rdlvl_incr_adj=1
	temp |= (0x1 << 16);
	mmio_write_32(&g_ddrphy_reg->PHY_CON[2], temp);

	/* Step 29-1. Set "ddr3_cmd=14'h105E" as default value (=PHY_CON[25][29:16]) */
	/* Step 29-2. Set "lpddr2_cmd=14'hE" as default value (=PHY_CON[25][13:0]) */

	temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[25 + 1])
		& ~((0x3FFF << 16) | (0x3FFF << 0));
	temp |= (0x105E << 16);						// ddr3_cmd= DDR3:0x105E
	temp |= (0x000E <<  0);						// lpddr2_cmd = LPDDDR2 or LPDDDR3:0x000E
	mmio_write_32(&g_ddrphy_reg->PHY_CON[25 + 1], temp);

	/* Step 29-3. Set "cmd_default=14'hE" as default value (=PHY_CON[25][13:0]) */
	temp = mmio_read_32 (&g_ddrphy_reg->PHY_CON[26 + 1]);
	temp &= ~(0x000F << 0);
	temp |= (0x000F << 0);
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[26 + 1], temp); 			// (DDR2, DDR3) : 107F, (LPDDR2, LPDDR3) : 0x000F

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
	g_ddr_lockvalue = (temp >> 8) & 0x1FF;

	/* Step 31-2. Update "ctrl_force[8:0]" in PHY_CON12[15:7] by the value of "ctrl_lock_value[8:0] */
	temp  = mmio_read_32(&g_ddrphy_reg->PHY_CON[12]);
	temp &= ~(0x7F <<  8);
	temp |= ((g_ddr_lockvalue >> 2) << 8);					// ctrl_force[14:8]
	mmio_write_32(&g_ddrphy_reg->PHY_CON[12], temp);

	/* Step XX. Set the Read ODT Disable (Disable the LPDDR3 unconditionally) */
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[0],  (0x1 << 5));			// ctrl_read_disable[5]= 1. Read ODT disable signal. Variable. Set to '1', when you need Read Leveling test.

	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

	/* Step 32. DDR Controller Calibration*/
	/* temporary code according to suspend/resume policy. */
//	if (is_resume == 0) {
	{
#if defined(DDR_CA_CALIBRATION_EN)
		/* Step 32 -1. CA Calibration */
		if (DDR3_LvlTr & LVLTR_CA_CAL) {
			if (ddr_ca_calibration() < 0)
				return -1;
		}
#endif

#if defined(DDR_GATE_LEVELING_EN)
		/*
		  * Step 32-2. Gate Leveling
		  * (It should be used only for DDR3 (800Mhz))
		  */
		if (DDR3_LvlTr & LVLTR_GT_LVL) {
			if (ddr_gate_leveling() < 0)
				return -1;
		}
#endif

#if defined(DDR_READ_DQ_CALIB_EN)
		/* Step 32-3. Read DQ Calibration */
		if (DDR3_LvlTr & LVLTR_RD_CAL) {
			if (ddr_read_dq_calibration() < 0)
				return -1;
		}
#endif

#if defined(DDR_WRITE_DQ_CALIB_EN)
		/* Step 32-5. Write DQ Calibration */
		if (DDR3_LvlTr & LVLTR_WR_CAL) {
			if (ddr_write_dq_calibration() < 0)
				return -1;
		}
#endif
	}
#endif	// Skip Training

	mmio_set_32  (&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization
	mmio_clear_32(&g_drex_reg->PHYCONTROL[0], (0x1 << 3));			// Force DLL Resyncronization

	/* Step 33. DLL turn on */
	mmio_set_32(&g_ddrphy_reg->PHY_CON[12], (0x1 << 5));			// ctrl_dll_on[12]=1
#if 1	// (Optional)
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
	mmio_set_32  (&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync=1
	mmio_clear_32(&g_ddrphy_reg->PHY_CON[10], (0x1 << 24));		// ctrl_resync=0

	/* Step 38. Send Precharge ALL command */
	send_directcmd(SDRAM_CMD_PALL, 0, (SDRAM_MODE_REG)CNULL, CNULL);
#if (LPDDR3_CS_NUM > 1)
	send_directcmd(SDRAM_CMD_PALL, 1, (SDRAM_MODE_REG)CNULL, CNULL);
#endif

#if 1	/* Step 39. Set the MemControl0 & PhyControl0 (Optional) */
	/* [Drex] Step 39-1. Set the Memory Control(MemControl)  (Optional) */

	/* [Drex] Step 39-2. Set the (Phy Control and Memory Control) */
	mmio_write_32(&g_drex_reg->PHYCONTROL[0],
#if (CFG_ODT_ENB == 1)
			(0x1  <<  31) |						// [   31] mem_term_en. Termination Enable for memory. Disable : 0, Enable : 1
			(0x1  <<  30) |						// [   30] phy_term_en. Termination Enable for PHY. Disable : 0, Enable : 1
			(0x0  <<   0) |						// [    0] mem_term_chips. Memory Termination between chips(2CS). Disable : 0, Enable : 1
#endif
			(0x0  <<  29) |						// [   29] ctrl_shgate. Duration of DQS Gating Signal. gate signal length <= 200MHz : 0, >200MHz : 1
			(0x0  <<  24) |						// [28:24] ctrl_pd. Input Gate for Power Down.
//			(0x0   <<   7) |						// [23: 7] reserved - SBZ
			(0x0  <<   4) |						// [ 6: 4] dqs_delay. Delay cycles for DQS cleaning. refer to DREX datasheet
			(0x0  <<   3) |						// [    3] fp_resync. Force DLL Resyncronization : 1. Test : 0x0
			(0x0  <<   1));						// [    1] sl_dll_dyn_con. Turn On PHY slave DLL dynamically. Disable : 0, Enable : 1
#endif

	/* [Drex] Step 40. Set the Controller Control */
	mmio_set_32(&g_drex_reg->CONCONTROL, (0x1 << 5));

	/* [Dphy] Step XX. Set the PhyControl0 */
	/* Clear the PhyCon0 (p0_cmd_en,  byte_rdlvl_en, ctrl_shgate) */
	temp = mmio_read_32(&g_ddrphy_reg->PHY_CON[0]);
	temp &= ~((0x1 << 14) | (0x1 << 13) | (0x1 << 8));
	mmio_write_32 (&g_ddrphy_reg->PHY_CON[0], temp);

	MEMMSG("DLL Lock Value  = %d\r\n", g_ddr_lockvalue);

	MEMMSG("CA CAL CODE = 0x%08X\r\n", mmio_read_32(&g_ddrphy_reg->PHY_CON[10]));
	MEMMSG("GATE CYC    = 0x%08X\r\n", mmio_read_32(&g_ddrphy_reg->PHY_CON[3]));
	MEMMSG("GATE CODE   = 0x%08X\r\n", mmio_read_32(&g_ddrphy_reg->PHY_CON[8]));
	MEMMSG("Read  DQ    = 0x%08X\r\n", mmio_read_32(&g_ddrphy_reg->PHY_CON[4]));
	MEMMSG("Write DQ    = 0x%08X\r\n", mmio_read_32(&g_ddrphy_reg->PHY_CON[6]));

	return 0;
}
