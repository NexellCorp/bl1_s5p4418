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
 *      Module          : Clock Generator
 *      File            : nx_clkgen.h
 *      Description     :
 *      Author          : SoC Team
 *      History         :
 */
#ifndef __NX_CLKGEN_H__
#define __NX_CLKGEN_H__

#include "../base/nx_prototype.h"

#ifdef	__cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @defgroup
//------------------------------------------------------------------------------
//@{

	struct	NX_CLKGEN_RegisterSet
	{
		volatile U32	CLKENB;			///< 0x40 : Clock Enable Register
		volatile U32	CLKGEN[4];			///< 0x44 : Clock Generate Register
	};


	//enum BCLKMODE
	//{
	//	BCLKMODE_DISABLE,
	//	BCLKMODE_DYNAMIC,
	//	BCLKMODE_ALWAYS,
	//	BCLKMODE_FORCE32 = 0x7fffffff
	//};


	//struct ClockGroupRegisterSet
	//{
    //	volatile U32 CLKENB;
	//    volatile U32 CLKGEN[1];
	//};
	CBOOL		NX_CLKGEN_Initialize( void );
	U32			NX_CLKGEN_GetNumberOfModule( void );
	U32 		NX_CLKGEN_GetPhysicalAddress( U32 ModuleIndex );
	U32			NX_CLKGEN_GetSizeOfRegisterSet( void );
	void		NX_CLKGEN_SetBaseAddress( U32 ModuleIndex, void* BaseAddress );
	void*		NX_CLKGEN_GetBaseAddress( U32 ModuleIndex );

	void		NX_CLKGEN_SetClockPClkMode( U32 ModuleIndex, NX_PCLKMODE mode );
	NX_PCLKMODE	NX_CLKGEN_GetClockPClkMode( U32 ModuleIndex );
	void		NX_CLKGEN_SetClockSource( U32 ModuleIndex, U32 Index, U32 ClkSrc );
	U32			NX_CLKGEN_GetClockSource( U32 ModuleIndex, U32 Index );
	void		NX_CLKGEN_SetClockDivisor( U32 ModuleIndex, U32 Index, U32 Divisor );
	U32			NX_CLKGEN_GetClockDivisor( U32 ModuleIndex, U32 Index );
	void		NX_CLKGEN_SetClockDivisorEnable( U32 ModuleIndex, CBOOL Enable );
	CBOOL		NX_CLKGEN_GetClockDivisorEnable( U32 ModuleIndex );

	void		NX_CLKGEN_SetClockBClkMode( U32 ModuleIndex, NX_BCLKMODE mode );
	NX_BCLKMODE	NX_CLKGEN_GetClockBClkMode( U32 ModuleIndex );

	//void		NX_CLKGEN_SetClockInInv( U32 ModuleIndex, CBOOL OutClkInv );
	//CBOOL		NX_CLKGEN_GetClockInInv( U32 ModuleIndex );

	void		NX_CLKGEN_SetClockOutInv( U32 ModuleIndex, U32 Index, CBOOL OutClkInv );
	CBOOL		NX_CLKGEN_GetClockOutInv( U32 ModuleIndex, U32 Index );

	CBOOL		NX_CLKGEN_SetInputInv( U32 ModuleIndex, U32 Index, CBOOL OutClkInv );
	CBOOL		NX_CLKGEN_GetInputInv( U32 ModuleIndex, U32 Index );

	void		NX_CLKGEN_SetClockOutShift( U32 ModuleIndex, CBOOL OutClkSft );
	CBOOL		NX_CLKGEN_GetClockOutShift( U32 ModuleIndex );
	void		NX_CLKGEN_SetClockOutDelay( U32 ModuleIndex, U32 OutClkDelay );
	U32			NX_CLKGEN_GetClockOutDelay( U32 ModuleIndex );


#ifdef	__cplusplus
}
#endif

#endif // __NX_CLKGEN_H__