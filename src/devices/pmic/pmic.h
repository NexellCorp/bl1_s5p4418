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
#ifndef __PMIC_H__
#define __PMIC_H__

#if defined(CHIPID_NXP4330)
void pmic_lepus(void);
void pmic_navi(void);
void pmic_smartvoice(void);
#else
void pmic_avn(void);
void pmic_drone(void);
void pmic_svt(void);
void pmic_raptor(void);
#endif

void pmic_initalize(void);

#endif
