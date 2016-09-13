#ifndef __PLAT_PM_H__
#define __PLAT_PM_H__

void s5p4418_cpuidle(int cpu_id, int int_id);
void s5p4418_cpu_off_wfi_ready(void);

 int s5p4418_cpu_check(unsigned int cpu_id);

 int s5p4418_cpu_on(unsigned int cpu_id);
 int s5p4418_cpu_off(unsigned int cpu_id);

void s5p4418_suspend(void);
void s5p4418_resume(void);

#endif	// __PLAT_PM_H__
