/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
 * (C) Copyright 2009
 * Eric Benard <eric@eukrea.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <status_led.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/imx-regs.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <asm/io.h>

static unsigned int saved_state[4] = {
	STATUS_LED_OFF,
	STATUS_LED_OFF,
	STATUS_LED_OFF,
	STATUS_LED_OFF
};

#define LED_4_GREEN	IMX_GPIO_NR(1, 17)
#define LED_4_RED	IMX_GPIO_NR(1, 21)
#define LED_5_GREEN	IMX_GPIO_NR(2, 1)
#define LED_5_RED	IMX_GPIO_NR(2, 2)

void coloured_LED_init(void)
{
	static iomux_v3_cfg_t const ixora_led_pads[] = {
		MX6_PAD_SD1_DAT1__GPIO1_IO17 | MUX_PAD_CTRL(NO_PAD_CTRL),
		MX6_PAD_SD1_DAT3__GPIO1_IO21 | MUX_PAD_CTRL(NO_PAD_CTRL),
		MX6_PAD_NANDF_D1__GPIO2_IO01 | MUX_PAD_CTRL(NO_PAD_CTRL),
		MX6_PAD_NANDF_D2__GPIO2_IO02 | MUX_PAD_CTRL(NO_PAD_CTRL),
	};

	imx_iomux_v3_setup_multiple_pads(ixora_led_pads,
					 ARRAY_SIZE(ixora_led_pads));
	gpio_direction_output(LED_4_GREEN, 0);
	gpio_direction_output(LED_4_RED, 0);
	gpio_direction_output(LED_5_GREEN, 0);
	gpio_direction_output(LED_5_RED, 0);
}

void red_led_off(void)
{
	gpio_set_value(LED_4_RED, 0);
	saved_state[STATUS_LED_RED] = STATUS_LED_OFF;
}

void green_led_off(void)
{
	gpio_set_value(LED_4_GREEN, 0);
	saved_state[STATUS_LED_GREEN] = STATUS_LED_OFF;
}

void red_led_on(void)
{
	gpio_set_value(LED_4_RED, 1);
	saved_state[STATUS_LED_RED] = STATUS_LED_ON;
}

void green_led_on(void)
{
	gpio_set_value(LED_4_GREEN, 1);
	saved_state[STATUS_LED_GREEN] = STATUS_LED_ON;
}

void __led_init(led_id_t mask, int state)
{
	__led_set(mask, state);
}

void __led_toggle(led_id_t mask)
{
	if (STATUS_LED_RED == mask) {
		if (STATUS_LED_ON == saved_state[STATUS_LED_RED])
			red_led_off();
		else
			red_led_on();
	} else if (STATUS_LED_GREEN == mask) {
		if (STATUS_LED_ON == saved_state[STATUS_LED_GREEN])
			green_led_off();
		else
			green_led_on();
	}
}

void __led_set(led_id_t mask, int state)
{
	if (STATUS_LED_RED == mask) {
		if (STATUS_LED_ON == state)
			red_led_on();
		else
			red_led_off();
	} else if (STATUS_LED_GREEN == mask) {
		if (STATUS_LED_ON == state)
			green_led_on();
		else
			green_led_off();
	}
}
