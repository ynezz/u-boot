/*
 * Copyright (c) 2011-2015 Toradex, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

#include <cli.h>
#include <malloc.h>
#include <mmc.h>
#include <nand.h>

DECLARE_GLOBAL_DATA_PTR;

#define TAG_VALID	0xcf01
#define TAG_MAC		0x0000
#define TAG_HW		0x0008

#define TAG_FLAG_VALID	0x1

#define TRDX_CFG_BLOCK_MAX_SIZE 64

struct toradex_tag {
	u32 len : 14;
	u32 flags : 2;
	u32 id : 16;
};

struct toradex_hw {
	u16 ver_major;
	u16 ver_minor;
	u16 ver_assembly;
	u16 prodid;
};

struct toradex_eth_addr {
	u32 oui : 24;
	u32 nic : 24;
} __attribute__((__packed__));

bool valid_cfgblock;
struct toradex_hw toradex_hw_tag;
struct toradex_eth_addr eth_addr;
u32 toradex_serial;

static const char* const toradex_modules[] = {
	[10] = "Colibri VF50 256MB", /* not on sale currently */
	[11] = "Colibri VF61 256MB",
	[12] = "Colibri VF61 256MB IT",
	[13] = "Colibri VF50 256MB IT",
};

#ifdef CONFIG_REVISION_TAG
u32 get_board_rev(void)
{
	/* Check validity */
	if (!toradex_hw_tag.ver_major)
		return 0;

	return ((toradex_hw_tag.ver_major & 0xff) << 8) |
		((toradex_hw_tag.ver_minor & 0xf) << 4) |
		((toradex_hw_tag.ver_assembly & 0xf) + 0xa);
}
#endif /* CONFIG_REVISION_TAG */

#ifdef CONFIG_SERIAL_TAG
void get_board_serial(struct tag_serialnr *serialnr)
{
	int array[8];
	unsigned int serial = toradex_serial;
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
#endif /* CONFIG_SERIAL_TAG */

#ifdef CONFIG_TRDX_CFG_BLOCK_IS_IN_MMC
static int read_trdx_cfg_block_from_mmc(unsigned char *config_block)
{
	struct mmc *mmc;
	int dev = CONFIG_TRDX_CFG_BLOCK_DEV;
	int offset = CONFIG_TRDX_CFG_BLOCK_OFFSET;
	uint part = CONFIG_TRDX_CFG_BLOCK_PART;
	uint blk_start;
	int ret = 0;

	/* Read production parameter config block from eMMC */
	mmc = find_mmc_device(dev);
	if (!mmc) {
		puts("No MMC card found\n");
		ret = -ENODEV;
		goto out;
	}
	if (part != mmc->part_num) {
		if (mmc_switch_part(dev, part)) {
			puts("MMC partition switch failed\n");
			ret = -ENODEV;
			goto out;
		}
	}
	if (offset < 0)
		offset += mmc->capacity;
	blk_start = ALIGN(offset, mmc->write_bl_len) / mmc->write_bl_len;

	/* Just reading one 512 byte block */
	if (mmc->block_dev.block_read(dev, blk_start, 1,
				      (unsigned char *)config_block) != 1) {
		ret = -EIO;
		goto out;
	}

out:
	/* Switch back to regular eMMC user partition */
	mmc_switch_part(0, 0);

	return ret;
}
#endif

#ifdef CONFIG_TRDX_CFG_BLOCK_IS_IN_NAND
static int read_trdx_cfg_block_from_nand(unsigned char *config_block)
{
	size_t size = TRDX_CFG_BLOCK_MAX_SIZE;

	/* Read production parameter config block from NAND page */
	return nand_read(&nand_info[0], CONFIG_TRDX_CFG_BLOCK_OFFSET,
			 &size, config_block);
}

static int write_trdx_cfg_block_to_nand(unsigned char *config_block)
{
	size_t size = TRDX_CFG_BLOCK_MAX_SIZE;

	/* Read production parameter config block from NAND page */
	return nand_write(&nand_info[0], CONFIG_TRDX_CFG_BLOCK_OFFSET,
			  &size, config_block);
}
#endif

int read_trdx_cfg_block(void)
{
	int ret = 0;
	unsigned char *config_block = NULL;
	struct toradex_tag *tag;
	size_t size = TRDX_CFG_BLOCK_MAX_SIZE;
	int offset;
	unsigned char ethaddr[6];
	char serial[9];

	/* Allocate RAM area for config block */
	config_block = malloc(size);
	if (!config_block) {
		printf("Not enough malloc space available!\n");
		return -ENOMEM;
	}

	memset((void *)config_block, 0, size);

#if defined(CONFIG_TRDX_CFG_BLOCK_IS_IN_MMC)
	ret = read_trdx_cfg_block_from_mmc(config_block);
#elif defined(CONFIG_TRDX_CFG_BLOCK_IS_IN_NAND)
	ret = read_trdx_cfg_block_from_nand(config_block);
#else
#error "Toradex config block location not set"
#endif

	/* Expect a valid tag first */
	tag = (struct toradex_tag *)config_block;
	if (tag->flags != TAG_FLAG_VALID || tag->id != TAG_VALID) {
		valid_cfgblock = false;
		ret = -EINVAL;
		goto out;
	}
	valid_cfgblock = true;
	offset = 4;

	while (true) {
		tag = (struct toradex_tag *)(config_block + offset);
		offset += 4;
		if (tag->flags != TAG_FLAG_VALID)
			break;

		switch (tag->id)
		{
		case TAG_MAC:
			memcpy(&eth_addr, config_block + offset, 6);

			/* The NIC part of the MAC address is the serial number */
			toradex_serial = ntohl(eth_addr.nic) >> 8;

			/* board serial-number */
			sprintf(serial, "%08u", toradex_serial);
			setenv("serial#", serial);

			/*
			 * Check if environment contains a valid MAC address,
			 * set the one from config block if not
			 */
			if (!eth_getenv_enetaddr("ethaddr", ethaddr))
				eth_setenv_enetaddr("ethaddr", (u8 *)&eth_addr);

#ifdef CONFIG_TRDX_CFG_BLOCK_2ND_ETHADDR
			if (!eth_getenv_enetaddr("eth1addr", ethaddr)) {
				/*
				 * Secondary MAC address is allocated from a block
				 * 0x100000 higher then the first MAC address
				 */
				memcpy(ethaddr, &eth_addr, 6);
				ethaddr[3] += 0x10;
				eth_setenv_enetaddr("eth1addr", ethaddr);
			}
#endif
			break;
		case TAG_HW:
			memcpy(&toradex_hw_tag, config_block + offset, 8);
			break;
		}

		/* Get to next tag according to current tags length */
		offset += tag->len * 4;
	}

out:
	free(config_block);
	return ret;
}

void display_board_info(void)
{
	printf("%s V%d.%d %c\n",
		toradex_modules[toradex_hw_tag.prodid],
		toradex_hw_tag.ver_major,
		toradex_hw_tag.ver_minor,
		(char)toradex_hw_tag.ver_assembly + 'A');
}

void get_board_serial_char(char *serialnr)
{
	if (!toradex_serial) {
		strcpy(serialnr, "UNKNOWN");
		return;
	}

	sprintf(serialnr, "%u", toradex_serial);
}

void get_board_product_number(unsigned short *prodnr)
{
	*prodnr = toradex_hw_tag.prodid;
}

static int get_cfgblock_interactive(void)
{
#ifdef CONFIG_TARGET_COLIBRI_VF
	char message[CONFIG_SYS_CBSIZE];
	char *soc;
	char it = 'n';
	int len;

	sprintf(message, "Is the module an IT version? [y/N] ");
	len = cli_readline(message);
	it = console_buffer[0];

	soc = getenv("soc");
	if (!strcmp("vf500", soc)) {
		if (it == 'y' || it == 'Y')
			toradex_hw_tag.prodid = 13;
		else
			toradex_hw_tag.prodid = 10;
	} else if (!strcmp("vf610", soc)) {
		if (it == 'y' || it == 'Y')
			toradex_hw_tag.prodid = 12;
		else
			toradex_hw_tag.prodid = 11;
	} else {
		printf("Module type not detectable due to unknown SoC\n");
		return -1;
	}

	while (len < 5) {
		sprintf(message, "Enter the module version (e.g. V1.1 B): V");
		len = cli_readline(message);
	}

	toradex_hw_tag.ver_major = console_buffer[0] - '0';
	toradex_hw_tag.ver_minor = console_buffer[2] - '0';
	toradex_hw_tag.ver_assembly = console_buffer[4] - 'A';

	while (len < 8) {
		sprintf(message, "Enter module serial number: ");
		len = cli_readline(message);
	}

	toradex_serial = simple_strtoul(console_buffer, NULL, 10);
#else
	printf("Interactive mode not supported\n");
#endif

	return 0;
}

static int get_cfgblock_barcode(char *barcode)
{
	if (strlen(barcode) < 16) {
		printf("Argument too short, barcode is 16 chars long\n");
		return -1;
	}

	/* Get hardware information from the first 8 digits */
	toradex_hw_tag.ver_major = barcode[4] - '0';
	toradex_hw_tag.ver_minor = barcode[5] - '0';
	toradex_hw_tag.ver_assembly = barcode[7] - '0';

	barcode[4] = '\0';
	toradex_hw_tag.prodid = simple_strtoul(barcode, NULL, 10);

	/* Parse second part of the barcode (serial number */
	barcode += 8;
	toradex_serial = simple_strtoul(barcode, NULL, 10);

	return 0;
}

static int do_cfgblock_create(cmd_tbl_t *cmdtp, int flag, int argc,
			      char * const argv[])
{
	u8 *config_block;
	struct toradex_tag *tag;
	size_t size = TRDX_CFG_BLOCK_MAX_SIZE;
	int offset = 0;
	int ret;

	config_block = malloc(size);
	memset(config_block, 0xff, size);

	read_trdx_cfg_block();
	if (valid_cfgblock) {
#ifdef CONFIG_TRDX_CFG_BLOCK_IS_IN_NAND
		/*
		 * On NAND devices, recreation is only allowed if the page is
		 * empty (config block invalid...)
		 */
		printf("NAND erase block %d need to be erased before creating "
		       "a Toradex config block\n",
		       CONFIG_TRDX_CFG_BLOCK_OFFSET / nand_info[0].erasesize);
		return 0;
#else
		char message[CONFIG_SYS_CBSIZE];
		sprintf(message, "A valid Toradex config block is present, "
			         "still recreate? [y/N] ");

		if (!cli_readline(message))
			return 0;

		if (console_buffer[0] != 'y' && console_buffer[0] != 'Y')
			return 0;
#endif
	}

	/* Parse new Toradex config data... */
	if (argc < 3)
		ret = get_cfgblock_interactive();
	else
		ret = get_cfgblock_barcode(argv[2]);

	if (ret)
		return ret;

	/* Convert serial number to MAC address (the storage format) */
	eth_addr.oui = htonl(0x00142dUL << 8);
	eth_addr.nic = htonl(toradex_serial << 8);

	/* Valid Tag */
	tag = (struct toradex_tag *)config_block;
	tag->id = TAG_VALID;
	tag->flags = TAG_FLAG_VALID;
	tag->len = 0;
	offset += 4;

	/* Product Tag */
	tag = (struct toradex_tag *)(config_block + offset);
	tag->id = TAG_HW;
	tag->flags = TAG_FLAG_VALID;
	tag->len = 2;
	offset += 4;

	memcpy(config_block + offset, &toradex_hw_tag, 8);
	offset += 8;

	/* MAC Tag */
	tag = (struct toradex_tag *)(config_block + offset);
	tag->id = TAG_MAC;
	tag->flags = TAG_FLAG_VALID;
	tag->len = 2;
	offset += 4;

	memcpy(config_block + offset, &eth_addr, 6);
	offset +=6;
	memset(config_block + offset, 0, 32 - offset);

#ifdef CONFIG_TRDX_CFG_BLOCK_IS_IN_NAND
	ret = write_trdx_cfg_block_to_nand(config_block);
	if (ret) {
		printf("Failed to write Toradex config block: %d\n", ret);
		return ret;
	}
#else
	return 0;
#endif

	printf("Toradex config block successfully written\n");
	return ret;
}

static int do_cfgblock(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "create")) {
		return do_cfgblock_create(cmdtp, flag, argc, argv);
	} else if (!strcmp(argv[1], "reload")) {
		ret = read_trdx_cfg_block();
		if (ret)
			printf("Failed to reload Toradex config block: %d\n", ret);
		return 0;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	cfgblock, 3, 0, do_cfgblock,
	"Toradex config block handling commands",
	"create [barcode] - (Re-)create Toradex config block\n"
	"cfgblock reload - Reload Toradex config block from flash"
);
