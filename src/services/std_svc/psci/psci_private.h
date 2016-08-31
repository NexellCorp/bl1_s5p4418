#ifndef __PSCI_PRIVATE_H__
#define __PSCI_PRIVATE_H__

#include <psci.h>

/*******************************************************************************
 * Function prototypes
 ******************************************************************************/
/* Private exported functions from psci_on.c */
int psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint);

void psci_cpu_on_finish(unsigned int cpu_idx,	psci_power_state_t *state_info);

/* Private exported functions from psci_off.c */
int psci_do_cpu_off(unsigned int cpu_id);

/* Private exported functions from psci_suspend.c */
 int psci_cpu_suspend_start(unsigned int entrypoint);
void psci_cpu_suspend_finish(unsigned int cpu_idx, psci_power_state_t *state_info);

/* Private exported functions from psci_system_off.c */
void psci_system_off(void);
void psci_system_reset(void);

#endif /* __PSCI_PRIVATE_H__ */
