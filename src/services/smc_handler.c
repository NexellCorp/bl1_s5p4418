#include <sysheader.h>
#include <smcc_helpers.h>
#include <psci.h>
#include <gic.h>

/* External function */
extern unsigned int std_svc_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3);

extern unsigned int sip_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3);

extern int s5p4418_bclk_dfs_handler(void);
extern int psci_cpu_off_handler(void);

#define IS_SMC_TYPE(_fid)	((_fid >> 24) & 0xF)

/* External Variable */
volatile int g_smc_id    = 0;
volatile int g_fiq_flag  = 0;
volatile int g_cpu_kill_num = 0;

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
	int ret = 0;

	set_secure_mode();

	if (g_smc_id == (int)0x84000002)
		ret = psci_cpu_off_handler();

	else if (g_smc_id == (int)0x82000009)
		ret = s5p4418_bclk_dfs_handler();
	else
		WARN("unknown parameter smc_id : 0x%08X \r\n", g_smc_id);

	set_nonsecure_mode();

	return ret;
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
