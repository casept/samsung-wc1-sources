/* GStreamer
 * Copyright (C) 2010 Marc-Andre Lureau <marcandre.lureau@gmail.com>
 * Copyright (C) 2010 Andoni Morales Alastruey <ylatuya@gmail.com>
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * m3u8.h:
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

#ifndef __M3U8_H__
#define __M3U8_H__

#include <glib.h>
#include <openssl/evp.h>
#include <gst/gst.h>

G_BEGIN_DECLS typedef struct _GstM3U8 GstM3U8;
typedef struct _GstM3U8MediaFile GstM3U8MediaFile;
typedef struct _GstM3U8Key GstM3U8Key;
typedef struct _GstM3U8Client GstM3U8Client;

#define GST_M3U8(m) ((GstM3U8*)m)
#define GST_M3U8_KEY(k) ((GstM3U8Key*)k)
#define GST_M3U8_MEDIA_FILE(f) ((GstM3U8MediaFile*)f)

#define GST_M3U8_CLIENT_LOCK(c) g_mutex_lock (c->lock);
#define GST_M3U8_CLIENT_UNLOCK(c) g_mutex_unlock (c->lock);

#define GST_M3U8_IV_LEN 16

GST_DEBUG_CATEGORY_EXTERN (hlsdemux2_m3u8_debug);

typedef enum {
  GST_M3U8_ENCRYPTED_NONE,
  GST_M3U8_ENCRYPTED_AES_128,
} GstM3U8EncryptionMethod;

struct _GstM3U8
{
  gchar *uri;

  gboolean endlist;             /* if ENDLIST has been reached */
  gint version;                 /* last EXT-X-VERSION */
  GstClockTime targetduration;  /* last EXT-X-TARGETDURATION */
  gchar *allowcache;            /* last EXT-X-ALLOWCACHE */

  gint bandwidth;
  gint program_id;
  gchar *codecs;
  gint width;
  gint height;
  GList *files;

  /*< private > */
  gchar *last_data;
  GList *lists;                 /* list of GstM3U8 from the main playlist */
  GList *current_variant;       /* Current variant playlist used */
  GstM3U8 *parent;              /* main playlist (if any) */
  guint mediasequence;          /* EXT-X-MEDIA-SEQUENCE & increased with new media file */
};

struct _GstM3U8MediaFile
{
  gchar *title;
  GstClockTime duration;
  gchar *uri;
  guint sequence;               /* the sequence nb of this file */
  gchar *key_url;
  unsigned char *key;
  unsigned char *iv;
};

struct _GstM3U8Key
{
  GstM3U8EncryptionMethod method;
  gchar *uri;
  guint8 *iv;
  guint sequence;
  guint8 *data;
};

struct _GstM3U8Client
{
  GstM3U8 *main;                /* main playlist */
  GstM3U8 *current;
  guint update_failed_count;
  gint sequence;                /* the next sequence for this client */
  GMutex *lock;
  EVP_CIPHER_CTX cipher_ctx;
};


GstM3U8Client *gst_m3u8_client_new (const gchar * uri);
void gst_m3u8_client_free (GstM3U8Client * client);
gboolean gst_m3u8_client_update (GstM3U8Client * client, gchar * data);
void gst_m3u8_client_set_current (GstM3U8Client * client, GstM3U8 * m3u8);
gboolean gst_m3u8_client_get_next_fragment (GstM3U8Client * client, GstClockTime cur_running_time,
    gboolean * discontinuity, const gchar ** uri, GstClockTime * duration,
    GstClockTime * timestamp, gchar **key_uri, gchar **iv);
void gst_m3u8_client_get_current_position (GstM3U8Client * client,
    GstClockTime * timestamp);
GstClockTime gst_m3u8_client_get_duration (GstM3U8Client * client);
GstClockTime gst_m3u8_client_get_target_duration (GstM3U8Client * client);
const gchar *gst_m3u8_client_get_uri(GstM3U8Client * client);
const gchar *gst_m3u8_client_get_current_uri(GstM3U8Client * client);
gboolean gst_m3u8_client_has_variant_playlist(GstM3U8Client * client);
gboolean gst_m3u8_client_is_live(GstM3U8Client * client);
GList * gst_m3u8_client_get_playlist_for_bitrate (GstM3U8Client * client,
    guint bitrate);
gboolean gst_m3u8_client_decrypt_init (GstM3U8Client * client, unsigned char *key, gchar *iv);
gboolean gst_m3u8_client_decrypt_update (GstM3U8Client * client,
    guint8 * out_data, gint * out_size, guint8 * in_data, gint in_size);
gboolean gst_m3u8_client_is_playlist_download_needed (GstM3U8Client * self);
GstClockTime gst_m3u8_client_get_last_fragment_duration (GstM3U8Client * client);
gint gst_m3u8_client_get_current_bandwidth (GstM3U8Client * client);
gboolean gst_m3u8_client_decrypt_final (GstM3U8Client * client, guint8 * out_data, gint * out_size);
gboolean gst_m3u8_client_decrypt_deinit (GstM3U8Client * client);
void gst_m3u8_client_decrement_sequence (GstM3U8Client * client);
GList *gst_m3u8_client_get_next_higher_bw_playlist (GstM3U8Client * client);
GList *gst_m3u8_client_get_next_lower_bw_playlist (GstM3U8Client * client);

G_END_DECLS
#endif /* __M3U8_H__ */
