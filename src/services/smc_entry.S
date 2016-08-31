/*
 *      Copyright (C) 2012 Nexell Co., All Rights Reserved
 *      Nexell Co. Proprietary & Confidential
 *
 *      NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *      AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *      BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
 *	FITNESS
 *      FOR A PARTICULAR PURPOSE.
 *
 *      Module          : PSCI (Power Stable Corditional)
 *      File            : psci_entry.S
 *      Description     :
 *      Author          : Firware Team
 *      History         : 2016.06.27 Deoks
 */
#include "nx_pyrope.h"
 
#define SRAM_BASEADDR	0xFFFF0000
#define SRAM_SIZE	(32*1024)

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
        b       .					// 1C - FIQ Interrupt

/*
 * (BL1) SMC Caller Handler
 */
.global bl1_smc_handler
.align 8
.global smc_asm_handler
smc_asm_handler:
	stmfd	sp!, {r4-r12, lr}
	bl	bl1_smc_handler
	ldmfd	sp!, {r4-r12, lr}
	movs 	pc, r14
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

	/* Secure, Monitor Mode - Set Stack Pointer */
	mrc	p15, 0, r12, c0, c0, 5              	// Get our cpu id
	and	r12, r12, #0xF                      	// Save CPU id
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE		// Add to internal sram size.
        sub     sp, sp, #0x800				// for monitor mode stack

        mov     r1, #0x100
        sub     r2, r12, #1
        and     r2, r2, #3
        mul     r1, r1, r2
        sub     sp, sp, r1
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
        orr     r11, r11, #(BIT1_V|BIT1_I|BIT1_Z)	// Clear V:13, I:12, Z:11 bit to disable High Vector, I-Cache, Branch Prediction
        mcr     p15, 0, r11, c1, c0, 0

	/* Stack size set to monitor mode  */
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE

        mrc     p15, 0, r12, c0, c0, 5			// Get our cpu id
        ands    r12, r12, #0xF				// Save CPU id

	mov     r1, #0x100
	sub     r2, r12, #1
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
