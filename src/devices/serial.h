#ifndef __SERIAL_H__
#define __SERIAL_H__

struct s5p4418_uart_reg
{
	volatile unsigned int dr;				// 0x00		// Data Register
	volatile unsigned int rsr_ecr;				// 0x04		// Receive Status Register / Error Clear Register
	volatile unsigned int reserved0[(0x18-0x08)/4];	// 0x08~0x14	// Reserved
	volatile unsigned int fr;				// 0x18		// Flag Register
	volatile unsigned int reserved1;			// 0x1C		// Reserved
	volatile unsigned int ilpr;				// 0x20		// IrDA Low Power Counter Register
	volatile unsigned int ibrd;				// 0x24		// IntegerBaud Rate Register
	volatile unsigned int fbrd;				// 0x28 		// Fractional Baud Rate Register
	volatile unsigned int LCR_H;				// 0x2C 		// Line Control Register
	volatile unsigned int CR;				// 0x30 		// Control Register
	volatile unsigned int ifls;				// 0x34		// Interrupt FIFO Level Select Register
	volatile unsigned int imsc;				// 0x38		// Interrupt Mask Set/Clear Register
	volatile unsigned int rts;				// 0x3C 		// Raw Interrupt Status Register
	volatile unsigned int mts;				// 0x40		// Masked Interrupt Status Register
	volatile unsigned int icr;				// 0x44		// Interrupt Clear Register
	volatile unsigned int dmacr;				// 0x48		// DMA Control Register
	volatile unsigned int reserved2[(0x80-0x4C)/4];	// 0x4C~0x7C	// Reserved
//	volatile unsigned int __Reserved3[(0x90-0x80)/4];	// 0x80~0x8C	// Reserved
	volatile unsigned int tcr;				// 0x80 		// Test Control Register
	volatile unsigned int itip;				// 0x84 		// Integration Test Input Register
	volatile unsigned int itop;				// 0x88		 // Integration Test Output Register
	volatile unsigned int tdr;				// 0x8C		 // Test Data Register
	volatile unsigned int reserved4[(0xFD0-0x90)/4];	// 0x90~0xFCC	// Reserved
	volatile unsigned int reserved5[(0xFE0-0xFD0)/4];	// 0xFD0~0xFDC	// Reserved
	volatile unsigned int periphid0;			// 0xFE0	// PeriphID0 Register
	volatile unsigned int periphid1;			// 0xFE4	// PeriphID1 Register
	volatile unsigned int periphid2;			// 0xFE8	// PeriphID2 Register
	volatile unsigned int periphid3;			// 0xFEC	// PeriphID3 Register
	volatile unsigned int pcellid0;				// 0xFF0	// PCellID0 Register
	volatile unsigned int pcellid1;				// 0xFF4	// PCellID1 Register
	volatile unsigned int pcellid2;				// 0xFF8	// PCellID2 Register
	volatile unsigned int pcellid3;				// 0xFFC	// PCellID3 Register
};

/* Function Define */
 int serial_init(unsigned int channel);

char serial_getch(void);
void serial_putch(char ch);

 int serial_is_uart_tx_done(void);
 int serial_is_tx_empty(void);
 int serial_is_busy(void);

#endif
