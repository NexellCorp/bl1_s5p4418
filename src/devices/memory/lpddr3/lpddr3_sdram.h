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
#ifndef __LPDDR3_SDRAM_H__
#define __LPDDR3_SDRAM_H__

#define RESP_MR41 0x5555
#define RESP_MR48 0x0101

#define MASK_MR41 0xFFFF
#define MASK_MR48 0x0303

struct lpddr3_init_info {
	unsigned char  chip_num;	// 0x88
	unsigned char  chip_row;	// 0x89
	unsigned char  bus_width;	// 0x8A
	unsigned char  chip_col;	// 0x8B

	unsigned short chip_mask;	// 0x8C
	unsigned short chip_size;	// 0x8E

	unsigned char  cwl;		// 0x90
	unsigned char  cl;		// 0x91
	unsigned char  mr1_al;		// 0x92
	unsigned char  mr0_wr;		// 0x93

	unsigned int read_delay;	// 0x94
	unsigned int write_delay;	// 0x98

	unsigned int timing_pzq;	// 0x9C
	unsigned int tiing_aref;	// 0xA0
	unsigned int timing_row;	// 0xA4
	unsigned int timing_data;	// 0xA8
	unsigned int timing_power;	// 0xAC
};

struct dram_device_info {
	unsigned char bank_num;
	unsigned char row_num;
	unsigned char column_num;

	unsigned int column_size;
	unsigned int row_size;
	unsigned long bank_size;
	unsigned short chip_size;
	unsigned long sdram_size;
};

typedef enum
{
	SDRAM_MODE_REG_MR0      = 0,
	SDRAM_MODE_REG_MR1      = 1,
	SDRAM_MODE_REG_MR2      = 2,
	SDRAM_MODE_REG_MR3      = 3,
	/* LPDDR3 Features */
	SDRAM_MODE_REG_MR10	= 10,
	SDRAM_MODE_REG_MR11	= 11,
	SDRAM_MODE_REG_MR16	= 16,
	SDRAM_MODE_REG_MR17	= 17,
	SDRAM_MODE_REG_MR32	= 32,
	SDRAM_MODE_REG_MR41	= 41,
	SDRAM_MODE_REG_MR42	= 42,
	SDRAM_MODE_REG_MR48	= 48,
	SDRAM_MODE_REG_MR63	= 63,
	SDRAM_MODE_REG_MAX_MRn  = 0xFF
} SDRAM_MODE_REG;

struct SDRAM_MR0 {
	volatile unsigned short BL		:2;     // 0 Burst Length	( 00: Fixed BL8, 01: 4 or 8 (A12), 10: Fixed BC4, 11: Reserved)
	volatile unsigned short CL0		:1;     // 2 CAS Latency 0 bit (0000: Reserved, 0010: 5, 0100: 6, 0110: 7, 1000: 8, 1010: 9, 1100:10, 1110:11, 0001:12, 0011:13
	volatile unsigned short BT		:1;     // 3 READ Burst Type (0: Sequential(nibble), 1: Interleaved)
	volatile unsigned short CL1		:3;     // 4 CAS Latency 0 bit
	volatile unsigned short SBZ0		:1;     // 7
	volatile unsigned short DLL		:1;     // 8 DLL Reset ( 0: No, 1: Yes)
	volatile unsigned short WR		:3;     // 9 Write Recovery (000: Reserved, 001: 5, 010: 6, 011: 7, 100: 8, 101: 10, 110: 12, 111: 14)
	volatile unsigned short PD		:1;     // 12 Precharge PD (0:DLL off (slow exit), 1:DLL on (fast exit)
	volatile unsigned short SBZ1		:3;     // 13 ~ 15
};
struct SDRAM_MR1 {
	volatile unsigned short DLL		:1;     // 0 DLL Enable (0: enable, 1: disable)
	volatile unsigned short ODS0		:1;     // 1 Output Drive Strenghth 0 bit (00: RZQ/6(40ohm), 01: RZQ/7(34ohm), 10, 11: reserved)
	volatile unsigned short RTT_Nom0	:1;     // 2
	volatile unsigned short AL		:2;     // 3 Additive Latency	(00: disable, 01: AL=CL-1, 10: AL=CL-2, 11: Reserved)
	volatile unsigned short ODS1		:1;     // 5 Output Drive Strenghth 1 bit
	volatile unsigned short RTT_Nom1	:1;     // 6
	volatile unsigned short WL		:1;     // 7 Write Levelization (0: disable, 1: enable)
	volatile unsigned short SBZ0		:1;     // 8
	volatile unsigned short RTT_Nom2	:1;     // 9 (000: disable, 001: RZQ/4(60ohm), 010: RZQ/2(120ohm), 011: RZQ/6(40ohm), 100: RZQ/12(20ohm), 101: RZQ/8(30ohm) 110, 111: Reserved)
	volatile unsigned short SBZ1		:1;     // 10
	volatile unsigned short TDQS		:1;     // 11 TDQS (0: disable, 1: enable)
	volatile unsigned short QOff		:1;     // 12 Q Off (0: enabled, 1: disabled)
	volatile unsigned short SBZ2		:3;     // 13 ~ 15
};

struct SDRAM_MR2 {
	volatile unsigned short SBZ0		:3;     // 0
	volatile unsigned short CWL		:3;     // 3 CAS Write Latency (000: 5CK(tCK>=2.5ns), 001: 6CK(2.5ns>tCK>=1.875ns, 010: 7CK(1.875ns>tCK>=1.25ns), 011: 8CK(1.5ns>tCK>=1.25ns), 100: 9CK(1.25ns>tCK>=1.07ns))
	volatile unsigned short ASR		:1;     // 6 Auto Self Refresh(option) (0: disabled:Manual, 1:Enabled: Automatic)
	volatile unsigned short SRT		:1;     // 7 Self Refresh Temperature (0: Normal(0 to 85 degree), 1: Extended(0 to 95 degree)
	volatile unsigned short SBZ1		:1;     // 8
	volatile unsigned short RTT_WR		:2;     // 9 Dynamic ODT (00: Rtt disabled, 01: RZQ/4, 10: RZQ/2, 11: reserved)
	volatile unsigned short SBZ2		:5;     // 11 ~ 15
};

struct SDRAM_MR3 {
	volatile unsigned short MPR_RF		:2;     // 0 MPR Read Function (00: Predefined pattern, 01, 10, 11: reserved)
	volatile unsigned short MPR		:1;     // 2 MPR Enable (0: Normal DRAM operation, 1: Dataflow from MPR)
	volatile unsigned short SBZ0		:13;    // 3 ~ 15
};

struct LPDDR3_MR0 {
	volatile unsigned short DAI		:1;     // 0 Device Auto-Initialization Status( 0: DAI still in progress)
	volatile unsigned short _RFU0		:2;     // 1 ~ 2
	volatile unsigned short RZQI		:2;     // 3 ~ 4 RZQ Information
	volatile unsigned short _RFU1		:1;     // 5
	volatile unsigned short WL		:1;     // 6
	volatile unsigned short RL3		:1;     // 7
	volatile unsigned short _reserved	:8;     // 8 ~ 15
};

struct LPDDR3_MR1 {
	volatile unsigned short BL		:3;     // 0 ~ 2
	volatile unsigned short _RFU0		:2;     // 3 ~ 4
	volatile unsigned short WR		:3;     // 5 ~ 7
	volatile unsigned short _reserved	:8;     // 8 ~ 15
};

struct LPDDR3_MR2 {
	volatile unsigned short RL_WL		:4;     // 0 ~ 3
	volatile unsigned short WRE		:1;     // 4
	volatile unsigned short _RFU0		:1;     // 5
	volatile unsigned short WL_SEL		:1;     // 6
	volatile unsigned short WR_LVL		:1;     // 7
	volatile unsigned short _reserved	:8;     // 8 ~ 15
};

struct LPDDR3_MR3 {
	volatile unsigned short DS		:4;     // 0 ~ 3
	volatile unsigned short _RFU0		:4;     // 4 ~ 7
	volatile unsigned short _reserved	:8;     // 8 ~ 15
};

struct LPDDR3_MR11 {
	volatile unsigned short DQ_ODT		:2;     // 0 ~ 1
	volatile unsigned short PD_CON		:1;     // 2
	volatile unsigned short _RFU0		:5;     // 3 ~ 7
	volatile unsigned short _reserved	:8;     // 8 ~ 15
};

union SDRAM_MR {
	struct SDRAM_MR0    MR0;
	struct SDRAM_MR1    MR1;
	struct SDRAM_MR2    MR2;
	struct SDRAM_MR3    MR3;

	struct LPDDR3_MR0   LP_MR0;
	struct LPDDR3_MR1   LP_MR1;
	struct LPDDR3_MR2   LP_MR2;
	struct LPDDR3_MR3   LP_MR3;
	struct LPDDR3_MR11  LP_MR11;

	volatile unsigned short Reg;
};

#endif // __LPDDR3_SDRAM_H__