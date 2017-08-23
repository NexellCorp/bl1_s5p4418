#ifndef __MAIN_H__
#define __MAIN_H__

/* Extern Function Define */
void DMC_Delay(int milisecond);

/* Set the Reset Generatior (IP Block) */
void ResetCon(unsigned int devicenum, int en);

/* EMA(Extra Margin Adjustments) Function */
void __init cache_setup_ema(void);
void ema_information(void);


/* Bus, Drex Fuction */
void setBusConfig(void);

/* (ddr3/lpdde3) sdram memory function define */
void enter_self_refresh(void);
void exit_self_refresh(void);

/* Secondary CPU Function */
void subcpu_bringup(void);

/* Build Infomation Function */
int build_information(void);


/* CRC Algorithm Check Function */
 int crc_check(void* buf, unsigned int size, unsigned int ref_crc);

/* NXP4330 Self Loading */
int __init nxp4330_self_boot(void);

/* Memory Test (Optional) */
void simple_memtest(void);
 int standard_memtester(void);

/* Extern Boot Mode Function */
void plat_load(struct sbi_header *tbi);

#endif