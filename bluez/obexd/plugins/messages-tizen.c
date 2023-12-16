/*
 *
 *  OBEX Server
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
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "messages.h"

#include <dbus/dbus.h>

#define QUERY_GET_FOLDER_TREE "GetFolderTree"
#define QUERY_GET_MSG_LIST "GetMessageList"
#define QUERY_GET_MESSAGE "GetMessage"
#define QUERY_PUSH_MESSAGE "PushMessage"
#define QUERY_PUSH_MESSAGE_DATA "PushMessageData"
#define QUERY_UPDATE_MESSAGE "UpdateMessage"
#define QUERY_SET_READ_STATUS "SetReadStatus"
#define QUERY_SET_DELETE_STATUS "SetDeleteStatus"
#define QUERY_NOTI_REGISTRATION "NotiRegistration"
#define QUERY_DESTROY_AGENT "DestroyAgent"

#define BT_MAP_SERVICE_OBJECT_PATH "/org/bluez/map_agent"
#define BT_MAP_SERVICE_NAME "org.bluez.map_agent"
#define BT_MAP_SERVICE_INTERFACE "org.bluez.MapAgent"

/* Added as per MAP specification */
#define BT_MAP_LIST_ITEM_MAX_LEN 256

static DBusConnection *g_conn = NULL;

struct mns_reg_data {
	uint8_t notification_status;
	char *remote_addr;
};

struct message_folder {
	char *name;
	GSList *subfolders;
};

struct session {
	char *cwd;
	struct message_folder *folder;
	char *name;
	uint16_t max;
	uint16_t offset;
	void *user_data;
	struct messages_filter *filter;
	struct messages_message *msg;
	void (*folder_list_cb)(void *session, int err, uint16_t size,
					const char *name, void *user_data);
	void (*msg_list_cb)(void *session, int err, int size, gboolean newmsg,
					const struct messages_message *entry,
					void *user_data);
	void (*push_msg_cb)(void *session, int err, guint64 handle,
				void *user_data);
	void (*get_msg_cb)(void *session, int err, gboolean fmore,
				const char *chunk, void *user_data);
	void (*msg_update_cb)(void *session, int err, void *user_data);
	void (*msg_status_cb)(void *session, int err, void *user_data);
};

static struct message_folder *folder_tree = NULL;

static void message_list_item_free(struct messages_message *data)
{
	DBG("+");
	g_free(data->handle);
	data->handle = NULL;

	g_free(data->subject);
	data->subject = NULL;

	g_free(data->datetime);
	data->datetime = NULL;

	g_free(data->sender_name);
	data->sender_name = NULL;

	g_free(data->sender_addressing);
	data->sender_addressing = NULL;

	g_free(data->replyto_addressing);
	data->replyto_addressing = NULL;

	g_free(data->recipient_name);
	data->recipient_name = NULL;

	g_free(data->recipient_addressing);
	data->recipient_addressing = NULL;

	g_free(data->type);
	data->type = NULL;

	g_free(data->reception_status);
	data->reception_status = NULL;

	g_free(data->size);
	data->size = NULL;

	g_free(data->attachment_size);
	data->attachment_size = NULL;
	DBG("-");
}

static void session_filter_free(struct messages_filter *data)
{
	DBG("+");
	if (NULL == data)
		return;

	g_free((gpointer)data->period_begin);
	g_free((gpointer)data->period_end);
	g_free((gpointer)data->recipient);
	g_free((gpointer)data->originator);
	g_free(data);
	DBG("-");
}

static gboolean is_time_in_period(char *ref_time, char *period)
{
	guint64 ref_date_val;
	guint64 date_val;

	guint64 ref_time_val;
	guint64 time_val;

	char *start;
	char *end = NULL;

	char temp[20];

	start = strtok_r(ref_time, "T", &end);
	if (NULL == start || NULL == end)
		return FALSE;

	snprintf(temp, sizeof(temp), "%s", start);
	ref_date_val = g_ascii_strtoull(temp, NULL, 16);
	snprintf(temp, sizeof(temp), "%s", end);
	ref_time_val = g_ascii_strtoull(temp, NULL, 16);

	start = strtok_r(period, "T", &end);
	if (NULL == start || NULL == end)
		return FALSE;

	snprintf(temp, sizeof(temp), "%s", start);
	date_val = g_ascii_strtoull(temp, NULL, 16);
	snprintf(temp, sizeof(temp), "%s", end);
	time_val = g_ascii_strtoull(temp, NULL, 16);

	if (ref_date_val < date_val) {
		return TRUE;
	} else if (ref_date_val > date_val) {
		return FALSE;
	} else {
		if (ref_time_val <= time_val)
			return TRUE;
		else
			return FALSE;
	}
}

static gboolean __filter_timebased(char *begin, char *end, char *time)
{
	gboolean ret = 0;

	if (!begin && !end) {
		/* If start and stop are not specified
		do not filter. */

		return TRUE;
	} else if (!end && begin) {
		/* If "FilterPeriodEnd" is not specified the returned
		message listing shall include the messages from
		"FilterPeriodBegin" to current time */

		return is_time_in_period(begin, time);
	} else if (!begin && end) {
		/* If "FilterPeriodBegin" is not specified the returned
		message listing shall include the messages older than
		"FilterPeriodEnd" */

		return is_time_in_period(time, end);
	} else {

		if (TRUE == is_time_in_period(end, begin))
			return FALSE;

		ret = is_time_in_period(begin, time);
		if (ret == TRUE)
			return is_time_in_period(time, end);
		else
			return FALSE;
	}
}

static uint8_t get_type_val(const char *type)
{
	if (!g_strcmp0(type, "SMS_GSM"))
		return 0x01;
	else if (!g_strcmp0(type, "SMS_CDMA"))
		return 0x02;
	else if (!g_strcmp0(type, "EMAIL"))
		return 0x04;
	else if (!g_strcmp0(type, "MMS"))
		return 0x08;
	else
		return 0x00;
}

static uint8_t get_read_status_val(gboolean read)
{
	if (read)
		return 0x02; /* Read messages */
	else
		return 0x01; /* Unread messages */
}

static uint8_t get_priority_val(gboolean priority)
{
	if (priority)
		return 0x01; /* High priority */
	else
		return 0x02; /* Low priority */
}

static struct message_folder *get_folder(const char *folder)
{
	GSList *folders = folder_tree->subfolders;
	struct message_folder *last = NULL;
	char **path;
	int i;

	if (g_strcmp0(folder, "/") == 0)
		return folder_tree;

	path = g_strsplit(folder, "/", 0);

	for (i = 1; path[i] != NULL; i++) {
		gboolean match_found = FALSE;
		GSList *l;

		for (l = folders; l != NULL; l = g_slist_next(l)) {
			struct message_folder *folder = l->data;

			if (g_ascii_strncasecmp(folder->name, path[i],
					strlen(folder->name)) == 0) {
				match_found = TRUE;
				last = l->data;
				folders = folder->subfolders;
				break;
			}
		}

		if (!match_found) {
			g_strfreev(path);
			return NULL;
		}
	}

	g_strfreev(path);

	return last;
}

static void destroy_folder_tree(void *root)
{
	struct message_folder *folder = root;
	GSList *tmp, *next;

	if (folder == NULL)
		return;

	g_free(folder->name);

	tmp = folder->subfolders;
	while (tmp != NULL) {
		next = g_slist_next(tmp);
		destroy_folder_tree(tmp->data);
		tmp = next;
	}
	g_slist_free(folder->subfolders);
	g_free(folder);
}

static struct message_folder *create_folder(const char *name)
{
	struct message_folder *folder = g_new0(struct message_folder, 1);

	folder->name = g_strdup(name);
	return folder;
}

static void create_folder_tree()
{

	struct message_folder *parent, *child;

	folder_tree = create_folder("/");

	parent = create_folder("telecom");
	folder_tree->subfolders = g_slist_append(folder_tree->subfolders,
								parent);

	child = create_folder("msg");
	parent->subfolders = g_slist_append(parent->subfolders, child);
}

int messages_init(void)
{
	g_conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	if (!g_conn) {
		error("Can't get on session bus");
		return -1;
	}

	return 0;
}

void messages_exit(void)
{
	if (g_conn) {
		dbus_connection_unref(g_conn);
		g_conn = NULL;
	}
}

static void message_get_folder_list(DBusMessage *reply, void *user_data)
{
	DBusMessageIter iter;
	DBusMessageIter iter_struct;
	DBusMessageIter entry;
	DBusError derr;
	const char *name = NULL;
	struct message_folder *parent = {0,}, *child = {0,};
	GSList *l;

	DBG("+\n");

	for (l = folder_tree->subfolders; l != NULL; l = parent->subfolders)
		parent = l->data;

	DBG("Last child folder = %s \n", parent->name);
	dbus_error_init(&derr);

	if (dbus_set_error_from_message(&derr, reply)) {
		error("Replied with an error: %s, %s", derr.name, derr.message);
		dbus_error_free(&derr);
	} else {
		dbus_message_iter_init(reply, &iter);
		dbus_message_iter_recurse(&iter, &iter_struct);

		while (dbus_message_iter_get_arg_type(&iter_struct) ==
							DBUS_TYPE_STRUCT) {
			dbus_message_iter_recurse(&iter_struct, &entry);

			dbus_message_iter_get_basic(&entry, &name);
			DBG("Folder name = %s \n", name);
			child = create_folder(name);
			parent->subfolders = g_slist_append(parent->subfolders,
							child);
			dbus_message_iter_next(&iter_struct);
		}
	}
	dbus_message_unref(reply);
	DBG("-\n");
}

static void message_get_msg_list(DBusPendingCall *call, void *user_data)
{
	DBusMessage *reply = dbus_pending_call_steal_reply(call);
	DBusMessageIter iter;
	DBusMessageIter iter_struct;
	DBusMessageIter entry;
	DBusError derr;
	const char *msg_handle;
	const char *subject;
	const char *datetime;
	const char *sender_name;
	const char *sender_addressing;
	const char *replyto_addressing;
	const char *recipient_name;
	const char *recipient_addressing;
	const char *type;
	const char *reception_status;
	const char *size;
	const char *attachment_size;
	gboolean text;
	gboolean read;
	gboolean sent;
	gboolean protect;
	gboolean priority;
	gboolean newmessage;
	guint64 count;
	uint8_t type_val;
	uint8_t read_val;
	uint8_t priority_val;
	uint32_t mask;

	struct session *session = user_data;
	struct messages_message *data = g_new0(struct messages_message, 1);

	DBG("+\n");
	DBG("parameter_mask = %x; type = %d; period_begin = %s;"
		"period_end = %s; read_status = %d; recipient = %s;"
		"originator = %s; priority = %d",
		session->filter->parameter_mask, session->filter->type,
		session->filter->period_begin, session->filter->period_end,
		session->filter->read_status, session->filter->recipient,
		session->filter->originator, session->filter->priority);

	dbus_error_init(&derr);

	if (dbus_set_error_from_message(&derr, reply)) {
		error("Replied with an error: %s, %s", derr.name, derr.message);
		dbus_error_free(&derr);
		session->msg_list_cb(session, -ENOENT, 0,
				FALSE, data, session->user_data);

		g_free(data);
		g_free(session->name);
		session_filter_free(session->filter);
		dbus_message_unref(reply);

		return;
	}

	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_get_basic(&iter, &newmessage);
	dbus_message_iter_next(&iter);
	dbus_message_iter_get_basic(&iter, &count);
	dbus_message_iter_next(&iter);

	if (session->max == 0)
		goto done;

	dbus_message_iter_recurse(&iter, &iter_struct);

	if (session->filter->parameter_mask == 0)
		mask = ~session->filter->parameter_mask;
	else
		mask = session->filter->parameter_mask;

	while (dbus_message_iter_get_arg_type(&iter_struct) ==
						DBUS_TYPE_STRUCT) {
		dbus_message_iter_recurse(&iter_struct, &entry);
		dbus_message_iter_get_basic(&entry, &msg_handle);

		if (msg_handle == NULL) {
			dbus_message_iter_next(&iter_struct);
			continue;
		}

		DBG("Msg handle = %s \n", msg_handle);
		data->handle = g_strdup(msg_handle);

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &subject);

		if (mask & PMASK_SUBJECT) {
			DBG("subject = %s\n", subject);
			data->subject = g_strndup(subject,
						BT_MAP_LIST_ITEM_MAX_LEN);
			data->mask |= PMASK_SUBJECT;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &datetime);

		if ((mask & PMASK_DATETIME) && (NULL != datetime)) {
			DBG("datetime = %s\n", datetime);
			char *begin = g_strdup(session->filter->period_begin);
			char *end = g_strdup(session->filter->period_end);
			char *time = g_strdup(datetime);
			gboolean filter;

			filter = __filter_timebased(begin, end, time);

			g_free(begin);
			g_free(end);
			g_free(time);

			if (TRUE == filter) {
				data->datetime = g_strdup(datetime);
				data->mask |= PMASK_DATETIME;
			} else {
				message_list_item_free(data);
				dbus_message_iter_next(&iter_struct);
				continue;
			}
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &sender_name);

		if ((mask & PMASK_SENDER_NAME) &&
				(NULL != session->filter->originator)) {
			DBG("sender_name = %s \n", sender_name);

			if (g_strstr_len(sender_name, -1,
					session->filter->originator)) {
				data->sender_name = g_strndup(sender_name,
						BT_MAP_LIST_ITEM_MAX_LEN);
				data->mask |= PMASK_SENDER_NAME;
			} else {
				message_list_item_free(data);
				dbus_message_iter_next(&iter_struct);
				continue;
			}
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &sender_addressing);

		if ((mask & PMASK_SENDER_ADDRESSING) &&
						(NULL != sender_addressing)) {
			DBG("sender_addressing = %s \n", sender_addressing);

			data->sender_addressing = g_strndup(sender_addressing,
						BT_MAP_LIST_ITEM_MAX_LEN);
				data->mask |= PMASK_SENDER_ADDRESSING;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &recipient_name);

		if ((mask & PMASK_RECIPIENT_NAME) &&
				(NULL != session->filter->recipient)) {
			DBG("recipient_name = %s \n", recipient_name);

			if (g_strstr_len(recipient_name, -1,
					session->filter->recipient)) {
				data->recipient_name =
					g_strndup(recipient_name,
					BT_MAP_LIST_ITEM_MAX_LEN);
				data->mask |= PMASK_RECIPIENT_NAME;
			} else {
				message_list_item_free(data);
				dbus_message_iter_next(&iter_struct);
				continue;
			}
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &recipient_addressing);

		if ((mask & PMASK_RECIPIENT_ADDRESSING) &&
				(NULL != recipient_addressing)) {
			DBG("recipient_addressing=%s\n", recipient_addressing);

			data->recipient_addressing =
					g_strndup(recipient_addressing,
					BT_MAP_LIST_ITEM_MAX_LEN);
			data->mask |= PMASK_RECIPIENT_ADDRESSING;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &type);

		if ((mask & PMASK_TYPE) && (NULL != type)) {
			DBG("type = %s \n", type);

			type_val = get_type_val(type);
			if (!(session->filter->type & type_val)) {
				data->type = g_strdup(type);
				data->mask |= PMASK_TYPE;
			}
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &size);

		if ((mask & PMASK_SIZE) && (NULL != size)) {
			DBG("size = %s \n", size);

			data->size = g_strdup(size);
			data->mask |= PMASK_SIZE;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &reception_status);

		if (mask & PMASK_RECEPTION_STATUS) {
			DBG("reception_status = %s \n", reception_status);

			data->reception_status = g_strdup(reception_status);
			data->mask |= PMASK_RECEPTION_STATUS;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &text);

		if (mask & PMASK_TEXT) {
			DBG("text = %d \n", text);
			data->text = text;
			data->mask |= PMASK_TEXT;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &attachment_size);

		if (mask & PMASK_ATTACHMENT_SIZE) {
			DBG("attachment_size = %s\n", attachment_size);

			data->attachment_size = g_strdup(attachment_size);
			data->mask |= PMASK_ATTACHMENT_SIZE;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &priority);

		if (mask & PMASK_PRIORITY) {
			DBG("priority = %d \n", priority);

			priority_val = get_priority_val(priority);
			if ((session->filter->priority == 0) ||
				(session->filter->priority & priority_val)) {
				data->priority = priority;
				data->mask |= PMASK_PRIORITY;
			} else {
				message_list_item_free(data);
				dbus_message_iter_next(&iter_struct);
				continue;
			}
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &read);

		if (mask & PMASK_READ) {
			DBG("read = %d \n", read);

			read_val = get_read_status_val(read);

			if ((session->filter->read_status == 0) ||
				(session->filter->read_status & read_val)) {
				data->read = read;
				data->mask |= PMASK_READ;
			} else {
				message_list_item_free(data);
				dbus_message_iter_next(&iter_struct);
				continue;
			}
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &sent);

		if (mask & PMASK_SENT) {
			DBG("sent = %d \n", sent);
			data->sent = sent;
			data->mask |= PMASK_SENT;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &protect);

		if (mask & PMASK_PROTECTED) {
			DBG("protect = %d \n", protect);
			data->protect = protect;
			data->mask |= PMASK_PROTECTED;
		}

		dbus_message_iter_next(&entry);
		dbus_message_iter_get_basic(&entry, &replyto_addressing);

		if ((mask & PMASK_REPLYTO_ADDRESSING) &&
						(0x04 == get_type_val(type))) {

			DBG("replyto_addressing = %s \n", replyto_addressing);
			if (replyto_addressing)
				data->replyto_addressing =
						g_strdup(replyto_addressing);
			else
				data->replyto_addressing = g_strdup("");

			data->mask |= PMASK_REPLYTO_ADDRESSING;
		}

		session->msg_list_cb(session, -EAGAIN, 1, newmessage, data,
							session->user_data);

		message_list_item_free(data);
		dbus_message_iter_next(&iter_struct);
	}

done:
	session->msg_list_cb(session, 0, count, newmessage, NULL,
							session->user_data);

	g_free(data);
	g_free(session->name);
	session_filter_free(session->filter);
	dbus_message_unref(reply);
	DBG("-\n");
}

static void message_get_msg(DBusPendingCall *call, void *user_data)
{
	DBusMessage *reply = dbus_pending_call_steal_reply(call);
	DBusMessageIter iter;
	DBusError derr;
	struct session *session = user_data;
	char *msg_body;
	gboolean fraction_deliver;

	DBG("+\n");

	dbus_error_init(&derr);
	if (dbus_set_error_from_message(&derr, reply)) {
		error("Replied with an error: %s, %s", derr.name, derr.message);
		dbus_error_free(&derr);
	} else {
		dbus_message_iter_init(reply, &iter);
		dbus_message_iter_get_basic(&iter, &fraction_deliver);
		dbus_message_iter_next(&iter);
		dbus_message_iter_get_basic(&iter, &msg_body);
		DBG("msg_body %s\n", msg_body);

		session->get_msg_cb(session, -EAGAIN, fraction_deliver,
					msg_body, session->user_data);
		session->get_msg_cb(session, 0, fraction_deliver,
					NULL, session->user_data);
	}
	dbus_message_unref(reply);
	DBG("-\n");
}

int messages_connect(void **s)
{
	DBusMessage *message;
	DBusMessage *reply;
	DBusError err;
	DBG("+\n");

	struct session *session = g_new0(struct session, 1);

	create_folder_tree();

	session->cwd = g_strdup("/");
	session->folder = folder_tree;

	*s = session;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_GET_FOLDER_TREE);
	if (!message) {
		error("Can't allocate new message");
		return -1;
	}

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(g_conn, message,
						DBUS_TIMEOUT_USE_DEFAULT, &err);
	if (!reply) {
		DBG(" Reply failed");
		if (dbus_error_is_set(&err)) {
			DBG("%s", err.message);
			dbus_error_free(&err);
		}

		dbus_message_unref(message);
		return -1;
	}

	message_get_folder_list(reply, session);

	dbus_message_unref(message);
	DBG("-\n");
	return 0;
}

void messages_disconnect(void *s)
{
	DBusMessage *message;
	struct session *session = s;
	DBG("+\n");

	destroy_folder_tree(folder_tree);
	folder_tree = NULL;
	g_free(session->cwd);
	g_free(session);

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_DESTROY_AGENT);

	if (!message) {
		error("Can't allocate new message");
		return;
	}

	if (dbus_connection_send(g_conn, message, NULL) == FALSE)
		error("Could not send dbus message");

	dbus_message_unref(message);

	DBG("-\n");
}

static gboolean notification_registration(gpointer user_data)
{
	DBG("+\n");
	DBusMessage *message = NULL;
	gboolean reg;
	struct mns_reg_data *data = (struct mns_reg_data *)user_data;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
					BT_MAP_SERVICE_OBJECT_PATH,
					BT_MAP_SERVICE_INTERFACE,
					QUERY_NOTI_REGISTRATION);
	if (!message) {
		error("Can't allocate new message");
		goto done;
	}

	DBG("data->notification_status = %d\n", data->notification_status);

	if (data->notification_status == 1)
		reg = TRUE;
	else
		reg = FALSE;

	dbus_message_append_args(message, DBUS_TYPE_STRING, &data->remote_addr,
						DBUS_TYPE_BOOLEAN, &reg,
						DBUS_TYPE_INVALID);

	if (dbus_connection_send(g_conn, message, NULL) == FALSE)
		error("Could not send dbus message");

	dbus_message_unref(message);

done:
	g_free(data->remote_addr);
	g_free(data);

	DBG("-\n");
	return FALSE;
}

int messages_set_notification_registration(void *session,
				char *address, uint8_t status,
				void *user_data)
{
	DBG("+\n");
	struct mns_reg_data *data = g_new0(struct mns_reg_data, 1);
	data->notification_status = status;
	data->remote_addr = g_strdup(address);

	DBG("status = %d\n", status);

	g_idle_add(notification_registration, data);
	DBG("-\n");
	return 1;
}

int messages_set_folder(void *s, const char *name, gboolean cdup)
{
	struct session *session = s;
	char *newrel = NULL;
	char *newabs;
	char *tmp;

	if (name && (strchr(name, '/') || strcmp(name, "..") == 0))
		return -EBADR;

	if (cdup) {
		if (session->cwd[0] == 0)
			return -ENOENT;

		newrel = g_path_get_dirname(session->cwd);

		/* We use empty string for indication of the root directory */
		if (newrel[0] == '.' && newrel[1] == 0)
			newrel[0] = 0;
	}

	tmp = newrel;
	if (!cdup && (!name || name[0] == 0))
		newrel = g_strdup("");
	else
		newrel = g_build_filename(newrel ? newrel : session->cwd, name,
									NULL);
	g_free(tmp);

	if (newrel[0] != '/')
		newabs = g_build_filename("/", newrel, NULL);
	else
		newabs = g_strdup(newrel);

	session->folder = get_folder(newabs);
	if (session->folder == NULL) {
		g_free(newrel);
		g_free(newabs);

		return -ENOENT;
	}

	g_free(newrel);
	g_free(session->cwd);
	session->cwd = newabs;

	return 0;
}

static gboolean async_get_folder_listing(void *s)
{
	struct session *session = s;
	int i;
	uint16_t folder_list_size = 0;
	char *path = NULL;
	struct message_folder *folder;
	GSList *dir;

	if (session->name && strchr(session->name, '/') != NULL)
		goto done;

	path = g_build_filename(session->cwd, session->name, NULL);

	if (path == NULL || strlen(path) == 0)
		goto done;

	folder = get_folder(path);

	if (folder == NULL)
		goto done;

	if (session->max == 0) {
		folder_list_size = g_slist_length(folder->subfolders);
		goto done;
	}

	dir = folder->subfolders;

	/* move to offset */
	for (i = 0; i < session->offset; i++) {
		if (dir == NULL)
			goto done;

		dir = g_slist_next(dir);
	}

	for (i = 0; i < session->max; i++) {
		struct message_folder *dir_data;

		if (dir == NULL)
			goto done;

		dir_data = dir->data;
		session->folder_list_cb(session, -EAGAIN, 0,
				dir_data->name, session->user_data);

		dir = g_slist_next(dir);
	}

 done:
	session->folder_list_cb(session, 0, folder_list_size,
			NULL, session->user_data);

	g_free(path);
	g_free(session->name);

	return FALSE;
}

int messages_get_folder_listing(void *s, const char *name,
					uint16_t max, uint16_t offset,
					messages_folder_listing_cb callback,
					void *user_data)
{
	DBG("+\n");
	struct session *session = s;
	session->name = g_strdup(name);
	session->max = max;
	session->offset = offset;
	session->folder_list_cb = callback;
	session->user_data = user_data;

	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, async_get_folder_listing,
						session, NULL);

	DBG("-\n");
	return 0;
}

int messages_get_messages_listing(void *session, const char *name,
				uint16_t max, uint16_t offset,
				uint8_t subject_len,
				const struct messages_filter *filter,
				messages_get_messages_listing_cb callback,
				void *user_data)
{
	DBusPendingCall *call;
	DBusMessage *message;
	struct session *s = session;

	if (name != NULL && strlen(name))
		s->name = g_strdup(name);
	else
		s->name = g_strdup(s->cwd);

	s->max = max;
	s->offset = offset;

	s->filter = g_new0(struct messages_filter, 1);
	s->filter->parameter_mask = filter->parameter_mask;
	s->filter->type = filter->type;
	s->filter->period_begin = g_strdup(filter->period_begin);
	s->filter->period_end = g_strdup(filter->period_end);
	s->filter->read_status = filter->read_status;
	s->filter->recipient = g_strdup(filter->recipient);
	s->filter->originator = g_strdup(filter->originator);
	s->filter->priority = filter->priority;

	s->msg_list_cb = (void *)callback;
	s->user_data = user_data;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_GET_MSG_LIST);
	if (!message) {
		error("Can't allocate new message");
		g_free(s->name);
		session_filter_free(s->filter);
		return -1;
	}

	dbus_message_append_args(message, DBUS_TYPE_STRING, &s->name,
						DBUS_TYPE_UINT16, &s->max,
						DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(g_conn, message, &call,
					DBUS_TIMEOUT_INFINITE) == FALSE) {
		error("Could not send dbus message");
		dbus_message_unref(message);
		g_free(s->name);
		session_filter_free(s->filter);
		return -1;
	}
	dbus_pending_call_set_notify(call, message_get_msg_list, s, NULL);
	dbus_message_unref(message);
	DBG("-\n");
	return 1;
}

int messages_push_message(void *session, const char *folder,
					uint8_t transparent, uint8_t retry,
					uint8_t charset,
					messages_push_message_cb callback,
					void *user_data)
{
	DBusMessage *message;
	DBusMessage *reply;
	DBusError err;
	struct session *s = session;

	gboolean save_copy = FALSE;  /* As per specs default value */
	gboolean retry_send = TRUE; /* As per specs default value */
	gboolean native = FALSE;
	gchar *folder_path = NULL;
	guint64 handle = 0;

	DBG("+\n");

	DBG("session->cwd %s +\n", s->cwd);

	if (folder)
		folder_path = g_strdup(folder);
	else
		folder_path = g_strdup(s->cwd);

	s->push_msg_cb = callback;
	s->user_data = user_data;

	if (transparent & 0x1)
		save_copy = TRUE;

	if (!(retry & 0x1)) {
		retry_send = FALSE;
		DBG("Retry send %d\n", retry_send);
	}

	if (charset & 0x1) {
		native = TRUE;
		DBG("native send %d\n", native);
	}

	DBG("save_copy  %d\n", save_copy);
	DBG("retry_send %d\n", retry_send);
	DBG("native %d\n", native);

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_PUSH_MESSAGE);
	if (!message) {
		error("Can't allocate new message");
		g_free(folder_path);
		return -1;
	}

	dbus_message_append_args(message, DBUS_TYPE_BOOLEAN, &save_copy,
						DBUS_TYPE_BOOLEAN, &retry_send,
						DBUS_TYPE_BOOLEAN, &native,
						DBUS_TYPE_STRING, &folder_path,
						DBUS_TYPE_INVALID);

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(
					g_conn, message,
					DBUS_TIMEOUT_USE_DEFAULT, &err);
	if (!reply) {
		DBG(" Reply failed");

		if (dbus_error_is_set(&err)) {
			DBG("%s", err.message);
			dbus_error_free(&err);
		}
		g_free(folder_path);
		dbus_message_unref(message);
		return -1;
	}

	if (!dbus_message_get_args(reply, &err, DBUS_TYPE_UINT64,
						&handle, DBUS_TYPE_INVALID)) {
		if (dbus_error_is_set(&err)) {
			error("err %s\n", err.message);
			dbus_error_free(&err);
		}
		g_free(folder_path);
		dbus_message_unref(message);
		dbus_message_unref(reply);
		return -1;
	}

	DBG("uint64 handle %"G_GUINT64_FORMAT"\n", handle);
	s->push_msg_cb(s, 0, handle, s->user_data);

	g_free(folder_path);
	dbus_message_unref(message);
	dbus_message_unref(reply);

	DBG("-\n");
	return 1;
}

int messages_push_message_data(void *session, const char *bmsg, void *user_data)
{
	DBusMessage *message;

	DBG("+\n");

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_PUSH_MESSAGE_DATA);
	if (!message) {
		error("Can't allocate new message");
		return -1;
	}

	dbus_message_append_args(message, DBUS_TYPE_STRING, &bmsg,
							DBUS_TYPE_INVALID);

	if (dbus_connection_send(g_conn, message, NULL) == FALSE) {
		error("Could not send dbus message");
		dbus_message_unref(message);
		return -1;
	}

	dbus_message_unref(message);
	DBG("-\n");
	return 1;
}

int messages_get_message(void *session,
					const char *handle,
					uint8_t attachment, uint8_t charset,
					uint8_t fraction_request,
					messages_get_message_cb callback,
					void *user_data)
{
	DBusPendingCall *call;
	DBusMessage *message;
	struct session *s = session;
	char *message_name;
	gboolean attach = FALSE;
	gboolean transcode = FALSE;
	gboolean first_request = TRUE;

	DBG("+\n");

	if (NULL != handle) {
		message_name =  g_strdup(handle);
		DBG("Message handle = %s\n", handle);
	} else {
		return -1;
	}
	s->get_msg_cb = callback;
	s->user_data = user_data;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_GET_MESSAGE);
	if (!message) {
		error("Can't allocate new message");
		g_free(message_name);
		return -1;
	}

	if (attachment & 0x1)
		attach = TRUE;

	if (charset & 0x1)
		transcode = TRUE;

	if (fraction_request & 0x1)
		first_request = FALSE;

	dbus_message_append_args(message, DBUS_TYPE_STRING, &message_name,
					DBUS_TYPE_BOOLEAN, &attach,
					DBUS_TYPE_BOOLEAN, &transcode,
					DBUS_TYPE_BOOLEAN, &first_request,
					DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(g_conn, message, &call, -1) ==
					FALSE) {
		error("Could not send dbus message");
		dbus_message_unref(message);
		g_free(message_name);
		return -1;
	}
	dbus_pending_call_set_notify(call, message_get_msg, s, NULL);
	dbus_message_unref(message);
	g_free(message_name);
	DBG("-\n");
	return 1;
}

#ifndef SUPPORT_SMS_ONLY
static void message_update_msg(DBusPendingCall *call, void *user_data)
{
	DBusMessage *reply = dbus_pending_call_steal_reply(call);
	DBusMessageIter iter;
	DBusError derr;
	struct session *session = user_data;
	int err;
	DBG("+\n");

	dbus_error_init(&derr);
	if (dbus_set_error_from_message(&derr, reply)) {
		error("Replied with an error: %s, %s", derr.name, derr.message);
		dbus_error_free(&derr);
	} else {
		dbus_message_iter_init(reply, &iter);
		if (dbus_message_iter_get_arg_type(&iter) ==
							DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&iter, &err);
			DBG("Error : %d\n", err);
			session->msg_update_cb(session, err,
						session->user_data);
		}
	}
	dbus_message_unref(reply);
	DBG("-\n");
}
#endif

int messages_update_inbox(void *session,
					messages_status_cb callback,
					void *user_data)
{
#ifdef SUPPORT_SMS_ONLY
	/* MAP.TS.1.0.3 : TP/MMB/BV-16-I
	   Currently support is only for SMS, Since SMS service does not
	   allow the polling of its mailbox, it must return Not implemented */

	return -ENOSYS;
#else
	DBusPendingCall *call;
	DBusMessage *message;
	struct session *s = session;

	DBG("+\n");

	s->msg_update_cb = callback;
	s->user_data = user_data;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_UPDATE_MESSAGE);
	if (!message) {
		error("Can't allocate new message");
		return -1;
	}

	if (dbus_connection_send_with_reply(g_conn, message, &call, -1) ==
					FALSE) {
		error("Could not send dbus message");
		dbus_message_unref(message);
		return -1;
	}
	dbus_pending_call_set_notify(call, message_update_msg, s, NULL);
	dbus_message_unref(message);
	DBG("-\n");
	return 1;
#endif
}

static void message_status_msg(DBusPendingCall *call, void *user_data)
{
	DBusMessage *reply = dbus_pending_call_steal_reply(call);
	DBusMessageIter iter;
	DBusError derr;
	struct session *session = user_data;
	int err;

	DBG("+\n");

	dbus_error_init(&derr);
	if (dbus_set_error_from_message(&derr, reply)) {
		error("Replied with an error: %s, %s", derr.name, derr.message);
		dbus_error_free(&derr);
	} else {
		dbus_message_iter_init(reply, &iter);
		if (dbus_message_iter_get_arg_type(&iter) ==
							DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&iter, &err);
			DBG("Error : %d\n", err);
			session->msg_status_cb(session, err,
						session->user_data);
		}
	}
	dbus_message_unref(reply);
	DBG("-\n");
}

int messages_set_read(void *session, const char *handle, uint8_t value,
		messages_status_cb callback, void *user_data)
{
	DBusPendingCall *call;
	DBusMessage *message;
	struct session *s = session;
	char *message_name;
	gboolean read;

	DBG("+\n");

	if (NULL == handle)
		return -1;

	DBG("Message handle = %s\n", handle);
	message_name = g_strdup(handle);

	s->msg_status_cb = callback;
	s->user_data = user_data;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_SET_READ_STATUS);
	if (!message) {
		error("Can't allocate new message");
		g_free(message_name);
		return -1;
	}

	read = value ? TRUE : FALSE;

	dbus_message_append_args(message, DBUS_TYPE_STRING, &message_name,
						DBUS_TYPE_BOOLEAN, &read,
						DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(g_conn, message, &call, -1) ==
					FALSE) {
		error("Could not send dbus message");
		g_free(message_name);
		dbus_message_unref(message);
		return -1;
	}

	dbus_pending_call_set_notify(call, message_status_msg, s, NULL);
	dbus_message_unref(message);
	g_free(message_name);
	DBG("-\n");
	return 1;
}

int messages_set_delete(void *session, const char *handle,
					uint8_t value,
					messages_status_cb callback,
					void *user_data)
{
	DBusPendingCall *call;
	DBusMessage *message;
	struct session *s = session;
	char *message_name;
	gboolean del;

	DBG("+\n");

	if (NULL == handle)
		return -1;

	DBG("Message handle = %s\n", handle);
	message_name = g_strdup(handle);

	s->msg_status_cb = callback;
	s->user_data = user_data;

	message = dbus_message_new_method_call(BT_MAP_SERVICE_NAME,
						BT_MAP_SERVICE_OBJECT_PATH,
						BT_MAP_SERVICE_INTERFACE,
						QUERY_SET_DELETE_STATUS);
	if (!message) {
		error("Can't allocate new message");
		g_free(message_name);
		return -1;
	}

	del = value ? TRUE : FALSE;

	dbus_message_append_args(message, DBUS_TYPE_STRING, &message_name,
						DBUS_TYPE_BOOLEAN, &del,
						DBUS_TYPE_INVALID);

	if (dbus_connection_send_with_reply(g_conn, message, &call, -1) ==
					FALSE) {
		error("Could not send dbus message");
		g_free(message_name);
		dbus_message_unref(message);
		return -1;
	}

	dbus_pending_call_set_notify(call, message_status_msg, s, NULL);
	dbus_message_unref(message);
	g_free(message_name);
	DBG("-\n");
	return 1;
}

void messages_abort(void *session)
{
}
