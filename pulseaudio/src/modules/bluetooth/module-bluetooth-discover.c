/***
  This file is part of PulseAudio.

  Copyright 2013 João Paulo Rechi Vita

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

#include <pulsecore/core-util.h>
#include <pulsecore/macro.h>
#include <pulsecore/module.h>
#ifdef BLUETOOTH_APTX_SUPPORT
#include <pulsecore/modargs.h>
#endif

#include "module-bluetooth-discover-symdef.h"

PA_MODULE_AUTHOR("João Paulo Rechi Vita");
PA_MODULE_DESCRIPTION("Detect available Bluetooth daemon and load the corresponding discovery module");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_USAGE("sco_sink=<name of sink> "
		"sco_source=<name of source> "
#ifdef BLUETOOTH_APTX_SUPPORT
		"aptx_lib_name=<name of aptx library name>"
#endif
);
PA_MODULE_LOAD_ONCE(true);

static const char* const valid_modargs[] = {
	"sco_sink",
	"sco_source",
	"async", /* deprecated */
#ifdef BLUETOOTH_APTX_SUPPORT
    "aptx_lib_name",
#endif
#ifdef __TIZEN_BT__
    "enable_scmst",
#endif
    NULL
};

struct userdata {
    uint32_t bluez5_module_idx;
    uint32_t bluez4_module_idx;
};

static bool pa_module_exists(const char *name) {
    const char *paths, *state = NULL;
    char *n, *p, *pathname;
    bool result;

    pa_assert(name);

    if (name[0] == PA_PATH_SEP_CHAR) {
        result = access(name, F_OK) == 0 ? true : false;
        pa_log_debug("Checking for existence of '%s': %s", name, result ? "success" : "failure");
        if (result)
            return true;
    }

    if (!(paths = lt_dlgetsearchpath()))
        return false;

    /* strip .so from the end of name, if present */
    n = pa_xstrdup(name);
    p = rindex(n, '.');
    if (p && pa_streq(p, PA_SOEXT))
        p[0] = 0;

    while ((p = pa_split(paths, ":", &state))) {
        pathname = pa_sprintf_malloc("%s" PA_PATH_SEP "%s" PA_SOEXT, p, n);
        result = access(pathname, F_OK) == 0 ? true : false;
        pa_log_debug("Checking for existence of '%s': %s", pathname, result ? "success" : "failure");
        pa_xfree(pathname);
        pa_xfree(p);
        if (result) {
            pa_xfree(n);
            return true;
        }
    }

    state = NULL;
    if (PA_UNLIKELY(pa_run_from_build_tree())) {
        while ((p = pa_split(paths, ":", &state))) {
            pathname = pa_sprintf_malloc("%s" PA_PATH_SEP ".libs" PA_PATH_SEP "%s" PA_SOEXT, p, n);
            result = access(pathname, F_OK) == 0 ? true : false;
            pa_log_debug("Checking for existence of '%s': %s", pathname, result ? "success" : "failure");
            pa_xfree(pathname);
            pa_xfree(p);
            if (result) {
                pa_xfree(n);
                return true;
            }
        }
    }

    pa_xfree(n);
    return false;
}


int pa__init(pa_module* m) {
    struct userdata *u;
    pa_module *mm;

#ifdef BLUETOOTH_APTX_SUPPORT
    pa_modargs *ma = NULL;
    const char *aptx_lib_name = NULL;
    char* args;
#endif
    pa_assert(m);

#ifdef BLUETOOTH_APTX_SUPPORT
    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log("Failed to parse module arguments");
        pa__done(m);
        return -1;
    }

    if (pa_modargs_get_value(ma, "async", NULL))
        pa_log_warn("The 'async' argument is deprecated and does nothing.");

    aptx_lib_name = pa_modargs_get_value(ma, "aptx_lib_name", NULL);
    args =  pa_sprintf_malloc("aptx_lib_name=%s",aptx_lib_name);
#endif

    m->userdata = u = pa_xnew0(struct userdata, 1);
    u->bluez5_module_idx = PA_INVALID_INDEX;
    u->bluez4_module_idx = PA_INVALID_INDEX;

    if (pa_module_exists("module-bluez5-discover")) {
#ifdef BLUETOOTH_APTX_SUPPORT
        mm = pa_module_load(m->core, "module-bluez5-discover", args);
#else
        mm = pa_module_load(m->core, "module-bluez5-discover", NULL);
#endif
        if (mm)
            u->bluez5_module_idx = mm->index;
    }

    if (pa_module_exists("module-bluez4-discover")) {
        mm = pa_module_load(m->core, "module-bluez4-discover",  NULL);
        if (mm)
            u->bluez4_module_idx = mm->index;
    }

    if (u->bluez5_module_idx == PA_INVALID_INDEX && u->bluez4_module_idx == PA_INVALID_INDEX) {
        pa_xfree(u);
	pa_xfree(args);
	pa_modargs_free(ma);
        return -1;
    }

    pa_xfree(args);
    pa_modargs_free(ma);

    return 0;
}

void pa__done(pa_module* m) {
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata))
        return;

    if (u->bluez5_module_idx != PA_INVALID_INDEX)
        pa_module_unload_by_index(m->core, u->bluez5_module_idx, true);

    if (u->bluez4_module_idx != PA_INVALID_INDEX)
        pa_module_unload_by_index(m->core, u->bluez4_module_idx, true);

    pa_xfree(u);
}
