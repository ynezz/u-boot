/*
 * Copyright (c) 2012-2015 Toradex, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/gp_padctrl.h>
#include <asm/arch/pinmux.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/tegra.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <g_dnl.h>
#include <i2c.h>

#include "pinmux-config-colibri_t30.h"
#include "../common/configblock.h"

int arch_misc_init(void)
{
	if (readl(NV_PA_BASE_SRAM + NVBOOTINFOTABLE_BOOTTYPE) ==
	    NVBOOTTYPE_RECOVERY) {
		printf("USB recovery mode, disabled autoboot\n");
		setenv("bootdelay", "-1");
	}

	return 0;
}

int checkboard(void)
{
#ifdef CONFIG_TRDX_CFG_BLOCK
	if (read_trdx_cfg_block())
		printf("Missing Toradex config block\n");
	else {
		display_board_info();
		return 0;
	}
#endif
	printf("Model: Toradex Colibri T30 1GB\n");

	return 0;
}

int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	unsigned short prodnr = 0;
	unsigned short usb_pid;

	get_board_product_number(&prodnr);

	put_unaligned(CONFIG_TRDX_VID, &dev->idVendor);

	if (prodnr != 30)
		usb_pid = CONFIG_TRDX_PID_COLIBRI_T30;
	else
		usb_pid = CONFIG_TRDX_PID_COLIBRI_T30_IT;

	put_unaligned(usb_pid, &dev->idProduct);

	return 0;
}

/*
 * Routine: pinmux_init
 * Description: Do individual peripheral pinmux configs
 */
void pinmux_init(void)
{
	pinmux_config_pingrp_table(tegra3_pinmux_common,
				   ARRAY_SIZE(tegra3_pinmux_common));

	pinmux_config_pingrp_table(unused_pins_lowpower,
				   ARRAY_SIZE(unused_pins_lowpower));

	/* Initialize any non-default pad configs (APB_MISC_GP regs) */
	pinmux_config_drvgrp_table(colibri_t30_padctrl,
				   ARRAY_SIZE(colibri_t30_padctrl));
}

/*
 * Enable AX88772B USB to LAN controller
 */
void pin_mux_usb(void)
{
	/* Reset ASIX using LAN_RESET */
	gpio_request(GPIO_PDD0, "LAN_RESET");
	gpio_direction_output(GPIO_PDD0, 0);
	udelay(5);
	gpio_set_value(GPIO_PDD0, 1);
}
