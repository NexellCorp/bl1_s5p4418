#ifndef __CLKPWR_H__
#define __CLKPWR_H__

struct	s5p4418_clkpwr_reg
{
		volatile unsigned int clk_mode_reg0;				///< 0x000 : Clock Mode Register 0
		volatile unsigned int reserved_0;				///< 0x004
		volatile unsigned int pllset[4];				///< 0x008 ~ 0x014 : PLL Setting Register
		volatile unsigned int reserved_1[2];				///< 0x018 ~ 0x01C
		volatile unsigned int dvo[5];					///< 0x020 ~ 0x030 : Divider Setting Register
		volatile unsigned int reserved_2[5];				///< 0x034 ~ 0x044
		volatile unsigned int pllset_sscg[6];				///< 0x048 ~ 0x05C
		volatile unsigned int reserved_3[8];				///< 0x060 ~ 0x07C
		volatile unsigned int reserved_4[(512-128)/4];			// padding (0x200-..)/4
		volatile unsigned int gpio_wakeup_rise_enb;			///< 0x200 : GPIO Rising Edge Detect Enable Register
		volatile unsigned int gpio_wakeup_fall_enb;			///< 0x204 : GPIO Falling Edge Detect Enable Registe
		volatile unsigned int gpio_rst_enb;				///< 0x208 : GPIO Reset Enable Register
		volatile unsigned int gpio_wakeup_enb;				///< 0x20C : GPIO Wakeup Source Enable
		volatile unsigned int gpio_int_enb;				///< 0x210 : Interrupt Enable Register
		volatile unsigned int gpio_int_pend;				///< 0x214 : Interrupt Pend Register
		volatile unsigned int reset_status;				///< 0x218 : Reset Status Register
		volatile unsigned int int_enable;				///< 0x21C : Interrupt Enable Register
		volatile unsigned int int_pend;					///< 0x220 : Interrupt Pend Register
		volatile unsigned int pwr_cont;					///< 0x224 : Power Control Register
		volatile unsigned int pwr_mode;					///< 0x228 : Power Mode Register
		volatile unsigned int reserved_5;				///< 0x22C : Reserved Region
		volatile unsigned int scratch[3];				///< 0x230 ~ 0x238	: Scratch Register
		volatile unsigned int sysrst_config;				///< 0x23C : System Reset Configuration Register.
		volatile unsigned char reserved_6[0x100-0x80];			///< 0x80 ~ 0xFC	: Reserved Region
		volatile unsigned int pad_strength_gpio[5][2];			///< 0x100, 0x104 : GPIOA Pad Strength Register
										///< 0x108, 0x10C : GPIOB Pad Strength Registe
										///< 0x110, 0x114 : GPIOC Pad Strength Registe
										///< 0x118, 0x11C : GPIOD Pad Strength Registe
										///< 0x120, 0x124 : GPIOE Pad Strength Register
		volatile unsigned int resetved_7[2];				///< 0x128 ~ 0x12C: Reserved Region
		volatile unsigned int pad_strength_bus;				///< 0x130 : Bus Pad Strength Register
};

/* Function Define */
 int clkpwr_get_baseaddr(void);
 int clock_set_mem_pll(int ca_after);

void clkpwr_set_oscfreq(unsigned int freq_khz);
 int clkpwr_get_pllfreq(unsigned int pll_num);
 int clkpwr_get_srcpll(unsigned int divider);
 int clkpwr_get_divide_value(unsigned int divider);

#endif	// #ifndef __CLKPWR_H__

