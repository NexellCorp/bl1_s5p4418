#include <sysheader.h>
#include <psci.h>

/* External function */
extern void system_sleep(void);
extern void s5pxx18_resume(void);
extern unsigned int __calc_crc(void *addr, int len);

/* Macro for Suspend/Resume */
#define PSCI_SUSPEND		0
#define PSCI_RESUME		1

/*******************************************************************************
 * Before entering suspend and Mark the location and promise Kernel.
 * Reference CRC, Jump Address, Memory Address(CRC), Size(CRC)
 ******************************************************************************/
static void suspend_mark(unsigned int state, unsigned int entrypoint, unsigned int crc,
						unsigned int mem, unsigned int size)
{
#if 0	// Not determined mark parmeter
	unsigned int crc;
	unsigned int mem = 0x40000000, size = (512*1024*1024);
#endif
	WriteIO32(&pReg_Alive->ALIVESCRATCHRSTREG, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST1, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST2, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST3, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST4, 0xFFFFFFFF);

	if (state == PSCI_SUSPEND) {
		crc = __calc_crc((void *)mem, size);
		WriteIO32(&pReg_Alive->ALIVESCRATCHSETREG, SUSPEND_SIGNATURE);
		WriteIO32(&pReg_Alive->ALIVESCRATCHSET1, entrypoint);
		WriteIO32(&pReg_Alive->ALIVESCRATCHSET2, crc);
		WriteIO32(&pReg_Alive->ALIVESCRATCHSET3, mem);
		WriteIO32(&pReg_Alive->ALIVESCRATCHSET4, size);
	}
}

/*******************************************************************************
 * Designed to meet the the PSCI Common Interface.
 ******************************************************************************/
int psci_cpu_suspend_start(unsigned int entrypoint)
{
	/* s5pxx18 suspend mark */
	suspend_mark(0, entrypoint, 0, 0x48000000, (128*1024));

	/* the function for operation to go sleep */
	system_sleep();

	return PSCI_E_SUCCESS;
}

/*******************************************************************************
 * Designed to meet the the PSCI Common Interface.
 ******************************************************************************/
void psci_cpu_suspend_finish(unsigned int cpu_idx,
			     psci_power_state_t *state_info)
{
	/* Remove warning for futrue externsibility */
	cpu_idx = cpu_idx;
	state_info = state_info;

	/* the function for system wakeup */
	s5pxx18_resume();
}
