#include <sysheader.h>
#include "psci.h"

/* External Function */
extern void ResetCon(U32 devicenum, CBOOL en);

 /*******************************************************************************
 * Must be S5P4418
 * CPU Power Off sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. CPUx Block Reset Assert
 * Step 02. Have to wait more than 5us.
 * Step 03. CPUCLKOFF Set to 1 except CPU0
 * Step 04. CPUx Block Reset Negate
 * Step 05. CPUx Power On
 ******************************************************************************/
void (*g_psci_ep)();

int psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint)
{
	unsigned int cpu_id = ((target_cpu >> 0) & 0xFF);

	g_psci_ep = (void (*)())entrypoint;

	/* Secondary CPU Wakeup Start Point (High Vector) */
	SetIO32(&pReg_Tieoff->TIEOFFREG[0], ((1 << cpu_id) << 18));

	/* Step 01. CPUx Block Reset Assert */
	ResetCon(cpu_id, CTRUE);

	/* Step 02. Waiting for 5us */

	/* Step 03. CPUCLKOFF Set to 1 except CPU0 */
	SetIO32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));

	/* Step 04. CPUx Block Reset Negate */
	ResetCon(cpu_id, CFALSE);

	/* Step 05. CPUx Power On (1: Power Down, 0:Active) */
	ClearIO32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));

	return PSCI_E_SUCCESS;
}