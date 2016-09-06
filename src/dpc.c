#include <sysheader.h>

#define DPC_BASEADDR		0xC0102800

#define DPCCTRL0		0x118

static void* dpc_get_baseaddr(unsigned int module)
{
	return (void*)(DPC_BASEADDR + (module * 0x4000));
}


void dpc_set_enable_all(unsigned int module, unsigned int enb)
{
	volatile void *base = dpc_get_baseaddr(module);
	unsigned int value;

	value = mmio_read_32(base + DPCCTRL0);

	value &= ~(1 << 15);
	value |= (U32)enb << 15;

	mmio_write_32((base + DPCCTRL0), value);
}

int dpc_get_pending_all(unsigned int module)
{
	volatile void *base = dpc_get_baseaddr(module);
	return (mmio_read_32(base + DPCCTRL0) >> 10);
}

void dpc_clear_pending_all(unsigned int module)
{
	volatile void* base = (void*)dpc_get_baseaddr(module);
	unsigned int value;

	value = mmio_read_32(base + DPCCTRL0);
	value |= (1 << 10);

	mmio_write_32((base + DPCCTRL0), value);
}

int  dpc_enabled(unsigned int module)
{
	volatile void* base = (void*)dpc_get_baseaddr(module);
	unsigned int value;

	value = mmio_read_32(base + DPCCTRL0);

	return value & (1 << 15);
}
