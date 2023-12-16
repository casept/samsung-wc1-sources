/***
  This file is part of PulseAudio.

  Copyright 2005-2006 Lennart Poettering

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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <pulse/xmalloc.h>

#include <pulsecore/module.h>
#include <pulsecore/log.h>
#include <pulsecore/namereg.h>
#include <pulsecore/sink.h>
#include <pulsecore/modargs.h>
#include <pulsecore/macro.h>
#ifdef HAVE_DBUS
#include <pulsecore/protocol-dbus.h>
#include <pulsecore/dbus-util.h>
#endif

#include "module-stream-mgr-symdef.h"
#include "tizen-audio.h"

PA_MODULE_AUTHOR("Sangchul Lee");
PA_MODULE_DESCRIPTION("Stream Manager module using IPC");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(TRUE);
PA_MODULE_USAGE("ipc_type=<pipe or dbus>");

static void io_event_callback(pa_mainloop_api *io, pa_io_event *e, int fd, pa_io_event_flags_t events, void*userdata);

static const char* const valid_modargs[] = {
    "ipc_type",
    NULL,
};

struct userdata {
    int fd;
    pa_io_event *io;
    pa_module *module;
    pa_usec_t before;
    pa_usec_t now;
#ifdef HAVE_DBUS
    pa_dbus_protocol *dbus_protocol;
#endif
};

#define FILE_FULL_PATH 1024        /* File path lenth */

struct ipc_data {
    char filename[FILE_FULL_PATH];
    int volume_config;
};

/* FIXME: jcsing temporary */
#define KEYTONE_PATH        "/tmp/keytone_stream_mgr"  /* Keytone pipe path */
#define KEYTONE_GROUP       6526            /* Keytone group : assigned by security */
#define DEFAULT_IPC_TYPE    IPC_TYPE_PIPE
#define AUDIO_VOLUME_CONFIG_TYPE(vol) (vol & 0x00FF)
#define AUDIO_VOLUME_CONFIG_GAIN(vol) (vol & 0xFF00)

static int init_ipc (struct userdata *u, const char *type) {

    pa_assert(u);
    pa_assert(type);

    pa_log_info("Initialization for IPC, type:[%s]", type);

    if(!strncmp(type, "pipe", 4)) {
        int pre_mask;

        pre_mask = umask(0);
        if (mknod(KEYTONE_PATH,S_IFIFO|0660,0)<0) {
            pa_log_warn("mknod failed. errno=[%d][%s]", errno, strerror(errno));
        }
        umask(pre_mask);

        u->fd = open(KEYTONE_PATH, O_RDWR);
        if (u->fd == -1) {
            pa_log_warn("Check ipc node %s\n", KEYTONE_PATH);
            return -1;
        }

        /* change access mode so group can use keytone pipe */
        if (fchmod (u->fd, 0666) == -1) {
            pa_log_warn("Changing keytone access mode is failed. errno=[%d][%s]", errno, strerror(errno));
        }

        /* change group due to security request */
        if (fchown (u->fd, -1, KEYTONE_GROUP) == -1) {
            pa_log_warn("Changing keytone group is failed. errno=[%d][%s]", errno, strerror(errno));
        }

        u->io = u->module->core->mainloop->io_new(u->module->core->mainloop, u->fd, PA_IO_EVENT_INPUT|PA_IO_EVENT_HANGUP, io_event_callback, u);
        pa_rtclock_now_args(&u->before);
        pa_rtclock_now_args(&u->now);
    } else if (!strncmp(type, "dbus", 4)) {
        /* NOTE : need implementation for DBUS work */
#ifdef HAVE_DBUS
        /* TODO : need to add codes for initialization of dbus */
#else
        pa_log_error("DBUS is not supported\n");
        return -1;
#endif

    } else {
        pa_log_error("Unknown type(%s) for IPC", type);
        return -1;
    }

    return 0;
}

static void deinit_ipc (struct userdata *u) {

    pa_assert(u);

    if (u->io)
        u->module->core->mainloop->io_free(u->io);

    if (u->fd > -1)
        close(u->fd);

#ifdef HAVE_DBUS
    /* TODO : need to add codes for de-initialization of dbus */
#endif
}

#define MAX_GAIN_TYPE  4
static const char* get_str_gain_type[MAX_GAIN_TYPE] =
{
    "GAIN_TYPE_DEFAULT",
    "GAIN_TYPE_DIALER",
    "GAIN_TYPE_TOUCH",
    "GAIN_TYPE_OTHERS"
};

#define MAX_NAME_LEN 256
static int _simple_play(struct userdata *u, const char *file_path, uint32_t volume_config) {
    int ret = 0;
    pa_sink *sink = NULL;
    pa_proplist *p;
    const char *name_prefix = "SIMPLE_PLAY";
    double volume_linear = 1.0f;
    int volume_type =  AUDIO_VOLUME_CONFIG_TYPE(volume_config);
    int volume_gain =  AUDIO_VOLUME_CONFIG_GAIN(volume_config)>>8;
    char name[MAX_NAME_LEN] = {0};

    uint32_t stream_idx;
    uint32_t play_idx = 0;

    p = pa_proplist_new();

    /* Set volume type of stream */
    pa_proplist_setf(p, PA_PROP_MEDIA_TIZEN_VOLUME_TYPE, "%d", volume_type);
    /* Set gain type of stream */
    pa_proplist_setf(p, PA_PROP_MEDIA_TIZEN_GAIN_TYPE, "%d", volume_gain);
    /* Set policy type of stream */
    pa_proplist_sets(p, PA_PROP_MEDIA_POLICY, "auto");
    /* Set policy for selecting sink */
    pa_proplist_sets(p, PA_PROP_MEDIA_POLICY_IGNORE_PRESET_SINK, "yes");
    sink = pa_namereg_get_default_sink(u->module->core);

    pa_log_debug_verbose("volume_config[type:0x%x,gain:0x%x[%s]]", volume_type, volume_gain,
                 volume_gain > (MAX_GAIN_TYPE-2) ? get_str_gain_type[MAX_GAIN_TYPE-1]: get_str_gain_type[volume_gain]);
    snprintf(name, sizeof(name)-1, "%s_%s", name_prefix, file_path);
    play_idx = pa_scache_get_id_by_name(u->module->core, name);
    if (play_idx != PA_IDXSET_INVALID) {
        pa_log_debug_verbose("found cached index [%u] for name [%s]", play_idx, file_path);
    } else {
        if ((ret = pa_scache_add_file_lazy(u->module->core, name, file_path, &play_idx)) != 0) {
            pa_log_error("failed to add file [%s]", file_path);
            goto exit;
        } else {
            pa_log_debug_verbose("success to add file [%s], index [%u]", file_path, play_idx);
        }
    }

    pa_log_debug_verbose("pa_scache_play_item() start");
    if ((ret = pa_scache_play_item(u->module->core, name, sink, PA_VOLUME_NORM, p, &stream_idx) < 0)) {
        pa_log_error("pa_scache_play_item fail");
        goto exit;
    }
    pa_log_debug_verbose("pa_scache_play_item() end");

exit:
    pa_proplist_free(p);
    return ret;
}


static void io_event_callback(pa_mainloop_api *io, pa_io_event *e, int fd, pa_io_event_flags_t events, void*userdata) {
    struct userdata *u = userdata;
    struct ipc_data data;
    int ret = 0;
    int size = 0;

    pa_usec_t diff = 0ULL;
    int gain = 0;

    pa_assert(io);
    pa_assert(u);

    if (events & (PA_IO_EVENT_HANGUP|PA_IO_EVENT_ERROR)) {
        pa_log_warn("Lost connection to client side");
        goto fail;
    }

    if (events & PA_IO_EVENT_INPUT) {
        size = sizeof(data);
        memset(&data, 0, size);
        ret = read(fd, (void *)&data, size);
        if(ret != -1) {
            gain = AUDIO_VOLUME_CONFIG_GAIN(data.volume_config)>>8;
            pa_rtclock_now_args(&u->now);
            diff = u->now - u->before;
            u->before = u->now;

            /* volume is increased sometime if client request to play tone quickly. 25ms*/
            if(gain == AUDIO_GAIN_TYPE_TOUCH) {
                pa_source* s = NULL;
                if(25000ULL > diff) {
                    pa_log_info("skip playing keytone. becuase client requested to play tone quickly.");
                    return;
                }

                s = pa_namereg_get_default_source(u->module->core);
                if(s != NULL && pa_source_check_suspend(s) > 0) {
                    pa_log_info("skip playing keytone. becuase of recording");
                    return;
                }
            }

            pa_log_info("name(%s), volume_config(0x%x), diff(%llu) from the event", data.filename, data.volume_config, diff);
            _simple_play(u, data.filename, data.volume_config);

        } else {
            pa_log_warn("Fail to read file");
        }
    }

    return;

fail:
    u->module->core->mainloop->io_free(u->io);
    u->io = NULL;

    pa_module_unload_request(u->module, TRUE);
}


int pa__init(pa_module*m) {
    struct userdata *u;
    pa_modargs *ma = NULL;
    const char *ipc_type = NULL;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log("Failed to parse module arguments");
        goto fail;
    }

    ipc_type = pa_modargs_get_value(ma, "ipc_type", "pipe");

    m->userdata = u = pa_xnew(struct userdata, 1);
    u->module = m;
    u->io = NULL;
    u->fd = -1;

    if (init_ipc(u, ipc_type))
        goto fail;

    pa_modargs_free(ma);

    return 0;

fail:

    pa_modargs_free(ma);
    pa__done(m);
    return -1;
}


void pa__done(pa_module*m) {
    struct userdata *u;
    pa_assert(m);

    if (!(u = m->userdata))
        return;

    deinit_ipc(u);

    pa_xfree(u);
}
