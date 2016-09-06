#include <sysheader.h>
#include <smcc_helpers.h>
#include <gic.h>

/* External function */
extern  int armv7_get_cpuid(void);
extern  int arm9_get_scr(void);
extern void arm9_set_scr(int value);
extern void set_nonsecure_mode(void);
extern void set_secure_mode(void);

extern unsigned int std_svc_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3);

extern unsigned int sip_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3);

#define IS_SMC_TYPE(_fid)	((_fid >> 24) & 0xF)

/* External Variable */
extern volatile int g_fiq_flag;

/*******************************************************************************
 * Monitor Mode FIQ Hanlder ofr servicing BL1 SMCs.
 * Currently purpose used for the BCLK DFS.
 ******************************************************************************/
void smc_set_monitor_fiq(int enable)
{
	int reg = 0;
	int bit_pos = 2;

	reg = arm9_get_scr();
	reg &= ~(1 << bit_pos);
	if (enable)
		reg |= (enable << bit_pos);
	arm9_set_scr(reg);
}


/*******************************************************************************
 * Monitor Mode FIQ Hanlder ofr servicing BL1 SMCs.
 * Currently purpose used for the BCLK DFS.
 ******************************************************************************/
int smc_monitor_fiq_handler(void)
{
	char* cpu_base = (char*)gicc_get_baseaddr();
	int cpu_id = armv7_get_cpuid();
	int eoir = 0;

	set_secure_mode();

	/* */
	eoir = gicc_get_iar(cpu_base);
	gicc_set_eoir(cpu_base, eoir);

	while(g_fiq_flag & (1 << cpu_id));

	set_nonsecure_mode();

	return 0;
}

/*******************************************************************************
 * Top level handler for servicing BL1 SMCs.
 ******************************************************************************/
unsigned int bl1_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3)
{
	unsigned char smc_type = IS_SMC_TYPE(smc_fid);

	switch (smc_type) {
	case OEN_SIP_START:
		return sip_smc_handler(smc_fid, r1, r2, r3);

	case OEN_STD_START:
		return std_svc_smc_handler(smc_fid, r1, r2, r3);
	}

	WARN("Unimplemented BL1 SMC Call: 0x%x \n", smc_fid);

	return 0;
}
