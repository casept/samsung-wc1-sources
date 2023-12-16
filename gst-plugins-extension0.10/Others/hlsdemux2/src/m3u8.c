/* GStreamer
 * Copyright (C) 2010 Marc-Andre Lureau <marcandre.lureau@gmail.com>
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * m3u8.c:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <glib.h>
#include <string.h>
#include "m3u8.h"

static GstM3U8 *gst_m3u8_new (void);
static void gst_m3u8_free (GstM3U8 * m3u8);
static gboolean gst_m3u8_update (GstM3U8 * m3u8, gchar * data,
    gboolean * updated);
static GstM3U8MediaFile *
gst_m3u8_media_file_new (gchar * uri, gchar * title, GstClockTime duration,
    gchar *key_url, gchar *IV, guint sequence);
static void gst_m3u8_media_file_free (GstM3U8MediaFile * self);
static void gst_m3u8_key_free (GstM3U8Key * self);

#define GST_CAT_DEFAULT hlsdemux2_m3u8_debug

#define BITRATE_SWITCH_UPPER_THRESHOLD 0.3
#define BITRATE_SWITCH_LOWER_THRESHOLD 0
#define SWITCH_TRIGGER_POINT 1

static GstM3U8 *
gst_m3u8_new (void)
{
  GstM3U8 *m3u8;

  m3u8 = g_new0 (GstM3U8, 1);
  m3u8->last_data = NULL;
  return m3u8;
}

static void
gst_m3u8_set_uri (GstM3U8 * self, gchar * uri)
{
  g_return_if_fail (self != NULL);

  if (self->uri)
    g_free (self->uri);
  self->uri = uri;
}

static void
gst_m3u8_free (GstM3U8 * self)
{
  g_return_if_fail (self != NULL);

  g_free (self->uri);
  g_free (self->allowcache);
  g_free (self->codecs);
  //g_free (self->cipher_ctx);

  g_list_foreach (self->files, (GFunc) gst_m3u8_media_file_free, NULL);
  g_list_free (self->files);

  g_free (self->last_data);
  g_list_foreach (self->lists, (GFunc) gst_m3u8_free, NULL);
  g_list_free (self->lists);

  g_free (self);
}

static GstM3U8MediaFile *
gst_m3u8_media_file_new (gchar * uri, gchar * title, GstClockTime duration,
    gchar *key_url, gchar *IV, guint sequence)
{
  GstM3U8MediaFile *file;

  file = g_new0 (GstM3U8MediaFile, 1);
  file->uri = uri;
  file->title = title;
  file->duration = duration;
  file->sequence = sequence;

  GST_DEBUG ("Duration of the file = %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(duration));

  if (key_url != NULL)
    file->key_url = g_strdup (key_url);
  else
    file->key_url = NULL;

  file->iv = IV;

  return file;
}

static void
gst_m3u8_media_file_free (GstM3U8MediaFile * self)
{
  g_return_if_fail (self != NULL);

  if (self->key_url) {
    g_free (self->key_url);
    self->key_url = NULL;
  }
  if (self->iv) {
    g_free (self->iv);
    self->iv = NULL;
  }
  if (self->title) {
    g_free (self->title);
    self->title = NULL;
  }
  if (self->uri){
    g_free (self->uri);
    self->uri = NULL;
  }
  g_free (self);
}

static gchar *
gst_m3u8_getIV_from_mediasequence (GstM3U8 *self)
{
  gchar *IV = NULL;

  IV = g_malloc0 (16);
  if (!IV) {
    GST_ERROR ("Failed to allocate memory...");
    return NULL;
  }

  if (self->mediasequence > INT_MAX) {
    GST_ERROR ("media sequnece is greater than INT_MAX...yet to handle");
  }

  IV [15] = (gchar)(self->mediasequence);
  IV [14] = (gchar)(self->mediasequence >> 8);
  IV [13] = (gchar)(self->mediasequence >> 16);
  IV [12] = (gchar)(self->mediasequence >> 24);
  return IV;
}

static gboolean
int_from_string (gchar * ptr, gchar ** endptr, gint * val, gint base)
{
  gchar *end;

  g_return_val_if_fail (ptr != NULL, FALSE);
  g_return_val_if_fail (val != NULL, FALSE);

  errno = 0;
  *val = strtol (ptr, &end, base);
  if ((errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN))
      || (errno != 0 && *val == 0)) {
    GST_WARNING ("%s", g_strerror (errno));
    return FALSE;
  }

  if (endptr)
    *endptr = end;

  return end != ptr;
}

static gboolean
double_from_string (gchar * ptr, gchar ** endptr, gdouble * val)
{
  gchar *end;
  gdouble ret;

  g_return_val_if_fail (ptr != NULL, FALSE);
  g_return_val_if_fail (val != NULL, FALSE);

  errno = 0;
  ret = g_strtod (ptr, &end);
  if ((errno == ERANGE && (ret == HUGE_VAL || ret == -HUGE_VAL))
      || (errno != 0 && ret == 0)) {
    GST_WARNING ("%s", g_strerror (errno));
    return FALSE;
  }

  if (!isfinite (ret)) {
    GST_WARNING ("%s", g_strerror (ERANGE));
    return FALSE;
  }

  if (endptr)
    *endptr = end;

  *val = (gdouble) ret;

  return end != ptr;
}

static gboolean
iv_from_string (gchar * ptr, gchar ** endptr, guint8 * val)
{
  guint8 idx;
  guint8 hex;

  g_return_val_if_fail (ptr != NULL, FALSE);
  g_return_val_if_fail (val != NULL, FALSE);

  if (*ptr == '0' && (*(ptr + 1) == 'x' || *(ptr + 1) == 'X')) {
    ptr = ptr + 2;  /* skip 0x or 0X */
  }

  for (idx = 0; idx < GST_M3U8_IV_LEN; idx++) {
    hex = *ptr++ - '0';
    if (hex >= 16)
      return FALSE;
    *(val + idx) = hex * 16;
    hex = *ptr++ - '0';
    if (hex >= 16)
      return FALSE;
    *(val + idx) += hex;
  }

  if (endptr)
    *endptr = ptr;

  return TRUE;
}

static gboolean
iv_from_uint (guint uint, guint8 * val)
{
  guint8 idx;

  g_return_val_if_fail (val != NULL, FALSE);

  val = val + GST_M3U8_IV_LEN - 1;
  for (idx = 0; idx < sizeof(guint); idx++) {
    *val-- = (guint8)uint;
    uint = uint >> 8;
  }
  return TRUE;
}

static gboolean
make_valid_uri (gchar *prefix, gchar *suffix, gchar **uri)
{
  gchar *slash;

  if (!prefix) {
    GST_WARNING ("uri prefix not set, can't build a valid uri");
    return FALSE;
  }
  slash = g_utf8_strrchr (prefix, -1, '/');
  if (!slash) {
    GST_WARNING ("Can't build a valid uri");
    return FALSE;
  }

  *slash = '\0';
  *uri = g_strdup_printf ("%s/%s", prefix, suffix);
  *slash = '/';

  return TRUE;
}

static gboolean
parse_attributes (gchar ** ptr, gchar ** a, gchar ** v)
{
  gchar *end = NULL, *p;

  g_return_val_if_fail (ptr != NULL, FALSE);
  g_return_val_if_fail (*ptr != NULL, FALSE);
  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (v != NULL, FALSE);

  /* [attribute=value,]* */

  *a = *ptr;
  end = p = g_utf8_strchr (*ptr, -1, ',');
/*
For #EXT-X-KEY with special kind of URI ( having , in between ) ( seen during MySpace.com site video playback )
We need to search for "" instead of , for finding the next attribute. Modified the logic for that.
*/
  if (end) {
    gchar *q = g_utf8_strchr (*ptr, -1, '"');
    if (q && q < end) {
      q = g_utf8_next_char (q);
      if (q) {
        q = g_utf8_strchr (q, -1, '"');
      }
      if (q) {
        end = p = g_utf8_strchr (q, -1, ',');
      }
    }
  }
  if (end) {
    do {
      end = g_utf8_next_char (end);
    } while (end && *end == ' ');
    *p = '\0';
  }

  *v = p = g_utf8_strchr (*ptr, -1, '=');
  if (*v) {
    *v = g_utf8_next_char (*v);
    *p = '\0';
  } else {
    GST_WARNING ("missing = after attribute");
    return FALSE;
  }

  *ptr = end;
  return TRUE;
}

static gint
_m3u8_compare_uri (GstM3U8 * a, gchar * uri)
{
  g_return_val_if_fail (a != NULL, 0);
  g_return_val_if_fail (uri != NULL, 0);

  return g_strcmp0 (a->uri, uri);
}

static gint
gst_m3u8_compare_playlist_by_bitrate (gconstpointer a, gconstpointer b)
{
  return ((GstM3U8 *) (a))->bandwidth - ((GstM3U8 *) (b))->bandwidth;
}

/*
 * @data: a m3u8 playlist text data, taking ownership
 */
static gboolean
gst_m3u8_update (GstM3U8 * self, gchar *data, gboolean * updated)
{
  gint val;
  GstClockTime duration = 0;
  gchar *title, *end;
//  gboolean discontinuity;
  GstM3U8 *list;
  gchar  *key_url = NULL;
  gchar *IV = NULL;
  gchar *actual_data = NULL;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (updated != NULL, FALSE);

  *updated = TRUE;

  /* check if the data changed since last update */
  if (self->last_data && g_str_equal (self->last_data, data)) {
    GST_DEBUG ("Playlist is the same as previous one");
    *updated = FALSE;
    g_free (data);
    return TRUE;
  }

  if (!g_str_has_prefix (data, "#EXTM3U")) {
    GST_WARNING ("Data doesn't start with #EXTM3U");
    *updated = FALSE;
    g_free (data);
    return FALSE;
  }

  if (self->last_data)
    g_free (self->last_data);

  self->last_data = g_strdup(data);

  actual_data = data;

  if (self->files) {
    g_list_foreach (self->files, (GFunc) gst_m3u8_media_file_free, NULL);
    g_list_free (self->files);
    self->files = NULL;
  }

  list = NULL;
  duration = 0;
  title = NULL;
  data += 7;

  while (TRUE) {

    end = g_utf8_strchr (data, -1, '\n');
    if (end)
      *end = '\0';

    if (data[0] != '#') {
      gchar *r;
      gchar *uri = NULL;

      if (duration <= 0 && list == NULL) {
        GST_LOG ("%s: got line without EXTINF or EXTSTREAMINF, dropping", data);
        goto next_line;
      }

      if (!gst_uri_is_valid (data)) {
        if (!make_valid_uri(self->uri, data, &uri))
          goto next_line;
      } else {
        uri = g_strdup (data);
      }

      r = g_utf8_strchr (uri, -1, '\r');
      if (r)
        *r = '\0';

      if (list != NULL) {
        if (g_list_find_custom (self->lists, uri, (GCompareFunc) _m3u8_compare_uri)) {
          GST_DEBUG ("Already have a list with this URI");
          gst_m3u8_free (list);
          g_free (uri);
        } else {
          gst_m3u8_set_uri (list, uri);
          self->lists = g_list_append (self->lists, list);
        }
        list = NULL;
      } else {
        GstM3U8MediaFile *file;
        gchar *send_IV = NULL;

        if (key_url) {
          GST_DEBUG ("AES-128 key url = %s", key_url);
          if (NULL == IV) {
            /* IV is not present in EXT-X-KEY tag. Prepare IV based on mediasequence */
            GST_DEBUG ("IV is not in EXT-X-KEY tag... generating from media_seq_num = %d", self->mediasequence);
            send_IV = gst_m3u8_getIV_from_mediasequence (self);
          } else {
            int i=0;
            send_IV = g_malloc0(16);
            for(i=0;i<16;i++){
              send_IV[i]=IV[i];
            }
          }
        }

        file =
            gst_m3u8_media_file_new (uri, title, duration, key_url, send_IV,
            self->mediasequence++);
        duration = 0;
        title = NULL;
        send_IV = NULL;

        self->files = g_list_append (self->files, file);
        GST_DEBUG ("------------>>>>>>appending uri = %s and key = %s\n", file->uri, file->key_url);
      }
    } else if (g_str_has_prefix (data, "#EXT-X-ENDLIST")) {
      GST_DEBUG ("*************** End list is present ****************");
      self->endlist = TRUE;
    } else if (g_str_has_prefix (data, "#EXT-X-VERSION:")) {
      if (int_from_string (data + 15, &data, &val, 10))
        self->version = val;
    } else if (g_str_has_prefix (data, "#EXT-X-STREAM-INF:")) {
      gchar *v, *a;

      if (list != NULL) {
        GST_WARNING ("Found a list without a uri..., dropping");
        gst_m3u8_free (list);
      }

      list = gst_m3u8_new ();
      data = data + 18;
      while (data && parse_attributes (&data, &a, &v)) {
        if (g_str_equal (a, "BANDWIDTH")) {
          if (!int_from_string (v, NULL, &list->bandwidth, 10))
            GST_WARNING ("Error while reading BANDWIDTH");
        } else if (g_str_equal (a, "PROGRAM-ID")) {
          if (!int_from_string (v, NULL, &list->program_id, 10))
            GST_WARNING ("Error while reading PROGRAM-ID");
        } else if (g_str_equal (a, "CODECS")) {
          g_free (list->codecs);
          list->codecs = g_strdup (v);
        } else if (g_str_equal (a, "RESOLUTION")) {
          if (!int_from_string (v, &v, &list->width, 10))
            GST_WARNING ("Error while reading RESOLUTION width");
          if (!v || *v != '=') {
            GST_WARNING ("Missing height");
          } else {
            v = g_utf8_next_char (v);
            if (!int_from_string (v, NULL, &list->height, 10))
              GST_WARNING ("Error while reading RESOLUTION height");
          }
        }
      }
    } else if (g_str_has_prefix (data, "#EXT-X-TARGETDURATION:")) {
      if (int_from_string (data + 22, &data, &val, 10))
        self->targetduration = val * GST_SECOND;
    } else if (g_str_has_prefix (data, "#EXT-X-MEDIA-SEQUENCE:")) {
      if (int_from_string (data + 22, &data, &val, 10))
        self->mediasequence = val;
    } else if (g_str_has_prefix (data, "#EXT-X-DISCONTINUITY")) {
      /* discontinuity = TRUE; */
    } else if (g_str_has_prefix (data, "#EXT-X-PROGRAM-DATE-TIME:")) {
      /* <YYYY-MM-DDThh:mm:ssZ> */
      GST_DEBUG ("FIXME parse date");
    } else if (g_str_has_prefix (data, "#EXT-X-ALLOW-CACHE:")) {
      g_free (self->allowcache);
      self->allowcache = g_strdup (data + 19);
    } else if (g_str_has_prefix (data, "#EXTINF:")) {
      gdouble fval;
      if (!double_from_string (data + 8, &data, &fval)) {
        GST_WARNING ("Can't read EXTINF duration");
        goto next_line;
      }
      duration = fval * (gdouble) GST_SECOND;
      if (duration > self->targetduration)
        GST_WARNING ("EXTINF duration > TARGETDURATION");
      if (!data || *data != ',')
        goto next_line;
      data = g_utf8_next_char (data);
      if (data != end) {
        g_free (title);
        title = g_strdup (data);
      }
    } else if (g_str_has_prefix (data, "#EXT-X-KEY:"))  {
      gchar *val, *attr;
      GST_DEBUG ("Found EXT-X-KEY tag...");

      /* handling encrypted content */
      data = data + 11; /* skipping "#EXT-X-KEY:" tag */

      while (data && parse_attributes (&data, &attr, &val)) {
        if (g_str_equal (attr, "METHOD")) {
          if (g_str_equal (val, "NONE")) {
            GST_LOG ("Non encrypted file...and skipping current line and going to next line\n");
            goto next_line;
          } else if (g_str_equal (val, "AES-128")) {
            /* media files are encrypted */
            GST_LOG ("media files are encrypted with AES-128\n");
            // TODO: indicate in flag whether encrypted files or not
          }
        } else if (g_str_equal (attr, "URI")) {
          gchar *end_dq = NULL;

          if (!val){
            GST_ERROR ("val is NULL");
            break;
          }

          val = val + 1; /* eliminating first double quote in url */
          if (!val) {
            GST_ERROR ("val is NULL");
            break;
          }

          end_dq = g_utf8_strrchr (val, -1, '"');
          if (!end_dq) {
            GST_ERROR ("end_dq is NULL");
            break;
          }

          *end_dq = '\0';
          GST_DEBUG ("Key URI = %s\n", val);

          if (!gst_uri_is_valid (val)) {
            gchar *slash;
            if (!self->uri) {
              GST_WARNING ("uri not set, can't build a valid uri");
              goto next_line;
            }
            slash = g_utf8_strrchr (self->uri, -1, '/');
            if (!slash) {
              GST_WARNING ("Can't build a valid uri");
              goto next_line;
            }
            *slash = '\0';
            key_url = g_strdup_printf ("%s/%s", self->uri, val);
            *slash = '/';
          } else {
            key_url= g_strdup (val);
          }
        } else if (g_str_equal (attr, "IV")) {
          gint iv_len = 0;
          gchar tmp_byte[3];
          gint tmp_val = 0;
          gint idx = 0;
          gchar *car = NULL;

          if (IV) {
            g_free (IV);
            IV = NULL;
          }

          IV = g_malloc0 (16);
          if (NULL == IV) {
            GST_ERROR ("Failed to allocate memory...\n");
            goto error;
          }

          /* eliminating 0x/0X prefix */
          val = val + 2;
          car = g_utf8_strchr (val, -1, '\r');
          if (car)
            *car = '\0';

          iv_len = g_utf8_strlen(val, -1);
          if (iv_len < 1 || iv_len >32) {
            GST_ERROR ("Wrong IV and iv_len = %d", iv_len);
            goto error;
          }

          val+=iv_len;
          idx = 15;
          while (iv_len > 0) {
            // TODO: val need to incremented I feel.. check again
            val-=(iv_len==1)?1:2;
            g_utf8_strncpy(tmp_byte, val, (iv_len==1)?1:2);
            tmp_byte[2] = '\0';
            tmp_val = 0;
            if (!int_from_string (tmp_byte, NULL, &tmp_val, 16))
              GST_WARNING ("Error while reading PROGRAM-ID");
            IV[idx] = tmp_val;
            idx--;
            iv_len = iv_len - 2;
          }
        }
      }
    } else {
      GST_LOG ("Ignored line: %s", data);
    }

  next_line:
    if (!end)
      break;
    data = g_utf8_next_char (end);      /* skip \n */
  }

  /* redorder playlists by bitrate */
  if (self->lists) {
    gchar *top_variant_uri = NULL;

    if (!self->current_variant)
      top_variant_uri = GST_M3U8 (self->lists->data)->uri;
    else
      top_variant_uri = GST_M3U8 (self->current_variant->data)->uri;

    self->lists =
        g_list_sort (self->lists,
        (GCompareFunc) gst_m3u8_compare_playlist_by_bitrate);

    self->current_variant = g_list_find_custom (self->lists, top_variant_uri,
        (GCompareFunc) _m3u8_compare_uri);
  }

  g_free (actual_data);

  return TRUE;

error:
  g_free (actual_data);
  return FALSE;
}


GstM3U8Client *
gst_m3u8_client_new (const gchar * uri)
{
  GstM3U8Client *client;

  g_return_val_if_fail (uri != NULL, NULL);

  client = g_new0 (GstM3U8Client, 1);
  client->main = gst_m3u8_new ();
  client->current = NULL;
  client->sequence = -1;
  client->update_failed_count = 0;
  client->lock = g_mutex_new ();
  gst_m3u8_set_uri (client->main, g_strdup (uri));

  return client;
}

void
gst_m3u8_client_free (GstM3U8Client * self)
{
  g_return_if_fail (self != NULL);

  gst_m3u8_free (self->main);
  g_mutex_free (self->lock);
  g_free (self);
}

void
gst_m3u8_client_set_current (GstM3U8Client * self, GstM3U8 * m3u8)
{
  g_return_if_fail (self != NULL);

  GST_M3U8_CLIENT_LOCK (self);
  if (m3u8 != self->current) {
    self->current = m3u8;
    self->update_failed_count = 0;
  }
  GST_M3U8_CLIENT_UNLOCK (self);
}

static guint
_get_start_sequence (GstM3U8Client * client)
{
  GList *l;
  GstClockTime duration_limit, duration_count = 0;
  guint sequence = -1;

  if (client->current->endlist) {
    l = g_list_first (client->current->files);
    sequence = GST_M3U8_MEDIA_FILE (l->data)->sequence;
  } else {
    duration_limit = client->current->targetduration * 3;
    for (l = g_list_last (client->current->files); l; l = l->prev) {
      duration_count += GST_M3U8_MEDIA_FILE (l->data)->duration;
      sequence = GST_M3U8_MEDIA_FILE (l->data)->sequence;
      if (duration_count >= duration_limit) {
        break;
      }
    }
  }
  return sequence;
}

gboolean
gst_m3u8_client_update (GstM3U8Client * self, gchar * data)
{
  GstM3U8 *m3u8;
  gboolean updated = FALSE;
  gboolean ret = FALSE;

  g_return_val_if_fail (self != NULL, FALSE);

  GST_M3U8_CLIENT_LOCK (self);
  m3u8 = self->current ? self->current : self->main;

  if (!gst_m3u8_update (m3u8, data, &updated))
    goto out;

  if (!updated) {
    self->update_failed_count++;
    goto out;
  }

  if (self->current && !self->current->files) {
    GST_ERROR ("Invalid media playlist, it does not contain any media files");
    goto out;
  }

  /* select the first playlist, for now */
  if (!self->current) {
    if (self->main->lists) {
      self->current = self->main->current_variant->data;
    } else {
      self->current = self->main;
    }
  }

  if (m3u8->files && self->sequence == -1) {
#if 0
    self->sequence = GST_M3U8_MEDIA_FILE (g_list_first (m3u8->files)->data)->sequence;
#else
    /* (spec : 6.3.3) In live case, the client SHOULD NOT choose a segment
        which starts less than three target durations from the end of the Playlist file */
    self->sequence = _get_start_sequence (self);
#endif
    GST_DEBUG ("Setting first sequence at %d", self->sequence);
  }

  ret = TRUE;
out:
  GST_M3U8_CLIENT_UNLOCK (self);
  return ret;
}

static gboolean
_find_next (GstM3U8MediaFile * file, GstM3U8Client * client)
{
  GST_DEBUG ("Found fragment %d", file->sequence);
  if (file->sequence >= client->sequence)
    return FALSE;
  return TRUE;
}

static gboolean
_find_key (GstM3U8Key * key, GstM3U8Client * client)
{
  if (key->sequence <= client->sequence)
    return FALSE;
  return TRUE;
}

void
gst_m3u8_client_get_current_position (GstM3U8Client * client,
    GstClockTime * timestamp)
{
  GList *l;
  GList *walk;

  l = g_list_find_custom (client->current->files, client,
      (GCompareFunc) _find_next);

  *timestamp = 0;
  for (walk = client->current->files; walk; walk = walk->next) {
    if (walk == l)
      break;
    *timestamp += GST_M3U8_MEDIA_FILE (walk->data)->duration;
  }
}

gboolean
gst_m3u8_client_get_next_fragment (GstM3U8Client * client, GstClockTime cur_running_time,
    gboolean * discontinuity, const gchar ** uri, GstClockTime * duration,
    GstClockTime * timestamp, gchar **key_uri, gchar **iv)
{
  GList *l;
  GstM3U8MediaFile *file;

  g_return_val_if_fail (client != NULL, FALSE);
  g_return_val_if_fail (client->current != NULL, FALSE);
  g_return_val_if_fail (discontinuity != NULL, FALSE);

  GST_M3U8_CLIENT_LOCK (client);

  //If the Client Sequence is -1 , that means playlist is not properly retrieved. So return false.
  GST_DEBUG ("Looking for fragment %d", client->sequence);
  if ((client->sequence < 0)||(!client->current->files)) {
    GST_ERROR ("Sequence Value is Invalid or Invalid File Pointer !!");
    GST_M3U8_CLIENT_UNLOCK (client);
    return FALSE;
  }

  if (client->main && client->main->lists && client->current->endlist) {
    GList *walk = NULL;
    GstClockTime current_pos, target_pos;
    gint current_sequence;
    gboolean found_sequence = FALSE;

    /* VOD playlist, so find mediasequence from time */
    // TODO: need to handle when live converts to VOD by putting END_LIST tag

    file = GST_M3U8_MEDIA_FILE (client->current->files->data);
    current_sequence = file->sequence;
    current_pos = 0 ;
    target_pos = cur_running_time;

    for (walk = client->current->files; walk; walk = walk->next) {
      file = walk->data;

      current_sequence = file->sequence;
      if (current_pos <= target_pos &&
        target_pos < current_pos + file->duration) {
        found_sequence = TRUE;
        break;
      }
      current_pos += file->duration;
    }

    if (found_sequence) {
      GST_ERROR ("changing client sequence from %d -> %d", client->sequence, current_sequence);
      client->sequence = current_sequence;
    } else {
      GST_WARNING ("max file_seq = %d and client_seq = %d", current_sequence, client->sequence);
      GST_M3U8_CLIENT_UNLOCK (client);
      return FALSE;
    }
  }

  l = g_list_find_custom (client->current->files, client, (GCompareFunc) _find_next);
  if (l == NULL) {
    GST_M3U8_CLIENT_UNLOCK (client);
    return FALSE;
  }

  gst_m3u8_client_get_current_position (client, timestamp);

  file = GST_M3U8_MEDIA_FILE (l->data);

  if (file->key_url)
    *key_uri = g_strdup (file->key_url);

  if (file->iv) {
    *iv = g_malloc0 (GST_M3U8_IV_LEN);
    //*iv = file->iv;
    memcpy (*iv, file->iv, GST_M3U8_IV_LEN);
  }

  *discontinuity = client->sequence != file->sequence;
  client->sequence = file->sequence + 1;

  if (file->uri)
    *uri = g_strdup (file->uri);

  *duration = file->duration;

  GST_M3U8_CLIENT_UNLOCK (client);
  return TRUE;
}

void gst_m3u8_client_decrement_sequence (GstM3U8Client * client)
{
  GST_M3U8_CLIENT_LOCK (client);
  client->sequence = client->sequence - 1;
  GST_M3U8_CLIENT_UNLOCK (client);
}

static void
_sum_duration (GstM3U8MediaFile * self, GstClockTime * duration)
{
  *duration += self->duration;
}

GstClockTime
gst_m3u8_client_get_duration (GstM3U8Client * client)
{
  GstClockTime duration = 0;

  g_return_val_if_fail (client != NULL, GST_CLOCK_TIME_NONE);

  GST_M3U8_CLIENT_LOCK (client);
  /* We can only get the duration for on-demand streams */
  if (!client->current || !client->current->endlist) {
    GST_M3U8_CLIENT_UNLOCK (client);
    return GST_CLOCK_TIME_NONE;
  }
  if (client->current->files)
     g_list_foreach (client->current->files, (GFunc) _sum_duration, &duration);

  GST_M3U8_CLIENT_UNLOCK (client);
  return duration;
}

GstClockTime
gst_m3u8_client_get_target_duration (GstM3U8Client * client)
{
  GstClockTime duration = 0;

  g_return_val_if_fail (client != NULL, GST_CLOCK_TIME_NONE);

  GST_M3U8_CLIENT_LOCK (client);
  duration = client->current->targetduration;
  GST_M3U8_CLIENT_UNLOCK (client);
  return duration;
}

/* used for LIVE case only, to update playlist file */
GstClockTime
gst_m3u8_client_get_last_fragment_duration (GstM3U8Client * client)
{
  GstClockTime duration = 0;
  GstM3U8MediaFile *last_file = NULL;
  GList *last_list = NULL;

  g_return_val_if_fail (client != NULL, GST_CLOCK_TIME_NONE);

  GST_M3U8_CLIENT_LOCK (client);

  if (!client->current) {
    GST_ERROR ("current variant is NULL.. return CLOCK_NONE");
    GST_M3U8_CLIENT_UNLOCK (client);
    return GST_CLOCK_TIME_NONE;
  }

  if (!client->current->files) {
    GST_WARNING ("Due to playlist updation problem.. did not update the playlist");
    GST_M3U8_CLIENT_UNLOCK (client);
    return 0; /* update playlist immediately */
  }

  last_list = g_list_last (client->current->files);

  if (!last_list) {
    GST_WARNING ("list does not have last object... return target_duration");
    GST_M3U8_CLIENT_UNLOCK (client);
    return client->current->targetduration;
  }

  last_file = (GstM3U8MediaFile *)(last_list->data);
  if(last_file)
    duration = last_file->duration;
  else
    duration=client->current->targetduration;

  GST_DEBUG ("Last file duration = %"GST_TIME_FORMAT"\n", GST_TIME_ARGS(duration));
  GST_M3U8_CLIENT_UNLOCK (client);
  return duration;
}

const gchar *
gst_m3u8_client_get_uri (GstM3U8Client * client)
{
  const gchar *uri;

  g_return_val_if_fail (client != NULL, NULL);

  GST_M3U8_CLIENT_LOCK (client);
  uri = client->main->uri;
  GST_M3U8_CLIENT_UNLOCK (client);
  return uri;
}

const gchar *
gst_m3u8_client_get_current_uri (GstM3U8Client * client)
{
  const gchar *uri;

  g_return_val_if_fail (client != NULL, NULL);

  GST_M3U8_CLIENT_LOCK (client);
  uri = client->current->uri;
  GST_M3U8_CLIENT_UNLOCK (client);
  return uri;
}

gint
gst_m3u8_client_get_current_bandwidth (GstM3U8Client * client)
{
  gint bandwidth;

  g_return_val_if_fail (client != NULL, -1);

  GST_M3U8_CLIENT_LOCK (client);
  bandwidth = client->current->bandwidth;
  GST_M3U8_CLIENT_UNLOCK (client);
  return bandwidth;
}

gboolean
gst_m3u8_client_has_variant_playlist (GstM3U8Client * client)
{
  gboolean ret;

  g_return_val_if_fail (client != NULL, FALSE);

  GST_M3U8_CLIENT_LOCK (client);
  ret = (client->main->lists != NULL);
  GST_M3U8_CLIENT_UNLOCK (client);
  return ret;
}

gboolean
gst_m3u8_client_is_live (GstM3U8Client * client)
{
  gboolean ret;

  g_return_val_if_fail (client != NULL, FALSE);

  GST_M3U8_CLIENT_LOCK (client);
  if (!client->current || client->current->endlist)
    ret = FALSE;
  else
    ret = TRUE;
  GST_M3U8_CLIENT_UNLOCK (client);
  return ret;
}

GList *
gst_m3u8_client_get_next_higher_bw_playlist (GstM3U8Client * client)
{
  GList *current_variant;

  g_return_val_if_fail (client != NULL, FALSE);

  GST_M3U8_CLIENT_LOCK (client);
  current_variant = g_list_next (client->main->current_variant);
  if (!current_variant) {
    GST_WARNING ("no next variant available...");
    GST_M3U8_CLIENT_UNLOCK (client);
    return NULL;
  }

  GST_DEBUG ("next variant = %p and uri = %s", current_variant, GST_M3U8(current_variant->data)->uri);
  GST_M3U8_CLIENT_UNLOCK (client);
  return current_variant;

}

GList *
gst_m3u8_client_get_next_lower_bw_playlist (GstM3U8Client * client)
{
  GList *current_variant;

  g_return_val_if_fail (client != NULL, FALSE);

  GST_M3U8_CLIENT_LOCK (client);
  current_variant = g_list_previous (client->main->current_variant);
  if (!current_variant) {
    GST_WARNING ("no previous variant available...");
    GST_M3U8_CLIENT_UNLOCK (client);
    return NULL;
  }

  GST_DEBUG ("previous variant = %p and uri = %s", current_variant, GST_M3U8(current_variant->data)->uri);
  GST_M3U8_CLIENT_UNLOCK (client);
  return current_variant;
}


#ifndef SWITCH_TRIGGER_POINT
GList *
gst_m3u8_client_get_playlist_for_bitrate (GstM3U8Client * client, guint bitrate)
{
  GList *list, *current_variant;

  GST_M3U8_CLIENT_LOCK (client);
  current_variant = client->main->current_variant;

  /*  Go to the highest possible bandwidth allowed */
  while (GST_M3U8 (current_variant->data)->bandwidth < bitrate) {
    list = g_list_next (current_variant);
    if (!list)
      break;
    current_variant = list;
  }

  while (GST_M3U8 (current_variant->data)->bandwidth > bitrate) {
    list = g_list_previous (current_variant);
    if (!list)
      break;
    current_variant = list;
  }
  GST_M3U8_CLIENT_UNLOCK (client);

  return current_variant;
}
#else
GList *
gst_m3u8_client_get_playlist_for_bitrate (GstM3U8Client * client, guint bitrate)
{
  GList *next_variant = NULL;
  GList *current_variant = NULL;
  guint current_bandwidth = 0;

  GST_M3U8_CLIENT_LOCK (client);
  current_variant = client->main->current_variant;
  current_bandwidth = GST_M3U8 (current_variant->data)->bandwidth;

  if ((current_bandwidth + BITRATE_SWITCH_UPPER_THRESHOLD * current_bandwidth) < bitrate) {
    GST_DEBUG ("Switch to upper band...\n");

    if (!g_list_next (current_variant)) {
      GST_DEBUG ("no next upper fragment...\n");
      GST_M3U8_CLIENT_UNLOCK (client);
      return current_variant;
    } else {
      GST_DEBUG (">>>>> UP : current_bw = %d, bitrate = %d\n", current_bandwidth, bitrate);
      next_variant = current_variant;

      /*  Go to the highest possible bandwidth allowed */
      while ((current_bandwidth + BITRATE_SWITCH_UPPER_THRESHOLD * current_bandwidth) < bitrate) {
        current_variant = next_variant;
        next_variant = g_list_next (next_variant);
        if (!next_variant) {
          GST_DEBUG ("no next upper fragment...\n");
          GST_M3U8_CLIENT_UNLOCK (client);
          return current_variant;
        }

        current_bandwidth = GST_M3U8 (next_variant->data)->bandwidth;
        GST_DEBUG ("current_bw in while= %d & URI = %s\n", current_bandwidth, GST_M3U8 (next_variant->data)->uri);
      }

      /* as condition failed fallback to previous */
      GST_M3U8_CLIENT_UNLOCK (client);

      return current_variant;
    }
  } else if ((current_bandwidth + BITRATE_SWITCH_LOWER_THRESHOLD * current_bandwidth) > bitrate) {
    GST_DEBUG ("Switch to lower band...\n");

    if (!g_list_previous (current_variant)) {
      GST_DEBUG ("no previous lower fragment...to go further down\n");
      GST_M3U8_CLIENT_UNLOCK (client);
      return current_variant;
    } else {

      GST_DEBUG (">>>>> LOW : current_bw = %d, bitrate = %d\n", current_bandwidth, bitrate);
      next_variant = current_variant;

      /*  Go to the lowest possible bandwidth allowed */
      while ((current_bandwidth + BITRATE_SWITCH_LOWER_THRESHOLD * current_bandwidth) > bitrate) {

        next_variant = g_list_previous (next_variant);
        if (!next_variant) {
          GST_DEBUG ("no previous lower fragment...\n");
          GST_M3U8_CLIENT_UNLOCK (client);
          return current_variant;
        }

        current_variant = next_variant;

        current_bandwidth = GST_M3U8 (current_variant->data)->bandwidth;
        GST_DEBUG ("current_bw in while= %d\n", current_bandwidth);
      }

      GST_M3U8_CLIENT_UNLOCK (client);

      /* as condition failed fallback to previous */
      return current_variant;
    }
  } else {
    GST_DEBUG ("No need to switch .... returning current_variant..\n");
  }

  GST_M3U8_CLIENT_UNLOCK (client);

  return current_variant;
}
#endif

gboolean
gst_m3u8_client_decrypt_init (GstM3U8Client * client, unsigned char *key, gchar *iv)
{
  //if (!client->current->cipher_ctx)
 //   client->current->cipher_ctx = g_malloc0 (sizeof(EVP_CIPHER_CTX));

  EVP_CIPHER_CTX_init (&(client->cipher_ctx));
  EVP_CIPHER_CTX_set_padding(&(client->cipher_ctx), 0); // added to check avodining final buffer decryption error

  return EVP_DecryptInit_ex (&(client->cipher_ctx), EVP_aes_128_cbc(), NULL, key, iv);
}

gboolean
gst_m3u8_client_decrypt_update (GstM3U8Client * client, guint8 * out_data,
    gint * out_size, guint8 * in_data, gint in_size)
{
  //g_return_val_if_fail (client->current->cipher_ctx != NULL, FALSE);

  return EVP_DecryptUpdate (&(client->cipher_ctx), out_data, out_size,
      in_data, in_size);
}

gboolean
gst_m3u8_client_decrypt_final (GstM3U8Client * client, guint8 * out_data,
    gint * out_size)
{
  //g_return_val_if_fail (client->current->cipher_ctx != NULL, FALSE);

  return EVP_DecryptFinal_ex (&(client->cipher_ctx), out_data, out_size);
}

gboolean
gst_m3u8_client_decrypt_deinit (GstM3U8Client * client)
{
  //g_return_val_if_fail (client->current->cipher_ctx != NULL, FALSE);

  return EVP_CIPHER_CTX_cleanup (&(client->cipher_ctx));
}


gboolean
gst_m3u8_client_is_playlist_download_needed (GstM3U8Client * self)
{
  /* download is not need, if last_data is present & not live (i.e. we can reuse last VOD child playlist)*/
  if (self->current->last_data && !gst_m3u8_client_is_live(self))
    return FALSE;
  else
    return TRUE;
}
