/*
 * hlsdemux2
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, Naveen Cherukuri <naveen.ch@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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


#ifndef __GST_HLSDEMUX2_H__
#define __GST_HLSDEMUX2_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "m3u8.h"

G_BEGIN_DECLS
#define GST_TYPE_HLSDEMUX2  (gst_hlsdemux2_get_type())
#define GST_HLSDEMUX2(obj)  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HLSDEMUX2, GstHLSDemux2))
#define GST_HLSDEMUX2_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_HLSDEMUX2,GstHLSDemux2Class))
#define GST_IS_HLSDEMUX2(obj)  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HLSDEMUX2))
#define GST_IS_HLSDEMUX2_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_HLSDEMUX2))

typedef struct _GstHLSDemux2 GstHLSDemux2;
typedef struct _GstHLSDemux2Class GstHLSDemux2Class;
typedef struct _GstHLSDemux2Stream GstHLSDemux2Stream;
typedef struct _GstHLSDemux2PvtStream GstHLSDemux2PvtStream;

#define GST_HLS_DEMUX_AES_BLOCK_SIZE 16

#define ON_ERROR_REQUEST_AGAIN // Orange server giving soup errors. so, request for fail count times

#define HLSDEMUX2_CHUNK_TIME_DURATION (0.5 * GST_SECOND)
#define HLSDEMUX2_NUM_CHUNK_DOWNLOADS_FAST_SWITCH 4
#define HLSDEMUX2_MAX_N_PAST_FRAG_DOWNLOADRATES 3

#define HLSDEMUX2_FAST_CHECK_CRITICAL_TIME_FACTOR 1.2
#define HLSDEMUX2_FAST_CHECK_WARNING_TIME_FACTOR 1

#ifdef USING_AVG_PAST_DOWNLOADRATES
#define HLSDEMUX2_MAX_N_PAST_DOWNLOADRATES 3
#endif


#define PLAYLIST_REFRESH_TIMEOUT (20 * GST_SECOND)

#define HLSDEMUX2_OVERHEAD (1 * GST_SECOND)

#define LATEST_AV_SYNC

typedef enum
{
  HLSDEMUX2_STREAM_VIDEO = 0,
  HLSDEMUX2_STREAM_AUDIO,
  HLSDEMUX2_STREAM_TEXT,
  HLSDEMUX2_STREAM_PRIVATE,
  HLSDEMUX2_STREAM_NUM,
}HLSDEMUX2_STREAM_TYPE;

typedef enum
{
  HLSDEMUX2_DOWNWARD_RECOVERY = 0,
  HLSDEMUX2_UPWARD_RECOVERY,
  HLSDEMUX2_NO_RECOVERY,
}HLSDEMUX2_RECOVERY_MODE;

typedef enum
{
  HLSDEMUX2_MULTI_VARIANT = 0,
  HLSDEMUX2_SINGLE_VARIANT
}HLSDEMUX2_STREAM_CONFIGURATION;

typedef struct
{
  GstBin *sinkbin;
  GstElement *parser; /*only for .aac stream */
  GstElement *queue;
  GstElement *sink;
}HLSDemux2SinkBin;

typedef struct
{
  GstElement *pipe;
  GstElement *urisrc;
  GstElement *typefind;
  GstElement *queue;
  GstElement *demuxer;
  GList *sinkbins;

  guint64 content_size;
  gboolean applied_fast_switch;

  gchar *remaining_data;
  guint remaining_size;

  GMutex *lock;
  GCond *cond;
  gboolean is_encrypted;

  gulong src_bprobe;
  gboolean get_next_frag; /* on error like NOT_FOUND, request next fragment */

  guint cur_stream_cnt;
  gboolean first_buffer;
  gboolean force_timestamps; /* useful in .aac files playback */
  gboolean error_rcvd;
  gboolean find_mediaseq;
  GstClockTime seeked_pos;
  GstClockTime cur_running_dur;

  /* for fragment download rate calculation */
  guint64 download_rate;
  GstClockTime download_start_ts;
  GstClockTime download_stop_ts;
  guint64 src_downloaded_size;
  guint64 queue_downloaded_size;
  gint ndownloaded; /* number of fragments downloaded */
  GArray *avg_chunk_drates; /* chunk download rates array */
  GArray *avg_frag_drates; /* fragment download rates array */
  guint64 chunk_downloaded_size;
  GstClockTime chunk_start_ts;
}HLSDemux2FragDownloader;

/* playlist downloader */
typedef struct
{
  GstElement *pipe;
  GstBus *bus;
  GstElement *urisrc;
  GstElement *sink;
  GMutex *lock;
  GCond *cond;

  GstBuffer *playlist;
  HLSDEMUX2_RECOVERY_MODE recovery_mode;
  GstClockTime download_start_ts;
}HLSDemux2PLDownloader;

/* Key downloader */
typedef struct
{
  GstElement *pipe;
  GstBus *bus;
  GstElement *urisrc;
  GstElement *sink;

  GMutex *lock;
  GCond *cond;

  GstBuffer *key;
  gchar *prev_key_uri;
}HLSDemux2KeyDownloader;

/**
 * GstHLSDemux2:
 *
 * Opaque #GstHLSDemux2 data structure.
 */
struct _GstHLSDemux2
{
  GstElement parent;

  GstPad *sinkpad;

  /* M3U8 client */
  GstM3U8Client *client;

  gboolean is_live;

  HLSDemux2FragDownloader *fdownloader; /* fragment downloader */
  HLSDemux2PLDownloader *pldownloader; /* playlist downloader */
  HLSDemux2KeyDownloader *kdownloader; /* key downloader */

  HLSDemux2SinkBin *gsinkbin;
  /* properties */
  gint blocksize;
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  gchar *proxy;
#endif

  /* master playlist got from upstream */
  GstBuffer *playlist;

  /* fragment download task */
  GstTask *download_task;
  GStaticRecMutex download_lock;

  /* Playlist updates task for live session */
  GStaticRecMutex updates_lock;
  GTimeVal next_update;         /* Time of the next update */

  /* playlist update condition */
  GCond *pl_update_cond;
  GMutex *pl_update_lock;

  /* array of pointers to each stream's list */
  GstHLSDemux2Stream *streams[HLSDEMUX2_STREAM_NUM];
  gint active_stream_cnt;

  gint percent; /* buffering percentage posting */
  GMutex *buffering_lock;
  gboolean is_buffering;
  GThread *buffering_posting_thread;
  GMutex *post_msg_lock;
  GCond *post_msg_start;
  GCond *post_msg_exit;

  /* HTTP request cookies. */
  gchar **lastCookie;
  gchar **playlistCookie;
  gchar **keyCookie;
  gchar **fragCookie;
  gchar *lastDomain;
  gchar *playlistDomain;
  gchar *keyDomain;
  gchar *fragDomain;

  gchar *user_agent;

  /* Properties */
  gfloat bitrate_switch_tol;    /* tolerance with respect to the fragment duration to switch the bitarate*/

  gboolean cancelled;
  gboolean end_of_playlist;

  guint64 total_cache_duration;
  guint64 target_duration;

#ifdef ON_ERROR_REQUEST_AGAIN
   /* requesting the same uron error */
  gchar *playlist_uri;
  gchar *frag_uri;  /* NOT supporting YET for fragment download errors*/
  gchar *key_uri;
#endif

  int soup_request_fail_cnt;
  /* can be removed [mainly for debugging & testing] */
  gboolean force_lower_bitrate;

  GstClockTime cfrag_dur;
  gboolean error_posted;
  gboolean flushing;
  GstClockTime ns_start;
  GstClockTime cur_audio_fts;

  GstHLSDemux2PvtStream * private_stream;
  gboolean has_image_buffer;
  HLSDEMUX2_STREAM_CONFIGURATION stream_config;
  GstBuffer *prev_image_buffer;
  GstBuffer *prev_video_buffer;
};

struct _GstHLSDemux2Class
{
  GstElementClass parent_class;
};

GType gst_hlsdemux2_get_type (void);

G_END_DECLS
#endif /* __GST_HLSDEMUX2_H__ */
