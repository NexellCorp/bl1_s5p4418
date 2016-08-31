#include <sysheader.h>
#include <psci.h>

void psci_system_off(void)
{
	DBGOUT(" %s \r\n", __func__);
	return;
}

/*******************************************************************************
 * s5pxx18 system reset (method: power control)
 ******************************************************************************/
 void reset_cpu(void)
{
	void *base = (void *)PHY_BASEADDR_CLKPWR_MODULE;
	const u32 sw_rst_enb_bitpos = 3;
	const u32 sw_rst_enb_mask = 1 << sw_rst_enb_bitpos;
	const u32 sw_rst_bitpos = 12;
	const u32 sw_rst_mask = 1 << sw_rst_bitpos;
	int pwrcont = 0x224;
	int pwrmode = 0x228;
	u32 reg;

	reg = readl((void *)(base + pwrcont));

	reg &= ~sw_rst_enb_mask;
	reg |= 1 << sw_rst_enb_bitpos;

	writel(reg, (void *)(base + pwrcont));
	writel(sw_rst_mask, (void *)(base + pwrmode));
}

/*******************************************************************************
 * System Reset the Reference Fucntion
 ******************************************************************************/
void psci_system_reset(void)
{
	/* s5pxx18 reset  */
	reset_cpu();
}
