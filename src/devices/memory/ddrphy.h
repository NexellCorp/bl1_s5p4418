#ifndef __DDR_PHY_H__
#define __DDR_PHY_H__

enum {
    RD_DESKEW_CODE          = 0x1,
    RD_DESKEW_CLEAR         = 0x2,
    WR_DESKEW_CODE          = 0x3,
    WR_DESKEW_CLEAR         = 0x4,

    VWM_LEFT                = 0x5,  // Valid Window Margin LEFT Value.
    VWM_RIGHT               = 0x6,  // Valid Window Margin RIGHT Value.
    VWM_CENTER              = 0x7,  // Valid Window Margin CENTER Value.

    RD_VWMC                 = 0x8,  // READ Valid Window Margin Center Value.
    WR_VWMC                 = 0x9,  // WRITE Valid Window Margin Center Value.
    DM_VWMC                 = 0xA,  // DM Valid Window Margin Center Value.
    GATE_VWMC               = 0xB,  // GATE Valid Window Margin Center Value.
    VWM_FAIL_STATUS         = 0xC,  // Valid Window Margin Search Fail Status.

    GATE_CENTER_CYCLE       = 0xD,  // Gate Center Cycle Adjust Value.
    GATE_CENTER_CODE        = 0xE,  // Gate Center Code Value.
};

struct  s5p4418_ddrphy_reg {
	unsigned int PHY_CON[44];
};

#endif // __DDR_PHY_H__
