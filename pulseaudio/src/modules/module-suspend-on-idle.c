/***
  This file is part of PulseAudio.

  Copyright 2006 Lennart Poettering

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/xmalloc.h>
#include <pulse/timeval.h>
#include <pulse/rtclock.h>

#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulsecore/sink-input.h>
#include <pulsecore/source-output.h>
#include <pulsecore/modargs.h>
#include <pulsecore/log.h>
#include <pulsecore/namereg.h>
#include <pulsecore/strbuf.h>
#include "module-suspend-on-idle-symdef.h"

PA_MODULE_AUTHOR("Lennart Poettering");
PA_MODULE_DESCRIPTION("When a sink/source is idle for too long, suspend it");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(TRUE);

#define USE_PM_LOCK /* Enable as default */
#ifdef USE_PM_LOCK
#include <pulsecore/mutex.h>
#include "pm-util.h"

typedef struct pa_pm_list pa_pm_list;

struct pa_pm_list {
    uint32_t index;
    pa_bool_t is_sink;
    PA_LLIST_FIELDS(pa_pm_list);
};

#endif


static const char* const valid_modargs[] = {
    "timeout",
    NULL,
};

struct userdata {
    pa_core *core;
    pa_usec_t timeout;
    pa_hashmap *device_infos;
    pa_hook_slot
        *sink_new_slot,
        *source_new_slot,
        *sink_unlink_slot,
        *source_unlink_slot,
        *sink_state_changed_slot,
        *source_state_changed_slot;

    pa_hook_slot
        *sink_input_new_slot,
        *source_output_new_slot,
        *sink_input_unlink_slot,
        *source_output_unlink_slot,
        *sink_input_move_start_slot,
        *source_output_move_start_slot,
        *sink_input_move_finish_slot,
        *source_output_move_finish_slot,
        *sink_input_state_changed_slot,
        *source_output_state_changed_slot;
#ifdef USE_PM_LOCK
    pa_mutex* pm_mutex;
    PA_LLIST_HEAD(pa_pm_list, pm_list);
    pa_bool_t is_pm_locked;
#endif /* USE_PM_LOCK */
};

struct device_info {
    struct userdata *userdata;
    pa_sink *sink;
    pa_source *source;
    pa_usec_t last_use;
    pa_time_event *time_event;
};


#ifdef USE_PM_LOCK

enum {
    PM_SUSPEND = 0,
    PM_RESUME
};

enum {
    PM_SOURCE = 0,
    PM_SINK
};

#define GET_STR(s) ((s)? "sink":"source")
#define USE_DEVICE_DEFAULT_TIMEOUT         -1

static void _pm_list_add_if_not_exist(struct userdata *u, int is_sink, uint32_t index_to_add)
{
    struct pa_pm_list *item_info = NULL;
    struct pa_pm_list *item_info_n = NULL;

    /* Search if exists */
    PA_LLIST_FOREACH_SAFE(item_info, item_info_n, u->pm_list) {
        if (item_info->is_sink == is_sink && item_info->index == index_to_add) {
        pa_log_debug_verbose("[PM] Already index (%s:%d) exists on list[%p], return",
                GET_STR(is_sink), index_to_add, u->pm_list);
        return;
        }
    }

    /* Add new index */
    item_info = pa_xnew0(pa_pm_list, 1);
    item_info->is_sink = is_sink;
    item_info->index = index_to_add;

    PA_LLIST_PREPEND(pa_pm_list, u->pm_list, item_info);

    pa_log_debug_verbose("[PM] Added (%s:%d) to list[%p]", GET_STR(is_sink), index_to_add, u->pm_list);
}

static void _pm_list_remove_if_exist(struct userdata *u, int is_sink, uint32_t index_to_remove)
{
    struct pa_pm_list *item_info = NULL;
    struct pa_pm_list *item_info_n = NULL;

    /* Remove if exists */
    PA_LLIST_FOREACH_SAFE(item_info, item_info_n, u->pm_list) {
        if (item_info->is_sink == is_sink && item_info->index == index_to_remove) {
            pa_log_debug_verbose("[PM] Found index (%s:%d) exists on list[%p], Remove it",
                    GET_STR(is_sink), index_to_remove, u->pm_list);

            PA_LLIST_REMOVE(struct pa_pm_list, u->pm_list, item_info);
            pa_xfree (item_info);
        }
    }
}

static void _pm_list_dump(struct userdata *u, pa_strbuf *s)
{
    struct pa_pm_list *list_info = NULL;
    struct pa_pm_list *list_info_n = NULL;

    if (u->pm_list) {
        PA_LLIST_FOREACH_SAFE(list_info, list_info_n, u->pm_list) {
            pa_strbuf_printf(s, "[%s:%d]", GET_STR(list_info->is_sink), list_info->index);
        }
    }
    if (pa_strbuf_isempty(s)) {
        pa_strbuf_puts(s, "empty");
    }
}

static void update_pm_status (struct userdata *u, int is_sink, int index, int is_resume)
{
    int ret = -1;
    pa_strbuf *before, *after;
    char *b = NULL, *a = NULL;

    pa_mutex_lock(u->pm_mutex);

    before = pa_strbuf_new();
    _pm_list_dump(u, before);

    if (is_resume) {
        _pm_list_add_if_not_exist(u, is_sink, index);

        if (u->pm_list) {
            if (!u->is_pm_locked) {
                ret = pm_display_lock();
                if (ret < 0)
                    pa_log_warn("pm_lock_state failed");
                else
                    u->is_pm_locked = TRUE;
            } else {
                pa_log_debug_verbose("already locked state, skip lock");
                goto exit;
            }
        }
    } else {
        _pm_list_remove_if_exist(u, is_sink, index);

        if (u->pm_list == NULL) {
            if (u->is_pm_locked) {
                ret = pm_display_unlock();
                if (ret < 0)
                    pa_log_warn("pm_unlock_state failed");
                else
                    u->is_pm_locked = FALSE;
            } else {
                pa_log_debug_verbose("already unlocked state, skip unlock");
                goto exit;
            }
        }
    }

    after = pa_strbuf_new();
    _pm_list_dump(u, after);

    b = pa_strbuf_tostring_free(before);
    a = pa_strbuf_tostring_free(after);
    pa_log_info("[PM] %s [%s:%d] ret[%d] list[%p] before:%s after:%s",
            (is_resume) ? "resume" : "suspend", GET_STR(is_sink), index, ret, u->pm_list,
            b, a);
    pa_xfree(b);
    pa_xfree(a);
    after = NULL;
    before = NULL;

exit:
    if (after)
        pa_strbuf_free(after);

    if (before)
        pa_strbuf_free(before);

    pa_mutex_unlock(u->pm_mutex);
}
#endif /* USE_PM_LOCK */

static pa_bool_t _is_audio_pm_needed (pa_proplist *p)
{
    char* value = pa_proplist_gets (p, "need_audio_pm");
    if (value) {
        return atoi (value);
    }
    return 0;
}

static void timeout_cb(pa_mainloop_api*a, pa_time_event* e, const struct timeval *t, void *userdata) {
    struct device_info *d = userdata;
    int ret = -1;

    pa_assert(d);

    d->userdata->core->mainloop->time_restart(d->time_event, NULL);

    /* SINK */
    if (d->sink && pa_sink_check_suspend(d->sink) <= 0 && !(d->sink->suspend_cause & PA_SUSPEND_IDLE)) {
        pa_log_info_verbose("Sink %s idle for too long, suspending ...", d->sink->name);
        pa_sink_suspend(d->sink, TRUE, PA_SUSPEND_IDLE);


#ifdef USE_PM_LOCK
        update_pm_status(d->userdata, PM_SINK, d->sink->index, PM_SUSPEND);
#endif /* USE_PM_LOCK */
    }

    /* SOURCE */
    if (d->source && pa_source_check_suspend(d->source) <= 0 && !(d->source->suspend_cause & PA_SUSPEND_IDLE)) {
        pa_log_info("Source %s idle for too long, suspending ...", d->source->name);
        pa_source_suspend(d->source, TRUE, PA_SUSPEND_IDLE);

#ifdef USE_PM_LOCK
        update_pm_status(d->userdata, PM_SOURCE, d->source->index, PM_SUSPEND);
#endif /* USE_PM_LOCK */
    }
}

static void restart(struct device_info *d, int input_timeout) {
    pa_usec_t now;
    const char *s = NULL;
    uint32_t timeout = d->userdata->timeout;

    pa_assert(d);
    pa_assert(d->sink || d->source);

    d->last_use = now = pa_rtclock_now();

    s = pa_proplist_gets(d->sink ? d->sink->proplist : d->source->proplist, "module-suspend-on-idle.timeout");
    if (!s || pa_atou(s, &timeout) < 0) {
        if (input_timeout >= 0)
            timeout = (uint32_t)input_timeout;
    }

#ifdef __TIZEN__
    /* Assume that timeout is milli seconds unit if large (>=100) enough */
    if (timeout >= 100) {
        pa_core_rttime_restart(d->userdata->core, d->time_event, now + timeout * PA_USEC_PER_MSEC);

        if (d->sink)
            pa_log_debug_verbose("Sink %s becomes idle, timeout in %u msec.", d->sink->name, timeout);
        if (d->source)
            pa_log_debug_verbose("Source %s becomes idle, timeout in %u msec.", d->source->name, timeout);
    } else {
        pa_core_rttime_restart(d->userdata->core, d->time_event, now + timeout * PA_USEC_PER_SEC);

        if (d->sink)
            pa_log_debug_verbose("Sink %s becomes idle, timeout in %u seconds.", d->sink->name, timeout);
        if (d->source)
            pa_log_debug_verbose("Source %s becomes idle, timeout in %u seconds.", d->source->name, timeout);
    }
#else
    pa_core_rttime_restart(d->userdata->core, d->time_event, now + timeout * PA_USEC_PER_SEC);

    if (d->sink)
        pa_log_debug_verbose("Sink %s becomes idle, timeout in %u seconds.", d->sink->name, timeout);
    if (d->source)
        pa_log_debug_verbose("Source %s becomes idle, timeout in %u seconds.", d->source->name, timeout);
#endif
}

static void resume(struct device_info *d) {
    int ret = 0;

    pa_assert(d);

    d->userdata->core->mainloop->time_restart(d->time_event, NULL);

    if (d->sink) {
#ifdef USE_PM_LOCK
        update_pm_status(d->userdata, PM_SINK, d->sink->index, PM_RESUME);
#endif /* USE_PM_LOCK */

        ret = pa_sink_suspend(d->sink, FALSE, PA_SUSPEND_IDLE);
        if (ret < 0) {
            pa_log_error("pa_sink_suspend(IDLE) sink:%d return error:%d", d->sink->index, ret);
#ifdef USE_PM_LOCK
            update_pm_status(d->userdata, PM_SINK, d->sink->index, PM_SUSPEND);
#endif /* USE_PM_LOCK */
        } else {
            pa_log_debug_verbose("Sink %s becomes busy.", d->sink->name);
        }
    }

    if (d->source) {
#ifdef USE_PM_LOCK
        update_pm_status(d->userdata, PM_SOURCE, d->source->index, PM_RESUME);
#endif /* USE_PM_LOCK */

        ret = pa_source_suspend(d->source, FALSE, PA_SUSPEND_IDLE);
        if (ret < 0) {
            pa_log_error("pa_source_suspend(IDLE) source:%d return error:%d", d->source->index, ret);
#ifdef USE_PM_LOCK
            update_pm_status(d->userdata, PM_SOURCE, d->source->index, PM_SUSPEND);
#endif /* USE_PM_LOCK */
        } else {
            pa_log_debug_verbose("Source %s becomes busy.", d->source->name);
        }
    }
}

static pa_hook_result_t sink_input_fixate_hook_cb(pa_core *c, pa_sink_input_new_data *data, struct userdata *u) {
    struct device_info *d;

    pa_assert(c);
    pa_assert(data);
    pa_assert(u);

    /* We need to resume the audio device here even for
     * PA_SINK_INPUT_START_CORKED, since we need the device parameters
     * to be fully available while the stream is set up.
     *in that case,make sure we close the sink again after the timeout interval*/

    if ((d = pa_hashmap_get(u->device_infos, data->sink))) {
        resume(d);
#ifdef __TIZEN__
        if (pa_sink_check_suspend(d->sink) <= 0)
            restart(d,USE_DEVICE_DEFAULT_TIMEOUT);
#endif
    }

    return PA_HOOK_OK;
}

static pa_hook_result_t source_output_fixate_hook_cb(pa_core *c, pa_source_output_new_data *data, struct userdata *u) {
    struct device_info *d;

    pa_assert(c);
    pa_assert(data);
    pa_assert(u);

    if (data->source->monitor_of)
        d = pa_hashmap_get(u->device_infos, data->source->monitor_of);
    else
        d = pa_hashmap_get(u->device_infos, data->source);

    if (d) {
        resume(d);
#ifdef __TIZEN__
        if (d->source) {
            if (pa_source_check_suspend(d->source) <= 0)
                restart(d,USE_DEVICE_DEFAULT_TIMEOUT);
        } else {
            /* The source output is connected to a monitor source. */
            pa_assert(d->sink);
            if (pa_sink_check_suspend(d->sink) <= 0)
                restart(d,USE_DEVICE_DEFAULT_TIMEOUT);
        }
#endif
    }

    return PA_HOOK_OK;
}

static pa_hook_result_t sink_input_unlink_hook_cb(pa_core *c, pa_sink_input *s, struct userdata *u) {
    pa_assert(c);
    pa_sink_input_assert_ref(s);
    pa_assert(u);

    if (!s->sink)
        return PA_HOOK_OK;

    if (pa_sink_check_suspend(s->sink) <= 0) {
        struct device_info *d;
        if ((d = pa_hashmap_get(u->device_infos, s->sink)))
            restart(d, USE_DEVICE_DEFAULT_TIMEOUT);
    }

    return PA_HOOK_OK;
}

static pa_hook_result_t source_output_unlink_hook_cb(pa_core *c, pa_source_output *s, struct userdata *u) {
    struct device_info *d = NULL;
    int n_source_output = 0;
    int timeout = USE_DEVICE_DEFAULT_TIMEOUT;
    const int nothing = 0;

    pa_assert(c);
    pa_source_output_assert_ref(s);
    pa_assert(u);

    if (!s->source)
        return PA_HOOK_OK;

    if (s->source->monitor_of) {
        if (pa_sink_check_suspend(s->source->monitor_of) <= 0)
            d = pa_hashmap_get(u->device_infos, s->source->monitor_of);
    } else {
        if (pa_source_check_suspend(s->source) <= 0)
            d = pa_hashmap_get(u->device_infos, s->source);
    }

    n_source_output = pa_source_linked_by(s->source);
    if(n_source_output == nothing) {
        timeout = 0; // set timeout 0, should be called immediately.
        pa_log_error("source outputs does't exist anymore. enter suspend state. name(%s), count(%d)",
            s->source->name, n_source_output);
    }

    if (d)
        restart(d, timeout);

    return PA_HOOK_OK;
}

static pa_hook_result_t sink_input_move_start_hook_cb(pa_core *c, pa_sink_input *s, struct userdata *u) {
    struct device_info *d;

    pa_assert(c);
    pa_sink_input_assert_ref(s);
    pa_assert(u);

    if (pa_sink_check_suspend(s->sink) <= 1)
        if ((d = pa_hashmap_get(u->device_infos, s->sink)))
            restart(d, USE_DEVICE_DEFAULT_TIMEOUT);

    return PA_HOOK_OK;
}

static pa_hook_result_t sink_input_move_finish_hook_cb(pa_core *c, pa_sink_input *s, struct userdata *u) {
    struct device_info *d;
    pa_sink_input_state_t state;

    pa_assert(c);
    pa_sink_input_assert_ref(s);
    pa_assert(u);

    state = pa_sink_input_get_state(s);
    if (state != PA_SINK_INPUT_RUNNING && state != PA_SINK_INPUT_DRAINED)
        return PA_HOOK_OK;

    if ((d = pa_hashmap_get(u->device_infos, s->sink)))
        resume(d);

    return PA_HOOK_OK;
}

static pa_hook_result_t source_output_move_start_hook_cb(pa_core *c, pa_source_output *s, struct userdata *u) {
    struct device_info *d = NULL;

    pa_assert(c);
    pa_source_output_assert_ref(s);
    pa_assert(u);

    if (s->source->monitor_of) {
        if (pa_sink_check_suspend(s->source->monitor_of) <= 1)
            d = pa_hashmap_get(u->device_infos, s->source->monitor_of);
    } else {
        if (pa_source_check_suspend(s->source) <= 1)
            d = pa_hashmap_get(u->device_infos, s->source);
    }

    if (d)
        restart(d, USE_DEVICE_DEFAULT_TIMEOUT);

    return PA_HOOK_OK;
}

static pa_hook_result_t source_output_move_finish_hook_cb(pa_core *c, pa_source_output *s, struct userdata *u) {
    struct device_info *d;

    pa_assert(c);
    pa_source_output_assert_ref(s);
    pa_assert(u);

    if (pa_source_output_get_state(s) != PA_SOURCE_OUTPUT_RUNNING)
        return PA_HOOK_OK;

    if (s->source->monitor_of)
        d = pa_hashmap_get(u->device_infos, s->source->monitor_of);
    else
        d = pa_hashmap_get(u->device_infos, s->source);

    if (d)
        resume(d);

    return PA_HOOK_OK;
}

static pa_hook_result_t sink_input_state_changed_hook_cb(pa_core *c, pa_sink_input *s, struct userdata *u) {
    struct device_info *d;
    pa_sink_input_state_t state;

    pa_assert(c);
    pa_sink_input_assert_ref(s);
    pa_assert(u);

    state = pa_sink_input_get_state(s);
    if (state == PA_SINK_INPUT_RUNNING || state == PA_SINK_INPUT_DRAINED)
        if ((d = pa_hashmap_get(u->device_infos, s->sink)))
            resume(d);

    return PA_HOOK_OK;
}

static pa_hook_result_t source_output_state_changed_hook_cb(pa_core *c, pa_source_output *s, struct userdata *u) {
    pa_assert(c);
    pa_source_output_assert_ref(s);
    pa_assert(u);

    if (pa_source_output_get_state(s) == PA_SOURCE_OUTPUT_RUNNING) {
        struct device_info *d;

        if (s->source->monitor_of)
            d = pa_hashmap_get(u->device_infos, s->source->monitor_of);
        else
            d = pa_hashmap_get(u->device_infos, s->source);

        if (d)
            resume(d);
    }

    return PA_HOOK_OK;
}

static pa_hook_result_t device_new_hook_cb(pa_core *c, pa_object *o, struct userdata *u) {
    struct device_info *d;
    pa_source *source;
    pa_sink *sink;

    pa_assert(c);
    pa_object_assert_ref(o);
    pa_assert(u);

    source = pa_source_isinstance(o) ? PA_SOURCE(o) : NULL;
    sink = pa_sink_isinstance(o) ? PA_SINK(o) : NULL;

    /* Never suspend monitors */
    if (source && source->monitor_of)
        return PA_HOOK_OK;

    pa_assert(source || sink);

    d = pa_xnew(struct device_info, 1);
    d->userdata = u;
    d->source = source ? pa_source_ref(source) : NULL;
    d->sink = sink ? pa_sink_ref(sink) : NULL;
    d->time_event = pa_core_rttime_new(c, PA_USEC_INVALID, timeout_cb, d);
    pa_hashmap_put(u->device_infos, o, d);

    if ((d->sink && pa_sink_check_suspend(d->sink) <= 0) ||
        (d->source && pa_source_check_suspend(d->source) <= 0))
        restart(d, USE_DEVICE_DEFAULT_TIMEOUT);

    return PA_HOOK_OK;
}

static void device_info_free(struct device_info *d) {
    pa_assert(d);

    if (d->source)
        pa_source_unref(d->source);
    if (d->sink)
        pa_sink_unref(d->sink);

    d->userdata->core->mainloop->time_free(d->time_event);

    pa_xfree(d);
}

static pa_hook_result_t device_unlink_hook_cb(pa_core *c, pa_object *o, struct userdata *u) {
    struct device_info *d;
    pa_sink *sink = NULL;
    pa_source *source = NULL;

    pa_assert(c);
    pa_object_assert_ref(o);
    pa_assert(u);

    if (pa_sink_isinstance(o)) {
        sink = PA_SINK(o);
        pa_log_info ("sink [%p][%d] is unlinked, now update pm", sink, (sink)? sink->index : -1);
        update_pm_status(u, PM_SINK, sink->index, PM_SUSPEND);
    } else if (pa_source_isinstance(o)) {
        source = PA_SOURCE(o);
        pa_log_info ("source [%p][%d] is unlinked, now update pm", source, (source)? source->index : -1);
        update_pm_status(u, PM_SOURCE, source->index, PM_SUSPEND);
    }

    if ((d = pa_hashmap_remove(u->device_infos, o)))
        device_info_free(d);

    return PA_HOOK_OK;
}

static pa_hook_result_t device_state_changed_hook_cb(pa_core *c, pa_object *o, struct userdata *u) {
    struct device_info *d;

    pa_assert(c);
    pa_object_assert_ref(o);
    pa_assert(u);

    if (!(d = pa_hashmap_get(u->device_infos, o)))
        return PA_HOOK_OK;

    if (pa_sink_isinstance(o)) {
        pa_sink *s = PA_SINK(o);
        pa_sink_state_t state = pa_sink_get_state(s);

#ifdef USE_PM_LOCK
        if(state == PA_SINK_SUSPENDED && (s->suspend_cause & PA_SUSPEND_USER))
            update_pm_status(d->userdata, PM_SINK, d->sink->index, PM_SUSPEND);
#endif
        if (pa_sink_check_suspend(s) <= 0)
            if (PA_SINK_IS_OPENED(state))
                restart(d, USE_DEVICE_DEFAULT_TIMEOUT);

    } else if (pa_source_isinstance(o)) {
        pa_source *s = PA_SOURCE(o);
        pa_source_state_t state = pa_source_get_state(s);

        if (pa_source_check_suspend(s) <= 0)
            if (PA_SOURCE_IS_OPENED(state))
                restart(d, USE_DEVICE_DEFAULT_TIMEOUT);
    }

    return PA_HOOK_OK;
}

int pa__init(pa_module*m) {
    pa_modargs *ma = NULL;
    struct userdata *u;
    uint32_t timeout = 5;
    uint32_t idx;
    pa_sink *sink;
    pa_source *source;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log("Failed to parse module arguments.");
        goto fail;
    }

    if (pa_modargs_get_value_u32(ma, "timeout", &timeout) < 0) {
        pa_log("Failed to parse timeout value.");
        goto fail;
    }

    m->userdata = u = pa_xnew(struct userdata, 1);
    u->core = m->core;
    u->timeout = timeout;
    u->device_infos = pa_hashmap_new(pa_idxset_trivial_hash_func, pa_idxset_trivial_compare_func);
#ifdef USE_PM_LOCK
    PA_LLIST_HEAD_INIT(pa_pm_list, u->pm_list);
#endif /* USE_PM_LOCK */

    for (sink = pa_idxset_first(m->core->sinks, &idx); sink; sink = pa_idxset_next(m->core->sinks, &idx))
        device_new_hook_cb(m->core, PA_OBJECT(sink), u);

    for (source = pa_idxset_first(m->core->sources, &idx); source; source = pa_idxset_next(m->core->sources, &idx))
        device_new_hook_cb(m->core, PA_OBJECT(source), u);

    u->sink_new_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_PUT], PA_HOOK_NORMAL, (pa_hook_cb_t) device_new_hook_cb, u);
    u->source_new_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_PUT], PA_HOOK_NORMAL, (pa_hook_cb_t) device_new_hook_cb, u);
    u->sink_unlink_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_UNLINK_POST], PA_HOOK_NORMAL, (pa_hook_cb_t) device_unlink_hook_cb, u);
    u->source_unlink_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_UNLINK_POST], PA_HOOK_NORMAL, (pa_hook_cb_t) device_unlink_hook_cb, u);
    u->sink_state_changed_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_STATE_CHANGED], PA_HOOK_NORMAL, (pa_hook_cb_t) device_state_changed_hook_cb, u);
    u->source_state_changed_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_STATE_CHANGED], PA_HOOK_NORMAL, (pa_hook_cb_t) device_state_changed_hook_cb, u);

    u->sink_input_new_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_INPUT_FIXATE], PA_HOOK_NORMAL, (pa_hook_cb_t) sink_input_fixate_hook_cb, u);
    u->source_output_new_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_FIXATE], PA_HOOK_NORMAL, (pa_hook_cb_t) source_output_fixate_hook_cb, u);
    u->sink_input_unlink_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_INPUT_UNLINK_POST], PA_HOOK_NORMAL, (pa_hook_cb_t) sink_input_unlink_hook_cb, u);
    u->source_output_unlink_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_UNLINK_POST], PA_HOOK_NORMAL, (pa_hook_cb_t) source_output_unlink_hook_cb, u);
    u->sink_input_move_start_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_INPUT_MOVE_START], PA_HOOK_NORMAL, (pa_hook_cb_t) sink_input_move_start_hook_cb, u);
    u->source_output_move_start_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_MOVE_START], PA_HOOK_NORMAL, (pa_hook_cb_t) source_output_move_start_hook_cb, u);
    u->sink_input_move_finish_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_INPUT_MOVE_FINISH], PA_HOOK_NORMAL, (pa_hook_cb_t) sink_input_move_finish_hook_cb, u);
    u->source_output_move_finish_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_MOVE_FINISH], PA_HOOK_NORMAL, (pa_hook_cb_t) source_output_move_finish_hook_cb, u);
    u->sink_input_state_changed_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SINK_INPUT_STATE_CHANGED], PA_HOOK_NORMAL, (pa_hook_cb_t) sink_input_state_changed_hook_cb, u);
    u->source_output_state_changed_slot = pa_hook_connect(&m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_STATE_CHANGED], PA_HOOK_NORMAL, (pa_hook_cb_t) source_output_state_changed_hook_cb, u);

#ifdef USE_PM_LOCK
    u->pm_mutex = pa_mutex_new(FALSE, FALSE);
    u->is_pm_locked = FALSE;
#endif

    pa_modargs_free(ma);
    return 0;

fail:

    if (ma)
        pa_modargs_free(ma);

    return -1;
}

void pa__done(pa_module*m) {
    struct userdata *u;
    struct device_info *d;

    pa_assert(m);

    if (!m->userdata)
        return;

    u = m->userdata;

    if (u->sink_new_slot)
        pa_hook_slot_free(u->sink_new_slot);
    if (u->sink_unlink_slot)
        pa_hook_slot_free(u->sink_unlink_slot);
    if (u->sink_state_changed_slot)
        pa_hook_slot_free(u->sink_state_changed_slot);

    if (u->source_new_slot)
        pa_hook_slot_free(u->source_new_slot);
    if (u->source_unlink_slot)
        pa_hook_slot_free(u->source_unlink_slot);
    if (u->source_state_changed_slot)
        pa_hook_slot_free(u->source_state_changed_slot);

    if (u->sink_input_new_slot)
        pa_hook_slot_free(u->sink_input_new_slot);
    if (u->sink_input_unlink_slot)
        pa_hook_slot_free(u->sink_input_unlink_slot);
    if (u->sink_input_move_start_slot)
        pa_hook_slot_free(u->sink_input_move_start_slot);
    if (u->sink_input_move_finish_slot)
        pa_hook_slot_free(u->sink_input_move_finish_slot);
    if (u->sink_input_state_changed_slot)
        pa_hook_slot_free(u->sink_input_state_changed_slot);

    if (u->source_output_new_slot)
        pa_hook_slot_free(u->source_output_new_slot);
    if (u->source_output_unlink_slot)
        pa_hook_slot_free(u->source_output_unlink_slot);
    if (u->source_output_move_start_slot)
        pa_hook_slot_free(u->source_output_move_start_slot);
    if (u->source_output_move_finish_slot)
        pa_hook_slot_free(u->source_output_move_finish_slot);
    if (u->source_output_state_changed_slot)
        pa_hook_slot_free(u->source_output_state_changed_slot);

    while ((d = pa_hashmap_steal_first(u->device_infos)))
        device_info_free(d);

    pa_hashmap_free(u->device_infos, NULL);

#ifdef USE_PM_LOCK
    if (u->pm_mutex) {
        pa_mutex_free(u->pm_mutex);
        u->pm_mutex = NULL;
    }
#endif
    pa_xfree(u);
}
