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
#include <iUSBBOOT.h>
#include <iSDHCBOOT.h>

static int plat_usbload(struct sbi_header *tbi)
{
#if (SUPPORT_KERNEL_3_4 == 0)
	return bl2_usbboot(tbi);
#else
	return normal_usbboot(tbi);
#endif
}

static void plat_launch(struct sbi_header *ptbi)
{
	/* step xx. jump the next bootloader */
	void (*pLaunch)(unsigned int, unsigned int)
		= (void (*)(unsigned int, unsigned int))ptbi->launch_addr;

	SYSMSG("Image Loading Done!\r\n");
	SYSMSG("Launch to 0x%08X\r\n", (unsigned int)pLaunch);
	while (!serial_done());
	pLaunch(0, 4330);

	ERROR("Image Loading Failure Try to USB boot\r\n");
	while (!serial_done());
}

static int plat_next_load(struct sbi_header *ptbi)
{
	int device = psbi->dbi.spibi.loaddevice;
	int ret = 0;

	switch (device) {
		case BOOT_FROM_USB:
			SYSMSG("Loading from usb...\r\n");
			ret = plat_usbload(ptbi);
			break;

#if defined(SUPPORT_SDMMC_BOOT)
		case BOOT_FROM_SDMMC:
			SYSMSG("Loading from sdmmc...\r\n");
			ret = iSDXCBOOT(ptbi);	// for SD boot
			break;
#endif
		default:
			SYSMSG("Loading from usb...\r\n");
			printf("Default Operation...!!\r\n");
			ret = plat_usbload(ptbi);
			break;

	}
	return ret;
}

void plat_load(struct sbi_header *ptbi)
{
	int success;

	success = plat_next_load(ptbi);

#if defined(CRC_CHECK_ON)
	/* step xx. check the memory crc check (optional) */
	ret = crc_check((void*)ptbi->load_addr, (unsigned int)ptbi->load_size
			,(unsigned int)ptbi->dbi.sdmmcbi.crc32);
#endif
	if (success)
		plat_launch(ptbi);
}
