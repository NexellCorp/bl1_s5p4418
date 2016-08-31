#include <sysheader.h>
#include <smcc_helpers.h>

extern unsigned int std_svc_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3);

extern unsigned int sip_smc_handler(unsigned int smc_fid,
	unsigned int r1, unsigned int r2, unsigned int r3);

#define IS_SMC_TYPE(_fid)	((_fid >> 24) & 0xF)

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
