/*
 * Copyright (c) 2013-2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PSCI_H__
#define __PSCI_H__

#define PLAT_MAX_PWR_LVL	1

/*******************************************************************************
 * PSCI version
 ******************************************************************************/
#define PSCI_MAJOR_VER		(1 << 16)
#define PSCI_MINOR_VER		0x0

/*******************************************************************************
 * PSCI error codes
 ******************************************************************************/
#define PSCI_E_SUCCESS		0
#define PSCI_E_NOT_SUPPORTED	-1
#define PSCI_E_INVALID_PARAMS	-2
#define PSCI_E_DENIED		-3
#define PSCI_E_ALREADY_ON	-4
#define PSCI_E_ON_PENDING	-5
#define PSCI_E_INTERN_FAIL	-6
#define PSCI_E_NOT_PRESENT	-7
#define PSCI_E_DISABLED		-8
#define PSCI_E_INVALID_ADDRESS	-9

/*
 * Macro to represent invalid affinity level within PSCI.
 */
#define PSCI_INVALID_PWR_LVL	(PLAT_MAX_PWR_LVL + 1)

/*
 * Type for representing the local power state at a particular level.
 */
typedef unsigned char plat_local_state_t;

/* The local state macro used to represent RUN state. */
#define PSCI_LOCAL_STATE_RUN  	0

/*****************************************************************************
 * This data structure defines the representation of the power state parameter
 * for its exchange between the generic PSCI code and the platform port. For
 * example, it is used by the platform port to specify the requested power
 * states during a power management operation. It is used by the generic code to
 * inform the platform about the target power states that each level should
 * enter.
 ****************************************************************************/
typedef struct psci_power_state {
	/*
	 * The pwr_domain_state[] stores the local power state at each level
	 * for the CPU.
	 */
	plat_local_state_t pwr_domain_state[PLAT_MAX_PWR_LVL + 1];
} psci_power_state_t;

/*******************************************************************************
 * Structure populated by platform specific code to export routines which
 * perform common low level power management functions
 ******************************************************************************/
typedef struct plat_psci_ops {
	void (*cpu_standby)(plat_local_state_t cpu_state);
	int (*pwr_domain_on)(unsigned long mpidr);
	void (*pwr_domain_off)(const psci_power_state_t *target_state);
	void (*pwr_domain_suspend)(const psci_power_state_t *target_state);
	void (*pwr_domain_on_finish)(const psci_power_state_t *target_state);
	void (*pwr_domain_suspend_finish)(
				const psci_power_state_t *target_state);
	void (*system_off)(void) ;//__dead2;
	void (*system_reset)(void) ;//__dead2;
	int (*validate_power_state)(unsigned int power_state,
				    psci_power_state_t *req_state);
	int (*validate_ns_entrypoint)(unsigned long long ns_entrypoint);
	void (*get_sys_suspend_power_state)(
				    psci_power_state_t *req_state);
} plat_psci_ops_t;

#endif /* __PSCI_H__ */
