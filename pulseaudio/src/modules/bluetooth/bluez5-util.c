/***
  This file is part of PulseAudio.

  Copyright 2008-2013 João Paulo Rechi Vita

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

#include <pulse/xmalloc.h>

#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulsecore/dbus-shared.h>
#include <pulsecore/log.h>
#include <pulsecore/macro.h>
#include <pulsecore/refcnt.h>
#include <pulsecore/shared.h>

#include "a2dp-codecs.h"
#include "hfaudioagent.h"

#include "bluez5-util.h"
#ifdef BLUETOOTH_APTX_SUPPORT
#include <dlfcn.h>
#endif

#define BLUEZ_SERVICE "org.bluez"
#define BLUEZ_ADAPTER_INTERFACE BLUEZ_SERVICE ".Adapter1"
#define BLUEZ_DEVICE_INTERFACE BLUEZ_SERVICE ".Device1"
#define BLUEZ_MEDIA_INTERFACE BLUEZ_SERVICE ".Media1"
#define BLUEZ_MEDIA_ENDPOINT_INTERFACE BLUEZ_SERVICE ".MediaEndpoint1"
#define BLUEZ_MEDIA_TRANSPORT_INTERFACE BLUEZ_SERVICE ".MediaTransport1"

#define BLUEZ_ERROR_NOT_SUPPORTED "org.bluez.Error.NotSupported"

#define A2DP_SOURCE_ENDPOINT "/MediaEndpoint/A2DPSource"
#define A2DP_SINK_ENDPOINT "/MediaEndpoint/A2DPSink"
#ifdef BLUETOOTH_APTX_SUPPORT
#define A2DP_APTX_SOURCE_ENDPOINT "/MediaEndpoint/A2DPSource_aptx"
#endif

#define ENDPOINT_INTROSPECT_XML                                         \
    DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE                           \
    "<node>"                                                            \
    " <interface name=\"" BLUEZ_MEDIA_ENDPOINT_INTERFACE "\">"          \
    "  <method name=\"SetConfiguration\">"                              \
    "   <arg name=\"transport\" direction=\"in\" type=\"o\"/>"          \
    "   <arg name=\"properties\" direction=\"in\" type=\"ay\"/>"        \
    "  </method>"                                                       \
    "  <method name=\"SelectConfiguration\">"                           \
    "   <arg name=\"capabilities\" direction=\"in\" type=\"ay\"/>"      \
    "   <arg name=\"configuration\" direction=\"out\" type=\"ay\"/>"    \
    "  </method>"                                                       \
    "  <method name=\"ClearConfiguration\">"                            \
    "   <arg name=\"transport\" direction=\"in\" type=\"o\"/>"          \
    "  </method>"                                                       \
    "  <method name=\"Release\">"                                       \
    "  </method>"                                                       \
    " </interface>"                                                     \
    " <interface name=\"org.freedesktop.DBus.Introspectable\">"         \
    "  <method name=\"Introspect\">"                                    \
    "   <arg name=\"data\" type=\"s\" direction=\"out\"/>"              \
    "  </method>"                                                       \
    " </interface>"                                                     \
    "</node>"

struct pa_bluetooth_discovery {
    PA_REFCNT_DECLARE;

    pa_core *core;
    pa_dbus_connection *connection;
    bool filter_added;
    bool matches_added;
    bool objects_listed;
    pa_hook hooks[PA_BLUETOOTH_HOOK_MAX];
    pa_hashmap *adapters;
    pa_hashmap *devices;
    pa_hashmap *transports;

    hf_audio_agent_data *hf_audio_agent;
    PA_LLIST_HEAD(pa_dbus_pending, pending);
};

#ifdef BLUETOOTH_APTX_SUPPORT
static void *aptx_handle = NULL;

int pa_unload_aptx(void)
{
	if (aptx_handle == NULL) {
		pa_log_warn("Unable to unload apt-X library");
		return -1;
	}

	dlclose(aptx_handle);
	aptx_handle = NULL;

	pa_log_debug("unloaded apt-X library successfully");
	return 0;
}

int pa_load_aptx(const char *aptx_lib_name)
{
	char* lib_path = NULL ;

        if(aptx_lib_name == NULL)
		return -1;

        lib_path = pa_sprintf_malloc("%s/%s", PA_DLSEARCHPATH, aptx_lib_name);

	if (!lib_path)
		return -1;

	pa_log_info("aptx_lib_path = [%s]", lib_path);

	aptx_handle = dlopen(lib_path, RTLD_LAZY);
	if (aptx_handle == NULL) {
		pa_log_warn("Unable to load apt-X library [%s]", dlerror());
		pa_xfree(lib_path);
		return -1;
	}

	pa_log_debug("loaded apt-X library successfully");
	pa_xfree(lib_path);

	return 0;
}

void* pa_aptx_get_handle(void)
{
	return aptx_handle;
}
#endif

static pa_dbus_pending* send_and_add_to_pending(pa_bluetooth_discovery *y, DBusMessage *m,
                                                                  DBusPendingCallNotifyFunction func, void *call_data) {
    pa_dbus_pending *p;
    DBusPendingCall *call;

    pa_assert(y);
    pa_assert(m);

    pa_assert_se(dbus_connection_send_with_reply(pa_dbus_connection_get(y->connection), m, &call, -1));

    p = pa_dbus_pending_new(pa_dbus_connection_get(y->connection), m, call, y, call_data);
    PA_LLIST_PREPEND(pa_dbus_pending, y->pending, p);
    dbus_pending_call_set_notify(call, func, p, NULL);

    return p;
}

static const char *check_variant_property(DBusMessageIter *i) {
    const char *key;

    pa_assert(i);

    if (dbus_message_iter_get_arg_type(i) != DBUS_TYPE_STRING) {
        pa_log_error("Property name not a string.");
        return NULL;
    }

    dbus_message_iter_get_basic(i, &key);

    if (!dbus_message_iter_next(i)) {
        pa_log_error("Property value missing");
        return NULL;
    }

    if (dbus_message_iter_get_arg_type(i) != DBUS_TYPE_VARIANT) {
        pa_log_error("Property value not a variant.");
        return NULL;
    }

    return key;
}

pa_bluetooth_transport *pa_bluetooth_transport_new(pa_bluetooth_device *d, const char *owner, const char *path,
                                                   pa_bluetooth_profile_t p, const uint8_t *config, size_t size) {
    pa_bluetooth_transport *t;

    t = pa_xnew0(pa_bluetooth_transport, 1);
    t->device = d;
    t->owner = pa_xstrdup(owner);
    t->path = pa_xstrdup(path);
    t->profile = p;
    t->config_size = size;

    if (size > 0) {
        t->config = pa_xnew(uint8_t, size);
        memcpy(t->config, config, size);
    }

    pa_assert_se(pa_hashmap_put(d->discovery->transports, t->path, t) >= 0);

    return t;
}

static const char *transport_state_to_string(pa_bluetooth_transport_state_t state) {
    switch(state) {
        case PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED:
            return "disconnected";
        case PA_BLUETOOTH_TRANSPORT_STATE_IDLE:
            return "idle";
        case PA_BLUETOOTH_TRANSPORT_STATE_PLAYING:
            return "playing";
    }

    return "invalid";
}

static void transport_state_changed(pa_bluetooth_transport *t, pa_bluetooth_transport_state_t state) {
    bool old_any_connected;

    pa_assert(t);

    if (t->state == state)
        return;

    old_any_connected = pa_bluetooth_device_any_transport_connected(t->device);

    pa_log_info("Transport %s state changed from %s to %s",
                 t->path, transport_state_to_string(t->state), transport_state_to_string(state));

    t->state = state;
    if (state == PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED)
        t->device->transports[t->profile] = NULL;

    pa_hook_fire(&t->device->discovery->hooks[PA_BLUETOOTH_HOOK_TRANSPORT_STATE_CHANGED], t);

    if (old_any_connected != pa_bluetooth_device_any_transport_connected(t->device))
        pa_hook_fire(&t->device->discovery->hooks[PA_BLUETOOTH_HOOK_DEVICE_CONNECTION_CHANGED], t->device);
}

void pa_bluetooth_transport_put(pa_bluetooth_transport *t) {
    transport_state_changed(t, PA_BLUETOOTH_TRANSPORT_STATE_IDLE);
}

void pa_bluetooth_transport_free(pa_bluetooth_transport *t) {
    pa_assert(t);

    pa_hashmap_remove(t->device->discovery->transports, t->path);
    pa_xfree(t->owner);
    pa_xfree(t->path);
    pa_xfree(t->config);
    pa_xfree(t);
}

static int bluez5_transport_acquire_cb(pa_bluetooth_transport *t, bool optional, size_t *imtu, size_t *omtu) {
    DBusMessage *m, *r;
    DBusError err;
    int ret;
    uint16_t i, o;
    const char *method = optional ? "TryAcquire" : "Acquire";

    pa_assert(t);
    pa_assert(t->device);
    pa_assert(t->device->discovery);

    pa_assert_se(m = dbus_message_new_method_call(t->owner, t->path, BLUEZ_MEDIA_TRANSPORT_INTERFACE, method));

    dbus_error_init(&err);

    r = dbus_connection_send_with_reply_and_block(pa_dbus_connection_get(t->device->discovery->connection), m, -1, &err);
    if (!r) {
        if (optional && pa_streq(err.name, "org.bluez.Error.NotAvailable"))
            pa_log_info("Failed optional acquire of unavailable transport %s", t->path);
        else
            pa_log_error("Transport %s() failed for transport %s (%s)", method, t->path, err.message);

        dbus_error_free(&err);
        return -1;
    }

    if (!dbus_message_get_args(r, &err, DBUS_TYPE_UNIX_FD, &ret, DBUS_TYPE_UINT16, &i, DBUS_TYPE_UINT16, &o,
                               DBUS_TYPE_INVALID)) {
        pa_log_error("Failed to parse %s() reply: %s", method, err.message);
        dbus_error_free(&err);
        ret = -1;
        goto finish;
    }

    if (imtu)
        *imtu = i;

    if (omtu)
        *omtu = o;

finish:
    dbus_message_unref(r);
    return ret;
}

static void bluez5_transport_release_cb(pa_bluetooth_transport *t) {
    DBusMessage *m;
    DBusError err;

    pa_assert(t);
    pa_assert(t->device);
    pa_assert(t->device->discovery);

    dbus_error_init(&err);

    if (t->state <= PA_BLUETOOTH_TRANSPORT_STATE_IDLE) {
        pa_log_info("Transport %s auto-released by BlueZ or already released", t->path);
        pa_log_info("Allow Release!");
    }

    pa_assert_se(m = dbus_message_new_method_call(t->owner, t->path, BLUEZ_MEDIA_TRANSPORT_INTERFACE, "Release"));
    dbus_connection_send_with_reply_and_block(pa_dbus_connection_get(t->device->discovery->connection), m, -1, &err);

    if (dbus_error_is_set(&err)) {
        pa_log_error("Failed to release transport %s: %s", t->path, err.message);
        dbus_error_free(&err);
    } else
        pa_log_info("Transport %s released", t->path);
}

bool pa_bluetooth_device_any_transport_connected(const pa_bluetooth_device *d) {
    unsigned i;

    pa_assert(d);

    if (d->device_info_valid != 1)
        return false;

    for (i = 0; i < PA_BLUETOOTH_PROFILE_COUNT; i++)
        if (d->transports[i] && d->transports[i]->state != PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED)
            return true;

    return false;
}

#ifdef __TIZEN_BT__
bool pa_bluetooth_device_sink_transport_connected(const pa_bluetooth_device *d) {
    unsigned i;

    pa_assert(d);

    if (d->device_info_valid != 1)
        return false;

    for (i = 0; i < PA_BLUETOOTH_PROFILE_COUNT; i++)
        if (d->transports[i] &&
	     d->transports[i]->profile == PA_BLUETOOTH_PROFILE_A2DP_SINK &&
	     d->transports[i]->state != PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED)
            return true;

    return false;
}

bool pa_bluetooth_device_source_transport_connected(const pa_bluetooth_device *d) {
    unsigned i;

    pa_assert(d);

    if (d->device_info_valid != 1)
        return false;

    for (i = 0; i < PA_BLUETOOTH_PROFILE_COUNT; i++)
        if (d->transports[i] &&
        d->transports[i]->profile == PA_BLUETOOTH_PROFILE_A2DP_SOURCE &&
        d->transports[i]->state != PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED)
            return true;

    return false;
}
#endif

static int transport_state_from_string(const char* value, pa_bluetooth_transport_state_t *state) {
    pa_assert(value);
    pa_assert(state);

    if (pa_streq(value, "idle"))
        *state = PA_BLUETOOTH_TRANSPORT_STATE_IDLE;
    else if (pa_streq(value, "pending") || pa_streq(value, "active"))
        *state = PA_BLUETOOTH_TRANSPORT_STATE_PLAYING;
    else
        return -1;

    return 0;
}

static void parse_transport_property(pa_bluetooth_transport *t, DBusMessageIter *i) {
    const char *key;
    DBusMessageIter variant_i;

    key = check_variant_property(i);
    if (key == NULL)
        return;

    dbus_message_iter_recurse(i, &variant_i);

    switch (dbus_message_iter_get_arg_type(&variant_i)) {

        case DBUS_TYPE_STRING: {

            const char *value;
            dbus_message_iter_get_basic(&variant_i, &value);

            if (pa_streq(key, "State")) {
                pa_bluetooth_transport_state_t state;

                if (transport_state_from_string(value, &state) < 0) {
                    pa_log_error("Invalid state received: %s", value);
                    return;
                }

                transport_state_changed(t, state);
            }

            break;
        }
    }

    return;
}

static int parse_transport_properties(pa_bluetooth_transport *t, DBusMessageIter *i) {
    DBusMessageIter element_i;

    dbus_message_iter_recurse(i, &element_i);

    while (dbus_message_iter_get_arg_type(&element_i) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter dict_i;

        dbus_message_iter_recurse(&element_i, &dict_i);

        parse_transport_property(t, &dict_i);

        dbus_message_iter_next(&element_i);
    }

    return 0;
}

static pa_bluetooth_device* device_create(pa_bluetooth_discovery *y, const char *path) {
    pa_bluetooth_device *d;

    pa_assert(y);
    pa_assert(path);

    d = pa_xnew0(pa_bluetooth_device, 1);
    d->discovery = y;
    d->path = pa_xstrdup(path);

#ifdef __TIZEN_BT__
    d->uuids = pa_hashmap_new(pa_idxset_string_hash_func, pa_idxset_string_compare_func);
#else
    d->uuids = pa_hashmap_new_full(pa_idxset_string_hash_func, pa_idxset_string_compare_func, NULL, pa_xfree);
#endif

    pa_hashmap_put(y->devices, d->path, d);

    return d;
}

pa_bluetooth_device* pa_bluetooth_discovery_get_device_by_path(pa_bluetooth_discovery *y, const char *path) {
    pa_bluetooth_device *d;

    pa_assert(y);
    pa_assert(PA_REFCNT_VALUE(y) > 0);
    pa_assert(path);

    if ((d = pa_hashmap_get(y->devices, path)))
        if (d->device_info_valid == 1)
            return d;

    return NULL;
}

pa_bluetooth_device* pa_bluetooth_discovery_get_device_by_address(pa_bluetooth_discovery *y, const char *remote, const char *local) {
    pa_bluetooth_device *d;
    void *state = NULL;

    pa_assert(y);
    pa_assert(PA_REFCNT_VALUE(y) > 0);
    pa_assert(remote);
    pa_assert(local);

    while ((d = pa_hashmap_iterate(y->devices, &state, NULL)))
        if (pa_streq(d->address, remote) && pa_streq(d->adapter->address, local))
            return d->device_info_valid == 1 ? d : NULL;

    return NULL;
}

static void device_free(pa_bluetooth_device *d) {
    unsigned i;

    pa_assert(d);

    for (i = 0; i < PA_BLUETOOTH_PROFILE_COUNT; i++) {
        pa_bluetooth_transport *t;

        if (!(t = d->transports[i]))
            continue;

        transport_state_changed(t, PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED);
        pa_bluetooth_transport_free(t);
    }

    if (d->uuids) {
#ifdef __TIZEN_BT__
        pa_hashmap_free(d->uuids, NULL);
#else
        pa_hashmap_free(d->uuids);
#endif
    }

    d->discovery = NULL;
    d->adapter = NULL;
    pa_xfree(d->path);
    pa_xfree(d->alias);
    pa_xfree(d->address);
    pa_xfree(d);
}

static void device_remove(pa_bluetooth_discovery *y, const char *path) {
    pa_bluetooth_device *d;

    if (!(d = pa_hashmap_remove(y->devices, path)))
        pa_log_warn("Unknown device removed %s", path);
    else {
        pa_log_debug("Device %s removed", path);
        device_free(d);
    }
}

static void device_remove_all(pa_bluetooth_discovery *y) {
    pa_bluetooth_device *d;

    pa_assert(y);

    while ((d = pa_hashmap_steal_first(y->devices))) {
        d->device_info_valid = -1;
        pa_hook_fire(&y->hooks[PA_BLUETOOTH_HOOK_DEVICE_CONNECTION_CHANGED], d);
        device_free(d);
   }
}

static pa_bluetooth_adapter* adapter_create(pa_bluetooth_discovery *y, const char *path) {
    pa_bluetooth_adapter *a;

    pa_assert(y);
    pa_assert(path);

    a = pa_xnew0(pa_bluetooth_adapter, 1);
    a->discovery = y;
    a->path = pa_xstrdup(path);

    pa_hashmap_put(y->adapters, a->path, a);

    return a;
}

static void adapter_free(pa_bluetooth_adapter *a) {
    pa_bluetooth_device *d;
    void *state;

    pa_assert(a);
    pa_assert(a->discovery);

    PA_HASHMAP_FOREACH(d, a->discovery->devices, state)
        if (d->adapter == a)
            d->adapter = NULL;

    pa_xfree(a->path);
    pa_xfree(a->address);
    pa_xfree(a);
}

static void adapter_remove(pa_bluetooth_discovery *y, const char *path) {
    pa_bluetooth_adapter *a;

    if (!(a = pa_hashmap_remove(y->adapters, path)))
        pa_log_warn("Unknown adapter removed %s", path);
    else {
        pa_log_debug("Adapter %s removed", path);
        adapter_free(a);
    }
}

static void adapter_remove_all(pa_bluetooth_discovery *y) {
    pa_bluetooth_adapter *a;

    pa_assert(y);

    /* When this function is called all devices have already been freed */

    while ((a = pa_hashmap_steal_first(y->adapters)))
        adapter_free(a);
}

static void parse_device_property(pa_bluetooth_device *d, DBusMessageIter *i, bool is_property_change) {
    const char *key;
    DBusMessageIter variant_i;

    pa_assert(d);

    key = check_variant_property(i);
    if (key == NULL) {
        pa_log_error("Received invalid property for device %s", d->path);
        return;
    }

    dbus_message_iter_recurse(i, &variant_i);

    switch (dbus_message_iter_get_arg_type(&variant_i)) {

        case DBUS_TYPE_STRING: {
            const char *value;
            dbus_message_iter_get_basic(&variant_i, &value);

            if (pa_streq(key, "Alias")) {
                pa_xfree(d->alias);
                d->alias = pa_xstrdup(value);
                pa_log_debug("%s: %s", key, value);
            } else if (pa_streq(key, "Address")) {
                if (is_property_change) {
                    pa_log_warn("Device property 'Address' expected to be constant but changed for %s, ignoring", d->path);
                    return;
                }

                if (d->address) {
                    pa_log_warn("Device %s: Received a duplicate 'Address' property, ignoring", d->path);
                    return;
                }

                d->address = pa_xstrdup(value);
                pa_log_debug("%s: %s", key, value);
            }

            break;
        }

        case DBUS_TYPE_OBJECT_PATH: {
            const char *value;
            dbus_message_iter_get_basic(&variant_i, &value);

            if (pa_streq(key, "Adapter")) {

                if (is_property_change) {
                    pa_log_warn("Device property 'Adapter' expected to be constant but changed for %s, ignoring", d->path);
                    return;
                }

                if (d->adapter_path) {
                    pa_log_warn("Device %s: Received a duplicate 'Adapter' property, ignoring", d->path);
                    return;
                }

                d->adapter_path = pa_xstrdup(value);

                d->adapter = pa_hashmap_get(d->discovery->adapters, value);
                if (!d->adapter)
                    pa_log_info("Device %s: 'Adapter' property references an unknown adapter %s.", d->path, value);

                pa_log_debug("%s: %s", key, value);
            }

            break;
        }

        case DBUS_TYPE_UINT32: {
            uint32_t value;
            dbus_message_iter_get_basic(&variant_i, &value);

            if (pa_streq(key, "Class")) {
                d->class_of_device = value;
                pa_log_debug("%s: %d", key, value);
            }

            break;
        }

        case DBUS_TYPE_ARRAY: {
            DBusMessageIter ai;
            dbus_message_iter_recurse(&variant_i, &ai);

            if (dbus_message_iter_get_arg_type(&ai) == DBUS_TYPE_STRING && pa_streq(key, "UUIDs")) {
                /* bluetoothd never removes UUIDs from a device object so there
                 * is no need to handle it here. */
                while (dbus_message_iter_get_arg_type(&ai) != DBUS_TYPE_INVALID) {
                    const char *value;
                    char *uuid;

                    dbus_message_iter_get_basic(&ai, &value);

                    if (pa_hashmap_get(d->uuids, value)) {
                        dbus_message_iter_next(&ai);
                        continue;
                    }

                    uuid = pa_xstrdup(value);
                    pa_hashmap_put(d->uuids, uuid, uuid);

                    pa_log_debug("%s: %s", key, value);
                    dbus_message_iter_next(&ai);
                }
            }

            break;
        }
    }
}

static int parse_device_properties(pa_bluetooth_device *d, DBusMessageIter *i, bool is_property_change) {
    DBusMessageIter element_i;
    int ret = 0;

    dbus_message_iter_recurse(i, &element_i);

    while (dbus_message_iter_get_arg_type(&element_i) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter dict_i;

        dbus_message_iter_recurse(&element_i, &dict_i);
        parse_device_property(d, &dict_i, is_property_change);
        dbus_message_iter_next(&element_i);
    }

    if (!d->address || !d->adapter_path || !d->alias) {
        pa_log_error("Non-optional information missing for device %s", d->path);
        d->device_info_valid = -1;
        return -1;
    }

    d->device_info_valid = 1;
    return ret;
}

static void parse_adapter_properties(pa_bluetooth_adapter *a, DBusMessageIter *i, bool is_property_change) {
    DBusMessageIter element_i;

    pa_assert(a);

    dbus_message_iter_recurse(i, &element_i);

    while (dbus_message_iter_get_arg_type(&element_i) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter dict_i, variant_i;
        const char *key;

        dbus_message_iter_recurse(&element_i, &dict_i);

        key = check_variant_property(&dict_i);
        if (key == NULL) {
            pa_log_error("Received invalid property for adapter %s", a->path);
            return;
        }

        dbus_message_iter_recurse(&dict_i, &variant_i);

        if (dbus_message_iter_get_arg_type(&variant_i) == DBUS_TYPE_STRING && pa_streq(key, "Address")) {
            char *value;

            if (is_property_change) {
                pa_log_warn("Adapter property 'Address' expected to be constant but changed for %s, ignoring", a->path);
                return;
            }

            if (a->address) {
                pa_log_warn("Adapter %s received a duplicate 'Address' property, ignoring", a->path);
                return;
            }

            dbus_message_iter_get_basic(&variant_i, &value);
            a->address = pa_xstrdup(value);
        }

        dbus_message_iter_next(&element_i);
    }
}

static void register_endpoint_reply(DBusPendingCall *pending, void *userdata) {
    DBusMessage *r;
    pa_dbus_pending *p;
    pa_bluetooth_discovery *y;
    char *endpoint;

    pa_assert(pending);
    pa_assert_se(p = userdata);
    pa_assert_se(y = p->context_data);
    pa_assert_se(endpoint = p->call_data);
    pa_assert_se(r = dbus_pending_call_steal_reply(pending));

    if (dbus_message_is_error(r, BLUEZ_ERROR_NOT_SUPPORTED)) {
        pa_log_info("Couldn't register endpoint %s because it is disabled in BlueZ", endpoint);
        goto finish;
    }

    if (dbus_message_get_type(r) == DBUS_MESSAGE_TYPE_ERROR) {
        pa_log_error(BLUEZ_MEDIA_INTERFACE ".RegisterEndpoint() failed: %s: %s", dbus_message_get_error_name(r),
                     pa_dbus_get_error_message(r));
        goto finish;
    }

finish:
    dbus_message_unref(r);

    PA_LLIST_REMOVE(pa_dbus_pending, y->pending, p);
    pa_dbus_pending_free(p);

    pa_xfree(endpoint);
}

static void register_endpoint(pa_bluetooth_discovery *y, const char *path, const char *endpoint, const char *uuid) {
    DBusMessage *m;
    DBusMessageIter i, d;
    uint8_t codec = 0;

#ifdef BLUETOOTH_APTX_SUPPORT
    if(pa_streq(endpoint,A2DP_APTX_SOURCE_ENDPOINT))
	codec = A2DP_CODEC_VENDOR;
#endif
    pa_log_debug("Registering %s on adapter %s", endpoint, path);

    pa_assert_se(m = dbus_message_new_method_call(BLUEZ_SERVICE, path, BLUEZ_MEDIA_INTERFACE, "RegisterEndpoint"));

    dbus_message_iter_init_append(m, &i);
    dbus_message_iter_append_basic(&i, DBUS_TYPE_OBJECT_PATH, &endpoint);
    dbus_message_iter_open_container(&i, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                         DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &d);
    pa_dbus_append_basic_variant_dict_entry(&d, "UUID", DBUS_TYPE_STRING, &uuid);
    pa_dbus_append_basic_variant_dict_entry(&d, "Codec", DBUS_TYPE_BYTE, &codec);

    if (pa_streq(uuid, PA_BLUETOOTH_UUID_A2DP_SOURCE) || pa_streq(uuid, PA_BLUETOOTH_UUID_A2DP_SINK)) {
#ifdef __TIZEN_BT__
    if (codec == A2DP_CODEC_SBC) {
	a2dp_sbc_t capabilities;
	capabilities.channel_mode = SBC_CHANNEL_MODE_MONO | SBC_CHANNEL_MODE_DUAL_CHANNEL | SBC_CHANNEL_MODE_STEREO |
					SBC_CHANNEL_MODE_JOINT_STEREO;
	capabilities.frequency = SBC_SAMPLING_FREQ_16000 | SBC_SAMPLING_FREQ_32000 | SBC_SAMPLING_FREQ_44100 |
					SBC_SAMPLING_FREQ_48000;
	capabilities.allocation_method = SBC_ALLOCATION_SNR | SBC_ALLOCATION_LOUDNESS;
	capabilities.subbands = SBC_SUBBANDS_4 | SBC_SUBBANDS_8;
	capabilities.block_length = SBC_BLOCK_LENGTH_4 | SBC_BLOCK_LENGTH_8 | SBC_BLOCK_LENGTH_12 | SBC_BLOCK_LENGTH_16;
	capabilities.min_bitpool = MIN_BITPOOL;
	capabilities.max_bitpool = MAX_BITPOOL;
	pa_dbus_append_basic_array_variant_dict_entry(&d, "Capabilities", DBUS_TYPE_BYTE, &capabilities, sizeof(capabilities));
    }
#ifdef BLUETOOTH_APTX_SUPPORT
	if (codec == A2DP_CODEC_VENDOR ) {
		/* aptx */
		a2dp_aptx_t capabilities;
		capabilities.vendor_id[0] = APTX_VENDOR_ID0;
		capabilities.vendor_id[1] = APTX_VENDOR_ID1;
		capabilities.vendor_id[2] = APTX_VENDOR_ID2;
		capabilities.vendor_id[3] = APTX_VENDOR_ID3;
		capabilities.codec_id[0] = APTX_CODEC_ID0;
		capabilities.codec_id[1] = APTX_CODEC_ID1;
		capabilities.channel_mode= APTX_CHANNEL_MODE_STEREO;
		capabilities.frequency= APTX_SAMPLING_FREQ_44100;
		pa_dbus_append_basic_array_variant_dict_entry(&d, "Capabilities", DBUS_TYPE_BYTE, &capabilities, sizeof(capabilities));
	}
#endif /* BLUETOOTH_APTX_SUPPORT */
#else
		a2dp_sbc_t capabilities;
		capabilities.channel_mode = SBC_CHANNEL_MODE_MONO | SBC_CHANNEL_MODE_DUAL_CHANNEL | SBC_CHANNEL_MODE_STEREO |
									SBC_CHANNEL_MODE_JOINT_STEREO;
		capabilities.frequency = SBC_SAMPLING_FREQ_16000 | SBC_SAMPLING_FREQ_32000 | SBC_SAMPLING_FREQ_44100 |
								 SBC_SAMPLING_FREQ_48000;
		capabilities.allocation_method = SBC_ALLOCATION_SNR | SBC_ALLOCATION_LOUDNESS;
		capabilities.subbands = SBC_SUBBANDS_4 | SBC_SUBBANDS_8;
		capabilities.block_length = SBC_BLOCK_LENGTH_4 | SBC_BLOCK_LENGTH_8 | SBC_BLOCK_LENGTH_12 | SBC_BLOCK_LENGTH_16;
		capabilities.min_bitpool = MIN_BITPOOL;
		capabilities.max_bitpool = MAX_BITPOOL;
		pa_dbus_append_basic_array_variant_dict_entry(&d, "Capabilities", DBUS_TYPE_BYTE, &capabilities, sizeof(capabilities));
#endif /* __TIZEN_BT__ */
	}

    dbus_message_iter_close_container(&i, &d);

    send_and_add_to_pending(y, m, register_endpoint_reply, pa_xstrdup(endpoint));
}

static void parse_interfaces_and_properties(pa_bluetooth_discovery *y, DBusMessageIter *dict_i) {
    DBusMessageIter element_i;
    const char *path;
    void *state;
    pa_bluetooth_device *d;

    pa_assert(dbus_message_iter_get_arg_type(dict_i) == DBUS_TYPE_OBJECT_PATH);
    dbus_message_iter_get_basic(dict_i, &path);

    pa_assert_se(dbus_message_iter_next(dict_i));
    pa_assert(dbus_message_iter_get_arg_type(dict_i) == DBUS_TYPE_ARRAY);

    dbus_message_iter_recurse(dict_i, &element_i);

    while (dbus_message_iter_get_arg_type(&element_i) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter iface_i;
        const char *interface;

        dbus_message_iter_recurse(&element_i, &iface_i);

        pa_assert(dbus_message_iter_get_arg_type(&iface_i) == DBUS_TYPE_STRING);
        dbus_message_iter_get_basic(&iface_i, &interface);

        pa_assert_se(dbus_message_iter_next(&iface_i));
        pa_assert(dbus_message_iter_get_arg_type(&iface_i) == DBUS_TYPE_ARRAY);

        if (pa_streq(interface, BLUEZ_ADAPTER_INTERFACE)) {
            pa_bluetooth_adapter *a;

            if ((a = pa_hashmap_get(y->adapters, path))) {
                pa_log_error("Found duplicated D-Bus path for device %s", path);
                return;
            } else
                a = adapter_create(y, path);

            pa_log_debug("Adapter %s found", path);

            parse_adapter_properties(a, &iface_i, false);
            if (!a->address)
                return;

#ifdef SUPPORT_A2DP_SINK
            register_endpoint(y, path, A2DP_SINK_ENDPOINT, PA_BLUETOOTH_UUID_A2DP_SINK);
#else
            register_endpoint(y, path, A2DP_SOURCE_ENDPOINT, PA_BLUETOOTH_UUID_A2DP_SOURCE);
#endif
#ifdef BLUETOOTH_APTX_SUPPORT
            if (aptx_handle)
		register_endpoint(y, path, A2DP_APTX_SOURCE_ENDPOINT, PA_BLUETOOTH_UUID_A2DP_SOURCE);
#endif
        } else if (pa_streq(interface, BLUEZ_DEVICE_INTERFACE)) {

            if ((d = pa_hashmap_get(y->devices, path))) {
                if (d->device_info_valid != 0) {
                    pa_log_error("Found duplicated D-Bus path for device %s", path);
                    return;
                }
            } else
                d = device_create(y, path);

            pa_log_debug("Device %s found", d->path);

            parse_device_properties(d, &iface_i, false);

        } else
            pa_log_debug("Unknown interface %s found, skipping", interface);

        dbus_message_iter_next(&element_i);
    }

    PA_HASHMAP_FOREACH(d, y->devices, state)
        if (!d->adapter && d->adapter_path) {
            d->adapter = pa_hashmap_get(d->discovery->adapters, d->adapter_path);
            if (!d->adapter) {
                pa_log_error("Device %s is child of nonexistent adapter %s", d->path, d->adapter_path);
                d->device_info_valid = -1;
            }
        }

    return;
}

static void get_managed_objects_reply(DBusPendingCall *pending, void *userdata) {
    pa_dbus_pending *p;
    pa_bluetooth_discovery *y;
    DBusMessage *r;
    DBusMessageIter arg_i, element_i;

    pa_assert_se(p = userdata);
    pa_assert_se(y = p->context_data);
    pa_assert_se(r = dbus_pending_call_steal_reply(pending));

    if (dbus_message_is_error(r, DBUS_ERROR_UNKNOWN_METHOD)) {
        pa_log_warn("BlueZ D-Bus ObjectManager not available");
        goto finish;
    }

    if (dbus_message_get_type(r) == DBUS_MESSAGE_TYPE_ERROR) {
        pa_log_error("GetManagedObjects() failed: %s: %s", dbus_message_get_error_name(r), pa_dbus_get_error_message(r));
        goto finish;
    }

    if (!dbus_message_iter_init(r, &arg_i) || !pa_streq(dbus_message_get_signature(r), "a{oa{sa{sv}}}")) {
        pa_log_error("Invalid reply signature for GetManagedObjects()");
        goto finish;
    }

    dbus_message_iter_recurse(&arg_i, &element_i);
    while (dbus_message_iter_get_arg_type(&element_i) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter dict_i;

        dbus_message_iter_recurse(&element_i, &dict_i);

        parse_interfaces_and_properties(y, &dict_i);

        dbus_message_iter_next(&element_i);
    }

    y->objects_listed = true;

finish:
    dbus_message_unref(r);

    PA_LLIST_REMOVE(pa_dbus_pending, y->pending, p);
    pa_dbus_pending_free(p);
}

static void get_managed_objects(pa_bluetooth_discovery *y) {
    DBusMessage *m;

    pa_assert(y);

    pa_assert_se(m = dbus_message_new_method_call(BLUEZ_SERVICE, "/", "org.freedesktop.DBus.ObjectManager",
                                                  "GetManagedObjects"));
    send_and_add_to_pending(y, m, get_managed_objects_reply, NULL);
}

pa_hook* pa_bluetooth_discovery_hook(pa_bluetooth_discovery *y, pa_bluetooth_hook_t hook) {
    pa_assert(y);
    pa_assert(PA_REFCNT_VALUE(y) > 0);

    return &y->hooks[hook];
}

static DBusHandlerResult filter_cb(DBusConnection *bus, DBusMessage *m, void *userdata) {
    pa_bluetooth_discovery *y;
    DBusError err;

    pa_assert(bus);
    pa_assert(m);
    pa_assert_se(y = userdata);

    dbus_error_init(&err);

    if (dbus_message_is_signal(m, "org.freedesktop.DBus", "NameOwnerChanged")) {
        const char *name, *old_owner, *new_owner;

        if (!dbus_message_get_args(m, &err,
                                   DBUS_TYPE_STRING, &name,
                                   DBUS_TYPE_STRING, &old_owner,
                                   DBUS_TYPE_STRING, &new_owner,
                                   DBUS_TYPE_INVALID)) {
            pa_log_error("Failed to parse org.freedesktop.DBus.NameOwnerChanged: %s", err.message);
            goto fail;
        }

        if (pa_streq(name, BLUEZ_SERVICE)) {
            if (old_owner && *old_owner) {
                pa_log_debug("Bluetooth daemon disappeared");
                device_remove_all(y);
                adapter_remove_all(y);
                y->objects_listed = false;
            }

            if (new_owner && *new_owner) {
                pa_log_debug("Bluetooth daemon appeared");
                get_managed_objects(y);
            }
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    } else if (dbus_message_is_signal(m, "org.freedesktop.DBus.ObjectManager", "InterfacesAdded")) {
        DBusMessageIter arg_i;

#ifndef __TIZEN_BT__
        if (!y->objects_listed)
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED; /* No reply received yet from GetManagedObjects */
#endif

        if (!dbus_message_iter_init(m, &arg_i) || !pa_streq(dbus_message_get_signature(m), "oa{sa{sv}}")) {
            pa_log_error("Invalid signature found in InterfacesAdded");
            goto fail;
        }

        parse_interfaces_and_properties(y, &arg_i);

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    } else if (dbus_message_is_signal(m, "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved")) {
        const char *p;
        DBusMessageIter arg_i;
        DBusMessageIter element_i;

        if (!y->objects_listed)
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED; /* No reply received yet from GetManagedObjects */

        if (!dbus_message_iter_init(m, &arg_i) || !pa_streq(dbus_message_get_signature(m), "oas")) {
            pa_log_error("Invalid signature found in InterfacesRemoved");
            goto fail;
        }

        dbus_message_iter_get_basic(&arg_i, &p);

        pa_assert_se(dbus_message_iter_next(&arg_i));
        pa_assert(dbus_message_iter_get_arg_type(&arg_i) == DBUS_TYPE_ARRAY);

        dbus_message_iter_recurse(&arg_i, &element_i);

        while (dbus_message_iter_get_arg_type(&element_i) == DBUS_TYPE_STRING) {
            const char *iface;

            dbus_message_iter_get_basic(&element_i, &iface);

            if (pa_streq(iface, BLUEZ_DEVICE_INTERFACE))
                device_remove(y, p);
            else if (pa_streq(iface, BLUEZ_ADAPTER_INTERFACE))
                adapter_remove(y, p);

            dbus_message_iter_next(&element_i);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    } else if (dbus_message_is_signal(m, "org.freedesktop.DBus.Properties", "PropertiesChanged")) {
        DBusMessageIter arg_i;
        const char *iface;

        if (!y->objects_listed)
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED; /* No reply received yet from GetManagedObjects */

        if (!dbus_message_iter_init(m, &arg_i) || !pa_streq(dbus_message_get_signature(m), "sa{sv}as")) {
            pa_log_error("Invalid signature found in PropertiesChanged");
            goto fail;
        }

        dbus_message_iter_get_basic(&arg_i, &iface);

        pa_assert_se(dbus_message_iter_next(&arg_i));
        pa_assert(dbus_message_iter_get_arg_type(&arg_i) == DBUS_TYPE_ARRAY);

        if (pa_streq(iface, BLUEZ_ADAPTER_INTERFACE)) {
            pa_bluetooth_adapter *a;

            pa_log_debug("Properties changed in adapter %s", dbus_message_get_path(m));

            if (!(a = pa_hashmap_get(y->adapters, dbus_message_get_path(m)))) {
                pa_log_warn("Properties changed in unknown adapter");
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }

            parse_adapter_properties(a, &arg_i, true);

        } else if (pa_streq(iface, BLUEZ_DEVICE_INTERFACE)) {
            pa_bluetooth_device *d;

            pa_log_info("Properties changed in device %s", dbus_message_get_path(m));

            if (!(d = pa_hashmap_get(y->devices, dbus_message_get_path(m)))) {
                pa_log_warn("Properties changed in unknown device");
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }

            if (d->device_info_valid != 1) {
                pa_log_warn("Properties changed in a device which information is unknown or invalid");
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }

            parse_device_properties(d, &arg_i, true);
        } else if (pa_streq(iface, BLUEZ_MEDIA_TRANSPORT_INTERFACE)) {
            pa_bluetooth_transport *t;

            pa_log_info("Properties changed in transport %s", dbus_message_get_path(m));

            if (!(t = pa_hashmap_get(y->transports, dbus_message_get_path(m))))
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

            parse_transport_properties(t, &arg_i);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

fail:
    dbus_error_free(&err);

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static uint8_t a2dp_default_bitpool(uint8_t freq, uint8_t mode) {
    /* These bitpool values were chosen based on the A2DP spec recommendation */
    switch (freq) {
        case SBC_SAMPLING_FREQ_16000:
        case SBC_SAMPLING_FREQ_32000:
            return 53;

        case SBC_SAMPLING_FREQ_44100:

            switch (mode) {
                case SBC_CHANNEL_MODE_MONO:
                case SBC_CHANNEL_MODE_DUAL_CHANNEL:
                    return 31;

                case SBC_CHANNEL_MODE_STEREO:
                case SBC_CHANNEL_MODE_JOINT_STEREO:
#ifdef __TIZEN_BT__
                    return 35;
#else
                    return 53;
#endif
            }

            pa_log_warn("Invalid channel mode %u", mode);
            return 53;

        case SBC_SAMPLING_FREQ_48000:

            switch (mode) {
                case SBC_CHANNEL_MODE_MONO:
                case SBC_CHANNEL_MODE_DUAL_CHANNEL:
                    return 29;

                case SBC_CHANNEL_MODE_STEREO:
                case SBC_CHANNEL_MODE_JOINT_STEREO:
                    return 51;
            }

            pa_log_warn("Invalid channel mode %u", mode);
            return 51;
    }

    pa_log_warn("Invalid sampling freq %u", freq);
    return 53;
}

#ifdef BLUETOOTH_APTX_SUPPORT
static DBusMessage *endpoint_select_configuration_for_aptx(DBusConnection *c, DBusMessage *m, void *userdata) {
    a2dp_aptx_t *cap;
    a2dp_aptx_t config;
    uint8_t *pconf = (uint8_t *) &config;
    int size;
    DBusMessage *r;
    DBusError e;

    dbus_error_init(&e);

    if (!dbus_message_get_args(m, &e, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &cap, &size, DBUS_TYPE_INVALID)) {
        pa_log("org.bluez.MediaEndpoint.SelectConfiguration: %s", e.message);
        dbus_error_free(&e);
        goto fail;
    }

    pa_assert(size == sizeof(config));

    memset(&config, 0, sizeof(config));

    if (cap->vendor_id[0] == APTX_VENDOR_ID0 &&
        cap->vendor_id[1] == APTX_VENDOR_ID1 &&
        cap->vendor_id[2] == APTX_VENDOR_ID2 &&
        cap->vendor_id[3] == APTX_VENDOR_ID3 &&
         cap->codec_id[0] == APTX_CODEC_ID0  &&
         cap->codec_id[1] == APTX_CODEC_ID1  )
        pa_log_debug("A2DP_CODEC_NON_A2DP and this is APTX Codec");
    else {
        pa_log_debug("A2DP_CODEC_NON_A2DP but this is not APTX Codec");
        goto fail;
    }

    memcpy(&config,cap, sizeof(config));

/* The below code shuld be re-written by aptx */
/* And we should configure pulseaudio freq */

    if (cap->frequency & APTX_SAMPLING_FREQ_44100)
        config.frequency = APTX_SAMPLING_FREQ_44100;
    else if (cap->frequency & APTX_SAMPLING_FREQ_48000)
        config.frequency = APTX_SAMPLING_FREQ_48000;
    else if (cap->frequency & APTX_SAMPLING_FREQ_32000)
        config.frequency = APTX_SAMPLING_FREQ_32000;
    else if (cap->frequency & APTX_SAMPLING_FREQ_16000)
        config.frequency = APTX_SAMPLING_FREQ_16000;
    else {
        pa_log_error("No aptx supported frequencies");
        goto fail;
    }

    if (cap->channel_mode & APTX_CHANNEL_MODE_JOINT_STEREO)
        config.channel_mode = APTX_CHANNEL_MODE_STEREO;
    else if (cap->channel_mode & APTX_CHANNEL_MODE_STEREO)
        config.channel_mode = APTX_CHANNEL_MODE_STEREO;
    else if (cap->channel_mode & APTX_CHANNEL_MODE_DUAL_CHANNEL)
        config.channel_mode = APTX_CHANNEL_MODE_STEREO;
    else {
        pa_log_error("No aptx supported channel modes");
        goto fail;
    }

    pa_assert_se(r = dbus_message_new_method_return(m));

    pa_assert_se(dbus_message_append_args(
                                     r,
                                     DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &pconf, size,
                                     DBUS_TYPE_INVALID));

    return r;

fail:
    pa_assert_se(r = (dbus_message_new_error(m, "org.bluez.MediaEndpoint.Error.InvalidArguments",
                                                        "Unable to select configuration")));
    return r;
}
#endif

const char *pa_bluetooth_profile_to_string(pa_bluetooth_profile_t profile) {
    switch(profile) {
        case PA_BLUETOOTH_PROFILE_A2DP_SINK:
            return "a2dp_sink";
        case PA_BLUETOOTH_PROFILE_A2DP_SOURCE:
            return "a2dp_source";
        case PA_BLUETOOTH_PROFILE_HEADSET_HEAD_UNIT:
            return "hsp";
        case PA_BLUETOOTH_PROFILE_HEADSET_AUDIO_GATEWAY:
            return "hfgw";
        case PA_BLUETOOTH_PROFILE_OFF:
            return "off";
    }

    return NULL;
}

static DBusMessage *endpoint_set_configuration(DBusConnection *conn, DBusMessage *m, void *userdata) {
    pa_bluetooth_discovery *y = userdata;
    pa_bluetooth_device *d;
    pa_bluetooth_transport *t;
    const char *sender, *path, *endpoint_path, *dev_path = NULL, *uuid = NULL;
#ifdef __TIZEN_BT__
    uint8_t codec = 0;
#endif
    const uint8_t *config = NULL;
    int size = 0;
    pa_bluetooth_profile_t p = PA_BLUETOOTH_PROFILE_OFF;
    DBusMessageIter args, props;
    DBusMessage *r;

    if (!dbus_message_iter_init(m, &args) || !pa_streq(dbus_message_get_signature(m), "oa{sv}")) {
        pa_log_error("Invalid signature for method SetConfiguration()");
        goto fail2;
    }

    dbus_message_iter_get_basic(&args, &path);

    if (pa_hashmap_get(y->transports, path)) {
        pa_log_error("Endpoint SetConfiguration(): Transport %s is already configured.", path);
        goto fail2;
    }

    pa_assert_se(dbus_message_iter_next(&args));

    dbus_message_iter_recurse(&args, &props);
    if (dbus_message_iter_get_arg_type(&props) != DBUS_TYPE_DICT_ENTRY)
        goto fail;

    /* Read transport properties */
    while (dbus_message_iter_get_arg_type(&props) == DBUS_TYPE_DICT_ENTRY) {
        const char *key;
        DBusMessageIter value, entry;
        int var;

        dbus_message_iter_recurse(&props, &entry);
        dbus_message_iter_get_basic(&entry, &key);

        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &value);

        var = dbus_message_iter_get_arg_type(&value);

        if (pa_streq(key, "UUID")) {
            if (var != DBUS_TYPE_STRING) {
                pa_log_error("Property %s of wrong type %c", key, (char)var);
                goto fail;
            }

            dbus_message_iter_get_basic(&value, &uuid);

            endpoint_path = dbus_message_get_path(m);
#ifdef BLUETOOTH_APTX_SUPPORT
            if (pa_streq(endpoint_path, A2DP_SOURCE_ENDPOINT) ||
		pa_streq(endpoint_path, A2DP_APTX_SOURCE_ENDPOINT)) {
                if (pa_streq(uuid, PA_BLUETOOTH_UUID_A2DP_SOURCE))
                    p = PA_BLUETOOTH_PROFILE_A2DP_SINK;
            }
#else
            if (pa_streq(endpoint_path, A2DP_SOURCE_ENDPOINT)) {
                if (pa_streq(uuid, PA_BLUETOOTH_UUID_A2DP_SOURCE))
                    p = PA_BLUETOOTH_PROFILE_A2DP_SINK;
            }
#endif
            else if (pa_streq(endpoint_path, A2DP_SINK_ENDPOINT)) {
                if (pa_streq(uuid, PA_BLUETOOTH_UUID_A2DP_SINK))
                    p = PA_BLUETOOTH_PROFILE_A2DP_SOURCE;
            }

            if (p == PA_BLUETOOTH_PROFILE_OFF) {
                pa_log_error("UUID %s of transport %s incompatible with endpoint %s", uuid, path, endpoint_path);
                goto fail;
            }
#ifdef __TIZEN_BT__
        } else if (pa_streq(key, "Codec")) {
	    if (var != DBUS_TYPE_BYTE) {
		pa_log_error("Property %s of wrong type %c", key, (char)var);
		goto fail;
	    }

	    dbus_message_iter_get_basic(&value, &codec);
#endif
        } else if (pa_streq(key, "Device")) {
            if (var != DBUS_TYPE_OBJECT_PATH) {
                pa_log_error("Property %s of wrong type %c", key, (char)var);
                goto fail;
            }

            dbus_message_iter_get_basic(&value, &dev_path);
        } else if (pa_streq(key, "Configuration")) {
            DBusMessageIter array;
            a2dp_sbc_t *c;

            if (var != DBUS_TYPE_ARRAY) {
                pa_log_error("Property %s of wrong type %c", key, (char)var);
                goto fail;
            }

            dbus_message_iter_recurse(&value, &array);
            var = dbus_message_iter_get_arg_type(&array);
            if (var != DBUS_TYPE_BYTE) {
                pa_log_error("%s is an array of wrong type %c", key, (char)var);
                goto fail;
            }

            dbus_message_iter_get_fixed_array(&array, &config, &size);
#ifndef BLUETOOTH_APTX_SUPPORT
            if (size != sizeof(a2dp_sbc_t)) {
                pa_log_error("Configuration array of invalid size");
                goto fail;
            }

            c = (a2dp_sbc_t *) config;

            if (c->frequency != SBC_SAMPLING_FREQ_16000 && c->frequency != SBC_SAMPLING_FREQ_32000 &&
                c->frequency != SBC_SAMPLING_FREQ_44100 && c->frequency != SBC_SAMPLING_FREQ_48000) {
                pa_log_error("Invalid sampling frequency in configuration");
                goto fail;
            }

            if (c->channel_mode != SBC_CHANNEL_MODE_MONO && c->channel_mode != SBC_CHANNEL_MODE_DUAL_CHANNEL &&
                c->channel_mode != SBC_CHANNEL_MODE_STEREO && c->channel_mode != SBC_CHANNEL_MODE_JOINT_STEREO) {
                pa_log_error("Invalid channel mode in configuration");
                goto fail;
            }

            if (c->allocation_method != SBC_ALLOCATION_SNR && c->allocation_method != SBC_ALLOCATION_LOUDNESS) {
                pa_log_error("Invalid allocation method in configuration");
                goto fail;
            }

            if (c->subbands != SBC_SUBBANDS_4 && c->subbands != SBC_SUBBANDS_8) {
                pa_log_error("Invalid SBC subbands in configuration");
                goto fail;
            }

            if (c->block_length != SBC_BLOCK_LENGTH_4 && c->block_length != SBC_BLOCK_LENGTH_8 &&
                c->block_length != SBC_BLOCK_LENGTH_12 && c->block_length != SBC_BLOCK_LENGTH_16) {
                pa_log_error("Invalid block length in configuration");
                goto fail;
            }
#endif
        }

        dbus_message_iter_next(&props);
    }

    if ((d = pa_hashmap_get(y->devices, dev_path))) {
        if (d->device_info_valid == -1) {
            pa_log_error("Information about device %s is invalid", dev_path);
            goto fail2;
        }
    } else {
        /* InterfacesAdded signal is probably on it's way, device_info_valid is kept as 0. */
        pa_log_warn("SetConfiguration() received for unknown device %s", dev_path);
        d = device_create(y, dev_path);
    }

    if (p == PA_BLUETOOTH_PROFILE_OFF || d->transports[p] != NULL) {
        pa_log_error("Cannot configure transport %s because profile %s is already used", path, pa_bluetooth_profile_to_string(p));
        goto fail2;
    }

    sender = dbus_message_get_sender(m);

    pa_assert_se(r = dbus_message_new_method_return(m));
    pa_assert_se(dbus_connection_send(pa_dbus_connection_get(y->connection), r, NULL));
    dbus_message_unref(r);

    d->transports[p] = t = pa_bluetooth_transport_new(d, sender, path, p, config, size);
#ifdef __TIZEN_BT__
    t->codec = codec;
    d->transports[p] = t;
#endif

    t->acquire = bluez5_transport_acquire_cb;
    t->release = bluez5_transport_release_cb;
    pa_bluetooth_transport_put(t);

    pa_log_info("Transport %s available for profile %s", t->path, pa_bluetooth_profile_to_string(t->profile));

    return NULL;

fail:
    pa_log_error("Endpoint SetConfiguration(): invalid arguments");

fail2:
    pa_assert_se(r = dbus_message_new_error(m, "org.bluez.Error.InvalidArguments", "Unable to set configuration"));
    return r;
}

static DBusMessage *endpoint_select_configuration(DBusConnection *conn, DBusMessage *m, void *userdata) {
    pa_bluetooth_discovery *y = userdata;
    a2dp_sbc_t *cap, config;
    uint8_t *pconf = (uint8_t *) &config;
    int i, size;
    DBusMessage *r;
    DBusError err;

    static const struct {
        uint32_t rate;
        uint8_t cap;
    } freq_table[] = {
        { 16000U, SBC_SAMPLING_FREQ_16000 },
        { 32000U, SBC_SAMPLING_FREQ_32000 },
        { 44100U, SBC_SAMPLING_FREQ_44100 },
        { 48000U, SBC_SAMPLING_FREQ_48000 }
    };

#ifdef BLUETOOTH_APTX_SUPPORT
    if (dbus_message_has_path(m, A2DP_APTX_SOURCE_ENDPOINT))
        return endpoint_select_configuration_for_aptx(conn ,m ,userdata);
#endif

    dbus_error_init(&err);

    if (!dbus_message_get_args(m, &err, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &cap, &size, DBUS_TYPE_INVALID)) {
        pa_log_error("Endpoint SelectConfiguration(): %s", err.message);
        dbus_error_free(&err);
        goto fail;
    }

    if (size != sizeof(config)) {
        pa_log_error("Capabilities array has invalid size");
        goto fail;
    }

    pa_zero(config);

    /* Find the lowest freq that is at least as high as the requested sampling rate */
    for (i = 0; (unsigned) i < PA_ELEMENTSOF(freq_table); i++)
        if (freq_table[i].rate >= y->core->default_sample_spec.rate && (cap->frequency & freq_table[i].cap)) {
            config.frequency = freq_table[i].cap;
            break;
        }

    if ((unsigned) i == PA_ELEMENTSOF(freq_table)) {
        for (--i; i >= 0; i--) {
            if (cap->frequency & freq_table[i].cap) {
                config.frequency = freq_table[i].cap;
                break;
            }
        }

        if (i < 0) {
            pa_log_error("Not suitable sample rate");
            goto fail;
        }
    }

    pa_assert((unsigned) i < PA_ELEMENTSOF(freq_table));

    if (y->core->default_sample_spec.channels <= 1) {
        if (cap->channel_mode & SBC_CHANNEL_MODE_MONO)
            config.channel_mode = SBC_CHANNEL_MODE_MONO;
        else if (cap->channel_mode & SBC_CHANNEL_MODE_JOINT_STEREO)
            config.channel_mode = SBC_CHANNEL_MODE_JOINT_STEREO;
        else if (cap->channel_mode & SBC_CHANNEL_MODE_STEREO)
            config.channel_mode = SBC_CHANNEL_MODE_STEREO;
        else if (cap->channel_mode & SBC_CHANNEL_MODE_DUAL_CHANNEL)
            config.channel_mode = SBC_CHANNEL_MODE_DUAL_CHANNEL;
        else {
            pa_log_error("No supported channel modes");
            goto fail;
        }
    }

    if (y->core->default_sample_spec.channels >= 2) {
        if (cap->channel_mode & SBC_CHANNEL_MODE_JOINT_STEREO)
            config.channel_mode = SBC_CHANNEL_MODE_JOINT_STEREO;
        else if (cap->channel_mode & SBC_CHANNEL_MODE_STEREO)
            config.channel_mode = SBC_CHANNEL_MODE_STEREO;
        else if (cap->channel_mode & SBC_CHANNEL_MODE_DUAL_CHANNEL)
            config.channel_mode = SBC_CHANNEL_MODE_DUAL_CHANNEL;
        else if (cap->channel_mode & SBC_CHANNEL_MODE_MONO)
            config.channel_mode = SBC_CHANNEL_MODE_MONO;
        else {
            pa_log_error("No supported channel modes");
            goto fail;
        }
    }

    if (cap->block_length & SBC_BLOCK_LENGTH_16)
        config.block_length = SBC_BLOCK_LENGTH_16;
    else if (cap->block_length & SBC_BLOCK_LENGTH_12)
        config.block_length = SBC_BLOCK_LENGTH_12;
    else if (cap->block_length & SBC_BLOCK_LENGTH_8)
        config.block_length = SBC_BLOCK_LENGTH_8;
    else if (cap->block_length & SBC_BLOCK_LENGTH_4)
        config.block_length = SBC_BLOCK_LENGTH_4;
    else {
        pa_log_error("No supported block lengths");
        goto fail;
    }

    if (cap->subbands & SBC_SUBBANDS_8)
        config.subbands = SBC_SUBBANDS_8;
    else if (cap->subbands & SBC_SUBBANDS_4)
        config.subbands = SBC_SUBBANDS_4;
    else {
        pa_log_error("No supported subbands");
        goto fail;
    }

    if (cap->allocation_method & SBC_ALLOCATION_LOUDNESS)
        config.allocation_method = SBC_ALLOCATION_LOUDNESS;
    else if (cap->allocation_method & SBC_ALLOCATION_SNR)
        config.allocation_method = SBC_ALLOCATION_SNR;

    config.min_bitpool = (uint8_t) PA_MAX(MIN_BITPOOL, cap->min_bitpool);
    config.max_bitpool = (uint8_t) PA_MIN(a2dp_default_bitpool(config.frequency, config.channel_mode), cap->max_bitpool);

    if (config.min_bitpool > config.max_bitpool)
        goto fail;

    pa_assert_se(r = dbus_message_new_method_return(m));
    pa_assert_se(dbus_message_append_args(r, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &pconf, size, DBUS_TYPE_INVALID));

    return r;

fail:
    pa_assert_se(r = dbus_message_new_error(m, "org.bluez.Error.InvalidArguments", "Unable to select configuration"));
    return r;
}

static DBusMessage *endpoint_clear_configuration(DBusConnection *conn, DBusMessage *m, void *userdata) {
    pa_bluetooth_discovery *y = userdata;
    pa_bluetooth_transport *t;
    DBusMessage *r;
    DBusError err;
    const char *path;

    dbus_error_init(&err);

    if (!dbus_message_get_args(m, &err, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID)) {
        pa_log_error("Endpoint ClearConfiguration(): %s", err.message);
        dbus_error_free(&err);
        goto fail;
    }

    if ((t = pa_hashmap_get(y->transports, path))) {
        pa_log_info("Clearing transport %s profile %s", t->path, pa_bluetooth_profile_to_string(t->profile));
        transport_state_changed(t, PA_BLUETOOTH_TRANSPORT_STATE_DISCONNECTED);
        pa_bluetooth_transport_free(t);
    }

    pa_assert_se(r = dbus_message_new_method_return(m));

    return r;

fail:
    pa_assert_se(r = dbus_message_new_error(m, "org.bluez.Error.InvalidArguments", "Unable to clear configuration"));
    return r;
}

static DBusMessage *endpoint_release(DBusConnection *conn, DBusMessage *m, void *userdata) {
    DBusMessage *r;

    pa_assert_se(r = dbus_message_new_error(m, BLUEZ_MEDIA_ENDPOINT_INTERFACE ".Error.NotImplemented",
                                            "Method not implemented"));

    return r;
}

static DBusHandlerResult endpoint_handler(DBusConnection *c, DBusMessage *m, void *userdata) {
    struct pa_bluetooth_discovery *y = userdata;
    DBusMessage *r = NULL;
    const char *path, *interface, *member;

    pa_assert(y);

    path = dbus_message_get_path(m);
    interface = dbus_message_get_interface(m);
    member = dbus_message_get_member(m);

    pa_log_info("dbus: path=%s, interface=%s, member=%s", path, interface, member);

#ifdef BLUETOOTH_APTX_SUPPORT
    if (!pa_streq(path, A2DP_SOURCE_ENDPOINT) && !pa_streq(path, A2DP_SINK_ENDPOINT)
         && !pa_streq(path,A2DP_APTX_SOURCE_ENDPOINT))
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
#else
    if (!pa_streq(path, A2DP_SOURCE_ENDPOINT) && !pa_streq(path, A2DP_SINK_ENDPOINT))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
#endif /* BLUETOOTH_APTX_SUPPORT */

    if (dbus_message_is_method_call(m, "org.freedesktop.DBus.Introspectable", "Introspect")) {
        const char *xml = ENDPOINT_INTROSPECT_XML;

        pa_assert_se(r = dbus_message_new_method_return(m));
        pa_assert_se(dbus_message_append_args(r, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID));

    } else if (dbus_message_is_method_call(m, BLUEZ_MEDIA_ENDPOINT_INTERFACE, "SetConfiguration"))
        r = endpoint_set_configuration(c, m, userdata);
    else if (dbus_message_is_method_call(m, BLUEZ_MEDIA_ENDPOINT_INTERFACE, "SelectConfiguration"))
        r = endpoint_select_configuration(c, m, userdata);
    else if (dbus_message_is_method_call(m, BLUEZ_MEDIA_ENDPOINT_INTERFACE, "ClearConfiguration"))
        r = endpoint_clear_configuration(c, m, userdata);
    else if (dbus_message_is_method_call(m, BLUEZ_MEDIA_ENDPOINT_INTERFACE, "Release"))
        r = endpoint_release(c, m, userdata);
    else
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (r) {
        pa_assert_se(dbus_connection_send(pa_dbus_connection_get(y->connection), r, NULL));
        dbus_message_unref(r);
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static void endpoint_init(pa_bluetooth_discovery *y, pa_bluetooth_profile_t profile) {
    static const DBusObjectPathVTable vtable_endpoint = {
        .message_function = endpoint_handler,
    };

    pa_assert(y);

    switch(profile) {
        case PA_BLUETOOTH_PROFILE_A2DP_SINK:
            pa_assert_se(dbus_connection_register_object_path(pa_dbus_connection_get(y->connection), A2DP_SOURCE_ENDPOINT,
                                                              &vtable_endpoint, y));
#ifdef BLUETOOTH_APTX_SUPPORT
            pa_assert_se(dbus_connection_register_object_path(pa_dbus_connection_get(y->connection), A2DP_APTX_SOURCE_ENDPOINT,
                                                              &vtable_endpoint, y));
#endif
            break;
        case PA_BLUETOOTH_PROFILE_A2DP_SOURCE:
            pa_assert_se(dbus_connection_register_object_path(pa_dbus_connection_get(y->connection), A2DP_SINK_ENDPOINT,
                                                              &vtable_endpoint, y));
            break;
        default:
            pa_assert_not_reached();
            break;
    }
}

static void endpoint_done(pa_bluetooth_discovery *y, pa_bluetooth_profile_t profile) {
    pa_assert(y);

    switch(profile) {
        case PA_BLUETOOTH_PROFILE_A2DP_SINK:
            dbus_connection_unregister_object_path(pa_dbus_connection_get(y->connection), A2DP_SOURCE_ENDPOINT);
#ifdef BLUETOOTH_APTX_SUPPORT
            dbus_connection_unregister_object_path(pa_dbus_connection_get(y->connection), A2DP_APTX_SOURCE_ENDPOINT);
#endif
            break;
        case PA_BLUETOOTH_PROFILE_A2DP_SOURCE:
            dbus_connection_unregister_object_path(pa_dbus_connection_get(y->connection), A2DP_SINK_ENDPOINT);
            break;
        default:
            pa_assert_not_reached();
            break;
    }
}

pa_bluetooth_discovery* pa_bluetooth_discovery_get(pa_core *c) {
    pa_bluetooth_discovery *y;
    DBusError err;
    DBusConnection *conn;
    unsigned i;

    y = pa_xnew0(pa_bluetooth_discovery, 1);
    PA_REFCNT_INIT(y);
    y->core = c;
    y->adapters = pa_hashmap_new(pa_idxset_string_hash_func, pa_idxset_string_compare_func);
    y->devices = pa_hashmap_new(pa_idxset_string_hash_func, pa_idxset_string_compare_func);
    y->transports = pa_hashmap_new(pa_idxset_string_hash_func, pa_idxset_string_compare_func);
    PA_LLIST_HEAD_INIT(pa_dbus_pending, y->pending);

    for (i = 0; i < PA_BLUETOOTH_HOOK_MAX; i++)
        pa_hook_init(&y->hooks[i], y);

    pa_shared_set(c, "bluetooth-discovery", y);

    dbus_error_init(&err);

    if (!(y->connection = pa_dbus_bus_get(y->core, DBUS_BUS_SYSTEM, &err))) {
        pa_log_error("Failed to get D-Bus connection: %s", err.message);
        goto fail;
    }

    conn = pa_dbus_connection_get(y->connection);

    /* dynamic detection of bluetooth audio devices */
    if (!dbus_connection_add_filter(conn, filter_cb, y, NULL)) {
        pa_log_error("Failed to add filter function");
        goto fail;
    }
    y->filter_added = true;

    if (pa_dbus_add_matches(conn, &err,
            "type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',member='NameOwnerChanged'"
            ",arg0='" BLUEZ_SERVICE "'",
            "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded'",
            "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.ObjectManager',"
            "member='InterfacesRemoved'",
            "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'"
            ",arg0='" BLUEZ_ADAPTER_INTERFACE "'",
            "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'"
            ",arg0='" BLUEZ_DEVICE_INTERFACE "'",
            "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged'"
            ",arg0='" BLUEZ_MEDIA_TRANSPORT_INTERFACE "'",
            NULL) < 0) {
        pa_log_error("Failed to add D-Bus matches: %s", err.message);
        goto fail;
    }
    y->matches_added = true;

    endpoint_init(y, PA_BLUETOOTH_PROFILE_A2DP_SINK);

#ifndef __TIZEN_BT__
    endpoint_init(y, PA_BLUETOOTH_PROFILE_A2DP_SOURCE);
#endif

#ifdef BLUETOOTH_APTX_SUPPORT
    endpoint_init(y, PA_BLUETOOTH_PROFILE_A2DP_SOURCE);
#endif

#ifndef __TIZEN_BT__
    y->hf_audio_agent = hf_audio_agent_init(c);
#endif

    get_managed_objects(y);

    return y;

fail:
    pa_bluetooth_discovery_unref(y);
    dbus_error_free(&err);

    return NULL;
}

pa_bluetooth_discovery* pa_bluetooth_discovery_ref(pa_bluetooth_discovery *y) {
    pa_assert(y);
    pa_assert(PA_REFCNT_VALUE(y) > 0);

    PA_REFCNT_INC(y);

    return y;
}

void pa_bluetooth_discovery_unref(pa_bluetooth_discovery *y) {
    pa_assert(y);
    pa_assert(PA_REFCNT_VALUE(y) > 0);

    if (PA_REFCNT_DEC(y) > 0)
        return;

    pa_dbus_free_pending_list(&y->pending);

    if (y->devices) {
        device_remove_all(y);
#ifdef __TIZEN_BT__
        pa_hashmap_free(y->devices, NULL);
#else
        pa_hashmap_free(y->devices);
#endif
    }

    if (y->adapters) {
        adapter_remove_all(y);
#ifdef __TIZEN_BT__
        pa_hashmap_free(y->adapters, NULL);
#else
        pa_hashmap_free(y->adapters);
#endif
    }

    if (y->transports) {
        pa_assert(pa_hashmap_isempty(y->transports));
#ifdef __TIZEN_BT__
        pa_hashmap_free(y->transports, NULL);
#else
        pa_hashmap_free(y->transports);
#endif
    }

#ifndef __TIZEN_BT__
    if (y->hf_audio_agent)
        hf_audio_agent_done(y->hf_audio_agent);
#endif

    if (y->connection) {

        if (y->matches_added)
            pa_dbus_remove_matches(pa_dbus_connection_get(y->connection),
                "type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',member='NameOwnerChanged',"
                "arg0='" BLUEZ_SERVICE "'",
                "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.ObjectManager',"
                "member='InterfacesAdded'",
                "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.ObjectManager',"
                "member='InterfacesRemoved'",
                "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.Properties',"
                "member='PropertiesChanged',arg0='" BLUEZ_ADAPTER_INTERFACE "'",
                "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.Properties',"
                "member='PropertiesChanged',arg0='" BLUEZ_DEVICE_INTERFACE "'",
                "type='signal',sender='" BLUEZ_SERVICE "',interface='org.freedesktop.DBus.Properties',"
                "member='PropertiesChanged',arg0='" BLUEZ_MEDIA_TRANSPORT_INTERFACE "'",
                NULL);

        if (y->filter_added)
            dbus_connection_remove_filter(pa_dbus_connection_get(y->connection), filter_cb, y);

        endpoint_done(y, PA_BLUETOOTH_PROFILE_A2DP_SINK);

#ifndef __TIZEN_BT__
        endpoint_done(y, PA_BLUETOOTH_PROFILE_A2DP_SOURCE);
#endif
#ifdef BLUETOOTH_APTX_SUPPORT
        endpoint_done(y, PA_BLUETOOTH_PROFILE_A2DP_SOURCE);
#endif

        pa_dbus_connection_unref(y->connection);
    }

    pa_shared_remove(y->core, "bluetooth-discovery");
    pa_xfree(y);
}
