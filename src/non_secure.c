#include "sysheader.h"

#define TIEOFFREG26	0x1068
#define TIEOFFREG29	0x1070

void tieoff_set_secure(void)
{
	void *base = (void *)0xC0010000;

	SetIO32((base + TIEOFFREG26), 0xFFFFFFFF);	
//	SetIO32((base + TIEOFFREG29), 0x1FF);		//VIP, DISP, SCALER
}

