#include "sysheader.h"

unsigned char* gic_cpuif_get_baseaddr(void)
{
	return ((unsigned char*)0xF0000100);
}

unsigned char* gic_disp_get_baseaddr(void)
{
	return ((unsigned char*)0xF0001000);
}

#if 0
void gic_disp_init(void)
{
	unsigned int dist_base = gic_disp_get_baseaddr();
	unsigned int cpu_base = gic_cpuif_get_baseaddr();

	int nonsecure = 1, secure = 1;
	int i;

	/* CPU Interface Enable */
	WriteIO32((cpu_base + GIC_CPUIF_CTRL), (3 << 0));

	/*
	 * Whether or not to activate the interrupt
	 * occurs GROUP0/GROUP1 on the GIC.
	 */
	WriteIO32((dist_base + GIC_DISP_CTRL), (nonsecure << 1) | (secure << 0));

	/*
	 * secure/non-secure used to determine
	 * the priority of the interrupt.
	 */
	WriteIO32((cpu_base + GIC_CPUIF_PRIORTY), 0xFFFFFFFF);

	/*
	 * GIC set the path that is connected to the
	 * internal IP interrupts, generated. (0: Secure, 1: Non-Secure)
	 */
	for (i = 0; i <= 0xC; i+=4)
		WriteIO32((dist_base + GIC_DISP_GROUP + i), 0xFFFFFFFF);

}
#endif
