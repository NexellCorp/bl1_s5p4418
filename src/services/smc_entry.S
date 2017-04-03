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
#include "nx_pyrope.h"

#define SRAM_BASEADDR	0xFFFF0000
#define SRAM_SIZE	(32*1024)

#define BL1_SMC_RUN_IMAGE               0x4
#define ENTRY_POINT_INFO_PC_OFFSET      0x08
#define ENTRY_POINT_INFO_ARGS_OFFSET    0x18
#define MODE32_MASK             0xf

.global smc_asm_handler

/*
 * Monitor Mode - Exception Vetor table
 *
 */
.align 8
.global monitor_vectors
monitor_vectors:
        b       .       				// 00 -
        b       .    					// 04 -
        b       smc_asm_handler				// 08 - SMC Interrupt
        b       .          				// 0C -
        b       .          				// 10 -
        b       .                   			// 14 -
        b       .                   			// 18 -
        b       .					// 1C -

/*
 * (BL1) SMC Caller Handler
 */
.global bl1_smc_handler
.align
.global smc_asm_handler
smc_asm_handler:
	stmfd	sp!, {r4-r12, lr}
	cmp	r0, #BL1_SMC_RUN_IMAGE
	bne	.
	mrc     p15, 0, lr, c1, c1, 0
	tst	lr, #(1 << 0)	// ARMv7 NS bit
	bne	unexpected_smc_call	// if not call from secure
	mov	r8, r1
	ldmfd	r8!, {r0, r1}
	mov	lr, r0
	msr	spsr, r1
	and	r0, r1, #MODE32_MASK
	cmp	r0, #0x6		// ARMv7 monitor mode code
	bne	unexpected_smc_call
	ldmfd	r8!, {r0-r7}
	movs	pc, lr		// run svc mode

unexpected_smc_call:
	wfi
	b	.
/*
 *  Only Monitor Mode - Deoks
 */
.global monitor_mode_init
monitor_mode_init:
	/* -----------------------------------------
	 * Secure - SVC Mode -> Monitor Mode
	 * Changes in the SVC Mode to MON Mode,
	 * and store the link register values.
	 * -----------------------------------------
	 */
	mov	r8, lr
	/* Switch to monitor mode  */
	msr	CPSR_c,  #(MODE_MON|I_Bit|F_Bit)

        mrc     p15, 0, r0, c1, c0, 0               	// Read CP15 System Control register
        bic     r0, r0, #(BIT1_V|BIT1_I|BIT1_Z)     	// Clear V:13, I:12, Z:11 bit to disable High Vector, I-Cache, Branch Prediction
        bic     r0, r0, #(BIT1_C|BIT1_M)            	// Clear C:2, M:0 bit to disable D Cache, MMU
        mcr     p15, 0, r0, c1, c0, 0               	// Write value back to CP15 System Control register

#if 0
	/* -----------------------------------------
	 * Only try to set SMP mode if not yet set
	 * cannot write AuxCtrl if in normal world
	 * -----------------------------------------
	 */
        mrc     p15, 0, r0, c1, c0, 1			//  Read Aux Control Register
        tst     r0, #(1 << 6)                       	//  SMP bit
        orreq   r0, r0, #(1 << 6)                   	//  Set SMP bit in aux control register
        orreq   r0, r0, #0x7                        	//  L2C control
        mcreq   p15, 0, r0, c1, c0, 1               	//  Write Aux Control Register
#endif

	/*
	 * Processor is in Secure Mode, when the FIQ occurs
	 * in, which specifies the vector jump to Monitor Mode.
	 */
	mrc	p15, 0, r0, c1, c1, 0			// Read SCR into Rt
	orr	r0, r0, #(1 << 2)			// SCR- FIQ Forward[2]
	mcr	p15, 0, r0, c1, c1, 0			// Write Rt to SCR

	//;  Configure CPU
	mrc	p15, 0, r0, c1, c0, 1
	//;  Only try to set SMP mode if not yet set - cannot write AuxCtrl if in normal world
	tst	r0, #(1 << 6)				// SMP bit
	orreq	r0, r0, #(1 << 6)			// Set SMP bit in aux control register
	orreq	r0, r0, #0x7				// L2C control
//	orreq	r0, r0, #(1 << 3)			// Zero
	mcreq	p15, 0, r0, c1, c0, 1			// Write Aux Control Register

        bl      Invalidate_entire_branch_predictor
        bl      InvalidateDataCache

	/* for pll change stable */
        mrc     p15, 0, r0, c1, c0, 0			// Read CP15 System Control register
        orr     r0, r0, #(BIT1_I|BIT1_Z)		// Set I:12, Z:11 bit to disable High Vector, I-Cache, Branch Prediction
        mcr     p15, 0, r0, c1, c0, 0

	/* Stack size set to monitor mode  */
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE

        mrc     p15, 0, r0, c0, c0, 5			// Get our cpu id
        ands    r0, r0, #0xF				// Save CPU id

	mov     r1, #0x100
	sub     r2, r0, #1
	and     r2, r2, #3
	mul     r1, r1, r2
	sub     sp, sp, r1

set_montior_vector:
	/* (MVBAR) set to monitor vector base address */
	ldr	r0, =monitor_vectors
	mcr	p15, 0, r0, c12, c0, 1

	/* Restores the LR Address value */
	mov	lr, r8
	bx	lr

.global wfi_spill
.global psci_power_down_wfi
psci_power_down_wfi:
	dsb	sy					// ensure write buffer empty
	wfi
wfi_spill:
	b	psci_power_down_wfi
