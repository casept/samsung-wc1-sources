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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "lib/bluetooth.h"
#include "lib/hci.h"

#include "btsnoop.h"

struct btsnoop_hdr {
	uint8_t		id[8];		/* Identification Pattern */
	uint32_t	version;	/* Version Number = 1 */
	uint32_t	type;		/* Datalink Type */
} __attribute__ ((packed));
#define BTSNOOP_HDR_SIZE (sizeof(struct btsnoop_hdr))

struct btsnoop_pkt {
	uint32_t	size;		/* Original Length */
	uint32_t	len;		/* Included Length */
	uint32_t	flags;		/* Packet Flags */
	uint32_t	drops;		/* Cumulative Drops */
	uint64_t	ts;		/* Timestamp microseconds */
	uint8_t		data[0];	/* Packet Data */
} __attribute__ ((packed));
#define BTSNOOP_PKT_SIZE (sizeof(struct btsnoop_pkt))

static const uint8_t btsnoop_id[] = { 0x62, 0x74, 0x73, 0x6e,
				      0x6f, 0x6f, 0x70, 0x00 };

static const uint32_t btsnoop_version = 1;
static uint32_t btsnoop_type = 0;

static int btsnoop_fd = -1;
static uint16_t btsnoop_index = 0xffff;

#ifdef __TIZEN_PATCH__
static char *btsnoop_path = NULL;
static int16_t btsnoop_rotate = -1;
static ssize_t btsnoop_size = -1;

void btsnoop_create(const char *path, uint32_t type,
		int16_t rotate_count, ssize_t file_size)
#else
void btsnoop_create(const char *path, uint32_t type)
#endif
{
	struct btsnoop_hdr hdr;
	ssize_t written;

	if (btsnoop_fd >= 0)
		return;

	btsnoop_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (btsnoop_fd < 0)
		return;

	btsnoop_type = type;

	memcpy(hdr.id, btsnoop_id, sizeof(btsnoop_id));
	hdr.version = htobe32(btsnoop_version);
	hdr.type = htobe32(btsnoop_type);

	written = write(btsnoop_fd, &hdr, BTSNOOP_HDR_SIZE);
	if (written < 0) {
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return;
	}

#ifdef __TIZEN_PATCH__
	if (rotate_count > 0 && file_size > 0) {
		btsnoop_path = strdup(path);
		btsnoop_rotate = rotate_count;
		btsnoop_size = file_size;
	}
#endif
}

#ifdef __TIZEN_PATCH__
static void btsnoop_create_2(void)
{
	struct btsnoop_hdr hdr;
	ssize_t written;

	if (btsnoop_fd >= 0)
		close(btsnoop_fd);

	btsnoop_fd = open(btsnoop_path,
			O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (btsnoop_fd < 0)
		goto fail;

	memcpy(hdr.id, btsnoop_id, sizeof(btsnoop_id));
	hdr.version = htobe32(btsnoop_version);
	hdr.type = htobe32(btsnoop_type);

	written = write(btsnoop_fd, &hdr, BTSNOOP_HDR_SIZE);
	if (written < 0) {
		close(btsnoop_fd);
		btsnoop_fd = -1;
		goto fail;
	}

	return;

fail:
	free(btsnoop_path);
	btsnoop_rotate = -1;
	btsnoop_size = -1;
	btsnoop_index = 0xffff;

	return;
}

static void btsnoop_rotate_files(void)
{
	char *filename = NULL;
	char *new_filename = NULL;
	int i;
	int postfix_width = 0;
	int err;

	if (btsnoop_rotate <= 1)
		return;

	for (i = btsnoop_rotate / 10; i; i /= 10)
		postfix_width++;

	for (i = btsnoop_rotate - 2; i >= 0; i--) {
		if (i == 0) {
			filename = strdup(btsnoop_path);
			err = (filename == NULL) ? -1 : 0;
		} else {
			err = asprintf(&filename, "%s.%0*d",
					btsnoop_path, postfix_width, i);
		}

		if (err < 0 || access(filename, F_OK) < 0)
			goto done;

		err = asprintf(&new_filename, "%s.%0*d",
				btsnoop_path, postfix_width, i + 1);
		if (err < 0)
			goto done;

		err = rename(filename, new_filename)

done:
		if (new_filename) {
			free(new_filename);
			new_filename = NULL;
		}

		if (filename) {
			free(filename);
			filename = NULL;
		}

		if (err < 0)
			break;
	}
}
#endif

void btsnoop_write(struct timeval *tv, uint32_t flags,
					const void *data, uint16_t size)
{
	struct btsnoop_pkt pkt;
	uint64_t ts;
	ssize_t written;

	ts = (tv->tv_sec - 946684800ll) * 1000000ll + tv->tv_usec;

	pkt.size  = htobe32(size);
	pkt.len   = htobe32(size);
	pkt.flags = htobe32(flags);
	pkt.drops = htobe32(0);
	pkt.ts    = htobe64(ts + 0x00E03AB44A676000ll);

#ifdef __TIZEN_PATCH__
	if ((btsnoop_rotate > 0 && btsnoop_size > 0) &&
			lseek(btsnoop_fd, 0x00, SEEK_CUR) +
			BTSNOOP_PKT_SIZE + size > btsnoop_size) {
		btsnoop_rotate_files();
		btsnoop_create_2();
		if (btsnoop_fd < 0)
			return;
	}
#endif

	written = write(btsnoop_fd, &pkt, BTSNOOP_PKT_SIZE);
	if (written < 0)
		return;

	if (data && size > 0) {
		written = write(btsnoop_fd, data, size);
		if (written < 0)
			return;
	}
}

static uint32_t get_flags_from_opcode(uint16_t opcode)
{
	switch (opcode) {
	case BTSNOOP_OPCODE_NEW_INDEX:
	case BTSNOOP_OPCODE_DEL_INDEX:
		break;
	case BTSNOOP_OPCODE_COMMAND_PKT:
		return 0x02;
	case BTSNOOP_OPCODE_EVENT_PKT:
		return 0x03;
	case BTSNOOP_OPCODE_ACL_TX_PKT:
		return 0x00;
	case BTSNOOP_OPCODE_ACL_RX_PKT:
		return 0x01;
	case BTSNOOP_OPCODE_SCO_TX_PKT:
	case BTSNOOP_OPCODE_SCO_RX_PKT:
		break;
	}

	return 0xff;
}

void btsnoop_write_hci(struct timeval *tv, uint16_t index, uint16_t opcode,
					const void *data, uint16_t size)
{
	uint32_t flags;

	if (!tv)
		return;

	if (btsnoop_fd < 0)
		return;

	switch (btsnoop_type) {
	case BTSNOOP_TYPE_HCI:
		if (btsnoop_index == 0xffff)
			btsnoop_index = index;

		if (index != btsnoop_index)
			return;

		flags = get_flags_from_opcode(opcode);
		if (flags == 0xff)
			return;
		break;

	case BTSNOOP_TYPE_MONITOR:
		flags = (index << 16) | opcode;
		break;

	default:
		return;
	}

	btsnoop_write(tv, flags, data, size);
}

void btsnoop_write_phy(struct timeval *tv, uint16_t frequency,
					const void *data, uint16_t size)
{
	uint32_t flags;

	if (!tv)
		return;

	if (btsnoop_fd < 0)
		return;

	switch (btsnoop_type) {
	case BTSNOOP_TYPE_SIMULATOR:
		flags = (1 << 16) | frequency;
		break;

	default:
		return;
	}

	btsnoop_write(tv, flags, data, size);
}

int btsnoop_open(const char *path, uint32_t *type)
{
	struct btsnoop_hdr hdr;
	ssize_t len;

	if (btsnoop_fd >= 0) {
		fprintf(stderr, "Too many open files\n");
		return -1;
	}

	btsnoop_fd = open(path, O_RDONLY | O_CLOEXEC);
	if (btsnoop_fd < 0) {
		perror("Failed to open file");
		return -1;
	}

	len = read(btsnoop_fd, &hdr, BTSNOOP_HDR_SIZE);
	if (len < 0 || len != BTSNOOP_HDR_SIZE) {
		perror("Failed to read header");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	if (memcmp(hdr.id, btsnoop_id, sizeof(btsnoop_id))) {
		fprintf(stderr, "Invalid btsnoop header\n");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	if (be32toh(hdr.version) != btsnoop_version) {
		fprintf(stderr, "Invalid btsnoop version\n");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	btsnoop_type = be32toh(hdr.type);

	if (type)
		*type = btsnoop_type;

	return 0;
}

static uint16_t get_opcode_from_flags(uint8_t type, uint32_t flags)
{
	switch (type) {
	case HCI_COMMAND_PKT:
		return BTSNOOP_OPCODE_COMMAND_PKT;
	case HCI_ACLDATA_PKT:
		if (flags & 0x01)
			return BTSNOOP_OPCODE_ACL_RX_PKT;
		else
			return BTSNOOP_OPCODE_ACL_TX_PKT;
	case HCI_SCODATA_PKT:
		if (flags & 0x01)
			return BTSNOOP_OPCODE_SCO_RX_PKT;
		else
			return BTSNOOP_OPCODE_SCO_TX_PKT;
	case HCI_EVENT_PKT:
		return BTSNOOP_OPCODE_EVENT_PKT;
	case 0xff:
		if (flags & 0x02) {
			if (flags & 0x01)
				return BTSNOOP_OPCODE_EVENT_PKT;
			else
				return BTSNOOP_OPCODE_COMMAND_PKT;
		} else {
			if (flags & 0x01)
				return BTSNOOP_OPCODE_ACL_RX_PKT;
			else
				return BTSNOOP_OPCODE_ACL_TX_PKT;
		}
		break;
	}

	return 0xff;
}

int btsnoop_read_hci(struct timeval *tv, uint16_t *index, uint16_t *opcode,
						void *data, uint16_t *size)
{
	struct btsnoop_pkt pkt;
	uint32_t toread, flags;
	uint64_t ts;
	uint8_t pkt_type;
	ssize_t len;

	if (btsnoop_fd < 0)
		return -1;

	len = read(btsnoop_fd, &pkt, BTSNOOP_PKT_SIZE);
	if (len == 0)
		return -1;

	if (len < 0 || len != BTSNOOP_PKT_SIZE) {
		perror("Failed to read packet");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	toread = be32toh(pkt.size);
	if (toread > BTSNOOP_MAX_PACKET_SIZE) {
		perror("Packet len suspicially big: %u", toread);
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	flags = be32toh(pkt.flags);

	ts = be64toh(pkt.ts) - 0x00E03AB44A676000ll;
	tv->tv_sec = (ts / 1000000ll) + 946684800ll;
	tv->tv_usec = ts % 1000000ll;

	switch (btsnoop_type) {
	case BTSNOOP_TYPE_HCI:
		*index = 0;
		*opcode = get_opcode_from_flags(0xff, flags);
		break;

	case BTSNOOP_TYPE_UART:
		len = read(btsnoop_fd, &pkt_type, 1);
		if (len < 0) {
			perror("Failed to read packet type");
			close(btsnoop_fd);
			btsnoop_fd = -1;
			return -1;
		}
		toread--;

		*index = 0;
		*opcode = get_opcode_from_flags(pkt_type, flags);
		break;

	case BTSNOOP_TYPE_MONITOR:
		*index = flags >> 16;
		*opcode = flags & 0xffff;
		break;

	default:
		fprintf(stderr, "Unknown packet type\n");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	len = read(btsnoop_fd, data, toread);
	if (len < 0) {
		perror("Failed to read data");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	*size = toread;

	return 0;
}

int btsnoop_read_phy(struct timeval *tv, uint16_t *frequency,
						void *data, uint16_t *size)
{
	struct btsnoop_pkt pkt;
	uint32_t toread, flags;
	uint64_t ts;
	ssize_t len;

	if (btsnoop_fd < 0)
		return -1;

	len = read(btsnoop_fd, &pkt, BTSNOOP_PKT_SIZE);
	if (len == 0)
		return -1;

	if (len < 0 || len != BTSNOOP_PKT_SIZE) {
		perror("Failed to read packet");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	toread = be32toh(pkt.size);
	flags = be32toh(pkt.flags);

	ts = be64toh(pkt.ts) - 0x00E03AB44A676000ll;
	tv->tv_sec = (ts / 1000000ll) + 946684800ll;
	tv->tv_usec = ts % 1000000ll;

	switch (btsnoop_type) {
	case BTSNOOP_TYPE_SIMULATOR:
		if ((flags >> 16) != 1)
			break;
		*frequency = flags & 0xffff;
		break;

	default:
		fprintf(stderr, "Unknown packet type\n");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	len = read(btsnoop_fd, data, toread);
	if (len < 0) {
		perror("Failed to read data");
		close(btsnoop_fd);
		btsnoop_fd = -1;
		return -1;
	}

	*size = toread;

	return 0;
}

void btsnoop_close(void)
{
	if (btsnoop_fd < 0)
		return;

#ifdef __TIZEN_PATCH__
	if (btsnoop_path) {
		free(btsnoop_path);
		btsnoop_path = NULL;
	}
	btsnoop_rotate = -1;
	btsnoop_size = -1;
#endif

	close(btsnoop_fd);
	btsnoop_fd = -1;

	btsnoop_index = 0xffff;
}
