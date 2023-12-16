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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <pulsecore/core-util.h>
#include <pulsecore/i18n.h>
#include <pulsecore/macro.h>
#include <pulsecore/sample-util.h>

#include "volume.h"

int pa_cvolume_equal(const pa_cvolume *a, const pa_cvolume *b) {
    int i;
    pa_assert(a);
    pa_assert(b);

    pa_return_val_if_fail(pa_cvolume_valid(a), 0);

    if (PA_UNLIKELY(a == b))
        return 1;

    pa_return_val_if_fail(pa_cvolume_valid(b), 0);

    if (a->channels != b->channels)
        return 0;

    for (i = 0; i < a->channels; i++)
        if (a->values[i] != b->values[i])
            return 0;

    return 1;
}

pa_cvolume* pa_cvolume_init(pa_cvolume *a) {
    unsigned c;

    pa_assert(a);

    a->channels = 0;

    for (c = 0; c < PA_CHANNELS_MAX; c++)
        a->values[c] = PA_VOLUME_INVALID;

    return a;
}

pa_cvolume* pa_cvolume_set(pa_cvolume *a, unsigned channels, pa_volume_t v) {
    int i;

    pa_assert(a);
    pa_assert(channels > 0);
    pa_assert(channels <= PA_CHANNELS_MAX);

    a->channels = (uint8_t) channels;

    for (i = 0; i < a->channels; i++)
        /* Clamp in case there is stale data that exceeds the current
         * PA_VOLUME_MAX */
        a->values[i] = PA_CLAMP_VOLUME(v);

    return a;
}

pa_volume_t pa_cvolume_avg(const pa_cvolume *a) {
    uint64_t sum = 0;
    unsigned c;

    pa_assert(a);
    pa_return_val_if_fail(pa_cvolume_valid(a), PA_VOLUME_MUTED);

    for (c = 0; c < a->channels; c++)
        sum += a->values[c];

    sum /= a->channels;

    return (pa_volume_t) sum;
}

pa_volume_t pa_cvolume_avg_mask(const pa_cvolume *a, const pa_channel_map *cm, pa_channel_position_mask_t mask) {
    uint64_t sum = 0;
    unsigned c, n;

    pa_assert(a);

    if (!cm)
        return pa_cvolume_avg(a);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(a, cm), PA_VOLUME_MUTED);

    for (c = n = 0; c < a->channels; c++) {

        if (!(PA_CHANNEL_POSITION_MASK(cm->map[c]) & mask))
            continue;

        sum += a->values[c];
        n ++;
    }

    if (n > 0)
        sum /= n;

    return (pa_volume_t) sum;
}

pa_volume_t pa_cvolume_max(const pa_cvolume *a) {
    pa_volume_t m = PA_VOLUME_MUTED;
    unsigned c;

    pa_assert(a);
    pa_return_val_if_fail(pa_cvolume_valid(a), PA_VOLUME_MUTED);

    for (c = 0; c < a->channels; c++)
        if (a->values[c] > m)
            m = a->values[c];

    return m;
}

pa_volume_t pa_cvolume_min(const pa_cvolume *a) {
    pa_volume_t m = PA_VOLUME_MAX;
    unsigned c;

    pa_assert(a);
    pa_return_val_if_fail(pa_cvolume_valid(a), PA_VOLUME_MUTED);

    for (c = 0; c < a->channels; c++)
        if (a->values[c] < m)
            m = a->values[c];

    return m;
}

pa_volume_t pa_cvolume_max_mask(const pa_cvolume *a, const pa_channel_map *cm, pa_channel_position_mask_t mask) {
    pa_volume_t m = PA_VOLUME_MUTED;
    unsigned c;

    pa_assert(a);

    if (!cm)
        return pa_cvolume_max(a);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(a, cm), PA_VOLUME_MUTED);

    for (c = 0; c < a->channels; c++) {

        if (!(PA_CHANNEL_POSITION_MASK(cm->map[c]) & mask))
            continue;

        if (a->values[c] > m)
            m = a->values[c];
    }

    return m;
}

pa_volume_t pa_cvolume_min_mask(const pa_cvolume *a, const pa_channel_map *cm, pa_channel_position_mask_t mask) {
    pa_volume_t m = PA_VOLUME_MAX;
    unsigned c;

    pa_assert(a);

    if (!cm)
        return pa_cvolume_min(a);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(a, cm), PA_VOLUME_MUTED);

    for (c = 0; c < a->channels; c++) {

        if (!(PA_CHANNEL_POSITION_MASK(cm->map[c]) & mask))
            continue;

        if (a->values[c] < m)
            m = a->values[c];
    }

    return m;
}

pa_volume_t pa_sw_volume_multiply(pa_volume_t a, pa_volume_t b) {

    pa_return_val_if_fail(PA_VOLUME_IS_VALID(a), PA_VOLUME_INVALID);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(b), PA_VOLUME_INVALID);

    /* cbrt((a/PA_VOLUME_NORM)^3*(b/PA_VOLUME_NORM)^3)*PA_VOLUME_NORM = a*b/PA_VOLUME_NORM */

    return (pa_volume_t) PA_CLAMP_VOLUME((((uint64_t) a * (uint64_t) b + (uint64_t) PA_VOLUME_NORM / 2ULL) / (uint64_t) PA_VOLUME_NORM));
}

pa_volume_t pa_sw_volume_divide(pa_volume_t a, pa_volume_t b) {

    pa_return_val_if_fail(PA_VOLUME_IS_VALID(a), PA_VOLUME_INVALID);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(b), PA_VOLUME_INVALID);

    if (b <= PA_VOLUME_MUTED)
        return 0;

    return (pa_volume_t) (((uint64_t) a * (uint64_t) PA_VOLUME_NORM + (uint64_t) b / 2ULL) / (uint64_t) b);
}

/* Amplitude, not power */
static double linear_to_dB(double v) {
    return 20.0 * log10(v);
}

static double dB_to_linear(double v) {
    return pow(10.0, v / 20.0);
}

pa_volume_t pa_sw_volume_from_dB(double dB) {
    if (isinf(dB) < 0 || dB <= PA_DECIBEL_MININFTY)
        return PA_VOLUME_MUTED;

    return pa_sw_volume_from_linear(dB_to_linear(dB));
}

double pa_sw_volume_to_dB(pa_volume_t v) {

    pa_return_val_if_fail(PA_VOLUME_IS_VALID(v), PA_DECIBEL_MININFTY);

    if (v <= PA_VOLUME_MUTED)
        return PA_DECIBEL_MININFTY;

    return linear_to_dB(pa_sw_volume_to_linear(v));
}

pa_volume_t pa_sw_volume_from_linear(double v) {

    if (v <= 0.0)
        return PA_VOLUME_MUTED;

    /*
     * We use a cubic mapping here, as suggested and discussed here:
     *
     * http://www.robotplanet.dk/audio/audio_gui_design/
     * http://lists.linuxaudio.org/pipermail/linux-audio-dev/2009-May/thread.html#23151
     *
     * We make sure that the conversion to linear and back yields the
     * same volume value! That's why we need the lround() below!
     */

    return (pa_volume_t) PA_CLAMP_VOLUME((uint64_t) lround(cbrt(v) * PA_VOLUME_NORM));
}

double pa_sw_volume_to_linear(pa_volume_t v) {
    double f;

    pa_return_val_if_fail(PA_VOLUME_IS_VALID(v), 0.0);

    if (v <= PA_VOLUME_MUTED)
        return 0.0;

    if (v == PA_VOLUME_NORM)
        return 1.0;

    f = ((double) v / PA_VOLUME_NORM);

    return f*f*f;
}

char *pa_cvolume_snprint(char *s, size_t l, const pa_cvolume *c) {
    unsigned channel;
    pa_bool_t first = TRUE;
    char *e;

    pa_assert(s);
    pa_assert(l > 0);
    pa_assert(c);

    pa_init_i18n();

    if (!pa_cvolume_valid(c)) {
        pa_snprintf(s, l, _("(invalid)"));
        return s;
    }

    *(e = s) = 0;

    for (channel = 0; channel < c->channels && l > 1; channel++) {
        l -= pa_snprintf(e, l, "%s%u: %3u%%",
                      first ? "" : " ",
                      channel,
                      (c->values[channel]*100+PA_VOLUME_NORM/2)/PA_VOLUME_NORM);

        e = strchr(e, 0);
        first = FALSE;
    }

    return s;
}

char *pa_volume_snprint(char *s, size_t l, pa_volume_t v) {
    pa_assert(s);
    pa_assert(l > 0);

    pa_init_i18n();

    if (!PA_VOLUME_IS_VALID(v)) {
        pa_snprintf(s, l, _("(invalid)"));
        return s;
    }

    pa_snprintf(s, l, "%3u%%", (v*100+PA_VOLUME_NORM/2)/PA_VOLUME_NORM);
    return s;
}

char *pa_sw_cvolume_snprint_dB(char *s, size_t l, const pa_cvolume *c) {
    unsigned channel;
    pa_bool_t first = TRUE;
    char *e;

    pa_assert(s);
    pa_assert(l > 0);
    pa_assert(c);

    pa_init_i18n();

    if (!pa_cvolume_valid(c)) {
        pa_snprintf(s, l, _("(invalid)"));
        return s;
    }

    *(e = s) = 0;

    for (channel = 0; channel < c->channels && l > 1; channel++) {
        double f = pa_sw_volume_to_dB(c->values[channel]);

        l -= pa_snprintf(e, l, "%s%u: %0.2f dB",
                         first ? "" : " ",
                         channel,
                         isinf(f) < 0 || f <= PA_DECIBEL_MININFTY ? -INFINITY : f);

        e = strchr(e, 0);
        first = FALSE;
    }

    return s;
}

char *pa_sw_volume_snprint_dB(char *s, size_t l, pa_volume_t v) {
    double f;

    pa_assert(s);
    pa_assert(l > 0);

    pa_init_i18n();

    if (!PA_VOLUME_IS_VALID(v)) {
        pa_snprintf(s, l, _("(invalid)"));
        return s;
    }

    f = pa_sw_volume_to_dB(v);
    pa_snprintf(s, l, "%0.2f dB", isinf(f) < 0 || f <= PA_DECIBEL_MININFTY ? -INFINITY : f);

    return s;
}

int pa_cvolume_channels_equal_to(const pa_cvolume *a, pa_volume_t v) {
    unsigned c;
    pa_assert(a);

    pa_return_val_if_fail(pa_cvolume_valid(a), 0);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(v), 0);

    for (c = 0; c < a->channels; c++)
        if (a->values[c] != v)
            return 0;

    return 1;
}

pa_cvolume *pa_sw_cvolume_multiply(pa_cvolume *dest, const pa_cvolume *a, const pa_cvolume *b) {
    unsigned i;

    pa_assert(dest);
    pa_assert(a);
    pa_assert(b);

    pa_return_val_if_fail(pa_cvolume_valid(a), NULL);
    pa_return_val_if_fail(pa_cvolume_valid(b), NULL);

    for (i = 0; i < a->channels && i < b->channels; i++)
        dest->values[i] = pa_sw_volume_multiply(a->values[i], b->values[i]);

    dest->channels = (uint8_t) i;

   return dest;
}

pa_cvolume *pa_sw_cvolume_multiply_scalar(pa_cvolume *dest, const pa_cvolume *a, pa_volume_t b) {
    unsigned i;

    pa_assert(dest);
    pa_assert(a);

    pa_return_val_if_fail(pa_cvolume_valid(a), NULL);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(b), NULL);

    for (i = 0; i < a->channels; i++)
        dest->values[i] = pa_sw_volume_multiply(a->values[i], b);

    dest->channels = (uint8_t) i;

    return dest;
}

pa_cvolume *pa_sw_cvolume_divide(pa_cvolume *dest, const pa_cvolume *a, const pa_cvolume *b) {
    unsigned i;

    pa_assert(dest);
    pa_assert(a);
    pa_assert(b);

    pa_return_val_if_fail(pa_cvolume_valid(a), NULL);
    pa_return_val_if_fail(pa_cvolume_valid(b), NULL);

    for (i = 0; i < a->channels && i < b->channels; i++)
        dest->values[i] = pa_sw_volume_divide(a->values[i], b->values[i]);

    dest->channels = (uint8_t) i;

    return dest;
}

pa_cvolume *pa_sw_cvolume_divide_scalar(pa_cvolume *dest, const pa_cvolume *a, pa_volume_t b) {
    unsigned i;

    pa_assert(dest);
    pa_assert(a);

    pa_return_val_if_fail(pa_cvolume_valid(a), NULL);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(b), NULL);

    for (i = 0; i < a->channels; i++)
        dest->values[i] = pa_sw_volume_divide(a->values[i], b);

    dest->channels = (uint8_t) i;

    return dest;
}

int pa_cvolume_valid(const pa_cvolume *v) {
    unsigned c;

    pa_assert(v);

    if (v->channels <= 0 || v->channels > PA_CHANNELS_MAX)
        return 0;

    for (c = 0; c < v->channels; c++)
        if (!PA_VOLUME_IS_VALID(v->values[c]))
            return 0;

    return 1;
}

static pa_bool_t on_left(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_LEFT);
}

static pa_bool_t on_right(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_RIGHT);
}

static pa_bool_t on_center(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_CENTER);
}

static pa_bool_t on_lfe(pa_channel_position_t p) {
    return p == PA_CHANNEL_POSITION_LFE;
}

static pa_bool_t on_front(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_FRONT);
}

static pa_bool_t on_rear(pa_channel_position_t p) {
    return !!(PA_CHANNEL_POSITION_MASK(p) & PA_CHANNEL_POSITION_MASK_REAR);
}

pa_cvolume *pa_cvolume_remap(pa_cvolume *v, const pa_channel_map *from, const pa_channel_map *to) {
    int a, b;
    pa_cvolume result;

    pa_assert(v);
    pa_assert(from);
    pa_assert(to);

    pa_return_val_if_fail(pa_channel_map_valid(to), NULL);
    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(v, from), NULL);

    if (pa_channel_map_equal(from, to))
        return v;

    result.channels = to->channels;

    for (b = 0; b < to->channels; b++) {
        pa_volume_t k = 0;
        int n = 0;

        for (a = 0; a < from->channels; a++)
            if (from->map[a] == to->map[b]) {
                k += v->values[a];
                n ++;
            }

        if (n <= 0) {
            for (a = 0; a < from->channels; a++)
                if ((on_left(from->map[a]) && on_left(to->map[b])) ||
                    (on_right(from->map[a]) && on_right(to->map[b])) ||
                    (on_center(from->map[a]) && on_center(to->map[b])) ||
                    (on_lfe(from->map[a]) && on_lfe(to->map[b]))) {

                    k += v->values[a];
                    n ++;
                }
        }

        if (n <= 0)
            k = pa_cvolume_avg(v);
        else
            k /= n;

        result.values[b] = k;
    }

    *v = result;
    return v;
}

int pa_cvolume_compatible(const pa_cvolume *v, const pa_sample_spec *ss) {

    pa_assert(v);
    pa_assert(ss);

    pa_return_val_if_fail(pa_cvolume_valid(v), 0);
    pa_return_val_if_fail(pa_sample_spec_valid(ss), 0);

    return v->channels == ss->channels;
}

int pa_cvolume_compatible_with_channel_map(const pa_cvolume *v, const pa_channel_map *cm) {
    pa_assert(v);
    pa_assert(cm);

    pa_return_val_if_fail(pa_cvolume_valid(v), 0);
    pa_return_val_if_fail(pa_channel_map_valid(cm), 0);

    return v->channels == cm->channels;
}

static void get_avg_lr(const pa_channel_map *map, const pa_cvolume *v, pa_volume_t *l, pa_volume_t *r) {
    int c;
    pa_volume_t left = 0, right = 0;
    unsigned n_left = 0, n_right = 0;

    pa_assert(v);
    pa_assert(map);
    pa_assert(map->channels == v->channels);
    pa_assert(l);
    pa_assert(r);

    for (c = 0; c < map->channels; c++) {
        if (on_left(map->map[c])) {
            left += v->values[c];
            n_left++;
        } else if (on_right(map->map[c])) {
            right += v->values[c];
            n_right++;
        }
    }

    if (n_left <= 0)
        *l = PA_VOLUME_NORM;
    else
        *l = left / n_left;

    if (n_right <= 0)
        *r = PA_VOLUME_NORM;
    else
        *r = right / n_right;
}

float pa_cvolume_get_balance(const pa_cvolume *v, const pa_channel_map *map) {
    pa_volume_t left, right;

    pa_assert(v);
    pa_assert(map);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(v, map), 0.0f);

    if (!pa_channel_map_can_balance(map))
        return 0.0f;

    get_avg_lr(map, v, &left, &right);

    if (left == right)
        return 0.0f;

    /*   1.0,  0.0  =>  -1.0
         0.0,  1.0  =>   1.0
         0.0,  0.0  =>   0.0
         0.5,  0.5  =>   0.0
         1.0,  0.5  =>  -0.5
         1.0,  0.25 => -0.75
         0.75, 0.25 => -0.66
         0.5,  0.25 => -0.5   */

    if (left > right)
        return -1.0f + ((float) right / (float) left);
    else
        return 1.0f - ((float) left / (float) right);
}

pa_cvolume* pa_cvolume_set_balance(pa_cvolume *v, const pa_channel_map *map, float new_balance) {
    pa_volume_t left, nleft, right, nright, m;
    unsigned c;

    pa_assert(map);
    pa_assert(v);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(v, map), NULL);
    pa_return_val_if_fail(new_balance >= -1.0f, NULL);
    pa_return_val_if_fail(new_balance <= 1.0f, NULL);

    if (!pa_channel_map_can_balance(map))
        return v;

    get_avg_lr(map, v, &left, &right);

    m = PA_MAX(left, right);

    if (new_balance <= 0) {
        nright = (new_balance + 1.0f) * m;
        nleft = m;
    } else {
        nleft = (1.0f - new_balance) * m;
        nright = m;
    }

    for (c = 0; c < map->channels; c++) {
        if (on_left(map->map[c])) {
            if (left == 0)
                v->values[c] = nleft;
            else
                v->values[c] = (pa_volume_t) PA_CLAMP_VOLUME(((uint64_t) v->values[c] * (uint64_t) nleft) / (uint64_t) left);
        } else if (on_right(map->map[c])) {
            if (right == 0)
                v->values[c] = nright;
            else
                v->values[c] = (pa_volume_t) PA_CLAMP_VOLUME(((uint64_t) v->values[c] * (uint64_t) nright) / (uint64_t) right);
        }
    }

    return v;
}

pa_cvolume* pa_cvolume_scale(pa_cvolume *v, pa_volume_t max) {
    unsigned c;
    pa_volume_t t = 0;

    pa_assert(v);

    pa_return_val_if_fail(pa_cvolume_valid(v), NULL);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(max), NULL);

    t = pa_cvolume_max(v);

    if (t <= PA_VOLUME_MUTED)
        return pa_cvolume_set(v, v->channels, max);

    for (c = 0; c < v->channels; c++)
        v->values[c] = (pa_volume_t) PA_CLAMP_VOLUME(((uint64_t) v->values[c] * (uint64_t) max) / (uint64_t) t);

    return v;
}

pa_cvolume* pa_cvolume_scale_mask(pa_cvolume *v, pa_volume_t max, pa_channel_map *cm, pa_channel_position_mask_t mask) {
    unsigned c;
    pa_volume_t t = 0;

    pa_assert(v);

    pa_return_val_if_fail(PA_VOLUME_IS_VALID(max), NULL);

    if (!cm)
        return pa_cvolume_scale(v, max);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(v, cm), NULL);

    t = pa_cvolume_max_mask(v, cm, mask);

    if (t <= PA_VOLUME_MUTED)
        return pa_cvolume_set(v, v->channels, max);

    for (c = 0; c < v->channels; c++)
        v->values[c] = (pa_volume_t) PA_CLAMP_VOLUME(((uint64_t) v->values[c] * (uint64_t) max) / (uint64_t) t);

    return v;
}

static void get_avg_fr(const pa_channel_map *map, const pa_cvolume *v, pa_volume_t *f, pa_volume_t *r) {
    int c;
    pa_volume_t front = 0, rear = 0;
    unsigned n_front = 0, n_rear = 0;

    pa_assert(v);
    pa_assert(map);
    pa_assert(map->channels == v->channels);
    pa_assert(f);
    pa_assert(r);

    for (c = 0; c < map->channels; c++) {
        if (on_front(map->map[c])) {
            front += v->values[c];
            n_front++;
        } else if (on_rear(map->map[c])) {
            rear += v->values[c];
            n_rear++;
        }
    }

    if (n_front <= 0)
        *f = PA_VOLUME_NORM;
    else
        *f = front / n_front;

    if (n_rear <= 0)
        *r = PA_VOLUME_NORM;
    else
        *r = rear / n_rear;
}

float pa_cvolume_get_fade(const pa_cvolume *v, const pa_channel_map *map) {
    pa_volume_t front, rear;

    pa_assert(v);
    pa_assert(map);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(v, map), 0.0f);

    if (!pa_channel_map_can_fade(map))
        return 0.0f;

    get_avg_fr(map, v, &front, &rear);

    if (front == rear)
        return 0.0f;

    if (rear > front)
        return -1.0f + ((float) front / (float) rear);
    else
        return 1.0f - ((float) rear / (float) front);
}

pa_cvolume* pa_cvolume_set_fade(pa_cvolume *v, const pa_channel_map *map, float new_fade) {
    pa_volume_t front, nfront, rear, nrear, m;
    unsigned c;

    pa_assert(map);
    pa_assert(v);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(v, map), NULL);
    pa_return_val_if_fail(new_fade >= -1.0f, NULL);
    pa_return_val_if_fail(new_fade <= 1.0f, NULL);

    if (!pa_channel_map_can_fade(map))
        return v;

    get_avg_fr(map, v, &front, &rear);

    m = PA_MAX(front, rear);

    if (new_fade <= 0) {
        nfront = (new_fade + 1.0f) * m;
        nrear = m;
    } else {
        nrear = (1.0f - new_fade) * m;
        nfront = m;
    }

    for (c = 0; c < map->channels; c++) {
        if (on_front(map->map[c])) {
            if (front == 0)
                v->values[c] = nfront;
            else
                v->values[c] = (pa_volume_t) PA_CLAMP_VOLUME(((uint64_t) v->values[c] * (uint64_t) nfront) / (uint64_t) front);
        } else if (on_rear(map->map[c])) {
            if (rear == 0)
                v->values[c] = nrear;
            else
                v->values[c] = (pa_volume_t) PA_CLAMP_VOLUME(((uint64_t) v->values[c] * (uint64_t) nrear) / (uint64_t) rear);
        }
    }

    return v;
}

pa_cvolume* pa_cvolume_set_position(
        pa_cvolume *cv,
        const pa_channel_map *map,
        pa_channel_position_t t,
        pa_volume_t v) {

    unsigned c;
    pa_bool_t good = FALSE;

    pa_assert(cv);
    pa_assert(map);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(cv, map), NULL);
    pa_return_val_if_fail(t < PA_CHANNEL_POSITION_MAX, NULL);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(v), NULL);

    for (c = 0; c < map->channels; c++)
        if (map->map[c] == t) {
            cv->values[c] = v;
            good = TRUE;
        }

    return good ? cv : NULL;
}

pa_volume_t pa_cvolume_get_position(
        pa_cvolume *cv,
        const pa_channel_map *map,
        pa_channel_position_t t) {

    unsigned c;
    pa_volume_t v = PA_VOLUME_MUTED;

    pa_assert(cv);
    pa_assert(map);

    pa_return_val_if_fail(pa_cvolume_compatible_with_channel_map(cv, map), PA_VOLUME_MUTED);
    pa_return_val_if_fail(t < PA_CHANNEL_POSITION_MAX, PA_VOLUME_MUTED);

    for (c = 0; c < map->channels; c++)
        if (map->map[c] == t)
            if (cv->values[c] > v)
                v = cv->values[c];

    return v;
}

pa_cvolume* pa_cvolume_merge(pa_cvolume *dest, const pa_cvolume *a, const pa_cvolume *b) {
    unsigned i;

    pa_assert(dest);
    pa_assert(a);
    pa_assert(b);

    pa_return_val_if_fail(pa_cvolume_valid(a), NULL);
    pa_return_val_if_fail(pa_cvolume_valid(b), NULL);

    for (i = 0; i < a->channels && i < b->channels; i++)
        dest->values[i] = PA_MAX(a->values[i], b->values[i]);

    dest->channels = (uint8_t) i;

    return dest;
}

pa_cvolume* pa_cvolume_inc_clamp(pa_cvolume *v, pa_volume_t inc, pa_volume_t limit) {
    pa_volume_t m;

    pa_assert(v);

    pa_return_val_if_fail(pa_cvolume_valid(v), NULL);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(inc), NULL);

    m = pa_cvolume_max(v);

    if (m >= limit - inc)
        m = limit;
    else
        m += inc;

    return pa_cvolume_scale(v, m);
}

pa_cvolume* pa_cvolume_inc(pa_cvolume *v, pa_volume_t inc){
    return pa_cvolume_inc_clamp(v, inc, PA_VOLUME_MAX);
}

pa_cvolume* pa_cvolume_dec(pa_cvolume *v, pa_volume_t dec) {
    pa_volume_t m;

    pa_assert(v);

    pa_return_val_if_fail(pa_cvolume_valid(v), NULL);
    pa_return_val_if_fail(PA_VOLUME_IS_VALID(dec), NULL);

    m = pa_cvolume_max(v);

    if (m <= PA_VOLUME_MUTED + dec)
        m = PA_VOLUME_MUTED;
    else
        m -= dec;

    return pa_cvolume_scale(v, m);
}

