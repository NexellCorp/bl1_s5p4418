#include <sysheader.h>
#include <nx_prototype.h>
#include <psci_main.h>
#include <psci.h>

extern int  armv7_get_cpu_id(void);
extern int  psci_validate_mpidr(unsigned int mpidr);

extern void psci_power_down_wfi(void);
extern int  psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint);
extern void psci_system_off(void);
extern void psci_system_reset(void);

extern int  psci_do_cpu_off(unsigned int cpu_id);
extern int  psci_cpu_suspend_start(unsigned int entrypoint);

unsigned int g_cpu_state = 0;


/* Macro to help build the psci capabilities bitfield */
#define define_psci_cap(x)		(1 << (x & 0x1f))

unsigned int psci_caps = define_psci_cap(PSCI_CPU_OFF) |
	define_psci_cap(PSCI_CPU_ON_AARCH32) |
	define_psci_cap(PSCI_SYSTEM_OFF) |
	define_psci_cap(PSCI_SYSTEM_SUSPEND_AARCH32);

/*******************************************************************************
 * PSCI frontend api for servicing SMCs. Described in the PSCI spec.
 ******************************************************************************/
int psci_cpu_on(unsigned int target_cpu,
		unsigned int entrypoint,
		unsigned int context_id)
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

unsigned int psci_version(void)
{
	return PSCI_MAJOR_VER | PSCI_MINOR_VER;
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

int psci_cpu_off(void)
{
	int cpu_id = armv7_get_cpu_id();
	int rc = 0;

	/* Check the CPUx to Kill*/
	g_cpu_state = 1UL << cpu_id;

#if 0	// SWI Type
	if (psci_set_sgi_caller())
		rc = PSCI_E_SUCCESS;
#endif

	DBGOUT(" %s \r\n", __func__);

	psci_power_down_wfi();

	return rc;
}

extern int arm9_get_mpidr(void);
int psci_affinity_info(unsigned int target_affinity,
		       unsigned int lowest_affinity_level)
{
	unsigned int cpu_id = (target_affinity & 0xF);
	unsigned int status = 0;

	/*  */
	target_affinity = target_affinity;
	lowest_affinity_level = lowest_affinity_level;

	/* Check the CPUx to Kill*/
#if 0	// Check the Hardware Signal 
	status = ReadIO32(&pReg_Tieoff->TIEOFFREG[1]) >> (cpu_id + 5);
#else	// Check the Cpu State Variable
	status = (g_cpu_state >> cpu_id) & 0x1;
#endif
	DBGOUT(" %s \r\n", __func__);

	if (status)
		return psci_do_cpu_off(cpu_id);

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
	unsigned int x1, unsigned int x2,
	unsigned int x3)
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
