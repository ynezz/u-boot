#include <common.h>
#include <watchdog.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx6-pins.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>

#ifdef CONFIG_FLEXISBC_WATCHDOG

#define HEARTBEAT_GPIO	IMX_GPIO_NR(3, 22)

void hw_watchdog_reset(void)
{
	gpio_set_value(HEARTBEAT_GPIO, 1);
	/* 
	 * FIXME: There should be 5ms pause, but currently it slows down boot
	 *        process. It works as it is, so leave it for now...
	 */
	gpio_set_value(HEARTBEAT_GPIO, 0);
}

void hw_watchdog_init(void)
{
	/* FIXME: setup has to be done in apalis_imx6.c as it doesn't work here */
#ifdef foobar
	static iomux_v3_cfg_t hb_pad = MX6_PAD_EIM_D22__GPIO3_IO22 | MUX_PAD_CTRL(NO_PAD_CTRL);
	imx_iomux_v3_setup_pad(hb_pad);
	gpio_direction_output(HEARTBEAT_GPIO, 0);
#endif
	hw_watchdog_reset();
}
#endif
