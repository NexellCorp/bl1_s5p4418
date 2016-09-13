#include <sysheader.h>
#include <plat_pm.h>
#include <gic.h>
#include "psci.h"

/* External Function */
extern void psci_power_down_wfi(void);

/* External Variable */
extern volatile int g_fiq_flag;
extern volatile int g_cpu_kill_num;

int psci_cpu_off_handler(void)
{
	char* cpu_base = (char*)gicc_get_baseaddr();
	int cpu_id = armv7_get_cpuid();
	int eoir = 0;
	int ret;

	eoir = gicc_get_iar(cpu_base);
	gicc_set_eoir(cpu_base, eoir);

	/* It is necessary in order to ensure sequential operation.*/
	if (cpu_id != 0) {
		g_fiq_flag |= (1 << cpu_id);
		do {
			cache_delay_ms(0xFFFFF);
		} while(g_fiq_flag & (1 << cpu_id));

		return 0;
	}

	/* cpu0 operated to the subcpu power off*/
	ret =  s5p4418_cpu_off(g_cpu_kill_num);
	if (ret > 0)
		g_fiq_flag = 0;

	return ret;
}

/*************************************************************
 * Must be S5P4418
 * CPU Power Off sequence in S5P4418
 * Reference is made to function psci interface .
 *************************************************************/
int psci_do_cpu_off(unsigned int target_cpu)
{
	unsigned int cpu_id = ((target_cpu >> 0) & 0xFF);

	s5p4418_cpu_off(cpu_id);

	return s5p4418_cpu_check(cpu_id);	// 0: ON, 1:OFF, 2:PENDING
}
