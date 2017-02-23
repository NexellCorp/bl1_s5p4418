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
#include <sysheader.h>
#include <gic.h>

/* External Function */
#if (CONFIG_SUSPEND_RESUME == 1)
extern void enter_self_refresh(void);
extern unsigned int __calc_crc(void *addr, int len);
extern void DMC_Delay(int milisecond);
#endif

/* External Variable */
extern volatile int g_subcpu_num;
extern volatile int g_cpu_kill_num;
#if (CONFIG_SUSPEND_RESUME == 1)
extern unsigned int  g_GateCycle;
extern unsigned int  g_GateCode;
extern unsigned int  g_RDvwmc;
extern unsigned int  g_WRvwmc;
#endif

static void smp_enable(int enable)
{
	int value;

	value = arm9_get_auxctrl();
	value &= ~(1 << 6);

	if (enable)
		value |= (1 << 6);

	arm9_set_auxctrl(value);
}

static void dmb(void)
{
	__asm__ __volatile__ ("dmb");
}

#if 0
/*
 * Must be S5P44418
 * For CPU Power Off, STANDBY_WFI[n]
 * the signal must wait until be High.
 */
static int core_get_standby_wfi(unsigned int cpu_id)
{
	cpu_id = cpu_id;

	return 0;
}
#endif

/*
  * Optionally available function S5P4418
  * This function is that the CORE Active/Power Down state,
  * it is not known for sure.
  * But It is that you can see at least try.
  * Return Parameter : 0: ON, 1:OFF, 2:PENDING
  */
int s5p4418_cpu_check(unsigned int cpu_id)
{
	return (mmio_read_32(&pReg_Tieoff->TIEOFFREG[0])
			>> (cpu_id + 6)) & 0x1;
}

/*************************************************************
 * Must be S5P4418
 * CPU Power On sequence in S5P4418
 * A sequence that satisfies both the Power Off
 * and Hotplug.
 * must go through the following steps:
 *
 * Step 01. CPUx Block Reset Assert
 * Step 02. CPUx Power Active (Gating Cells)
 * Step 03. Have to wait more than 5us(tPC).
 * Step 04. CPUCLKOFF Set to 0 except CPU0
 * Step 05. Have to wait more than 15Cycle (tRC1).
 * Step 06. CPUCLKOFF Set to 1 except CPU0
 * Step 07. Have to wait more than 10us(tCR).
 * Step 08. CPUx Block Reset Negate
 * Step 09. Set to (CPUx) Clmap Signal Low.
 * Step 10. Have to wait more than 20us(tRC2).
 * Step 11. CPUx Power On (CPUCLKOFF Set to 0)
 *************************************************************/

int s5p4418_cpu_on(unsigned int cpu_id)
{
	/* Secondary CPU Wakeup Start Point (High Vector) */
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpu_id) << 18));

	/* Step 01. CPUx Block Reset Assert */
	reset_con(cpu_id, CTRUE);
	dmb();

	/* Step 02. CPUx Power Active (Power Gating Cells)  (1: Power Down, 0:Active) */
	mmio_clear_32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpu_id) << 6));
	dmb();

	/* Step 03 Waiting for 5us(tPC) */
	cache_delay_ms(0x2000);
	dmb();

	/* Step 04 CPUCLKOFF Set to 0 except CPU0 */
	mmio_clear_32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));
	dmb();

	/* Step 05 Waiting for 15Cycle(tRC1) */
	cache_delay_ms(0x2000);
	dmb();

	/* Step 06. CPUCLKOFF Set to 1 except CPU0 */
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));
	dmb();

	/* Step 07. Waiting for 10us (tCR) */
	cache_delay_ms(0x2000);
	dmb();

	/* Step 08. CPUx Block Reset Negate */
	reset_con(cpu_id, CFALSE);
	dmb();

	/* Step 09. Set to (CPUx) Clamp Signal Low */
	mmio_clear_32(&pReg_Tieoff->TIEOFFREG[0], (1 << (cpu_id + 1)));
	dmb();

	/* Step 10. Waiting for 20us (tRC2) */
	cache_delay_ms(0xFFFF);
	dmb();

	/* Step 11. CPUCLKOFF Set to 0 except CPU0 */
	mmio_clear_32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));
	dmb();

	return 0;		// return - 0: Success
}

void s5p4418_cpu_off_wfi_ready(void)
{
	set_secure_mode();

 	/* CPUx Interface Control - Group 0, 1 OFF */
	mmio_clear_32((void*)gicc_get_baseaddr(), (3 << 0));

	/* All Function Clean & SMP Detach */
	smp_enable(0);

	set_nonsecure_mode();
}

/*************************************************************
 * Must be S5P4418
 * CPU Power Off sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. Set the Clam Signal (High)
 * Step 02. Have to wait more than 20us.
 * Step 03. Waiting for the Standard WFI Signal Gating.
 * Step 04. Set the CPUx Power Down
 * Step 05. Reset to the CPUx Block.
 *************************************************************/
int s5p4418_cpu_off(unsigned int cpu_id)
{
	int ret = 1;

	/* Step 01. Check to Core Standard WFI Signal */
#if 0
	while(!core_get_standby_wfi(cpu_id));
#else
	cache_delay_ms(0xFFFFF);
#endif
	dmb();

	/* Step XX. Waiting for tCC Paramter */
	cache_delay_ms(0xFFF);
	dmb();

	/* Step 02. Set to (CPUx) Clamp Signal High */
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[0], (1 << (cpu_id + 1)));
	dmb();

	/* Step 03. Waiting for 20us */
	cache_delay_ms(0xFFFF);
	dmb();

	/* Step 04. Waiting for the Standard WFI Signal Gating. */
#if 0
	while(core_get_standby_wfi(cpu_id));
#else
	cache_delay_ms(0xFFFF);
#endif
	dmb();

	/* Step 05. CPUx Power DeActive (Power Gating Cells)  (1: Power Down, 0:Active) */
	mmio_set_32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpu_id) << 6));
	dmb();

	return ret;		// 0: ON, 1:OFF, 2:PENDING
}

/*************************************************************
 * s5p4418 system reset (method: power control)
 *************************************************************/
void s5p4418_reset_cpu(void)
{
	void *base = (void *)PHY_BASEADDR_CLKPWR_MODULE;
	const unsigned int sw_rst_enb_bitpos = 3;
	const unsigned int sw_rst_enb_mask = 1 << sw_rst_enb_bitpos;
	const unsigned int sw_rst_bitpos = 12;
	const unsigned int sw_rst_mask = 1 << sw_rst_bitpos;
	int pwrcont = 0x224;
	int pwrmode = 0x228;
	unsigned int reg;

	reg = mmio_read_32((void *)(base + pwrcont));

	reg &= ~sw_rst_enb_mask;
	reg |= 1 << sw_rst_enb_bitpos;

	mmio_write_32((void *)(base + pwrcont), reg);
	mmio_write_32((void *)(base + pwrmode), sw_rst_mask);
}

#if (CONFIG_SUSPEND_RESUME == 1)
extern U32  g_GateCycle;
extern U32  g_GateCode;
extern U32  g_RDvwmc;
extern U32  g_WRvwmc;

extern void enter_self_refresh(void);
extern U32 calc_crc(void *addr, int len);
extern void DMC_Delay(int milisecond);

int s5p4418_resume_check(void)
{
	int signature;
	int is_resume = 0;		// 0: no resume, 1:resume

	/* Get resume information. */
	signature = mmio_read_32(&pReg_Alive->ALIVESCRATCHREADREG);
	if ((SUSPEND_SIGNATURE == signature) &&
			mmio_read_32(&pReg_Alive->WAKEUPSTATUS)) {
		is_resume = 1;
	}

	return is_resume;
}

void s5p4418_resume(void)
{
	unsigned int kernel_addr, signature, mem, ref_crc, len;
	void (*jumpkernel)(void) = 0;

	mmio_write_32(&pReg_Alive->ALIVEPWRGATEREG, 1); 		// open alive power gate
	signature   = mmio_read_32(&pReg_Alive->ALIVESCRATCHREADREG);
	kernel_addr = mmio_read_32(&pReg_Alive->ALIVESCRATCHVALUE1);
	ref_crc     = mmio_read_32(&pReg_Alive->ALIVESCRATCHVALUE2);
	mem = mmio_read_32(&pReg_Alive->ALIVESCRATCHVALUE3);
	len = mmio_read_32(&pReg_Alive->ALIVESCRATCHVALUE4);
	jumpkernel = (void (*)(void))kernel_addr;

	mmio_write_32(&pReg_Alive->ALIVESCRATCHRSTREG, 0xFFFFFFFF);
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST1, 0xFFFFFFFF);
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST2, 0xFFFFFFFF);
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST3, 0xFFFFFFFF);
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST4, 0xFFFFFFFF);

	if (SUSPEND_SIGNATURE == signature) {
		unsigned int crc = calc_crc((void *)mem, len);
		NOTICE("Reference CRC : 0x%08X, Calcurated CRC : 0x%08X \r\n", ref_crc, crc);
		if (kernel_addr && (ref_crc == crc)) {
			NOTICE("It's WARM BOOT\r\nJump to Kernel!\r\n");
			NOTICE("Kernel Address : %08X(%08X) \r\n", jumpkernel, kernel_addr );
			while(!serial_done());
			jumpkernel();
		}
	} else {
		WARN("Suspend Signature is different\r\nRead Signature :0x%08X\r\n", signature);
	}

	NOTICE("It's COLD BOOT\r\n");
}

static void suspend_vdd_pwroff(void)
{
	mmio_clear_32(&pReg_ClkPwr->PWRCONT, (0xFF << 8));			// Clear USE_WFI & USE_WFE bits for STOP mode.
	mmio_write_32(&pReg_Alive->ALIVEPWRGATEREG, 0x00000001);		// alive power gate open

#if 1
	/* Save leveling & training values.*/

	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST5,    0xFFFFFFFF);		// clear - ctrl_shiftc
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST6,    0xFFFFFFFF);		// clear - ctrl_offsetC
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST7,    0xFFFFFFFF);		// clear - ctrl_offsetr
	mmio_write_32(&pReg_Alive->ALIVESCRATCHRST8,    0xFFFFFFFF);		// clear - ctrl_offsetw

	mmio_write_32(&pReg_Alive->ALIVESCRATCHSET5,    g_GateCycle);		// store - ctrl_shiftc
	mmio_write_32(&pReg_Alive->ALIVESCRATCHSET6,    g_GateCode);		// store - ctrl_offsetc
	mmio_write_32(&pReg_Alive->ALIVESCRATCHSET7,    g_RDvwmc);		// store - ctrl_offsetr
	mmio_write_32(&pReg_Alive->ALIVESCRATCHSET8,    g_WRvwmc);		// store - ctrl_offsetw
#endif
	mmio_write_32(&pReg_Alive->VDDOFFCNTVALUERST,  0xFFFFFFFF);		// clear delay counter, refrence rtc clock
	mmio_write_32(&pReg_Alive->VDDOFFCNTVALUESET,  0x00000001);		// set minimum delay time for VDDPWRON pin. 1 cycle per 32.768Kh (about 30us)

	__asm__ __volatile__ ("cpsid i");					// core interrupt off.
	mmio_write_32(&pReg_Alive->VDDCTRLSETREG,      0x000003FC);		// Retention off (Pad hold off)
	mmio_write_32(&pReg_Alive->VDDCTRLRSTREG,      0x00000001);		// vddpoweron off, start counting down.

	DMC_Delay(600);     // 600 : 110us, Delay for Pending Clear. When CPU clock is 400MHz, this value is minimum delay value.

	mmio_write_32(&pReg_Alive->ALIVEGPIODETECTPENDREG, 0xFF);		// all alive pend pending clear until power down.
//	mmio_write_32(&pReg_Alive->ALIVEPWRGATEREG, 0x00000000);		// alive power gate close

	while(1) {
//		mmio_set_32  (&pReg_ClkPwr->PWRMODE, (0x1 << 1));		// enter STOP mode.
		mmio_write_32(&pReg_ClkPwr->PWRMODE, (0x1 << 1)); 		// enter STOP mode.
		__asm__ __volatile__ ("wfi");					// now real entering point to stop mode.
	}									// this time, core power will off and so cpu will die.
}

 /************************************************************
 * Must be S5P4418
 * Susepnd Off sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. (SDRAM) Enter the Self Refresh.
 * Step 02. (BOARD & CORE) VDD Power OFF
 *************************************************************/
void s5p4418_suspend(void)
{
	enter_self_refresh();
	suspend_vdd_pwroff();
}
#endif	// #if (CONFIG_SUSPEND_RESUME == 1)
