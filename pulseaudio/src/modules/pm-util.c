/***
  This file is part of PulseAudio.

  Copyright 2013 Seungbae Shin <seungbae.shin@samsung.com>
  Copyright 2013 Seunghun Pi <sh.pi@samsung.com>

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <dbus/dbus.h>

#include "pm-util.h"

#include <pulsecore/log.h>

#define BUS_NAME       "org.tizen.system.deviced"
#define OBJECT_PATH    "/Org/Tizen/System/DeviceD"
#define INTERFACE_NAME BUS_NAME

#define DEVICED_PATH_DISPLAY		OBJECT_PATH"/Display"
#define DEVICED_INTERFACE_DISPLAY	INTERFACE_NAME".display"

#define METHOD_LOCK_STATE		"lockstate"
#define METHOD_UNLOCK_STATE		"unlockstate"
#define METHOD_CHANGE_STATE		"changestate"

#define STR_LCD_OFF   "lcdoff"
#define STR_LCD_DIM   "lcddim"
#define STR_LCD_ON    "lcdon"
#define STR_SUSPEND   "suspend"

#define STR_STAYCURSTATE "staycurstate"
#define STR_GOTOSTATENOW "gotostatenow"

#define STR_HOLDKEYBLOCK "holdkeyblock"
#define STR_STANDBYMODE  "standbymode"
#define STR_NULL         "NULL"

#define STR_SLEEP_MARGIN "sleepmargin"
#define STR_RESET_TIMER  "resettimer"
#define STR_KEEP_TIMER   "keeptimer"

#define DBG(fmt, argc...)   pa_log_debug_verbose(fmt"\n", ##argc)
#define ERR(fmt, argc...)   pa_log_error(fmt"\n", ##argc)

#define DBUS_REPLY_TIMEOUT (120 * 1000)

static int append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
    char *ch;
    int i;
    int int_type;
    uint64_t int64_type;

    if (!sig || !param)
        return 0;

    for (ch = (char*)sig, i = 0; *ch != '\0'; ++i, ++ch) {
        switch (*ch) {
        case 'i':
            int_type = atoi(param[i]);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
            break;
        case 'u':
            int_type = atoi(param[i]);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
            break;
        case 't':
            int64_type = atoi(param[i]);
            dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
            break;
        case 's':
            dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
            break;
        default:
            return -EINVAL;
        }
    }

    return 0;
}

static DBusMessage *invoke_dbus_method_sync(const char *dest, const char *path,
        const char *interface, const char *method,
        const char *sig, char *param[])
{
    DBusConnection *conn;
    DBusMessage *msg;
    DBusMessageIter iter;
    DBusMessage *reply;
    DBusError err;
    int r;

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
    if (!conn) {
        ERR("dbus_bus_get error");
        return NULL;
    }

    msg = dbus_message_new_method_call(dest, path, interface, method);
    if (!msg) {
        ERR("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
        return NULL;
    }

    dbus_message_iter_init_append(msg, &iter);
    r = append_variant(&iter, sig, param);
    if (r < 0) {
        ERR("append_variant error(%d)", r);
        return NULL;
    }

    dbus_error_init(&err);

    reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);
    if (!reply) {
        ERR("dbus_connection_send error(No reply)");
    }

    if (dbus_error_is_set(&err)) {
        ERR("dbus_connection_send error(%s:%s)", err.name, err.message);
        reply = NULL;
    }

    dbus_message_unref(msg);
    dbus_error_free(&err);

    return reply;
}

/*Async functions are added because sync functions take about 600ms sometimes*/
static bool invoke_dbus_method_async(const char *dest, const char *path,
        const char *interface, const char *method,
        const char *sig, char *param[])
{
    DBusConnection *conn;
    DBusMessage *msg;
    DBusMessageIter iter;
    DBusError err;
    bool result;
    int r;

    conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
    if (!conn) {
        ERR("dbus_bus_get error");
        return NULL;
    }

    msg = dbus_message_new_method_call(dest, path, interface, method);
    if (!msg) {
        ERR("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
        return NULL;
    }

    dbus_message_iter_init_append(msg, &iter);
    r = append_variant(&iter, sig, param);
    if (r < 0) {
        ERR("append_variant error(%d)", r);
        return NULL;
    }

    dbus_error_init(&err);

    result= dbus_connection_send(conn, msg, NULL);
    if (!result) {
        ERR("dbus_connection_send error");
    }

    if (dbus_error_is_set(&err)) {
        ERR("dbus_connection_send error(%s:%s)", err.name, err.message);
    }

    dbus_message_unref(msg);

    return result;
}

static int display_lock_state(char *state, char *flag, unsigned int timeout)
{
    DBusError err;
    DBusMessage *msg;
    char *pa[4];
    char str_timeout[32];
    int ret, val;

    pa[0] = state;
    pa[1] = flag;
    pa[2] = STR_NULL;
    snprintf(str_timeout, sizeof(str_timeout), "%d", timeout);
    pa[3] = str_timeout;

    msg = invoke_dbus_method_sync(BUS_NAME, DEVICED_PATH_DISPLAY, DEVICED_INTERFACE_DISPLAY,
            METHOD_LOCK_STATE, "sssi", pa);
    if (!msg)
        return -EBADMSG;

    dbus_error_init(&err);

    ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
    if (!ret) {
        ERR("no message : [%s:%s]", err.name, err.message);
        val = -EBADMSG;
    }
    dbus_message_unref(msg);
    dbus_error_free(&err);

    DBG("%s-%s : %d", DEVICED_INTERFACE_DISPLAY, METHOD_LOCK_STATE, val);
    return val;
}

/*Async functions are added because sync functions take about 600ms sometimes*/
static int display_lock_state_async(char *state, char *flag, unsigned int timeout)
{
    char *pa[4];
    char str_timeout[32];
    int ret = 0, val = 0;

    pa[0] = state;
    pa[1] = flag;
    pa[2] = STR_NULL;
    snprintf(str_timeout, sizeof(str_timeout), "%d", timeout);
    pa[3] = str_timeout;
    ret = invoke_dbus_method_async(BUS_NAME, DEVICED_PATH_DISPLAY, DEVICED_INTERFACE_DISPLAY,
            METHOD_LOCK_STATE, "sssi", pa);

    if (!ret) {
        ERR("invoke_dbus_method_async failed");
        val = -ECOMM;
    }

    return val;
}

static int display_unlock_state(char *state, char *flag)
{
    DBusError err;
    DBusMessage *msg;
    char *pa[2];
    int ret, val;

    pa[0] = state;
    pa[1] = flag;

    msg = invoke_dbus_method_sync(BUS_NAME, DEVICED_PATH_DISPLAY, DEVICED_INTERFACE_DISPLAY,
            METHOD_UNLOCK_STATE, "ss", pa);
    if (!msg)
        return -EBADMSG;

    dbus_error_init(&err);

    ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
    if (!ret) {
        ERR("no message : [%s:%s]", err.name, err.message);
        val = -EBADMSG;
    }

    dbus_message_unref(msg);
    dbus_error_free(&err);

    DBG("%s-%s : %d", DEVICED_INTERFACE_DISPLAY, METHOD_UNLOCK_STATE, val);
    return val;

}

/*Async functions are added because sync functions take about 600ms sometimes*/
static int display_unlock_state_async(char *state, char *flag)
{
    char *pa[2];
    int ret = 0, val = 0;

    pa[0] = state;
    pa[1] = flag;

    ret = invoke_dbus_method_async(BUS_NAME, DEVICED_PATH_DISPLAY, DEVICED_INTERFACE_DISPLAY,
            METHOD_UNLOCK_STATE, "ss", pa);

    if (!ret) {
        ERR("invoke_dbus_method_async failed");
        val = -ECOMM;
    }

    return val;
}

/* EXPORT Functions */
int pm_display_lock (void)
{
    return display_lock_state_async(STR_LCD_OFF, STR_STAYCURSTATE, 0);
}

int pm_display_unlock (void)
{
    return display_unlock_state_async(STR_LCD_OFF, STR_SLEEP_MARGIN);
}
