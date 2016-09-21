/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sangjong, Han <hans@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "sysheader.h"

#define UARTSRC 		2
#define SOURCE_DIVID		(4UL)
#define BAUD_RATE 		(115200)

extern U32 getquotient(int dividend, int divisor);
extern U32 getremainder(int dividend, int divisor);
extern void NX_CLKPWR_SetOSCFreq(U32 FreqKHz);

void ResetCon(U32 devicenum, CBOOL en);
void GPIOSetAltFunction(U32 AltFunc);
U32 NX_CLKPWR_GetPLLFrequency(U32 PllNumber);

static struct NX_UART_RegisterSet *pReg_Uart;

const U32 UARTBASEADDR[] = {
	PHY_BASEADDR_UART0_MODULE, 
	PHY_BASEADDR_pl01115_Uart_modem_MODULE,
	PHY_BASEADDR_UART1_MODULE, 
	PHY_BASEADDR_pl01115_Uart_nodma0_MODULE,
	PHY_BASEADDR_pl01115_Uart_nodma1_MODULE
};
const U32 UARTCLKGENADDR[] = {
	PHY_BASEADDR_CLKGEN22_MODULE, 
	PHY_BASEADDR_CLKGEN24_MODULE,
	PHY_BASEADDR_CLKGEN23_MODULE, 
	PHY_BASEADDR_CLKGEN25_MODULE,
	PHY_BASEADDR_CLKGEN26_MODULE
};

const U8 RESETNUM[] = {
	RESETINDEX_OF_UART0_MODULE_nUARTRST,
	RESETINDEX_OF_pl01115_Uart_modem_MODULE_nUARTRST,
	RESETINDEX_OF_UART1_MODULE_nUARTRST,
	RESETINDEX_OF_pl01115_Uart_nodma0_MODULE_nUARTRST,
	RESETINDEX_OF_pl01115_Uart_nodma1_MODULE_nUARTRST
};

const U32 GPIOALTNUM[] = {
	PADINDEX_OF_UART0_UARTRXD, PADINDEX_OF_UART0_UARTTXD,
	PADINDEX_OF_pl01115_Uart_modem_UARTRXD,	PADINDEX_OF_pl01115_Uart_modem_UARTTXD, 
	PADINDEX_OF_UART1_UARTRXD,PADINDEX_OF_UART1_UARTTXD, 
	PADINDEX_OF_pl01115_Uart_nodma0_UARTRXD,PADINDEX_OF_pl01115_Uart_nodma0_UARTTXD,
	PADINDEX_OF_pl01115_Uart_nodma1_UARTRXD,PADINDEX_OF_pl01115_Uart_nodma1_UARTTXD,
};
const U32 UARTSMC[] = {
	TIEOFFINDEX_OF_UART0_USESMC,			TIEOFFINDEX_OF_UART0_SMCTXENB,
	TIEOFFINDEX_OF_UART0_SMCRXENB,			TIEOFFINDEX_OF_UART_MODEM0_USESMC,
	TIEOFFINDEX_OF_UART_MODEM0_SMCTXENB,	TIEOFFINDEX_OF_UART_MODEM0_SMCRXENB,
	TIEOFFINDEX_OF_UART1_USESMC,			TIEOFFINDEX_OF_UART1_SMCTXENB,
	TIEOFFINDEX_OF_UART1_SMCRXENB,			TIEOFFINDEX_OF_UART_NODMA0_USESMC,
	TIEOFFINDEX_OF_UART_NODMA0_SMCTXENB,	TIEOFFINDEX_OF_UART_NODMA0_SMCRXENB,
	TIEOFFINDEX_OF_UART_NODMA1_USESMC,		TIEOFFINDEX_OF_UART_NODMA1_SMCTXENB,
	TIEOFFINDEX_OF_UART_NODMA1_SMCRXENB
};

CBOOL DebugInit(U32 ch)
{
	U32 SOURCE_CLOCK;
	struct NX_CLKGEN_RegisterSet *const pReg_UartClkGen =
		(struct NX_CLKGEN_RegisterSet *)UARTCLKGENADDR[ch];

	pReg_Uart = (struct NX_UART_RegisterSet *)UARTBASEADDR[ch];

	NX_CLKPWR_SetOSCFreq(OSC_KHZ);

	SOURCE_CLOCK = NX_CLKPWR_GetPLLFrequency(UARTSRC);

	GPIOSetAltFunction(GPIOALTNUM[ch * 2 + 0]);
	GPIOSetAltFunction(GPIOALTNUM[ch * 2 + 1]);

	pReg_Tieoff->TIEOFFREG[((UARTSMC[ch * 3 + 0]) & 0xFFFF) >> 5] &=
		(~(1 << ((UARTSMC[ch * 3 + 0]) & 0x1F)));
	pReg_Tieoff->TIEOFFREG[((UARTSMC[ch * 3 + 1]) & 0xFFFF) >> 5] &=
		(~(1 << ((UARTSMC[ch * 3 + 1]) & 0x1F)));
	pReg_Tieoff->TIEOFFREG[((UARTSMC[ch * 3 + 2]) & 0xFFFF) >> 5] &=
		(~(1 << ((UARTSMC[ch * 3 + 2]) & 0x1F)));

	ResetCon(RESETNUM[ch], CTRUE);			// reset on
	ResetCon(RESETNUM[ch], CFALSE);			// reset negate

	pReg_UartClkGen->CLKENB =(1 << 3); 		// PCLKMODE : always, Clock Gen Disable
	pReg_UartClkGen->CLKGEN[0] =
		((SOURCE_DIVID - 1) << 5) | (UARTSRC << 2);

	pReg_Uart->LCR_H = 0x0070;			// 8 bit, none parity, stop 1, normal mode
	pReg_Uart->CR = 0x0300;				// rx, tx enable

	pReg_Uart->IBRD =
		(U16)getquotient(getquotient(SOURCE_CLOCK, SOURCE_DIVID),
				((BAUD_RATE / 1) * 16)); // IBRD = 8, 115200bps
	pReg_Uart->FBRD = (U16)(getquotient(((getremainder(getquotient(SOURCE_CLOCK, SOURCE_DIVID),
					((BAUD_RATE / 1) * 16)) + 32) * 64),
					((BAUD_RATE / 1) * 16))); // FBRD = 0,
	pReg_UartClkGen->CLKENB =
		(1 << 3) | (1 << 2);			// PCLKMODE : always, Clock Gen Enable
	pReg_Uart->CR = 0x0301;				// rx, tx, uart enable

	return CTRUE;
}

void DebugPutch(S8 ch)
{
	const U16 TX_FIFO_FULL = 1 << 5;
	while (pReg_Uart->FR & TX_FIFO_FULL);
	pReg_Uart->DR = (U32)ch;
}

CBOOL DebugIsUartTxDone(void)
{
	const U16 UART_TX_BUSY = 1 << 3;
	const U16 TX_FIFO_EMPTY = 1 << 7;
	return ((pReg_Uart->FR & (UART_TX_BUSY | TX_FIFO_EMPTY)) ==
			TX_FIFO_EMPTY ? CTRUE : CFALSE);
}

CBOOL DebugIsTXEmpty(void)
{
	const U16 TX_FIFO_EMPTY = 1 << 7;
	return (CBOOL)(pReg_Uart->FR & TX_FIFO_EMPTY);
}

CBOOL DebugIsBusy(void)
{
	const U16 UART_TX_BUSY = 1 << 3;
	return (CBOOL)(pReg_Uart->FR & UART_TX_BUSY);
}

S8 DebugGetch(void)
{
	const U16 RX_FIFO_EMPTY = 1 << 4;
	while (pReg_Uart->FR & RX_FIFO_EMPTY);
	return (S8)pReg_Uart->DR;
}

#if 0
void DebugPutString(const S8 *const String)
{
	const S8 *pString;

	pString = (const S8 *)String;
	while (CNULL != *pString)
		DebugPutch(*pString++);
}

S32 DebugGetString(S8 *const pStringBuffer)
{
	S8 *pString = pStringBuffer;
	S8 buf;
	S32 iSize = 0;

	while (1) {
		/* get character */
		buf = DebugGetch();

		/* backspace */
		if (buf == 0x08) {
			if (iSize > 0) {
				DebugPutch(buf);
				DebugPutch(' ');
				DebugPutch(buf);

				pString--;
				iSize--;
			}

			continue;
		}

		/* print character */
		DebugPutch(buf);

		if (buf == '\r')
			break;
		/* increase string index */
		*pString++ = buf;
		iSize++;
	}

	*pString++ = '\0';

	return iSize;
}

void DebugPrint(const S8 *const FormatString, ...)
{
	static S8 String[256];

	va_list argptr;
	va_start(argptr, FormatString);
	vsprintf((S8 *)String, FormatString, argptr);
	va_end(argptr);

	DebugPutString(String);
}

void DebugPutDec(S32 value)
{
	S8 ch[16];
	U32 data;
	S32 i, chsize;

	data = (value < 0) ? (U32)(-value) : (U32)value;

	chsize = 0;
	do {
		ch[chsize++] = getremainder(data, 10) + '0';
		data = getquotient(data, 10);
	} while (data != 0);

	if (value < 0)
		DebugPutch('-');

	for (i = 0; i < chsize; i++) {
		DebugPutch(ch[chsize - i - 1]);
	}
}

void DebugPutHex(S32 value)
{
	S8 ch;
	U32 data;
	S32 i;

	data = (U32)value;

	DebugPutch('0');
	DebugPutch('x');

	for (i = 0; i < 8; i++) {
		ch = (S8)((data >> (28 - i * 4)) & 0xF);
		ch = (ch > 9) ? (ch - 10 + 'A') : (ch + '0');
		DebugPutch(ch);
	}
}

void DebugPutByte(S8 value)
{
	S8 ch;
	U32 data;
	S32 i;

	data = (U32)value;

	for (i = 0; i < 2; i++) {
		ch = (S8)((data >> (4 - i * 4)) & 0xF);
		ch = (ch > 9) ? (ch - 10 + 'A') : (ch + '0');
		DebugPutch(ch);
	}
}

void DebugPutWord(S16 value)
{
	S8 ch;
	U32 data;
	S32 i;

	data = (U32)value;

	for (i = 0; i < 4; i++) {
		ch = (S8)((data >> (12 - i * 4)) & 0xF);
		ch = (ch > 9) ? (ch - 10 + 'A') : (ch + '0');
		DebugPutch(ch);
	}
}
#endif
