#ifndef foopulsesourceoutputhfoo
#define foopulsesourceoutputhfoo

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering

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

#include <inttypes.h>

typedef struct pa_source_output pa_source_output;

#include <pulse/sample.h>
#include <pulse/format.h>
#include <pulsecore/memblockq.h>
#include <pulsecore/resampler.h>
#include <pulsecore/module.h>
#include <pulsecore/client.h>
#include <pulsecore/source.h>
#include <pulsecore/core.h>
#include <pulsecore/sink-input.h>

typedef enum pa_source_output_state {
    PA_SOURCE_OUTPUT_INIT,
    PA_SOURCE_OUTPUT_RUNNING,
    PA_SOURCE_OUTPUT_CORKED,
    PA_SOURCE_OUTPUT_UNLINKED
} pa_source_output_state_t;

static inline pa_bool_t PA_SOURCE_OUTPUT_IS_LINKED(pa_source_output_state_t x) {
    return x == PA_SOURCE_OUTPUT_RUNNING || x == PA_SOURCE_OUTPUT_CORKED;
}

typedef enum pa_source_output_flags {
    PA_SOURCE_OUTPUT_VARIABLE_RATE = 1,
    PA_SOURCE_OUTPUT_DONT_MOVE = 2,
    PA_SOURCE_OUTPUT_START_CORKED = 4,
    PA_SOURCE_OUTPUT_NO_REMAP = 8,
    PA_SOURCE_OUTPUT_NO_REMIX = 16,
    PA_SOURCE_OUTPUT_FIX_FORMAT = 32,
    PA_SOURCE_OUTPUT_FIX_RATE = 64,
    PA_SOURCE_OUTPUT_FIX_CHANNELS = 128,
    PA_SOURCE_OUTPUT_DONT_INHIBIT_AUTO_SUSPEND = 256,
    PA_SOURCE_OUTPUT_NO_CREATE_ON_SUSPEND = 512,
    PA_SOURCE_OUTPUT_KILL_ON_SUSPEND = 1024,
    PA_SOURCE_OUTPUT_PASSTHROUGH = 2048
} pa_source_output_flags_t;

struct pa_source_output {
    pa_msgobject parent;

    uint32_t index;
    pa_core *core;

    pa_source_output_state_t state;
    pa_source_output_flags_t flags;

    char *driver;                         /* may be NULL */
    pa_proplist *proplist;

    pa_module *module;                    /* may be NULL */
    pa_client *client;                    /* may be NULL */

    pa_source *source;                    /* NULL while being moved */
    pa_source *destination_source;        /* only set by filter sources */

    /* A source output can monitor just a single input of a sink, in which case we find it here */
    pa_sink_input *direct_on_input;       /* may be NULL */

    pa_sample_spec sample_spec;
    pa_channel_map channel_map;
    pa_format_info *format;

    /* Also see http://pulseaudio.org/wiki/InternalVolumes */
    pa_cvolume volume;             /* The volume clients are informed about */
    pa_cvolume reference_ratio;    /* The ratio of the stream's volume to the source's reference volume */
    pa_cvolume real_ratio;         /* The ratio of the stream's volume to the source's real volume */
    pa_cvolume volume_factor;      /* An internally used volume factor that can be used by modules to apply effects and suchlike without having that visible to the outside */
    pa_cvolume soft_volume;        /* The internal software volume we apply to all PCM data while it passes through. Usually calculated as real_ratio * volume_factor */

    pa_cvolume volume_factor_source; /* A second volume factor in format of the source this stream is connected to */

    pa_bool_t volume_writable:1;

    pa_bool_t muted:1;

    /* if TRUE then the source we are connected to and/or the volume
     * set is worth remembering, i.e. was explicitly chosen by the
     * user and not automatically. module-stream-restore looks for
     * this.*/
    pa_bool_t save_source:1, save_volume:1, save_muted:1;

    pa_resample_method_t requested_resample_method, actual_resample_method;

    /* Pushes a new memchunk into the output. Called from IO thread
     * context. */
    void (*push)(pa_source_output *o, const pa_memchunk *chunk); /* may NOT be NULL */

    /* Only relevant for monitor sources right now: called when the
     * recorded stream is rewound. Called from IO context */
    void (*process_rewind)(pa_source_output *o, size_t nbytes); /* may be NULL */

    /* Called whenever the maximum rewindable size of the source
     * changes. Called from IO thread context. */
    void (*update_max_rewind) (pa_source_output *o, size_t nbytes); /* may be NULL */

    /* Called whenever the configured latency of the source
     * changes. Called from IO context. */
    void (*update_source_requested_latency) (pa_source_output *o); /* may be NULL */

    /* Called whenever the latency range of the source changes. Called
     * from IO context. */
    void (*update_source_latency_range) (pa_source_output *o); /* may be NULL */

    /* Called whenever the fixed latency of the source changes, if there
     * is one. Called from IO context. */
    void (*update_source_fixed_latency) (pa_source_output *i); /* may be NULL */

    /* If non-NULL this function is called when the output is first
     * connected to a source or when the rtpoll/asyncmsgq fields
     * change. You usually don't need to implement this function
     * unless you rewrite a source that is piggy-backed onto
     * another. Called from IO thread context */
    void (*attach) (pa_source_output *o);           /* may be NULL */

    /* If non-NULL this function is called when the output is
     * disconnected from its source. Called from IO thread context */
    void (*detach) (pa_source_output *o);           /* may be NULL */

    /* If non-NULL called whenever the source this output is attached
     * to suspends or resumes. Called from main context */
    void (*suspend) (pa_source_output *o, pa_bool_t b);   /* may be NULL */

    /* If non-NULL called whenever the source this output is attached
     * to suspends or resumes. Called from IO context */
    void (*suspend_within_thread) (pa_source_output *o, pa_bool_t b);   /* may be NULL */

    /* If non-NULL called whenever the source output is moved to a new
     * source. Called from main context after the source output has been
     * detached from the old source and before it has been attached to
     * the new source. If dest is NULL the move was executed in two
     * phases and the second one failed; the stream will be destroyed
     * after this call. */
    void (*moving) (pa_source_output *o, pa_source *dest);   /* may be NULL */

    /* Supposed to unlink and destroy this stream. Called from main
     * context. */
    void (*kill)(pa_source_output* o);              /* may NOT be NULL */

    /* Return the current latency (i.e. length of buffered audio) of
    this stream. Called from main context. This is added to what the
    PA_SOURCE_OUTPUT_MESSAGE_GET_LATENCY message sent to the IO thread
    returns */
    pa_usec_t (*get_latency) (pa_source_output *o); /* may be NULL */

    /* If non-NULL this function is called from thread context if the
     * state changes. The old state is found in thread_info.state.  */
    void (*state_change) (pa_source_output *o, pa_source_output_state_t state); /* may be NULL */

    /* If non-NULL this function is called before this source output
     * is moved to a source and if it returns FALSE the move
     * will not be allowed */
    pa_bool_t (*may_move_to) (pa_source_output *o, pa_source *s); /* may be NULL */

    /* If non-NULL this function is used to dispatch asynchronous
     * control events. */
    void (*send_event)(pa_source_output *o, const char *event, pa_proplist* data);

    /* If non-NULL this function is called whenever the source output
     * volume changes. Called from main context */
    void (*volume_changed)(pa_source_output *o); /* may be NULL */

    /* If non-NULL this function is called whenever the source output
     * mute status changes. Called from main context */
    void (*mute_changed)(pa_source_output *o); /* may be NULL */

    struct {
        pa_source_output_state_t state;

        pa_cvolume soft_volume;
        pa_bool_t muted:1;

        pa_bool_t attached:1; /* True only between ->attach() and ->detach() calls */

        pa_sample_spec sample_spec;

        pa_resampler* resampler;              /* may be NULL */

        /* We maintain a delay memblockq here for source outputs that
         * don't implement rewind() */
        pa_memblockq *delay_memblockq;

        /* The requested latency for the source */
        pa_usec_t requested_source_latency;

        pa_sink_input *direct_on_input;       /* may be NULL */
    } thread_info;

    void *userdata;
#ifdef __TIZEN__    /* && ENABLE_PCM_DUMP */
    FILE *dump_fp;
    char *dump_path;
#endif
};

PA_DECLARE_PUBLIC_CLASS(pa_source_output);
#define PA_SOURCE_OUTPUT(o) pa_source_output_cast(o)

enum {
    PA_SOURCE_OUTPUT_MESSAGE_GET_LATENCY,
    PA_SOURCE_OUTPUT_MESSAGE_SET_RATE,
    PA_SOURCE_OUTPUT_MESSAGE_SET_STATE,
    PA_SOURCE_OUTPUT_MESSAGE_SET_REQUESTED_LATENCY,
    PA_SOURCE_OUTPUT_MESSAGE_GET_REQUESTED_LATENCY,
    PA_SOURCE_OUTPUT_MESSAGE_SET_SOFT_VOLUME,
    PA_SOURCE_OUTPUT_MESSAGE_SET_SOFT_MUTE,
    PA_SOURCE_OUTPUT_MESSAGE_MAX
};

typedef struct pa_source_output_send_event_hook_data {
    pa_source_output *source_output;
    const char *event;
    pa_proplist *data;
} pa_source_output_send_event_hook_data;

typedef struct pa_source_output_new_data {
    pa_source_output_flags_t flags;

    pa_proplist *proplist;
    pa_sink_input *direct_on_input;

    const char *driver;
    pa_module *module;
    pa_client *client;

    pa_source *source;
    pa_source *destination_source;

    pa_resample_method_t resample_method;

    pa_sample_spec sample_spec;
    pa_channel_map channel_map;
    pa_format_info *format;
    pa_idxset *req_formats;
    pa_idxset *nego_formats;

    pa_cvolume volume, volume_factor, volume_factor_source;
    pa_bool_t muted:1;

    pa_bool_t sample_spec_is_set:1;
    pa_bool_t channel_map_is_set:1;

    pa_bool_t volume_is_set:1, volume_factor_is_set:1, volume_factor_source_is_set:1;
    pa_bool_t muted_is_set:1;

    pa_bool_t volume_is_absolute:1;

    pa_bool_t volume_writable:1;

    pa_bool_t save_source:1, save_volume:1, save_muted:1;
} pa_source_output_new_data;

pa_source_output_new_data* pa_source_output_new_data_init(pa_source_output_new_data *data);
void pa_source_output_new_data_set_sample_spec(pa_source_output_new_data *data, const pa_sample_spec *spec);
void pa_source_output_new_data_set_channel_map(pa_source_output_new_data *data, const pa_channel_map *map);
pa_bool_t pa_source_output_new_data_is_passthrough(pa_source_output_new_data *data);
void pa_source_output_new_data_set_volume(pa_source_output_new_data *data, const pa_cvolume *volume);
void pa_source_output_new_data_apply_volume_factor(pa_source_output_new_data *data, const pa_cvolume *volume_factor);
void pa_source_output_new_data_apply_volume_factor_source(pa_source_output_new_data *data, const pa_cvolume *volume_factor);
void pa_source_output_new_data_set_muted(pa_source_output_new_data *data, pa_bool_t mute);
pa_bool_t pa_source_output_new_data_set_source(pa_source_output_new_data *data, pa_source *s, pa_bool_t save);
pa_bool_t pa_source_output_new_data_set_formats(pa_source_output_new_data *data, pa_idxset *formats);
void pa_source_output_new_data_done(pa_source_output_new_data *data);

/* To be called by the implementing module only */

int pa_source_output_new(
        pa_source_output**o,
        pa_core *core,
        pa_source_output_new_data *data);

void pa_source_output_put(pa_source_output *o);
void pa_source_output_unlink(pa_source_output*o);

void pa_source_output_set_name(pa_source_output *o, const char *name);

pa_usec_t pa_source_output_set_requested_latency(pa_source_output *o, pa_usec_t usec);

void pa_source_output_cork(pa_source_output *o, pa_bool_t b);

int pa_source_output_set_rate(pa_source_output *o, uint32_t rate);
int pa_source_output_update_rate(pa_source_output *o);

size_t pa_source_output_get_max_rewind(pa_source_output *o);

/* Callable by everyone */

/* External code may request disconnection with this function */
void pa_source_output_kill(pa_source_output*o);

pa_usec_t pa_source_output_get_latency(pa_source_output *o, pa_usec_t *source_latency);

pa_bool_t pa_source_output_is_volume_readable(pa_source_output *o);
pa_bool_t pa_source_output_is_passthrough(pa_source_output *o);
void pa_source_output_set_volume(pa_source_output *o, const pa_cvolume *volume, pa_bool_t save, pa_bool_t absolute);
pa_cvolume *pa_source_output_get_volume(pa_source_output *o, pa_cvolume *volume, pa_bool_t absolute);

void pa_source_output_set_mute(pa_source_output *o, pa_bool_t mute, pa_bool_t save);
pa_bool_t pa_source_output_get_mute(pa_source_output *o);

void pa_source_output_update_proplist(pa_source_output *o, pa_update_mode_t mode, pa_proplist *p);

pa_resample_method_t pa_source_output_get_resample_method(pa_source_output *o);

void pa_source_output_send_event(pa_source_output *o, const char *name, pa_proplist *data);

pa_bool_t pa_source_output_may_move(pa_source_output *o);
pa_bool_t pa_source_output_may_move_to(pa_source_output *o, pa_source *dest);
int pa_source_output_move_to(pa_source_output *o, pa_source *dest, pa_bool_t save);

/* The same as pa_source_output_move_to() but in two separate steps,
 * first the detaching from the old source, then the attaching to the
 * new source */
int pa_source_output_start_move(pa_source_output *o);
int pa_source_output_finish_move(pa_source_output *o, pa_source *dest, pa_bool_t save);
void pa_source_output_fail_move(pa_source_output *o);

#define pa_source_output_get_state(o) ((o)->state)

pa_usec_t pa_source_output_get_requested_latency(pa_source_output *o);

/* To be used exclusively by the source driver thread */

void pa_source_output_push(pa_source_output *o, const pa_memchunk *chunk);
void pa_source_output_process_rewind(pa_source_output *o, size_t nbytes);
void pa_source_output_update_max_rewind(pa_source_output *o, size_t nbytes);

void pa_source_output_set_state_within_thread(pa_source_output *o, pa_source_output_state_t state);

int pa_source_output_process_msg(pa_msgobject *mo, int code, void *userdata, int64_t offset, pa_memchunk *chunk);

pa_usec_t pa_source_output_set_requested_latency_within_thread(pa_source_output *o, pa_usec_t usec);

#define pa_source_output_assert_io_context(s) \
    pa_assert(pa_thread_mq_get() || !PA_SOURCE_OUTPUT_IS_LINKED((s)->state))

#endif
