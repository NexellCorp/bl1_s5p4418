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
#include <nx_chip.h>
#include "nx-vip-primitive.h"

static struct nx_vip_register_set *__g_p_register[NUMBER_OF_VIP_MODULE];

void nx_vip_set_base_address(u32 module_index, void *base_address)
{
	__g_p_register[module_index] =
		(struct nx_vip_register_set *)base_address;
}

void nx_vip_set_interrupt_enable(u32 module_index, u32 int_num, int enable)
{
	register u16 regvalue;
	register struct nx_vip_register_set *p_register;
	const u16 odintenb_bitpos = 8;

	p_register = __g_p_register[module_index];
	if (2 > int_num) {
		regvalue = p_register->vip_hvint & ~(1 << (int_num));
		if (enable)
			regvalue |= (1 << (int_num));
		else
			regvalue &= ~(1 << (int_num));
		writel(regvalue, &p_register->vip_hvint);
	} else {
		if (enable)
			writel((1 << odintenb_bitpos), &p_register->vip_odint);
		else
			writel(0, &p_register->vip_odint);
	}
}

void nx_vip_set_interrupt_enable_all(u32 module_index, int enable)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	if (enable) {
		writel((u16)(0x03 << 8), &p_register->vip_hvint);
		writel((u16)(0x01 << 8), &p_register->vip_odint);
	} else {
		writel((u16)0x00, &p_register->vip_hvint);
		writel((u16)0x00, &p_register->vip_odint);
	}
}

void nx_vip_clear_interrupt_pending_all(u32 module_index)
{
	register struct nx_vip_register_set *p_register;
	register u16 regvalue;
	const u16 vhsintpend_mask = 0x03;

	const u16 odintpend_mask = 0x01;

	p_register = __g_p_register[module_index];
	regvalue = p_register->vip_hvint;
	regvalue |= vhsintpend_mask;
	writel(regvalue, &p_register->vip_hvint);
	regvalue = p_register->vip_odint;
	regvalue |= odintpend_mask;
	writel(regvalue, &p_register->vip_odint);
}

void nx_vip_set_vipenable(u32 module_index, int b_vipenb, int b_sep_enb,
			  int b_clip_enb, int b_deci_enb)
{
	register u16 temp;
	register struct nx_vip_register_set *p_register;
	const u16 vipenb = 1u << 0;

	const u16 sepenb = 1u << 8;

	const u16 clipenb = 1u << 1;

	const u16 decienb = 1u << 0;

	p_register = __g_p_register[module_index];
	temp = 0;
	if (b_sep_enb)
		temp |= (u16)sepenb;
	if (b_clip_enb)
		temp |= (u16)clipenb;
	if (b_deci_enb)
		temp |= (u16)decienb;
	writel(temp, &p_register->vip_cdenb);
	temp = p_register->vip_config;
	if (b_vipenb)
		temp |= (u16)vipenb;
	else
		temp &= (u16)~vipenb;
	writel(temp, &p_register->vip_config);
}

void nx_vip_set_input_port(u32 module_index, u32 input_port)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u32)input_port, &p_register->vip_vip1);
}

void nx_vip_set_data_mode(u32 module_index, u32 data_order, u32 data_width)
{
	const u32 dorder_pos = 2;

	const u32 dorder_mask = 3ul << dorder_pos;

	const u32 dwidth_pos = 1;

	const u32 dwidth_mask = 1ul << dwidth_pos;

	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_config;
	temp &= ~(dorder_mask | dwidth_mask);
	temp |= ((u32)data_order << dorder_pos);
	temp |= ((8 == data_width) ? dwidth_mask : 0);
	writel((u16)temp, &p_register->vip_config);
}

void nx_vip_set_sync(u32 module_index, int b_ext_sync,
		     u32 source_bits, u32 avw, u32 avh,
		     u32 pcs, u32 hp, u32 vp, u32 hsw, u32 hfp,
		     u32 hbp, u32 vsw, u32 vfp, u32 vbp)
{
	const u32 drange = 1ul << 9;
	const u32 extsyncenb = 1ul << 8;

	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel(0xffffffff, &p_register->vip_vbegin);
	writel(0xffffffff, &p_register->vip_vend);
	writel(0xffffffff, &p_register->vip_hbegin);
	writel(0xffffffff, &p_register->vip_hend);

	temp = (u32)p_register->vip_config;
	temp &= ~drange;
	if (b_ext_sync)
		temp |= extsyncenb;
	else
		temp &= ~extsyncenb;
	writel((u16)temp, &p_register->vip_config);

	switch (source_bits) {
	case nx_vip_vd_8bit:
		if (b_ext_sync)
			writel((u16)(avw >> 1), &p_register->vip_imgwidth);
		else
			writel((u16)(avw >> 1) + 2, &p_register->vip_imgwidth);
		writel((u16)avh, &p_register->vip_imgheight);
		break;
	case nx_vip_vd_16bit:
		writel((u16)avw, &p_register->vip_imgwidth);
		writel((u16)avh, &p_register->vip_imgheight);
		break;
	default:
		break;
	}

	if (b_ext_sync) {
		if (0 != hp) {
			temp = (u32)p_register->vip_syncctrl;
			temp &= ~(1 << 8);
			temp |= (hp << 8);
			writel((u16)temp, &p_register->vip_syncctrl);
		}
		if (0 != vp) {
			temp = (u32)p_register->vip_syncctrl;
			temp &= ~(1 << 9);
			temp |= (vp << 9);
			writel((u16)temp, &p_register->vip_syncctrl);
		}
		if (0 != vbp) {
			temp = (u32)p_register->vip_syncctrl;
			temp &= ~(3 << 11);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(vbp - 1), &p_register->vip_vbegin);
			writel((u16)(vbp + avh - 1), &p_register->vip_vend);
		} else {
			temp = (u32)p_register->vip_syncctrl;
			temp |= (3 << 11);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(vfp + 1), &p_register->vip_vbegin);
			writel((u16)(vfp + vsw + 1), &p_register->vip_vend);
		}
		if (0 != hbp) {
			temp = (u32)p_register->vip_syncctrl;
			temp &= ~(1 << 10);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(hbp - 1), &p_register->vip_hbegin);
			writel((u16)(hbp + avw - 1), &p_register->vip_hend);
		} else {
			temp = (u32)p_register->vip_syncctrl;
			temp |= (1 << 10);
			writel((u16)temp, &p_register->vip_syncctrl);
			writel((u16)(hfp), &p_register->vip_hbegin);
			writel((u16)(hfp + hsw), &p_register->vip_hend);
		}
	} else {
		writel((u16)(vfp + 1), &p_register->vip_vbegin);
		writel((u16)(vfp + vsw + 1), &p_register->vip_vend);
		writel((u16)(hfp - 7), &p_register->vip_hbegin);
		writel((u16)(hfp + hsw - 7), &p_register->vip_hend);
	}
}

void nx_vip_set_hvsync(u32 module_index, int b_ext_sync, u32 avw, u32 avh,
		       u32 pcs, u32 hp, u32 vp, u32 hsw, u32 hfp, u32 hbp,
		       u32 vsw, u32 vfp, u32 vbp)
{
	nx_vip_set_sync(module_index, b_ext_sync, nx_vip_vd_8bit, avw, avh,
			pcs, hp, vp, hsw, hfp, hbp, vsw, vfp, vbp);
}

void nx_vip_set_dvalid_mode(u32 module_index, int b_ext_dvalid,
			    int b_dvalid_pol, int b_sync_pol)
{
	const u32 dvalidpol = 1ul << 4;
	const u32 extdvenb = 1ul << 2;
	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_syncctrl;
	if (b_ext_dvalid)
		temp |= extdvenb;
	else
		temp &= ~extdvenb;
	if (b_dvalid_pol)
		temp |= dvalidpol;
	else
		temp &= ~dvalidpol;
	temp &= ~((1 << 8) | (1 << 9));
	temp |= (((b_sync_pol & 0x1) << 8) | ((b_sync_pol & 0x1) << 9));
	writel((u16)temp, &p_register->vip_syncctrl);
}

void nx_vip_set_field_mode(u32 module_index, int b_ext_field,
			   u32 field_sel, int b_interlace,
			   int b_inv_field)
{
	const u32 extfieldenb = 1ul << 3;
	const u32 nx_vip_fieldsel_mask = 3ul << 0;
	const u32 interlaceenb = 1ul << 1;
	const u32 fieldinv = 1ul << 0;
	register u32 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = (u32)p_register->vip_syncctrl;
	if (b_ext_field)
		temp |= extfieldenb;
	else
		temp &= ~extfieldenb;
	temp &= ~nx_vip_fieldsel_mask;
	temp |= (u32)(field_sel);
	writel((u16)temp, &p_register->vip_syncctrl);
	writel((u16)(((b_interlace) ? interlaceenb : 0) |
		     ((b_inv_field) ? fieldinv : 0)),
	       &p_register->vip_scanmode);
}

void nx_vip_set_fiforeset_mode(u32 module_index, u32 fiforeset)
{
	const u32 resetfifosel_pos = 1ul;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)(fiforeset << resetfifosel_pos), &p_register->vip_fifoctrl);
}

void nx_vip_reset_fifo(u32 module_index)
{
	const u16 resetfifo = 1u << 0;
	register u16 temp;
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	temp = p_register->vip_fifoctrl;
	writel((u16)(temp | resetfifo), &p_register->vip_fifoctrl);
	writel((u16)temp, &p_register->vip_fifoctrl);
}

void nx_vip_set_clip_region(u32 module_index, u32 left, u32 top, u32 right,
			    u32 bottom)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)left, &p_register->clip_left);
	writel((u16)right, &p_register->clip_right);
	writel((u16)top, &p_register->clip_top);
	writel((u16)bottom, &p_register->clip_bottom);
}

static u32 deci_src_width[3];
static u32 deci_src_height[3];
void nx_vip_set_decimation(u32 module_index, u32 src_width, u32 src_height,
			   u32 dst_width, u32 dst_height)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)dst_width, &p_register->deci_targetw);
	writel((u16)dst_height, &p_register->deci_targeth);
	writel((u16)(src_width - dst_width), &p_register->deci_deltaw);
	writel((u16)(src_height - dst_height), &p_register->deci_deltah);
	writel((int16_t)((dst_width << 1) - src_width),
	       (u16 *)&p_register->deci_clearw);
	writel((int16_t)((dst_height << 1) - src_height),
	       (u16 *)&p_register->deci_clearh);
	deci_src_width[module_index] = src_width;
	deci_src_height[module_index] = src_height;
}

void nx_vip_set_clipper_format(u32 module_index, u32 format)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
#ifdef CONFIG_ARCH_S5P4418
	if (format == nx_vip_format_yuyv) {
		writel(1, &p_register->clip_yuyvenb);
	} else {
		writel(0, &p_register->clip_yuyvenb);
#endif
	writel((u16)format, &p_register->clip_format);

#ifdef CONFIG_ARCH_S5P4418
	}
	writel(0, &p_register->clip_rotflip);
#endif
}

void nx_vip_set_decimator_format(u32 module_index, u32 format)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel((u16)format, &p_register->deci_format);
}

void nx_vip_set_clipper_addr(u32 module_index, u32 format, u32 width,
			     u32 height, u32 lu_addr, u32 cb_addr, u32 cr_addr,
			     u32 stride_y, u32 stride_cb_cr)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];

	if (format == nx_vip_format_420) {
		register u32 segment, left, top;

		segment = lu_addr >> 30;
		left = lu_addr & 0x00007fff;
		top = (lu_addr & 0x3fff8000) >> 15;

		writel(segment, &p_register->clip_luseg);
		writel(left, &p_register->clip_luleft);
		writel(left + width, &p_register->clip_luright);
		writel(top, &p_register->clip_lutop);
		writel(top + height, &p_register->clip_lubottom);

		width >>= 1;
		height >>= 1;

		segment = cb_addr >> 30;
		left = cb_addr & 0x00007fff;
		top = (cb_addr & 0x3fff8000) >> 15;

		writel(segment, &p_register->clip_cbseg);
		writel(left, &p_register->clip_cbleft);
		writel(left + width, &p_register->clip_cbright);
		writel(top, &p_register->clip_cbtop);
		writel(top + height, &p_register->clip_cbbottom);

		segment = cr_addr >> 30;
		left = cr_addr & 0x00007fff;
		top = (cr_addr & 0x3fff8000) >> 15;

		writel(segment, &p_register->clip_crseg);
		writel(left, &p_register->clip_crleft);
		writel(left + width, &p_register->clip_crright);
		writel(top, &p_register->clip_crtop);
		writel(top + height, &p_register->clip_crbottom);

		writel(stride_y, &p_register->clip_strideh);
		writel(stride_cb_cr, &p_register->clip_stridel);
	} else {
		/* yuyv 422 packed */
		stride_y >>= 1;

		writel(lu_addr >> 16, &p_register->clip_baseaddrh);
		writel(lu_addr & 0xffff, &p_register->clip_baseaddrl);
		writel(stride_y >> 16, &p_register->clip_strideh);
		writel(stride_y & 0xffff, &p_register->clip_stridel);
	}
}

void nx_vip_set_decimator_addr(u32 module_index, u32 format,
			       u32 width, u32 height, u32 lu_addr, u32 cb_addr,
			       u32 cr_addr, u32 stride_y, u32 stride_cb_cr)
{
	register struct nx_vip_register_set *p_register;
	register u32 segment, left, top;

	p_register = __g_p_register[module_index];

	segment = lu_addr >> 30;
	left = lu_addr & 0x00007fff;
	top = (lu_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->deci_luseg);
	writel(left, &p_register->deci_luleft);
	writel(left + width, &p_register->deci_luright);
	writel(top, &p_register->deci_lutop);
	writel(top + height, &p_register->deci_lubottom);

	if (format == nx_vip_format_420) {
		width >>= 1;
		height >>= 1;
	} else if (format == nx_vip_format_422) {
		width >>= 1;
	}

	segment = cb_addr >> 30;
	left = cb_addr & 0x00007fff;
	top = (cb_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->deci_cbseg);
	writel(left, &p_register->deci_cbleft);
	writel(left + width, &p_register->deci_cbright);
	writel(top, &p_register->deci_cbtop);
	writel(top + height, &p_register->deci_cbbottom);

	segment = cr_addr >> 30;
	left = cr_addr & 0x00007fff;
	top = (cr_addr & 0x3fff8000) >> 15;

	writel(segment, &p_register->deci_crseg);
	writel(left, &p_register->deci_crleft);
	writel(left + width, &p_register->deci_crright);
	writel(top, &p_register->deci_crtop);
	writel(top + height, &p_register->deci_crbottom);
}

void nx_vip_clear_input_fifo(u32 module_index)
{
	register struct nx_vip_register_set *p_register;

	p_register = __g_p_register[module_index];
	writel(0xffff, &p_register->vip_infifoclr);
	writel(0x4, &p_register->vip_infifoclr);
}
