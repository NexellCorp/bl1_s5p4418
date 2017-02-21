#include <sysheader.h>

#if defined (NXE1500_PMIC_ENABLE)
#include <i2c_gpio.h>
#include <nxe1500.h>

int nxe1500_write(char addr, char *pdata, int size)
{
	return i2c_gpio_write(I2C_ADDR_NXE1500, addr, pdata, size);
}

int nxe1500_read(char addr, char *pdata, int size)
{
	return i2c_gpio_read(I2C_ADDR_NXE1500, addr, pdata, size);
}

unsigned char nxe1500_get_dcdc_step(int want_vol)
{
	unsigned int vol_step = 0;

	if (want_vol < NXE1500_DEF_DDCx_VOL_MIN) {
		want_vol = NXE1500_DEF_DDCx_VOL_MIN;
	} else if (want_vol > NXE1500_DEF_DDCx_VOL_MAX) {
		want_vol = NXE1500_DEF_DDCx_VOL_MAX;
	}

	vol_step = (want_vol - NXE1500_DEF_DDCx_VOL_MIN +
			NXE1500_DEF_DDCx_VOL_STEP - 1) /
		NXE1500_DEF_DDCx_VOL_STEP;

	return (unsigned char)(vol_step & 0xFF);
}
#endif // #if defined (NXE1500_PMIC_ENABLE)