/*
 * Copyright (C) 2013 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "TizenSystemUtilities.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#include <dbus/dbus.h>

namespace WTF {

int invokeDbusMethod(const char* destination, const char* path, const char* name, const char* method, const char* signal, const char* parameters[])
{
    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, 0);
    if (!connection) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("dbus_bus_get error");
#endif
        return -EBADMSG;
    }
    dbus_connection_set_exit_on_disconnect(connection, false);

    DBusMessage* message = dbus_message_new_method_call(destination, path, name, method);
    if (!message) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("dbus_message_new_method_call(%s:%s-%s) error", path, name, method);
#endif
        dbus_connection_unref(connection);
        return -EBADMSG;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);

    int returnedValue = 0;
    if (signal && parameters) {
        unsigned i = 0;
        int int_type;
        uint64_t int64_type;
        for (char* ch = const_cast<char*>(signal); *ch != '\0'; ++i, ++ch) {
            switch (*ch) {
            case 'i':
                int_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &int_type);
                break;
            case 'u':
                int_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &int_type);
                break;
            case 't':
                int64_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &int64_type);
                break;
            case 's':
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, parameters[i]);
                break;
            default:
                returnedValue = -EINVAL;
            }
        }
    }

    if (returnedValue < 0) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("append_variant error(%d)", returnedValue);
#endif
        dbus_connection_unref(connection);
        dbus_message_unref(message);
        return -EBADMSG;
    }

    dbus_bool_t result = dbus_connection_send(connection, message, 0);
    dbus_message_unref(message);
    dbus_connection_flush(connection);
    dbus_connection_unref(connection);
    if (!result) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("dbus_connection_send error");
#endif
        return -EBADMSG;
    }

    return result;
}

#if ENABLE(TIZEN_BROWSER_DASH_MODE)
void setWebkitDashMode(DashMode mode, int flag)
{
    const char destination[] = "org.tizen.system.deviced";
    const char path[] = "/Org/Tizen/System/DeviceD/PmQos";
    const char name[] = "org.tizen.system.deviced.PmQos";
    const char* modeName = 0;

    switch(mode) {
    case DashModeBrowserDash:
        modeName = "BrowserDash";
        break;
    case DashModeBrowserLoading:
        modeName = "BrowserLoading";
        break;
    case DashModeBrowserJavaScript:
        modeName = "BrowserJavaScript";
        break;
    case DashModeNone:
    default:
        return;
    }

    const char* parameters[1];
    char val[32];
    snprintf(val, sizeof(val), "%d", flag);
    parameters[0] = val;

    int ret = invokeDbusMethod(destination, path, name, modeName, "i", parameters);
    if (ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("setWebkitDashMode(%d:%d) returns %d", mode, flag, ret);
#endif
    }
}
#endif

} // namespace WTF
