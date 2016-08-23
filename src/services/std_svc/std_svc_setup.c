#include "sysheader.h"
#include "std_svc.h"

/* External function */
extern int psci_smc_handler(unsigned int smc_fid, unsigned int x1, unsigned int x2, unsigned int x3);

/*
 * Top-level Standard Service SMC handler. This handler will in turn dispatch
 * calls to PSCI SMC handler
 */
unsigned int std_svc_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3)
{
	/*
	 * Dispatch PSCI calls to PSCI SMC handler and return its return
	 * value
	 */
	if (is_psci_fid(smc_fid)) {
		return psci_smc_handler(smc_fid, r1, r2, r3);
	}

	switch (smc_fid) {
	case ARM_STD_SVC_CALL_COUNT:
		/*
		 * Return the number of Standard Service Calls. PSCI is the only
		 * standard service implemented; so return number of PSCI calls
		 */
		break;

	case ARM_STD_SVC_UID:
		/* Return UID to the caller */
		break;

	case ARM_STD_SVC_VERSION:
		/* Return the version of current implementation */
		break;

	default:
		WARN("Unimplemented Standard Service Call: 0x%x \n", smc_fid);
		break;
	}

	return 0;
}
