#ifndef __GIC_H__
#define __GIC_H__

#define GIC_CPUIF_CTRL		0x0
#define GIC_CPUIF_PRIORTY	0x104

#define GIC_DIST_CTRL		0x0
#define GIC_DIST_GROUP		0x80
#define GIC_DIST_TARGET		0x800
#define GIC_DIST_SGIR		0xF00
#define GIC_DIST_SGIMASK	0xF20

unsigned char* gic_cpuif_get_baseaddr(void);
unsigned char* gic_disp_get_baseaddr(void);

#endif
