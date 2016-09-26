/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: DeokJin, Lee <truevirtue@nexell.co.kr>
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
#ifndef __COMMON_H__
#define __COMMON_H__

/* "startup.S" startup code function */
void system_sleep(void);

/* "armv7_lib.S" armv7 function */
void set_nonsecure_mode(void);
void set_secure_mode(void);

 int arm9_get_scr(void);
void arm9_set_scr(int reg);

 int arm9_get_auxctrl(void);
void arm9_set_auxctrl(int value);

 int armv7_get_cpuid(void);

void cache_delay_ms(int ms);

#endif
