#include <sysheader.h>
#include <plat_pm.h>
#include "psci.h"

/*************************************************************
 * Must be S5P4418
 * CPU Power Off sequence in S5P4418
 * Reference is made to function psci interface .
 *************************************************************/
void (*g_psci_ep)();

int psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint)
{
	unsigned int cpu_id = ((target_cpu >> 0) & 0xFF);

	g_psci_ep = (void (*)())entrypoint;

	s5p4418_cpu_on(cpu_id);

	return s5p4418_cpu_check(cpu_id);
}
