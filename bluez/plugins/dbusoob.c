/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  ST-Ericsson SA
 *
 *  Author: Szymon Janc <szymon.janc@tieto.com> for ST-Ericsson
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

#include <errno.h>
#include "gdbus/gdbus.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/sdp.h>

#include "src/plugin.h"
#include "src/log.h"
#include "src/dbus-common.h"
#include "src/adapter.h"
#include "src/oob.h"
#include "src/device.h"
#include "src/eir.h"
#include "src/agent.h"
#include "src/hcid.h"
#include "src/error.h"

#define OOB_INTERFACE	"org.bluez.OutOfBand"

struct oob_request {
	struct btd_adapter *adapter;
	DBusMessage *msg;
};

static GSList *oob_requests = NULL;
static DBusConnection *connection = NULL;

static gint oob_request_cmp(gconstpointer a, gconstpointer b)
{
	const struct oob_request *data = a;
	const struct btd_adapter *adapter = b;

	return data->adapter != adapter;
}

static struct oob_request *find_oob_request(struct btd_adapter *adapter)
{
	GSList *match;

	match = g_slist_find_custom(oob_requests, adapter, oob_request_cmp);

	if (match)
		return match->data;

	return NULL;
}

static void read_local_data_complete(struct btd_adapter *adapter, uint8_t *hash,
				uint8_t *randomizer)
{
	struct DBusMessage *reply;
	struct oob_request *oob_request;

	oob_request = find_oob_request(adapter);
	if (!oob_request)
		return;

	if (hash && randomizer)
		reply = g_dbus_create_reply(oob_request->msg,
			DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &hash, 16,
			DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &randomizer, 16,
			DBUS_TYPE_INVALID);
	else
		reply = btd_error_failed(oob_request->msg,
					"Failed to read local OOB data.");

	oob_requests = g_slist_remove(oob_requests, oob_request);
	dbus_message_unref(oob_request->msg);
	g_free(oob_request);

	if (!reply) {
		error("Couldn't allocate D-Bus message");
		return;
	}

	if (!g_dbus_send_message(connection, reply))
		error("D-Bus send failed");
}

static DBusMessage *read_local_data(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct btd_adapter *adapter = data;
	struct oob_request *oob_request;
	struct oob_handler *handler;

	if (find_oob_request(adapter))
		return btd_error_in_progress(msg);

	if (btd_adapter_read_local_oob_data(adapter))
		return btd_error_failed(msg, "Request failed.");

	oob_request = g_new(struct oob_request, 1);
	oob_request->adapter = adapter;
	oob_requests = g_slist_append(oob_requests, oob_request);
	oob_request->msg = dbus_message_ref(msg);

	handler = g_new0(struct oob_handler, 1);
#ifdef __TIZEN_PATCH__
	handler->read_local_cb = (oob_read_local_cb_t)oob_read_local_data_complete;
#else
	handler->read_local_cb = oob_read_local_data_complete;
#endif
	handler->user_data = dbus_message_ref(oob_request->msg);

	btd_adapter_set_oob_handler(oob_request->adapter, handler);

	return NULL;
}

static DBusMessage *add_remote_data(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct btd_adapter *adapter = data;
	uint8_t *hash, *randomizer;
	int32_t hlen, rlen;
	const char *addr;
	bdaddr_t bdaddr;

	if (!dbus_message_get_args(msg, NULL,
			DBUS_TYPE_STRING, &addr,
			DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &hash, &hlen,
			DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &randomizer, &rlen,
			DBUS_TYPE_INVALID))
		return btd_error_invalid_args(msg);

	if (hlen != 16 || rlen != 16 || bachk(addr))
		return btd_error_invalid_args(msg);

	str2ba(addr, &bdaddr);

	if (btd_adapter_add_remote_oob_data(adapter, &bdaddr, hash, randomizer))
		return btd_error_failed(msg, "Request failed");

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *remove_remote_data(DBusConnection *conn, DBusMessage *msg,
								void *data)
{
	struct btd_adapter *adapter = data;
	const char *addr;
	bdaddr_t bdaddr;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &addr,
			DBUS_TYPE_INVALID))
		return btd_error_invalid_args(msg);

	if (bachk(addr))
		return btd_error_invalid_args(msg);

	str2ba(addr, &bdaddr);

	if (btd_adapter_remove_remote_oob_data(adapter, &bdaddr))
		return btd_error_failed(msg, "Request failed");

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static const GDBusMethodTable oob_methods[] = {
	{ GDBUS_METHOD("AddRemoteData",
			GDBUS_ARGS({ "address", "s" }, { "hash", "ay" },
					{ "randomizer", "ay" }), NULL,
			add_remote_data) },
	{ GDBUS_METHOD("RemoveRemoteData",
			GDBUS_ARGS({ "address", "s" }), NULL,
			remove_remote_data) },
	{ GDBUS_ASYNC_METHOD("ReadLocalData",
			NULL, GDBUS_ARGS({ "hash", "ay" },
						{ "randomizer", "ay" }),
			read_local_data) },
	{ }
};

static int oob_probe(struct btd_adapter *adapter)
{
	const char *path = adapter_get_path(adapter);

	DBG("dbusoob probe");
	DBG("adapter path: %s", path);

	if (!g_dbus_register_interface(connection, path, OOB_INTERFACE,
				oob_methods, NULL, NULL, adapter, NULL)) {
			error("OOB interface init failed on path %s", path);
			return -EIO;
		}

	return 0;
}

static void oob_remove(struct btd_adapter *adapter)
{
	read_local_data_complete(adapter, NULL, NULL);

	g_dbus_unregister_interface(connection, adapter_get_path(adapter),
							OOB_INTERFACE);
}

static struct btd_adapter_driver oob_driver = {
	.name	= "oob",
	.probe	= oob_probe,
	.remove	= oob_remove,
};

static int dbusoob_init(void)
{
	DBG("Setup dbusoob plugin");

	connection = btd_get_dbus_connection();

	oob_register_cb(read_local_data_complete);

	return btd_register_adapter_driver(&oob_driver);
}

static void dbusoob_exit(void)
{
	DBG("Cleanup dbusoob plugin");

	btd_unregister_adapter_driver(&oob_driver);
}

BLUETOOTH_PLUGIN_DEFINE(dbusoob, VERSION, BLUETOOTH_PLUGIN_PRIORITY_DEFAULT,
						dbusoob_init, dbusoob_exit)
