#include "psci.h"

/* External function */
extern int arm_check_mpidr(unsigned int mpidr);

/*******************************************************************************
 * Simple routine to determine whether a mpidr is valid or not.
 ******************************************************************************/
int psci_validate_mpidr(unsigned int mpidr)
{
	if (arm_check_mpidr(mpidr) < 0)
		return PSCI_E_INVALID_PARAMS;

	return PSCI_E_SUCCESS;
}
