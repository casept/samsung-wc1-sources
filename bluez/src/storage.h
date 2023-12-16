/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

int read_discoverable_timeout(const char *src, int *timeout);
int read_pairable_timeout(const char *src, int *timeout);
int read_on_mode(const char *src, char *mode, int length);
int read_local_name(const bdaddr_t *bdaddr, char *name);
#ifdef __TIZEN_PATCH__
int write_device_characteristics(const bdaddr_t *sba, const bdaddr_t *dba,
					uint8_t bdaddr_type, uint16_t handle,
					const char *chars);
char *read_device_characteristics(const bdaddr_t *sba, const bdaddr_t *dba,
					uint8_t bdaddr_type, uint16_t handle);
int write_device_attribute(const bdaddr_t *sba, const bdaddr_t *dba,
				uint8_t bdaddr_type, uint16_t handle,
				const char *chars);
#endif
sdp_record_t *record_from_string(const char *str);
sdp_record_t *find_record_in_list(sdp_list_t *recs, const char *uuid);
