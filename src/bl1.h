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
#ifndef __BL1_H__
#define __BL1_H__

#include <s5p4418.h>
#include <type.h>

#define HEADER_ID       ((((U32)'N')<< 0) | (((U32)'S')<< 8) | (((U32)'I')<<16) | (((U32)'H')<<24))

#define LVLTR_WR_LVL    (1 << 0)
#define LVLTR_CA_CAL    (1 << 1)
#define LVLTR_GT_LVL    (1 << 2)
#define LVLTR_RD_CAL    (1 << 3)
#define LVLTR_WR_CAL    (1 << 4)

enum {
	BOOT_FROM_USB   = 0UL,
	BOOT_FROM_SPI   = 1UL,
	BOOT_FROM_NAND  = 2UL,
	BOOT_FROM_SDMMC = 3UL,
	BOOT_FROM_SDFS  = 4UL,
	BOOT_FROM_UART  = 5UL
};

enum {
	ROMBOOT_FROM_SDFS = 1,
	ROMBOOT_FROM_USBH = 2,
	ROMBOOT_FROM_UART = 3,
	ROMBOOT_FROM_SPI  = 4,
	ROMBOOT_FROM_MMC  = 5,
	ROMBOOT_FROM_USB  = 6,
	ROMBOOT_FROM_NAND = 7
};

struct nx_nandboot_info
{
	uint8_t  addrstep;
	uint8_t  tcos;
	uint8_t  tacc;
	uint8_t  toch;
#if 0
	uint32_t pagesize    :24;    // 1 byte unit
	uint32_t loaddevice  :8;
#else
	uint8_t  pagesize;		// 512bytes unit
	uint8_t  tioffset;		// 3rd boot Image copy Offset. 1MB unit.
	uint8_t  copycount;		// 3rd boot image copy count
	uint8_t  loaddevice;		// device chip select number
#endif
	uint32_t crc32;
};

struct nx_spiboot_info
{
	uint8_t  addrstep;
	uint8_t  _reserved0[2];
	uint8_t  portnumber;

	uint32_t _reserved1  : 24;
	uint32_t loaddevice  : 8;

	uint32_t crc32;
};

struct nx_uartboot_info
{
	uint32_t _reserved0;

	uint32_t _reserved1  : 24;
	uint32_t loaddevice  : 8;

	uint32_t crc32;
};

struct nx_sdmmcboot_info
{
#if 1
	uint8_t  portnumber;
	uint8_t  _reserved0[3];
#else
	uint8_t  _reserved0[3];
	uint8_t  portnumber;
#endif

	uint32_t _reserved1  : 24;
	uint32_t loaddevice  : 8;

	uint32_t crc32;
};

struct nx_ddr3dev_drvds_info
{
	uint8_t  mr2_rtt_wr;
	uint8_t  mr1_ods;
	uint8_t  mr1_rtt_nom;
	uint8_t  _reserved0;
};

struct nx_lpddr3dev_drvds_info
{
	uint8_t  mr3_ds      : 4;
	uint8_t  mr11_dq_odt : 2;
	uint8_t  mr11_pd_con : 1;
	uint8_t  _reserved0  : 1;

	uint8_t  _reserved1;
	uint8_t  _reserved2;
	uint8_t  _reserved3;
};

struct nx_ddrphy_drvds_info
{
	uint8_t  drvds_byte0;		// data slice 0
	uint8_t  drvds_byte1;		// data slice 1
	uint8_t  drvds_byte2;		// data slice 2
	uint8_t  drvds_byte3;		// data slice 3

	uint8_t  drvds_ck;		// ck
	uint8_t  drvds_cke;		// cke
	uint8_t  drvds_cs;		// cs
	uint8_t  drvds_ca;		// ca[9:0], ras, cas, wen, odt[1:0], reset, bank[2:0]

	uint8_t  zq_dds;		// zq mode driver strength selection.
	uint8_t  zq_odt;
	uint8_t  _reserved0[2];
};

struct nx_sdfsboot_info
{
	char file_name[12];		// 8.3 format ex)"NXDATA.TBL"
};

union nx_deviceboot_info
{
	struct nx_nandboot_info  nandbi;
	struct nx_spiboot_info   spibi;
	struct nx_sdmmcboot_info sdmmcbi;
	struct nx_sdfsboot_info  sdfsbi;
	struct nx_uartboot_info  uartbi;
};

struct nx_ddrinit_info
{
	uint8_t  chipnum;		// 0x88
	uint8_t  chiprow;		// 0x89
	uint8_t  buswidth;		// 0x8A
	uint8_t  chipcol;		// 0x8B

	uint16_t chipmask;		// 0x8C
	uint16_t chipbase;		// 0x8E

#if 0
	uint8_t  cwl;			// 0x90
	uint8_t  wl;			// 0x91
	uint8_t  rl;			// 0x92
	uint8_t  ddrrl;			// 0x93
#else
	uint8_t  cwl;			// 0x90
	uint8_t  cl;			// 0x91
	uint8_t  mr1_al;		// 0x92, MR2_RLWL (LPDDR3)
	uint8_t  mr0_wr;		// 0x93, MR1_NWR (LPDDR3)
#endif

	uint32_t readdelay;		// 0x94
	uint32_t writedelay;		// 0x98

	uint32_t timingpzq;		// 0x9C
	uint32_t timingaref;		// 0xA0
	uint32_t timingrow;		// 0xA4
	uint32_t timingdata;		// 0xA8
	uint32_t timingpower;		// 0xAC
};

struct sbi_header
{
	uint32_t vector[8];		// 0x000 ~ 0x01C
	uint32_t vector_rel[8];		// 0x020 ~ 0x03C

	uint32_t device_addr;		// 0x040

	uint32_t load_size;		// 0x044
	uint32_t load_addr;		// 0x048
	uint32_t launch_addr;		// 0x04C
	union nx_deviceboot_info dbi;	// 0x050~0x058

	uint32_t pll[4];		// 0x05C ~ 0x068
	uint32_t pll_spread[2];		// 0x06C ~ 0x070

#if defined(ARCH_NXP4330) || defined(ARCH_S5P4418)

	uint32_t dvo[5];		// 0x074 ~ 0x084

	struct nx_ddrinit_info dii;	// 0x088 ~ 0x0AC

#if defined(MEM_TYPE_DDR3)
	struct nx_ddr3dev_drvds_info	ddr3_dsinfo;	// 0x0B0
#endif
#if defined(MEM_TYPE_LPDDR23)
	struct nx_lpddr3dev_drvds_info	lpddr3_dsinfo;	// 0x0B0
#endif
	struct nx_ddrphy_drvds_info	phy_dsinfo;	// 0x0B4 ~ 0x0BC

	uint16_t lvltr_mode;		// 0x0C0 ~ 0x0C1
	uint16_t flyby_mode;		// 0x0C2 ~ 0x0C3

	uint32_t bl2_start;		// 0x0C4
	uint32_t gatecycle;		// 0x0C8
	uint32_t gatecode;		// 0x0CC
	uint32_t rdvwmc;		// 0x0D0
	uint32_t wrvwmc;		// 0x0D4
	uint32_t entrypoint;		// 0x0D8

	uint32_t stub[(0x1ec-0x0dc)/4];	// 0x0DC ~ 0x1EC
#endif

	uint32_t memtestaddr;		// 0x1EC
	uint32_t memtestsize;		// 0x1F0
	uint32_t memtesttrycount;	// 0x1F4

	uint32_t build_info;		// 0x1F8

	uint32_t signature;		// 0x1FC    "NSIH"
} __attribute__ ((packed,aligned(4)));

// [0] : Use ICache
// [1] : Change PLL
// [2] : Decrypt
// [3] : Suspend Check

#endif //__BL1_H__
