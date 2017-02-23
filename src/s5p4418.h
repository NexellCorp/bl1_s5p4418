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

#endif
