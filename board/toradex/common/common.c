/*
 * Copyright (c) 2015 Toradex, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "configblock.h"
#include <common.h>
#include <g_dnl.h>
#include <libfdt.h>

static char trdx_serial_str[9];
static char trdx_board_rev_str[6];

__weak int checkboard_fallback(void)
{
	return 0;
}

#ifdef CONFIG_REVISION_TAG
#ifdef CONFIG_TRDX_CFG_BLOCK
u32 get_board_rev(void)
{
	/* Check validity */
	if (!trdx_hw_tag.ver_major)
		return 0;

	return ((trdx_hw_tag.ver_major & 0xff) << 8) |
		((trdx_hw_tag.ver_minor & 0xf) << 4) |
		((trdx_hw_tag.ver_assembly & 0xf) + 0xa);
}
#else
u32 get_board_rev(void)
{
	return 0;
}
#endif /* CONFIG_TRDX_CFG_BLOCK */
#endif /* CONFIG_REVISION_TAG */

#ifdef CONFIG_SERIAL_TAG
#ifdef CONFIG_TRDX_CFG_BLOCK
void get_board_serial(struct tag_serialnr *serialnr)
{
	int array[8];
	unsigned int serial = trdx_serial;
	int i;

	serialnr->low = 0;
	serialnr->high = 0;

	/* Check validity */
	if (serial) {
		/*
		 * Convert to Linux serial number format (hexadecimal coded
		 * decimal)
		 */
		i = 7;
		while (serial) {
			array[i--] = serial % 10;
			serial /= 10;
		}
		while (i >= 0)
			array[i--] = 0;
		serial = array[0];
		for (i = 1; i < 8; i++) {
			serial *= 16;
			serial += array[i];
		}

		serialnr->low = serial;
	}
}
#else
u32 get_board_rev(void)
{
	return 0;
}
#endif /* CONFIG_TRDX_CFG_BLOCK */
#endif /* CONFIG_SERIAL_TAG */

int checkboard(void)
{
#ifdef CONFIG_TRDX_CFG_BLOCK
	unsigned char ethaddr[6];

	if (read_trdx_cfg_block()) {
		printf("Missing Toradex config block\n");
		checkboard_fallback();
		return 0;
	}

	/* board serial-number */
	sprintf(trdx_serial_str, "%08u", trdx_serial);
	sprintf(trdx_board_rev_str, "V%1d.%1d%c",
		trdx_hw_tag.ver_major,
		trdx_hw_tag.ver_minor,
		(char)trdx_hw_tag.ver_assembly + 'A');


	setenv("serial#", trdx_serial_str);

	/*
	 * Check if environment contains a valid MAC address,
	 * set the one from config block if not
	 */
	if (!eth_getenv_enetaddr("ethaddr", ethaddr))
		eth_setenv_enetaddr("ethaddr", (u8 *)&trdx_eth_addr);

#ifdef CONFIG_TRDX_CFG_BLOCK_2ND_ETHADDR
	if (!eth_getenv_enetaddr("eth1addr", ethaddr)) {
		/*
		 * Secondary MAC address is allocated from block
		 * 0x100000 higher then the first MAC address
		 */
		memcpy(ethaddr, &trdx_eth_addr, 6);
		ethaddr[3] += 0x10;
		eth_setenv_enetaddr("eth1addr", ethaddr);
	}
#endif

	printf("Model: Toradex %s %s, Serial# %s\n",
		toradex_modules[trdx_hw_tag.prodid],
		trdx_board_rev_str,
		trdx_serial_str);
#else
	checkboard_fallback();
#endif
	return 0;
}

#ifdef CONFIG_USBDOWNLOAD_GADGET
int g_dnl_bind_fixup(struct usb_device_descriptor *dev, const char *name)
{
	unsigned short usb_pid = 0xffff;

#ifdef CONFIG_TRDX_CONFIGBLOCK
	usb_pid = 0x4000 + trdx_hw_tag.prodid;
#endif
	put_unaligned(usb_pid, &dev->idProduct);

	return 0;
}
#endif /* CONFIG_USBDOWNLOAD_GADGET */

#if defined(CONFIG_OF_LIBFDT) && defined(CONFIG_OF_SYSTEM_SETUP) && \
	defined(CONFIG_TRDX_CFG_BLOCK)
int ft_system_setup(void *blob, bd_t *bd)
{
	if (trdx_serial) {
		fdt_setprop(blob, 0, "serial-number", trdx_serial_str,
			    strlen(trdx_serial_str) + 1);
	}

	if (trdx_hw_tag.ver_major) {
		char prod_id[5];

		sprintf(prod_id, "%04u", trdx_hw_tag.prodid);
		fdt_setprop(blob, 0, "toradex,product-id", prod_id, 5);

		fdt_setprop(blob, 0, "toradex,board-rev", trdx_board_rev_str,
			    strlen(trdx_board_rev_str) + 1);
	}

	return 0;
}
#endif
