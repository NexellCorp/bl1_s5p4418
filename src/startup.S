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
#include "nx_pyrope.h"
#include "cfgBootDefine.h"

        .align
/* Start and e.nd of BSS */

.globl __bss_start__
.globl __bss_end__

/*
 * entry point of main function
 */
.global BootMain
.global SubCPUBoot
.global exception_v
.global monitor_mode_init
.global tieoff_set_secure
.text
//==================================================================
// vectors
//==================================================================
.global vectors
vectors:
        b       reset_handler       		// 00 - Reset
        b       exception_v          		// 04 - Undefined instructions
        b       .                   		// 08 - SWI instructions
        b       exception_v          		// 0C - Instruction fetch aborts
        b       exception_v          		// 10 - Data access aborts
        b       .                  		// 14 - Reserved (was address exception)
        b       .                   		// 18 - IRQ interrupts
        b       .                   		// 1C - FIQ interrupts

	b	.				// 20 - Reserved.

buildinfo:
        .word   0x44180602          		// 24 - Chip name - 4418, Build num - v0.6.01

exception_v:
        msr     CPSR_c, #(Mode_SVC|I_Bit|F_Bit)

        mrc     p15, 0, r0, c1, c0, 0		// Read CP15 System Control register
        bic     r0, r0, #(BIT1_V|BIT1_I|BIT1_Z)	// Clear V:13, I:12, Z:11 bit to disable High Vector, I-Cache, Branch Prediction
        bic     r0, r0, #(BIT1_C|BIT1_M)	// Clear C:2, M:0 bit to disable D Cache, MMU
        mcr     p15, 0, r0, c1, c0, 0		// Write value back to CP15 System Control register

        movt	r1, #0xC001			// r1 <= 0xc0010000
        ldr     r0, [r1, #0x27C]		// Get Boot Config

        bic     r0, r0, #0x7			// clear boot mode
        orr     r0, r0, #0x6			// set boot mode to USB

        mov     r1, #0x1C4			// rom boot jump address
        bx      r1

reset_handler:
#if defined(SECURE_MODE)
        mrc     p15, 0, r12, c0, c0, 5		// Get our cpu id
        ands    r12, r12, #0xF			// Save CPU id
        bne     secondary_startup		// jump to secondary cpu
#else	// Non-Secure Mode
	/* Set the SVC Vector(VBAR) */
	ldr	r0, =vectors
	mcr	p15, 0, r0, c12, c0, 0		// set svc vector(VBAR)

	/* Set the initialization for PSCI (Monitor Mode) */
	bl	monitor_mode_init

#if 0
	/* GIC initialization settings for the Non-Secure change. */
	bl	gic_disp_init
#else
	/* GIC secure part enable, route to non-secure */
	mrc	p15, 4, r0, c15, c0, 0		// Read CBAR
	mov     r1, #(0xB << 0)			// FIQ_En[3], EnableGrp1[1], EnableGrp0[0]
	str	r1, [r0, #0x100]		// gicc enable
	orr	r2, r0, #0x1000			// base of gicd
	str     r1, [r2, #0x000]		// gicd secure enable

        mov     r1, #0xFFFFFFFF
	str	r1, [r0, #0x104]		// gicc - interrupt priority to best
        str     r1, [r2, #0x084]		// gicd - interrupt route to non-secure
        str     r1, [r2, #0x088]		// gicd - interrupt group
        str     r1, [r2, #0x08C]		// gicd - interrupt group

	mov	r1, #0xFFFF00FF			// gicd - interrupt secure-route
        str     r1, [r2, #0x080]		// gicd - interrupat Group Register

        ands    r12, r12, #0xF			// Save CPU id
#endif

	/*
	 * Set the Non-Secure FPU
	 * psw0523 add for fpu setting
	 */
	mrc	p15, 0, r0, c1, c1, 2
	orr	r0, r0, #3 << 10		// enable fpu/neon
	bic	r0, r0, #3 << 14		// temp
	mcr	p15, 0, r0, c1, c1, 2

	ldr 	r0, =(0xF << 20)
	mcr 	p15, 0, r0, c1, c0, 2

	mov	r3, #0x40000000
	vmsr	fpexc, r3

        mrc     p15, 0, r12, c0, c0, 5         	// Get our cpu id
        ands    r12, r12, #0xF			// Save CPU id
        bne     main
#endif // if defined(SECURE_MODE)

	/* Release pad holding */
	ldr     r0, =0xC0010800			// alive base address
	mov     r1, #0x01
	str     r1, [r0, #0x00]			// open write gate
//	mov     r1, #0x3C0
	mov     r1, #0x3FC
	str     r1, [r0, #0x9c]			// disable pad holding

#if !defined(SECURE_MODE)
	/* (tieoff) Setting for the PBUS Non-Secure change.*/
	bl 	tieoff_set_secure

	/*
	 * Processor is in Secure Mode, when the FIQ occurs
	 * in, which specifies the vector jump to Monitor Mode.
	 */
	mrc	p15, 0, r0, c1, c1, 0		// Read SCR into Rt
	orr	r0, r0, #(1 << 2)		// SCR- FIQ Forward[2]
	mcr	p15, 0, r0, c1, c1, 0		// Write Rt to SCR
#endif

	/* Set to zero, Clear SRAM */
        ldr     r1, =__bss_start__		// this is auto-relocated!
        ldr     r2, =__bss_end__		// this is auto-relocated!

        mov     r3, #0x00000000			// prepare zero to clear BSS

clbss_l:
        cmp     r1, r2				// while not at end of BSS
        strlo   r3, [r1]			// clear 32-bit BSS word
        addlo   r1, r1, #4			// move to next
        blo     clbss_l

main:
#if defined(SECURE_MODE)
	/* Setup to Stack */
        msr     CPSR_c, #(Mode_SVC|I_Bit|F_Bit)

        /* Set stack pointer */
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE	// Add to internal sram size.
        sub     sp, sp, #0x60
        mov     r0, r12

        bl      BootMain			// save this in register for possible long jump
        b       exception_v
#else	// Non-Secure Mode
	/* Change to Secure -> Non-Secure */
	bl	set_nonsecure_mode

        bl      Invalidate_entire_branch_predictor
        bl      InvalidateDataCache

	MOV     r0,  #0
	MOV     r1,  #0
	MOV     r2,  #0
	MOV     r3,  #0
	MOV     r4,  #0
	MOV     r5,  #0
	MOV     r6,  #0
	MOV     r7,  #0
	MOV     r8,  #0
	MOV     r9,  #0
	MOV     r10, #0
	MOV     r11, #0
	MOV     r12, #0

	/* switch to supervisor mode */
#if 1
	ldr	lr, =svc_start

        mrc     p15, 0, r12, c0, c0, 5		// Get our cpu id
        ands    r12, r12, #0xF			// Save CPU id
        beq	cpux_fiq_en

cpux_fiq_en:
	mov	r0, #(Mode_SVC|I_Bit|F_Bit)	// CPU 1,2,3

cpu0_fiq_en:
	mov	r0, #(Mode_SVC|I_Bit)		// CPU 0

	msr     SPSR_cxsf, r0
	movs	pc, lr
#else
        mrc     p15, 0, r12, c0, c0, 5		//; Get our cpu id
        ands    r12, r12, #0xF			//; Save CPU id
        beq	cpux_fiq_en

cpux_fiq_en:
	mov	r0, #(Mode_SVC|I_Bit|F_Bit)	// CPU 1,2,3

cpu0_fiq_en:
	mov	r0, #(Mode_SVC|I_Bit)		// CPU 0
#endif

svc_start:
	/*
	 * For PLL Change Stable
	 * Before the PLL (P, M, S) changes,
	 * preventing the Bus access, enable
	 * the I-Cache to fetch only the opcode.
	 */
        mrc     p15, 0, r0, c1, c0, 0		//; Read CP15 System Control register
        orr     r0, r0, #(BIT1_V|BIT1_I|BIT1_Z)	//; Clear V:13, I:12, Z:11 bit to disable High Vector, I-Cache, Branch Prediction
        mcr     p15, 0, r0, c1, c0, 0

        /*
         * Set stack pointer
         */
	mrc	p15, 0, r12, c0, c0, 5		//; Get our cpu id
	and	r12, r12, #0xF			//; Save CPU id
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE	//; Add to internal sram size.
        sub     sp, sp, #0x400			//; for monitor mode stack

        mov     r1, #0x100
        sub     r2, r12, #1
        and     r2, r2, #3
        mul     r1, r1, r2
        sub     sp, sp, r1

        movs    r0, r12

	blne	subcpu_boot
        bleq    BootMain                            //; save this in register for possible long jump
        b       exception_v
#endif	// #if defined(SECURE_MODE)

#if defined(SECURE_MODE)
//===================================================================
// CPU 1, 2, 3 initialize
//===================================================================
secondary_startup:
        mrc     p15, 0, r0, c1, c0, 0			// Read CP15 System Control register
        bic     r0, r0, #(BIT1_V|BIT1_I|BIT1_Z)		// Clear V:13, I:12, Z:11 bit to disable High Vector, I-Cache, Branch Prediction
        bic     r0, r0, #(BIT1_C|BIT1_M)		// Clear C:2, M:0 bit to disable D Cache, MMU
        mcr     p15, 0, r0, c1, c0, 0			// Write value back to CP15 System Control register

        msr     CPSR_c, #(Mode_SVC|I_Bit|F_Bit)		// Set CPU to SVC32 Mode

//===================================================================
// Set Vector Base Address Register (VBAR) to point to this
// application's vector table
//===================================================================
        ldr     r0, =vectors
        mcr     p15, 0, r0, c12, c0, 0			// Write Secure or Non-secure Vector Base Address
        mrc     p15, 0, r0, c1, c0, 0			// Read SCTLR
        bic     r0, #(1 << 13)				// Clear V-bit
        mcr     p15, 0, r0, c1, c0, 0			// Write SCTLR

        bl      Invalidate_entire_branch_predictor
        bl      InvalidateDataCache

//==================================================================
// Set SMP Mode for CPU 1, 2, 3
//==================================================================
        // Ensure interrupts can get through the priority mask
        mov     r1, #0xF0000000
        mov     r0, #0xFF
        str     r0, [r1, #0x104]

        mov     r0, #1
        orr     r2, r1, #0x1000
        str     r0, [r2]                            	// distributor enable

        str     r0, [r1, #0x100]                    	// CPU IF enable

        mov     r0, #0x00FF
        orr     r0, #0xFF00
        str     r0, [r2, #100]                      	// distributor set enable

        //;  Configure CPU
        mrc     p15, 0, r0, c1, c0, 1
        //;  Only try to set SMP mode if not yet set - cannot write AuxCtrl if in normal world
        tst     r0, #(1 << 6)                       	//  SMP bit
        orreq   r0, r0, #(1 << 6)                   	//  Set SMP bit in aux control register
        orreq   r0, r0, #0x7                        	//  L2C control
        mcreq   p15, 0, r0, c1, c0, 1               	//  write Aux Control Register

        //; Set stack pointer
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE		// Add to internal sram size.

        mov     r1, #0x20
        sub     r2, r12, #1
        and     r2, r2, #3
        mul     r1, r1, r2
        sub     sp, sp, r1

        mov     r0, r12
        b       subcpu_boot
//==================================================================
// CPU 1, 2, 3 wait
//==================================================================
.global subcpu_wfi
subcpu_wfi:
        WFI

        mov     r0, #0xC0000000				// peri base
        orr     r0, r0, #0x00010800			// Alive offset
        ldr     r2, [r0]				// save power gating status
        mov     r1, #1
        str     r1, [r0]				// power gate open
        ldr     r3, [r0, #0x41C]			// get jump address
        cmp     r3, #0xFFFFFFFF				// check if invalid address
        bxne    r3					// if valid address, get jump.
//	streq   r2, [r0]					// restore power gating status
        beq     subcpu_wfi
#endif

.align 8
.global system_sleep
//==================================================================
// suspend/resume routine
//==================================================================
#if (CONFIG_SUSPEND_RESUME == 1)	// this code is call linux kernel, so here is virtual memory space.
.global s5p4418_suspend
system_sleep:				// r0:alive r1:drex
disable_mmu:
        mrc     p15, 0, r1, c1, c0, 0   		// Read control register
        bic     r1, r1, #0x5            		// Disable MMU & DC.
        bic     r1, r1, #0x1000         		// Disable IC.

        ldr     r0, =physical_start
        cmp     r0, #0                  		// make sure no stall on "mov pc,r0" below

        // Disable the MMU.
        mcr     p15, 0, r1, c1, c0, 0

        //; Jump to the physical address of the 'PhysicalStart' label.
        //;
        mov     pc, r0                  		// jump to new physical address
        nop
        nop
        nop

        // MMU & Caches Now Disabled.
        .align 5
physical_start:
        // Disable IRQ & FIQ.
        mrs     r0, CPSR
        orr     r0, r0, #(I_Bit|F_Bit)
        msr     CPSR_cxsf, r0

        // Set to Stack pointer
        ldr     sp, =BASEADDR_SRAM
        add     sp, sp, #INTERNAL_SRAM_SIZE         	// Add to internal sram size.

        // Goto sleepMain function.
        bl      s5p4418_suspend
#else
SystemSleep:
#endif
        b       .                                   	// this time, core power will off and so cpu will die.

	.ltorg