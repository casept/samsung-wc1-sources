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

#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulsecore/macro.h>
#include <pulsecore/module.h>
#include <pulsecore/shared.h>
#ifdef __TIZEN_BT__
#include <pulsecore/modargs.h>
#endif
#include "bluez5-util.h"

#include "module-bluez5-discover-symdef.h"

PA_MODULE_AUTHOR("João Paulo Rechi Vita");
PA_MODULE_DESCRIPTION("Detect available BlueZ 5 Bluetooth audio devices and load BlueZ 5 Bluetooth audio drivers");
PA_MODULE_VERSION(PACKAGE_VERSION);
#ifdef __TIZEN_BT__
PA_MODULE_USAGE("sco_sink=<name of sink> "
					"sco_source=<name of source> "
#ifdef BLUETOOTH_APTX_SUPPORT
					"aptx_lib_name=<name of aptx library name>"
#endif
);
#endif
PA_MODULE_LOAD_ONCE(true);

#ifdef __TIZEN_BT__
static int bt_suspend_on_idle_timeout = 1; /*1 sec*/

static const char* const valid_modargs[] = {
    "sco_sink",
    "sco_source",
#ifdef BLUETOOTH_APTX_SUPPORT
    "aptx_lib_name",
#endif
    NULL
};
#endif

struct userdata {
    pa_module *module;
    pa_core *core;
    pa_hashmap *loaded_device_paths;
    pa_hook_slot *device_connection_changed_slot;
    pa_bluetooth_discovery *discovery;
};

static pa_hook_result_t device_connection_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_device *d, struct userdata *u) {
    bool module_loaded;

    pa_assert(d);
    pa_assert(u);

    module_loaded = pa_hashmap_get(u->loaded_device_paths, d->path) ? true : false;

    if (module_loaded && !pa_bluetooth_device_any_transport_connected(d)) {
        /* disconnection, the module unloads itself */
        pa_log_debug("Unregistering module for %s", d->path);
        pa_hashmap_remove(u->loaded_device_paths, d->path);
        return PA_HOOK_OK;
    }

    if (!module_loaded && pa_bluetooth_device_any_transport_connected(d)) {
        /* a new device has been connected */
        pa_module *m;

#ifdef __TIZEN_BT__
    char *args = pa_sprintf_malloc("address=\"%s\" path=\"%s\"", d->address, d->path);
    char *tmp;
#else
    char *args = pa_sprintf_malloc("path=%s", d->path);
#endif

#ifdef __TIZEN_BT__
    if (pa_bluetooth_device_sink_transport_connected(d) == true) {
        tmp = pa_sprintf_malloc("%s profile=\"a2dp_sink\"", args);
        pa_xfree(args);
        args = tmp;
    }

    if (pa_streq("CARENS", d->alias)){
	bt_suspend_on_idle_timeout = 0; /* 0ms */
        pa_log_info("Apply suspend on idle timeout (%d)", bt_suspend_on_idle_timeout);
    }

    tmp = pa_sprintf_malloc("%s sink_properties=\"module-suspend-on-idle.timeout=%d\"",
                                              args, bt_suspend_on_idle_timeout);
    pa_xfree(args);
    args = tmp;

    if (pa_bluetooth_device_source_transport_connected(d) == true) {
        args = pa_sprintf_malloc("%s profile=\"a2dp_source\"", args);
	pa_xfree(args);
	args = tmp;
    }
#endif

        pa_log_debug("Loading module-bluez5-device %s", args);
        m = pa_module_load(u->module->core, "module-bluez5-device", args);
        pa_xfree(args);

        if (m)
            /* No need to duplicate the path here since the device object will
             * exist for the whole hashmap entry lifespan */
            pa_hashmap_put(u->loaded_device_paths, d->path, d->path);
        else
            pa_log_warn("Failed to load module for device %s", d->path);

        return PA_HOOK_OK;
    }

    return PA_HOOK_OK;
}

int pa__init(pa_module *m) {
    struct userdata *u;
#ifdef BLUETOOTH_APTX_SUPPORT
    pa_modargs *ma = NULL;
    const char *aptx_lib_name = NULL;
#endif

    pa_assert(m);

#ifdef BLUETOOTH_APTX_SUPPORT
    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log("Failed to parse module arguments");
        goto fail;
    }

    if (pa_modargs_get_value(ma, "async", NULL))
        pa_log_warn("The 'async' argument is deprecated and does nothing.");

    aptx_lib_name = pa_modargs_get_value(ma, "aptx_lib_name", NULL);
    if (aptx_lib_name)
        pa_load_aptx(aptx_lib_name);
    else
        pa_log("Failed to parse aptx_lib_name argument.");
#endif

    m->userdata = u = pa_xnew0(struct userdata, 1);
    u->module = m;
    u->core = m->core;
    u->loaded_device_paths = pa_hashmap_new(pa_idxset_string_hash_func, pa_idxset_string_compare_func);

    if (!(u->discovery = pa_bluetooth_discovery_get(u->core)))
        goto fail;

    u->device_connection_changed_slot =
        pa_hook_connect(pa_bluetooth_discovery_hook(u->discovery, PA_BLUETOOTH_HOOK_DEVICE_CONNECTION_CHANGED),
                        PA_HOOK_NORMAL, (pa_hook_cb_t) device_connection_changed_cb, u);

    if (ma)
	pa_modargs_free(ma);

    return 0;

fail:
    if (ma)
	pa_modargs_free(ma);

    pa__done(m);
    return -1;
}

void pa__done(pa_module *m) {
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata))
        return;

    if (u->device_connection_changed_slot)
        pa_hook_slot_free(u->device_connection_changed_slot);

    if (u->discovery)
        pa_bluetooth_discovery_unref(u->discovery);

    if (u->loaded_device_paths)
#ifdef __TIZEN_BT__
        pa_hashmap_free(u->loaded_device_paths,NULL);
#else
    	 pa_hashmap_free(u->loaded_device_paths);
#endif

#ifdef BLUETOOTH_APTX_SUPPORT
    pa_unload_aptx();
#endif

    pa_xfree(u);
}
