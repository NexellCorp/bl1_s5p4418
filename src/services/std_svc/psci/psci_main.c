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
#include <nx_prototype.h>
#include <plat_pm.h>
#include <psci_main.h>
#include <psci.h>

/* External Function */
extern int  psci_validate_mpidr(unsigned int mpidr);
extern void psci_power_down_wfi(void);

extern int  psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint);
extern int  psci_do_cpu_off(unsigned int cpu_id);

extern void psci_system_off(void);
extern void psci_system_reset(void);
extern int  psci_cpu_suspend_start(unsigned int entrypoint);

/* External Variable */
extern volatile int g_smc_id;
extern volatile int g_fiq_flag;
extern volatile int g_cpu_kill_num;


/* Macro to help build the psci capabilities bitfield */
#define define_psci_cap(x)		(1 << (x & 0x1f))

unsigned int psci_caps = define_psci_cap(PSCI_CPU_OFF) |
	define_psci_cap(PSCI_CPU_ON_AARCH32) |
	define_psci_cap(PSCI_SYSTEM_OFF) |
	define_psci_cap(PSCI_SYSTEM_SUSPEND_AARCH32);

/*******************************************************************************
 * PSCI frontend api for servicing SMCs. Described in the PSCI spec.
 ******************************************************************************/
void psci_set_sgi_caller(int cpu_id)
{
#if 0	// Considering that the original sequence.
	int id;

	for (id = 0; id < 4; id++) {
		if (cpu_id != id) {
			g_fiq_flag |= (1 << id);
			s5p4418_cpuidle(id, (id + 8));
		}
	}
#else	// Sequences that are usually used.
	cpu_id = cpu_id;
	s5p4418_cpuidle(0, (0 + 8));
#endif
}

unsigned int psci_version(void)
{
	return PSCI_MAJOR_VER | PSCI_MINOR_VER;
}

int psci_cpu_on(unsigned int target_cpu,
		unsigned int entrypoint, unsigned int context_id)
{
	int rc;

	context_id = context_id;

	/* Determine if the cpu exists of not */
	rc = psci_validate_mpidr(target_cpu);
	if (rc != PSCI_E_SUCCESS)
		return PSCI_E_INVALID_PARAMS;

	/* s5p4418(cortex-a9) Actual secondary cpu on */
	rc = psci_cpu_on_start(target_cpu, entrypoint);

	return rc;
}

int psci_cpu_off(void)
{
	int cpu_id = armv7_get_cpuid();
	int rc = 0;

	/* Check the CPUx to Kill*/
	g_smc_id = PSCI_CPU_OFF;
	g_cpu_kill_num = cpu_id;

	/* Check the SGI Call CPUx */
	psci_set_sgi_caller(cpu_id);

	s5p4418_cpu_off_wfi_ready();
	psci_power_down_wfi();

	return rc;
}

int psci_system_suspend(unsigned int entrypoint, unsigned int context_id)
{
	int rc = 0;

	entrypoint = entrypoint;
	context_id = context_id;
	rc = rc;

	rc = psci_cpu_suspend_start(entrypoint);

	return rc;
}

int psci_affinity_info(unsigned int target_affinity,
		       unsigned int lowest_affinity_level)
{
	unsigned int cpu_id = (target_affinity & 0xF);
	unsigned int status = 0;

	target_affinity = target_affinity;
	lowest_affinity_level = lowest_affinity_level;

	status = s5p4418_cpu_check(cpu_id);

	return status;
}

int psci_features(unsigned int psci_fid)
{
	unsigned int local_caps = psci_caps;

	/* TODO: sanity check of psci_fid */

	/* Check if the psci fid is supported or not */
	if (!(local_caps & define_psci_cap(psci_fid)))
		return PSCI_E_NOT_SUPPORTED;

	/* Return 0 for all other fid's */
	return PSCI_E_SUCCESS;
}

int psci_smc_handler(
	unsigned int smc_fid,
	unsigned int x1, unsigned int x2, unsigned int x3)
{

	DBGOUT(" R0: %X, R1: %X, R2: %X, R3: %X \r\n",
		smc_fid, x1, x2, x3);

	/* Check the psci smc_fid  */
	if (((smc_fid >> FUNCID_CC_SHIFT) & FUNCID_CC_MASK) == SMC_32) {
		/* 32-bit PSCI function, clear top parameter bits */

		switch (smc_fid) {
		case PSCI_VERSION:
			return psci_version();

		case PSCI_CPU_OFF:
			return psci_cpu_off();

		case PSCI_CPU_ON_AARCH32:
			return psci_cpu_on(x1, x2, x3);

		case PSCI_AFFINITY_INFO_AARCH32:
			return psci_affinity_info(x1, x2);

		case PSCI_MIG_INFO_TYPE:
			return PSCI_E_NOT_SUPPORTED;

		case PSCI_SYSTEM_SUSPEND_AARCH32:
			return psci_system_suspend(x1, x2);

		case PSCI_SYSTEM_OFF:
			psci_system_off();
			WARN("PSCI_SYSTEM_OFF\r\n");
			/* We should never return from psci_system_off() */
			break;

		case PSCI_SYSTEM_RESET:
			psci_system_reset();
			WARN("PSCI_SYSTEM_RESET\r\n");
			/* We should never return from psci_system_reset() */
			break;

		case PSCI_FEATURES:
			return psci_features(x1);

		default:
			break;
		}
	}

	WARN("Unimplemented PSCI Call: 0x%x \r\n", smc_fid);

	return -1;
}
