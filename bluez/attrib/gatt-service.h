/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
 *  Copyright (C) 2011  Marcel Holtmann <marcel@holtmann.org>
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

typedef enum {
	GATT_OPT_INVALID = 0,

	/* bt_uuid_t* value */
	GATT_OPT_CHR_UUID,

	/* a uint16 value */
	GATT_OPT_CHR_UUID16,

#ifdef __TIZEN_PATCH__
	/* bt_uuid_t* value */
	GATT_OPT_DESC_UUID,

	/* a uint16 value */
	GATT_OPT_DESC_UUID16,
#endif

	GATT_OPT_CHR_PROPS,
	GATT_OPT_CHR_VALUE_CB,
#ifdef __TIZEN_PATCH__
	GATT_OPT_DESC_PROPS,
	GATT_OPT_DESC_VALUE_CB,
#endif
	GATT_OPT_CHR_AUTHENTICATION,
	GATT_OPT_CHR_AUTHORIZATION,

	/* Get attribute handle for characteristic value */
	GATT_OPT_CHR_VALUE_GET_HANDLE,

	/* Get handle for ccc attribute */
	GATT_OPT_CCC_GET_HANDLE,

	/* arguments for authentication/authorization */
	GATT_CHR_VALUE_READ,
	GATT_CHR_VALUE_WRITE,
	GATT_CHR_VALUE_BOTH,
} gatt_option;

typedef enum {
	ATTRIB_READ,
	ATTRIB_WRITE,
} attrib_event_t;

gboolean gatt_service_add(struct btd_adapter *adapter, uint16_t uuid,
					bt_uuid_t *svc_uuid, gatt_option opt1, ...);
#ifdef __TIZEN_PATCH__
bool gatt_add_descriptor(struct btd_adapter *adapter, uint16_t *handle,
					uint16_t start_handle, gatt_option opt1, ...);
bool gatt_add_characteristic(struct btd_adapter *adapter,
				uint16_t *handle, uint16_t start_handle,
				 gatt_option opt1, ...);
uint16_t gatt_prim_service_add(struct btd_adapter *adapter, uint16_t uuid,
				bt_uuid_t *svc_uuid, unsigned int size,
				uint16_t *start_handle);
void attrib_remove_service(struct btd_adapter *adapter, uint16_t start_handle,
							uint16_t end_handle);
bool gatt_update_db(struct btd_adapter *adapter, const bt_uuid_t *uuid,
					uint8_t *value, size_t len);
bool gatt_send_noty_ind(struct btd_adapter *adapter, const bt_uuid_t *uuid,
					uint8_t *value, size_t len);
bool gatt_send_service_changed_ind(struct btd_adapter *adapter, bt_uuid_t *uuid,
                    uint16_t start_handle, uint16_t end_handle);
#endif
#if 0
guint gatt_char_value_notify(GAttrib *attrib, uint16_t handle, uint8_t *value,
					int vlen, GAttribResultFunc func,	gpointer user_data);
guint gatt_char_value_indicate(GAttrib *attrib, uint16_t handle, uint8_t *value,
					int vlen, GAttribResultFunc func,	gpointer user_data);
#endif
