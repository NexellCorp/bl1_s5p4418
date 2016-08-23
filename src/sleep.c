#include <sysheader.h>

extern U32  g_GateCycle;
extern U32  g_GateCode;
extern U32  g_RDvwmc;
extern U32  g_WRvwmc;

#if (CONFIG_SUSPEND_RESUME == 1)
extern void enterSelfRefresh(void);
extern U32 __calc_crc(void *addr, int len);
extern void DMC_Delay(int milisecond);

void s5pxx18_resume(void)
{
	U32 kernel_addr, signature, mem, ref_crc, len;
	void (*jumpkernel)(void) = 0;

	WriteIO32(&pReg_Alive->ALIVEPWRGATEREG, 1); 			// open alive power gate
	signature   = ReadIO32(&pReg_Alive->ALIVESCRATCHREADREG);
	kernel_addr = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE1);
	ref_crc     = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE2);
	mem = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE3);
	len = ReadIO32(&pReg_Alive->ALIVESCRATCHVALUE4);
	jumpkernel = (void (*)(void))kernel_addr;

	WriteIO32(&pReg_Alive->ALIVESCRATCHRSTREG, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST1, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST2, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST3, 0xFFFFFFFF);
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST4, 0xFFFFFFFF);

	if (SUSPEND_SIGNATURE == signature) {
		unsigned int crc = __calc_crc((void *)mem, len);
		printf("Reference CRC : 0x%08X, Calcurated CRC : 0x%08X \r\n", ref_crc, crc);
		if (kernel_addr && (ref_crc == crc)) {
			printf("It's WARM BOOT\r\nJump to Kernel!\r\n");
			printf("Kernel Address : %08X(%08X) \r\n", jumpkernel, kernel_addr );
			while(!DebugIsUartTxDone());
			jumpkernel();
		}
	} else {
		printf("Suspend Signature is different\r\nRead Signature :0x%08X\r\n", signature);
	}

	printf("It's COLD BOOT\r\n");
}

static void suspend_vdd_pwroff(void)
{
	ClearIO32( &pReg_ClkPwr->PWRCONT, (0xFF << 8));			// Clear USE_WFI & USE_WFE bits for STOP mode.
	WriteIO32( &pReg_Alive->ALIVEPWRGATEREG, 0x00000001 );		// alive power gate open

#if 1
	/* Save leveling & training values.*/

	WriteIO32(&pReg_Alive->ALIVESCRATCHRST5,    0xFFFFFFFF);        // clear - ctrl_shiftc
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST6,    0xFFFFFFFF);        // clear - ctrl_offsetC
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST7,    0xFFFFFFFF);        // clear - ctrl_offsetr
	WriteIO32(&pReg_Alive->ALIVESCRATCHRST8,    0xFFFFFFFF);        // clear - ctrl_offsetw

	WriteIO32(&pReg_Alive->ALIVESCRATCHSET5,    g_GateCycle);       // store - ctrl_shiftc
	WriteIO32(&pReg_Alive->ALIVESCRATCHSET6,    g_GateCode);        // store - ctrl_offsetc
	WriteIO32(&pReg_Alive->ALIVESCRATCHSET7,    g_RDvwmc);          // store - ctrl_offsetr
	WriteIO32(&pReg_Alive->ALIVESCRATCHSET8,    g_WRvwmc);          // store - ctrl_offsetw
#endif
	WriteIO32( &pReg_Alive->VDDOFFCNTVALUERST,  0xFFFFFFFF );       // clear delay counter, refrence rtc clock
	WriteIO32( &pReg_Alive->VDDOFFCNTVALUESET,  0x00000001 );       // set minimum delay time for VDDPWRON pin. 1 cycle per 32.768Kh (about 30us)

	__asm__ __volatile__ ("cpsid i");                               // core interrupt off.
	WriteIO32( &pReg_Alive->VDDCTRLSETREG,      0x000003FC );       // Retention off (Pad hold off)
	WriteIO32( &pReg_Alive->VDDCTRLRSTREG,      0x00000001 );       // vddpoweron off, start counting down.

	DMC_Delay(600);     // 600 : 110us, Delay for Pending Clear. When CPU clock is 400MHz, this value is minimum delay value.

	WriteIO32( &pReg_Alive->ALIVEGPIODETECTPENDREG, 0xFF );         // all alive pend pending clear until power down.
//	WriteIO32( &pReg_Alive->ALIVEPWRGATEREG, 0x00000000 );		// alive power gate close

	while(1) {
//		SetIO32  (&pReg_ClkPwr->PWRMODE, (0x1 << 1));		// enter STOP mode.
		WriteIO32(&pReg_ClkPwr->PWRMODE, (0x1 << 1)); 		// enter STOP mode.
		__asm__ __volatile__ ("wfi");                           // now real entering point to stop mode.
	}                                                               // this time, core power will off and so cpu will die.
}


void s5pxx18_suspend(void)
{
	enterSelfRefresh();
	suspend_vdd_pwroff();
}
#endif	// #if (CONFIG_SUSPEND_RESUME == 1)
