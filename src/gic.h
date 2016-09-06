#ifndef __GIC_H__
#define __GIC_H__

#define GICC_BASEADDR		0xF0000100
#define GICD_BASEADDR		0xF0001000

#define GIC_CPUIF_CTRL		0x0
#define GIC_CPUIF_PRIORTY	0x004
#define GIC_CPUIF_IAR		0x00C
#define GIC_CPUIF_EOIR		0x10
#define GIC_CPUIF_PPR		0x14
#define GIC_CPUIF_HPPIR		0x18
#define GIC_CPUIF_AIAR		0x20
#define GIC_CPUIF_AEOIR		0x24

#define GIC_DIST_CTRL		0x0
#define GIC_DIST_GROUP		0x80
#define GIC_DIST_SENABLE	0x100 	// SGI and PPI : 0x100, SPIs : 0x104 ~ 0x13C
#define GIC_DIST_CENABLE	0x180	// SGI and PPI : 0x180, SPIs : 0x184 ~ 0x1BC

#define GIC_DIST_SPEND		0x200	// 0x200 ~ 0x23C
#define GIC_DIST_CPEND		0x280	// 0x280 ~ 0x2BC
#define GIC_DIST_CACTI		0x380	// 0x380 ~ 0x3BC

#define GIC_DIST_TARGET		0x800
#define GIC_DIST_SGIR		0xF00
#define GIC_DIST_CPENDSGIR	0xF10
#define GIC_DIST_SPENDSGIR	0xF20

unsigned char* gicc_get_baseaddr(void);
unsigned int gicc_get_iar(void* base);

void gicc_set_ctrl(void* base, int val);
void gicc_set_eoir(void* base, int val);

unsigned char* gicd_get_baseaddr(void);

void gicd_set_enable(void* base, int val);
void gicd_set_group(void* base, int val);
void gicd_set_sgir(void* base, int val);

#endif  /* __GIC_V1_H__ */