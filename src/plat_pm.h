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
#ifndef __PLAT_PM_H__
#define __PLAT_PM_H__

void s5p4418_cpuidle(int cpu_id, int int_id);
void s5p4418_cpu_off_wfi_ready(void);

 int s5p4418_cpu_check(unsigned int cpu_id);

 int s5p4418_cpu_on(unsigned int cpu_id);
 int s5p4418_cpu_off(unsigned int cpu_id);

void s5p4418_suspend(void);
void s5p4418_resume(void);

#endif	// __PLAT_PM_H__
