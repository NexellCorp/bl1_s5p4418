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
#ifndef __PSCI_PRIVATE_H__
#define __PSCI_PRIVATE_H__

#include <psci.h>

/*******************************************************************************
 * Function prototypes
 ******************************************************************************/
/* Private exported functions from psci_on.c */
int psci_cpu_on_start(unsigned int target_cpu, unsigned int entrypoint);

void psci_cpu_on_finish(unsigned int cpu_idx,	psci_power_state_t *state_info);

/* Private exported functions from psci_off.c */
int psci_do_cpu_off(unsigned int cpu_id);

/* Private exported functions from psci_suspend.c */
 int psci_cpu_suspend_start(unsigned int entrypoint);
void psci_cpu_suspend_finish(unsigned int cpu_idx, psci_power_state_t *state_info);

/* Private exported functions from psci_system_off.c */
void psci_system_off(void);
void psci_system_reset(void);

#endif /* __PSCI_PRIVATE_H__ */
