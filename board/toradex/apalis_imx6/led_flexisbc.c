#include <common.h>
#include <status_led.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/imx-regs.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/arch/gpio.h>
#include <asm/gpio.h>
#include <asm/io.h>

static unsigned int saved_state[3] = {
	STATUS_LED_OFF,
	STATUS_LED_OFF,
	STATUS_LED_OFF,
};

#define LED_GREEN	IMX_GPIO_NR(1, 17)
#define LED_RED		IMX_GPIO_NR(4, 18)
#define LED_BLUE	IMX_GPIO_NR(4, 19)

void coloured_LED_init(void)
{
	static iomux_v3_cfg_t const flexisbc_led_pads[] = {
		MX6_PAD_SD1_DAT1__GPIO1_IO17 | MUX_PAD_CTRL(NO_PAD_CTRL),
		MX6_PAD_DI0_PIN2__GPIO4_IO18 | MUX_PAD_CTRL(NO_PAD_CTRL),
		MX6_PAD_DI0_PIN3__GPIO4_IO19 | MUX_PAD_CTRL(NO_PAD_CTRL),
	};

	imx_iomux_v3_setup_multiple_pads(flexisbc_led_pads,
					 ARRAY_SIZE(flexisbc_led_pads));
	gpio_direction_output(LED_GREEN, 1);
	gpio_direction_output(LED_RED, 1);
	gpio_direction_output(LED_BLUE, 1);
}

void red_led_off(void)
{
	gpio_set_value(LED_RED, 1);
	saved_state[STATUS_LED_RED] = STATUS_LED_OFF;
}

void green_led_off(void)
{
	gpio_set_value(LED_GREEN, 1);
	saved_state[STATUS_LED_GREEN] = STATUS_LED_OFF;
}

void blue_led_off(void)
{
	gpio_set_value(LED_BLUE, 1);
	saved_state[STATUS_LED_BLUE] = STATUS_LED_OFF;
}

void red_led_on(void)
{
	gpio_set_value(LED_RED, 0);
	saved_state[STATUS_LED_RED] = STATUS_LED_ON;
}

void green_led_on(void)
{
	gpio_set_value(LED_GREEN, 0);
	saved_state[STATUS_LED_GREEN] = STATUS_LED_ON;
}

void blue_led_on(void)
{
	gpio_set_value(LED_BLUE, 0);
	saved_state[STATUS_LED_BLUE] = STATUS_LED_ON;
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
	} else if (STATUS_LED_BLUE == mask) {
		if (STATUS_LED_ON == saved_state[STATUS_LED_BLUE])
			blue_led_off();
		else
			blue_led_on();
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
	} else if (STATUS_LED_BLUE == mask) {
		if (STATUS_LED_ON == state)
			blue_led_on();
		else
			blue_led_off();
	}
}
