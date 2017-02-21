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

#ifndef __LPDDR3_AC_TIMING_H__
#define __LPDDR3_AC_TIMING_H__

/* Support the LPDDR3 Clock*/
#define LPDDR3_MEMCLK_533MHZ	0
#define LPDDR3_MEMCLK_666MHZ	1
#define LPDDR3_MEMCLK_800MHZ	3

/* Support the DDR3 Memory Size */
#define LPDDR3_MEMSIZE_1GB	(0x40000000)
#define LPDDR3_MEMSIZE_512MB	(LPDDR3_MEMSIZE_1GB/2)
#define LPDDR3_MEMSIZE_256MB	(LPDDR3_MEMSIZE_512MB/2)

/* User Define - DDR3 Device Specifiacation */
#define LPDDR3_MEM_CLK		LPDDR3_MEMCLK_533MHZ

/* Refer to Memory(DREX, DPHY) Datasheet - Register Set*/
#define LPDDR3_CS_PERSIZE	((CONFIG_LPDDR3_CS_PERSIZE) >> 24)
#define LPDDR3_CS_PERMASK	(0x800 - LPDDR3_CS_PERSIZE)			// Capacity per nCS: 2G=0x780, 1G=0x7C0(Tabletx2, VTK)
#define LPDDR3_TOTAL_MEMSIZE	(CONFIG_LPDDR3_MEMSIZE)

#define LPDDR3_CS_NUM		(CONFIG_LPDDR3_CS_NUM)
#define LPDDR3_BUS_WIDTH	(CONFIG_LPDDR3_BUS_WIDTH)
#define LPDDR3_ROW_NUM		(CONFIG_LPDDR3_ROW_NUM - 12)			// ROW address bit : 15bit : 3(Tabletx2), 16bit : 4(Tabletx4, Elcomtech)
#define LPDDR3_COL_NUM		(CONFIG_LPDDR3_COLUMN_NUM -  7)			// Column Address Bit. 2:9bit, 3:10bit, others:Reserved
#define LPDDR3_BANK_NUM		(CONFIG_LPDDR3_BANK_NUM)					// Bank bit : 3:8bank, 2:4bank
#define LPDDR3_CS0_BASEADDR	(0x00)
#define LPDDR3_CS1_BASEADDR	(LPDDR3_CS0_BASEADDR + LPDDR3_CS_PERSIZE)

#define RDFETCH			0x1						// CONCONTROL.rd_fetch[14:12]

#define MR3_DS			CONFIG_DRAM_MR3_ODS
#define MR11_DQ_ODT		CONFIG_DRAM_MR11_DQ_ODT
#define MR11_PD_CTL		CONFIG_DRAM_MR11_PD_CTL

/* LPDDR3 AC Timing */
#if (LPDDR3_MEM_CLK == LPDDR3_MEMCLK_800MHZ)
#define nCWL			8						// CAS Write Latency(CWL).
#define nCL			(11 + 0)					// CAS Latency(CL). Sometimes plus is needed.

#define MR1_nWR			0xC
//#define MR2_RLWL		0xC
#define MR1_AL			0xC

#define tPZQ			0x8035
#define tREFI			0x30C

//#define tRFC			0x34
#define tRRD			0x4
#define tRP			0x9
#define tRCD			0x8
#define tRC			0x1A
#define tRAS			0x11

#define tWTR			0x3
#define tWR			0x6
#define tRTP			0x3
#define tPPD			0x0
#define W2W_C2C			0x1
#define R2R_C2C			0x1
#define tDQSCK			0x5
#define tWL			0x6
#define tRL			0xC

#define tFAW			0x14
#define tXSR			0x38
#define tXP			0x3
#define tCKE			0x3
#define tMRD			0x6

//#define tADR			0x4						// Micron (20ns)
#define tADR			0x3						// Samsung (15ns)
#define tWLO			(tADR)
#endif  //#if (LPDDR3_MEM_CLK == DDR3_MEMCLK_800MHZ)


#if (LPDDR3_MEM_CLK == LPDDR3_MEMCLK_666MHZ)
#define nCW			7						// CAS Write Latency(CWL).
#define nCL			(9 + 0)						// CAS Latency(CL). Sometimes plus is needed.

#define MR1_nWR			0xA
//#define MR2_RLWL		0xA
#define MR1_AL			0xA

#define tPZQ			0x8014
#define tREFI			0x28A

//#define tRFC			0x2C
#define tRRD			0x4
#define tRP			0x7
#define tRCD			0x6
#define tRC			0x15
#define tRAS			0xE

#define tWTR			0x3
#define tWR			0x5
#define tRTP			0x3
#define tPPD			0x0						// 0:LPDDR3-1600, 1:LPDDR3-1866/2133
#define W2W_C2C			0x1
#define R2R_C2C			0x1
#define tDQSCK			0x4						// LPDDR3 : 4
#define tWL			0x6
#define tRL			0xA

#define tFAW			0x11
#define tXSR			0x2F
#define tXP			0x3
#define tCKE			0x3
#define tMRD			0x5

//#define tADR			0x4						// Micron (20ns)
#define tADR			0x3						// Samsung (15ns)
#define tWLO			(tADR)
#endif  //#if (LPDDR3_MEM_CLK == DDR3_MEMCLK_666MHZ)

#if (LPDDR3_MEM_CLK == LPDDR3_MEMCLK_533MHZ)
#define nCWL			6						// CAS Write Latency(CWL).
#define nCL			(7 + 0)						// CAS Latency(CL). Sometimes plus is needed.

#define MR1_nWR			0x8
//#define MR2_RLWL		0x9
#define MR1_AL			0x9

#define tPZQ			0x8020
#define tREFI			0x208

//#define tRFC			0x38
#define tRRD			0x3
#define tRP			0x6
#define tRCD			0x5
#define tRC			0x12
#define tRAS			0xC

#define tWTR			0x2
#define tWR			0x4
#define tRTP			0x2
#define tPPD			0x0						// 0:LPDDR3-1600, 1:LPDDR3-1866/2133
#define W2W_C2C			0x1
#define R2R_C2C			0x1
#define tDQSCK			0x4						// LPDDR3 : 4
#define tWL			0x5
#define tRL			0x9

#define tFAW			0xE	// 0xE
#define tXSR			0x3B	//0x26
#define tXP			0x2
#define tCKE			0x2
#define tMRD			0x5

//#define tADR			0x3						// Micron (20ns)
#define tADR			0x3						// Samsung (15ns)
#define tWLO			(tADR)
#endif  //#if (LPDDR3_MEM_CLK == DDR3_MEMCLK_533MHZ)

/* Timing parameters that depend on memory size.*/
/* Criterion for this value is that the bus width is always 32 bits. */
#if   (LPDDR3_TOTAL_MEMSIZE == LPDDR3_MEMSIZE_1GB)
#define tRFC			0x38
#elif (LPDDR3_TOTAL_MEMSIZE == LPDDR3_MEMSIZE_512MB)
#define tRFC			0x26
#else
#define tRFC			0x38						// unkndown?
#endif

#define nWL             	(tWL)
#define nRL             	(tRL)

#endif  //#ifndef __DDR3_AC_TIMING_H__