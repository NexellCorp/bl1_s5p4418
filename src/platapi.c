/*
 * Copyright (C) 2016	Nexell Co., Ltd. All Rights Reserved.
 * Nexell Co. Proprietary & Confidential
 *
 * NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 * AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * Author: Sangjong, Han <hans@nexell.co.kr>
 */

#define NULL	0
#define false	0
#define true	1

#include <nx_chip.h>
#include <nx_gpio.h>
#include <nx_clkpwr.h>
#include <nx_clkgen.h>
#include <nx-vip-primitive.h>

#include "printf.h"

void reset_con(unsigned int device_num, int enable);

static void udelay(unsigned int utime)
{
	register volatile unsigned int i;
	utime /= 21;
	if (utime == 0)
		utime = 1;
	for (i = 0; i < 53 * utime; i++);
}

enum devicephyindex {
	diGPIOA,
	diVIP0,
	diVIP0_CLKGEN,
	diVIP1,
	diVIP1_CLKGEN,
};
static unsigned int ioVIRBASEADDR[] = {
	0xC001A000,	// GPIOA
	0xC0063000,	// vip1
	0xC00C2000,	// vip1 clkgen
	0xC0064000,	// vip0
	0xC00C1000,	// vip0 clkgen
};

struct portinfo {
	unsigned char gpg;      // group
	unsigned char gpn;      // pad num
	unsigned char gpa;      // alt
	unsigned char gppen;    // pull en
	unsigned char gpud;     // 1: pullup, 0: pulldown
	unsigned char gpio;     // 1: input, 0: output
	unsigned char gpol;     // 0: low, 1: high
};
static struct portinfo dpcameractrlport[] = {
	// g group	,pad,al,pu,ud,io,lh
	{NX_GPIO_GROUP_E,  4, 0, 0, 0, 0, 1},	// viclk0
	{NX_GPIO_GROUP_E,  5, 0, 0, 0, 0, 1},	// vihsync0
	{NX_GPIO_GROUP_E,  6, 0, 0, 0, 0, 1},	// vivsync0

	{NX_GPIO_GROUP_A, 28, 0, 0, 0, 0, 1},	// viclk1
	{NX_GPIO_GROUP_E, 13, 0, 0, 0, 0, 1},	// vihsync1
	{NX_GPIO_GROUP_E,  7, 0, 0, 0, 0, 1},	// vivsync1
};


static int port_init(void)
{
	int i, cpn = sizeof(dpcameractrlport) / sizeof(struct portinfo);
	volatile struct NX_GPIO_RegisterSet * pgpa =
		(volatile struct NX_GPIO_RegisterSet *)ioVIRBASEADDR[diGPIOA];

	for (i = 0; i < cpn; i++) {
		struct portinfo *ppi = &dpcameractrlport[i];
		volatile struct NX_GPIO_RegisterSet * pgp = pgpa + ppi->gpg;

		pgp->GPIOxALTFN[ppi->gpn >> 4] &= ~(3 << ((ppi->gpn & 0xf) * 2));
		pgp->GPIOxALTFN[ppi->gpn >> 4] |= (ppi->gpa & 3) << ((ppi->gpn & 0xf) * 2);
		if (ppi->gpud) {
			pgp->GPIOx_PULLSEL |= 1 << ppi->gpn;	// pull up/down
			pgp->GPIOx_PULLSEL_DISABLE_DEFAULT |= 1 << ppi->gpn;	//
		} else
			pgp->GPIOx_PULLSEL &= ~(1 << ppi->gpn);	// pull up/down
		if (ppi->gppen) {
			pgp->GPIOx_PULLENB |= 1 << ppi->gpn;	// pull enable
			pgp->GPIOx_PULLENB_DISABLE_DEFAULT |= 1 << ppi->gpn;	// pull enable
		} else
			pgp->GPIOx_PULLENB &= ~(1 << ppi->gpn);	// pull enable
		pgp->GPIOx_DRV0 |= 1 << ppi->gpn;		// drv strength max
		pgp->GPIOx_DRV1 |= 1 << ppi->gpn;
		pgp->GPIOx_DRV0_DISABLE_DEFAULT |= 1 << ppi->gpn;
		pgp->GPIOx_DRV1_DISABLE_DEFAULT |= 1 << ppi->gpn;
		pgp->GPIOx_SLEW &= ~(1 << ppi->gpn);		// slew rate min
		pgp->GPIOx_SLEW_DISABLE_DEFAULT |= 1 << ppi->gpn;

		if (ppi->gpol)
			pgp->GPIOxOUT |= 1 << ppi->gpn;		// out value
		else
			pgp->GPIOxOUT &= ~(1 << ppi->gpn);	// out value
		if (!ppi->gpio & 1)
			pgp->GPIOxOUTENB |= 1 << ppi->gpn;	// in/out
		else
			pgp->GPIOxOUTENB &= ~(1 << ppi->gpn);	// in/out
	}

	return 1;
}

#define ALIGN(X, N)             ((X+N-1) & (~(N-1)))
#define YUV_STRIDE_ALIGN_FACTOR                 64
#define YUV_Y_STRIDE(w)         ALIGN(w, YUV_STRIDE_ALIGN_FACTOR)
#define YUV_CB_STRIDE(w)        ALIGN(w, YUV_STRIDE_ALIGN_FACTOR / 2)
#define YUV_CR_STRIDE(w)        ALIGN(w, YUV_STRIDE_ALIGN_FACTOR / 2)

enum FRAME_KIND {
	Y,
	CB,
	CR
};

static int stride_cal(int width, enum FRAME_KIND type)
{
	int stride;

	switch (type) {
	case Y:
		stride = YUV_Y_STRIDE(width);
		break;
	case CB:
		stride = YUV_CB_STRIDE(width / 2);
		break;
	case CR:
		stride = YUV_CR_STRIDE(width / 2);
		break;

	}
	return stride;
}

static void vip_init(void)
{
	nx_vip_set_base_address(0, (void*)ioVIRBASEADDR[diVIP0]);
	nx_vip_set_base_address(1, (void*)ioVIRBASEADDR[diVIP1]);
	struct NX_CLKGEN_RegisterSet *pvip0 =
		(struct NX_CLKGEN_RegisterSet *)ioVIRBASEADDR[diVIP0_CLKGEN];
	struct NX_CLKGEN_RegisterSet *pvip1 =
		(struct NX_CLKGEN_RegisterSet *)ioVIRBASEADDR[diVIP1_CLKGEN];
	pvip0->CLKENB = 1 << 2 | 3 << 0;
	pvip1->CLKENB = 1 << 2 | 3 << 0;

	udelay(10);

	reset_con(RESETINDEX_OF_VIP0_MODULE_i_nRST, 0);
	reset_con(RESETINDEX_OF_VIP1_MODULE_i_nRST, 0);
	nx_vip_clear_interrupt_pending_all(0);	// clear all pending int
	nx_vip_clear_interrupt_pending_all(1);

	nx_vip_set_interrupt_enable_all(0, 0);	// disable all vip int
	nx_vip_set_interrupt_enable_all(1, 0);

	nx_vip_set_input_port(0, 0);
	nx_vip_set_input_port(1, 0);
}

static void vip_set_capture(void)
{
	int width = 448, height = 1040, lu_stride, cb_stride;
	int fmt = nx_vip_dataorder_y0cby1cr, format = nx_vip_format_l422;

	nx_vip_clear_interrupt_pending_all(0);	// clear all pending int
	nx_vip_clear_interrupt_pending_all(1);

	nx_vip_set_interrupt_enable_all(0, 0);	// disable all vip int
	nx_vip_set_interrupt_enable_all(1, 0);

	nx_vip_set_input_port(0, 0);		// input at bt601
	nx_vip_set_input_port(1, 0);

	nx_vip_set_field_mode(0, false, nx_vip_fieldsel_bypass,
			0, false);	// field:0, no interlace, bypass
	nx_vip_set_field_mode(1, false, nx_vip_fieldsel_bypass,
			0, false);

	nx_vip_set_data_mode(0, fmt, 8);	// order: 2, width:8
	nx_vip_set_data_mode(1, fmt, 8);

	nx_vip_set_dvalid_mode(0, false, false, false);	// dvalid:0, bypass, ext sync
	nx_vip_set_dvalid_mode(1, false, false, false);

	nx_vip_set_hvsync(0,
			1,
			width * 2,
			height,
			0, 0, 0, 480,
			0, 0, 480, 0, 0);
	nx_vip_set_hvsync(1,
			1,
			width * 2,
			height,
			0, 0, 0, 480,
			0, 0, 480, 0, 0);

	nx_vip_set_fiforeset_mode(0, nx_vip_fiforeset_all);
	nx_vip_set_fiforeset_mode(1, nx_vip_fiforeset_all);
	nx_vip_set_clip_region(0, 0, 0, width, height);
	nx_vip_set_clip_region(1, 0, 0, width, height);

	nx_vip_set_clipper_format(0, format);	// l422
	nx_vip_set_clipper_format(1, format);

	lu_stride = stride_cal(width, Y);
	cb_stride = stride_cal(width, CB);

	if (format == nx_vip_format_l422)
		lu_stride *= 2;

	nx_vip_set_clipper_format(0, format);	// l422
	nx_vip_set_clipper_format(1, format);

	nx_vip_set_clipper_addr(0,
			format,
			width,
			height,
			0x40000000,
			0, 0, lu_stride, cb_stride);
	nx_vip_set_clipper_addr(1,
			format,
			width,
			height,
			0x40400000,
			0, 0, lu_stride, cb_stride);

	nx_vip_clear_interrupt_pending_all(0);
	nx_vip_clear_interrupt_pending_all(1);

	nx_vip_set_vipenable(0, true, true, true, false);
	nx_vip_set_vipenable(1, true, true, true, false);

	nx_vip_set_interrupt_enable(0, nx_vip_int_done, true);	// vsync int enable
	nx_vip_set_interrupt_enable(1, nx_vip_int_done, true);
}

static void vip_sync_gen(void)
{
	register volatile struct NX_GPIO_RegisterSet * pgpa =
		(volatile struct NX_GPIO_RegisterSet *)ioVIRBASEADDR[diGPIOA];
	register volatile struct NX_GPIO_RegisterSet * pgpe =
		(volatile struct NX_GPIO_RegisterSet *)(ioVIRBASEADDR[diGPIOA] + 4*0x1000);
	register unsigned int p, h, pclk, hsync, vsync, pclkh, hsynch, vsynch;

	/* vip 0 sync */
	pclk = pgpe->GPIOxOUT;
	hsync = pgpe->GPIOxOUT;
	vsync = pgpe->GPIOxOUT;
	pclkh = pgpe->GPIOxOUT | 1 << 4;
	hsynch = pgpe->GPIOxOUT | 1 << 5;
	vsynch = pgpe->GPIOxOUT | 1 << 6 | 1 << 5;

	pgpe->GPIOxOUT = vsynch;
		pgpe->GPIOxOUT = hsynch;
			pgpa->GPIOxOUT = pclkh;
			pgpa->GPIOxOUT = pclk;
		pgpe->GPIOxOUT = hsync;
	pgpe->GPIOxOUT = vsync;

	for (p = 0; p < 896 * 2; p++) {
		pgpe->GPIOxOUT = pclkh;
		pgpe->GPIOxOUT = pclk;
	}
	pgpe->GPIOxOUT = vsynch;
		pgpe->GPIOxOUT = hsynch;
			pgpe->GPIOxOUT = pclkh;
			pgpe->GPIOxOUT = pclk;
		pgpe->GPIOxOUT = hsync;
	pgpe->GPIOxOUT = vsync;

	/* vip 1 sync */
	pclk = pgpa->GPIOxOUT;
	hsync = pgpe->GPIOxOUT;
	vsync = pgpe->GPIOxOUT;
	pclkh = pgpa->GPIOxOUT | 1 << 28;
	hsynch = pgpe->GPIOxOUT | 1 << 13;
	vsynch = pgpe->GPIOxOUT | 1 << 7 | 1 << 13;

	pgpe->GPIOxOUT = vsynch;
		pgpe->GPIOxOUT = hsynch;
			pgpa->GPIOxOUT = pclkh;
			pgpa->GPIOxOUT = pclk;
		pgpe->GPIOxOUT = hsync;
	pgpe->GPIOxOUT = vsync;

	for (p = 0; p < 896 * 2; p++) {
		pgpa->GPIOxOUT = pclkh;
		pgpa->GPIOxOUT = pclk;
	}
	pgpe->GPIOxOUT = vsynch;
		pgpe->GPIOxOUT = hsynch;
			pgpa->GPIOxOUT = pclkh;
			pgpa->GPIOxOUT = pclk;
		pgpe->GPIOxOUT = hsync;
	pgpe->GPIOxOUT = vsync;
}

static void vip_capture(void)
{
	vip_set_capture();
	vip_sync_gen();
}

#include <nx_wdt.h>
static struct NX_WDT_RegisterSet *const pReg_WDT =
	(struct NX_WDT_RegisterSet * const)PHY_BASEADDR_WDT_MODULE;
static void wdt_reset(unsigned short ms)
{
	/* Enable watchdog to eliminate booting failuire */
	reset_con(RESETINDEX_OF_WDT_MODULE_PRESETn, 0);	// reset off
	udelay(10);
	reset_con(RESETINDEX_OF_WDT_MODULE_nPOR, 0);	// reset off

	pReg_WDT->WTCON =
			156 << 8 |	// prescaler value
			0x03 << 3 |	// division factor (3:128)
			0x01 << 2;	// watchdog reset enable
	// 200MHz/157/128 = 9952.229229, 100.48us, ms*10*100us
	pReg_WDT->WTCNT = ms * 10;
	pReg_WDT->WTCON =
			156 << 8 |	// prescaler value
			0x01 << 5 |	// watchdog timer enable
			0x03 << 3 |	// division factor (3:128)
			0x01 << 2;	// watchdog reset enable
}

static void plat_init(void)
{
	port_init();
	vip_init();
}

void capture_emul(void)
{
	plat_init();
	vip_capture();
	wdt_reset(10);
}
