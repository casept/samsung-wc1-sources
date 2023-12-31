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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "lib/bluetooth.h"
#include "lib/sdp.h"
#include "lib/uuid.h"

#include "src/adapter.h"
#include "src/shared/util.h"
#include "attrib/gattrib.h"
#include "attrib/att.h"
#include "attrib/gatt.h"
#include "attrib/att-database.h"
#include "src/attrib-server.h"
#include "attrib/gatt-service.h"
#include "src/log.h"
#ifdef __TIZEN_PATCH__
#include "src/device.h"
#endif

struct gatt_info {
	bt_uuid_t uuid;
	uint8_t props;
	int authentication;
	int authorization;
	GSList *callbacks;
	unsigned int num_attrs;
	uint16_t *value_handle;
	uint16_t *ccc_handle;
};

struct attrib_cb {
	attrib_event_t event;
	void *fn;
	void *user_data;
};

static inline void put_uuid_le(const bt_uuid_t *src, void *dst)
{
	if (src->type == BT_UUID16)
		put_le16(src->value.u16, dst);
	else
		/* Convert from 128-bit BE to LE */
		bswap_128(&src->value.u128, dst);
}

static GSList *parse_opts(gatt_option opt1, va_list args)
{
	gatt_option opt = opt1;
	struct gatt_info *info;
	struct attrib_cb *cb;
	GSList *l = NULL;

	info = g_new0(struct gatt_info, 1);
	l = g_slist_append(l, info);

	while (opt != GATT_OPT_INVALID) {
		switch (opt) {
		case GATT_OPT_CHR_UUID16:
#ifdef __TIZEN_PATCH__
		case GATT_OPT_DESC_UUID16:
#endif
			bt_uuid16_create(&info->uuid, va_arg(args, int));
			/* characteristic declaration and value */
			info->num_attrs += 2;
			break;
		case GATT_OPT_CHR_UUID:
#ifdef __TIZEN_PATCH__
		case GATT_OPT_DESC_UUID:
#endif
			memcpy(&info->uuid, va_arg(args, bt_uuid_t *),
							sizeof(bt_uuid_t));
			/* characteristic declaration and value */
			info->num_attrs += 2;
			break;
		case GATT_OPT_CHR_PROPS:
#ifdef __TIZEN_PATCH__
		case GATT_OPT_DESC_PROPS:
#endif
			info->props = va_arg(args, int);

			if (info->props & (GATT_CHR_PROP_NOTIFY |
						GATT_CHR_PROP_INDICATE))
				/* client characteristic configuration */
				info->num_attrs += 1;

			/* TODO: "Extended Properties" property requires a
			 * descriptor, but it is not supported yet. */
			break;
		case GATT_OPT_CHR_VALUE_CB:
#ifdef __TIZEN_PATCH__
		case GATT_OPT_DESC_VALUE_CB:
#endif
			cb = g_new0(struct attrib_cb, 1);
			cb->event = va_arg(args, attrib_event_t);
			cb->fn = va_arg(args, void *);
			cb->user_data = va_arg(args, void *);
			info->callbacks = g_slist_append(info->callbacks, cb);
			break;
		case GATT_OPT_CHR_VALUE_GET_HANDLE:
			info->value_handle = va_arg(args, void *);
			break;
		case GATT_OPT_CCC_GET_HANDLE:
			info->ccc_handle = va_arg(args, void *);
			break;
		case GATT_OPT_CHR_AUTHENTICATION:
			info->authentication = va_arg(args, gatt_option);
			break;
		case GATT_OPT_CHR_AUTHORIZATION:
			info->authorization = va_arg(args, gatt_option);
			break;
		case GATT_CHR_VALUE_READ:
		case GATT_CHR_VALUE_WRITE:
		case GATT_CHR_VALUE_BOTH:
		case GATT_OPT_INVALID:
		default:
			error("Invalid option: %d", opt);
		}

		opt = va_arg(args, gatt_option);
#ifdef __TIZEN_PATCH__
		if (opt == GATT_OPT_CHR_UUID16 || opt == GATT_OPT_CHR_UUID ||
			opt == GATT_OPT_DESC_UUID16 || opt == GATT_OPT_DESC_UUID) {
#else
		if (opt == GATT_OPT_CHR_UUID16 || opt == GATT_OPT_CHR_UUID) {
#endif
			info = g_new0(struct gatt_info, 1);
			l = g_slist_append(l, info);
		}
	}

	return l;
}

static struct attribute *add_service_declaration(struct btd_adapter *adapter,
				uint16_t handle, uint16_t svc, bt_uuid_t *uuid)
{
	bt_uuid_t bt_uuid;
	uint8_t atval[16];
	int len;

	put_uuid_le(uuid, &atval[0]);
	len = bt_uuid_len(uuid);

	bt_uuid16_create(&bt_uuid, svc);

	return attrib_db_add(adapter, handle, &bt_uuid, ATT_NONE,
						ATT_NOT_PERMITTED, atval, len);
}

static int att_read_req(int authorization, int authentication, uint8_t props)
{
	if (authorization == GATT_CHR_VALUE_READ ||
				authorization == GATT_CHR_VALUE_BOTH)
		return ATT_AUTHORIZATION;
	else if (authentication == GATT_CHR_VALUE_READ ||
				authentication == GATT_CHR_VALUE_BOTH)
		return ATT_AUTHENTICATION;
	else if (!(props & GATT_CHR_PROP_READ))
		return ATT_NOT_PERMITTED;

	return ATT_NONE;
}

static int att_write_req(int authorization, int authentication, uint8_t props)
{
	if (authorization == GATT_CHR_VALUE_WRITE ||
				authorization == GATT_CHR_VALUE_BOTH)
		return ATT_AUTHORIZATION;
	else if (authentication == GATT_CHR_VALUE_WRITE ||
				authentication == GATT_CHR_VALUE_BOTH)
		return ATT_AUTHENTICATION;
	else if (!(props & (GATT_CHR_PROP_WRITE |
					GATT_CHR_PROP_WRITE_WITHOUT_RESP)))
		return ATT_NOT_PERMITTED;

	return ATT_NONE;
}

static int find_callback(gconstpointer a, gconstpointer b)
{
	const struct attrib_cb *cb = a;
	unsigned int event = GPOINTER_TO_UINT(b);

	return cb->event - event;
}

#ifdef __TIZEN_PATCH__
static gboolean add_descriptor(struct btd_adapter *adapter,
				uint16_t *handle, struct gatt_info *info)
{
	uint16_t h = *handle;
	struct attribute *a;
	bt_uuid_t bt_uuid;
	uint8_t atval[ATT_MAX_VALUE_LEN];
	GSList *l;

	if (info->uuid.type != BT_UUID16 && info->uuid.type != BT_UUID128) {
		error("Discriptors UUID or properties are missing");
		return FALSE;
	}

	/* Description declaration */
	if (info->uuid.type == BT_UUID16)
		bt_uuid16_create(&bt_uuid, info->uuid.value.u16);
	else
		bt_uuid128_create(&bt_uuid, info->uuid.value.u128);

	put_uuid_le(&info->uuid, &atval[0]);
/*	API not available in bluez 5.25
 * att_put_uuid(info->uuid, &atval[0]);*/

	a = attrib_db_add(adapter, h++, &bt_uuid, ATT_AUTHENTICATION,
			ATT_AUTHENTICATION, atval, info->uuid.type / 8);

	if (a == NULL) {
		return FALSE;
	}

	for (l = info->callbacks; l != NULL; l = l->next) {
		struct attrib_cb *cb = l->data;

		switch (cb->event) {
		case ATTRIB_READ:
			a->read_cb = cb->fn;
			break;
		case ATTRIB_WRITE:
			a->write_cb = cb->fn;
			break;
		}

		a->cb_user_data = cb->user_data;
	}

	if (info->value_handle != NULL)
		*info->value_handle = a->handle;

	*handle = h;

	return TRUE;
}
#endif

static gboolean add_characteristic(struct btd_adapter *adapter,
				uint16_t *handle, struct gatt_info *info)
{
	int read_req, write_req;
	uint16_t h = *handle;
	struct attribute *a;
	bt_uuid_t bt_uuid;
	uint8_t atval[ATT_MAX_VALUE_LEN];
	GSList *l;

	if ((info->uuid.type != BT_UUID16 && info->uuid.type != BT_UUID128) ||
								!info->props) {
		error("Characteristic UUID or properties are missing");
		return FALSE;
	}

	read_req = att_read_req(info->authorization, info->authentication,
								info->props);
	write_req = att_write_req(info->authorization, info->authentication,
								info->props);

	/* TODO: static characteristic values are not supported, therefore a
	 * callback must be always provided if a read/write property is set */
	if (read_req != ATT_NOT_PERMITTED) {
		gpointer reqs = GUINT_TO_POINTER(ATTRIB_READ);

		if (!g_slist_find_custom(info->callbacks, reqs,
							find_callback)) {
			error("Callback for read required");
			return FALSE;
		}
	}

	if (write_req != ATT_NOT_PERMITTED) {
		gpointer reqs = GUINT_TO_POINTER(ATTRIB_WRITE);

		if (!g_slist_find_custom(info->callbacks, reqs,
							find_callback)) {
			error("Callback for write required");
			return FALSE;
		}
	}

	/* characteristic declaration */
	bt_uuid16_create(&bt_uuid, GATT_CHARAC_UUID);
	atval[0] = info->props;
	put_le16(h + 1, &atval[1]);
	put_uuid_le(&info->uuid, &atval[3]);
	if (attrib_db_add(adapter, h++, &bt_uuid, ATT_NONE, ATT_NOT_PERMITTED,
				atval, 3 + info->uuid.type / 8) == NULL)
		return FALSE;

	/* characteristic value */
	a = attrib_db_add(adapter, h++, &info->uuid, read_req, write_req,
								NULL, 0);
	if (a == NULL)
		return FALSE;

	for (l = info->callbacks; l != NULL; l = l->next) {
		struct attrib_cb *cb = l->data;

		switch (cb->event) {
		case ATTRIB_READ:
			a->read_cb = cb->fn;
			break;
		case ATTRIB_WRITE:
			a->write_cb = cb->fn;
			break;
		}

		a->cb_user_data = cb->user_data;
	}

	if (info->value_handle != NULL)
		*info->value_handle = a->handle;

#ifndef __TIZEN_PATCH__
/* Since old attrib service implementation add descriptor by default
  * if notification and indication properties are set, As per new gatt server implemenation
  * CCCD are added by the application*/
	/* client characteristic configuration descriptor */
	if (info->props & (GATT_CHR_PROP_NOTIFY | GATT_CHR_PROP_INDICATE)) {
		uint8_t cfg_val[2];

		bt_uuid16_create(&bt_uuid, GATT_CLIENT_CHARAC_CFG_UUID);
		cfg_val[0] = 0x00;
		cfg_val[1] = 0x00;
		a = attrib_db_add(adapter, h++, &bt_uuid, ATT_NONE,
				ATT_AUTHENTICATION, cfg_val, sizeof(cfg_val));
		if (a == NULL)
			return FALSE;

		if (info->ccc_handle != NULL)
			*info->ccc_handle = a->handle;
	}
#endif

	*handle = h;

	return TRUE;
}

static void free_gatt_info(void *data)
{
	struct gatt_info *info = data;

	g_slist_free_full(info->callbacks, g_free);
	g_free(info);
}

static void service_attr_del(struct btd_adapter *adapter, uint16_t start_handle,
							uint16_t end_handle)
{
	uint16_t handle;

	/* For a 128-bit category primary service below handle should be checked
	 * for both non-zero as well as >= 0xffff. As on last iteration the
	 * handle will turn to 0 from 0xffff and loop will be infinite.
	 */
	for (handle = start_handle; (handle != 0 && handle <= end_handle);
								handle++) {
		if (attrib_db_del(adapter, handle) < 0)
			error("Can't delete handle 0x%04x", handle);
	}
}

#ifdef __TIZEN_PATCH__
static int is_gatt_connected(gconstpointer a1, gconstpointer a2)
{
	const struct btd_device *dev = a1;

	if (device_get_gatt_connected(dev))
		return 0;
	else
		return -1;
}

bool gatt_send_noty_ind(struct btd_adapter *adapter, const bt_uuid_t *uuid,
					uint8_t *value, size_t len)
{
	struct attribute *a;
	GAttrib *attrib;
	GList *l, *connections;
	struct btd_device *dev;
	uint16_t desc_handle;

	a = attribute_find(adapter, uuid);
	if (!a)
		return FALSE;

	desc_handle = a->handle + 1;

	connections = (GList *)btd_adapter_get_connections(adapter);

	do {
		l = g_list_find_custom(connections, GUINT_TO_POINTER(NULL),
							is_gatt_connected);
		if (l) {
			dev = l->data;
			attrib = attrib_from_device(dev);
			if (attrib) {
				/* Send the characteristic handle and value
				 * as notification or indication*/
				attrib_send_noty_ind(dev, attrib, a->handle,
							desc_handle, value, len);
				g_attrib_unref(attrib);
			}
			connections = (GList *)g_slist_next(connections);
		} else
			break;
	} while(l);

	return TRUE;
}

bool gatt_update_db(struct btd_adapter *adapter, const bt_uuid_t *uuid,
					uint8_t *value, size_t len)
{
	struct attribute *a;
	uint8_t status;

	a = attribute_find(adapter, uuid);
	if (!a)
		return FALSE;
	status = attrib_db_update(adapter, a->handle, NULL, value, len, NULL);

	if (status)
		return FALSE;

	return TRUE;
}

bool gatt_add_descriptor(struct btd_adapter *adapter, uint16_t *handle,
					uint16_t start_handle, gatt_option opt1, ...)
{
	va_list args;
	GSList *disc, *l;
	uint16_t disc_handle = *handle;

	va_start(args, opt1);
	disc = parse_opts(opt1, args);
	va_end(args);

	for (l = disc; l != NULL; l = l->next) {
		struct gatt_info *info = l->data;

		DBG("New Descriptor: handle 0x%04x, start handle 0x%04x",
						disc_handle, start_handle);
		if (!add_descriptor(adapter, &disc_handle, info)) {
			goto fail;
		}
	}

	g_slist_free_full(disc, free_gatt_info);
	*handle = disc_handle;

	return TRUE;

fail:
	g_slist_free_full(disc, free_gatt_info);
	return FALSE;
}

bool gatt_add_characteristic(struct btd_adapter *adapter,
		uint16_t *handle, uint16_t start_handle,
		gatt_option opt1, ...)
{
	va_list args;
	GSList *chrs, *l;
	uint16_t char_handle = *handle;

	va_start(args, opt1);
	chrs = parse_opts(opt1, args);
	va_end(args);

	for (l = chrs; l != NULL; l = l->next) {
		struct gatt_info *info = l->data;

		DBG("New characteristic: handle 0x%04x, start handle 0x%04x",
						char_handle, start_handle);
		if (!add_characteristic(adapter, &char_handle, info)) {
			service_attr_del(adapter, start_handle,
						char_handle - 1);
			goto fail;
		}
	}

	g_slist_free_full(chrs, free_gatt_info);
	*handle = char_handle;

	return TRUE;

fail:
	g_slist_free_full(chrs, free_gatt_info);
	return FALSE;
}

void attrib_remove_service(struct btd_adapter *adapter, uint16_t start_handle,
							uint16_t end_handle)
{
	DBG("gatt_remove_service");
	service_attr_del(adapter, start_handle, end_handle);
}

uint16_t gatt_prim_service_add(struct btd_adapter *adapter, uint16_t uuid,
				bt_uuid_t *svc_uuid, unsigned int size,
				uint16_t *start_handle)
{
	char uuidstr[MAX_LEN_UUID_STR];
	uint16_t handle, h;

	bt_uuid_to_string(svc_uuid, uuidstr, MAX_LEN_UUID_STR);
	if (svc_uuid->type != BT_UUID16 && svc_uuid->type != BT_UUID128) {
		error("Invalid service uuid: %s", uuidstr);
		return FALSE;
	}

	handle = attrib_db_find_avail(adapter, svc_uuid, size);
	if (handle == 0) {
		error("Not enough free handles to register service");
		goto fail;
	}

	DBG("New service: handle 0x%04x, UUID %s, %d attributes",
						handle, uuidstr, size);

	/* service declaration */
	h = handle;
	*start_handle = handle;
	if (add_service_declaration(adapter, h++, uuid, svc_uuid) == NULL)
		goto fail;

	return h;

fail:
	return 0;
}
#endif

gboolean gatt_service_add(struct btd_adapter *adapter, uint16_t uuid,
				bt_uuid_t *svc_uuid, gatt_option opt1, ...)
{
	char uuidstr[MAX_LEN_UUID_STR];
	uint16_t start_handle, h;
	unsigned int size;
	va_list args;
	GSList *chrs, *l;

	bt_uuid_to_string(svc_uuid, uuidstr, MAX_LEN_UUID_STR);

	if (svc_uuid->type != BT_UUID16 && svc_uuid->type != BT_UUID128) {
		error("Invalid service uuid: %s", uuidstr);
		return FALSE;
	}

	va_start(args, opt1);
	chrs = parse_opts(opt1, args);
	va_end(args);

	/* calculate how many attributes are necessary for this service */
	for (l = chrs, size = 1; l != NULL; l = l->next) {
		struct gatt_info *info = l->data;
		size += info->num_attrs;
	}

	start_handle = attrib_db_find_avail(adapter, svc_uuid, size);
	if (start_handle == 0) {
		error("Not enough free handles to register service");
		goto fail;
	}

	DBG("New service: handle 0x%04x, UUID %s, %d attributes",
						start_handle, uuidstr, size);

	/* service declaration */
	h = start_handle;
	if (add_service_declaration(adapter, h++, uuid, svc_uuid) == NULL)
		goto fail;

	for (l = chrs; l != NULL; l = l->next) {
		struct gatt_info *info = l->data;

		DBG("New characteristic: handle 0x%04x", h);
		if (!add_characteristic(adapter, &h, info)) {
			service_attr_del(adapter, start_handle, h - 1);
			goto fail;
		}
	}

	g_assert(size < USHRT_MAX);
	g_assert(h == 0 || (h - start_handle == (uint16_t) size));
	g_slist_free_full(chrs, free_gatt_info);

	return TRUE;

fail:
	g_slist_free_full(chrs, free_gatt_info);
	return FALSE;
}

#if 0
guint gatt_char_value_notify(GAttrib *attrib, uint16_t handle, uint8_t *value,
					int vlen, GAttribResultFunc func,	gpointer user_data)
{
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(attrib, &buflen);
	guint16 plen;

	plen = enc_notification(handle, value, vlen, buf, buflen);
	if (plen == 0)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, func, user_data, NULL);
}

guint gatt_char_value_indicate(GAttrib *attrib, uint16_t handle, uint8_t *value,
					int vlen, GAttribResultFunc func,	gpointer user_data)
{
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(attrib, &buflen);
	guint16 plen;

	plen = enc_indication(handle, value, vlen, buf, buflen);
	if (plen == 0)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, func, user_data, NULL);
}
#endif
