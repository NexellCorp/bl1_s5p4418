#ifndef __MAIN_H__
#define __MAIN_H__

/* Extern Function Define */
void DMC_Delay(int milisecond);

/* Set the Reset Generatior (IP Block) */
void ResetCon(unsigned int devicenum, int en);

/* EMA(Extra Margin Adjustments) Function */
void __init cache_setup_ema(void);
void ema_information(void);

/* Clock(PLL) Function */
void initClock(void);
void printClkInfo(void);

/* PMIC(power management ic) Function */
void pmic_initalize(void);

/* Bus, Drex Fuction */
void setBusConfig(void);

/* (ddr3/lpdde3) sdram memory function define */
void init_DDR3(unsigned int);
void init_LPDDR3(unsigned int);
void enterSelfRefresh(void);
void exitSelfRefresh(void);

/* Secondary CPU Function */
void s5p4418_resume(void);

/* Secondary CPU Function */
void subcpu_bringup(void);

/* Build Infomation Function */
int build_information(void);

void device_reset(void);

/* CRC Algorithm Check Function */
 int crc_check(void* buf, unsigned int size, unsigned int ref_crc);

/* NXP4330 Self Loading */
int __init nxp4330_self_boot(void);

/* Extern Boot Mode Function */
 int iUSBBOOT(struct NX_SecondBootInfo * pTBI);
 int iSDXCBOOT(struct NX_SecondBootInfo * pTBI);

#endif