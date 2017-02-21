#ifndef __DREX_H__
#define __DREX_H__

struct s5p4418_drex_qos {
	volatile unsigned int QOSCONTROL;
	volatile unsigned int _Reserved;
};

struct s5p4418_drex_actiming {
	volatile unsigned int TIMINGROW;					///< 0x034 : AC Timing for SDRAM Row
	volatile unsigned int TIMINGDATA;					///< 0x038 : AC Timing for SDRAM Data
	volatile unsigned int TIMINGPOWER;					///< 0x03C : AC Timing for Power Mode of SDRAM
};

struct s5p4418_drex_bp {
	volatile unsigned int BP_CONTROL;					///< 0x2x0 : Back Pressure Control
	volatile unsigned int BP_CONFIG_R;					///< 0x2x4 : Back Pressure Configuration for Read
	volatile unsigned int BP_CONFIG_W;					///< 0x2x8 : Back Pressure Configuration for Write
	volatile unsigned int _Reserved;					///< 0x2xC
};

struct  s5p4418_drex_sdram_reg {
	volatile unsigned int CONCONTROL;					// 0x00
	volatile unsigned int MEMCONTROL;					// 0x04
	volatile unsigned int MEMCONFIG[2];					// 0x08 ~ 0x0C
	volatile unsigned int DIRECTCMD;					// 0x10
	volatile unsigned int PRECHCONFIG;					// 0x14
	volatile unsigned int PHYCONTROL[4];					// 0x18 ~ 0x24
	volatile unsigned int PWRDNCONFIG;					// 0x28
	volatile unsigned int TIMINGPZQ;					// 0x2C
	volatile unsigned int TIMINGAREF;					// 0x30
	volatile unsigned int TIMINGROW;					// 0x34
	volatile unsigned int TIMINGDATA;					// 0x38
	volatile unsigned int TIMINGPOWER;					// 0x3C
	volatile unsigned int PHYSTATUS;					// 0x40
	volatile unsigned int PAD_0[1];						// 0x44
	volatile unsigned int CHIPSTATUS;					// 0x48
	volatile unsigned int PAD_1[2];						// 0x4C ~ 0x50
	volatile unsigned int MRSTATUS;						// 0x54
	volatile unsigned int PAD_2[2];						// 0x58 ~ 0x5C
	struct s5p4418_drex_qos QOSCONTROL[16];					// 0x60 ~ 0xDC
	volatile unsigned int PAD_19[5];					// 0xE0 ~ 0xF0

	volatile unsigned int WRTRA_CONFIG;					// 0xF4
	volatile unsigned int RDLVL_CONFIG;					// 0xF8
	volatile unsigned int PAD_20[1];					// 0xFC

	volatile unsigned int BRBRSVCONTROL;					// 0x100
	volatile unsigned int BRBRSVCONFIG;					// 0x104
	volatile unsigned int BRBQOSCONFIG;					// 0x108
	volatile unsigned int MEMBASECONFIG[2];					// 0x10C ~ 0x110
	volatile unsigned int PAD_21[3];					// 0x114 ~ 0x11C

	volatile unsigned int WRLVL_CONFIG[2];					// 0x120 ~ 0x124
	volatile unsigned int WRLVL_STATUS;					// 0x128
	volatile unsigned int PAD_22[9];					// 0x12C ~ 0x14C

	volatile unsigned int CTRL_IO_RDATA;					// 0x150
	volatile unsigned int PAD_23[3];					// 0x154 ~ 0x15C

	volatile unsigned int CACAL_CONFIG[2];					// 0x160 ~ 0x164
	volatile unsigned int CACAL_STATUS;					// 0x168
};

typedef enum {
	SDRAM_CMD_MRS		= 0x0,						// MRS/EMRS	mode register
	SDRAM_CMD_EMRS		= 0x0,
	SDRAM_CMD_PALL		= 0x1,						// all banks precharge
	SDRAM_CMD_PRE		= 0x2,						// per bank precharge
	SDRAM_CMD_REFS		= 0x4,						// self refresh
	SDRAM_CMD_REFA		= 0x5,						// auto refresh
	// don't use this command if pb_ref_en is enabled in MemControl register
	SDRAM_CMD_CKEL		= 0x6,						// active/precharge power down
	SDRAM_CMD_NOP		= 0x7,						// exit from active/precharge power down
	SDRAM_CMD_REFSX		= 0x8,						// exit from self refresh
	SDRAM_CMD_MRR		= 0x9,						// mode register reading
	SDRAM_CMD_ZQINIT	= 0xa,						// ZQ calibration init
	SDRAM_CMD_ZQOPER	= 0xb,						// ZQ calibration long
	SDRAM_CMD_ZQCS		= 0xc,						// ZQ calibration short
	SDRAM_CMD_SRR		= 0xd						// for Wide IO Memory
} SDRAM_CMD;

#endif	// #ifndef __DREX_H__
