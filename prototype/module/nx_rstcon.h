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
 *      Module          : Reset Generator
 *      File            : nx_rstcon.h
 *      Description     :
 *      Author          : SoC Team
 *      History         :
 */
#ifndef __NX_RSTCON_H__
#define __NX_RSTCON_H__

#include "../base/nx_prototype.h"

#define __def_RSTCON__RSTREGISTERCNT 1

#ifdef	__cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
/// @defgroup
//------------------------------------------------------------------------------
//@{

	struct	NX_RSTCON_RegisterSet
	{
		volatile U32	REGRST[__def_RSTCON__RSTREGISTERCNT];			//
	};

	typedef enum
	{
		RSTCON_nDISABLE = 0UL,
		RSTCON_nENABLE	= 1UL
	}RSTCON_nRST;

	typedef enum
	{
		RSTCON_ENABLE	= 1UL,
		RSTCON_DISABLE	= 0UL
	}RSTCON_RST;


	//struct ClockGroupRegisterSet
	//{
    //	volatile U32 CLKENB;
	//    volatile U32 CLKGEN[1];
	//};
	CBOOL		NX_RSTCON_Initialize( void );
	U32 		NX_RSTCON_GetPhysicalAddress( void );
	U32			NX_RSTCON_GetSizeOfRegisterSet( void );
	void		NX_RSTCON_SetBaseAddress( void* BaseAddress );
	void*		NX_RSTCON_GetBaseAddress( void );

	void		NX_RSTCON_SetnRST(U32 RSTIndex, RSTCON_nRST STATUS);
	void		NX_RSTCON_SetRST(U32 RSTIndex, RSTCON_RST STATUS);
	RSTCON_nRST	NX_RSTCON_GetnRST(U32 RSTIndex);
	RSTCON_RST	NX_RSTCON_GetRST(U32 RSTIndex);



#ifdef	__cplusplus
}
#endif

#endif // __NX_CLKGEN_H__