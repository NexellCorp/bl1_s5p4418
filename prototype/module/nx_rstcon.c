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
 *      Module          : Reset Controller
 *      File            : nx_rstcon.c
 *      Description     :
 *      Author          : SoC Team
 *      History         :
 */
#include "nx_rstcon.h"

static struct NX_RSTCON_RegisterSet *__g_pRegister;


CBOOL	NX_RSTCON_Initialize( void )
{
	static CBOOL bInit = CFALSE;

	if( CFALSE == bInit )
	{
		__g_pRegister = CNULL;
		bInit = CTRUE;
	}
	return CTRUE;
}

U32  NX_RSTCON_GetPhysicalAddress( void )
{
    const U32 PhysicalAddr[] =  {   PHY_BASEADDR_LIST( RSTCON )  }; // PHY_BASEADDR_RSTCON_MODULE
    NX_CASSERT( NUMBER_OF_RSTCON_MODULE == (sizeof(PhysicalAddr)/sizeof(PhysicalAddr[0])) );
    NX_ASSERT( PHY_BASEADDR_RSTCON_MODULE == PhysicalAddr[0] );
    return (U32)PhysicalAddr[0];
}

U32	 NX_RSTCON_GetSizeOfRegisterSet( void )
{
	return sizeof(struct NX_RSTCON_RegisterSet);
}

void NX_RSTCON_SetBaseAddress( void* BaseAddress )
{
	NX_ASSERT( CNULL != BaseAddress );
	__g_pRegister = (struct NX_RSTCON_RegisterSet *)BaseAddress;
}

void* NX_RSTCON_GetBaseAddress( void )
{
	return (void*)__g_pRegister;
}

void		NX_RSTCON_SetnRST(U32 RSTIndex, RSTCON_nRST STATUS)
{
	U32 regNum, bitPos, curStat;
	regNum 		= RSTIndex >> 5;
	curStat		= (U32)ReadIO32(&__g_pRegister->REGRST[regNum]);
	bitPos		= RSTIndex & 0x1f;
	curStat		&= ~(1UL << bitPos);
	curStat		|= (STATUS & 0x01) << bitPos;
	WriteIO32(&__g_pRegister->REGRST[regNum], curStat);
}

void		NX_RSTCON_SetRST(U32 RSTIndex, RSTCON_RST STATUS)
{
	U32 regNum, bitPos, curStat;
	regNum 		= RSTIndex >> 5;
	curStat		= (U32)ReadIO32(&__g_pRegister->REGRST[regNum]);
	bitPos		= RSTIndex & 0x1f;
	curStat		&= ~(1UL << bitPos);
	curStat		|= (STATUS & 0x01) << bitPos;
	WriteIO32(&__g_pRegister->REGRST[regNum], curStat);
}

RSTCON_nRST		NX_RSTCON_GetnRST(U32 RSTIndex)
{
	U32 regNum, bitPos, curStat;
	regNum 		= RSTIndex >> 5;
	curStat		= (U32)ReadIO32(&__g_pRegister->REGRST[regNum]);
	bitPos	= RSTIndex & 0x1f;
	curStat		= 0x01 & (curStat >> bitPos);
	return (RSTCON_nRST) curStat;
}

RSTCON_RST		NX_RSTCON_GetRST(U32 RSTIndex)
{
	U32 regNum, bitPos, curStat;
	regNum 		= RSTIndex >> 5;
	curStat		= (U32)ReadIO32(&__g_pRegister->REGRST[regNum]);
	bitPos	= RSTIndex & 0x1f;
	curStat		= 0x01 & (curStat >> bitPos);

	return (RSTCON_RST) curStat;
}

