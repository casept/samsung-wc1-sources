/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2015  Google Inc.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include "lib/bluetooth.h"
#include "lib/sdp.h"
#include "lib/sdp_lib.h"
#include "lib/uuid.h"
#include "btio/btio.h"
#include "gdbus/gdbus.h"
#include "src/shared/util.h"
#include "src/shared/queue.h"
#include "src/shared/att.h"
#include "src/shared/gatt-db.h"
#include "src/shared/gatt-server.h"
#include "log.h"
#include "error.h"
#include "adapter.h"
#include "device.h"
#include "gatt-database.h"
#include "dbus-common.h"
#include "profile.h"

#ifndef ATT_CID
#define ATT_CID 4
#endif

#ifndef ATT_PSM
#define ATT_PSM 31
#endif

#define GATT_MANAGER_IFACE	"org.bluez.GattManager1"
#define GATT_SERVICE_IFACE	"org.bluez.GattService1"
#define GATT_CHRC_IFACE		"org.bluez.GattCharacteristic1"
#define GATT_DESC_IFACE		"org.bluez.GattDescriptor1"

#define UUID_GAP	0x1800
#define UUID_GATT	0x1801

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

struct btd_gatt_database {
	struct btd_adapter *adapter;
	struct gatt_db *db;
	unsigned int db_id;
	GIOChannel *le_io;
	GIOChannel *l2cap_io;
	uint32_t gap_handle;
	uint32_t gatt_handle;
	struct queue *device_states;
	struct queue *ccc_callbacks;
	struct gatt_db_attribute *svc_chngd;
	struct gatt_db_attribute *svc_chngd_ccc;
	struct queue *services;
	struct queue *profiles;
};

struct external_service {
	struct btd_gatt_database *database;
	bool failed;
	char *owner;
	char *path;	/* Path to GattService1 */
	DBusMessage *reg;
	GDBusClient *client;
	GDBusProxy *proxy;
	struct gatt_db_attribute *attrib;
	uint16_t attr_cnt;
	struct queue *chrcs;
	struct queue *descs;
};

struct external_profile {
	struct btd_gatt_database *database;
	char *owner;
	char *path;	/* Path to GattProfile1 */
	unsigned int id;
	struct queue *profiles; /* btd_profile list */
};

struct external_chrc {
	struct external_service *service;
	char *path;
	GDBusProxy *proxy;
	uint8_t props;
	uint8_t ext_props;
	struct gatt_db_attribute *attrib;
	struct gatt_db_attribute *ccc;
	struct queue *pending_reads;
	struct queue *pending_writes;
	unsigned int ntfy_cnt;
};

struct external_desc {
	struct external_service *service;
	char *chrc_path;
	GDBusProxy *proxy;
	struct gatt_db_attribute *attrib;
	bool handled;
	struct queue *pending_reads;
	struct queue *pending_writes;
};

struct pending_op {
	unsigned int id;
	struct gatt_db_attribute *attrib;
	struct queue *owner_queue;
	struct iovec data;
#ifdef __TIZEN_PATCH__
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;
#endif
};

struct device_state {
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;
	struct queue *ccc_states;
};

struct ccc_state {
	uint16_t handle;
	uint8_t value[2];
};

struct ccc_cb_data {
	uint16_t handle;
	btd_gatt_database_ccc_write_t callback;
	btd_gatt_database_destroy_t destroy;
	void *user_data;
};

struct device_info {
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;
};

static void ccc_cb_free(void *data)
{
	struct ccc_cb_data *ccc_cb = data;

	if (ccc_cb->destroy)
		ccc_cb->destroy(ccc_cb->user_data);

	free(ccc_cb);
}

static bool ccc_cb_match_service(const void *data, const void *match_data)
{
	const struct ccc_cb_data *ccc_cb = data;
	const struct gatt_db_attribute *attrib = match_data;
	uint16_t start, end;

	if (!gatt_db_attribute_get_service_handles(attrib, &start, &end))
		return false;

	return ccc_cb->handle >= start && ccc_cb->handle <= end;
}

static bool ccc_cb_match_handle(const void *data, const void *match_data)
{
	const struct ccc_cb_data *ccc_cb = data;
	uint16_t handle = PTR_TO_UINT(match_data);

	return ccc_cb->handle == handle;
}

static bool dev_state_match(const void *a, const void *b)
{
	const struct device_state *dev_state = a;
	const struct device_info *dev_info = b;

	return bacmp(&dev_state->bdaddr, &dev_info->bdaddr) == 0 &&
				dev_state->bdaddr_type == dev_info->bdaddr_type;
}

static struct device_state *
find_device_state(struct btd_gatt_database *database, bdaddr_t *bdaddr,
							uint8_t bdaddr_type)
{
	struct device_info dev_info;

	memset(&dev_info, 0, sizeof(dev_info));

	bacpy(&dev_info.bdaddr, bdaddr);
	dev_info.bdaddr_type = bdaddr_type;

	return queue_find(database->device_states, dev_state_match, &dev_info);
}

#ifdef __TIZEN_PATCH__
static bool dev_addr_match(const void *a, const void *b)
{
	const struct device_state *dev_state = a;
	const struct device_info *dev_info = b;

	if (bacmp(&dev_state->bdaddr, &dev_info->bdaddr) == 0)
		return TRUE;

	return FALSE;
}

static struct device_state *
find_device_state_from_address(struct btd_gatt_database *database, const bdaddr_t *bdaddr)
{
	struct device_info dev_info;

	memset(&dev_info, 0, sizeof(dev_info));

	bacpy(&dev_info.bdaddr, bdaddr);

	return queue_find(database->device_states, dev_addr_match, &dev_info);
}
#endif

static bool ccc_state_match(const void *a, const void *b)
{
	const struct ccc_state *ccc = a;
	uint16_t handle = PTR_TO_UINT(b);

	return ccc->handle == handle;
}

static struct ccc_state *find_ccc_state(struct device_state *dev_state,
								uint16_t handle)
{
	return queue_find(dev_state->ccc_states, ccc_state_match,
							UINT_TO_PTR(handle));
}

static struct device_state *device_state_create(bdaddr_t *bdaddr,
							uint8_t bdaddr_type)
{
	struct device_state *dev_state;

	dev_state = new0(struct device_state, 1);
	if (!dev_state)
		return NULL;

	dev_state->ccc_states = queue_new();
	if (!dev_state->ccc_states) {
		free(dev_state);
		return NULL;
	}

	bacpy(&dev_state->bdaddr, bdaddr);
	dev_state->bdaddr_type = bdaddr_type;

	return dev_state;
}

static struct device_state *get_device_state(struct btd_gatt_database *database,
							bdaddr_t *bdaddr,
							uint8_t bdaddr_type)
{
	struct device_state *dev_state;

	/*
	 * Find and return a device state. If a matching state doesn't exist,
	 * then create a new one.
	 */
	dev_state = find_device_state(database, bdaddr, bdaddr_type);
	if (dev_state)
		return dev_state;

	dev_state = device_state_create(bdaddr, bdaddr_type);
	if (!dev_state)
		return NULL;

	queue_push_tail(database->device_states, dev_state);

	return dev_state;
}

static struct ccc_state *get_ccc_state(struct btd_gatt_database *database,
							bdaddr_t *bdaddr,
							uint8_t bdaddr_type,
							uint16_t handle)
{
	struct device_state *dev_state;
	struct ccc_state *ccc;

	dev_state = get_device_state(database, bdaddr, bdaddr_type);
	if (!dev_state)
		return NULL;

	ccc = find_ccc_state(dev_state, handle);
	if (ccc)
		return ccc;

	ccc = new0(struct ccc_state, 1);
	if (!ccc)
		return NULL;

	ccc->handle = handle;
	queue_push_tail(dev_state->ccc_states, ccc);

	return ccc;
}

static void device_state_free(void *data)
{
	struct device_state *state = data;

	queue_destroy(state->ccc_states, free);
	free(state);
}

static void cancel_pending_read(void *data)
{
	struct pending_op *op = data;

	gatt_db_attribute_read_result(op->attrib, op->id,
					BT_ATT_ERROR_REQUEST_NOT_SUPPORTED,
					NULL, 0);
	op->owner_queue = NULL;
}

static void cancel_pending_write(void *data)
{
	struct pending_op *op = data;

	gatt_db_attribute_write_result(op->attrib, op->id,
					BT_ATT_ERROR_REQUEST_NOT_SUPPORTED);
	op->owner_queue = NULL;
}

static void chrc_free(void *data)
{
	struct external_chrc *chrc = data;

	queue_destroy(chrc->pending_reads, cancel_pending_read);
	queue_destroy(chrc->pending_writes, cancel_pending_write);

	g_free(chrc->path);

	g_dbus_proxy_set_property_watch(chrc->proxy, NULL, NULL);
	g_dbus_proxy_unref(chrc->proxy);

	free(chrc);
}

static void desc_free(void *data)
{
	struct external_desc *desc = data;

	queue_destroy(desc->pending_reads, cancel_pending_read);
	queue_destroy(desc->pending_writes, cancel_pending_write);

	g_dbus_proxy_unref(desc->proxy);
	g_free(desc->chrc_path);

	free(desc);
}

static void service_free(void *data)
{
	struct external_service *service = data;

	queue_destroy(service->chrcs, chrc_free);
	queue_destroy(service->descs, desc_free);

	gatt_db_remove_service(service->database->db, service->attrib);

	if (service->client) {
		g_dbus_client_set_disconnect_watch(service->client, NULL, NULL);
		g_dbus_client_set_proxy_handlers(service->client, NULL, NULL,
								NULL, NULL);
		g_dbus_client_set_ready_watch(service->client, NULL, NULL);
		g_dbus_proxy_unref(service->proxy);
		g_dbus_client_unref(service->client);
	}

	if (service->reg)
		dbus_message_unref(service->reg);

	g_free(service->owner);
	g_free(service->path);

	free(service);
}

static void profile_remove(void *data)
{
	struct btd_profile *p = data;

	DBG("Removed \"%s\"", p->name);

	adapter_foreach(adapter_remove_profile, p);

	g_free((void *) p->name);
	g_free((void *) p->remote_uuid);

	free(p);
}

static void profile_release(struct external_profile *profile)
{
	DBusMessage *msg;

	if (!profile->id)
		return;

	DBG("Releasing \"%s\"", profile->owner);

	g_dbus_remove_watch(btd_get_dbus_connection(), profile->id);

	msg = dbus_message_new_method_call(profile->owner, profile->path,
						"org.bluez.GattProfile1",
						"Release");
	if (msg)
		g_dbus_send_message(btd_get_dbus_connection(), msg);
}

static void profile_free(void *data)
{
	struct external_profile *profile = data;

	queue_destroy(profile->profiles, profile_remove);

	profile_release(profile);

	g_free(profile->owner);
	g_free(profile->path);

	free(profile);
}

static void gatt_database_free(void *data)
{
	struct btd_gatt_database *database = data;

	if (database->le_io) {
		g_io_channel_shutdown(database->le_io, FALSE, NULL);
		g_io_channel_unref(database->le_io);
	}

	if (database->l2cap_io) {
		g_io_channel_shutdown(database->l2cap_io, FALSE, NULL);
		g_io_channel_unref(database->l2cap_io);
	}

	if (database->gatt_handle)
		adapter_service_remove(database->adapter,
							database->gatt_handle);

	if (database->gap_handle)
		adapter_service_remove(database->adapter, database->gap_handle);

	/* TODO: Persistently store CCC states before freeing them */
	gatt_db_unregister(database->db, database->db_id);

	queue_destroy(database->device_states, device_state_free);
	queue_destroy(database->services, service_free);
	queue_destroy(database->profiles, profile_free);
	queue_destroy(database->ccc_callbacks, ccc_cb_free);
	database->device_states = NULL;
	database->ccc_callbacks = NULL;

	gatt_db_unref(database->db);

	btd_adapter_unref(database->adapter);
	free(database);
}

static void connect_cb(GIOChannel *io, GError *gerr, gpointer user_data)
{
	struct btd_adapter *adapter;
	struct btd_device *device;
	uint8_t dst_type;
	bdaddr_t src, dst;

	DBG("New incoming LE ATT connection");

	if (gerr) {
		error("%s", gerr->message);
		return;
	}

	bt_io_get(io, &gerr, BT_IO_OPT_SOURCE_BDADDR, &src,
						BT_IO_OPT_DEST_BDADDR, &dst,
						BT_IO_OPT_DEST_TYPE, &dst_type,
						BT_IO_OPT_INVALID);
	if (gerr) {
		error("bt_io_get: %s", gerr->message);
		g_error_free(gerr);
		return;
	}

	adapter = adapter_find(&src);
	if (!adapter)
		return;

	device = btd_adapter_get_device(adapter, &dst, dst_type);
	if (!device)
		return;

	device_attach_att(device, io);
}

static void gap_device_name_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct btd_gatt_database *database = user_data;
	uint8_t error = 0;
	size_t len = 0;
	const uint8_t *value = NULL;
	const char *device_name;

	DBG("GAP Device Name read request\n");

	device_name = btd_adapter_get_name(database->adapter);
	len = strlen(device_name);

	if (offset > len) {
		error = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	len -= offset;
	value = len ? (const uint8_t *) &device_name[offset] : NULL;

done:
	gatt_db_attribute_read_result(attrib, id, error, value, len);
}

static void gap_appearance_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct btd_gatt_database *database = user_data;
	uint8_t error = 0;
	size_t len = 2;
	const uint8_t *value = NULL;
	uint8_t appearance[2];
	uint32_t dev_class;

	DBG("GAP Appearance read request\n");

	dev_class = btd_adapter_get_class(database->adapter);

	if (offset > 2) {
		error = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	appearance[0] = dev_class & 0x00ff;
	appearance[1] = (dev_class >> 8) & 0x001f;

	len -= offset;
	value = len ? &appearance[offset] : NULL;

done:
	gatt_db_attribute_read_result(attrib, id, error, value, len);
}

static sdp_record_t *record_new(uuid_t *uuid, uint16_t start, uint16_t end)
{
	sdp_list_t *svclass_id, *apseq, *proto[2], *root, *aproto;
	uuid_t root_uuid, proto_uuid, l2cap;
	sdp_record_t *record;
	sdp_data_t *psm, *sh, *eh;
	uint16_t lp = ATT_PSM;

	if (uuid == NULL)
		return NULL;

	if (start > end)
		return NULL;

	record = sdp_record_alloc();
	if (record == NULL)
		return NULL;

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(NULL, &root_uuid);
	sdp_set_browse_groups(record, root);
	sdp_list_free(root, NULL);

	svclass_id = sdp_list_append(NULL, uuid);
	sdp_set_service_classes(record, svclass_id);
	sdp_list_free(svclass_id, NULL);

	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(NULL, &l2cap);
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(NULL, proto[0]);

	sdp_uuid16_create(&proto_uuid, ATT_UUID);
	proto[1] = sdp_list_append(NULL, &proto_uuid);
	sh = sdp_data_alloc(SDP_UINT16, &start);
	proto[1] = sdp_list_append(proto[1], sh);
	eh = sdp_data_alloc(SDP_UINT16, &end);
	proto[1] = sdp_list_append(proto[1], eh);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(NULL, apseq);
	sdp_set_access_protos(record, aproto);

	sdp_data_free(psm);
	sdp_data_free(sh);
	sdp_data_free(eh);
	sdp_list_free(proto[0], NULL);
	sdp_list_free(proto[1], NULL);
	sdp_list_free(apseq, NULL);
	sdp_list_free(aproto, NULL);

	return record;
}

static uint32_t database_add_record(struct btd_gatt_database *database,
					uint16_t uuid,
					struct gatt_db_attribute *attr,
					const char *name)
{
	sdp_record_t *record;
	uint16_t start, end;
	uuid_t svc, gap_uuid;

	sdp_uuid16_create(&svc, uuid);
	gatt_db_attribute_get_service_handles(attr, &start, &end);

	record = record_new(&svc, start, end);
	if (!record)
		return 0;

	if (name != NULL)
#ifdef __TIZEN_PATCH__
		sdp_set_info_attr(record, name, "Samsung", NULL);
#else
		sdp_set_info_attr(record, name, "BlueZ", NULL);
#endif

	sdp_uuid16_create(&gap_uuid, UUID_GAP);
#ifndef __TIZEN_PATCH__
	if (sdp_uuid_cmp(&svc, &gap_uuid) == 0) {
		sdp_set_url_attr(record, "http://www.bluez.org/",
				"http://www.bluez.org/",
				"http://www.bluez.org/");
	}
#endif

	if (adapter_service_add(database->adapter, record) == 0)
		return record->handle;

	sdp_record_free(record);
	return 0;
}

static void populate_gap_service(struct btd_gatt_database *database)
{
	bt_uuid_t uuid;
	struct gatt_db_attribute *service;

	/* Add the GAP service */
	bt_uuid16_create(&uuid, UUID_GAP);
	service = gatt_db_add_service(database->db, &uuid, true, 5);
	database->gap_handle = database_add_record(database, UUID_GAP, service,
						"Generic Access Profile");

	/*
	 * Device Name characteristic.
	 */
	bt_uuid16_create(&uuid, GATT_CHARAC_DEVICE_NAME);
	gatt_db_service_add_characteristic(service, &uuid, BT_ATT_PERM_READ,
							BT_GATT_CHRC_PROP_READ,
							gap_device_name_read_cb,
							NULL, database);

	/*
	 * Device Appearance characteristic.
	 */
	bt_uuid16_create(&uuid, GATT_CHARAC_APPEARANCE);
	gatt_db_service_add_characteristic(service, &uuid, BT_ATT_PERM_READ,
							BT_GATT_CHRC_PROP_READ,
							gap_appearance_read_cb,
							NULL, database);

	gatt_db_service_set_active(service, true);
}

static bool get_dst_info(struct bt_att *att, bdaddr_t *dst, uint8_t *dst_type)
{
	GIOChannel *io = NULL;
	GError *gerr = NULL;

	io = g_io_channel_unix_new(bt_att_get_fd(att));
	if (!io)
		return false;

	bt_io_get(io, &gerr, BT_IO_OPT_DEST_BDADDR, dst,
						BT_IO_OPT_DEST_TYPE, dst_type,
						BT_IO_OPT_INVALID);
	if (gerr) {
		error("gatt: bt_io_get: %s", gerr->message);
		g_error_free(gerr);
		g_io_channel_unref(io);
		return false;
	}

	g_io_channel_unref(io);
	return true;
}

static void gatt_ccc_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct btd_gatt_database *database = user_data;
	struct ccc_state *ccc;
	uint16_t handle;
	uint8_t ecode = 0;
	const uint8_t *value = NULL;
	size_t len = 0;
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;

	handle = gatt_db_attribute_get_handle(attrib);

	DBG("CCC read called for handle: 0x%04x", handle);

	if (offset > 2) {
		ecode = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	if (!get_dst_info(att, &bdaddr, &bdaddr_type)) {
		ecode = BT_ATT_ERROR_UNLIKELY;
		goto done;
	}

	ccc = get_ccc_state(database, &bdaddr, bdaddr_type, handle);
	if (!ccc) {
		ecode = BT_ATT_ERROR_UNLIKELY;
		goto done;
	}

	len = 2 - offset;
	value = len ? &ccc->value[offset] : NULL;

done:
	gatt_db_attribute_read_result(attrib, id, ecode, value, len);
}

static void gatt_ccc_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct btd_gatt_database *database = user_data;
	struct ccc_state *ccc;
	struct ccc_cb_data *ccc_cb;
	uint16_t handle;
	uint8_t ecode = 0;
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;

	handle = gatt_db_attribute_get_handle(attrib);

	DBG("CCC write called for handle: 0x%04x", handle);

	if (!value || len != 2) {
		ecode = BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN;
		goto done;
	}

	if (offset > 2) {
		ecode = BT_ATT_ERROR_INVALID_OFFSET;
		goto done;
	}

	if (!get_dst_info(att, &bdaddr, &bdaddr_type)) {
		ecode = BT_ATT_ERROR_UNLIKELY;
		goto done;
	}

	ccc = get_ccc_state(database, &bdaddr, bdaddr_type, handle);
	if (!ccc) {
		ecode = BT_ATT_ERROR_UNLIKELY;
		goto done;
	}

	ccc_cb = queue_find(database->ccc_callbacks, ccc_cb_match_handle,
			UINT_TO_PTR(gatt_db_attribute_get_handle(attrib)));
	if (!ccc_cb) {
		ecode = BT_ATT_ERROR_UNLIKELY;
		goto done;
	}

	/* If value is identical, then just succeed */
	if (ccc->value[0] == value[0] && ccc->value[1] == value[1])
		goto done;

	if (ccc_cb->callback)
		ecode = ccc_cb->callback(get_le16(value), ccc_cb->user_data);

	if (!ecode) {
		ccc->value[0] = value[0];
		ccc->value[1] = value[1];
	}

done:
	gatt_db_attribute_write_result(attrib, id, ecode);
}

static struct gatt_db_attribute *
service_add_ccc(struct gatt_db_attribute *service,
				struct btd_gatt_database *database,
				btd_gatt_database_ccc_write_t write_callback,
				void *user_data,
				btd_gatt_database_destroy_t destroy)
{
	struct gatt_db_attribute *ccc;
	struct ccc_cb_data *ccc_cb;
	bt_uuid_t uuid;

	ccc_cb = new0(struct ccc_cb_data, 1);
	if (!ccc_cb) {
		error("Could not allocate memory for callback data");
		return NULL;
	}

	bt_uuid16_create(&uuid, GATT_CLIENT_CHARAC_CFG_UUID);
	ccc = gatt_db_service_add_descriptor(service, &uuid,
				BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
				gatt_ccc_read_cb, gatt_ccc_write_cb, database);
	if (!ccc) {
		error("Failed to create CCC entry in database");
		free(ccc_cb);
		return NULL;
	}

	ccc_cb->handle = gatt_db_attribute_get_handle(ccc);
	ccc_cb->callback = write_callback;
	ccc_cb->destroy = destroy;
	ccc_cb->user_data = user_data;

	queue_push_tail(database->ccc_callbacks, ccc_cb);

	return ccc;
}

struct gatt_db_attribute *
btd_gatt_database_add_ccc(struct btd_gatt_database *database,
				uint16_t service_handle,
				btd_gatt_database_ccc_write_t write_callback,
				void *user_data,
				btd_gatt_database_destroy_t destroy)
{
	struct gatt_db_attribute *service;

	if (!database || !service_handle)
		return NULL;

	service = gatt_db_get_attribute(database->db, service_handle);
	if (!service) {
		error("No service exists with handle: 0x%04x", service_handle);
		return NULL;
	}

	return service_add_ccc(service, database, write_callback, user_data,
								destroy);
}

static void populate_gatt_service(struct btd_gatt_database *database)
{
	bt_uuid_t uuid;
	struct gatt_db_attribute *service;

	/* Add the GATT service */
	bt_uuid16_create(&uuid, UUID_GATT);
	service = gatt_db_add_service(database->db, &uuid, true, 4);
	database->gatt_handle = database_add_record(database, UUID_GATT,
						service,
						"Generic Attribute Profile");

	bt_uuid16_create(&uuid, GATT_CHARAC_SERVICE_CHANGED);
	database->svc_chngd = gatt_db_service_add_characteristic(service, &uuid,
				BT_ATT_PERM_READ, BT_GATT_CHRC_PROP_INDICATE,
				NULL, NULL, database);

	database->svc_chngd_ccc = service_add_ccc(service, database, NULL, NULL,
									NULL);

	gatt_db_service_set_active(service, true);
}

static void register_core_services(struct btd_gatt_database *database)
{
	populate_gap_service(database);
	populate_gatt_service(database);
}

struct notify {
	struct btd_gatt_database *database;
	uint16_t handle, ccc_handle;
	const uint8_t *value;
	uint16_t len;
	bool indicate;
};

#ifdef __TIZEN_PATCH__
struct notify_indicate {
	struct btd_gatt_database *database;
	GDBusProxy *proxy;
	uint16_t handle, ccc_handle;
	const uint8_t *value;
	uint16_t len;
	bool indicate;
};

struct notify_indicate_cb {
	GDBusProxy *proxy;
	struct btd_device *device;
};

static void indicate_confirm_free(void *data)
{
	struct notify_indicate_cb *indicate = data;

	if (indicate)
		free(indicate);
}

static void indicate_confirm_setup_cb(DBusMessageIter *iter, void *user_data)
{
	struct btd_device *device = user_data;
	char dstaddr[18] = { 0 };
	char *addr_value = NULL;
	gboolean complete = FALSE;

	ba2str(device_get_address(device), dstaddr);
	addr_value = g_strdup(dstaddr);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING,
							&addr_value);

	complete = TRUE;
	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN,
							&complete);
}

static void indicate_confirm_reply_cb(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		DBG("Failed to send indication/notification");
		dbus_error_free(&error);
		return;
	}
}
#endif

static void conf_cb(void *user_data)
{
	DBG("GATT server received confirmation");
#ifdef __TIZEN_PATCH__
	struct notify_indicate_cb *confirm = user_data;

	if (confirm) {
		/* Send confirmation to application */
		if (g_dbus_proxy_method_call(confirm->proxy, "IndicateConfirm",
							indicate_confirm_setup_cb,
							indicate_confirm_reply_cb, confirm->device,
							NULL) == TRUE)
			return;
	}
#endif
}

#ifdef __TIZEN_PATCH__
static void send_notification_indication_to_device(void *data, void *user_data)
{
	struct device_state *device_state = data;
	struct notify_indicate *notify_indicate = user_data;
	struct ccc_state *ccc;
	struct btd_device *device;
	struct notify_indicate_cb *confirm;

	ccc = find_ccc_state(device_state, notify_indicate->ccc_handle);
	if (!ccc)
		return;

	if (!ccc->value[0] || (notify_indicate->indicate && !(ccc->value[0] & 0x02)))
		return;

	device = btd_adapter_get_device(notify_indicate->database->adapter,
						&device_state->bdaddr,
						device_state->bdaddr_type);
	if (!device)
		return;

	confirm = new0(struct notify_indicate_cb, 1);
	confirm->proxy = notify_indicate->proxy;
	confirm->device = device;
	/*
	 * TODO: If the device is not connected but bonded, send the
	 * notification/indication when it becomes connected.
	 */
	if (!notify_indicate->indicate) {
		DBG("GATT server sending notification");
		bt_gatt_server_send_notification(
					btd_device_get_gatt_server(device),
					notify_indicate->handle, notify_indicate->value,
					notify_indicate->len);
		/* In case of Notification, send response to application
		 * as remote device do not respond for notification */
		conf_cb(confirm);
		return;
	}

	DBG("GATT server sending indication");

	bt_gatt_server_send_indication(btd_device_get_gatt_server(device),
							notify_indicate->handle,
							notify_indicate->value,
							notify_indicate->len, conf_cb,
							confirm, indicate_confirm_free);
}

static void send_notification_indication_to_devices(GDBusProxy *proxy,
					struct btd_gatt_database *database,
					uint16_t handle, const uint8_t *value,
					uint16_t len, uint16_t ccc_handle,
					bool indicate)
{
	struct notify_indicate notify_indicate;
	DBG("");
	memset(&notify_indicate, 0, sizeof(notify_indicate));

	notify_indicate.database = database;
	notify_indicate.proxy = proxy;
	notify_indicate.handle = handle;
	notify_indicate.ccc_handle = ccc_handle;
	notify_indicate.value = value;
	notify_indicate.len = len;
	notify_indicate.indicate = indicate;

	queue_foreach(database->device_states, send_notification_indication_to_device,
								&notify_indicate);
}

static void send_unicast_notification_indication_to_device(GDBusProxy *proxy,
					struct btd_gatt_database *database,
					uint16_t handle, const uint8_t *value,
					uint16_t len, uint16_t ccc_handle,
					bool indicate, const bdaddr_t *unicast_addr)
{
	struct device_state *dev_state;
	struct notify_indicate notify_indicate;
	DBG("");

	memset(&notify_indicate, 0, sizeof(notify_indicate));

	notify_indicate.database = database;
	notify_indicate.proxy = proxy;
	notify_indicate.handle = handle;
	notify_indicate.ccc_handle = ccc_handle;
	notify_indicate.value = value;
	notify_indicate.len = len;
	notify_indicate.indicate = indicate;

	 /* Find and return a device state. */
	dev_state = find_device_state_from_address(database, unicast_addr);

	if (dev_state)
		send_notification_indication_to_device(dev_state, &notify_indicate);
}
#endif

static void send_notification_to_device(void *data, void *user_data)
{
	struct device_state *device_state = data;
	struct notify *notify = user_data;
	struct ccc_state *ccc;
	struct btd_device *device;

	ccc = find_ccc_state(device_state, notify->ccc_handle);
	if (!ccc)
		return;

	if (!ccc->value[0] || (notify->indicate && !(ccc->value[0] & 0x02)))
		return;

	device = btd_adapter_get_device(notify->database->adapter,
						&device_state->bdaddr,
						device_state->bdaddr_type);
	if (!device)
		return;

	/*
	 * TODO: If the device is not connected but bonded, send the
	 * notification/indication when it becomes connected.
	 */
	if (!notify->indicate) {
		DBG("GATT server sending notification");
		bt_gatt_server_send_notification(
					btd_device_get_gatt_server(device),
					notify->handle, notify->value,
					notify->len);
		return;
	}

	DBG("GATT server sending indication");
	bt_gatt_server_send_indication(btd_device_get_gatt_server(device),
							notify->handle,
							notify->value,
							notify->len, conf_cb,
							NULL, NULL);
}

static void send_notification_to_devices(struct btd_gatt_database *database,
					uint16_t handle, const uint8_t *value,
					uint16_t len, uint16_t ccc_handle,
					bool indicate)
{
	struct notify notify;

	memset(&notify, 0, sizeof(notify));

	notify.database = database;
	notify.handle = handle;
	notify.ccc_handle = ccc_handle;
	notify.value = value;
	notify.len = len;
	notify.indicate = indicate;

	queue_foreach(database->device_states, send_notification_to_device,
								&notify);
}

static void send_service_changed(struct btd_gatt_database *database,
					struct gatt_db_attribute *attrib)
{
	uint16_t start, end;
	uint8_t value[4];
	uint16_t handle, ccc_handle;

	if (!gatt_db_attribute_get_service_handles(attrib, &start, &end)) {
		error("Failed to obtain changed service handles");
		return;
	}

	handle = gatt_db_attribute_get_handle(database->svc_chngd);
	ccc_handle = gatt_db_attribute_get_handle(database->svc_chngd_ccc);

	if (!handle || !ccc_handle) {
		error("Failed to obtain handles for \"Service Changed\""
							" characteristic");
		return;
	}

	put_le16(start, value);
	put_le16(end, value + 2);

	send_notification_to_devices(database, handle, value, sizeof(value),
							ccc_handle, true);
}

static void gatt_db_service_added(struct gatt_db_attribute *attrib,
								void *user_data)
{
	struct btd_gatt_database *database = user_data;

	DBG("GATT Service added to local database");

	send_service_changed(database, attrib);
}

static bool ccc_match_service(const void *data, const void *match_data)
{
	const struct ccc_state *ccc = data;
	const struct gatt_db_attribute *attrib = match_data;
	uint16_t start, end;

	if (!gatt_db_attribute_get_service_handles(attrib, &start, &end))
		return false;

	return ccc->handle >= start && ccc->handle <= end;
}

static void remove_device_ccc(void *data, void *user_data)
{
	struct device_state *state = data;

	queue_remove_all(state->ccc_states, ccc_match_service, user_data, free);
}

static void gatt_db_service_removed(struct gatt_db_attribute *attrib,
								void *user_data)
{
	struct btd_gatt_database *database = user_data;

	DBG("Local GATT service removed");

	send_service_changed(database, attrib);

	queue_foreach(database->device_states, remove_device_ccc, attrib);
	queue_remove_all(database->ccc_callbacks, ccc_cb_match_service, attrib,
								ccc_cb_free);
}

struct svc_match_data {
	const char *path;
	const char *sender;
};

static bool match_service(const void *a, const void *b)
{
	const struct external_service *service = a;
	const struct svc_match_data *data = b;

	return g_strcmp0(service->path, data->path) == 0 &&
				g_strcmp0(service->owner, data->sender) == 0;
}

static gboolean service_free_idle_cb(void *data)
{
	service_free(data);

	return FALSE;
}

static void service_remove_helper(void *data)
{
	struct external_service *service = data;

	queue_remove(service->database->services, service);

	/*
	 * Do not run in the same loop, this may be a disconnect
	 * watch call and GDBusClient should not be destroyed.
	 */
	g_idle_add(service_free_idle_cb, service);
}

static void client_disconnect_cb(DBusConnection *conn, void *user_data)
{
	DBG("Client disconnected");

	service_remove_helper(user_data);
}

static void service_remove(void *data)
{
	struct external_service *service = data;

	/*
	 * Set callback to NULL to avoid potential race condition
	 * when calling remove_service and GDBusClient unref.
	 */
	g_dbus_client_set_disconnect_watch(service->client, NULL, NULL);

	/*
	 * Set proxy handlers to NULL, so that this gets called only once when
	 * the first proxy that belongs to this service gets removed.
	 */
	g_dbus_client_set_proxy_handlers(service->client, NULL, NULL,
								NULL, NULL);

	service_remove_helper(service);
}

static struct external_chrc *chrc_create(struct external_service *service,
							GDBusProxy *proxy,
							const char *path)
{
	struct external_chrc *chrc;

	chrc = new0(struct external_chrc, 1);
	if (!chrc)
		return NULL;

	chrc->pending_reads = queue_new();
	if (!chrc->pending_reads) {
		free(chrc);
		return NULL;
	}

	chrc->pending_writes = queue_new();
	if (!chrc->pending_writes) {
		queue_destroy(chrc->pending_reads, NULL);
		free(chrc);
		return NULL;
	}

	chrc->path = g_strdup(path);
	if (!chrc->path) {
		queue_destroy(chrc->pending_reads, NULL);
		queue_destroy(chrc->pending_writes, NULL);
		free(chrc);
		return NULL;
	}

	chrc->service = service;
	chrc->proxy = g_dbus_proxy_ref(proxy);

	return chrc;
}

static struct external_desc *desc_create(struct external_service *service,
							GDBusProxy *proxy,
							const char *chrc_path)
{
	struct external_desc *desc;

	desc = new0(struct external_desc, 1);
	if (!desc)
		return NULL;

	desc->pending_reads = queue_new();
	if (!desc->pending_reads) {
		free(desc);
		return NULL;
	}

	desc->pending_writes = queue_new();
	if (!desc->pending_writes) {
		queue_destroy(desc->pending_reads, NULL);
		free(desc);
		return NULL;
	}

	desc->chrc_path = g_strdup(chrc_path);
	if (!desc->chrc_path) {
		queue_destroy(desc->pending_reads, NULL);
		queue_destroy(desc->pending_writes, NULL);
		free(desc);
		return NULL;
	}

	desc->service = service;
	desc->proxy = g_dbus_proxy_ref(proxy);

	return desc;
}

static bool incr_attr_count(struct external_service *service, uint16_t incr)
{
	if (service->attr_cnt > UINT16_MAX - incr)
		return false;

	service->attr_cnt += incr;

	return true;
}

static bool parse_path(GDBusProxy *proxy, const char *name, const char **path)
{
	DBusMessageIter iter;

	if (!g_dbus_proxy_get_property(proxy, name, &iter))
		return false;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_OBJECT_PATH)
		return false;

	dbus_message_iter_get_basic(&iter, path);

	return true;
}

static bool check_service_path(GDBusProxy *proxy,
					struct external_service *service)
{
	const char *service_path;

	if (!parse_path(proxy, "Service", &service_path))
		return false;

	return g_strcmp0(service_path, service->path) == 0;
}

static bool parse_flags(GDBusProxy *proxy, uint8_t *props, uint8_t *ext_props)
{
	DBusMessageIter iter, array;
	const char *flag;

	*props = *ext_props = 0;

	if (!g_dbus_proxy_get_property(proxy, "Flags", &iter))
		return false;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
		return false;

	dbus_message_iter_recurse(&iter, &array);

	do {
		if (dbus_message_iter_get_arg_type(&array) != DBUS_TYPE_STRING)
			return false;

		dbus_message_iter_get_basic(&array, &flag);

		if (!strcmp("broadcast", flag))
			*props |= BT_GATT_CHRC_PROP_BROADCAST;
		else if (!strcmp("read", flag))
			*props |= BT_GATT_CHRC_PROP_READ;
		else if (!strcmp("write-without-response", flag))
			*props |= BT_GATT_CHRC_PROP_WRITE_WITHOUT_RESP;
		else if (!strcmp("write", flag))
			*props |= BT_GATT_CHRC_PROP_WRITE;
		else if (!strcmp("notify", flag))
			*props |= BT_GATT_CHRC_PROP_NOTIFY;
		else if (!strcmp("indicate", flag))
			*props |= BT_GATT_CHRC_PROP_INDICATE;
		else if (!strcmp("authenticated-signed-writes", flag))
			*props |= BT_GATT_CHRC_PROP_AUTH;
		else if (!strcmp("reliable-write", flag))
			*ext_props |= BT_GATT_CHRC_EXT_PROP_RELIABLE_WRITE;
		else if (!strcmp("writable-auxiliaries", flag))
			*ext_props |= BT_GATT_CHRC_EXT_PROP_WRITABLE_AUX;
		else {
			error("Invalid characteristic flag: %s", flag);
			return false;
		}
	} while (dbus_message_iter_next(&array));

	if (*ext_props)
		*props |= BT_GATT_CHRC_PROP_EXT_PROP;

	return true;
}

static void proxy_added_cb(GDBusProxy *proxy, void *user_data)
{
	struct external_service *service = user_data;
	const char *iface, *path;

	if (service->failed || service->attrib)
		return;

	iface = g_dbus_proxy_get_interface(proxy);
	path = g_dbus_proxy_get_path(proxy);

	if (!g_str_has_prefix(path, service->path))
		return;

	if (g_strcmp0(iface, GATT_SERVICE_IFACE) == 0) {
		if (service->proxy)
			return;

		/*
		 * TODO: We may want to support adding included services in a
		 * single hierarchy.
		 */
		if (g_strcmp0(path, service->path) != 0) {
			error("Multiple services added within hierarchy");
			service->failed = true;
			return;
		}

		/* Add 1 for the service declaration */
		if (!incr_attr_count(service, 1)) {
			error("Failed to increment attribute count");
			service->failed = true;
			return;
		}

		service->proxy = g_dbus_proxy_ref(proxy);
	} else if (g_strcmp0(iface, GATT_CHRC_IFACE) == 0) {
		struct external_chrc *chrc;

		if (g_strcmp0(path, service->path) == 0) {
			error("Characteristic path same as service path");
			service->failed = true;
			return;
		}

		chrc = chrc_create(service, proxy, path);
		if (!chrc) {
			service->failed = true;
			return;
		}

		/*
		 * Add 2 for the characteristic declaration and the value
		 * attribute.
		 */
		if (!incr_attr_count(service, 2)) {
			error("Failed to increment attribute count");
			service->failed = true;
			return;
		}

		/*
		 * Parse characteristic flags (i.e. properties) here since they
		 * are used to determine if any special descriptors should be
		 * created.
		 */
		if (!parse_flags(proxy, &chrc->props, &chrc->ext_props)) {
			error("Failed to parse characteristic properties");
			service->failed = true;
			return;
		}

		if ((chrc->props & BT_GATT_CHRC_PROP_NOTIFY ||
				chrc->props & BT_GATT_CHRC_PROP_INDICATE) &&
				!incr_attr_count(service, 1)) {
			error("Failed to increment attribute count for CCC");
			service->failed = true;
			return;
		}

		if (chrc->ext_props && !incr_attr_count(service, 1)) {
			error("Failed to increment attribute count for CEP");
			service->failed = true;
			return;
		}

		queue_push_tail(service->chrcs, chrc);
	} else if (g_strcmp0(iface, GATT_DESC_IFACE) == 0) {
		struct external_desc *desc;
		const char *chrc_path;

		if (!parse_path(proxy, "Characteristic", &chrc_path)) {
			error("Failed to obtain characteristic path for "
								"descriptor");
			service->failed = true;
			return;
		}

		desc = desc_create(service, proxy, chrc_path);
		if (!desc) {
			service->failed = true;
			return;
		}

		/* Add 1 for the descriptor attribute */
		if (!incr_attr_count(service, 1)) {
			error("Failed to increment attribute count");
			service->failed = true;
			return;
		}

		queue_push_tail(service->descs, desc);
	} else {
		DBG("Ignoring unrelated interface: %s", iface);
		return;
	}

	DBG("Object added to service - path: %s, iface: %s", path, iface);
}

static void proxy_removed_cb(GDBusProxy *proxy, void *user_data)
{
	struct external_service *service = user_data;
	const char *path;

	path = g_dbus_proxy_get_path(proxy);

	if (!g_str_has_prefix(path, service->path))
		return;

	DBG("Proxy removed - removing service: %s", service->path);

	service_remove(service);
}

static bool parse_uuid(GDBusProxy *proxy, bt_uuid_t *uuid)
{
	DBusMessageIter iter;
	bt_uuid_t tmp;
	const char *uuidstr;

	if (!g_dbus_proxy_get_property(proxy, "UUID", &iter))
		return false;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
		return false;

	dbus_message_iter_get_basic(&iter, &uuidstr);

	if (bt_string_to_uuid(uuid, uuidstr) < 0)
		return false;

	/* GAP & GATT services are created and managed by BlueZ */
	bt_uuid16_create(&tmp, UUID_GAP);
	if (!bt_uuid_cmp(&tmp, uuid)) {
		error("GAP service must be handled by BlueZ");
		return false;
	}

	bt_uuid16_create(&tmp, UUID_GATT);
	if (!bt_uuid_cmp(&tmp, uuid)) {
		error("GATT service must be handled by BlueZ");
		return false;
	}

	return true;
}

static bool parse_primary(GDBusProxy *proxy, bool *primary)
{
	DBusMessageIter iter;
	dbus_bool_t val;

	if (!g_dbus_proxy_get_property(proxy, "Primary", &iter))
		return false;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_BOOLEAN)
		return false;

	dbus_message_iter_get_basic(&iter, &val);

	*primary = val;

	return true;
}

static uint8_t dbus_error_to_att_ecode(const char *error_name)
{
	/* TODO: Parse error ATT ecode from error_message */

	if (strcmp(error_name, "org.bluez.Error.Failed") == 0)
		return 0x80;  /* For now return this "application error" */

	if (strcmp(error_name, "org.bluez.Error.NotSupported") == 0)
		return BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;

	if (strcmp(error_name, "org.bluez.Error.NotAuthorized") == 0)
		return BT_ATT_ERROR_AUTHORIZATION;

	if (strcmp(error_name, "org.bluez.Error.InvalidValueLength") == 0)
		return BT_ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LEN;

	return 0;
}

static void read_reply_cb(DBusMessage *message, void *user_data)
{
	struct pending_op *op = user_data;
	DBusError err;
	DBusMessageIter iter, array;
	uint8_t ecode = 0;
	uint8_t *value = NULL;
	int len = 0;

	if (!op->owner_queue) {
		DBG("Pending read was canceled when object got removed");
		return;
	}

	dbus_error_init(&err);

	if (dbus_set_error_from_message(&err, message) == TRUE) {
		DBG("Failed to read value: %s: %s", err.name, err.message);
		ecode = dbus_error_to_att_ecode(err.name);
		ecode = ecode ? ecode : BT_ATT_ERROR_READ_NOT_PERMITTED;
		dbus_error_free(&err);
		goto done;
	}

	dbus_message_iter_init(message, &iter);

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		/*
		 * Return not supported for this, as the external app basically
		 * doesn't properly support reading from this characteristic.
		 */
		ecode = BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;
		error("Invalid return value received for \"ReadValue\"");
		goto done;
	}

	dbus_message_iter_recurse(&iter, &array);
	dbus_message_iter_get_fixed_array(&array, &value, &len);

	if (len < 0) {
		ecode = BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;
		value = NULL;
		len = 0;
		goto done;
	}

	/* Truncate the value if it's too large */
	len = MIN(BT_ATT_MAX_VALUE_LEN, len);
	value = len ? value : NULL;

done:
	gatt_db_attribute_read_result(op->attrib, op->id, ecode, value, len);
}

static void pending_op_free(void *data)
{
	struct pending_op *op = data;

	if (op->owner_queue)
		queue_remove(op->owner_queue, op);

	free(op);
}

#ifdef __TIZEN_PATCH__
static struct pending_op *pending_read_new(struct queue *owner_queue,
					struct gatt_db_attribute *attrib,
					struct bt_att *att,
					unsigned int id)
#else
static struct pending_op *pending_read_new(struct queue *owner_queue,
					struct gatt_db_attribute *attrib,
					unsigned int id)
#endif
{
	struct pending_op *op;
#ifdef __TIZEN_PATCH__
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;
	char address[18];
#endif

	op = new0(struct pending_op, 1);
	if (!op)
		return NULL;

#ifdef __TIZEN_PATCH__
	if (!get_dst_info(att, &bdaddr, &bdaddr_type)) {
		return NULL;
	}
#endif

	op->owner_queue = owner_queue;
#ifdef __TIZEN_PATCH__
	memcpy(&op->bdaddr, &bdaddr, sizeof(bdaddr_t));
	op->bdaddr_type = bdaddr_type;
#endif

	op->attrib = attrib;
	op->id = id;
	queue_push_tail(owner_queue, op);

	return op;
}

#ifdef __TIZEN_PATCH__
static void read_setup_cb(DBusMessageIter *iter, void *user_data)
{
	struct pending_op *op = user_data;
	char dstaddr[18] = { 0 };
	char *addr_value = NULL;
	uint16_t offset = 0;

	ba2str(&op->bdaddr, dstaddr);
	addr_value = g_strdup(dstaddr);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING,
							&addr_value);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE,
							&op->id);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16,
							&offset);
}
#endif

#ifdef __TIZEN_PATCH__
static void send_read(struct gatt_db_attribute *attrib, struct bt_att *att,
						GDBusProxy *proxy, struct queue *owner_queue,
						unsigned int id)
#else
static void send_read(struct gatt_db_attribute *attrib, GDBusProxy *proxy,
						struct queue *owner_queue,
						unsigned int id)
#endif
{
	struct pending_op *op;
	uint8_t ecode = BT_ATT_ERROR_UNLIKELY;

#ifdef __TIZEN_PATCH__
	op = pending_read_new(owner_queue, attrib, att, id);
#else
	op = pending_read_new(owner_queue, attrib, id);
#endif
	if (!op) {
		error("Failed to allocate memory for pending read call");
		ecode = BT_ATT_ERROR_INSUFFICIENT_RESOURCES;
		goto error;
	}

#ifdef __TIZEN_PATCH__
	if (g_dbus_proxy_method_call(proxy, "ReadValue", read_setup_cb, read_reply_cb,
						op, pending_op_free) == TRUE)
#else
	if (g_dbus_proxy_method_call(proxy, "ReadValue", NULL, read_reply_cb,
						op, pending_op_free) == TRUE)
#endif
		return;

	pending_op_free(op);

error:
	gatt_db_attribute_read_result(attrib, id, ecode, NULL, 0);
}

static void write_setup_cb(DBusMessageIter *iter, void *user_data)
{
	struct pending_op *op = user_data;
	DBusMessageIter array;
#ifdef __TIZEN_PATCH__
	char dstaddr[18] = { 0 };
	char *addr_value = NULL;
	uint16_t offset = 0;
#endif

#ifdef __TIZEN_PATCH__
	ba2str(&op->bdaddr, dstaddr);
	addr_value = g_strdup(dstaddr);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING,
							&addr_value);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE,
							&op->id);

	dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16,
							&offset);
#endif

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &array);
	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
					&op->data.iov_base, op->data.iov_len);
	dbus_message_iter_close_container(iter, &array);
}

static void write_reply_cb(DBusMessage *message, void *user_data)
{
	struct pending_op *op = user_data;
	DBusError err;
	DBusMessageIter iter;
	uint8_t ecode = 0;

	if (!op->owner_queue) {
		DBG("Pending write was canceled when object got removed");
		return;
	}

	dbus_error_init(&err);

	if (dbus_set_error_from_message(&err, message) == TRUE) {
		DBG("Failed to write value: %s: %s", err.name, err.message);
		ecode = dbus_error_to_att_ecode(err.name);
		ecode = ecode ? ecode : BT_ATT_ERROR_WRITE_NOT_PERMITTED;
		dbus_error_free(&err);
		goto done;
	}

	dbus_message_iter_init(message, &iter);
	if (dbus_message_iter_has_next(&iter)) {
		/*
		 * Return not supported for this, as the external app basically
		 * doesn't properly support the "WriteValue" API.
		 */
		ecode = BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;
		error("Invalid return value received for \"WriteValue\"");
	}

done:
	gatt_db_attribute_write_result(op->attrib, op->id, ecode);
}

#ifdef __TIZEN_PATCH__
static struct pending_op *pending_write_new(struct queue *owner_queue,
					struct gatt_db_attribute *attrib, struct bt_att *att,
					unsigned int id,
					const uint8_t *value,
					size_t len)
#else
static struct pending_op *pending_write_new(struct queue *owner_queue,
					struct gatt_db_attribute *attrib,
					unsigned int id,
					const uint8_t *value,
					size_t len)
#endif
{
	struct pending_op *op;
#ifdef __TIZEN_PATCH__
	bdaddr_t bdaddr;
	uint8_t bdaddr_type;
	char address[18];
#endif

	op = new0(struct pending_op, 1);
	if (!op)
		return NULL;
#ifdef __TIZEN_PATCH__
	if (!get_dst_info(att, &bdaddr, &bdaddr_type)) {
		return NULL;
	}
#endif

	op->data.iov_base = (uint8_t *) value;
	op->data.iov_len = len;
#ifdef __TIZEN_PATCH__
	memcpy(&op->bdaddr, &bdaddr, sizeof(bdaddr_t));
	op->bdaddr_type = bdaddr_type;
#endif

	op->owner_queue = owner_queue;
	op->attrib = attrib;
	op->id = id;
	queue_push_tail(owner_queue, op);

	return op;
}

#ifdef __TIZEN_PATCH__
static void send_write(struct gatt_db_attribute *attrib, struct bt_att *att,
					GDBusProxy *proxy, struct queue *owner_queue,
					unsigned int id, const uint8_t *value, size_t len)
#else
static void send_write(struct gatt_db_attribute *attrib, GDBusProxy *proxy,
					struct queue *owner_queue,
					unsigned int id,
					const uint8_t *value, size_t len)
#endif
{
	struct pending_op *op;
	uint8_t ecode = BT_ATT_ERROR_UNLIKELY;

#ifdef __TIZEN_PATCH__
	op = pending_write_new(owner_queue, attrib, att, id, value, len);
#else
	op = pending_write_new(owner_queue, attrib, id, value, len);
#endif
	if (!op) {
		error("Failed to allocate memory for pending read call");
		ecode = BT_ATT_ERROR_INSUFFICIENT_RESOURCES;
		goto error;
	}

	if (g_dbus_proxy_method_call(proxy, "WriteValue", write_setup_cb,
						write_reply_cb, op,
						pending_op_free) == TRUE)
		return;

	pending_op_free(op);

error:
	gatt_db_attribute_write_result(attrib, id, ecode);
}

static uint32_t permissions_from_props(uint8_t props, uint8_t ext_props)
{
	uint32_t perm = 0;

	if (props & BT_GATT_CHRC_PROP_WRITE ||
			props & BT_GATT_CHRC_PROP_WRITE_WITHOUT_RESP ||
			ext_props & BT_GATT_CHRC_EXT_PROP_RELIABLE_WRITE)
		perm |= BT_ATT_PERM_WRITE;

	if (props & BT_GATT_CHRC_PROP_READ)
		perm |= BT_ATT_PERM_READ;

	return perm;
}

static uint8_t ccc_write_cb(uint16_t value, void *user_data)
{
	struct external_chrc *chrc = user_data;

	DBG("External CCC write received with value: 0x%04x", value);

	/* Notifications/indications disabled */
	if (!value) {
		if (!chrc->ntfy_cnt)
			return 0;

		if (__sync_sub_and_fetch(&chrc->ntfy_cnt, 1))
			return 0;

		/*
		 * Send request to stop notifying. This is best-effort
		 * operation, so simply ignore the return the value.
		 */
		g_dbus_proxy_method_call(chrc->proxy, "StopNotify", NULL,
							NULL, NULL, NULL);
		return 0;
	}

	/*
	 * TODO: All of the errors below should fall into the so called
	 * "Application Error" range. Since there is no well defined error for
	 * these, we return a generic ATT protocol error for now.
	 */

	if (chrc->ntfy_cnt == UINT_MAX) {
		/* Maximum number of per-device CCC descriptors configured */
		return BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;
	}

	/* Don't support undefined CCC values yet */
	if (value > 2 ||
		(value == 1 && !(chrc->props & BT_GATT_CHRC_PROP_NOTIFY)) ||
		(value == 2 && !(chrc->props & BT_GATT_CHRC_PROP_INDICATE)))
		return BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;

	/*
	 * Always call StartNotify for an incoming enable and ignore the return
	 * value for now.
	 */
	if (g_dbus_proxy_method_call(chrc->proxy,
						"StartNotify", NULL, NULL,
						NULL, NULL) == FALSE)
		return BT_ATT_ERROR_REQUEST_NOT_SUPPORTED;

	__sync_fetch_and_add(&chrc->ntfy_cnt, 1);

	return 0;
}

static void property_changed_cb(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
	struct external_chrc *chrc = user_data;
	DBusMessageIter array;
	uint8_t *value = NULL;
	int len = 0;
#ifdef __TIZEN_PATCH__
	bool enable = FALSE;
	const bdaddr_t *unicast_addr = NULL;
#endif

#ifdef __TIZEN_PATCH__
	if (strcmp(name, "Value") == 0) {
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY) {
			DBG("Malformed \"Value\" property received");
			return;
		}

		dbus_message_iter_recurse(iter, &array);
		dbus_message_iter_get_fixed_array(&array, &value, &len);

		if (len < 0) {
			DBG("Malformed \"Value\" property received");
			return;
		}

		/* Truncate the value if it's too large */
		len = MIN(BT_ATT_MAX_VALUE_LEN, len);
		value = len ? value : NULL;
	} else if (strcmp(name, "Notifying") == 0) {
		gboolean notify_indicate = FALSE;

		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_BOOLEAN) {
			DBG("Malformed \"Notifying\" property received");
			return;
		}

		dbus_message_iter_get_basic(iter, &notify_indicate);

		DBG("Set Notification %d", notify_indicate);
		/* Set notification/indication */
		set_ccc_notify_indicate(chrc->ccc, notify_indicate);
		return;
	} else if (strcmp(name, "Unicast") == 0) {
		const char *address = NULL;
		if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING) {
			DBG("Malformed \"Value\" property received");
			return;
		}

		dbus_message_iter_get_basic(iter, &address);

		if (address) {
			/* Set the address for unicast notification/indication */
			set_ccc_unicast_address(chrc->ccc, address);
		}
		return;
	} else
		return;

	enable = get_ccc_notify_indicate(chrc->ccc);

	if (enable) {

		unicast_addr = get_ccc_unicast_address(chrc->ccc);

		if (unicast_addr && bacmp(unicast_addr, BDADDR_ANY)) {
			send_unicast_notification_indication_to_device(proxy,
					chrc->service->database,
					gatt_db_attribute_get_handle(chrc->attrib),
					value, len,
					gatt_db_attribute_get_handle(chrc->ccc),
					chrc->props & BT_GATT_CHRC_PROP_INDICATE,
					unicast_addr);
			/* reset the unicast address */
			set_ccc_unicast_address(chrc->ccc, NULL);
		} else
			send_notification_indication_to_devices(proxy,
					chrc->service->database,
					gatt_db_attribute_get_handle(chrc->attrib),
					value, len,
					gatt_db_attribute_get_handle(chrc->ccc),
					chrc->props & BT_GATT_CHRC_PROP_INDICATE);

		set_ccc_notify_indicate(chrc->ccc, FALSE);
	}
#else
	if (strcmp(name, "Value"))
		return;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY) {
		DBG("Malformed \"Value\" property received");
		return;
	}

	dbus_message_iter_recurse(iter, &array);
	dbus_message_iter_get_fixed_array(&array, &value, &len);

	if (len < 0) {
		DBG("Malformed \"Value\" property received");
		return;
	}

	/* Truncate the value if it's too large */
	len = MIN(BT_ATT_MAX_VALUE_LEN, len);
	value = len ? value : NULL;

	send_notification_to_devices(chrc->service->database,
				gatt_db_attribute_get_handle(chrc->attrib),
				value, len,
				gatt_db_attribute_get_handle(chrc->ccc),
				chrc->props & BT_GATT_CHRC_PROP_INDICATE);
#endif
}

static bool database_add_ccc(struct external_service *service,
						struct external_chrc *chrc)
{
	if (!(chrc->props & BT_GATT_CHRC_PROP_NOTIFY) &&
				!(chrc->props & BT_GATT_CHRC_PROP_INDICATE))
		return true;

	chrc->ccc = service_add_ccc(service->attrib, service->database,
						ccc_write_cb, chrc, NULL);
	if (!chrc->ccc) {
		error("Failed to create CCC entry for characteristic");
		return false;
	}

	if (g_dbus_proxy_set_property_watch(chrc->proxy, property_changed_cb,
							chrc) == FALSE) {
		error("Failed to set up property watch for characteristic");
		return false;
	}

	DBG("Created CCC entry for characteristic");

	return true;
}

static void cep_write_cb(struct gatt_db_attribute *attrib, int err,
								void *user_data)
{
	if (err)
		DBG("Failed to store CEP value in the database");
	else
		DBG("Stored CEP value in the database");
}

static bool database_add_cep(struct external_service *service,
						struct external_chrc *chrc)
{
	struct gatt_db_attribute *cep;
	bt_uuid_t uuid;
	uint8_t value[2];

	if (!chrc->ext_props)
		return true;

	bt_uuid16_create(&uuid, GATT_CHARAC_EXT_PROPER_UUID);
	cep = gatt_db_service_add_descriptor(service->attrib, &uuid,
							BT_ATT_PERM_READ,
							NULL, NULL, NULL);
	if (!cep) {
		error("Failed to create CEP entry for characteristic");
		return false;
	}

	memset(value, 0, sizeof(value));
	value[0] = chrc->ext_props;

	if (!gatt_db_attribute_write(cep, 0, value, sizeof(value), 0, NULL,
							cep_write_cb, NULL)) {
		DBG("Failed to store CEP value in the database");
		return false;
	}

	DBG("Created CEP entry for characteristic");

	return true;
}

static void desc_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct external_desc *desc = user_data;

	if (desc->attrib != attrib) {
		error("Read callback called with incorrect attribute");
		return;
	}
#ifdef __TIZEN_PATCH__
	send_read(attrib, att, desc->proxy, desc->pending_reads, id);
#else
	send_read(attrib, desc->proxy, desc->pending_reads, id);
#endif
}

static void desc_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct external_desc *desc = user_data;

	if (desc->attrib != attrib) {
		error("Read callback called with incorrect attribute");
		return;
	}

#ifdef __TIZEN_PATCH__
	send_write(attrib, att, desc->proxy, desc->pending_writes, id, value, len);
#else
	send_write(attrib, desc->proxy, desc->pending_writes, id, value, len);
#endif
}

static bool database_add_desc(struct external_service *service,
						struct external_desc *desc)
{
	bt_uuid_t uuid;

	if (!parse_uuid(desc->proxy, &uuid)) {
		error("Failed to read \"UUID\" property of descriptor");
		return false;
	}

	/*
	 * TODO: Set permissions based on a D-Bus property of the external
	 * descriptor.
	 */
	desc->attrib = gatt_db_service_add_descriptor(service->attrib, &uuid,
					BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
					desc_read_cb, desc_write_cb, desc);
	if (!desc->attrib) {
		error("Failed to create descriptor entry in database");
		return false;
	}

	desc->handled = true;

	return true;
}

static void chrc_read_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct external_chrc *chrc = user_data;

	if (chrc->attrib != attrib) {
		error("Read callback called with incorrect attribute");
		return;
	}

#ifdef __TIZEN_PATCH__
	send_read(attrib, att, chrc->proxy, chrc->pending_reads, id);
#else
	send_read(attrib, chrc->proxy, chrc->pending_reads, id);
#endif
}

static void chrc_write_cb(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
	struct external_chrc *chrc = user_data;

	if (chrc->attrib != attrib) {
		error("Write callback called with incorrect attribute");
		return;
	}

#ifdef __TIZEN_PATCH__
	send_write(attrib, att, chrc->proxy, chrc->pending_writes, id, value, len);
#else
	send_write(attrib, chrc->proxy, chrc->pending_writes, id, value, len);
#endif
}

#ifdef __TIZEN_PATCH__
static bool database_check_ccc_desc(struct external_desc *desc)
{
	bt_uuid_t uuid, uuid_ccc;
	char uuidstr[MAX_LEN_UUID_STR];

	if (!parse_uuid(desc->proxy, &uuid)) {
		error("Failed to read \"UUID\" property of descriptor");
		return false;
	}

	bt_uuid16_create(&uuid_ccc, GATT_CLIENT_CHARAC_CFG_UUID);
	if (bt_uuid_cmp(&uuid, &uuid_ccc) == 0)
		return true;
	else
		return false;
}
#endif

static bool database_add_chrc(struct external_service *service,
						struct external_chrc *chrc)
{
	bt_uuid_t uuid;
	uint32_t perm;
	const struct queue_entry *entry;

	if (!parse_uuid(chrc->proxy, &uuid)) {
		error("Failed to read \"UUID\" property of characteristic");
		return false;
	}

	if (!check_service_path(chrc->proxy, service)) {
		error("Invalid service path for characteristic");
		return false;
	}

	/*
	 * TODO: Once shared/gatt-server properly supports permission checks,
	 * set the permissions based on a D-Bus property of the external
	 * characteristic.
	 */
	perm = permissions_from_props(chrc->props, chrc->ext_props);
	chrc->attrib = gatt_db_service_add_characteristic(service->attrib,
						&uuid, perm,
						chrc->props, chrc_read_cb,
						chrc_write_cb, chrc);
	if (!chrc->attrib) {
		error("Failed to create characteristic entry in database");
		return false;
	}

#ifndef __TIZEN_PATCH__
	/* Existing implementation adds CCC descriptor by default
	  * if notification and indication properties are set. But as per the requirment
	  * CCCD shall be added by the application */
	if (!database_add_ccc(service, chrc))
		return false;
#endif

	if (!database_add_cep(service, chrc))
		return false;

	/* Handle the descriptors that belong to this characteristic. */
	entry = queue_get_entries(service->descs);
	while (entry) {
		struct external_desc *desc = entry->data;

		if (desc->handled || g_strcmp0(desc->chrc_path, chrc->path)) {
#ifdef __TIZEN_PATCH__
			entry = entry->next;
#endif
			continue;
		}
#ifdef __TIZEN_PATCH__
		/* Check if Application wants to add CCC and use existing
		 * implemenation to add CCC descriptors */
		if (database_check_ccc_desc(desc)) {
			if (!database_add_ccc(service, chrc)) {
				chrc->attrib = NULL;
				return false;
			}
			desc->attrib = chrc->ccc;
			desc->handled = true;
		} else if (!database_add_desc(service, desc)) {
			chrc->attrib = NULL;
			error("Failed to create descriptor entry");
				return false;
		}
#else
		if (!database_add_desc(service, desc)) {
			chrc->attrib = NULL;
			error("Failed to create descriptor entry");
			return false;
		}
#endif
		entry = entry->next;
	}

	return true;
}

static bool match_desc_unhandled(const void *a, const void *b)
{
	const struct external_desc *desc = a;

	return !desc->handled;
}

static bool create_service_entry(struct external_service *service)
{
	bt_uuid_t uuid;
	bool primary;
	const struct queue_entry *entry;

	if (!parse_uuid(service->proxy, &uuid)) {
		error("Failed to read \"UUID\" property of service");
		return false;
	}

	if (!parse_primary(service->proxy, &primary)) {
		error("Failed to read \"Primary\" property of service");
		return false;
	}

	service->attrib = gatt_db_add_service(service->database->db, &uuid,
						primary, service->attr_cnt);
	if (!service->attrib)
		return false;

	entry = queue_get_entries(service->chrcs);
	while (entry) {
		struct external_chrc *chrc = entry->data;

		if (!database_add_chrc(service, chrc)) {
			error("Failed to add characteristic");
			goto fail;
		}

		entry = entry->next;
	}

	/* If there are any unhandled descriptors, return an error */
	if (queue_find(service->descs, match_desc_unhandled, NULL)) {
		error("Found descriptor with no matching characteristic!");
		goto fail;
	}

	gatt_db_service_set_active(service->attrib, true);

	return true;

fail:
	gatt_db_remove_service(service->database->db, service->attrib);
	service->attrib = NULL;

	return false;
}

static void client_ready_cb(GDBusClient *client, void *user_data)
{
	struct external_service *service = user_data;
	DBusMessage *reply;
	bool fail = false;

	if (!service->proxy || service->failed) {
		error("No valid external GATT objects found");
		fail = true;
		reply = btd_error_failed(service->reg,
					"No valid service object found");
		goto reply;
	}

	if (!create_service_entry(service)) {
		error("Failed to create GATT service entry in local database");
		fail = true;
		reply = btd_error_failed(service->reg,
					"Failed to create entry in database");
		goto reply;
	}

	DBG("GATT service registered: %s", service->path);

	reply = dbus_message_new_method_return(service->reg);

reply:
	g_dbus_send_message(btd_get_dbus_connection(), reply);
	dbus_message_unref(service->reg);
	service->reg = NULL;

	if (fail)
		service_remove(service);
}

static struct external_service *service_create(DBusConnection *conn,
					DBusMessage *msg, const char *path)
{
	struct external_service *service;
	const char *sender = dbus_message_get_sender(msg);

	if (!path || !g_str_has_prefix(path, "/"))
		return NULL;

	service = new0(struct external_service, 1);
	if (!service)
		return NULL;

	service->client = g_dbus_client_new_full(conn, sender, path, path);
	if (!service->client)
		goto fail;

	service->owner = g_strdup(sender);
	if (!service->owner)
		goto fail;

	service->path = g_strdup(path);
	if (!service->path)
		goto fail;

	service->chrcs = queue_new();
	if (!service->chrcs)
		goto fail;

	service->descs = queue_new();
	if (!service->descs)
		goto fail;

	service->reg = dbus_message_ref(msg);

	g_dbus_client_set_disconnect_watch(service->client,
						client_disconnect_cb, service);
	g_dbus_client_set_proxy_handlers(service->client, proxy_added_cb,
							proxy_removed_cb, NULL,
							service);
	g_dbus_client_set_ready_watch(service->client, client_ready_cb,
								service);

	return service;

fail:
	service_free(service);
	return NULL;
}

static DBusMessage *manager_register_service(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	struct btd_gatt_database *database = user_data;
	const char *sender = dbus_message_get_sender(msg);
	DBusMessageIter args;
	const char *path;
	struct external_service *service;
	struct svc_match_data match_data;

	if (!dbus_message_iter_init(msg, &args))
		return btd_error_invalid_args(msg);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_OBJECT_PATH)
		return btd_error_invalid_args(msg);

	dbus_message_iter_get_basic(&args, &path);

	match_data.path = path;
	match_data.sender = sender;

	if (queue_find(database->services, match_service, &match_data))
		return btd_error_already_exists(msg);

	dbus_message_iter_next(&args);
	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
		return btd_error_invalid_args(msg);

	service = service_create(conn, msg, path);
	if (!service)
		return btd_error_failed(msg, "Failed to register service");

	DBG("Registering service - path: %s", path);

	service->database = database;
	queue_push_tail(database->services, service);

	return NULL;
}

static DBusMessage *manager_unregister_service(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	struct btd_gatt_database *database = user_data;
	const char *sender = dbus_message_get_sender(msg);
	const char *path;
	DBusMessageIter args;
	struct external_service *service;
	struct svc_match_data match_data;

	if (!dbus_message_iter_init(msg, &args))
		return btd_error_invalid_args(msg);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_OBJECT_PATH)
		return btd_error_invalid_args(msg);

	dbus_message_iter_get_basic(&args, &path);

	match_data.path = path;
	match_data.sender = sender;

	service = queue_remove_if(database->services, match_service,
								&match_data);
	if (!service)
		return btd_error_does_not_exist(msg);

	service_free(service);

	return dbus_message_new_method_return(msg);
}

static void profile_exited(DBusConnection *conn, void *user_data)
{
	struct external_profile *profile = user_data;

	DBG("\"%s\" exited", profile->owner);

	profile->id = 0;

	queue_remove(profile->database->profiles, profile);

	profile_free(profile);
}

static int profile_add(struct external_profile *profile, const char *uuid)
{
	struct btd_profile *p;

	p = new0(struct btd_profile, 1);
	if (!p)
		return -ENOMEM;

	/* Assign directly to avoid having extra fields */
	p->name = (const void *) g_strdup_printf("%s%s/%s", profile->owner,
							profile->path, uuid);
	if (!p->name)
		return -ENOMEM;

	p->remote_uuid = (const void *) g_strdup(uuid);
	if (!p->remote_uuid)
		return -ENOMEM;

	p->auto_connect = true;

	queue_push_tail(profile->profiles, p);

	DBG("Added \"%s\"", p->name);

	return 0;
}

static void add_profile(void *data, void *user_data)
{
	struct btd_adapter *adapter = user_data;

	adapter_add_profile(adapter, data);
}

static int profile_create(DBusConnection *conn,
				struct btd_gatt_database *database,
				const char *sender, const char *path,
				DBusMessageIter *iter)
{
	struct external_profile *profile;
	DBusMessageIter uuids;

	if (!path || !g_str_has_prefix(path, "/"))
		return -EINVAL;

	profile = new0(struct external_profile, 1);
	if (!profile)
		return -ENOMEM;

	profile->owner = g_strdup(sender);
	if (!profile->owner)
		goto fail;

	profile->path = g_strdup(path);
	if (!profile->path)
		goto fail;

	profile->profiles = queue_new();
	if (!profile->profiles)
		goto fail;

	profile->database = database;
	profile->id = g_dbus_add_disconnect_watch(conn, sender, profile_exited,
								profile, NULL);

	dbus_message_iter_recurse(iter, &uuids);

	while (dbus_message_iter_get_arg_type(&uuids) == DBUS_TYPE_STRING) {
		const char *uuid;

		dbus_message_iter_get_basic(&uuids, &uuid);

		if (profile_add(profile, uuid) < 0)
			goto fail;

		dbus_message_iter_next(&uuids);
	}

	if (queue_isempty(profile->profiles))
		goto fail;

	queue_foreach(profile->profiles, add_profile, database->adapter);
	queue_push_tail(database->profiles, profile);

	return 0;

fail:
	profile_free(profile);
	return -EINVAL;
}

static bool match_profile(const void *a, const void *b)
{
	const struct external_profile *profile = a;
	const struct svc_match_data *data = b;

	return g_strcmp0(profile->path, data->path) == 0 &&
				g_strcmp0(profile->owner, data->sender) == 0;
}

static DBusMessage *manager_register_profile(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	struct btd_gatt_database *database = user_data;
	const char *sender = dbus_message_get_sender(msg);
	DBusMessageIter args;
	const char *path;
	struct svc_match_data match_data;

	DBG("sender %s", sender);

	if (!dbus_message_iter_init(msg, &args))
		return btd_error_invalid_args(msg);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_OBJECT_PATH)
		return btd_error_invalid_args(msg);

	dbus_message_iter_get_basic(&args, &path);

	match_data.path = path;
	match_data.sender = sender;

	if (queue_find(database->profiles, match_profile, &match_data))
		return btd_error_already_exists(msg);

	dbus_message_iter_next(&args);
	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
		return btd_error_invalid_args(msg);

	if (profile_create(conn, database, sender, path, &args) < 0)
		return btd_error_failed(msg, "Failed to register profile");

	return dbus_message_new_method_return(msg);
}

static DBusMessage *manager_unregister_profile(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	struct btd_gatt_database *database = user_data;
	const char *sender = dbus_message_get_sender(msg);
	const char *path;
	DBusMessageIter args;
	struct external_profile *profile;
	struct svc_match_data match_data;

	if (!dbus_message_iter_init(msg, &args))
		return btd_error_invalid_args(msg);

	if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_OBJECT_PATH)
		return btd_error_invalid_args(msg);

	dbus_message_iter_get_basic(&args, &path);

	match_data.path = path;
	match_data.sender = sender;

	profile = queue_remove_if(database->profiles, match_profile,
								&match_data);
	if (!profile)
		return btd_error_does_not_exist(msg);

	profile_free(profile);

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable manager_methods[] = {
#ifdef __TIZEN_PATCH__
	{ GDBUS_ASYNC_METHOD("RegisterService",
			GDBUS_ARGS({ "service", "o" }, { "options", "a{sv}" }),
			NULL, manager_register_service) },
	{ GDBUS_ASYNC_METHOD("UnregisterService",
					GDBUS_ARGS({ "service", "o" }),
					NULL, manager_unregister_service) },
	{ GDBUS_ASYNC_METHOD("RegisterProfile",
			GDBUS_ARGS({ "profile", "o" }, { "UUIDs", "as" },
			{ "options", "a{sv}" }), NULL,
			manager_register_profile) },
	{ GDBUS_ASYNC_METHOD("UnregisterProfile",
					GDBUS_ARGS({ "profile", "o" }),
					NULL, manager_unregister_profile) },
	{ }
#else
	{ GDBUS_EXPERIMENTAL_ASYNC_METHOD("RegisterService",
			GDBUS_ARGS({ "service", "o" }, { "options", "a{sv}" }),
			NULL, manager_register_service) },
	{ GDBUS_EXPERIMENTAL_ASYNC_METHOD("UnregisterService",
					GDBUS_ARGS({ "service", "o" }),
					NULL, manager_unregister_service) },
	{ GDBUS_EXPERIMENTAL_ASYNC_METHOD("RegisterProfile",
			GDBUS_ARGS({ "profile", "o" }, { "UUIDs", "as" },
			{ "options", "a{sv}" }), NULL,
			manager_register_profile) },
	{ GDBUS_EXPERIMENTAL_ASYNC_METHOD("UnregisterProfile",
					GDBUS_ARGS({ "profile", "o" }),
					NULL, manager_unregister_profile) },
	{ }
#endif
};

struct btd_gatt_database *btd_gatt_database_new(struct btd_adapter *adapter)
{
	struct btd_gatt_database *database;
	GError *gerr = NULL;
	const bdaddr_t *addr;

	if (!adapter)
		return NULL;

	database = new0(struct btd_gatt_database, 1);
	if (!database)
		return NULL;

	database->adapter = btd_adapter_ref(adapter);
	database->db = gatt_db_new();
	if (!database->db)
		goto fail;

	database->device_states = queue_new();
	if (!database->device_states)
		goto fail;

	database->services = queue_new();
	if (!database->services)
		goto fail;

	database->profiles = queue_new();
	if (!database->profiles)
		goto fail;

	database->ccc_callbacks = queue_new();
	if (!database->ccc_callbacks)
		goto fail;

	database->db_id = gatt_db_register(database->db, gatt_db_service_added,
							gatt_db_service_removed,
							database, NULL);
	if (!database->db_id)
		goto fail;

	addr = btd_adapter_get_address(adapter);
	database->le_io = bt_io_listen(connect_cb, NULL, NULL, NULL, &gerr,
					BT_IO_OPT_SOURCE_BDADDR, addr,
					BT_IO_OPT_SOURCE_TYPE, BDADDR_LE_PUBLIC,
					BT_IO_OPT_CID, ATT_CID,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_LOW,
					BT_IO_OPT_INVALID);
	if (!database->le_io) {
		error("Failed to start listening: %s", gerr->message);
		g_error_free(gerr);
		goto fail;
	}

	/* BR/EDR socket */
	database->l2cap_io = bt_io_listen(connect_cb, NULL, NULL, NULL, &gerr,
					BT_IO_OPT_SOURCE_BDADDR, addr,
					BT_IO_OPT_PSM, ATT_PSM,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_LOW,
					BT_IO_OPT_INVALID);
	if (database->l2cap_io == NULL) {
		error("Failed to start listening: %s", gerr->message);
		g_error_free(gerr);
		goto fail;
	}

	if (g_dbus_register_interface(btd_get_dbus_connection(),
						adapter_get_path(adapter),
						GATT_MANAGER_IFACE,
						manager_methods, NULL, NULL,
						database, NULL))
		DBG("GATT Manager registered for adapter: %s",
						adapter_get_path(adapter));

	register_core_services(database);

	return database;

fail:
	gatt_database_free(database);

	return NULL;
}

void btd_gatt_database_destroy(struct btd_gatt_database *database)
{
	if (!database)
		return;

	g_dbus_unregister_interface(btd_get_dbus_connection(),
					adapter_get_path(database->adapter),
					GATT_MANAGER_IFACE);

	gatt_database_free(database);
}

struct gatt_db *btd_gatt_database_get_db(struct btd_gatt_database *database)
{
	if (!database)
		return NULL;

	return database->db;
}
