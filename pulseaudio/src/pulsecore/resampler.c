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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#ifdef HAVE_SEC_SRC
#include <samsung_src.h>
#endif

#ifdef HAVE_LIBSAMPLERATE
#include <samplerate.h>
#endif

#ifdef HAVE_SPEEX
#include <speex/speex_resampler.h>
#endif

#include <pulse/xmalloc.h>
#include <pulsecore/sconv.h>
#include <pulsecore/log.h>
#include <pulsecore/macro.h>
#include <pulsecore/strbuf.h>
#include <pulsecore/remap.h>
#include <pulsecore/core-util.h>
#include "ffmpeg/avcodec.h"

#include "resampler.h"

/* Number of samples of extra space we allow the resamplers to return */
#define EXTRA_FRAMES 128

struct pa_resampler {
    pa_resample_method_t method;
    pa_resample_flags_t flags;

    pa_sample_spec i_ss, o_ss;
    pa_channel_map i_cm, o_cm;
    size_t i_fz, o_fz, w_sz;
    pa_mempool *mempool;

    pa_memchunk to_work_format_buf;
    pa_memchunk remap_buf;
    pa_memchunk resample_buf;
    pa_memchunk from_work_format_buf;
    unsigned to_work_format_buf_samples;
    size_t remap_buf_size;
    unsigned resample_buf_samples;
    unsigned from_work_format_buf_samples;
    bool remap_buf_contains_leftover_data;

    pa_sample_format_t work_format;
    uint8_t work_channels;

    pa_convert_func_t to_work_format_func;
    pa_convert_func_t from_work_format_func;

    pa_remap_t remap;
    bool map_required;

    void (*impl_free)(pa_resampler *r);
    void (*impl_update_rates)(pa_resampler *r);
    void (*impl_resample)(pa_resampler *r, const pa_memchunk *in, unsigned in_samples, pa_memchunk *out, unsigned *out_samples);
    void (*impl_reset)(pa_resampler *r);

    struct { /* data specific to the trivial resampler */
        unsigned o_counter;
        unsigned i_counter;
    } trivial;

    struct { /* data specific to the peak finder pseudo resampler */
        unsigned o_counter;
        unsigned i_counter;

        float max_f[PA_CHANNELS_MAX];
        int16_t max_i[PA_CHANNELS_MAX];

    } peaks;

#ifdef HAVE_LIBSAMPLERATE
    struct { /* data specific to libsamplerate */
        SRC_STATE *state;
    } src;
#endif

#ifdef HAVE_SPEEX
    struct { /* data specific to speex */
        SpeexResamplerState* state;
    } speex;
#endif

#ifdef HAVE_SEC_SRC
	struct { /* data specific to samsung src */
		SamsungSRCState *state;
	} secsrc;
#endif

    struct { /* data specific to ffmpeg */
        struct AVResampleContext *state;
        pa_memchunk buf[PA_CHANNELS_MAX];
    } ffmpeg;
};

static int copy_init(pa_resampler *r);
static int trivial_init(pa_resampler*r);
#ifdef HAVE_SPEEX
static int speex_init(pa_resampler*r);
#endif
static int ffmpeg_init(pa_resampler*r);
static int peaks_init(pa_resampler*r);
#ifdef HAVE_LIBSAMPLERATE
static int libsamplerate_init(pa_resampler*r);
#endif

#ifdef HAVE_SEC_SRC
static int secsrc_init(pa_resampler*r);
#endif

static void calc_map_table(pa_resampler *r);

static int (* const init_table[])(pa_resampler*r) = {
#ifdef HAVE_LIBSAMPLERATE
    [PA_RESAMPLER_SRC_SINC_BEST_QUALITY]   = libsamplerate_init,
    [PA_RESAMPLER_SRC_SINC_MEDIUM_QUALITY] = libsamplerate_init,
    [PA_RESAMPLER_SRC_SINC_FASTEST]        = libsamplerate_init,
    [PA_RESAMPLER_SRC_ZERO_ORDER_HOLD]     = libsamplerate_init,
    [PA_RESAMPLER_SRC_LINEAR]              = libsamplerate_init,
#else
    [PA_RESAMPLER_SRC_SINC_BEST_QUALITY]   = NULL,
    [PA_RESAMPLER_SRC_SINC_MEDIUM_QUALITY] = NULL,
    [PA_RESAMPLER_SRC_SINC_FASTEST]        = NULL,
    [PA_RESAMPLER_SRC_ZERO_ORDER_HOLD]     = NULL,
    [PA_RESAMPLER_SRC_LINEAR]              = NULL,
#endif
    [PA_RESAMPLER_TRIVIAL]                 = trivial_init,
#ifdef HAVE_SPEEX
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+0]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+1]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+2]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+3]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+4]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+5]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+6]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+7]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+8]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+9]      = speex_init,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+10]     = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+0]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+1]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+2]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+3]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+4]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+5]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+6]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+7]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+8]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+9]      = speex_init,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+10]     = speex_init,
#else
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+0]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+1]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+2]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+3]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+4]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+5]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+6]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+7]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+8]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+9]      = NULL,
    [PA_RESAMPLER_SPEEX_FLOAT_BASE+10]     = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+0]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+1]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+2]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+3]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+4]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+5]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+6]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+7]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+8]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+9]      = NULL,
    [PA_RESAMPLER_SPEEX_FIXED_BASE+10]     = NULL,
#endif
    [PA_RESAMPLER_FFMPEG]                  = ffmpeg_init,
#ifdef HAVE_SEC_SRC
    [PA_RESAMPLER_SECSRC]                  = secsrc_init,
#endif
    [PA_RESAMPLER_AUTO]                    = NULL,
    [PA_RESAMPLER_COPY]                    = copy_init,
    [PA_RESAMPLER_PEAKS]                   = peaks_init,
};

#ifdef HAVE_SEC_SRC
static pa_bool_t _check_secsrc_valid_rate (int rate)
{
	switch(rate)
	{
		 case 8000:
		 case 11025:
		 case 16000:
		 case 22050:
		 case 24000:
		 case 32000:
		 case 44100:
		 case 48000:
			 return TRUE;
		 default:
			 return FALSE;
	}
}
#endif

pa_resampler* pa_resampler_new(
        pa_mempool *pool,
        const pa_sample_spec *a,
        const pa_channel_map *am,
        const pa_sample_spec *b,
        const pa_channel_map *bm,
        pa_resample_method_t method,
        pa_resample_flags_t flags) {

    pa_resampler *r = NULL;

    pa_assert(pool);
    pa_assert(a);
    pa_assert(b);
    pa_assert(pa_sample_spec_valid(a));
    pa_assert(pa_sample_spec_valid(b));
    pa_assert(method >= 0);
    pa_assert(method < PA_RESAMPLER_MAX);

    /* Fix method */

    if (!(flags & PA_RESAMPLER_VARIABLE_RATE) && a->rate == b->rate) {
        pa_log_debug_verbose("Forcing resampler 'copy', because of fixed, identical sample rates.");
        method = PA_RESAMPLER_COPY;
    }

    if (!pa_resample_method_supported(method)) {
        pa_log_warn("Support for resampler '%s' not compiled in, reverting to 'auto'.", pa_resample_method_to_string(method));
        method = PA_RESAMPLER_AUTO;
    }

#ifdef HAVE_SEC_SRC
    if (method == PA_RESAMPLER_SECSRC) {
        if (_check_secsrc_valid_rate (a->rate) == FALSE) {
            pa_log_warn("Resampler 'secsrc' cannot change sampling rate [%d], reverting to resampler 'ffmpeg'.", a->rate);
            method = PA_RESAMPLER_FFMPEG;
        } else if (b->channels > 2) {
            pa_log_warn("Resampler 'secsrc' cannot handle [%d] channels, reverting to resampler 'ffmpeg'.", b->channels);
            method = PA_RESAMPLER_FFMPEG;
        }
    }
#endif


    if (method == PA_RESAMPLER_FFMPEG && (flags & PA_RESAMPLER_VARIABLE_RATE)) {
        pa_log_info("Resampler 'ffmpeg' cannot do variable rate, reverting to resampler 'auto'.");
        method = PA_RESAMPLER_AUTO;
    }

    if (method == PA_RESAMPLER_COPY && ((flags & PA_RESAMPLER_VARIABLE_RATE) || a->rate != b->rate)) {
        pa_log_info("Resampler 'copy' cannot change sampling rate, reverting to resampler 'auto'.");
        method = PA_RESAMPLER_AUTO;
    }

    if (method == PA_RESAMPLER_AUTO) {
#ifdef HAVE_SPEEX
        method = PA_RESAMPLER_SPEEX_FLOAT_BASE + 1;
#else
        if (flags & PA_RESAMPLER_VARIABLE_RATE)
            method = PA_RESAMPLER_TRIVIAL;
        else
            method = PA_RESAMPLER_FFMPEG;
#endif
    }

    r = pa_xnew0(pa_resampler, 1);
    r->mempool = pool;
    r->method = method;
    r->flags = flags;

    /* Fill sample specs */
    r->i_ss = *a;
    r->o_ss = *b;

    /* set up the remap structure */
    r->remap.i_ss = &r->i_ss;
    r->remap.o_ss = &r->o_ss;
    r->remap.format = &r->work_format;

    if (am)
        r->i_cm = *am;
    else if (!pa_channel_map_init_auto(&r->i_cm, r->i_ss.channels, PA_CHANNEL_MAP_DEFAULT))
        goto fail;

    if (bm)
        r->o_cm = *bm;
    else if (!pa_channel_map_init_auto(&r->o_cm, r->o_ss.channels, PA_CHANNEL_MAP_DEFAULT))
        goto fail;

    r->i_fz = pa_frame_size(a);
    r->o_fz = pa_frame_size(b);

    calc_map_table(r);

    if ((method >= PA_RESAMPLER_SPEEX_FIXED_BASE && method <= PA_RESAMPLER_SPEEX_FIXED_MAX) ||
        (method == PA_RESAMPLER_FFMPEG)
#ifdef HAVE_SEC_SRC
        || (method == PA_RESAMPLER_SECSRC)
#endif
        )
        r->work_format = PA_SAMPLE_S16NE;
    else if (method == PA_RESAMPLER_TRIVIAL || method == PA_RESAMPLER_COPY || method == PA_RESAMPLER_PEAKS) {

        if (r->map_required || a->format != b->format || method == PA_RESAMPLER_PEAKS) {

            if (a->format == PA_SAMPLE_S16NE || b->format == PA_SAMPLE_S16NE)
                r->work_format = PA_SAMPLE_S16NE;
            else if (a->format == PA_SAMPLE_S32NE || a->format == PA_SAMPLE_S32RE ||
                a->format == PA_SAMPLE_FLOAT32NE || a->format == PA_SAMPLE_FLOAT32RE ||
                a->format == PA_SAMPLE_S24NE || a->format == PA_SAMPLE_S24RE ||
                a->format == PA_SAMPLE_S24_32NE || a->format == PA_SAMPLE_S24_32RE ||
                b->format == PA_SAMPLE_S32NE || b->format == PA_SAMPLE_S32RE ||
                b->format == PA_SAMPLE_FLOAT32NE || b->format == PA_SAMPLE_FLOAT32RE ||
                b->format == PA_SAMPLE_S24NE || b->format == PA_SAMPLE_S24RE ||
                b->format == PA_SAMPLE_S24_32NE || b->format == PA_SAMPLE_S24_32RE)
                r->work_format = PA_SAMPLE_FLOAT32NE;
            else
                r->work_format = PA_SAMPLE_S16NE;

        } else
            r->work_format = a->format;

    } else
        r->work_format = PA_SAMPLE_FLOAT32NE;

    pa_log_info_verbose("Using resampler '%s', %s as working format.",
                pa_resample_method_to_string(method), pa_sample_format_to_string(r->work_format));

    r->w_sz = pa_sample_size_of_format(r->work_format);

    if (r->i_ss.format != r->work_format) {
        if (r->work_format == PA_SAMPLE_FLOAT32NE) {
            if (!(r->to_work_format_func = pa_get_convert_to_float32ne_function(r->i_ss.format)))
                goto fail;
        } else {
            pa_assert(r->work_format == PA_SAMPLE_S16NE);
            if (!(r->to_work_format_func = pa_get_convert_to_s16ne_function(r->i_ss.format)))
                goto fail;
        }
    }

    if (r->o_ss.format != r->work_format) {
        if (r->work_format == PA_SAMPLE_FLOAT32NE) {
            if (!(r->from_work_format_func = pa_get_convert_from_float32ne_function(r->o_ss.format)))
                goto fail;
        } else {
            pa_assert(r->work_format == PA_SAMPLE_S16NE);
            if (!(r->from_work_format_func = pa_get_convert_from_s16ne_function(r->o_ss.format)))
                goto fail;
        }
    }

    if (r->o_ss.channels <= r->i_ss.channels)
        r->work_channels = r->o_ss.channels;
    else
        r->work_channels = r->i_ss.channels;

#ifdef __TIZEN_LOG__
    pa_log_debug("Resampler rate:%d->%d(%s) format:%s->%s(%s) channels:%d->%d(resampling %d)",
        a->rate, b->rate, pa_resample_method_to_string(r->method),
        pa_sample_format_to_string(a->format), pa_sample_format_to_string(b->format), pa_sample_format_to_string(r->work_format),
        a->channels, b->channels, r->work_channels);
#else
    pa_log_debug("Resampler:\n  rate %d -> %d (method %s),\n  format %s -> %s (intermediate %s),\n  channels %d -> %d (resampling %d)",
        a->rate, b->rate, pa_resample_method_to_string(r->method),
        pa_sample_format_to_string(a->format), pa_sample_format_to_string(b->format), pa_sample_format_to_string(r->work_format),
        a->channels, b->channels, r->work_channels);
#endif

    /* initialize implementation */
    if (init_table[method](r) < 0)
        goto fail;

    return r;

fail:
    pa_xfree(r);

    return NULL;
}

void pa_resampler_free(pa_resampler *r) {
    pa_assert(r);

    if (r->impl_free)
        r->impl_free(r);

    if (r->to_work_format_buf.memblock)
        pa_memblock_unref(r->to_work_format_buf.memblock);
    if (r->remap_buf.memblock)
        pa_memblock_unref(r->remap_buf.memblock);
    if (r->resample_buf.memblock)
        pa_memblock_unref(r->resample_buf.memblock);
    if (r->from_work_format_buf.memblock)
        pa_memblock_unref(r->from_work_format_buf.memblock);

    pa_xfree(r);
}

void pa_resampler_set_input_rate(pa_resampler *r, uint32_t rate) {
    pa_assert(r);
    pa_assert(rate > 0);

    if (r->i_ss.rate == rate)
        return;

    r->i_ss.rate = rate;

    r->impl_update_rates(r);
}

void pa_resampler_set_output_rate(pa_resampler *r, uint32_t rate) {
    pa_assert(r);
    pa_assert(rate > 0);

    if (r->o_ss.rate == rate)
        return;

    r->o_ss.rate = rate;

    r->impl_update_rates(r);
}

size_t pa_resampler_request(pa_resampler *r, size_t out_length) {
    pa_assert(r);

    /* Let's round up here to make it more likely that the caller will get at
     * least out_length amount of data from pa_resampler_run().
     *
     * We don't take the leftover into account here. If we did, then it might
     * be in theory possible that this function would return 0 and
     * pa_resampler_run() would also return 0. That could lead to infinite
     * loops. When the leftover is ignored here, such loops would eventually
     * terminate, because the leftover would grow each round, finally
     * surpassing the minimum input threshold of the resampler. */
    return ((((uint64_t) ((out_length + r->o_fz-1) / r->o_fz) * r->i_ss.rate) + r->o_ss.rate-1) / r->o_ss.rate) * r->i_fz;
}

size_t pa_resampler_result(pa_resampler *r, size_t in_length) {
    size_t frames;

    pa_assert(r);

    /* Let's round up here to ensure that the caller will always allocate big
     * enough output buffer. */

    frames = (in_length + r->i_fz - 1) / r->i_fz;

    if (r->remap_buf_contains_leftover_data)
        frames += r->remap_buf.length / (r->w_sz * r->o_ss.channels);

    return (((uint64_t) frames * r->o_ss.rate + r->i_ss.rate - 1) / r->i_ss.rate) * r->o_fz;
}

size_t pa_resampler_max_block_size(pa_resampler *r) {
    size_t block_size_max;
    pa_sample_spec max_ss;
    size_t max_fs;
    size_t frames;

    pa_assert(r);

    block_size_max = pa_mempool_block_size_max(r->mempool);

    /* We deduce the "largest" sample spec we're using during the
     * conversion */
    max_ss.channels = (uint8_t) (PA_MAX(r->i_ss.channels, r->o_ss.channels));

    /* We silently assume that the format enum is ordered by size */
    max_ss.format = PA_MAX(r->i_ss.format, r->o_ss.format);
    max_ss.format = PA_MAX(max_ss.format, r->work_format);

    max_ss.rate = PA_MAX(r->i_ss.rate, r->o_ss.rate);

    max_fs = pa_frame_size(&max_ss);
    frames = block_size_max / max_fs - EXTRA_FRAMES;

    if (r->remap_buf_contains_leftover_data)
        frames -= r->remap_buf.length / (r->w_sz * r->o_ss.channels);

    return ((uint64_t) frames * r->i_ss.rate / max_ss.rate) * r->i_fz;
}

void pa_resampler_reset(pa_resampler *r) {
    pa_assert(r);

    if (r->impl_reset)
        r->impl_reset(r);

    r->remap_buf_contains_leftover_data = false;
}

pa_resample_method_t pa_resampler_get_method(pa_resampler *r) {
    pa_assert(r);

    return r->method;
}

const pa_channel_map* pa_resampler_input_channel_map(pa_resampler *r) {
    pa_assert(r);

    return &r->i_cm;
}

const pa_sample_spec* pa_resampler_input_sample_spec(pa_resampler *r) {
    pa_assert(r);

    return &r->i_ss;
}

const pa_channel_map* pa_resampler_output_channel_map(pa_resampler *r) {
    pa_assert(r);

    return &r->o_cm;
}

const pa_sample_spec* pa_resampler_output_sample_spec(pa_resampler *r) {
    pa_assert(r);

    return &r->o_ss;
}

static const char * const resample_methods[] = {
    "src-sinc-best-quality",
    "src-sinc-medium-quality",
    "src-sinc-fastest",
    "src-zero-order-hold",
    "src-linear",
    "trivial",
    "speex-float-0",
    "speex-float-1",
    "speex-float-2",
    "speex-float-3",
    "speex-float-4",
    "speex-float-5",
    "speex-float-6",
    "speex-float-7",
    "speex-float-8",
    "speex-float-9",
    "speex-float-10",
    "speex-fixed-0",
    "speex-fixed-1",
    "speex-fixed-2",
    "speex-fixed-3",
    "speex-fixed-4",
    "speex-fixed-5",
    "speex-fixed-6",
    "speex-fixed-7",
    "speex-fixed-8",
    "speex-fixed-9",
    "speex-fixed-10",
    "ffmpeg",
#ifdef HAVE_SEC_SRC
    "secsrc",
#endif
    "auto",
    "copy",
    "peaks"
};

const char *pa_resample_method_to_string(pa_resample_method_t m) {

    if (m < 0 || m >= PA_RESAMPLER_MAX)
        return NULL;

    return resample_methods[m];
}

int pa_resample_method_supported(pa_resample_method_t m) {

    if (m < 0 || m >= PA_RESAMPLER_MAX)
        return 0;

#ifndef HAVE_LIBSAMPLERATE
    if (m <= PA_RESAMPLER_SRC_LINEAR)
        return 0;
#endif

#ifndef HAVE_SPEEX
    if (m >= PA_RESAMPLER_SPEEX_FLOAT_BASE && m <= PA_RESAMPLER_SPEEX_FLOAT_MAX)
        return 0;
    if (m >= PA_RESAMPLER_SPEEX_FIXED_BASE && m <= PA_RESAMPLER_SPEEX_FIXED_MAX)
        return 0;
#endif

    return 1;
}

pa_resample_method_t pa_parse_resample_method(const char *string) {
    pa_resample_method_t m;

    pa_assert(string);

    for (m = 0; m < PA_RESAMPLER_MAX; m++)
        if (pa_streq(string, resample_methods[m]))
            return m;

    if (pa_streq(string, "speex-fixed"))
        return PA_RESAMPLER_SPEEX_FIXED_BASE + 1;

    if (pa_streq(string, "speex-float"))
        return PA_RESAMPLER_SPEEX_FLOAT_BASE + 1;

    return PA_RESAMPLER_INVALID;
}

static bool on_left(pa_channel_position_t p) {

    return
        p == PA_CHANNEL_POSITION_FRONT_LEFT ||
        p == PA_CHANNEL_POSITION_REAR_LEFT ||
        p == PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER ||
        p == PA_CHANNEL_POSITION_SIDE_LEFT ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_LEFT ||
        p == PA_CHANNEL_POSITION_TOP_REAR_LEFT;
}

static bool on_right(pa_channel_position_t p) {

    return
        p == PA_CHANNEL_POSITION_FRONT_RIGHT ||
        p == PA_CHANNEL_POSITION_REAR_RIGHT ||
        p == PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER ||
        p == PA_CHANNEL_POSITION_SIDE_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_REAR_RIGHT;
}

static bool on_center(pa_channel_position_t p) {

    return
        p == PA_CHANNEL_POSITION_FRONT_CENTER ||
        p == PA_CHANNEL_POSITION_REAR_CENTER ||
        p == PA_CHANNEL_POSITION_TOP_CENTER ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_CENTER ||
        p == PA_CHANNEL_POSITION_TOP_REAR_CENTER;
}

static bool on_lfe(pa_channel_position_t p) {
    return
        p == PA_CHANNEL_POSITION_LFE;
}

static bool on_front(pa_channel_position_t p) {
    return
        p == PA_CHANNEL_POSITION_FRONT_LEFT ||
        p == PA_CHANNEL_POSITION_FRONT_RIGHT ||
        p == PA_CHANNEL_POSITION_FRONT_CENTER ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_LEFT ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_CENTER ||
        p == PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER ||
        p == PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER;
}

static bool on_rear(pa_channel_position_t p) {
    return
        p == PA_CHANNEL_POSITION_REAR_LEFT ||
        p == PA_CHANNEL_POSITION_REAR_RIGHT ||
        p == PA_CHANNEL_POSITION_REAR_CENTER ||
        p == PA_CHANNEL_POSITION_TOP_REAR_LEFT ||
        p == PA_CHANNEL_POSITION_TOP_REAR_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_REAR_CENTER;
}

static bool on_side(pa_channel_position_t p) {
    return
        p == PA_CHANNEL_POSITION_SIDE_LEFT ||
        p == PA_CHANNEL_POSITION_SIDE_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_CENTER;
}

enum {
    ON_FRONT,
    ON_REAR,
    ON_SIDE,
    ON_OTHER
};

static int front_rear_side(pa_channel_position_t p) {
    if (on_front(p))
        return ON_FRONT;
    if (on_rear(p))
        return ON_REAR;
    if (on_side(p))
        return ON_SIDE;
    return ON_OTHER;
}

static void calc_map_table(pa_resampler *r) {
    unsigned oc, ic;
    unsigned n_oc, n_ic;
    bool ic_connected[PA_CHANNELS_MAX];
    bool remix;
    pa_strbuf *s;
    char *t;
    pa_remap_t *m;

    pa_assert(r);

    if (!(r->map_required = (r->i_ss.channels != r->o_ss.channels || (!(r->flags & PA_RESAMPLER_NO_REMAP) && !pa_channel_map_equal(&r->i_cm, &r->o_cm)))))
        return;

    m = &r->remap;

    n_oc = r->o_ss.channels;
    n_ic = r->i_ss.channels;

    memset(m->map_table_f, 0, sizeof(m->map_table_f));
    memset(m->map_table_i, 0, sizeof(m->map_table_i));

    memset(ic_connected, 0, sizeof(ic_connected));
    remix = (r->flags & (PA_RESAMPLER_NO_REMAP | PA_RESAMPLER_NO_REMIX)) == 0;

    if (r->flags & PA_RESAMPLER_NO_REMAP) {
        pa_assert(!remix);

        for (oc = 0; oc < PA_MIN(n_ic, n_oc); oc++)
            m->map_table_f[oc][oc] = 1.0f;

    } else if (r->flags & PA_RESAMPLER_NO_REMIX) {
        pa_assert(!remix);
        for (oc = 0; oc < n_oc; oc++) {
            pa_channel_position_t b = r->o_cm.map[oc];

            for (ic = 0; ic < n_ic; ic++) {
                pa_channel_position_t a = r->i_cm.map[ic];

                /* We shall not do any remixing. Hence, just check by name */
                if (a == b)
                    m->map_table_f[oc][ic] = 1.0f;
            }
        }
    } else {

        /* OK, we shall do the full monty: upmixing and downmixing. Our
         * algorithm is relatively simple, does not do spacialization, delay
         * elements or apply lowpass filters for LFE. Patches are always
         * welcome, though. Oh, and it doesn't do any matrix decoding. (Which
         * probably wouldn't make any sense anyway.)
         *
         * This code is not idempotent: downmixing an upmixed stereo stream is
         * not identical to the original. The volume will not match, and the
         * two channels will be a linear combination of both.
         *
         * This is loosely based on random suggestions found on the Internet,
         * such as this:
         * http://www.halfgaar.net/surround-sound-in-linux and the alsa upmix
         * plugin.
         *
         * The algorithm works basically like this:
         *
         * 1) Connect all channels with matching names.
         *
         * 2) Mono Handling:
         *    S:Mono: Copy into all D:channels
         *    D:Mono: Avg all S:channels
         *
         * 3) Mix D:Left, D:Right:
         *    D:Left: If not connected, avg all S:Left
         *    D:Right: If not connected, avg all S:Right
         *
         * 4) Mix D:Center
         *    If not connected, avg all S:Center
         *    If still not connected, avg all S:Left, S:Right
         *
         * 5) Mix D:LFE
         *    If not connected, avg all S:*
         *
         * 6) Make sure S:Left/S:Right is used: S:Left/S:Right: If not
         *    connected, mix into all D:left and all D:right channels. Gain is
         *    1/9.
         *
         * 7) Make sure S:Center, S:LFE is used:
         *
         *    S:Center, S:LFE: If not connected, mix into all D:left, all
         *    D:right, all D:center channels. Gain is 0.5 for center and 0.375
         *    for LFE. C-front is only mixed into L-front/R-front if available,
         *    otherwise into all L/R channels. Similarly for C-rear.
         *
         * 8) Normalize each row in the matrix such that the sum for each row is
         *    not larger than 1.0 in order to avoid clipping.
         *
         * S: and D: shall relate to the source resp. destination channels.
         *
         * Rationale: 1, 2 are probably obvious. For 3: this copies front to
         * rear if needed. For 4: we try to find some suitable C source for C,
         * if we don't find any, we avg L and R. For 5: LFE is mixed from all
         * channels. For 6: the rear channels should not be dropped entirely,
         * however have only minimal impact. For 7: movies usually encode
         * speech on the center channel. Thus we have to make sure this channel
         * is distributed to L and R if not available in the output. Also, LFE
         * is used to achieve a greater dynamic range, and thus we should try
         * to do our best to pass it to L+R.
         */

        unsigned
            ic_left = 0,
            ic_right = 0,
            ic_center = 0,
            ic_unconnected_left = 0,
            ic_unconnected_right = 0,
            ic_unconnected_center = 0,
            ic_unconnected_lfe = 0;
        bool ic_unconnected_center_mixed_in = 0;

        pa_assert(remix);

        for (ic = 0; ic < n_ic; ic++) {
            if (on_left(r->i_cm.map[ic]))
                ic_left++;
            if (on_right(r->i_cm.map[ic]))
                ic_right++;
            if (on_center(r->i_cm.map[ic]))
                ic_center++;
        }

        for (oc = 0; oc < n_oc; oc++) {
            bool oc_connected = false;
            pa_channel_position_t b = r->o_cm.map[oc];

            for (ic = 0; ic < n_ic; ic++) {
                pa_channel_position_t a = r->i_cm.map[ic];

                if (a == b || a == PA_CHANNEL_POSITION_MONO) {
                    m->map_table_f[oc][ic] = 1.0f;

                    oc_connected = true;
                    ic_connected[ic] = true;
                }
                else if (b == PA_CHANNEL_POSITION_MONO) {
                    m->map_table_f[oc][ic] = 1.0f / (float) n_ic;

                    oc_connected = true;
                    ic_connected[ic] = true;
                }
            }

            if (!oc_connected) {
                /* Try to find matching input ports for this output port */

                if (on_left(b)) {

                    /* We are not connected and on the left side, let's
                     * average all left side input channels. */

                    if (ic_left > 0)
                        for (ic = 0; ic < n_ic; ic++)
                            if (on_left(r->i_cm.map[ic])) {
                                m->map_table_f[oc][ic] = 1.0f / (float) ic_left;
                                ic_connected[ic] = true;
                            }

                    /* We ignore the case where there is no left input channel.
                     * Something is really wrong in this case anyway. */

                } else if (on_right(b)) {

                    /* We are not connected and on the right side, let's
                     * average all right side input channels. */

                    if (ic_right > 0)
                        for (ic = 0; ic < n_ic; ic++)
                            if (on_right(r->i_cm.map[ic])) {
                                m->map_table_f[oc][ic] = 1.0f / (float) ic_right;
                                ic_connected[ic] = true;
                            }

                    /* We ignore the case where there is no right input
                     * channel. Something is really wrong in this case anyway.
                     * */

                } else if (on_center(b)) {

                    if (ic_center > 0) {

                        /* We are not connected and at the center. Let's average
                         * all center input channels. */

                        for (ic = 0; ic < n_ic; ic++)
                            if (on_center(r->i_cm.map[ic])) {
                                m->map_table_f[oc][ic] = 1.0f / (float) ic_center;
                                ic_connected[ic] = true;
                            }

                    } else if (ic_left + ic_right > 0) {

                        /* Hmm, no center channel around, let's synthesize it
                         * by mixing L and R.*/

                        for (ic = 0; ic < n_ic; ic++)
                            if (on_left(r->i_cm.map[ic]) || on_right(r->i_cm.map[ic])) {
                                m->map_table_f[oc][ic] = 1.0f / (float) (ic_left + ic_right);
                                ic_connected[ic] = true;
                            }
                    }

                    /* We ignore the case where there is not even a left or
                     * right input channel. Something is really wrong in this
                     * case anyway. */

                } else if (on_lfe(b) && !(r->flags & PA_RESAMPLER_NO_LFE)) {

                    /* We are not connected and an LFE. Let's average all
                     * channels for LFE. */

                    for (ic = 0; ic < n_ic; ic++)
                        m->map_table_f[oc][ic] = 1.0f / (float) n_ic;

                    /* Please note that a channel connected to LFE doesn't
                     * really count as connected. */
                }
            }
        }

        for (ic = 0; ic < n_ic; ic++) {
            pa_channel_position_t a = r->i_cm.map[ic];

            if (ic_connected[ic])
                continue;

            if (on_left(a))
                ic_unconnected_left++;
            else if (on_right(a))
                ic_unconnected_right++;
            else if (on_center(a))
                ic_unconnected_center++;
            else if (on_lfe(a))
                ic_unconnected_lfe++;
        }

        for (ic = 0; ic < n_ic; ic++) {
            pa_channel_position_t a = r->i_cm.map[ic];

            if (ic_connected[ic])
                continue;

            for (oc = 0; oc < n_oc; oc++) {
                pa_channel_position_t b = r->o_cm.map[oc];

                if (on_left(a) && on_left(b))
                    m->map_table_f[oc][ic] = (1.f/9.f) / (float) ic_unconnected_left;

                else if (on_right(a) && on_right(b))
                    m->map_table_f[oc][ic] = (1.f/9.f) / (float) ic_unconnected_right;

                else if (on_center(a) && on_center(b)) {
                    m->map_table_f[oc][ic] = (1.f/9.f) / (float) ic_unconnected_center;
                    ic_unconnected_center_mixed_in = true;

                } else if (on_lfe(a) && !(r->flags & PA_RESAMPLER_NO_LFE))
                    m->map_table_f[oc][ic] = .375f / (float) ic_unconnected_lfe;
            }
        }

        if (ic_unconnected_center > 0 && !ic_unconnected_center_mixed_in) {
            unsigned ncenter[PA_CHANNELS_MAX];
            bool found_frs[PA_CHANNELS_MAX];

            memset(ncenter, 0, sizeof(ncenter));
            memset(found_frs, 0, sizeof(found_frs));

            /* Hmm, as it appears there was no center channel we
               could mix our center channel in. In this case, mix it into
               left and right. Using .5 as the factor. */

            for (ic = 0; ic < n_ic; ic++) {

                if (ic_connected[ic])
                    continue;

                if (!on_center(r->i_cm.map[ic]))
                    continue;

                for (oc = 0; oc < n_oc; oc++) {

                    if (!on_left(r->o_cm.map[oc]) && !on_right(r->o_cm.map[oc]))
                        continue;

                    if (front_rear_side(r->i_cm.map[ic]) == front_rear_side(r->o_cm.map[oc])) {
                        found_frs[ic] = true;
                        break;
                    }
                }

                for (oc = 0; oc < n_oc; oc++) {

                    if (!on_left(r->o_cm.map[oc]) && !on_right(r->o_cm.map[oc]))
                        continue;

                    if (!found_frs[ic] || front_rear_side(r->i_cm.map[ic]) == front_rear_side(r->o_cm.map[oc]))
                        ncenter[oc]++;
                }
            }

            for (oc = 0; oc < n_oc; oc++) {

                if (!on_left(r->o_cm.map[oc]) && !on_right(r->o_cm.map[oc]))
                    continue;

                if (ncenter[oc] <= 0)
                    continue;

                for (ic = 0; ic < n_ic; ic++) {

                    if (!on_center(r->i_cm.map[ic]))
                        continue;

                    if (!found_frs[ic] || front_rear_side(r->i_cm.map[ic]) == front_rear_side(r->o_cm.map[oc]))
                        m->map_table_f[oc][ic] = .5f / (float) ncenter[oc];
                }
            }
        }
    }

    for (oc = 0; oc < n_oc; oc++) {
        float sum = 0.0f;
        for (ic = 0; ic < n_ic; ic++)
            sum += m->map_table_f[oc][ic];

        if (sum > 1.0f)
            for (ic = 0; ic < n_ic; ic++)
                m->map_table_f[oc][ic] /= sum;
    }

    /* make an 16:16 int version of the matrix */
    for (oc = 0; oc < n_oc; oc++)
        for (ic = 0; ic < n_ic; ic++)
            m->map_table_i[oc][ic] = (int32_t) (m->map_table_f[oc][ic] * 0x10000);

    s = pa_strbuf_new();

    pa_strbuf_printf(s, "     ");
    for (ic = 0; ic < n_ic; ic++)
        pa_strbuf_printf(s, "  I%02u ", ic);
    pa_strbuf_puts(s, "\n    +");

    for (ic = 0; ic < n_ic; ic++)
        pa_strbuf_printf(s, "------");
    pa_strbuf_puts(s, "\n");

    for (oc = 0; oc < n_oc; oc++) {
        pa_strbuf_printf(s, "O%02u |", oc);

        for (ic = 0; ic < n_ic; ic++)
            pa_strbuf_printf(s, " %1.3f", m->map_table_f[oc][ic]);

        pa_strbuf_puts(s, "\n");
    }

    pa_log_debug_verbose("Channel matrix:\n%s", t = pa_strbuf_tostring_free(s));
    pa_xfree(t);

    /* initialize the remapping function */
    pa_init_remap(m);
}

static pa_memchunk* convert_to_work_format(pa_resampler *r, pa_memchunk *input) {
    unsigned n_samples;
    void *src, *dst;

    pa_assert(r);
    pa_assert(input);
    pa_assert(input->memblock);

    /* Convert the incoming sample into the work sample format and place them
     * in to_work_format_buf. */

    if (!r->to_work_format_func || !input->length)
        return input;

    n_samples = (unsigned) ((input->length / r->i_fz) * r->i_ss.channels);

    r->to_work_format_buf.index = 0;
    r->to_work_format_buf.length = r->w_sz * n_samples;

    if (!r->to_work_format_buf.memblock || r->to_work_format_buf_samples < n_samples) {
        if (r->to_work_format_buf.memblock)
            pa_memblock_unref(r->to_work_format_buf.memblock);

        r->to_work_format_buf_samples = n_samples;
        r->to_work_format_buf.memblock = pa_memblock_new(r->mempool, r->to_work_format_buf.length);
    }

    src = pa_memblock_acquire_chunk(input);
    dst = pa_memblock_acquire(r->to_work_format_buf.memblock);

    r->to_work_format_func(n_samples, src, dst);

    pa_memblock_release(input->memblock);
    pa_memblock_release(r->to_work_format_buf.memblock);

    return &r->to_work_format_buf;
}

static pa_memchunk *remap_channels(pa_resampler *r, pa_memchunk *input) {
    unsigned in_n_samples, out_n_samples, in_n_frames, out_n_frames;
    void *src, *dst;
    size_t leftover_length = 0;
    bool have_leftover;

    pa_assert(r);
    pa_assert(input);
    pa_assert(input->memblock);

    /* Remap channels and place the result in remap_buf. There may be leftover
     * data in the beginning of remap_buf. The leftover data is already
     * remapped, so it's not part of the input, it's part of the output. */

    have_leftover = r->remap_buf_contains_leftover_data;
    r->remap_buf_contains_leftover_data = false;

    if (!have_leftover && (!r->map_required || input->length <= 0))
        return input;
    else if (input->length <= 0)
        return &r->remap_buf;

    in_n_samples = (unsigned) (input->length / r->w_sz);
    in_n_frames = out_n_frames = in_n_samples / r->i_ss.channels;

    if (have_leftover) {
        leftover_length = r->remap_buf.length;
        out_n_frames += leftover_length / (r->w_sz * r->o_ss.channels);
    }

    out_n_samples = out_n_frames * r->o_ss.channels;
    r->remap_buf.length = out_n_samples * r->w_sz;

    if (have_leftover) {
        if (r->remap_buf_size < r->remap_buf.length) {
            pa_memblock *new_block = pa_memblock_new(r->mempool, r->remap_buf.length);

            src = pa_memblock_acquire(r->remap_buf.memblock);
            dst = pa_memblock_acquire(new_block);
            memcpy(dst, src, leftover_length);
            pa_memblock_release(r->remap_buf.memblock);
            pa_memblock_release(new_block);

            pa_memblock_unref(r->remap_buf.memblock);
            r->remap_buf.memblock = new_block;
            r->remap_buf_size = r->remap_buf.length;
        }

    } else {
        if (!r->remap_buf.memblock || r->remap_buf_size < r->remap_buf.length) {
            if (r->remap_buf.memblock)
                pa_memblock_unref(r->remap_buf.memblock);

            r->remap_buf_size = r->remap_buf.length;
            r->remap_buf.memblock = pa_memblock_new(r->mempool, r->remap_buf.length);
        }
    }

    src = pa_memblock_acquire_chunk(input);
    dst = (uint8_t *) pa_memblock_acquire(r->remap_buf.memblock) + leftover_length;

    if (r->map_required) {
        pa_remap_t *remap = &r->remap;

        pa_assert(remap->do_remap);
        remap->do_remap(remap, dst, src, in_n_frames);

    } else
        memcpy(dst, src, input->length);

    pa_memblock_release(input->memblock);
    pa_memblock_release(r->remap_buf.memblock);

    return &r->remap_buf;
}

static pa_memchunk *resample(pa_resampler *r, pa_memchunk *input) {
    unsigned in_n_frames, in_n_samples;
    unsigned out_n_frames, out_n_samples;

    pa_assert(r);
    pa_assert(input);

    /* Resample the data and place the result in resample_buf. */

    if (!r->impl_resample || !input->length)
        return input;

    in_n_samples = (unsigned) (input->length / r->w_sz);
    in_n_frames = (unsigned) (in_n_samples / r->work_channels);

    out_n_frames = ((in_n_frames*r->o_ss.rate)/r->i_ss.rate)+EXTRA_FRAMES;
    out_n_samples = out_n_frames * r->work_channels;

    r->resample_buf.index = 0;
    r->resample_buf.length = r->w_sz * out_n_samples;

    if (!r->resample_buf.memblock || r->resample_buf_samples < out_n_samples) {
        if (r->resample_buf.memblock)
            pa_memblock_unref(r->resample_buf.memblock);

        r->resample_buf_samples = out_n_samples;
        r->resample_buf.memblock = pa_memblock_new(r->mempool, r->resample_buf.length);
    }

    r->impl_resample(r, input, in_n_frames, &r->resample_buf, &out_n_frames);
    r->resample_buf.length = out_n_frames * r->w_sz * r->work_channels;

    return &r->resample_buf;
}

static pa_memchunk *convert_from_work_format(pa_resampler *r, pa_memchunk *input) {
    unsigned n_samples, n_frames;
    void *src, *dst;

    pa_assert(r);
    pa_assert(input);

    /* Convert the data into the correct sample type and place the result in
     * from_work_format_buf. */

    if (!r->from_work_format_func || !input->length)
        return input;

    n_samples = (unsigned) (input->length / r->w_sz);
    n_frames = n_samples / r->o_ss.channels;

    r->from_work_format_buf.index = 0;
    r->from_work_format_buf.length = r->o_fz * n_frames;

    if (!r->from_work_format_buf.memblock || r->from_work_format_buf_samples < n_samples) {
        if (r->from_work_format_buf.memblock)
            pa_memblock_unref(r->from_work_format_buf.memblock);

        r->from_work_format_buf_samples = n_samples;
        r->from_work_format_buf.memblock = pa_memblock_new(r->mempool, r->from_work_format_buf.length);
    }

    src = pa_memblock_acquire_chunk(input);
    dst = pa_memblock_acquire(r->from_work_format_buf.memblock);
    r->from_work_format_func(n_samples, src, dst);
    pa_memblock_release(input->memblock);
    pa_memblock_release(r->from_work_format_buf.memblock);

    return &r->from_work_format_buf;
}

void pa_resampler_run(pa_resampler *r, const pa_memchunk *in, pa_memchunk *out) {
    pa_memchunk *buf;

    pa_assert(r);
    pa_assert(in);
    pa_assert(out);
    pa_assert(in->length);
    pa_assert(in->memblock);
    pa_assert(in->length % r->i_fz == 0);

    buf = (pa_memchunk*) in;
    buf = convert_to_work_format(r, buf);
    /* Try to save resampling effort: if we have more output channels than
     * input channels, do resampling first, then remapping. */
    if (r->o_ss.channels <= r->i_ss.channels) {
        buf = remap_channels(r, buf);
        buf = resample(r, buf);
    } else {
        buf = resample(r, buf);
        buf = remap_channels(r, buf);
    }

    if (buf->length) {
        buf = convert_from_work_format(r, buf);
        *out = *buf;

        if (buf == in)
            pa_memblock_ref(buf->memblock);
        else
            pa_memchunk_reset(buf);
    } else
        pa_memchunk_reset(out);
}

static void save_leftover(pa_resampler *r, void *buf, size_t len) {
    void *dst;

    pa_assert(r);
    pa_assert(buf);
    pa_assert(len > 0);

    /* Store the leftover to remap_buf. */

    r->remap_buf.length = len;

    if (!r->remap_buf.memblock || r->remap_buf_size < r->remap_buf.length) {
        if (r->remap_buf.memblock)
            pa_memblock_unref(r->remap_buf.memblock);

        r->remap_buf_size = r->remap_buf.length;
        r->remap_buf.memblock = pa_memblock_new(r->mempool, r->remap_buf.length);
    }

    dst = pa_memblock_acquire(r->remap_buf.memblock);
    memcpy(dst, buf, r->remap_buf.length);
    pa_memblock_release(r->remap_buf.memblock);

    r->remap_buf_contains_leftover_data = true;
}

/*** libsamplerate based implementation ***/

#ifdef HAVE_LIBSAMPLERATE
static void libsamplerate_resample(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    SRC_DATA data;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    memset(&data, 0, sizeof(data));

    data.data_in = pa_memblock_acquire_chunk(input);
    data.input_frames = (long int) in_n_frames;

    data.data_out = pa_memblock_acquire_chunk(output);
    data.output_frames = (long int) *out_n_frames;

    data.src_ratio = (double) r->o_ss.rate / r->i_ss.rate;
    data.end_of_input = 0;

    pa_assert_se(src_process(r->src.state, &data) == 0);

    if (data.input_frames_used < in_n_frames) {
        void *leftover_data = data.data_in + data.input_frames_used * r->work_channels;
        size_t leftover_length = (in_n_frames - data.input_frames_used) * sizeof(float) * r->work_channels;

        save_leftover(r, leftover_data, leftover_length);
    }

    pa_memblock_release(input->memblock);
    pa_memblock_release(output->memblock);

    *out_n_frames = (unsigned) data.output_frames_gen;
}

static void libsamplerate_update_rates(pa_resampler *r) {
    pa_assert(r);

    pa_assert_se(src_set_ratio(r->src.state, (double) r->o_ss.rate / r->i_ss.rate) == 0);
}

static void libsamplerate_reset(pa_resampler *r) {
    pa_assert(r);

    pa_assert_se(src_reset(r->src.state) == 0);
}

static void libsamplerate_free(pa_resampler *r) {
    pa_assert(r);

    if (r->src.state)
        src_delete(r->src.state);
}

static int libsamplerate_init(pa_resampler *r) {
    int err;

    pa_assert(r);

    if (!(r->src.state = src_new(r->method, r->o_ss.channels, &err)))
        return -1;

    r->impl_free = libsamplerate_free;
    r->impl_update_rates = libsamplerate_update_rates;
    r->impl_resample = libsamplerate_resample;
    r->impl_reset = libsamplerate_reset;

    return 0;
}
#endif

/*** samsung src based implementation ***/

#ifdef HAVE_SEC_SRC
static void secsrc_resample(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    int ret = 0;
    short* data_in = NULL;
    short* data_out = NULL;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    data_in = (short*)((uint8_t*) pa_memblock_acquire(input->memblock) + input->index);
    data_out = (short*)((uint8_t*) pa_memblock_acquire(output->memblock) + output->index);

    SRC_InoutConfig(r->secsrc.state, data_in, data_out);
    ret = SRC_Exe(r->secsrc.state, in_n_frames * r->work_channels, *out_n_frames * r->w_sz * r->work_channels);

#ifdef DEBUG_RESAMPLE
    pa_log_info("RESAMPLES : state(%p), in_n(%5d), *out_n(%5d) => exe (%5d,%5d), frame_size(%d) => result(%5d), *out_n(%5d)",
            r->secsrc.state,
            in_n_frames, *out_n_frames,
            in_n_frames * r->work_channels, *out_n_frames * r->w_sz * r->work_channels, r->w_sz * r->work_channels,
            ret, ret / r->work_channels);
#endif
    *out_n_frames = ret / r->work_channels;

    pa_memblock_release(input->memblock);
    pa_memblock_release(output->memblock);
}

static void secsrc_update_rates(pa_resampler *r) {
    pa_assert(r);

    pa_log_info("UPDATE RATES : state = %p", r->secsrc.state);

    /* TODO : fill here */
}

static int _secsrc_convert_samplerate_code (int samplerate)
{
	int code = 0;

	switch(samplerate)
	{
	     case 8000:  code = SR08k; break;
	     case 11025: code = SR11k; break;
	     case 16000: code = SR16k; break;
	     case 22050: code = SR22k; break;
	     case 24000: code = SR24k; break;
	     case 32000: code = SR32k; break;
	     case 44100: code = SR44k; break;
	     case 48000: code = SR48k; break;
	     default: 	 code = SR08k; break;
	}

	return code;
}

static void secsrc_reset(pa_resampler *r) {
    int samplerate_in = SR08k;
    int samplerate_out = SR44k;

    pa_assert(r);

    samplerate_in = _secsrc_convert_samplerate_code (r->i_ss.rate);
    samplerate_out = _secsrc_convert_samplerate_code (r->o_ss.rate);

    if(r->secsrc.state != NULL) {
        SRC_Reset(r->secsrc.state);
        r->secsrc.state = NULL;
    }

   if ((r->secsrc.state = SRC_Init(r->work_channels, samplerate_in, samplerate_out, Q_MID)) == NULL) {
        pa_log_error("SRCSRC_Init fail!!!");
        return -1;
    }

    pa_log_info("reset resampler '%s', in=%d(%d), out=%d(%d), ch=%d",
            pa_resample_method_to_string(PA_RESAMPLER_SECSRC),
            r->i_ss.rate, samplerate_in,
            r->o_ss.rate, samplerate_out,
            r->work_channels);
}

static void secsrc_free(pa_resampler *r) {
    pa_assert(r);

    pa_log_info("FREE : state = %p", r->secsrc.state);
    /* FIXME: return error check */
    if (r->secsrc.state) {
       SRC_Reset(r->secsrc.state);
       r->secsrc.state = NULL;
    }
}

static int secsrc_init(pa_resampler *r) {
    int samplerate_in = SR08k;
    int samplerate_out = SR44k;

    pa_assert(r);

    samplerate_in = _secsrc_convert_samplerate_code (r->i_ss.rate);
    samplerate_out = _secsrc_convert_samplerate_code (r->o_ss.rate);

    if (r->i_ss.channels != r->work_channels) {
        pa_log_info("in = %d, out = %d", r->i_ss.channels, r->work_channels);
        /* FIXME */
    }

    pa_log_info("Using resampler '%s', in=%d(%d), out=%d(%d), ch=%d",
            pa_resample_method_to_string(PA_RESAMPLER_SECSRC),
            r->i_ss.rate, samplerate_in,
            r->o_ss.rate, samplerate_out,
            r->work_channels);
    if ((r->secsrc.state = SRC_Init(r->work_channels, samplerate_in, samplerate_out, Q_MID)) == NULL) {
        return -1;
    }

    /* Current in use */
    r->impl_free = secsrc_free;
    r->impl_resample = secsrc_resample;

    /* Dummy */
    r->impl_update_rates = secsrc_update_rates;
    r->impl_reset = secsrc_reset;

    pa_log_info("Resampler '%s' init success", pa_resample_method_to_string(PA_RESAMPLER_SECSRC));

    return 0;
}
#endif /* HAVE_SEC_SRC */

#ifdef HAVE_SPEEX
/*** speex based implementation ***/

static void speex_resample_float(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    float *in, *out;
    uint32_t inf = in_n_frames, outf = *out_n_frames;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    in = pa_memblock_acquire_chunk(input);
    out = pa_memblock_acquire_chunk(output);

    pa_assert_se(speex_resampler_process_interleaved_float(r->speex.state, in, &inf, out, &outf) == 0);

    pa_memblock_release(input->memblock);
    pa_memblock_release(output->memblock);

    pa_assert(inf == in_n_frames);
    *out_n_frames = outf;
}

static void speex_resample_int(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    int16_t *in, *out;
    uint32_t inf = in_n_frames, outf = *out_n_frames;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    in = pa_memblock_acquire_chunk(input);
    out = pa_memblock_acquire_chunk(output);

    pa_assert_se(speex_resampler_process_interleaved_int(r->speex.state, in, &inf, out, &outf) == 0);

    pa_memblock_release(input->memblock);
    pa_memblock_release(output->memblock);

    pa_assert(inf == in_n_frames);
    *out_n_frames = outf;
}

static void speex_update_rates(pa_resampler *r) {
    pa_assert(r);

    pa_assert_se(speex_resampler_set_rate(r->speex.state, r->i_ss.rate, r->o_ss.rate) == 0);
}

static void speex_reset(pa_resampler *r) {
    pa_assert(r);

    pa_assert_se(speex_resampler_reset_mem(r->speex.state) == 0);
}

static void speex_free(pa_resampler *r) {
    pa_assert(r);

    if (!r->speex.state)
        return;

    speex_resampler_destroy(r->speex.state);
}

static int speex_init(pa_resampler *r) {
    int q, err;

    pa_assert(r);

    r->impl_free = speex_free;
    r->impl_update_rates = speex_update_rates;
    r->impl_reset = speex_reset;

    if (r->method >= PA_RESAMPLER_SPEEX_FIXED_BASE && r->method <= PA_RESAMPLER_SPEEX_FIXED_MAX) {

        q = r->method - PA_RESAMPLER_SPEEX_FIXED_BASE;
        r->impl_resample = speex_resample_int;

    } else {
        pa_assert(r->method >= PA_RESAMPLER_SPEEX_FLOAT_BASE && r->method <= PA_RESAMPLER_SPEEX_FLOAT_MAX);

        q = r->method - PA_RESAMPLER_SPEEX_FLOAT_BASE;
        r->impl_resample = speex_resample_float;
    }

    pa_log_info("Choosing speex quality setting %i.", q);

    if (!(r->speex.state = speex_resampler_init(r->work_channels, r->i_ss.rate, r->o_ss.rate, q, &err)))
        return -1;

    return 0;
}
#endif

/* Trivial implementation */

static void trivial_resample(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    size_t fz;
    unsigned i_index, o_index;
    void *src, *dst;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    fz = r->w_sz * r->work_channels;

    src = pa_memblock_acquire_chunk(input);
    dst = pa_memblock_acquire_chunk(output);

    for (o_index = 0;; o_index++, r->trivial.o_counter++) {
        i_index = ((uint64_t) r->trivial.o_counter * r->i_ss.rate) / r->o_ss.rate;
        i_index = i_index > r->trivial.i_counter ? i_index - r->trivial.i_counter : 0;

        if (i_index >= in_n_frames)
            break;

        pa_assert_fp(o_index * fz < pa_memblock_get_length(output->memblock));

        memcpy((uint8_t*) dst + fz * o_index, (uint8_t*) src + fz * i_index, (int) fz);
    }

    pa_memblock_release(input->memblock);
    pa_memblock_release(output->memblock);

    *out_n_frames = o_index;

    r->trivial.i_counter += in_n_frames;

    /* Normalize counters */
    while (r->trivial.i_counter >= r->i_ss.rate) {
        pa_assert(r->trivial.o_counter >= r->o_ss.rate);

        r->trivial.i_counter -= r->i_ss.rate;
        r->trivial.o_counter -= r->o_ss.rate;
    }
}

static void trivial_update_rates_or_reset(pa_resampler *r) {
    pa_assert(r);

    r->trivial.i_counter = 0;
    r->trivial.o_counter = 0;
}

static int trivial_init(pa_resampler*r) {
    pa_assert(r);

    r->trivial.o_counter = r->trivial.i_counter = 0;

    r->impl_resample = trivial_resample;
    r->impl_update_rates = trivial_update_rates_or_reset;
    r->impl_reset = trivial_update_rates_or_reset;

    return 0;
}

/* Peak finder implementation */

static void peaks_resample(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    unsigned c, o_index = 0;
    unsigned i, i_end = 0;
    void *src, *dst;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    src = pa_memblock_acquire_chunk(input);
    dst = pa_memblock_acquire_chunk(output);

    i = ((uint64_t) r->peaks.o_counter * r->i_ss.rate) / r->o_ss.rate;
    i = i > r->peaks.i_counter ? i - r->peaks.i_counter : 0;

    while (i_end < in_n_frames) {
        i_end = ((uint64_t) (r->peaks.o_counter + 1) * r->i_ss.rate) / r->o_ss.rate;
        i_end = i_end > r->peaks.i_counter ? i_end - r->peaks.i_counter : 0;

        pa_assert_fp(o_index * r->w_sz * r->o_ss.channels < pa_memblock_get_length(output->memblock));

        /* 1ch float is treated separately, because that is the common case */
        if (r->o_ss.channels == 1 && r->work_format == PA_SAMPLE_FLOAT32NE) {
            float *s = (float*) src + i;
            float *d = (float*) dst + o_index;

            for (; i < i_end && i < in_n_frames; i++) {
                float n = fabsf(*s++);

                if (n > r->peaks.max_f[0])
                    r->peaks.max_f[0] = n;
            }

            if (i == i_end) {
                *d = r->peaks.max_f[0];
                r->peaks.max_f[0] = 0;
                o_index++, r->peaks.o_counter++;
            }
        } else if (r->work_format == PA_SAMPLE_S16NE) {
            int16_t *s = (int16_t*) src + r->i_ss.channels * i;
            int16_t *d = (int16_t*) dst + r->o_ss.channels * o_index;

            for (; i < i_end && i < in_n_frames; i++)
                for (c = 0; c < r->o_ss.channels; c++) {
                    int16_t n = abs(*s++);

                    if (n > r->peaks.max_i[c])
                        r->peaks.max_i[c] = n;
                }

            if (i == i_end) {
                for (c = 0; c < r->o_ss.channels; c++, d++) {
                    *d = r->peaks.max_i[c];
                    r->peaks.max_i[c] = 0;
                }
                o_index++, r->peaks.o_counter++;
            }
        } else {
            float *s = (float*) src + r->i_ss.channels * i;
            float *d = (float*) dst + r->o_ss.channels * o_index;

            for (; i < i_end && i < in_n_frames; i++)
                for (c = 0; c < r->o_ss.channels; c++) {
                    float n = fabsf(*s++);

                    if (n > r->peaks.max_f[c])
                        r->peaks.max_f[c] = n;
                }

            if (i == i_end) {
                for (c = 0; c < r->o_ss.channels; c++, d++) {
                    *d = r->peaks.max_f[c];
                    r->peaks.max_f[c] = 0;
                }
                o_index++, r->peaks.o_counter++;
            }
        }
    }

    pa_memblock_release(input->memblock);
    pa_memblock_release(output->memblock);

    *out_n_frames = o_index;

    r->peaks.i_counter += in_n_frames;

    /* Normalize counters */
    while (r->peaks.i_counter >= r->i_ss.rate) {
        pa_assert(r->peaks.o_counter >= r->o_ss.rate);

        r->peaks.i_counter -= r->i_ss.rate;
        r->peaks.o_counter -= r->o_ss.rate;
    }
}

static void peaks_update_rates_or_reset(pa_resampler *r) {
    pa_assert(r);

    r->peaks.i_counter = 0;
    r->peaks.o_counter = 0;
}

static int peaks_init(pa_resampler*r) {
    pa_assert(r);
    pa_assert(r->i_ss.rate >= r->o_ss.rate);
    pa_assert(r->work_format == PA_SAMPLE_S16NE || r->work_format == PA_SAMPLE_FLOAT32NE);

    r->peaks.o_counter = r->peaks.i_counter = 0;
    memset(r->peaks.max_i, 0, sizeof(r->peaks.max_i));
    memset(r->peaks.max_f, 0, sizeof(r->peaks.max_f));

    r->impl_resample = peaks_resample;
    r->impl_update_rates = peaks_update_rates_or_reset;
    r->impl_reset = peaks_update_rates_or_reset;

    return 0;
}

/*** ffmpeg based implementation ***/

static void ffmpeg_resample(pa_resampler *r, const pa_memchunk *input, unsigned in_n_frames, pa_memchunk *output, unsigned *out_n_frames) {
    unsigned used_frames = 0, c;
    int previous_consumed_frames = -1;

    pa_assert(r);
    pa_assert(input);
    pa_assert(output);
    pa_assert(out_n_frames);

    for (c = 0; c < r->work_channels; c++) {
        unsigned u;
        pa_memblock *b, *w;
        int16_t *p, *t, *k, *q, *s;
        int consumed_frames;

        /* Allocate a new block */
        b = pa_memblock_new(r->mempool, r->ffmpeg.buf[c].length + in_n_frames * sizeof(int16_t));
        p = pa_memblock_acquire(b);

        /* Now copy the input data, splitting up channels */
        t = (int16_t*) pa_memblock_acquire_chunk(input) + c;
        k = p;
        for (u = 0; u < in_n_frames; u++) {
            *k = *t;
            t += r->work_channels;
            k ++;
        }
        pa_memblock_release(input->memblock);

        /* Allocate buffer for the result */
        w = pa_memblock_new(r->mempool, *out_n_frames * sizeof(int16_t));
        q = pa_memblock_acquire(w);

        /* Now, resample */
        used_frames = (unsigned) av_resample(r->ffmpeg.state,
                                             q, p,
                                             &consumed_frames,
                                             (int) in_n_frames, (int) *out_n_frames,
                                             c >= (unsigned) (r->work_channels-1));

        pa_memblock_release(b);
        pa_memblock_unref(b);

        pa_assert(consumed_frames <= (int) in_n_frames);
        pa_assert(previous_consumed_frames == -1 || consumed_frames == previous_consumed_frames);
        previous_consumed_frames = consumed_frames;

        /* And place the results in the output buffer */
        s = (int16_t *) pa_memblock_acquire_chunk(output) + c;
        for (u = 0; u < used_frames; u++) {
            *s = *q;
            q++;
            s += r->work_channels;
        }
        pa_memblock_release(output->memblock);
        pa_memblock_release(w);
        pa_memblock_unref(w);
    }

    if (previous_consumed_frames < (int) in_n_frames) {
        void *leftover_data = (int16_t *) pa_memblock_acquire_chunk(input) + previous_consumed_frames * r->o_ss.channels;
        size_t leftover_length = (in_n_frames - previous_consumed_frames) * r->o_ss.channels * sizeof(int16_t);

        save_leftover(r, leftover_data, leftover_length);
        pa_memblock_release(input->memblock);
    }

    *out_n_frames = used_frames;
}

static void ffmpeg_free(pa_resampler *r) {
    unsigned c;

    pa_assert(r);

    if (r->ffmpeg.state)
        av_resample_close(r->ffmpeg.state);

    for (c = 0; c < PA_ELEMENTSOF(r->ffmpeg.buf); c++)
        if (r->ffmpeg.buf[c].memblock)
            pa_memblock_unref(r->ffmpeg.buf[c].memblock);
}

static int ffmpeg_init(pa_resampler *r) {
    unsigned c;

    pa_assert(r);

    /* We could probably implement different quality levels by
     * adjusting the filter parameters here. However, ffmpeg
     * internally only uses these hardcoded values, so let's use them
     * here for now as well until ffmpeg makes this configurable. */

    if (!(r->ffmpeg.state = av_resample_init((int) r->o_ss.rate, (int) r->i_ss.rate, 16, 10, 0, 0.8)))
        return -1;

    r->impl_free = ffmpeg_free;
    r->impl_resample = ffmpeg_resample;

    for (c = 0; c < PA_ELEMENTSOF(r->ffmpeg.buf); c++)
        pa_memchunk_reset(&r->ffmpeg.buf[c]);

    return 0;
}

/*** copy (noop) implementation ***/

static int copy_init(pa_resampler *r) {
    pa_assert(r);

    pa_assert(r->o_ss.rate == r->i_ss.rate);

    return 0;
}
