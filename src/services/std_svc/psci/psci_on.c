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
#include <sysheader.h>
#include <plat_pm.h>
#include "psci.h"

/*************************************************************
 * Must be S5P4418
 * CPU Power Off sequence in S5P4418
 * Reference is made to function psci interface .
 *************************************************************/
void (*g_psci_ep)();

int psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint)
{
	unsigned int cpu_id = ((target_cpu >> 0) & 0xFF);

	g_psci_ep = (void (*)())entrypoint;

	s5p4418_cpu_on(cpu_id);

	return s5p4418_cpu_check(cpu_id);
}
