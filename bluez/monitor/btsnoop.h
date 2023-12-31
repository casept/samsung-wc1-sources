/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011-2014  Intel Corporation
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdint.h>
#include <sys/time.h>

#define BTSNOOP_TYPE_HCI		1001
#define BTSNOOP_TYPE_UART		1002
#define BTSNOOP_TYPE_BCSP		1003
#define BTSNOOP_TYPE_3WIRE		1004
#define BTSNOOP_TYPE_MONITOR		2001
#define BTSNOOP_TYPE_SIMULATOR		2002

#define BTSNOOP_OPCODE_NEW_INDEX	0
#define BTSNOOP_OPCODE_DEL_INDEX	1
#define BTSNOOP_OPCODE_COMMAND_PKT	2
#define BTSNOOP_OPCODE_EVENT_PKT	3
#define BTSNOOP_OPCODE_ACL_TX_PKT	4
#define BTSNOOP_OPCODE_ACL_RX_PKT	5
#define BTSNOOP_OPCODE_SCO_TX_PKT	6
#define BTSNOOP_OPCODE_SCO_RX_PKT	7

struct btsnoop_opcode_new_index {
	uint8_t  type;
	uint8_t  bus;
	uint8_t  bdaddr[6];
	char     name[8];
} __attribute__((packed));

#ifdef __TIZEN_PATCH__
void btsnoop_create(const char *path, uint32_t type,
		int16_t rotate_count, ssize_t file_size);
#else
void btsnoop_create(const char *path, uint32_t type);
#endif
void btsnoop_write(struct timeval *tv, uint32_t flags,
					const void *data, uint16_t size);
void btsnoop_write_hci(struct timeval *tv, uint16_t index, uint16_t opcode,
					const void *data, uint16_t size);
void btsnoop_write_phy(struct timeval *tv, uint16_t frequency,
					const void *data, uint16_t size);
int btsnoop_open(const char *path, uint32_t *type);
int btsnoop_read_hci(struct timeval *tv, uint16_t *index, uint16_t *opcode,
						void *data, uint16_t *size);
int btsnoop_read_phy(struct timeval *tv, uint16_t *frequency,
						void *data, uint16_t *size);
void btsnoop_close(void);
