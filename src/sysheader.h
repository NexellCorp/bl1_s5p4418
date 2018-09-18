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
#ifndef __SYS_HEADER_H__
#define __SYS_HEADER_H__

#include <s5p4418.h>

#include <nx_pyrope.h>
#include <nx_type.h>
#include <nx_chip.h>
#include <nx_rstcon.h>

#include <nx_clkpwr.h>
#include <nx_ecid.h>
#include <nx_gpio.h>
#include <nx_alive.h>
#include <nx_rtc.h>
#include <nx_tieoff.h>
#include <nx_intc.h>
#include <nx_clkgen.h>
#include <nx_ssp.h>
#include <nx_uart.h>
#include <nx_wdt.h>

#include "bl1.h"
#include <printf.h>
#include <serial.h>
#include <type.h>
#include <common.h>

#include <clock.h>
#include <clkgen.h>
#include <clkpwr.h>
#include <resetcon.h>

#include <gpio.h>
#include <memory.h>
#include <pmic.h>
#include <plat_pm.h>

#include <libstd.h>

#include <memory.h>

#include <serial.h>
#include <printf.h>

#if defined(CHIPID_NXP4330)
#if defined(LEPUS)
#include <nxp4330_lepus.h>
#elif defined(NAVI)
#include <nxp4330_navi.h>
#elif defined(SMART_VOICE)
#include <nxp4330_smart_voice.h>
#elif defined(ZH_DRAGON)
#include <nxp4330_zh_dragon.h>
#elif defined(ZH_BLACKDRAGON)
#include <nxp4330_zh_blackdragon.h>
#elif defined(DAUDIO)
#include <nxp4330_daudio.h>
#elif defined(FF_VOICE)
#include <nxp4330_ff_voice.h>
#elif defined(CLOVA)
#include <nxp4330_clova.h>
#elif defined(SVM)
#include <nxp4330_svm.h>
#elif defined(CLUSTER)
#include <nxp4330_cluster.h>
#elif defined(ALLO_DISPLAY)
#include <nxp4330_allo_display.h>
#elif defined(CONVERGENCE_SVMC)
#include <nxp4330_convergence_svmc.h>
#elif defined(CON_SVMA)
#include <nxp4330_con_svma.h>
#endif
#endif

#if defined(CHIPID_S5P4418)
#if defined(AVN)
#include <s5p4418_avn_ref.h>
#elif defined(DRONE)
#include <s5p4418_drone.h>
#elif defined(SVT)
#include <s5p4418_svt.h>
#elif defined(RAPTOR)
#include <s5p4418_raptor.h>
#elif defined(ALLO_DISPLAY)
#include <nxp4330_allo_display.h>
#else
#include <s5p4418_general.h>
#endif
#endif

#if defined(SYSLOG_ON)
#define SYSMSG	printf
#else
#define SYSMSG	empty_printf
#endif

// Memory debug message
#if defined(SYSLOG_ON)
#define MEMMSG  printf
#else
#define MEMMSG	empty_printf
#endif

// UserDebug Message
#if 0
#define DBGOUT  printf
#else
#define DBGOUT	empty_printf
#endif

#define LOG_LEVEL			30

#define LOG_LEVEL_NONE			0
#define LOG_LEVEL_ERROR			10
#define LOG_LEVEL_NOTICE		20
#define LOG_LEVEL_WARNING		30
#define LOG_LEVEL_INFO			40
#define LOG_LEVEL_VERBOSE		50


#if ((LOG_LEVEL >= LOG_LEVEL_NOTICE) && defined(SYSLOG_ON))
# define NOTICE(...)	printf("NOTICE:  " __VA_ARGS__)
#else
# define NOTICE(...)	empty_printf("NOTICE:  " __VA_ARGS__)
#endif

#if ((LOG_LEVEL >= LOG_LEVEL_ERROR) && defined(SYSLOG_ON))
# define ERROR(...)	printf("ERROR:   " __VA_ARGS__)
#else
# define ERROR(...)	empty_printf("ERROR:   " __VA_ARGS__)
#endif

#if ((LOG_LEVEL >= LOG_LEVEL_WARNING) && defined(SYSLOG_ON))
# define WARN(...)	printf("WARNING: " __VA_ARGS__)
#else
# define WARN(...)	empty_printf("WARNING: " __VA_ARGS__)
#endif

#if ((LOG_LEVEL >= LOG_LEVEL_INFO) && defined(SYSLOG_ON))
# define INFO(...)	printf("INFO:    " __VA_ARGS__)
#else
# define INFO(...)	empty_printf("INFO:    " __VA_ARGS__)
#endif

#if ((LOG_LEVEL >= LOG_LEVEL_VERBOSE) && defined(SYSLOG_ON))
# define VERBOSE(...)	printf("VERBOSE: " __VA_ARGS__)
#else
# define VERBOSE(...)	empty_printf("VERBOSE: " __VA_ARGS__)
#endif

#define BL1_SDRAMBOOT_LOADADDR			(0xFFFF0000)
#define BL1_SDMMCBOOT_DEVADDR			(0x200)
#define BL1_SDMMCBOOT_LOADSIZE			(16*1024)
#define SRAM_MAXSIZE				(32*1024)
#define BL1_STACKSIZE				(3072)

#define BL2_LOADADDR				0xB0FE0000

#define __section(S)	__attribute__ ((__section__(#S)))
#define __init		__section(.init.text)
#define __initdata	__section(.init.data)
#define __initcode	__section(.init.code)

//------------------------------------------------------------------------------
//  Set global variables
//------------------------------------------------------------------------------

#if defined(__SET_GLOBAL_VARIABLES)

struct sbi_header *const __initdata psbi =
    (struct sbi_header * const)BASEADDR_SRAM;
struct sbi_header *const __initdata ptbi =
    (struct sbi_header * const)BASEADDR_SRAM;
struct NX_GPIO_RegisterSet (*const __initdata pReg_GPIO)[1] =
    (struct NX_GPIO_RegisterSet (*const)[])PHY_BASEADDR_GPIOA_MODULE;
struct NX_ALIVE_RegisterSet *const pReg_Alive =
    (struct NX_ALIVE_RegisterSet * const)PHY_BASEADDR_ALIVE_MODULE;
struct NX_TIEOFF_RegisterSet *const __initdata pReg_Tieoff =
    (struct NX_TIEOFF_RegisterSet * const)PHY_BASEADDR_TIEOFF_MODULE;
struct NX_ECID_RegisterSet *const pReg_ECID =
    (struct NX_ECID_RegisterSet * const)PHY_BASEADDR_ECID_MODULE;
struct NX_CLKPWR_RegisterSet *const __initdata pReg_ClkPwr =
    (struct NX_CLKPWR_RegisterSet * const)PHY_BASEADDR_CLKPWR_MODULE;
struct NX_RSTCON_RegisterSet *const __initdata pReg_RstCon =
    (struct NX_RSTCON_RegisterSet * const)PHY_BASEADDR_RSTCON_MODULE;
struct NX_DREXSDRAM_RegisterSet *const pReg_Drex =
    (struct NX_DREXSDRAM_RegisterSet * const)PHY_BASEADDR_DREX_MODULE_CH0_APB;
struct NX_DDRPHY_RegisterSet *const pReg_DDRPHY =
    (struct NX_DDRPHY_RegisterSet * const)PHY_BASEADDR_DREX_MODULE_CH1_APB;
struct NX_RTC_RegisterSet *const pReg_RTC =
    (struct NX_RTC_RegisterSet * const)PHY_BASEADDR_RTC_MODULE;

struct NX_WDT_RegisterSet *const pReg_WDT =
    (struct NX_WDT_RegisterSet * const)PHY_BASEADDR_WDT_MODULE;

#else

extern struct sbi_header *const __initdata psbi; // second boot info
extern struct sbi_header *const __initdata ptbi; // third boot info
extern struct NX_GPIO_RegisterSet (*const pReg_GPIO)[1];
extern struct NX_ALIVE_RegisterSet *const pReg_Alive;
extern struct NX_TIEOFF_RegisterSet *const __initdata pReg_Tieoff;
extern struct NX_ECID_RegisterSet *const pReg_ECID;
extern struct NX_CLKPWR_RegisterSet *const __initdata pReg_ClkPwr;
extern struct NX_RSTCON_RegisterSet *const __initdata pReg_RstCon;
extern struct NX_DREXSDRAM_RegisterSet *const pReg_Drex;
extern struct NX_DDRPHY_RegisterSet *const pReg_DDRPHY;
extern struct NX_RTC_RegisterSet *const pReg_RTC;
#endif

#endif //	__SYS_HEADER_H__
