/*
 * Copyright (c) 2011-2014 Toradex, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _TRDX_CONFIGBLOCK_H
#define _TRDX_CONFIGBLOCK_H

int read_trdx_cfg_block(void);
void display_board_info(void);
void get_board_serial_char(char *serialnr);
void get_board_product_number(unsigned short *prodnr);

#endif /* _TRDX_CONFIGBLOCK_H */
