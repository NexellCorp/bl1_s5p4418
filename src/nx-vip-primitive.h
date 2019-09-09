/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: Sungwoo, Park <swpark@nexell.co.kr>
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

#ifndef _NX_VIP_PRIMITIVE_H
#define _NX_VIP_PRIMITIVE_H

#define u32 unsigned int
#define u16 unsigned short
#define u8 unsigned char
#define int32_t u32
#define int16_t u16
//#define bool int
#define true 1
#define false 0
#define NULL 0
typedef int bool;

#define writel(x,y)	*y = x
//#define readl(x,y)	x = *y

//#define pr_info drDbgPrintLnf
#define pr_info printf

struct nx_vip_register_set {
	u32 vip_config;
	u32 vip_hvint;
	u32 vip_syncctrl;
	u32 vip_syncmon;
	u32 vip_vbegin;
	u32 vip_vend;
	u32 vip_hbegin;
	u32 vip_hend;
	u32 vip_fifoctrl;
	u32 vip_hcount;
	u32 vip_vcount;
	u32 __reserved;
	u32 vip_infifoclr;
	u8 __reserved00[0x200 - 0x34];
	u32 vip_cdenb;
	u32 vip_odint;
	u32 vip_imgwidth;
	u32 vip_imgheight;
	u32 clip_left;
	u32 clip_right;
	u32 clip_top;
	u32 clip_bottom;
	u32 deci_targetw;
	u32 deci_targeth;
	u32 deci_deltaw;
	u32 deci_deltah;
	int32_t deci_clearw;
	int32_t deci_clearh;
	u32 deci_luseg;
	u32 deci_crseg;
	u32 deci_cbseg;
	u32 deci_format;
	u32 deci_rotflip;
	u32 deci_luleft;
	u32 deci_crleft;
	u32 deci_cbleft;
	u32 deci_luright;
	u32 deci_crright;
	u32 deci_cbright;
	u32 deci_lutop;
	u32 deci_crtop;
	u32 deci_cbtop;
	u32 deci_lubottom;
	u32 deci_crbottom;
	u32 deci_cbbottom;
	u32 clip_luseg;
	u32 clip_crseg;
	u32 clip_cbseg;
	u32 clip_format;
	u32 clip_rotflip;
	u32 clip_luleft;
	u32 clip_crleft;
	u32 clip_cbleft;
	u32 clip_luright;
	u32 clip_crright;
	u32 clip_cbright;
	u32 clip_lutop;
	u32 clip_crtop;
	u32 clip_cbtop;
	u32 clip_lubottom;
	u32 clip_crbottom;
	u32 clip_cbbottom;
	u32 vip_scanmode;
	u32 clip_yuyvenb;
	u32 clip_baseaddrh;
	u32 clip_baseaddrl;
	u32 clip_strideh;
	u32 clip_stridel;
	u32 vip_vip1;
};

enum {
	nx_vip_int_vsync = 0ul,
	nx_vip_int_hsync = 1ul,
	nx_vip_int_done = 2ul
};

enum {
	nx_vip_clksrc_fpll = 0ul,
	nx_vip_clksrc_upll = 1ul,
	nx_vip_clksrc_iclkin = 3ul,
	nx_vip_clksrc_inviclkin = 4ul
};

enum {
	nx_vip_dataorder_cby0cry1 = 0ul,
	nx_vip_dataorder_cry1cby0 = 1ul,
	nx_vip_dataorder_y0cby1cr = 2ul,
	nx_vip_dataorder_y1cry0cb = 3ul
};

enum {
	nx_vip_fieldsel_bypass = 0ul,
	nx_vip_fieldsel_invert = 1ul,
	nx_vip_fieldsel_even = 2ul,
	nx_vip_fieldsel_odd = 3ul
};

enum {
	nx_vip_fiforeset_frameend = 0ul,
	nx_vip_fiforeset_framestart = 1ul,
	nx_vip_fiforeset_cpu = 2ul,
	nx_vip_fiforeset_all = 3ul
};

enum {
	nx_vip_fstatus_full = 1 << 0ul,
	nx_vip_fstatus_empty = 1 << 1ul,
	nx_vip_fstatus_reading = 1 << 2ul,
	nx_vip_fstatus_writing = 1 << 3ul
};

enum {
	nx_vip_format_420 = 0ul,
	nx_vip_format_422 = 1ul,
	nx_vip_format_444 = 2ul,
	nx_vip_format_l422 = 3ul,
	nx_vip_format_yuyv = 3ul,
	nx_vip_format_420_cbcr = 4ul,
	nx_vip_format_422_cbcr = 5ul,
	nx_vip_format_444_cbcr = 6ul,
	nx_vip_format_reserved00 = 7ul,
	nx_vip_format_420_crcb = 8ul,
	nx_vip_format_422_crcb = 9ul,
	nx_vip_format_444_crcb = 10ul,
	nx_vip_format_reserved01 = 11ul
};

enum {
	nx_vip_inputport_a = 0ul,
	nx_vip_inputport_b = 1ul
};

enum {
	nx_vip_vd_8bit = 0,
	nx_vip_vd_16bit = 1,
};

void nx_vip_set_base_address(u32 module_index, void *base_address);
void nx_vip_set_interrupt_enable(u32 module_index, u32 int_num, int enable);
void nx_vip_set_interrupt_enable_all(u32 module_index, int enable);
void nx_vip_clear_interrupt_pending_all(u32 module_index);
void nx_vip_set_vipenable(u32 module_index, int b_vipenb, int b_sep_enb,
			  int b_clip_enb, int b_deci_enb);
void nx_vip_set_input_port(u32 module_index, u32 input_port);
void nx_vip_set_data_mode(u32 module_index, u32 data_order,
			  u32 data_width);
void nx_vip_set_sync(u32 module_index, int b_ext_sync,
		     u32 source_bits, u32 avw, u32 avh,
		     u32 pcs, u32 hp, u32 vp, u32 hsw, u32 hfp, u32 hbp,
		     u32 vsw, u32 vfp, u32 vbp);
void nx_vip_set_hvsync(u32 module_index, int b_ext_sync, u32 avw, u32 avh,
		       u32 pcs, u32 hp, u32 vp, u32 hsw, u32 hfp, u32 hbp,
		       u32 vsw, u32 vfp, u32 vbp);
void nx_vip_set_dvalid_mode(u32 module_index, int b_ext_dvalid,
			    int b_dvalid_pol, int b_sync_pol);
void nx_vip_set_field_mode(u32 module_index, int b_ext_field,
			   u32 field_sel, int b_interlace,
			   int b_inv_field);
void nx_vip_set_fiforeset_mode(u32 module_index, u32 fiforeset);
void nx_vip_reset_fifo(u32 module_index);
void nx_vip_set_clip_region(u32 module_index, u32 left, u32 top, u32 right,
			    u32 bottom);
void nx_vip_set_clipper_format(u32 module_index, u32 format);
void nx_vip_set_clipper_addr(u32 module_index, u32 format, u32 width,
			     u32 height, u32 lu_addr, u32 cb_addr, u32 cr_addr,
			     u32 stride_y, u32 stride_cb_cr);
void nx_vip_set_decimation(u32 module_index, u32 src_width, u32 src_height,
			   u32 dst_width, u32 dst_height);
void nx_vip_set_decimator_format(u32 module_index, u32 format);
void nx_vip_set_decimator_addr(u32 module_index, u32 format,
			       u32 width, u32 height, u32 lu_addr, u32 cb_addr,
			       u32 cr_addr, u32 ystride, u32 cbcrstride);
void nx_vip_clear_input_fifo(u32 module_index);

#endif
