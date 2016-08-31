#include <sysheader.h>
#include <gic.h>
#include "psci.h"

/* External Function */
extern void ResetCon(U32 devicenum, CBOOL en);
/* External Variable */
extern unsigned int g_cpu_state;

/*
 * Must be S5P44418
 * For CPU Power Off, STANDBY_WFI[n]
 * the signal must wait until be High.
 */
int psci_get_standby_wfi(unsigned int cpu_id)
{
	unsigned int bitpos = 12;

	return (ReadIO32(&pReg_ClkPwr->PWRCONT) >> (cpu_id + bitpos));
}

 /*******************************************************************************
 * Must be S5P4418
 * CPU Power Off sequence in S5P4418
 * must go through the following steps:
 *
 * Step 01. Set the Clam Signal(High)
 * Step 02. Have to wait more than 20us.
 * Step 03. Waiting for the Standard WFI Signal Gating.
 * Step 04. Set the CPUx Power Down
 * Step 05. Reset to the CPUx Block.
 ******************************************************************************/
int psci_do_cpu_off(unsigned int cpu_id)
{
	int ret = 1;

	/* Step 01. Set to (CPUx) Clamp Signal High */
	SetIO32(&pReg_Tieoff->TIEOFFREG[0], (1 << (cpu_id + 1)));

	/* Step 02. Waiting for 20us */
	delay_ms(0xFFFFF);

	/* Step 03. Waiting for the Standard WFI Signal Gating. */
	while(psci_get_standby_wfi(cpu_id));

	/* Step 04. CPUx Power Down (1: Power Down, 0:Active) */
	SetIO32(&pReg_Tieoff->TIEOFFREG[1], ((1 << cpu_id) << (37 - 32)));

	/* Step 05. CPUx Block Reset Assert */
	ResetCon(cpu_id, CTRUE);

	return ret;		// 0: ON, 1:OFF, 2:PENDING
}
