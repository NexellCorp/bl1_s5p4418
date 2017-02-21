#ifndef __S5P4418_H__
#define __S5P4418_H__

/* NXPXX30 - Chip Product */
#define ARCH_S5P4418

/* Support the Kernel Version */
#if defined(KERNEL_VER_3_4)
#define SUPPORT_KERNEL_3_4			1
#else
#define SUPPORT_KERNEL_3_4			0
#endif

/* System Option */
#define CONFIG_BUS_RECONFIG			0
#define CONFIG_SUSPEND_RESUME			1
#define CONFIG_SET_MEM_TRANING_FROM_NSIH	1
#define CONFIG_MMU_ENABLE			0
#define CONFIG_CACHE_L2X0			0


/* DRAM(DDR3/LPDDR3) Memory Configuration */
#ifdef MEMTYPE_DDR3
#define MEM_TYPE_DDR3
#endif
#ifdef MEMTYPE_LPDDR3
#define MEM_TYPE_LPDDR23
#endif

/* Serial Console Configuration */
#define CONFIG_S5P_SERIAL
#define CONFIG_S5P_SERIAL_INDEX			0
#define CONFIG_S5P_SERIAL_CLOCK			50000000

#define CONFIG_S5P_SERIAL_SRCCLK		0
#define CONFIG_S5P_SERIAL_DIVID			4
#define CONFIG_S5P_SERIAL_

#define CONFIG_BAUDRATE				115200

#define CONFIG_UART_CLKGEN_CLOCK_HZ		0

#endif
