/*
 *      Copyright (C) 2012 Nexell Co., All Rights Reserved
 *      Nexell Co. Proprietary & Confidential
 *
 *      NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
 *      AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
 *      BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
 *	FITNESS
 *      FOR A PARTICULAR PURPOSE.
 *
 *      Module          : 
 *      File            : ResetCon.c
 *      Description     :
 *      Author          : Hans
 *      History         : 2013.01.10 First implementation
 */
#include "sysheader.h"

static unsigned int RESET_IDX_LIST[2] =
{
	RESETINDEX_OF_TIMER_MODULE_PRESETn,
	RESETINDEX_OF_PWM_MODULE_PRESETn
};

void ResetCon(U32 devicenum, CBOOL en)
{
	if (en)
		ClearIO32(&pReg_RstCon->REGRST[(devicenum >> 5) & 0x3],
			  (0x1 << (devicenum & 0x1F))); // reset
	else
		SetIO32(&pReg_RstCon->REGRST[(devicenum >> 5) & 0x3],
			(0x1 << (devicenum & 0x1F))); // reset negate
}

void device_reset(void)
{
	unsigned int i;
	for (i = 0; i < (sizeof(RESET_IDX_LIST)/sizeof(unsigned int)); i++) {
		ResetCon(RESET_IDX_LIST[i], CTRUE);	// reset on
		ResetCon(RESET_IDX_LIST[i], CFALSE);	// reset negate
	}
}


