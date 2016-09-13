#include <sysheader.h>
#include <psci.h>

void psci_system_off(void)
{
	DBGOUT(" %s \r\n", __func__);
	return;
}

/*************************************************************
 * s5p4418 system reset (method: power control)
 *************************************************************/
 void s5p4418_reset_cpu(void)
{
	void *base = (void *)PHY_BASEADDR_CLKPWR_MODULE;
	const unsigned int sw_rst_enb_bitpos = 3;
	const unsigned int sw_rst_enb_mask = 1 << sw_rst_enb_bitpos;
	const unsigned int sw_rst_bitpos = 12;
	const unsigned int sw_rst_mask = 1 << sw_rst_bitpos;
	int pwrcont = 0x224;
	int pwrmode = 0x228;
	unsigned int reg;

	reg = mmio_read_32((void *)(base + pwrcont));

	reg &= ~sw_rst_enb_mask;
	reg |= 1 << sw_rst_enb_bitpos;

	mmio_write_32((void *)(base + pwrcont), reg);
	mmio_write_32((void *)(base + pwrmode), sw_rst_mask);
}

/*************************************************************
 * System Reset the Reference Fucntion
 *************************************************************/
void psci_system_reset(void)
{
	/* s5p4418 Reset  */
	s5p4418_reset_cpu();
}
