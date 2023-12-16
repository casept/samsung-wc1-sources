/***
  This file is part of PulseAudio.

  Copyright 2011 Intel Corporation
  Copyright 2011 Collabora Multimedia
  Copyright 2011 Arun Raghavan <arun.raghavan@collabora.co.uk>

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

#include <json.h>

#include <pulse/internal.h>
#include <pulse/xmalloc.h>

#include <pulsecore/core-util.h>
#include <pulsecore/i18n.h>
#include <pulsecore/macro.h>

#include "format.h"

#define PA_JSON_MIN_KEY "min"
#define PA_JSON_MAX_KEY "max"

static int pa_format_info_prop_compatible(const char *one, const char *two);

static const char* const _encoding_str_table[]= {
    [PA_ENCODING_PCM] = "pcm",
    [PA_ENCODING_AC3_IEC61937] = "ac3-iec61937",
    [PA_ENCODING_EAC3_IEC61937] = "eac3-iec61937",
    [PA_ENCODING_MPEG_IEC61937] = "mpeg-iec61937",
    [PA_ENCODING_DTS_IEC61937] = "dts-iec61937",
    [PA_ENCODING_MPEG2_AAC_IEC61937] = "mpeg2-aac-iec61937",
    [PA_ENCODING_ANY] = "any",
};

const char *pa_encoding_to_string(pa_encoding_t e) {
    if (e < 0 || e >= PA_ENCODING_MAX)
        return NULL;

    return _encoding_str_table[e];
}

pa_encoding_t pa_encoding_from_string(const char *encoding) {
    pa_encoding_t e;

    for (e = PA_ENCODING_ANY; e < PA_ENCODING_MAX; e++)
        if (pa_streq(_encoding_str_table[e], encoding))
            return e;

    return PA_ENCODING_INVALID;
}

pa_format_info* pa_format_info_new(void) {
    pa_format_info *f = pa_xnew(pa_format_info, 1);

    f->encoding = PA_ENCODING_INVALID;
    f->plist = pa_proplist_new();

    return f;
}

pa_format_info* pa_format_info_copy(const pa_format_info *src) {
    pa_format_info *dest;

    pa_assert(src);

    dest = pa_xnew(pa_format_info, 1);

    dest->encoding = src->encoding;

    if (src->plist)
        dest->plist = pa_proplist_copy(src->plist);
    else
        dest->plist = NULL;

    return dest;
}

void pa_format_info_free(pa_format_info *f) {
    pa_assert(f);

    pa_proplist_free(f->plist);
    pa_xfree(f);
}

int pa_format_info_valid(const pa_format_info *f) {
    return (f->encoding >= 0 && f->encoding < PA_ENCODING_MAX && f->plist != NULL);
}

int pa_format_info_is_pcm(const pa_format_info *f) {
    return f->encoding == PA_ENCODING_PCM;
}

char *pa_format_info_snprint(char *s, size_t l, const pa_format_info *f) {
    char *tmp;

    pa_assert(s);
    pa_assert(l > 0);
    pa_assert(f);

    pa_init_i18n();

    if (!pa_format_info_valid(f))
        pa_snprintf(s, l, _("(invalid)"));
    else {
        tmp = pa_proplist_to_string_sep(f->plist, "  ");
        if (tmp[0])
            pa_snprintf(s, l, "%s, %s", pa_encoding_to_string(f->encoding), tmp);
        else
            pa_snprintf(s, l, "%s", pa_encoding_to_string(f->encoding));
        pa_xfree(tmp);
    }

    return s;
}

pa_format_info* pa_format_info_from_string(const char *str) {
    pa_format_info *f = pa_format_info_new();
    char *encoding = NULL, *properties = NULL;
    size_t pos;

    pos = strcspn(str, ",");

    encoding = pa_xstrndup(str, pos);
    f->encoding = pa_encoding_from_string(pa_strip(encoding));
    if (f->encoding == PA_ENCODING_INVALID)
        goto error;

    if (pos != strlen(str)) {
        pa_proplist *plist;

        properties = pa_xstrdup(&str[pos+1]);
        plist = pa_proplist_from_string(properties);

        if (!plist)
            goto error;

        pa_proplist_free(f->plist);
        f->plist = plist;
    }

out:
    if (encoding)
        pa_xfree(encoding);
    if (properties)
        pa_xfree(properties);
    return f;

error:
    pa_format_info_free(f);
    f = NULL;
    goto out;
}

int pa_format_info_is_compatible(pa_format_info *first, pa_format_info *second) {
    const char *key;
    void *state = NULL;

    pa_assert(first);
    pa_assert(second);

    if (first->encoding != second->encoding)
        return FALSE;

    while ((key = pa_proplist_iterate(first->plist, &state))) {
        const char *value_one, *value_two;

        value_one = pa_proplist_gets(first->plist, key);
        value_two = pa_proplist_gets(second->plist, key);

        if (!value_two || !pa_format_info_prop_compatible(value_one, value_two))
            return FALSE;
    }

    return TRUE;
}

pa_format_info* pa_format_info_from_sample_spec(pa_sample_spec *ss, pa_channel_map *map) {
    char cm[PA_CHANNEL_MAP_SNPRINT_MAX];
    pa_format_info *f;

    pa_assert(ss && pa_sample_spec_valid(ss));
    pa_assert(!map || pa_channel_map_valid(map));

    f = pa_format_info_new();
    f->encoding = PA_ENCODING_PCM;

    pa_format_info_set_sample_format(f, ss->format);
    pa_format_info_set_rate(f, ss->rate);
    pa_format_info_set_channels(f, ss->channels);

    if (map) {
        pa_channel_map_snprint(cm, sizeof(cm), map);
        pa_format_info_set_prop_string(f, PA_PROP_FORMAT_CHANNEL_MAP, cm);
    }

    return f;
}

/* For compressed streams */
static int pa_format_info_to_sample_spec_fake(pa_format_info *f, pa_sample_spec *ss) {
    int rate;

    pa_assert(f);
    pa_assert(ss);

    /* Note: When we add support for non-IEC61937 encapsulated compressed
     * formats, this function should return a non-zero values for these. */

    ss->format = PA_SAMPLE_S16LE;
    ss->channels = 2;

    pa_return_val_if_fail(pa_format_info_get_prop_int(f, PA_PROP_FORMAT_RATE, &rate) == 0, -PA_ERR_INVALID);
    ss->rate = (uint32_t) rate;

    if (f->encoding == PA_ENCODING_EAC3_IEC61937)
        ss->rate *= 4;

    return 0;
}

/* For PCM streams */
int pa_format_info_to_sample_spec(pa_format_info *f, pa_sample_spec *ss, pa_channel_map *map) {
    char *sf = NULL, *m = NULL;
    int rate, channels;
    int ret = -PA_ERR_INVALID;

    pa_assert(f);
    pa_assert(ss);

    if (!pa_format_info_is_pcm(f))
        return pa_format_info_to_sample_spec_fake(f, ss);

    if (pa_format_info_get_prop_string(f, PA_PROP_FORMAT_SAMPLE_FORMAT, &sf))
        goto out;
    if (pa_format_info_get_prop_int(f, PA_PROP_FORMAT_RATE, &rate))
        goto out;
    if (pa_format_info_get_prop_int(f, PA_PROP_FORMAT_CHANNELS, &channels))
        goto out;

    if ((ss->format = pa_parse_sample_format(sf)) == PA_SAMPLE_INVALID)
        goto out;

    ss->rate = (uint32_t) rate;
    ss->channels = (uint8_t) channels;

    if (map) {
        pa_channel_map_init(map);

        if (pa_format_info_get_prop_string(f, PA_PROP_FORMAT_CHANNEL_MAP, &m) == 0)
            if (pa_channel_map_parse(map, m) == NULL)
                goto out;
    }

    ret = 0;

out:
    if (sf)
        pa_xfree(sf);
    if (m)
        pa_xfree(m);

    return ret;
}

pa_prop_type_t pa_format_info_get_prop_type(pa_format_info *f, const char *key) {
    const char *str;
    json_object *o, *o1;
    pa_prop_type_t type;

    pa_assert(f);
    pa_assert(key);

    str = pa_proplist_gets(f->plist, key);
    if (!str)
        return PA_PROP_TYPE_INVALID;

    o = json_tokener_parse(str);
    if (is_error(o))
        return PA_PROP_TYPE_INVALID;

    switch (json_object_get_type(o)) {
        case json_type_int:
            type = PA_PROP_TYPE_INT;
            break;

        case json_type_string:
            type = PA_PROP_TYPE_STRING;
            break;

        case json_type_array:
            if (json_object_array_length(o) == 0) {
                /* Unlikely, but let's account for this anyway. We need at
                 * least one element to figure out the array type. */
                type = PA_PROP_TYPE_INVALID;
                break;
            }

            o1 = json_object_array_get_idx(o, 1);

            if (json_object_get_type(o1) == json_type_int)
                type = PA_PROP_TYPE_INT_ARRAY;
            else if (json_object_get_type(o1) == json_type_string)
                type = PA_PROP_TYPE_STRING_ARRAY;
            else
                type = PA_PROP_TYPE_INVALID;

            json_object_put(o1);
            break;

        case json_type_object:
            /* We actually know at this point that it's a int range, but let's
             * confirm. */
            o1 = json_object_object_get(o, PA_JSON_MIN_KEY);
            if (!o1) {
                type = PA_PROP_TYPE_INVALID;
                break;
            }
            json_object_put(o1);

            o1 = json_object_object_get(o, PA_JSON_MAX_KEY);
            if (!o1) {
                type = PA_PROP_TYPE_INVALID;
                break;
            }
            json_object_put(o1);

            type = PA_PROP_TYPE_INT_RANGE;
            break;

        default:
            type = PA_PROP_TYPE_INVALID;
            break;
    }

    json_object_put(o);
    return type;
}

int pa_format_info_get_prop_int(pa_format_info *f, const char *key, int *v) {
    const char *str;
    json_object *o;

    pa_assert(f);
    pa_assert(key);
    pa_assert(v);

    str = pa_proplist_gets(f->plist, key);
    if (!str)
        return -PA_ERR_NOENTITY;

    o = json_tokener_parse(str);
    if (is_error(o))
        return -PA_ERR_INVALID;

    if (json_object_get_type(o) != json_type_int) {
        json_object_put(o);
        return -PA_ERR_INVALID;
    }

    *v = json_object_get_int(o);
    json_object_put(o);

    return 0;
}

int pa_format_info_get_prop_int_range(pa_format_info *f, const char *key, int *min, int *max) {
    const char *str;
    json_object *o, *o1;
    int ret = -PA_ERR_INVALID;

    pa_assert(f);
    pa_assert(key);
    pa_assert(min);
    pa_assert(max);

    str = pa_proplist_gets(f->plist, key);
    if (!str)
        return -PA_ERR_NOENTITY;

    o = json_tokener_parse(str);
    if (is_error(o))
        return -PA_ERR_INVALID;

    if (json_object_get_type(o) != json_type_object)
        goto out;

    if (!(o1 = json_object_object_get(o, PA_JSON_MIN_KEY)))
        goto out;

    *min = json_object_get_int(o1);
    json_object_put(o1);

    if (!(o1 = json_object_object_get(o, PA_JSON_MAX_KEY)))
        goto out;

    *max = json_object_get_int(o1);
    json_object_put(o1);

    ret = 0;

out:
    json_object_put(o);
    return ret;
}

int pa_format_info_get_prop_int_array(pa_format_info *f, const char *key, int **values, int *n_values)
{
    const char *str;
    json_object *o, *o1;
    int i, ret = -PA_ERR_INVALID;

    pa_assert(f);
    pa_assert(key);
    pa_assert(values);
    pa_assert(n_values);

    str = pa_proplist_gets(f->plist, key);
    if (!str)
        return -PA_ERR_NOENTITY;

    o = json_tokener_parse(str);
    if (is_error(o))
        return -PA_ERR_INVALID;

    if (json_object_get_type(o) != json_type_array)
        goto out;

    *n_values = json_object_array_length(o);
    *values = pa_xnew(int, *n_values);

    for (i = 0; i < *n_values; i++) {
        o1 = json_object_array_get_idx(o, i);

        if (json_object_get_type(o1) != json_type_int) {
            json_object_put(o1);
            goto out;
        }

        (*values)[i] = json_object_get_int(o1);
        json_object_put(o1);
    }

    ret = 0;

out:
    json_object_put(o);
    return ret;
}

int pa_format_info_get_prop_string(pa_format_info *f, const char *key, char **v) {
    const char *str = NULL;
    json_object *o;

    pa_assert(f);
    pa_assert(key);
    pa_assert(v);

    str = pa_proplist_gets(f->plist, key);
    if (!str)
        return -PA_ERR_NOENTITY;

    o = json_tokener_parse(str);
    if (is_error(o))
        return -PA_ERR_INVALID;

    if (json_object_get_type(o) != json_type_string) {
        json_object_put(o);
        return -PA_ERR_INVALID;
    }

    *v = pa_xstrdup(json_object_get_string(o));
    json_object_put(o);

    return 0;
}

int pa_format_info_get_prop_string_array(pa_format_info *f, const char *key, char ***values, int *n_values)
{
    const char *str;
    json_object *o, *o1;
    int i, ret = -PA_ERR_INVALID;

    pa_assert(f);
    pa_assert(key);
    pa_assert(values);
    pa_assert(n_values);

    str = pa_proplist_gets(f->plist, key);
    if (!str)
        return -PA_ERR_NOENTITY;

    o = json_tokener_parse(str);
    if (is_error(o))
        return -PA_ERR_INVALID;

    if (json_object_get_type(o) != json_type_array)
        goto out;

    *n_values = json_object_array_length(o);
    *values = pa_xnew(char *, *n_values);

    for (i = 0; i < *n_values; i++) {
        o1 = json_object_array_get_idx(o, i);

        if (json_object_get_type(o1) != json_type_string) {
            json_object_put(o1);
            goto out;
        }

        (*values)[i] = pa_xstrdup(json_object_get_string(o1));
        json_object_put(o1);
    }

    ret = 0;

out:
    json_object_put(o);
    return ret;
}

void pa_format_info_free_string_array(char **values, int n_values) {
    int i;

    for (i = 0; i < n_values; i++)
        pa_xfree(values[i]);

    pa_xfree(values);
}

void pa_format_info_set_sample_format(pa_format_info *f, pa_sample_format_t sf) {
    pa_format_info_set_prop_string(f, PA_PROP_FORMAT_SAMPLE_FORMAT, pa_sample_format_to_string(sf));
}

void pa_format_info_set_rate(pa_format_info *f, int rate) {
    pa_format_info_set_prop_int(f, PA_PROP_FORMAT_RATE, rate);
}

void pa_format_info_set_channels(pa_format_info *f, int channels) {
    pa_format_info_set_prop_int(f, PA_PROP_FORMAT_CHANNELS, channels);
}

void pa_format_info_set_channel_map(pa_format_info *f, const pa_channel_map *map) {
    char map_str[PA_CHANNEL_MAP_SNPRINT_MAX];

    pa_channel_map_snprint(map_str, sizeof(map_str), map);

    pa_format_info_set_prop_string(f, PA_PROP_FORMAT_CHANNEL_MAP, map_str);
}

void pa_format_info_set_prop_int(pa_format_info *f, const char *key, int value) {
    json_object *o;

    pa_assert(f);
    pa_assert(key);

    o = json_object_new_int(value);

    pa_proplist_sets(f->plist, key, json_object_to_json_string(o));

    json_object_put(o);
}

void pa_format_info_set_prop_int_array(pa_format_info *f, const char *key, const int *values, int n_values) {
    json_object *o;
    int i;

    pa_assert(f);
    pa_assert(key);

    o = json_object_new_array();

    for (i = 0; i < n_values; i++)
        json_object_array_add(o, json_object_new_int(values[i]));

    pa_proplist_sets(f->plist, key, json_object_to_json_string(o));

    json_object_put(o);
}

void pa_format_info_set_prop_int_range(pa_format_info *f, const char *key, int min, int max) {
    json_object *o;

    pa_assert(f);
    pa_assert(key);

    o = json_object_new_object();

    json_object_object_add(o, PA_JSON_MIN_KEY, json_object_new_int(min));
    json_object_object_add(o, PA_JSON_MAX_KEY, json_object_new_int(max));

    pa_proplist_sets(f->plist, key, json_object_to_json_string(o));

    json_object_put(o);
}

void pa_format_info_set_prop_string(pa_format_info *f, const char *key, const char *value) {
    json_object *o;

    pa_assert(f);
    pa_assert(key);

    o = json_object_new_string(value);

    pa_proplist_sets(f->plist, key, json_object_to_json_string(o));

    json_object_put(o);
}

void pa_format_info_set_prop_string_array(pa_format_info *f, const char *key, const char **values, int n_values) {
    json_object *o;
    int i;

    pa_assert(f);
    pa_assert(key);

    o = json_object_new_array();

    for (i = 0; i < n_values; i++)
        json_object_array_add(o, json_object_new_string(values[i]));

    pa_proplist_sets(f->plist, key, json_object_to_json_string(o));

    json_object_put(o);
}

static pa_bool_t pa_json_is_fixed_type(json_object *o)
{
    switch(json_object_get_type(o)) {
        case json_type_object:
        case json_type_array:
            return FALSE;

        default:
            return TRUE;
    }
}

static int pa_json_value_equal(json_object *o1, json_object *o2) {
    return (json_object_get_type(o1) == json_object_get_type(o2)) &&
        pa_streq(json_object_to_json_string(o1), json_object_to_json_string(o2));
}

static int pa_format_info_prop_compatible(const char *one, const char *two) {
    json_object *o1 = NULL, *o2 = NULL;
    int i, ret = 0;

    o1 = json_tokener_parse(one);
    if (is_error(o1))
        goto out;

    o2 = json_tokener_parse(two);
    if (is_error(o2))
        goto out;

    /* We don't deal with both values being non-fixed - just because there is no immediate need (FIXME) */
    pa_return_val_if_fail(pa_json_is_fixed_type(o1) || pa_json_is_fixed_type(o2), FALSE);

    if (pa_json_is_fixed_type(o1) && pa_json_is_fixed_type(o2)) {
        ret = pa_json_value_equal(o1, o2);
        goto out;
    }

    if (pa_json_is_fixed_type(o1)) {
        json_object *tmp = o2;
        o2 = o1;
        o1 = tmp;
    }

    /* o2 is now a fixed type, and o1 is not */

    if (json_object_get_type(o1) == json_type_array) {
        for (i = 0; i < json_object_array_length(o1); i++) {
            if (pa_json_value_equal(json_object_array_get_idx(o1, i), o2)) {
                ret = 1;
                break;
            }
        }
    } else if (json_object_get_type(o1) == json_type_object) {
        /* o1 should be a range type */
        int min, max, v;
        json_object *o_min = NULL, *o_max = NULL;

        if (json_object_get_type(o2) != json_type_int) {
            /* We don't support non-integer ranges */
            goto out;
        }

        o_min = json_object_object_get(o1, PA_JSON_MIN_KEY);
        if (!o_min || json_object_get_type(o_min) != json_type_int)
            goto out;

        o_max = json_object_object_get(o1, PA_JSON_MAX_KEY);
        if (!o_max || json_object_get_type(o_max) != json_type_int)
            goto out;

        v = json_object_get_int(o2);
        min = json_object_get_int(o_min);
        max = json_object_get_int(o_max);

        ret = v >= min && v <= max;
    } else {
        pa_log_warn("Got a format type that we don't support");
    }

out:
    if (o1)
        json_object_put(o1);
    if (o2)
        json_object_put(o2);

    return ret;
}
