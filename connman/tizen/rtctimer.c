/*
 *
 *  Tizen RTC (hardware-based) timer library
 *
 *  Copyright (C) 2012-2013  Danny Jeongseok Seo <S.Seo@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
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
#include <unistd.h>

#include <gdbus.h>

#include <connman/log.h>
#include <connman/dbus.h>

#include "rtctimer.h"

#define TIZEN_RTC_TIMER_SERVICE		"com.samsung.alarm.manager"
#define TIZEN_RTC_TIMER_INTERFACE	"com.samsung.alarm.manager"
#define TIZEN_RTC_TIMER_PATH		"/com/samsung/alarm/manager"

#define TIZEN_RTC_TIMER_CLIENT_INTERFACE	"com.samsung.alarm.client"
#define TIZEN_RTC_TIMER_CLIENT_PATH			"/com/samsung/alarm/client"

static DBusConnection *connection = NULL;
static GHashTable *timer_list = NULL;

struct RTCTimer {
	gpointer data;
	GSourceFunc func;
	GDestroyNotify destroy_func;
};

struct GDestroyNotifyIdle {
	gpointer data;
	GDestroyNotify destroy_func;
};

static int __rtc_timeout_create(struct tm duetime)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	int pid = 0, value = 0;
	int timer = 0, return_code = 0;
	const char *connman_service = CONNMAN_SERVICE;
	const char *cookie = "";

	message = dbus_message_new_method_call(TIZEN_RTC_TIMER_SERVICE,
					TIZEN_RTC_TIMER_PATH, TIZEN_RTC_TIMER_INTERFACE,
					"alarm_create");
	if (message == NULL)
		return -ENOMEM;

	dbus_message_iter_init_append(message, &iter);

	pid = getpid();
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &pid);

	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &connman_service);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &connman_service);

	value = duetime.tm_year + 1900;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	value = duetime.tm_mon + 1;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	value = duetime.tm_mday;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	value = duetime.tm_hour;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	value = duetime.tm_min;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	value = duetime.tm_sec;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	value = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);
	value = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);
	value = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	/* interval 0, only once */
	value = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);
	/* ALARM_REPEAT_MODE_ONCE, only once */
	value = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	/* ALARM_TYPE_VOLATILE 0x02 | ALARM_TYPE_RELATIVE 0x80000000*/
	value = 0x02 | 0x80000000;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);
	/* reserved */
	value = 0;
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &value);

	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &connman_service);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &connman_service);

	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &cookie);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection,
			message, 10 * 1000, &error);
	if (reply == NULL) {
		if (dbus_error_is_set(&error) == TRUE) {
			DBG("%s", error.message);
			dbus_error_free(&error);
		} else {
			DBG("Failed to request RTC timer");
		}
		dbus_message_unref(message);
		return 0;
	}
	dbus_message_unref(message);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		DBG("Failed to request RTC timer");
		return 0;
	}

	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_get_basic(&iter, &timer);
	dbus_message_iter_get_basic(&iter, &return_code);
	DBG("RTC timer id(%d), return(%d)", timer, return_code);

	dbus_message_unref(reply);

	return timer;
}

static gboolean __rtc_timeout_remove(int timer)
{
	DBusMessage *message, *reply;
	DBusMessageIter iter;
	DBusError error;
	int pid = 0, return_code = 0;
	const char *cookie = "";

	message = dbus_message_new_method_call(TIZEN_RTC_TIMER_SERVICE,
					TIZEN_RTC_TIMER_PATH, TIZEN_RTC_TIMER_INTERFACE,
					"alarm_delete");
	if (message == NULL)
		return -ENOMEM;

	dbus_message_iter_init_append(message, &iter);

	pid = getpid();
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &pid);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &timer);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &cookie);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection,
			message, 10 * 1000, &error);
	if (reply == NULL) {
		if (dbus_error_is_set(&error) == TRUE) {
			DBG("%s", error.message);
			dbus_error_free(&error);
		} else {
			DBG("Failed to remove RTC timer");
		}
		dbus_message_unref(message);
		return FALSE;
	}
	dbus_message_unref(message);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
		DBG("Failed to remove RTC timer");
		return FALSE;
	}

	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_get_basic(&iter, &return_code);
	DBG("RTC timer id(%d), return(%d)", timer, return_code);

	dbus_message_unref(reply);

	return TRUE;
}

int rtc_timeout_add_seconds(guint interval, GSourceFunc function,
							gpointer data, GDestroyNotify notify)
{
	int timer;
	time_t current_time;
	struct tm duetime;
	struct RTCTimer *timer_data;

	time(&current_time);
	current_time += interval;
	localtime_r(&current_time, &duetime);

	timer = __rtc_timeout_create(duetime);
	if (timer <= 0)
		return timer;

	timer_data = g_try_new0(struct RTCTimer, 1);
	if (timer_data == NULL)
		return 0;

	DBG("RTC timer %p", timer_data);

	timer_data->data = data;
	timer_data->func = function;
	timer_data->destroy_func = notify;

	g_hash_table_replace(timer_list, GINT_TO_POINTER(timer),
						(gpointer)timer_data);

	return timer;
}

gboolean rtc_timeout_remove(int timer)
{
	if (g_hash_table_lookup(timer_list, GINT_TO_POINTER(timer)) == NULL)
		return FALSE;

	if (__rtc_timeout_remove(timer) == FALSE)
		return FALSE;

	return g_hash_table_remove(timer_list, GINT_TO_POINTER(timer));
}

static DBusMessage *timer_expired(DBusConnection *conn,
								DBusMessage *msg, void *user_data)
{
	int timer = 0;
	DBusMessageIter iter;
	gpointer data;
	GSourceFunc func;
	struct RTCTimer *timer_data;

	DBG("conn %p", conn);

	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_get_basic(&iter, &timer);

	timer_data = (struct RTCTimer *)g_hash_table_lookup(timer_list,
										GINT_TO_POINTER(timer));
	if (timer_data == NULL)
		return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);

	DBG("RTC timer %p", timer_data);

	func = timer_data->func;
	data = timer_data->data;
	if (func != NULL)
		g_idle_add(func, data);

	g_hash_table_remove(timer_list, GINT_TO_POINTER(timer));

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static const GDBusMethodTable timer_methods[] = {
	{ GDBUS_METHOD("alarm_expired",
			GDBUS_ARGS({ "alarm_id", "i" }, { "service_name", "s" }),
			NULL, timer_expired) },
	{ },
};

static gboolean remove_timer_idle(gpointer data)
{
	struct GDestroyNotifyIdle *destroy = (struct GDestroyNotifyIdle *)data;

	DBG("");

	if (destroy == NULL)
		return FALSE;

	if (destroy->destroy_func != NULL)
		destroy->destroy_func(destroy->data);

	g_free(destroy);

	return FALSE;
}

static void remove_timer(gpointer data)
{
	struct GDestroyNotifyIdle *destroy;
	struct RTCTimer *timer_data = (struct RTCTimer *)data;

	DBG("");

	if (timer_data == NULL)
		return;

	if (timer_data->destroy_func == NULL)
		return;

	destroy = g_try_new0(struct GDestroyNotifyIdle, 1);
	if (destroy == NULL)
		return;

	destroy->data = timer_data->data;
	destroy->destroy_func = timer_data->destroy_func;

	g_idle_add(remove_timer_idle, destroy);

	g_free(timer_data);
}

int __connman_rtctimer_init(void)
{
	DBG("");

	connection = connman_dbus_get_connection();
	if (connection == NULL)
		return -1;

	timer_list = g_hash_table_new_full(g_direct_hash, g_direct_equal,
					NULL, remove_timer);

	g_dbus_register_interface(connection, TIZEN_RTC_TIMER_CLIENT_PATH,
					TIZEN_RTC_TIMER_CLIENT_INTERFACE,
					timer_methods,
					NULL, NULL, NULL, NULL);

	return 0;
}

void __connman_rtctimer_cleanup(void)
{
	DBG("");

	g_hash_table_destroy(timer_list);
	timer_list = NULL;

	dbus_connection_unref(connection);
}
