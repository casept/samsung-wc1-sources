#ifndef fooresamplerhfoo
#define fooresamplerhfoo

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

#include <pulse/sample.h>
#include <pulse/channelmap.h>
#include <pulsecore/memblock.h>
#include <pulsecore/memchunk.h>

typedef struct pa_resampler pa_resampler;

typedef enum pa_resample_method {
    PA_RESAMPLER_INVALID                 = -1,
    PA_RESAMPLER_SRC_SINC_BEST_QUALITY   = 0, /* = SRC_SINC_BEST_QUALITY */
    PA_RESAMPLER_SRC_SINC_MEDIUM_QUALITY = 1, /* = SRC_SINC_MEDIUM_QUALITY */
    PA_RESAMPLER_SRC_SINC_FASTEST        = 2, /* = SRC_SINC_FASTEST */
    PA_RESAMPLER_SRC_ZERO_ORDER_HOLD     = 3, /* = SRC_ZERO_ORDER_HOLD */
    PA_RESAMPLER_SRC_LINEAR              = 4, /* = SRC_LINEAR */
    PA_RESAMPLER_TRIVIAL,
    PA_RESAMPLER_SPEEX_FLOAT_BASE,
    PA_RESAMPLER_SPEEX_FLOAT_MAX = PA_RESAMPLER_SPEEX_FLOAT_BASE + 10,
    PA_RESAMPLER_SPEEX_FIXED_BASE,
    PA_RESAMPLER_SPEEX_FIXED_MAX = PA_RESAMPLER_SPEEX_FIXED_BASE + 10,
    PA_RESAMPLER_FFMPEG,
#ifdef HAVE_SEC_SRC
    PA_RESAMPLER_SECSRC,
#endif
    PA_RESAMPLER_AUTO, /* automatic select based on sample format */
    PA_RESAMPLER_COPY,
    PA_RESAMPLER_PEAKS,
    PA_RESAMPLER_MAX
} pa_resample_method_t;

typedef enum pa_resample_flags {
    PA_RESAMPLER_VARIABLE_RATE = 0x0001U,
    PA_RESAMPLER_NO_REMAP      = 0x0002U,  /* implies NO_REMIX */
    PA_RESAMPLER_NO_REMIX      = 0x0004U,
    PA_RESAMPLER_NO_LFE        = 0x0008U
} pa_resample_flags_t;

pa_resampler* pa_resampler_new(
        pa_mempool *pool,
        const pa_sample_spec *a,
        const pa_channel_map *am,
        const pa_sample_spec *b,
        const pa_channel_map *bm,
        pa_resample_method_t resample_method,
        pa_resample_flags_t flags);

void pa_resampler_free(pa_resampler *r);

/* Returns the size of an input memory block which is required to return the specified amount of output data */
size_t pa_resampler_request(pa_resampler *r, size_t out_length);

/* Inverse of pa_resampler_request() */
size_t pa_resampler_result(pa_resampler *r, size_t in_length);

/* Returns the maximum size of input blocks we can process without needing bounce buffers larger than the mempool tile size. */
size_t pa_resampler_max_block_size(pa_resampler *r);

/* Pass the specified memory chunk to the resampler and return the newly resampled data */
void pa_resampler_run(pa_resampler *r, const pa_memchunk *in, pa_memchunk *out);

/* Change the input rate of the resampler object */
void pa_resampler_set_input_rate(pa_resampler *r, uint32_t rate);

/* Change the output rate of the resampler object */
void pa_resampler_set_output_rate(pa_resampler *r, uint32_t rate);

/* Reinitialize state of the resampler, possibly due to seeking or other discontinuities */
void pa_resampler_reset(pa_resampler *r);

/* Return the resampling method of the resampler object */
pa_resample_method_t pa_resampler_get_method(pa_resampler *r);

/* Try to parse the resampler method */
pa_resample_method_t pa_parse_resample_method(const char *string);

/* return a human readable string for the specified resampling method. Inverse of pa_parse_resample_method() */
const char *pa_resample_method_to_string(pa_resample_method_t m);

/* Return 1 when the specified resampling method is supported */
int pa_resample_method_supported(pa_resample_method_t m);

const pa_channel_map* pa_resampler_input_channel_map(pa_resampler *r);
const pa_sample_spec* pa_resampler_input_sample_spec(pa_resampler *r);
const pa_channel_map* pa_resampler_output_channel_map(pa_resampler *r);
const pa_sample_spec* pa_resampler_output_sample_spec(pa_resampler *r);

#endif
