/*
 *
 *  OBEX Client
 *
 *  Copyright (C) 2012 Samsung Electronics Co., Ltd.
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
#include <glib.h>
#include <gdbus.h>
#include <string.h>

#include "log.h"

#include "transfer.h"
#include "session.h"
#include "driver.h"
#include "map_ap.h"
#include "mns-tizen.h"

#define OBEX_MNS_UUID \
	"\xBB\x58\x2B\x41\x42\x0C\x11\xDB\xB0\xDE\x08\x00\x20\x0C\x9A\x66"
#define OBEX_MNS_UUID_LEN 16

#define MNS_INTERFACE  "org.openobex.MessageNotification"
#define ERROR_INF MNS_INTERFACE ".Error"
#define MNS_UUID "00001133-0000-1000-8000-00805f9b34fb"

enum msg_event_type {
	EVENT_TYPE_NEW_MESSAGE,
	EVENT_TYPE_DELIVERY_SUCCESS,
	EVENT_TYPE_SENDING_SUCCESS,
	EVENT_TYPE_DELIVERY_FAILURE,
	EVENT_TYPE_SENDING_FAILURE,
	EVENT_TYPE_MEMORY_FULL,
	EVENT_TYPE_MEMORY_AVAILABLE,
	EVENT_TYPE_MESSAGE_DELETED,
	EVENT_TYPE_MESSAGE_SHIFT,
	EVENT_TYPE_UNKNOWN,
};

struct sendevent_apparam {
	uint8_t     masinstanceid_tag;
	uint8_t     masinstanceid_len;
	uint8_t     masinstanceid;
} __attribute__ ((packed));

struct mns_data {
	struct obc_session *session;
	DBusMessage *msg;
};

static DBusConnection *conn = NULL;

static int get_event_type(gchar *event_type)
{
	DBG("event_type = %s\n", event_type);

	if (!g_strcmp0(event_type, "NewMessage"))
		return EVENT_TYPE_NEW_MESSAGE;
	else if (!g_strcmp0(event_type, "DeliverySuccess"))
		return EVENT_TYPE_DELIVERY_SUCCESS;
	else if (!g_strcmp0(event_type, "SendingSuccess"))
		return EVENT_TYPE_SENDING_SUCCESS;
	else if (!g_strcmp0(event_type, "DeliveryFailure"))
		return EVENT_TYPE_DELIVERY_FAILURE;
	else if (!g_strcmp0(event_type, "SendingFailure"))
		return EVENT_TYPE_SENDING_FAILURE;
	else if (!g_strcmp0(event_type, "MemoryFull"))
		return EVENT_TYPE_MEMORY_FULL;
	else if (!g_strcmp0(event_type, "MemoryAvailable"))
		return EVENT_TYPE_MEMORY_AVAILABLE;
	else if (!g_strcmp0(event_type, "MessageDeleted"))
		return EVENT_TYPE_MESSAGE_DELETED;
	else if (!g_strcmp0(event_type, "MessageShift"))
		return EVENT_TYPE_MESSAGE_SHIFT;
	else
		return EVENT_TYPE_UNKNOWN;

}

static gchar *generate_event_report(gchar *event_type,
				guint64 handle, gchar *folder,
				gchar *old_folder, gchar *msg_type)
{
	GString *buf;
	int event;

	event = get_event_type(event_type);
	if (event == EVENT_TYPE_UNKNOWN)
		return NULL;

	buf = g_string_new("<MAP-event-report version=\"1.0\">");
	g_string_append_printf(buf, "<event type=\"%s\" ", event_type);

	if (event == EVENT_TYPE_MEMORY_FULL ||
			event == EVENT_TYPE_MEMORY_AVAILABLE)
		goto done;

	g_string_append_printf(buf, "handle=\"%llx\" ", handle);
	g_string_append_printf(buf, "folder=\"%s\" ", folder);

	if (event == EVENT_TYPE_MESSAGE_SHIFT)
		g_string_append_printf(buf, " old_folder=\"%s\" ", old_folder);

	g_string_append_printf(buf, "msg_type=\"%s\" ", msg_type);

done:
	g_string_append(buf, "/></MAP-event-report>");

	return g_string_free(buf, FALSE);
}

static DBusMessage *send_event(DBusConnection *connection,
					DBusMessage *message, void *user_data)
{
	struct mns_data *mns = user_data;
	struct obc_transfer *transfer;
	struct sendevent_apparam apparam;
	gchar *event_type;
	gchar *folder;
	gchar *old_folder;
	gchar *msg_type;
	gchar *buf;
	guint64 handle;
	GError *err;
	DBusMessage *reply;

	if (dbus_message_get_args(message, NULL,
			DBUS_TYPE_STRING, &event_type,
			DBUS_TYPE_UINT64, &handle,
			DBUS_TYPE_STRING, &folder,
			DBUS_TYPE_STRING, &old_folder,
			DBUS_TYPE_STRING, &msg_type,
			DBUS_TYPE_INVALID) == FALSE)
		return g_dbus_create_error(message,
					"org.openobex.Error.InvalidArguments",
					NULL);

	buf = generate_event_report(event_type, handle, folder,
				old_folder, msg_type);
	if (!buf)
		return g_dbus_create_error(message,
				"org.openobex.Error.InvalidArguments", NULL);

	transfer = obc_transfer_put("x-bt/MAP-event-report", NULL, NULL,
				buf, strlen(buf), &err);

	g_free(buf);

	if (transfer == NULL)
		goto fail;

	apparam.masinstanceid_tag = MAP_AP_MASINSTANCEID;
	apparam.masinstanceid_len = 1;
	/* Obexd currently supports single SDP for MAS */
	apparam.masinstanceid = 0;

	obc_transfer_set_apparam(transfer, &apparam);

	if (obc_session_queue(mns->session, transfer, NULL, NULL, &err))
		return dbus_message_new_method_return(message);

fail:
	reply = g_dbus_create_error(message, ERROR_INF ".Failed", "%s",
								err->message);
	g_error_free(err);
	return reply;
}

static GDBusMethodTable mns_methods[] = {
        { GDBUS_ASYNC_METHOD("SendEvent",
		GDBUS_ARGS({ "event_type", "s" }, { "handle", "t" },
				{ "folder", "s" }, { "old_folder", "s" },
				{ "msg_type", "s" }),
		NULL,
		send_event) },
	{ }
};

static void mns_free(void *data)
{
	struct mns_data *mns = data;

	obc_session_unref(mns->session);
	g_free(mns);
}

static int mns_probe(struct obc_session *session)
{
	struct mns_data *mns;
	const char *path;

	path = obc_session_get_path(session);

	DBG("%s", path);

	mns = g_try_new0(struct mns_data, 1);
	if (!mns)
		return -ENOMEM;

	mns->session = obc_session_ref(session);

	if (!g_dbus_register_interface(conn, path, MNS_INTERFACE, mns_methods,
					NULL, NULL, mns, mns_free)) {
		mns_free(mns);
		return -ENOMEM;
	}

	return 0;
}

static void mns_remove(struct obc_session *session)
{
	const char *path = obc_session_get_path(session);

	DBG("%s", path);

	g_dbus_unregister_interface(conn, path, MNS_INTERFACE);
}

static struct obc_driver mns = {
	.service = "MNS",
	.uuid = MNS_UUID,
	.target = OBEX_MNS_UUID,
	.target_len = OBEX_MNS_UUID_LEN,
	.probe = mns_probe,
	.remove = mns_remove
};

int mns_init(void)
{
	int err;

	DBG("");

	conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	if (!conn)
		return -EIO;

	err = obc_driver_register(&mns);
	if (err < 0) {
		dbus_connection_unref(conn);
		conn = NULL;
		return err;
	}

	return 0;
}

void mns_exit(void)
{
	DBG("");

	dbus_connection_unref(conn);
	conn = NULL;

	obc_driver_unregister(&mns);
}
