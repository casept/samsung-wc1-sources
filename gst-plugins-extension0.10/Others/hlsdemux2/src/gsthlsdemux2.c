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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* FIXME 0.11: suppress warnings for deprecated API such as GStaticRecMutex
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gst/tag/tag.h>
#include <fcntl.h>
#include <unistd.h>
#include <gst/gst.h>

#include "gsthlsdemux2.h"

#define TEMP_FIX_FOR_TASK_STOP // During Continuous Realize Unrealize , we are observing Task Join is failing. This is a defensive fix for that.

/*
Changes done for http://slp-info.sec.samsung.net/gerrit/#/c/662956/ are kept under flag for later use.Related to HLS freeze during player_prepare. Since as of now the original issue is not seen , so disabling the changes temporarily.
*/
//#define ENABLE_HLS_SYNC_MODE_HANDLER

enum
{
  PROP_0,
  PROP_BITRATE_SWITCH_TOLERANCE,
  PROP_FORCE_LOWER_BITRATE,
  PROP_BLOCKSIZE,
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  PROP_HLS_PROXY,
#endif
  PROP_LAST
};

#define HLS_VIDEO_CAPS \
  GST_STATIC_CAPS (\
    "video/mpeg, " \
      "mpegversion = (int) { 1, 2, 4 }, " \
      "systemstream = (boolean) FALSE; " \
    "video/x-h264,stream-format=(string)byte-stream," \
      "alignment=(string)nal;" \
    "video/x-dirac;" \
    "video/x-wmv," \
      "wmvversion = (int) 3, " \
      "format = (fourcc) WVC1" \
  )

#define HLS_AUDIO_CAPS \
  GST_STATIC_CAPS ( \
    "audio/mpeg, " \
      "mpegversion = (int) { 1, 4 };" \
    "audio/x-lpcm, " \
      "width = (int) { 16, 20, 24 }, " \
      "rate = (int) { 48000, 96000 }, " \
      "channels = (int) [ 1, 8 ], " \
      "dynamic_range = (int) [ 0, 255 ], " \
      "emphasis = (boolean) { FALSE, TRUE }, " \
      "mute = (boolean) { FALSE, TRUE }; " \
    "audio/x-ac3; audio/x-eac3;" \
    "audio/x-dts;" \
    "audio/x-private-ts-lpcm;" \
    "application/x-id3" \
  )

/* Can also use the subpicture pads for text subtitles? */
#define HLS_SUBPICTURE_CAPS \
    GST_STATIC_CAPS ("subpicture/x-pgs; video/x-dvd-subpicture")

static GstStaticPadTemplate hlsdemux2_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-hls"));

static GstStaticPadTemplate hlsdemux2_videosrc_template =
GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    HLS_VIDEO_CAPS);

static GstStaticPadTemplate hlsdemux2_audiosrc_template =
GST_STATIC_PAD_TEMPLATE ("audio",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    HLS_AUDIO_CAPS);

static GstStaticPadTemplate hlsdemux2_subpicture_template =
GST_STATIC_PAD_TEMPLATE ("subpicture",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    HLS_SUBPICTURE_CAPS);

static GstStaticPadTemplate hlsdemux2_private_template =
GST_STATIC_PAD_TEMPLATE ("private",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_hlsdemux2_debug);
#define GST_CAT_DEFAULT gst_hlsdemux2_debug

GST_DEBUG_CATEGORY (hlsdemux2_m3u8_debug);

static const float update_interval_factor[] = { 1, 0.5, 1.5, 3 };

#define HLS_DEFAULT_FRAME_DURATION (0.04 * GST_SECOND) // 40ms
#define DEFAULT_BLOCKSIZE (8 * 1024) // 8 kbytes
#define DEFAULT_FAST_SWITCH_BUFFER_SIZE 0.7 // factor
static void _do_init (GType type)
{
  GST_DEBUG_CATEGORY_INIT (gst_hlsdemux2_debug, "hlsdemux2", 0, "hlsdemux2 element");
  GST_DEBUG_CATEGORY_INIT (hlsdemux2_m3u8_debug, "hlsdemux2m3u8", 0, "m3u8 parser");
}

GST_BOILERPLATE_FULL (GstHLSDemux2, gst_hlsdemux2, GstElement, GST_TYPE_ELEMENT, _do_init);

#define DEFAULT_BITRATE_SWITCH_TOLERANCE 0.4
#define DEFAULT_FAILED_COUNT 3
#define DEFAULT_TARGET_DURATION 10
#define DEFAULT_NUM_FRAGMENTS_CACHE 3
#define DEFAULT_TOTAL_CACHE_DURATION (DEFAULT_NUM_FRAGMENTS_CACHE * DEFAULT_TARGET_DURATION * GST_SECOND)

#define PREDEFINED_VIDEO_FRAME_LOCATION "/usr/etc/sec_audio_fixed_qvga.264"
//#define PREDEFINED_VIDEO_FRAME_LOCATION "/usr/etc/blackframe_QVGA.264"
#define PREDEFINED_IMAGE_FRAME_LOCATION "/usr/etc/sec_audio_fixed_qvga.jpg"

#define HLSDEMUX2_SOUP_FAILED_CNT 10
#define DEFAULT_FORCE_LOWER_BITRATE FALSE
#define FORCE_LOW_BITRATE_AFTER_CNT 4
#define HLSDEMUX2_HTTP_TIMEOUT 30 //30sec

enum {
  HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE = 0,
  HLSDEMUX2_HTTP_ERROR_RECOVERABLE = 1,
};

typedef struct _HLSDemux2_HTTP_error HLSDemux2_HTTP_error;

typedef gboolean (*HLS_HTTP_error_handle_function) (GstHLSDemux2 *demux, gchar *element_name);

struct _HLSDemux2_HTTP_error {
  guint HTTP_error_code;
  const gchar *error_phrase;
  guint error_type;
  HLS_HTTP_error_handle_function handle_error;
};

struct _GstHLSDemux2PvtStream
{
  void *parent;
  GstPad *sinkpad;
  GstElement *sink;
  gulong sink_eprobe;
  gulong sink_bprobe;
  HLSDEMUX2_STREAM_TYPE type;
  GValue      codec_type;
  GstBuffer  *id3_buffer;
  GstBuffer  *image_buffer;
  GstBuffer  *video_buffer;
  gboolean    got_img_buffer;
  GstElement *convert_pipe;
  GCond      *img_load_cond;
  GMutex     *img_load_lock;
  GCond      *convert_cond;
  GMutex     *convert_lock;
};

struct _GstHLSDemux2Stream
{
  void *parent;
  GstPad *pad;
  GQueue *queue;
  GMutex *queue_lock;
  GCond *queue_full;
  GCond *queue_empty;
  gint64 percent;
  HLSDEMUX2_STREAM_TYPE type;
  gboolean is_linked;
  gboolean eos;
  gboolean apply_disc;
  gint64 cached_duration;
  GThread *dummy_data_thread; /* for pushing dummy video data */
  GstClockTime base_ts; /* base ts start */
  GstClockTime fts; /* first valid timestamp in a fragment */
  GstClockTime lts; /* last valid timestamp in a fragment */
  /* expected first timestamp in next fragment : usefule for handling fragments discontinuities*/
  GstClockTime nts;
  GstClockTime prev_nts; /* stores last nts for handling .aac files */
  gboolean valid_fts_rcvd;
  GstClockTime cdisc; /* current disc time */
  GstClockTime frame_duration;
  guint64 total_stream_time;
  GstClockTime total_disc;
  GQueue *downloader_queue;
  gboolean need_newsegment;
  GstEvent *newsegment;
};

/* GStreamer virtual functions */
static void gst_hlsdemux2_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_hlsdemux2_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_hlsdemux2_sink_event (GstPad * pad, GstEvent * event);
static GstStateChangeReturn gst_hlsdemux2_change_state (GstElement * element, GstStateChange transition);
static void gst_hlsdemux2_dispose (GObject * obj);
static GstFlowReturn gst_hlsdemux2_chain (GstPad * pad, GstBuffer * buf);
static gboolean gst_hlsdemux2_handle_src_event (GstPad * pad, GstEvent * event);

/* init & de-init functions */
static void gst_hlsdemux2_private_init (GstHLSDemux2 *demux);
static void gst_hlsdemux2_private_deinit (GstHLSDemux2 *demux);
static void gst_hlsdemux2_playlist_downloader_init (GstHLSDemux2 *demux);
static void gst_hlsdemux2_playlist_downloader_deinit (GstHLSDemux2 *demux);
static void gst_hlsdemux2_key_downloader_init (GstHLSDemux2 *demux);
static void gst_hlsdemux2_key_downloader_deinit (GstHLSDemux2 *demux);
static void gst_hlsdemux2_fragment_downloader_init (GstHLSDemux2 *demux);
static void gst_hlsdemux2_fragment_downloader_deinit (GstHLSDemux2 *demux);

/* helper functions */
static void gst_hlsdemux2_stop (GstHLSDemux2 * demux);
static gboolean gst_hlsdemux2_set_location (GstHLSDemux2 * demux, const gchar * uri);
static gchar *gst_hlsdemux2_src_buf_to_utf8_playlist (GstBuffer * buf);
static void gst_hlsdemux2_new_pad_added (GstElement *element, GstPad *pad, gpointer data);
static void gst_hlsdemux2_get_cookies(GstHLSDemux2 *demux);
static void gst_hlsdemux2_get_user_agent(GstHLSDemux2 *demux);
static gboolean gst_hlsdemux2_update_playlist (GstHLSDemux2 * demux, gboolean update, gboolean *is_error);
static gboolean gst_hlsdemux2_schedule (GstHLSDemux2 * demux);
static void gst_hlsdemux2_calculate_pushed_duration (GstHLSDemux2 *demux, GstHLSDemux2Stream *stream,
    GstBuffer *inbuf);

/* stream specific functions */
static void gst_hlsdemux2_stream_init (GstHLSDemux2 *demux, GstHLSDemux2Stream *stream,
    HLSDEMUX2_STREAM_TYPE stream_type, gchar *name, GstCaps *src_caps);
static void gst_hlsdemux2_stream_deinit (GstHLSDemux2Stream *stream, gpointer data);
static void gst_hlsdemux2_stop_stream (GstHLSDemux2 *demux);
static HLSDemux2SinkBin *gst_hlsdemux2_create_stream (GstHLSDemux2 *demux, gchar *name, GstCaps *caps);

/* task functions */
static void gst_hlsdemux2_push_loop (GstHLSDemux2Stream *stream);
static void gst_hlsdemux2_fragment_download_loop (GstHLSDemux2 * demux);

/* playlist download related functions */
static void gst_hlsdemux2_on_playlist_buffer (GstElement * appsink, void* data);
#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
static GstBusSyncReply gst_hlsdemux2_playlist_download_bus_sync_cb (GstBus * bus, GstMessage *msg, gpointer data);
#else
static gboolean gst_hlsdemux2_playlist_download_bus_cb(GstBus *bus, GstMessage *msg, gpointer data);
#endif

/* key file download related functions */
static gboolean gst_hlsdemux2_key_download_bus_cb(GstBus *bus, GstMessage *msg, gpointer data);
static void gst_hlsdemux2_on_key_buffer (GstElement *appsink, void *data);

/* fragment download related functions */
static gboolean gst_hlsdemux2_fragment_download_bus_cb(GstBus *bus, GstMessage *msg, gpointer data);
static void gst_hlsdemux2_downloader_new_buffer (GstElement *appsink, void *user_data);
static void gst_hlsdemux2_downloader_eos (GstElement * appsink, void* user_data);

/* probe functions */
static gboolean gst_hlsdemux2_sink_event_handler (GstPad * pad, GstEvent * event, gpointer data);
static gboolean gst_hlsdemux2_change_playlist (GstHLSDemux2 * demux, guint max_bitrate, gboolean *is_switched);
static gboolean gst_hlsdemux2_download_monitor_thread (GstHLSDemux2 *demux);
static void gst_hlsdemux2_push_eos (GstHLSDemux2 *demux);
static void gst_hlsdemux2_apply_disc (GstHLSDemux2 * demux);
static gboolean gst_hlsdemux2_queue_buffer_handler (GstPad * pad, GstBuffer *buffer, gpointer data);
static gboolean hlsdemux2_HTTP_not_found(GstHLSDemux2 *demux, gchar *element_name);
static gboolean hlsdemux2_HTTP_time_out (GstHLSDemux2 *demux, gchar *element_name);
static gboolean hlsdemux2_HTTP_repeat_request (GstHLSDemux2 *demux, gchar *element_name);
static void gst_hlsdemux2_handle_private_pad (GstHLSDemux2 *demux, GstPad *srcpad);
static void gst_hlsdemux2_private_sink_on_new_buffer (GstElement *appsink, void *user_data);
static void gst_hlsdemux2_private_sink_on_eos (GstElement * appsink, void* user_data);
static gboolean gst_hlsdemux2_imagebuf_pipe_bus_cb (GstBus *bus, GstMessage *msg, gpointer data);
static gboolean gst_hlsdemux2_set_video_buffer (GstPad * srcpad, GstBuffer * buffer, gpointer user_data);
static gboolean gst_hlsdemux2_done_video_buffer (GstPad * srcpad, GstEvent * event, gpointer user_data);
static gboolean gst_hlsdemux2_check_fast_switch (GstHLSDemux2 *demux);

static const HLSDemux2_HTTP_error http_errors[] = {

  /* transport errors by libsoup */
  { 1, "Cancelled", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 2, "Cannot resolve hostname", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 3, "Cannot resolve proxy hostname", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 4, "Cannot connect to destination", HLSDEMUX2_HTTP_ERROR_RECOVERABLE, hlsdemux2_HTTP_repeat_request },
  { 5, "Cannot connect to proxy", HLSDEMUX2_HTTP_ERROR_RECOVERABLE, hlsdemux2_HTTP_repeat_request },
  { 6, "SSL handshake failed", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 7, "Connection terminated unexpectedly", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 8, "Message Corrupt", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 9, "Too many redirects", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },

  /* Client error */
  { 400, "Bad Request", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 401, "Unauthorized", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 402, "Payment Required", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
  { 403, "Forbidden", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
#else
  { 403, "Forbidden", HLSDEMUX2_HTTP_ERROR_RECOVERABLE, hlsdemux2_HTTP_repeat_request }, // TODO: Currently taking it as recoverable for testing
#endif
  { 404, "Not Found", HLSDEMUX2_HTTP_ERROR_RECOVERABLE, hlsdemux2_HTTP_not_found },
  { 405, "Method Not Allowed", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 406, "Not Acceptable", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 407, "Proxy Authentication Required", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 408, "Request Timeout", HLSDEMUX2_HTTP_ERROR_RECOVERABLE, hlsdemux2_HTTP_time_out },
  { 409, "Conflict", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 410, "Gone", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 411, "Length Required", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 412, "Precondition Failed", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 413, "Request Entity Too Large", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 414, "Request-URI Too Long", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 415, "Unsupported Media Type", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 416, "Requested Range Not Satisfiable", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 417, "Expectation Failed", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 418, "Unprocessable Entity", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 419, "Locked", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 420, "Failed Dependency", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },

  /* Server error */
  { 500, "Internal Server Error", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 501, "Not Implemented", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 502, "Bad Gateway", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 503, "Service Unavailable", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL }, // TODO: need to make as recoverable with timout value
  { 504, "Gateway Timeout", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 505, "HTTP Version Not Supported", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 506, "Insufficient Storage", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
  { 507, "Not Extended", HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE, NULL },
};


static void
gst_hlsdemux2_base_init (gpointer g_class)
{
  GstElementClass *element_class= GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&hlsdemux2_videosrc_template));
  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&hlsdemux2_audiosrc_template));
  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&hlsdemux2_subpicture_template));
  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&hlsdemux2_sink_template));

  gst_element_class_set_details_simple (element_class,
      "HLS Demuxer2",
      "Demuxer/URIList",
      "HTTP Live Streaming (HLS) demuxer",
      "Naveen Cherukuri<naveen.ch@samsung.com>");
}

static void
gst_hlsdemux2_class_init (GstHLSDemux2Class * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_hlsdemux2_set_property;
  gobject_class->get_property = gst_hlsdemux2_get_property;
  gobject_class->dispose = gst_hlsdemux2_dispose;

  g_object_class_install_property (gobject_class, PROP_BITRATE_SWITCH_TOLERANCE,
      g_param_spec_float ("bitrate-switch-tolerance",
          "Bitrate switch tolerance",
          "Tolerance with respect of the fragment duration to switch to "
          "a different bitrate if the client is too slow/fast.",
          0, 1, DEFAULT_BITRATE_SWITCH_TOLERANCE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FORCE_LOWER_BITRATE,
      g_param_spec_boolean ("force-low-bitrate",
          "forcing lower variant", "forcing lower variant after every few fragments [for debugging purpose only]",
          DEFAULT_FORCE_LOWER_BITRATE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BLOCKSIZE,
      g_param_spec_ulong ("blocksize", "Block size",
          "Size in bytes to read per buffer (-1 = default)", 0, G_MAXULONG,
          DEFAULT_BLOCKSIZE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  g_object_class_install_property (gobject_class,PROP_HLS_PROXY,
      g_param_spec_string ("hls-proxy", "HLS Proxy",
          "HTTP proxy server URI for HLS", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_hlsdemux2_change_state);
}

static void
gst_hlsdemux2_init (GstHLSDemux2 * demux, GstHLSDemux2Class * klass)
{
  /* sink pad */
  demux->sinkpad = gst_pad_new_from_static_template (&hlsdemux2_sink_template, "sink");
  gst_pad_set_chain_function (demux->sinkpad, GST_DEBUG_FUNCPTR (gst_hlsdemux2_chain));
  gst_pad_set_event_function (demux->sinkpad, GST_DEBUG_FUNCPTR (gst_hlsdemux2_sink_event));
  gst_element_add_pad (GST_ELEMENT (demux), demux->sinkpad);

  gst_hlsdemux2_private_init (demux);

  /* fragment downloader init */
  gst_hlsdemux2_fragment_downloader_init (demux);

  /* playlist downloader init */
  gst_hlsdemux2_playlist_downloader_init (demux);

  /* key downloader init */
  gst_hlsdemux2_key_downloader_init (demux);

}

static void
gst_hlsdemux2_dispose (GObject * obj)
{
  GstHLSDemux2 *demux = GST_HLSDEMUX2 (obj);

  GST_INFO_OBJECT (demux,"entering");

  gst_hlsdemux2_stop (demux);

  gst_hlsdemux2_private_deinit (demux);

  gst_hlsdemux2_fragment_downloader_deinit (demux);

  gst_hlsdemux2_playlist_downloader_deinit (demux);

  gst_hlsdemux2_key_downloader_deinit (demux);

  GST_INFO_OBJECT (demux,"leaving");

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_hlsdemux2_private_init (GstHLSDemux2 *demux)
{
  demux->is_live = TRUE;
  demux->bitrate_switch_tol = DEFAULT_BITRATE_SWITCH_TOLERANCE;
  demux->percent = 100;
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  demux->proxy = NULL;
#endif
  demux->active_stream_cnt = 0;
  demux->download_task = NULL;
  demux->pl_update_lock = g_mutex_new ();
  demux->pl_update_cond = g_cond_new ();
  demux->cancelled = FALSE;
  demux->total_cache_duration = DEFAULT_TOTAL_CACHE_DURATION;
  demux->target_duration = DEFAULT_TARGET_DURATION;
  demux->keyCookie = NULL;
  demux->playlistCookie = NULL;
  demux->fragCookie = NULL;
  demux->lastCookie = NULL;
  demux->keyDomain = NULL;
  demux->playlistDomain = NULL;
  demux->fragDomain = NULL;
  demux->lastDomain = NULL;
  demux->user_agent = NULL;
  demux->buffering_lock = g_mutex_new ();
  demux->soup_request_fail_cnt = HLSDEMUX2_SOUP_FAILED_CNT;
  demux->force_lower_bitrate = DEFAULT_FORCE_LOWER_BITRATE;
  demux->cfrag_dur = 0;
  demux->error_posted = FALSE;
  demux->playlist_uri = NULL;
  demux->frag_uri = NULL;
  demux->key_uri = NULL;
  demux->blocksize = DEFAULT_BLOCKSIZE;
  demux->flushing = FALSE;
  demux->ns_start = 0;
  demux->cur_audio_fts = GST_CLOCK_TIME_NONE;
  demux->is_buffering = TRUE;
  demux->buffering_posting_thread = NULL;
  demux->post_msg_lock = g_mutex_new ();
  demux->post_msg_start = g_cond_new ();
  demux->post_msg_exit = g_cond_new ();
  demux->stream_config = HLSDEMUX2_SINGLE_VARIANT;
  demux->has_image_buffer = FALSE;
  demux->prev_image_buffer = NULL;
  demux->prev_video_buffer = NULL;
  demux->private_stream = NULL;
  demux->gsinkbin = NULL;
}

static void
gst_hlsdemux2_private_deinit (GstHLSDemux2 *demux)
{
  demux->end_of_playlist = FALSE;

  if (demux->pl_update_lock) {
    g_mutex_free (demux->pl_update_lock);
    demux->pl_update_lock = NULL;
  }

  if (demux->pl_update_cond) {
    g_cond_free (demux->pl_update_cond);
    demux->pl_update_cond = NULL;
  }

  if (demux->buffering_lock) {
    g_mutex_free (demux->buffering_lock);
    demux->buffering_lock = NULL;
  }
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  if (demux->proxy)
  {
    g_free (demux->proxy);
    demux->proxy = NULL;
  }
#endif
  if (demux->user_agent) {
    g_free (demux->user_agent);
    demux->user_agent = NULL;
  }

  if (demux->playlist) {
    gst_buffer_unref (demux->playlist);
    demux->playlist = NULL;
  }

  if (demux->fragCookie) {
    g_strfreev (demux->fragCookie);
    demux->fragCookie = NULL;
  }

  if (demux->keyCookie) {
    g_strfreev (demux->keyCookie);
    demux->keyCookie = NULL;
  }

  if (demux->lastCookie) {
    g_strfreev (demux->lastCookie);
    demux->lastCookie = NULL;
  }

  if (demux->playlistCookie) {
    g_strfreev (demux->playlistCookie);
    demux->playlistCookie = NULL;
  }

  if (demux->fragDomain) {
    g_free (demux->fragDomain);
    demux->fragDomain = NULL;
  }

  if (demux->keyDomain) {
    g_free (demux->keyDomain);
    demux->keyDomain = NULL;
  }

  if (demux->lastDomain) {
    g_free (demux->lastDomain);
    demux->lastDomain = NULL;
  }

  if (demux->playlistDomain) {
    g_free (demux->playlistDomain);
    demux->playlistDomain = NULL;
  }

  if (demux->playlist_uri) {
    g_free (demux->playlist_uri);
    demux->playlist_uri = NULL;
  }

  if (demux->key_uri) {
    g_free (demux->key_uri);
    demux->key_uri = NULL;
  }

  if (demux->frag_uri) {
    g_free (demux->frag_uri);
    demux->frag_uri = NULL;
  }

  if (demux->prev_image_buffer) {
    gst_buffer_unref (demux->prev_image_buffer);
    demux->prev_image_buffer = NULL;
  }

  if (demux->prev_video_buffer) {
    gst_buffer_unref (demux->prev_video_buffer);
    demux->prev_video_buffer = NULL;
  }

  if (demux->private_stream) {
    g_free (demux->private_stream);
    demux->private_stream = NULL;
  }

  if (demux->client) {
    gst_m3u8_client_free (demux->client);
    demux->client = NULL;
  }

  if(demux->gsinkbin){
	if (demux->gsinkbin->sinkbin)
		gst_element_set_state (GST_ELEMENT(demux->gsinkbin->sinkbin), GST_STATE_NULL);

      if (demux->gsinkbin) {
        g_free (demux->gsinkbin);
        demux->gsinkbin = NULL;
      }
  }
}

static void
gst_hlsdemux2_fragment_downloader_init (GstHLSDemux2 *demux)
{
  demux->fdownloader = g_new0 (HLSDemux2FragDownloader, 1);
  demux->fdownloader->pipe = NULL;
  demux->fdownloader->urisrc = NULL;
  demux->fdownloader->queue = NULL;
  demux->fdownloader->typefind = NULL;
  demux->fdownloader->demuxer = NULL;
  demux->fdownloader->sinkbins = NULL;
  demux->fdownloader->lock = g_mutex_new ();
  demux->fdownloader->cond = g_cond_new ();
  demux->fdownloader->is_encrypted = FALSE;
  demux->fdownloader->content_size = 0;
  demux->fdownloader->get_next_frag = FALSE;
  demux->fdownloader->applied_fast_switch = FALSE;
  demux->fdownloader->remaining_data = NULL;
  demux->fdownloader->remaining_size = 0;
  demux->fdownloader->first_buffer = TRUE;
  demux->fdownloader->cur_stream_cnt = 0;
  demux->fdownloader->force_timestamps = FALSE;
  demux->fdownloader->error_rcvd = FALSE;
  demux->fdownloader->find_mediaseq = FALSE;
  demux->fdownloader->seeked_pos = 0;
  demux->fdownloader->cur_running_dur = 0;
  demux->fdownloader->download_rate = -1;
  demux->fdownloader->download_start_ts = 0;
  demux->fdownloader->download_stop_ts = 0;
  demux->fdownloader->src_downloaded_size = 0;
  demux->fdownloader->queue_downloaded_size = 0;
  demux->fdownloader->ndownloaded = 0;
  demux->fdownloader->chunk_downloaded_size = 0;
  demux->fdownloader->chunk_start_ts = 0;
  demux->fdownloader->avg_chunk_drates = g_array_new(FALSE, TRUE, sizeof(guint64));
  demux->fdownloader->avg_frag_drates = g_array_new(FALSE, TRUE, sizeof(guint64));
}

static void
gst_hlsdemux2_fragment_downloader_deinit (GstHLSDemux2 *demux)
{
  if (!demux->fdownloader)
    return;

  /* free fragment downloader structure */
  if (demux->fdownloader->lock) {
    g_mutex_free (demux->fdownloader->lock);
    demux->fdownloader->lock = NULL;
  }

  if (demux->fdownloader->cond) {
    g_cond_free (demux->fdownloader->cond);
    demux->fdownloader->cond = NULL;
  }

  if (demux->fdownloader->remaining_data) {
    g_free (demux->fdownloader->remaining_data);
    demux->fdownloader->remaining_data = NULL;
  }

  if (demux->fdownloader->avg_chunk_drates) {
    g_array_free(demux->fdownloader->avg_chunk_drates, TRUE);
    demux->fdownloader->avg_chunk_drates = NULL;
  }

  if (demux->fdownloader->avg_frag_drates) {
    g_array_free(demux->fdownloader->avg_frag_drates, TRUE);
    demux->fdownloader->avg_frag_drates = NULL;
  }

  g_free (demux->fdownloader);
  demux->fdownloader = NULL;
}

static void
gst_hlsdemux2_playlist_downloader_init (GstHLSDemux2 *demux)
{
  demux->pldownloader = g_new0 (HLSDemux2PLDownloader, 1);
  demux->pldownloader->pipe = NULL;
  demux->pldownloader->bus = NULL;
  demux->pldownloader->urisrc = NULL;
  demux->pldownloader->sink = NULL;
  demux->pldownloader->lock = g_mutex_new ();
  demux->pldownloader->cond = g_cond_new ();
  demux->pldownloader->playlist = NULL;
  demux->pldownloader->recovery_mode = HLSDEMUX2_NO_RECOVERY;
}

static void
gst_hlsdemux2_playlist_downloader_deinit (GstHLSDemux2 *demux)
{
  if (!demux->pldownloader)
    return;

  /* free playlist downloader structure */
  if (demux->pldownloader->lock) {
    g_mutex_free (demux->pldownloader->lock);
    demux->pldownloader->lock = NULL;
  }

  if (demux->pldownloader->cond) {
    g_cond_free (demux->pldownloader->cond);
    demux->pldownloader->cond = NULL;
  }

  if (demux->pldownloader->playlist){
    gst_buffer_unref (demux->pldownloader->playlist);
    demux->pldownloader->playlist = NULL;
  }

  g_free (demux->pldownloader);
  demux->pldownloader = NULL;
}

static void
gst_hlsdemux2_key_downloader_init (GstHLSDemux2 *demux)
{
  demux->kdownloader = g_new0 (HLSDemux2KeyDownloader, 1);
  demux->kdownloader->pipe = NULL;
  demux->kdownloader->bus = NULL;
  demux->kdownloader->urisrc = NULL;
  demux->kdownloader->sink = NULL;
  demux->kdownloader->lock = g_mutex_new ();
  demux->kdownloader->cond = g_cond_new ();
  demux->kdownloader->key = NULL;
  demux->kdownloader->prev_key_uri = NULL;
}

static void
gst_hlsdemux2_key_downloader_deinit (GstHLSDemux2 *demux)
{
  /* free key file downloader structure */
  if (!demux->kdownloader)
    return;

  if (demux->kdownloader->lock) {
    g_mutex_free (demux->kdownloader->lock);
    demux->kdownloader->lock = NULL;
  }

  if (demux->kdownloader->cond) {
    g_cond_free (demux->kdownloader->cond);
    demux->kdownloader->cond = NULL;
  }

  if (demux->kdownloader->key){
    gst_buffer_unref (demux->kdownloader->key);
    demux->kdownloader->key = NULL;
  }

  if (demux->kdownloader->prev_key_uri) {
    g_free (demux->kdownloader->prev_key_uri);
    demux->kdownloader->prev_key_uri = NULL;
  }

  g_free (demux->kdownloader);
  demux->kdownloader = NULL;
}

static void
gst_hlsdemux2_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstHLSDemux2 *demux = GST_HLSDEMUX2 (object);

  switch (prop_id) {
    case PROP_BITRATE_SWITCH_TOLERANCE:
      demux->bitrate_switch_tol = g_value_get_float (value);
      break;
    case PROP_FORCE_LOWER_BITRATE:
      demux->force_lower_bitrate = g_value_get_boolean (value);
      break;
    case PROP_BLOCKSIZE:
      demux->blocksize = g_value_get_ulong (value);
      break;
#ifdef GST_EXT_HLS_PROXY_ROUTINE
    case PROP_HLS_PROXY:
	{
	  if (demux->proxy)
	  {
	    g_free (demux->proxy);
	    demux->proxy = NULL;
	  }
	  demux->proxy = g_value_dup_string (value);

	  if (demux->proxy == NULL) {
	    GST_ERROR_OBJECT (demux,"proxy property cannot be NULL");
	    break;
	  }
	  GST_WARNING_OBJECT (demux,"web proxy set as = %s",demux->proxy);
	  break;
	}
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_hlsdemux2_get_property (GObject * object, guint prop_id,
   GValue * value, GParamSpec * pspec)
{
  GstHLSDemux2 *demux = GST_HLSDEMUX2 (object);

  switch (prop_id) {
    case PROP_BITRATE_SWITCH_TOLERANCE:
      g_value_set_float (value, demux->bitrate_switch_tol);
      break;
    case PROP_FORCE_LOWER_BITRATE:
      g_value_set_boolean (value, demux->force_lower_bitrate);
      break;
    case PROP_BLOCKSIZE:
      g_value_set_ulong (value, demux->blocksize);
      break;
#ifdef GST_EXT_HLS_PROXY_ROUTINE
    case PROP_HLS_PROXY:
      if (demux->proxy == NULL)
        g_value_set_static_string (value, "");
      else {
        g_value_set_string (value, demux->proxy);
      }
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_hlsdemux2_sink_event (GstPad * pad, GstEvent * event)
{
  GstHLSDemux2 *demux = GST_HLSDEMUX2 (gst_pad_get_parent (pad));
  GstQuery *query = NULL;
  gboolean ret = FALSE;
  gchar *uri = NULL;

  GST_LOG_OBJECT (demux, "Received event '%s' on sink pad...", GST_EVENT_TYPE_NAME (event));

  switch (event->type) {
    case GST_EVENT_EOS: {
      gchar *playlist = NULL;
      GError *err = NULL;

      if (demux->playlist == NULL) {
        GST_ERROR_OBJECT (demux, "Received EOS without a playlist.");
        goto error;
      }

      GST_INFO_OBJECT (demux, "Got EOS on the sink pad: playlist file fetched");

      query = gst_query_new_uri ();

      /* query the location from upstream element for absolute path */
      ret = gst_pad_peer_query (demux->sinkpad, query);
      if (ret) {
        gst_query_parse_uri (query, &uri);
        gst_hlsdemux2_set_location (demux, uri);
        g_free (uri);
      } else {
        GST_ERROR_OBJECT (demux, "failed to query URI from upstream");
        goto error;
      }
      gst_query_unref (query);
      query = NULL;

      /* get cookies from upstream httpsrc */
      gst_hlsdemux2_get_cookies (demux);

      /* get user-agent from upstream httpsrc */
      gst_hlsdemux2_get_user_agent (demux);

      playlist = gst_hlsdemux2_src_buf_to_utf8_playlist (demux->playlist);
      demux->playlist = NULL;

      if (playlist == NULL) {
        GST_ERROR_OBJECT (demux, "Error validating first playlist.");
        GST_ELEMENT_ERROR (demux, STREAM, DECODE, ("Invalid playlist."), (NULL));
        goto error;
      }

      if (!gst_m3u8_client_update (demux->client, playlist)) {
        /* In most cases, this will happen if we set a wrong url in the
         * source element and we have received the 404 HTML response instead of
         * the playlist */
        GST_ERROR_OBJECT (demux, "failed to parse playlist...");
        GST_ELEMENT_ERROR (demux, STREAM, DECODE, ("Invalid playlist."), (NULL));
        goto error;
      }

      /* Loop to download the fragments & updates playlist */
      g_static_rec_mutex_init (&demux->download_lock);
      demux->download_task = gst_task_create ((GstTaskFunction) gst_hlsdemux2_fragment_download_loop, demux);
      if (NULL == demux->download_task) {
        GST_ERROR_OBJECT (demux, "failed to create download task...");
        GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("failed to fragment download task"), (NULL));
        goto error;
      }
      gst_task_set_lock (demux->download_task, &demux->download_lock);
      gst_task_start (demux->download_task);

      /* create thread to send dummy data */
      demux->buffering_posting_thread = g_thread_create (
        (GThreadFunc) gst_hlsdemux2_download_monitor_thread, demux, TRUE, &err);

      /* Swallow EOS, we'll push our own */
      gst_event_unref (event);
      gst_object_unref (demux);
      return TRUE;
    }
    case GST_EVENT_NEWSEGMENT:
      /* Swallow newsegments, we'll push our own */
      gst_event_unref (event);
      gst_object_unref (demux);
      return TRUE;
    default:
      gst_object_unref (demux);
      break;
  }

  return gst_pad_event_default (pad, event);

error:
  gst_hlsdemux2_stop (demux);
  gst_event_unref (event);
  gst_object_unref (demux);

  if (query)
    gst_query_unref (query);

  GST_LOG_OBJECT (demux,"Returning from sink event...");
  return FALSE;
}

static GstStateChangeReturn
  gst_hlsdemux2_change_state (GstElement * element, GstStateChange transition)
  {
    GstStateChangeReturn ret;
    GstHLSDemux2 *demux = GST_HLSDEMUX2 (element);
    gboolean bret = FALSE;

    switch (transition) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        GST_INFO_OBJECT (demux,"PAUSED->PLAYING");
        break;
      case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_INFO_OBJECT (demux,"PAUSED->READY before parent");
        gst_hlsdemux2_stop (demux);
        break;
      default:
        break;
    }

    ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

    switch (transition) {
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        GST_INFO_OBJECT (demux,"PLAYING->PAUSED");
        break;
      case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_INFO_OBJECT (demux,"PAUSED->READY after parent");
        if (demux->download_task) {
          bret = gst_task_join (demux->download_task);
          GST_DEBUG_OBJECT (demux,"Joining download task : %d", bret);
        }
        break;
      case GST_STATE_CHANGE_READY_TO_NULL:
        GST_INFO_OBJECT (demux,"READY->NULL");
        break;
      default:
        break;
    }
    return ret;
  }



static GstFlowReturn
gst_hlsdemux2_chain (GstPad * pad, GstBuffer * buf)
{
  GstHLSDemux2 *demux = GST_HLSDEMUX2 (gst_pad_get_parent (pad));

  if (demux->playlist == NULL)
    demux->playlist = buf;
  else
    demux->playlist = gst_buffer_join (demux->playlist, buf);
  gst_object_unref (demux);

  return GST_FLOW_OK;
}

static gboolean
gst_hlsdemux2_handle_src_event (GstPad * pad, GstEvent * event)
{
  GstHLSDemux2 *hlsdemux2 = NULL;
  GstHLSDemux2Stream *stream = NULL;

  stream = (GstHLSDemux2Stream *) (gst_pad_get_element_private (pad));
  hlsdemux2 = stream->parent;

  switch (event->type) {
    case GST_EVENT_SEEK: {
      gdouble rate;
      GstFormat format;
      GstSeekFlags flags;
      GstSeekType start_type, stop_type;
      gint64 start, stop;
      gint idx = 0;

      if (gst_m3u8_client_is_live (hlsdemux2->client)) {
        GST_WARNING_OBJECT (stream->pad, "SEEK is NOT Supported in LIVE");
        return FALSE;
      }

      GST_INFO_OBJECT (stream->pad, "received SEEK event..");

      gst_event_parse_seek (event, &rate, &format, &flags, &start_type, &start,
                &stop_type, &stop);

      if (format != GST_FORMAT_TIME) {
        GST_WARNING_OBJECT (stream->pad, "received seek with unsupported format");
        return FALSE;
      }

      // TODO: Validate requested SEEK time is within the duration or not

      hlsdemux2->end_of_playlist = FALSE; // resent end of playlist

      GST_DEBUG_OBJECT (stream->pad, "SEEK event with rate: %f start: %" GST_TIME_FORMAT
          " stop: %" GST_TIME_FORMAT, rate, GST_TIME_ARGS (start),
          GST_TIME_ARGS (stop));

      if (!(flags & GST_SEEK_FLAG_FLUSH)) {
        GST_WARNING_OBJECT (stream->pad, "non-flush seek is not supported yet");
        return FALSE;
      }

      hlsdemux2->flushing = TRUE;

      /* signal queue full condition to come out */
      for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
        GstHLSDemux2Stream *cur_stream = hlsdemux2->streams[idx];

        if (cur_stream) {
          cur_stream->eos = FALSE; // resent stream EOS
          if (cur_stream->queue) {
            GST_INFO_OBJECT (cur_stream->pad, "signalling stream queue");
            g_cond_signal (cur_stream->queue_full); /* to signal downloader eos blocking */
            g_cond_signal (cur_stream->queue_empty); /* incase push_loop blocked on condition */
          }
        }
      }

      /* send FLUSH_START event to all source pads */
      if (flags & GST_SEEK_FLAG_FLUSH) {

        GST_INFO_OBJECT (hlsdemux2, "sending flush start on all the stream pads");

        for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
          GstHLSDemux2Stream *cur_stream = hlsdemux2->streams[idx];

          if (cur_stream && GST_PAD_TASK(cur_stream->pad)) {
            gboolean bret = FALSE;
            bret = gst_pad_push_event (cur_stream->pad, gst_event_new_flush_start ());
            GST_DEBUG_OBJECT (cur_stream->pad, "flush_start returned = %d", bret);
          }
        }
      }

      /* wait till all stream pad's task paused */
      for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
        GstHLSDemux2Stream *cur_stream = hlsdemux2->streams[idx];

        if (cur_stream && GST_PAD_TASK(cur_stream->pad)) {
          GST_INFO_OBJECT (cur_stream->pad, "trying acquire stream lock");
          GST_PAD_STREAM_LOCK (cur_stream->pad);
          GST_INFO_OBJECT (cur_stream->pad, "acquired stream lock");
          GST_PAD_STREAM_UNLOCK (cur_stream->pad);
        }
      }

      /* pause fragment download loop */
      g_mutex_lock (hlsdemux2->kdownloader->lock);
      GST_INFO_OBJECT (stream->pad, "Signalling key downloader condition");
      g_cond_signal (hlsdemux2->kdownloader->cond);
      g_mutex_unlock (hlsdemux2->kdownloader->lock);

      g_mutex_lock (hlsdemux2->fdownloader->lock);
      GST_INFO_OBJECT (stream->pad, "Signalling fragment downloader condition");
      g_cond_signal (hlsdemux2->fdownloader->cond);
      g_mutex_unlock (hlsdemux2->fdownloader->lock);

      g_mutex_lock (hlsdemux2->pldownloader->lock);
      GST_INFO_OBJECT (stream->pad, "Signalling playlist downloader condition");
      g_cond_signal (hlsdemux2->pldownloader->cond);
      g_mutex_unlock (hlsdemux2->pldownloader->lock);

      GST_INFO_OBJECT (stream->pad, "waiting for download task to pause...");
      g_static_rec_mutex_lock (&hlsdemux2->download_lock);
      g_static_rec_mutex_unlock (&hlsdemux2->download_lock);

      GST_INFO_OBJECT (hlsdemux2, "Download task paused");

      /* clear internal stream queues */
      for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
        GstHLSDemux2Stream *cur_stream = hlsdemux2->streams[idx];

        if (cur_stream && cur_stream->queue) {
          while (!g_queue_is_empty (cur_stream->queue)) {
            gpointer data = NULL;
            data = g_queue_pop_head (cur_stream->queue);
            if (GST_IS_BUFFER(data))
              gst_buffer_unref(GST_BUFFER(data));
            else if (GST_IS_EVENT(data))
               gst_event_unref (GST_EVENT(data));
            else
            gst_object_unref (data);
          }
          g_queue_clear (cur_stream->queue);
          cur_stream->cached_duration = 0;
          stream->percent = 0;

          while (!g_queue_is_empty (cur_stream->downloader_queue)) {
            gpointer data = NULL;
            data = g_queue_pop_head (cur_stream->downloader_queue);
            if (GST_IS_BUFFER(data))
              gst_buffer_unref(GST_BUFFER(data));
            else if (GST_IS_EVENT(data))
              gst_event_unref (GST_EVENT(data));
            else
            gst_object_unref (data);
          }
          g_queue_clear (cur_stream->downloader_queue);
        }
      }

#if 0 /* useful when we want to switch to lowest variant on seek for faster buffering */
      if (gst_m3u8_client_has_variant_playlist (hlsdemux2->client)) {
        /* intentionally sending zero download rate, to move to least possible variant */
        gst_hlsdemux2_change_playlist (hlsdemux2, 0, NULL);
      }
#endif

      /* send FLUSH_STOP event to all source pads */
      if (flags & GST_SEEK_FLAG_FLUSH) {

        GST_INFO_OBJECT (hlsdemux2, "sending flush stop");

        for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
          GstHLSDemux2Stream *cur_stream = hlsdemux2->streams[idx];

          if (cur_stream && GST_PAD_TASK(cur_stream->pad)) {
            gboolean bret = FALSE;
            bret = gst_pad_push_event (cur_stream->pad, gst_event_new_flush_stop ());
            GST_DEBUG_OBJECT (cur_stream->pad, "flush_stop returned = %d", bret);
          }
        }
      }

      hlsdemux2->flushing = FALSE;
      hlsdemux2->fdownloader->find_mediaseq = TRUE;
      hlsdemux2->fdownloader->seeked_pos = start;

      /* start all streams loop tasks & fragment download task */
      for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
        GstHLSDemux2Stream *cur_stream = hlsdemux2->streams[idx];

        if (cur_stream && GST_PAD_TASK(cur_stream->pad)) {
          GST_INFO_OBJECT (cur_stream->pad, "Starting push task");
          cur_stream->need_newsegment = TRUE;
          cur_stream->nts = GST_CLOCK_TIME_NONE; /*useful for audio-only seeking */
          cur_stream->cdisc = 0;
          cur_stream->fts = GST_CLOCK_TIME_NONE;
          cur_stream->valid_fts_rcvd = FALSE;
          cur_stream->total_stream_time = 0;

          if (!gst_pad_start_task (cur_stream->pad, (GstTaskFunction) gst_hlsdemux2_push_loop, cur_stream)) {
            GST_ERROR_OBJECT (hlsdemux2, "failed to start stream task...");
            GST_ELEMENT_ERROR (hlsdemux2, RESOURCE, FAILED, ("failed to create stream push loop"), (NULL));
            goto error;
          }
        }
      }

      gst_hlsdemux2_apply_disc (hlsdemux2);

      GST_INFO_OBJECT (hlsdemux2, "Starting Download task..");

      gst_task_start (hlsdemux2->download_task);

      GST_INFO_OBJECT (hlsdemux2, "Successfully configured SEEK...");
      gst_event_unref (event);
      return TRUE;
    }
    default:
      break;
  }

  return gst_pad_event_default (pad, event);

error:
  GST_ERROR_OBJECT (hlsdemux2, "seeking failed...");
  gst_event_unref (event);
  return FALSE;
}

static gboolean
gst_hlsdemux2_src_query (GstPad *pad, GstQuery * query)
{
  GstHLSDemux2 *hlsdemux2 = NULL;
  gboolean ret = FALSE;
  GstHLSDemux2Stream *stream = NULL;

  if (query == NULL)
    return FALSE;

  stream = (GstHLSDemux2Stream *) (gst_pad_get_element_private (pad));
  hlsdemux2 = stream->parent;

  switch (query->type) {
    case GST_QUERY_DURATION:{
      GstClockTime duration = -1;
      GstFormat fmt;

      gst_query_parse_duration (query, &fmt, NULL);
      if (fmt == GST_FORMAT_TIME) {
        duration = gst_m3u8_client_get_duration (hlsdemux2->client);
        if (GST_CLOCK_TIME_IS_VALID (duration) && duration > 0) {
          gst_query_set_duration (query, GST_FORMAT_TIME, duration);
          ret = TRUE;
        }
      }
      GST_LOG_OBJECT (hlsdemux2, "GST_QUERY_DURATION returns %s with duration %"
          GST_TIME_FORMAT, ret ? "TRUE" : "FALSE", GST_TIME_ARGS (duration));
      break;
    }
    case GST_QUERY_URI:
      if (hlsdemux2->client) {
        /* FIXME: Do we answer with the variant playlist, with the current
         * playlist or the the uri of the least downlowaded fragment? */
        gst_query_set_uri (query, gst_m3u8_client_get_uri (hlsdemux2->client));
        ret = TRUE;
      }
      break;
    case GST_QUERY_SEEKING:{
      GstFormat fmt;
      gint64 stop = -1;

      gst_query_parse_seeking (query, &fmt, NULL, NULL, NULL);
      GST_INFO_OBJECT (hlsdemux2, "Received GST_QUERY_SEEKING with format %d",
          fmt);
      if (fmt == GST_FORMAT_TIME) {
        GstClockTime duration;

        duration = gst_m3u8_client_get_duration (hlsdemux2->client);
        if (GST_CLOCK_TIME_IS_VALID (duration) && duration > 0)
          stop = duration;

        gst_query_set_seeking (query, fmt,
            !gst_m3u8_client_is_live (hlsdemux2->client), 0, stop);
        ret = TRUE;
        GST_INFO_OBJECT (hlsdemux2, "GST_QUERY_SEEKING returning with stop : %"
            GST_TIME_FORMAT, GST_TIME_ARGS (stop));
      }
      break;
    }
    default:
      /* Don't fordward queries upstream because of the special nature of this
       * "demuxer", which relies on the upstream element only to be fed with the
       * first playlist */
      break;
  }

  return ret;
}

static gchar *
gst_hlsdemux2_uri_get_domain ( GstHLSDemux2 *demux, gchar * uri)
{
  gchar *pointer = uri;
  gchar *domain = NULL;
  guint flag = 0;
  guint counter=0;

  domain = g_malloc0(strlen (uri));
  if (!domain) {
    GST_ERROR_OBJECT (demux, "failed to allocate memory...\n");
    GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
    return NULL;
  }

  while(*pointer != '\0') {
    switch(*pointer) {
      case '.':
        flag = 1;
        break;
      case '/':
        if(flag==1)
          flag = 2;
        break;
    }
    if(flag==1) {
      domain[counter]=*pointer;
      counter++;
    } else if(flag == 2) {
      domain[counter]='\0';
      break;
    }
    pointer++;
  }

  GST_DEBUG_OBJECT (demux, "uri = %s and domain = %s", uri, domain);
  return domain;
}

static void
gst_hlsdemux2_get_cookies(GstHLSDemux2 *demux)
{
  GstQuery *cquery;
  GstStructure *structure;
  gboolean bret = FALSE;

  structure = gst_structure_new ("HTTPCookies",
                                 "cookies", G_TYPE_STRING, NULL, NULL);

  cquery = gst_query_new_application (GST_QUERY_CUSTOM, structure);

  bret = gst_pad_peer_query (demux->sinkpad, cquery);
  if (bret) {
    const GstStructure *s;
    const GValue *value;

    s = gst_query_get_structure (cquery);
    value = gst_structure_get_value (s, "cookies");
    demux->playlistCookie = g_strdupv (g_value_get_boxed (value));

    GST_INFO_OBJECT (demux, "Received playlist cookies from upstream element : %s", demux->playlistCookie ? *(demux->playlistCookie) : NULL);
    if(demux->playlistCookie){
        demux->lastCookie = g_strdupv(demux->playlistCookie);
    }
  } else {
    GST_WARNING_OBJECT (demux, "Failed to get cookies");
  }

  gst_query_unref (cquery);
}

static void
gst_hlsdemux2_get_user_agent(GstHLSDemux2 *demux)
{
  GstQuery *cquery;
  GstStructure *structure;
  gboolean bret = FALSE;

  structure = gst_structure_new ("HTTPUserAgent",
                                 "user-agent", G_TYPE_STRING, NULL, NULL);

  cquery = gst_query_new_application (GST_QUERY_CUSTOM, structure);

  bret = gst_pad_peer_query (demux->sinkpad, cquery);
  if (bret) {
    const GstStructure *s;
    const GValue *value;

    GST_INFO_OBJECT (demux, "Received user-agent from upstream element...");

    s = gst_query_get_structure (cquery);
    value = gst_structure_get_value (s, "user-agent");
    demux->user_agent = g_strdup (g_value_get_string (value));

    GST_INFO_OBJECT (demux,"User agent received from query : %s", demux->user_agent);
  } else {
    GST_WARNING_OBJECT (demux, "Failed to get user-agent");
  }

  gst_query_unref (cquery);
}

static gboolean
gst_hlsdemux2_create_playlist_download (GstHLSDemux2 *demux, const gchar *playlist_uri)
{
  if (!gst_uri_is_valid (playlist_uri)) {
    GST_ERROR_OBJECT (demux, "invalid uri : %s...", playlist_uri == NULL ? "NULL" : playlist_uri);
    return FALSE;
  }

  if (demux->cancelled || demux->flushing) {
    GST_WARNING_OBJECT (demux,"returning from download playlist due to cancel or flushing..");
    return FALSE;
  }

  /* create playlist downloader pipeline */
  demux->pldownloader->pipe = gst_pipeline_new ("playlist-downloader");
  if (!demux->pldownloader->pipe) {
    GST_ERROR_OBJECT (demux, "failed to create pipeline");
    return FALSE;
  }

  demux->pldownloader->bus = gst_pipeline_get_bus (GST_PIPELINE (demux->pldownloader->pipe));
#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
  gst_bus_set_sync_handler(demux->pldownloader->bus, gst_hlsdemux2_playlist_download_bus_sync_cb, demux);
#else
  gst_bus_add_watch (demux->pldownloader->bus, (GstBusFunc)gst_hlsdemux2_playlist_download_bus_cb, demux);
#endif
  gst_object_unref (demux->pldownloader->bus);

  GST_INFO_OBJECT (demux, "Creating source element for the URI : %s", playlist_uri);

  /* creating source element */
  demux->pldownloader->urisrc = gst_element_make_from_uri (GST_URI_SRC, playlist_uri, "playlisturisrc");
  if (!demux->pldownloader->urisrc) {
    GST_ERROR_OBJECT (demux, "failed to create urisrc");
    return FALSE;
  }
  g_object_set (G_OBJECT (demux->pldownloader->urisrc), "timeout", HLSDEMUX2_HTTP_TIMEOUT, NULL);
  g_object_set (G_OBJECT (demux->pldownloader->urisrc), "user-agent", demux->user_agent, NULL);
  g_object_set (G_OBJECT (demux->pldownloader->urisrc), "ahs-streaming", TRUE, NULL);
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  if (demux->proxy) {
  	GST_DEBUG_OBJECT (demux, "pldownloader Proxy getting set : %s", demux->proxy);
       g_object_set (G_OBJECT (demux->pldownloader->urisrc), "proxy", demux->proxy, NULL);
  }
#endif
  if(demux->playlistDomain){
	g_free (demux->playlistDomain);
	demux->playlistDomain = NULL;
  }
  demux->playlistDomain = gst_hlsdemux2_uri_get_domain(demux, playlist_uri);
  if(!g_strcmp0(demux->playlistDomain, demux->lastDomain) && demux->lastCookie) {
    g_strfreev (demux->playlistCookie);
    demux->playlistCookie = g_strdupv (demux->lastCookie);
  }

  if (demux->playlistCookie) {
    GST_DEBUG_OBJECT (demux, "Setting cookies before PLAYLIST download, before PLAYING: %s", *(demux->playlistCookie));
    /* setting cookies */
    g_object_set (demux->pldownloader->urisrc, "cookies", demux->playlistCookie, NULL);
  }

  /* create sink element */
  demux->pldownloader->sink =  gst_element_factory_make ("appsink", "playlistsink");
  if (!demux->pldownloader->sink) {
    GST_ERROR_OBJECT (demux, "failed to create playlist sink element");
    return FALSE;
  }
  g_object_set (G_OBJECT (demux->pldownloader->sink), "sync", FALSE, "emit-signals", TRUE, NULL);
  g_signal_connect (demux->pldownloader->sink, "new-buffer",  G_CALLBACK (gst_hlsdemux2_on_playlist_buffer), demux);

  gst_bin_add_many (GST_BIN (demux->pldownloader->pipe), demux->pldownloader->urisrc, demux->pldownloader->sink, NULL);

  if (!gst_element_link_many (demux->pldownloader->urisrc, demux->pldownloader->sink, NULL)) {
    GST_ERROR_OBJECT (demux, "failed to link src & demux elements...");
    return FALSE;
  }

  return TRUE;
}

static void
gst_hlsdemux2_destroy_playlist_download (GstHLSDemux2 * demux)
{
  if (demux->pldownloader->pipe) {
    GST_DEBUG_OBJECT (demux, "Shutting down playlist download pipeline");
    gst_element_set_state (demux->pldownloader->pipe, GST_STATE_NULL);
    gst_element_get_state (demux->pldownloader->pipe, NULL, NULL, GST_CLOCK_TIME_NONE);
    gst_object_unref (demux->pldownloader->pipe);
    demux->pldownloader->pipe = NULL;
  }
}

static gboolean
gst_hlsdemux2_download_playlist (GstHLSDemux2 *demux, const gchar * uri)
{
  GstStateChangeReturn ret;

  if (demux->playlist_uri) {
    g_free (demux->playlist_uri);
  }
  demux->playlist_uri = g_strdup (uri);

  demux->pldownloader->download_start_ts = gst_util_get_timestamp();

  if (!gst_hlsdemux2_create_playlist_download (demux, uri)) {
    GST_ERROR_OBJECT (demux, "failed to create playlist download pipeline");
    return FALSE;
  }

  ret = gst_element_set_state (demux->pldownloader->pipe, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    GST_ERROR_OBJECT (demux, "set_state failed...");
    return FALSE;
  }

  /* schedule the next update of playlist */
  if (demux->is_live)
    gst_hlsdemux2_schedule (demux);

  /* wait until:
   *   - the download succeed (EOS)
   *   - the download failed (Error message on the fetcher bus)
   *   - the download was canceled
   */
  GST_LOG_OBJECT (demux, "Waiting to fetch the URI");
  g_cond_wait (demux->pldownloader->cond, demux->pldownloader->lock);
  GST_LOG_OBJECT (demux, "Recived signal to shutdown...");

  if (demux->playlistCookie)
    g_strfreev (demux->playlistCookie);

  g_object_get (demux->pldownloader->urisrc, "cookies", &demux->playlistCookie, NULL);

  GST_DEBUG_OBJECT (demux, "Got cookies after PLAYLIST download : %s", demux->playlistCookie ? *(demux->playlistCookie) : NULL);

  if(demux->playlistCookie){
    g_strfreev(demux->lastCookie);
    g_free(demux->lastDomain);
    demux->lastCookie = g_strdupv(demux->playlistCookie);
    demux->lastDomain = g_strdup(demux->playlistDomain);
  }

  gst_hlsdemux2_destroy_playlist_download (demux);

  if (demux->cancelled || demux->flushing)
    return FALSE;

  return TRUE;
}


static gboolean
gst_hlsdemux2_update_playlist (GstHLSDemux2 * demux, gboolean update, gboolean *is_error)
{
  gchar *playlist = NULL;
  gboolean updated = FALSE;
  const gchar *playlist_uri = gst_m3u8_client_get_current_uri (demux->client);

  *is_error = FALSE;

  if (!gst_m3u8_client_is_playlist_download_needed (demux->client)) {
    GST_INFO_OBJECT (demux, "Playlist download is not needed...");
    return TRUE;
  }

  g_mutex_lock (demux->pldownloader->lock);

try_again:

  if (!gst_hlsdemux2_download_playlist (demux, playlist_uri)) {
    GST_ERROR_OBJECT (demux, "failed to create playlist download");
    goto error;
  }

  if ((demux->pldownloader->playlist == NULL) || (GST_BUFFER_DATA(demux->pldownloader->playlist) == NULL)) {
    // TODO: don't exact reason, why eos is coming from soup without data...
    GST_ERROR_OBJECT (demux, "Received playlist eos, without playlist data -> TRY AGAIN");
    goto try_again;
  }

  /* reset the count */
  demux->soup_request_fail_cnt = HLSDEMUX2_SOUP_FAILED_CNT;

  playlist = gst_hlsdemux2_src_buf_to_utf8_playlist (demux->pldownloader->playlist);
  if (!playlist) {
    GST_ERROR_OBJECT(demux, "failed to create playlist");
    goto error;
  }
  demux->pldownloader->playlist = NULL;

  updated = gst_m3u8_client_update (demux->client, playlist);
  g_mutex_unlock (demux->pldownloader->lock);

  return updated;

error:
  if (demux->flushing)
    *is_error = TRUE;
  g_mutex_unlock (demux->pldownloader->lock);
  return FALSE;
}

static gboolean
gst_hlsdemux2_create_key_download (GstHLSDemux2 *demux, const gchar *key_uri)
{
  if (!gst_uri_is_valid (key_uri)) {
    GST_ERROR_OBJECT (demux, "invalid uri : %s...", key_uri == NULL ? "NULL" : key_uri);
    return FALSE;
  }

  if (demux->cancelled || demux->flushing) {
    GST_WARNING_OBJECT (demux,"returning from download key due to cancel or flushing..");
    return FALSE;
  }

  demux->kdownloader->pipe = gst_pipeline_new ("keydownloader");
  if (!demux->kdownloader->pipe) {
    GST_ERROR_OBJECT (demux, "failed to create pipeline");
    return FALSE;
  }

  demux->kdownloader->bus = gst_pipeline_get_bus (GST_PIPELINE (demux->kdownloader->pipe));
  gst_bus_add_watch (demux->kdownloader->bus, (GstBusFunc)gst_hlsdemux2_key_download_bus_cb, demux);
  gst_object_unref (demux->kdownloader->bus);

  GST_INFO_OBJECT (demux, "Creating source element for the URI : %s", key_uri);

  demux->kdownloader->urisrc = gst_element_make_from_uri (GST_URI_SRC, key_uri, "keyurisrc");
  if (!demux->kdownloader->urisrc) {
    GST_ERROR_OBJECT (demux, "failed to create urisrc");
    return FALSE;
  }

  g_object_set (G_OBJECT (demux->kdownloader->urisrc), "timeout", HLSDEMUX2_HTTP_TIMEOUT, NULL);
  g_object_set (G_OBJECT (demux->kdownloader->urisrc), "user-agent", demux->user_agent, NULL);
  g_object_set (G_OBJECT (demux->kdownloader->urisrc), "ahs-streaming", TRUE, NULL);
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  if (demux->proxy) {
  	GST_DEBUG_OBJECT (demux, "kdownloader Proxy getting set : %s", demux->proxy);
	g_object_set (G_OBJECT (demux->kdownloader->urisrc), "proxy", demux->proxy, NULL);
  }
#endif
  if(demux->keyDomain){
	g_free (demux->keyDomain);
	demux->keyDomain = NULL;
  }
  demux->keyDomain = gst_hlsdemux2_uri_get_domain(demux, key_uri);
  if(!g_strcmp0(demux->keyDomain, demux->lastDomain) && demux->lastCookie) {
    g_strfreev(demux->keyCookie);
    demux->keyCookie = g_strdupv(demux->lastCookie);
  }

  if (demux->keyCookie) {
    GST_DEBUG_OBJECT (demux, "Setting cookies before KEY download goto PLAYING : %s", *(demux->keyCookie));
    g_object_set (demux->kdownloader->urisrc, "cookies", demux->keyCookie, NULL);
  }

  demux->kdownloader->sink =  gst_element_factory_make ("appsink", "keyfilesink");
  if (!demux->kdownloader->sink) {
    GST_ERROR_OBJECT (demux, "failed to create playlist sink element");
    return FALSE;
  }
  g_object_set (G_OBJECT (demux->kdownloader->sink), "sync", FALSE, "emit-signals", TRUE, NULL);
  g_signal_connect (demux->kdownloader->sink, "new-buffer",  G_CALLBACK (gst_hlsdemux2_on_key_buffer), demux);

  gst_bin_add_many (GST_BIN (demux->kdownloader->pipe), demux->kdownloader->urisrc, demux->kdownloader->sink, NULL);

  if (!gst_element_link_many (demux->kdownloader->urisrc, demux->kdownloader->sink, NULL)) {
    GST_ERROR_OBJECT (demux, "failed to link src & demux elements...");
    return FALSE;
  }

  return TRUE;
}

static void
gst_hlsdemux2_destroy_key_download (GstHLSDemux2 * demux)
{
  if (demux->kdownloader->pipe) {
    GST_DEBUG_OBJECT (demux, "Shutting down key download pipeline");
    gst_element_set_state (demux->kdownloader->pipe, GST_STATE_NULL);
    gst_element_get_state (demux->kdownloader->pipe, NULL, NULL, GST_CLOCK_TIME_NONE);
    gst_object_unref (demux->kdownloader->pipe);
    demux->kdownloader->pipe = NULL;
  }
}

static gboolean
gst_hlsdemux2_download_key (GstHLSDemux2 *demux, const gchar * uri)
{
  GstStateChangeReturn ret;

  if (demux->key_uri) {
    g_free (demux->key_uri);
  }
  demux->key_uri = g_strdup (uri);

  g_mutex_lock (demux->kdownloader->lock);

  if (!gst_hlsdemux2_create_key_download (demux, uri)) {
    GST_ERROR_OBJECT (demux, "failed to create key download pipeline");
    return FALSE;
  }

  ret = gst_element_set_state (demux->kdownloader->pipe, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    GST_ERROR_OBJECT (demux, "set_state failed...");
    return FALSE;
  }

  /* wait until:
   *   - the download succeed (EOS)
   *   - the download failed (Error message on the fetcher bus)
   *   - the download was canceled
   */
  GST_DEBUG_OBJECT (demux, "Waiting to fetch the key URI");
  g_cond_wait (demux->kdownloader->cond, demux->kdownloader->lock);
  GST_DEBUG_OBJECT (demux, "Recived signal to shutdown key downloader...");
  g_mutex_unlock (demux->kdownloader->lock);

  if (demux->keyCookie) {
    g_object_get (demux->kdownloader->urisrc, "cookies", &demux->keyCookie, NULL);
  }

  GST_DEBUG_OBJECT (demux, "Got cookies after KEY download : %s", demux->keyCookie ? *(demux->keyCookie) : NULL);

  if(demux->keyCookie){
      g_strfreev(demux->lastCookie);
      g_free(demux->lastDomain);
      demux->lastCookie = g_strdupv(demux->keyCookie);
      demux->lastDomain = g_strdup(demux->keyDomain);
  }

  gst_hlsdemux2_destroy_key_download (demux);

  if (demux->cancelled || demux->flushing)
    return FALSE;

  return TRUE;
}

static void
gst_hlsdemux2_have_type_cb (GstElement * typefind, guint probability,
    GstCaps * caps, GstHLSDemux2 * demux)
{
  GstStructure *structure = NULL;

  structure = gst_caps_get_structure (caps, 0);

  GST_DEBUG_OBJECT (demux, "typefind found caps %" GST_PTR_FORMAT, caps);

  if (gst_structure_has_name (structure, "video/mpegts")) {
    /* found ts fragment */
    demux->fdownloader->demuxer = gst_element_factory_make ("mpegtsdemux", "tsdemux");
    if (!demux->fdownloader->demuxer) {
      GST_ERROR_OBJECT (demux, "failed to create mpegtsdemux element");
      GST_ELEMENT_ERROR (demux, CORE, MISSING_PLUGIN, ("failed to create mpegtsdemuxer element"), (NULL));
      return;
    }
    g_signal_connect (demux->fdownloader->demuxer, "pad-added", G_CALLBACK (gst_hlsdemux2_new_pad_added), demux);

    gst_bin_add (GST_BIN (demux->fdownloader->pipe), demux->fdownloader->demuxer);

    /* set queue element to PLAYING state */
    gst_element_set_state (demux->fdownloader->demuxer, GST_STATE_PLAYING);

    if (!gst_element_link (demux->fdownloader->typefind, demux->fdownloader->demuxer)) {
      GST_ERROR_OBJECT (demux, "failed to link src and demux elements...");
      GST_ELEMENT_ERROR (demux, CORE, NEGOTIATION, ("failed to link typefind & demuxer"), (NULL));
      return;
    }
  } else if (gst_structure_has_name (structure, "application/x-id3")) {
    /* found ts fragment */
    demux->fdownloader->demuxer = gst_element_factory_make ("id3demux", "id3demuxer");
    if (!demux->fdownloader->demuxer) {
      GST_ERROR_OBJECT (demux, "failed to create mpegtsdemux element");
      GST_ELEMENT_ERROR (demux, CORE, MISSING_PLUGIN, ("failed to create id3demuxer element"), (NULL));
      return;
    }
    g_signal_connect (demux->fdownloader->demuxer, "pad-added", G_CALLBACK (gst_hlsdemux2_new_pad_added), demux);

    gst_bin_add (GST_BIN (demux->fdownloader->pipe), demux->fdownloader->demuxer);

    /* set queue element to PLAYING state */
    gst_element_set_state (demux->fdownloader->demuxer, GST_STATE_PLAYING);

    if (!gst_element_link_many (demux->fdownloader->typefind, demux->fdownloader->demuxer, NULL)) {
      GST_ERROR_OBJECT (demux, "failed to link src and demux elements...");
      GST_ELEMENT_ERROR (demux, CORE, NEGOTIATION, ("failed to link typefind & demuxer"), (NULL));
      return;
    }

    /* as the fragment does not contain in container format, force the timestamps based on fragment duration */
    demux->fdownloader->force_timestamps = TRUE;
  }
}

static gboolean
gst_hlsdemux2_download_monitor_thread (GstHLSDemux2 *demux)
{
  GTimeVal post_msg_update = {0, };

  g_mutex_lock (demux->post_msg_lock);
  GST_INFO_OBJECT (demux, "waiting for streaming to start to post message...");
  g_cond_wait (demux->post_msg_start, demux->post_msg_lock);
  GST_INFO_OBJECT (demux, "Start posting messages...");
  g_mutex_unlock (demux->post_msg_lock);

  while (TRUE) {
    gboolean do_post = FALSE;
    guint64 percent = 0;
    guint idx = 0;
    guint64 total_percent = 0;
    gboolean bret = FALSE;

    if (demux->cancelled) {
      GST_WARNING_OBJECT (demux, "returning msg posting thread...");
      return TRUE;
    }

    /* schedule the next update using the target duration field of the
     * playlist */
    post_msg_update.tv_sec = 0;
    post_msg_update.tv_usec = 0;

    g_get_current_time (&post_msg_update);
    g_time_val_add (&post_msg_update, 200000); // posts buffering msg after every 0.5 sec

    /* verify fast switch from here */
  /*  g_mutex_lock (demux->fdownloader->lock);
    if (demux->fdownloader->ndownloaded
      && !demux->fdownloader->applied_fast_switch && demux->fdownloader->download_start_ts) {
      gst_hlsdemux2_check_fast_switch (demux);
    }
    g_mutex_unlock (demux->fdownloader->lock);
*/

    /* calculate avg percent of streams' percentages */
    for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
      GstHLSDemux2Stream *stream = demux->streams[idx];
      if (stream ) {
        guint64 stream_percent = 0;

        stream_percent = ((stream->cached_duration * 100) / (demux->total_cache_duration / DEFAULT_NUM_FRAGMENTS_CACHE));
        GST_DEBUG_OBJECT (stream->pad, "stream percent = %"G_GUINT64_FORMAT, stream_percent);
        total_percent += stream_percent;
      }
    }
    percent = total_percent / demux->active_stream_cnt;

    if (demux->is_buffering) {
      do_post = TRUE;
      if (percent >= 100)
        demux->is_buffering = FALSE;
    } else {
      if (percent < 10) {
        /* if buffering drops below 10% lets start posting msgs */
        demux->is_buffering = TRUE;
        do_post = TRUE;
      }
    }

    if (do_post) {
      GstMessage *message;

      if (percent > 100) {
        //GST_LOG_OBJECT (demux, " exceeded percent = %d", (gint) percent);
        percent = 100;
      }

      if (demux->end_of_playlist) {
        GST_INFO_OBJECT (demux, "end of the playlist reached... always post 100%");
        percent = 100;
      }

      if (percent != demux->percent) {
        g_mutex_lock (demux->buffering_lock);
        demux->percent = percent;
        g_mutex_unlock (demux->buffering_lock);

        GST_LOG_OBJECT (demux, "buffering %d percent", (gint) percent);

        /* posting buffering to internal bus, which will take average & post to main bus */
        message = gst_message_new_buffering (GST_OBJECT_CAST (demux), (gint) demux->percent);
        if (!gst_element_post_message (GST_ELEMENT_CAST (demux), message)) {
          GST_WARNING_OBJECT (demux, "failed to post buffering msg...");
        }
      }
    }

    g_mutex_lock (demux->post_msg_lock);
    /*  block until the next scheduled update or the exit signal */
    bret = g_cond_timed_wait (demux->post_msg_exit, demux->post_msg_lock, &post_msg_update);
    g_mutex_unlock (demux->post_msg_lock);

    if (bret) {
      GST_WARNING_OBJECT (demux, "signalled to exit posting msgs...");
      break;
    }
  }

  return TRUE;
}

static gboolean
gst_hlsdemux2_fragurisrc_event_handler (GstPad * pad, GstEvent *event, gpointer data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;

  if (GST_EVENT_TYPE (event) == GST_EVENT_EOS) {
    GTimeVal time = {0, };

    GST_INFO_OBJECT (demux, "received EOS from fragment downloader's httpsrc");

    if (demux->fdownloader->error_rcvd) {
      GST_WARNING_OBJECT (demux, "received EOS event after ERROR.. discard this");
      demux->fdownloader->src_downloaded_size = 0;
      demux->fdownloader->download_stop_ts = demux->fdownloader->download_start_ts = 0;
      return FALSE;
    }

    /* download rate calculation : note down stop time*/
    g_get_current_time (&time);
    demux->fdownloader->download_stop_ts = gst_util_get_timestamp();

    GST_INFO_OBJECT (demux, "download stop_ts = %"G_GUINT64_FORMAT, demux->fdownloader->download_stop_ts);
    GST_INFO_OBJECT (demux, "time taken to download current fragment = %"G_GUINT64_FORMAT, demux->fdownloader->download_stop_ts - demux->fdownloader->download_start_ts);
    GST_INFO_OBJECT (demux, "fragment downloaded size = %"G_GUINT64_FORMAT" and content_size = %"G_GUINT64_FORMAT,
      demux->fdownloader->src_downloaded_size, demux->fdownloader->content_size);

    if (demux->fdownloader->src_downloaded_size && (demux->fdownloader->src_downloaded_size == demux->fdownloader->content_size)) {
      guint64 download_rate = 0;
      guint i = 0;
      guint64 dsum = 0;

      /* calculate current fragment download rate */
      download_rate = (demux->fdownloader->src_downloaded_size * 8 * GST_SECOND) /
        (demux->fdownloader->download_stop_ts - demux->fdownloader->download_start_ts);

      if (demux->fdownloader->avg_frag_drates->len == HLSDEMUX2_MAX_N_PAST_FRAG_DOWNLOADRATES) {
        /* removing starting element */
        g_array_remove_index (demux->fdownloader->avg_frag_drates, 0);
        GST_DEBUG_OBJECT (demux, "removing index from download rates");
      }
      g_array_append_val (demux->fdownloader->avg_frag_drates, download_rate);

      for (i = 0 ; i < demux->fdownloader->avg_frag_drates->len; i++) {
        dsum += g_array_index (demux->fdownloader->avg_frag_drates, guint64, i);
      }

      demux->fdownloader->download_rate = dsum / demux->fdownloader->avg_frag_drates->len;

      GST_INFO_OBJECT (demux, " >>>> DOWNLOAD RATE = %"G_GUINT64_FORMAT, demux->fdownloader->download_rate);
    } else {
      GST_WARNING_OBJECT (demux, "something wrong better not to calculate download rate with this and discard EOS...");
      demux->fdownloader->src_downloaded_size = 0;
      demux->fdownloader->download_stop_ts = demux->fdownloader->download_start_ts = 0;
      return FALSE;
    }

    demux->fdownloader->src_downloaded_size = 0;
    demux->fdownloader->download_stop_ts = demux->fdownloader->download_start_ts = 0;
  }

  return TRUE;
}

static gboolean
gst_hlsdemux2_check_fast_switch (GstHLSDemux2 *demux)
{
  GstClockTime elapsed_time = 0;
  guint i = 0;
  guint64 download_rate = 0;

  elapsed_time = gst_util_get_timestamp();

  if (gst_m3u8_client_has_variant_playlist (demux->client) &&
    ((elapsed_time - demux->fdownloader->download_start_ts) >
      (HLSDEMUX2_FAST_CHECK_WARNING_TIME_FACTOR * demux->target_duration))) {
    guint64 drate_sum = 0;
    GList *previous_variant, *current_variant;
    gint old_bandwidth, new_bandwidth;

    GST_INFO_OBJECT (demux, "Doing fast switch using array len %d...", demux->fdownloader->avg_chunk_drates->len);

    for (i = 0 ; i < demux->fdownloader->avg_chunk_drates->len; i++) {
      drate_sum += g_array_index (demux->fdownloader->avg_chunk_drates, guint64, i);
    }

    /* 0.9 factor is for compensating for connection time*/
    download_rate = (drate_sum * 0.9) / demux->fdownloader->avg_chunk_drates->len;

    GST_INFO_OBJECT (demux, "average chunk download rate = %"G_GUINT64_FORMAT, download_rate);

    if ((elapsed_time - demux->fdownloader->download_start_ts) >
      (HLSDEMUX2_FAST_CHECK_CRITICAL_TIME_FACTOR * demux->target_duration)) {
      GST_WARNING_OBJECT (demux, "Reached FAST_SWITCH critical point...do fast_switch");
      goto fast_switch;
    }

    previous_variant = demux->client->main->current_variant;
    current_variant = gst_m3u8_client_get_playlist_for_bitrate (demux->client, download_rate);

    old_bandwidth = GST_M3U8 (previous_variant->data)->bandwidth;
    new_bandwidth = GST_M3U8 (current_variant->data)->bandwidth;

    GST_INFO_OBJECT (demux, "variant's bandwidth got using average chunk download rate is %d", new_bandwidth);

    /* Don't do anything else if the playlist is the same */
    if (new_bandwidth == old_bandwidth) {
      GST_WARNING_OBJECT (demux, "Either BW is recovering or NO LOWER variant available than current, DON'T DO FAST_SWITCH...");
      return TRUE;
    }

    GST_WARNING_OBJECT (demux, "Going to do fast_switch due to BW problem...");
    goto fast_switch;
  }

  return TRUE;

fast_switch:

  demux->fdownloader->download_rate = download_rate;

  /* flush previous fragment downloads history */
  for (i = 3; i ; --i) {
    g_array_remove_index (demux->fdownloader->avg_frag_drates, i - 1);
  }

  GST_INFO_OBJECT (demux, "in fast download rate : len %d...", demux->fdownloader->avg_frag_drates->len);
  g_array_append_val (demux->fdownloader->avg_frag_drates, demux->fdownloader->download_rate);

  GST_INFO_OBJECT (demux, " >>>> DOWNLOAD RATE in fast switch = %"G_GUINT64_FORMAT, demux->fdownloader->download_rate);

  demux->fdownloader->applied_fast_switch = TRUE;

  /* close current fragment */
  g_cond_signal (demux->fdownloader->cond);

  return FALSE;

}

static gboolean
gst_hlsdemux2_fragurisrc_buffer_handler (GstPad * pad, GstBuffer *buffer, gpointer data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  GstClockTime elapsed_ts = 0;
  guint64 chunk_drate = 0;

  /* if block_switch already enabled no need to cross check. IMP check*/
  if (demux->fdownloader->applied_fast_switch) {
    GST_WARNING_OBJECT (demux, "already applied fast_switch.. drop current buffer");
    return FALSE;
  }

  demux->fdownloader->src_downloaded_size += GST_BUFFER_SIZE(buffer);

  if (demux->fdownloader->first_buffer) {
    /* get the content size for doing last buffer decryption & for vaidating complete download happend or not */
    GST_INFO_OBJECT (demux, "just received first buffer...");
    g_object_get (demux->fdownloader->urisrc, "content-size", &demux->fdownloader->content_size, NULL);
    GST_INFO_OBJECT (demux, "content size = %"G_GUINT64_FORMAT, demux->fdownloader->content_size);
    demux->fdownloader->first_buffer = FALSE;
    demux->fdownloader->chunk_start_ts = gst_util_get_timestamp();
    demux->fdownloader->chunk_downloaded_size = 0;
  }


  demux->fdownloader->chunk_downloaded_size += GST_BUFFER_SIZE(buffer);
  elapsed_ts = gst_util_get_timestamp();

  if ((elapsed_ts - demux->fdownloader->chunk_start_ts) > HLSDEMUX2_CHUNK_TIME_DURATION) {
    GST_DEBUG_OBJECT (demux, "chunk downloaded time = %"GST_TIME_FORMAT,
      GST_TIME_ARGS(elapsed_ts - demux->fdownloader->chunk_start_ts));

    chunk_drate = (demux->fdownloader->chunk_downloaded_size * 8 * GST_SECOND) / (elapsed_ts - demux->fdownloader->chunk_start_ts);
    demux->fdownloader->chunk_start_ts = elapsed_ts;
    demux->fdownloader->chunk_downloaded_size = 0;

    GST_DEBUG_OBJECT(demux, "CHUNK DOWNLOAD RATE = %"G_GUINT64_FORMAT, chunk_drate);

    if (demux->fdownloader->avg_chunk_drates->len == HLSDEMUX2_NUM_CHUNK_DOWNLOADS_FAST_SWITCH) {
      /* removing starting element */
      g_array_remove_index (demux->fdownloader->avg_chunk_drates, 0);
    }
    g_array_append_val (demux->fdownloader->avg_chunk_drates, chunk_drate);
  }

  GST_LOG_OBJECT (demux, "fragment downloaded size at urisrc = %"G_GUINT64_FORMAT, demux->fdownloader->src_downloaded_size);

  return TRUE;
}

static gboolean
gst_hlsdemux2_queue_buffer_handler (GstPad *pad, GstBuffer *buffer, gpointer data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  unsigned char *encry_data = NULL;
  unsigned char *outdata = NULL;
  unsigned char *remained_data = NULL;
  unsigned char* total_data = NULL;

  demux->fdownloader->queue_downloaded_size += GST_BUFFER_SIZE(buffer);

  GST_DEBUG_OBJECT (demux, "total downloaded size = %"G_GUINT64_FORMAT, demux->fdownloader->queue_downloaded_size);

  if (demux->fdownloader->is_encrypted) {
    guint encry_data_size = 0;
    unsigned char *indata = GST_BUFFER_DATA (buffer);
    guint insize = GST_BUFFER_SIZE (buffer);
    gint out_len = 0;

    if (demux->fdownloader->remaining_data && demux->fdownloader->remaining_size) {
      GST_LOG_OBJECT (demux, "remain size = %d..", demux->fdownloader->remaining_size);

      encry_data = (unsigned char *) malloc (insize + demux->fdownloader->remaining_size);
      if (!encry_data) {
        GST_ERROR_OBJECT (demux, "failed to allocate memory...\n");
        GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
        goto error;
      }

      /* copy remaining data in previous iteration */
      memcpy (encry_data, demux->fdownloader->remaining_data, demux->fdownloader->remaining_size);
      /* copy input encrypted data of current iteration */
      memcpy (encry_data + demux->fdownloader->remaining_size, indata, insize);

      encry_data_size = insize + demux->fdownloader->remaining_size;

      free (demux->fdownloader->remaining_data);
      demux->fdownloader->remaining_data = NULL;
      demux->fdownloader->remaining_size = 0;
    } else {
      encry_data_size = insize;
      encry_data = indata;
    }

    demux->fdownloader->remaining_size = encry_data_size % GST_HLS_DEMUX_AES_BLOCK_SIZE;

    if (demux->fdownloader->remaining_size) {
      GST_LOG_OBJECT (demux, "Remining in current iteration = %d\n", demux->fdownloader->remaining_size);
      demux->fdownloader->remaining_data = (gchar *) malloc (demux->fdownloader->remaining_size);
      if ( demux->fdownloader->remaining_data == NULL) {
        GST_ERROR_OBJECT (demux, "failed to allocate memory...\n");
        GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
        goto error;
      }

      /* copy remained portion to buffer */
      memcpy (demux->fdownloader->remaining_data,
              encry_data + encry_data_size - demux->fdownloader->remaining_size,
              demux->fdownloader->remaining_size);
    }

    out_len = encry_data_size - demux->fdownloader->remaining_size + GST_HLS_DEMUX_AES_BLOCK_SIZE;
    outdata = (unsigned char *) malloc (out_len);
    if (!outdata) {
      GST_ERROR_OBJECT (demux, "failed to allocate memory...\n");
      GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
      goto error;
    }

    if (!gst_m3u8_client_decrypt_update (demux->client, outdata, &out_len,
        encry_data, encry_data_size - demux->fdownloader->remaining_size)) {
      GST_ERROR_OBJECT (demux, "failed to decrypt...\n");
      GST_ELEMENT_ERROR (demux, STREAM, FAILED, ("failed to decrypt"), (NULL));
      goto error;
    }

    if (encry_data != indata) {
      g_free (encry_data);
      encry_data = NULL;
    }

    g_free (GST_BUFFER_MALLOCDATA(buffer));

    /* decrypt the final buffer */
    if (demux->fdownloader->queue_downloaded_size == demux->fdownloader->content_size) {
      gint remained_size = 0;
      gint total_len = 0;

      GST_LOG_OBJECT (demux, "remained data = %p & remained size = %d", demux->fdownloader->remaining_data, demux->fdownloader->remaining_size);

      remained_data =  (unsigned char *) malloc (GST_HLS_DEMUX_AES_BLOCK_SIZE);
      if (!remained_data) {
        GST_ERROR_OBJECT (demux, "failed to allocate memory...\n");
        GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
        goto error;
      }
      gst_m3u8_client_decrypt_final (demux->client, remained_data, &remained_size);
      GST_LOG_OBJECT (demux,"remained size = %d\n", remained_size);

      if (remained_size) {
        total_len = out_len + remained_size;

        total_data = (unsigned char *) malloc (total_len);
        if (!total_data) {
          GST_ERROR_OBJECT (demux, "failed to allocate memory...\n");
          GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
          goto error;
        }

        memcpy (total_data, outdata, out_len);
        memcpy (total_data+out_len, remained_data, remained_size);

        GST_BUFFER_MALLOCDATA(buffer) = GST_BUFFER_DATA(buffer) = total_data;
        GST_BUFFER_SIZE(buffer) = total_len;
        g_free (remained_data);
        g_free (outdata);
      } else {
        GST_BUFFER_MALLOCDATA(buffer) = GST_BUFFER_DATA(buffer) = outdata;
        GST_BUFFER_SIZE(buffer) = out_len;
        g_free (remained_data);
      }

      if (!gst_m3u8_client_decrypt_deinit (demux->client)) {
        GST_WARNING_OBJECT (demux, "decryption cleanup failed...");
      }
    } else {
      GST_BUFFER_MALLOCDATA(buffer) = GST_BUFFER_DATA(buffer) = outdata;
      GST_BUFFER_SIZE(buffer) = out_len;
    }
  }

  return TRUE;

error:
  if (encry_data)
    free (encry_data);

  if (outdata)
    free (outdata);

  if (remained_data)
    free (remained_data);

 return FALSE;
}

static gboolean
gst_hlsdemux2_create_fragment_download (GstHLSDemux2 * demux, const gchar * uri)
{
  GstBus *bus = NULL;
  GstPad *srcpad = NULL;

  if (!gst_uri_is_valid (uri)) {
    GST_ERROR_OBJECT (demux, "invalid uri : %s...", uri == NULL ? "NULL" : uri);
    return FALSE;
  }

  /* re-initialize fragment variables */
  demux->fdownloader->first_buffer = TRUE;
  demux->fdownloader->cur_stream_cnt = 0;
  demux->fdownloader->error_rcvd = FALSE;

  demux->fdownloader->pipe = gst_pipeline_new ("frag-fdownloader");
  if (!demux->fdownloader->pipe) {
    GST_ERROR_OBJECT (demux, "failed to create pipeline");
    return FALSE;
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (demux->fdownloader->pipe));
  gst_bus_add_watch (bus, (GstBusFunc)gst_hlsdemux2_fragment_download_bus_cb, demux);
  gst_object_unref (bus);

  GST_INFO_OBJECT (demux, "Creating source element for the URI : %s", uri);

  demux->fdownloader->urisrc = gst_element_make_from_uri (GST_URI_SRC, uri, "fragurisrc");
  if (!demux->fdownloader->urisrc) {
    GST_ERROR_OBJECT (demux, "failed to create urisrc");
    return FALSE;
  }

  g_object_set (G_OBJECT (demux->fdownloader->urisrc), "timeout", HLSDEMUX2_HTTP_TIMEOUT, NULL);
  g_object_set (G_OBJECT (demux->fdownloader->urisrc), "user-agent", demux->user_agent, NULL);
  g_object_set (G_OBJECT (demux->fdownloader->urisrc), "ahs-streaming", TRUE, NULL);
  g_object_set (G_OBJECT (demux->fdownloader->urisrc), "blocksize", DEFAULT_BLOCKSIZE, NULL);
#ifdef GST_EXT_HLS_PROXY_ROUTINE
  if (demux->proxy) {
     GST_DEBUG_OBJECT (demux, "fdownloader Proxy getting set : %s", demux->proxy);
     g_object_set (G_OBJECT (demux->fdownloader->urisrc), "proxy", demux->proxy, NULL);
  }
#endif
  if(demux->fragDomain){
     g_free (demux->fragDomain);
     demux->fragDomain = NULL;
  }
  demux->fragDomain = gst_hlsdemux2_uri_get_domain(demux, uri);
  if(!g_strcmp0 (demux->fragDomain, demux->lastDomain) && demux->lastCookie) {
    g_strfreev(demux->fragCookie);
    demux->fragCookie = g_strdupv (demux->lastCookie);
  }

  if (demux->fragCookie) {
    GST_DEBUG_OBJECT (demux, "Setting cookies before FRAG download goto PLAYING: %s", *(demux->fragCookie));
    g_object_set (demux->fdownloader->urisrc, "cookies", demux->fragCookie, NULL);
  }

  srcpad = gst_element_get_static_pad (demux->fdownloader->urisrc, "src");
  if (!srcpad) {
    GST_ERROR_OBJECT (demux, "failed to get source pad from urisrc...");
    return FALSE;
  }

  gst_pad_add_event_probe (srcpad, G_CALLBACK (gst_hlsdemux2_fragurisrc_event_handler), demux);
  gst_pad_add_buffer_probe (srcpad, G_CALLBACK (gst_hlsdemux2_fragurisrc_buffer_handler), demux);

  gst_object_unref (srcpad);

  demux->fdownloader->queue = gst_element_factory_make ("queue2", "frag_queue");
  if (!demux->fdownloader->queue) {
    GST_ERROR_OBJECT (demux, "failed to create queue2");
    return FALSE;
  }

  g_object_set (G_OBJECT (demux->fdownloader->queue),
    "max-size-buffers", 0,
    "max-size-bytes", 0,
    "max-size-time", 0 * GST_SECOND, NULL);

  srcpad = gst_element_get_static_pad (demux->fdownloader->queue, "src");
  if (!srcpad) {
    GST_ERROR_OBJECT (demux, "failed to get source pad from fragment queue...");
    return FALSE;
  }

  gst_pad_add_buffer_probe (srcpad, G_CALLBACK (gst_hlsdemux2_queue_buffer_handler), demux);

  demux->fdownloader->typefind = gst_element_factory_make ("typefind", "typefinder");
  if (!demux->fdownloader->typefind) {
    GST_ERROR_OBJECT (demux, "failed to create typefind element");
    gst_object_unref (srcpad);
    return FALSE;
  }

  gst_bin_add_many (GST_BIN (demux->fdownloader->pipe),
    demux->fdownloader->urisrc, demux->fdownloader->queue, demux->fdownloader->typefind, NULL);

  if (!gst_element_link_many (demux->fdownloader->urisrc,
    demux->fdownloader->queue, demux->fdownloader->typefind, NULL)) {
    GST_ERROR_OBJECT (demux, "failed to link src, queue & typefind elements...");
    gst_object_unref (srcpad);
    return FALSE;
  }

  /* typefind callback to determine file type */
  g_signal_connect (demux->fdownloader->typefind, "have-type", G_CALLBACK (gst_hlsdemux2_have_type_cb), demux);

  gst_object_unref (srcpad);

  return TRUE;
}

static void
gst_hlsdemux2_destroy_fragment_download (GstHLSDemux2 * demux)
{
  if (demux->fdownloader->pipe) {
    GST_DEBUG_OBJECT (demux, "completely closing fragment downloader pipeline");
    gst_element_set_state (demux->fdownloader->pipe, GST_STATE_NULL);
    gst_element_get_state (demux->fdownloader->pipe, NULL, NULL, GST_CLOCK_TIME_NONE);
    GST_DEBUG_OBJECT (demux, "completely closed fragment downloader pipeline");
    gst_object_unref (demux->fdownloader->pipe);
    demux->has_image_buffer = FALSE;
    demux->fdownloader->pipe = NULL;
    demux->fdownloader->cur_stream_cnt = 0;
    g_list_free (demux->fdownloader->sinkbins); // all sink elements will be unreffed when pipeline is unreffed
    demux->fdownloader->sinkbins = 0;
  }
}

static gboolean
gst_hlsdemux2_download_fragment (GstHLSDemux2 *demux, const gchar * uri)
{
  GstStateChangeReturn ret;

  g_mutex_lock (demux->fdownloader->lock);

  if (demux->cancelled || demux->flushing) {
    GST_WARNING_OBJECT (demux,"returning from download fragment due to cancel or flushing..");
    g_mutex_unlock (demux->fdownloader->lock);
    return FALSE;
  }

  if (demux->frag_uri) {
    g_free (demux->frag_uri);
  }
  demux->frag_uri = g_strdup (uri);

  demux->fdownloader->get_next_frag = FALSE;

  if (!gst_hlsdemux2_create_fragment_download (demux, uri)) {
    GST_ERROR_OBJECT (demux, "failed to create download pipeline");
    g_mutex_unlock (demux->fdownloader->lock);
    return FALSE;
  }

  /* download rate calculation : note down start time*/
  demux->fdownloader->download_start_ts = gst_util_get_timestamp();
  GST_INFO_OBJECT (demux, "download start_ts = %"G_GUINT64_FORMAT, demux->fdownloader->download_start_ts);

  ret = gst_element_set_state (demux->fdownloader->pipe, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    GST_ERROR_OBJECT (demux, "set_state failed...");
    g_mutex_unlock (demux->fdownloader->lock);
    return FALSE;
  }
  gst_element_get_state (demux->fdownloader->pipe, NULL, NULL, -1);

  /* wait until:
   *   - the download succeed (EOS)
   *   - the download failed (Error message on the fetcher bus)
   *   - the download was canceled
   */
  GST_DEBUG_OBJECT (demux, "Waiting to fetch the URI");
  g_cond_wait (demux->fdownloader->cond, demux->fdownloader->lock);
  GST_INFO_OBJECT (demux, "Recived signal to shutdown...");

  if (demux->fragCookie)
    g_strfreev (demux->fragCookie);

  g_object_get (demux->fdownloader->urisrc, "cookies", &demux->fragCookie, NULL);

  GST_DEBUG_OBJECT (demux, "Got cookies after FRAG download : %s", demux->fragCookie ? *(demux->fragCookie) : NULL);

  if (demux->fdownloader->applied_fast_switch) {
    if (demux->fdownloader->src_downloaded_size == demux->fdownloader->content_size) {
      /* to handle case, when both fast_switch & bus_callback signalled at same moment */
      GST_WARNING_OBJECT (demux, "ignoring fast_switch as fragment download is completed...");
      demux->fdownloader->applied_fast_switch = FALSE;
    } else {
      GST_WARNING_OBJECT (demux, "decrement the media sequence by one, due to fast_switch");
      gst_m3u8_client_decrement_sequence (demux->client);
    }
  }

  if (demux->cancelled || demux->flushing ||
      demux->fdownloader->get_next_frag || demux->fdownloader->applied_fast_switch) {
    GstBuffer *buf = NULL;
    guint idx = 0;

    /* clear stream queues if we already downloaded some data */
    for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
      GstHLSDemux2Stream *stream = demux->streams[idx];
      if (stream ) {
        while (!g_queue_is_empty (stream->downloader_queue)) {
          buf = g_queue_pop_head (stream->downloader_queue);
          gst_buffer_unref (buf);
        }
        GST_LOG_OBJECT (stream->pad, "cleared stream download queue...");
        g_queue_clear (stream->downloader_queue);
      }
    }
  }

  if(demux->fragCookie){
    g_strfreev(demux->lastCookie);
    g_free(demux->lastDomain);
    demux->lastCookie = g_strdupv(demux->fragCookie);
    demux->lastDomain = g_strdup(demux->fragDomain);
  }

  if(demux->active_stream_cnt == 1 && !demux->private_stream) {
    gchar *image_header = NULL;
    GValue codec_type = G_VALUE_INIT;
    GValue image_data = G_VALUE_INIT;
    GstMessage * tag_message = NULL;
    struct stat stat_results;
    gint iread = 0;
    GstBuffer *image_buffer = NULL;
    GstTagList * tag_list = gst_tag_list_new();
    int dummy_fp = -1;

    GST_INFO_OBJECT (demux, "Going to show fixed image");
    dummy_fp = open (PREDEFINED_IMAGE_FRAME_LOCATION, O_RDONLY);
    if (dummy_fp < 0) {
      GST_WARNING_OBJECT (demux, "failed to open fixed image file : %s...", PREDEFINED_IMAGE_FRAME_LOCATION);
      goto skip_posting_image;
    }

    GST_LOG_OBJECT (demux, "opened fixed image file %s successfully...", PREDEFINED_IMAGE_FRAME_LOCATION);

    if (fstat (dummy_fp, &stat_results) < 0) {
      GST_WARNING_OBJECT (demux, "failed to get stats of a file...");
      close (dummy_fp);
      goto skip_posting_image;
    }

    GST_LOG_OBJECT (demux, "size of the dummy file = %d", stat_results.st_size);

    image_buffer = gst_buffer_new_and_alloc (stat_results.st_size);
    if (!image_buffer) {
      GST_ERROR_OBJECT (demux, "failed to allocate memory...");
      GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
      close (dummy_fp);
      goto error;
    }

    iread = read (dummy_fp, GST_BUFFER_DATA (image_buffer), stat_results.st_size);

    GST_LOG_OBJECT (demux, "read size = %d successfully", iread);

    close (dummy_fp);

    /* check whether it is same as previous image */
    if (demux->prev_image_buffer &&
      (GST_BUFFER_SIZE(demux->prev_image_buffer) == GST_BUFFER_SIZE(image_buffer))) {
      if (!memcmp (GST_BUFFER_DATA(demux->prev_image_buffer), GST_BUFFER_DATA(image_buffer), GST_BUFFER_SIZE(image_buffer))) {
        GST_INFO_OBJECT (demux, "current & previous embedded images are same..no need to post image again");
        gst_buffer_unref (image_buffer);
        goto skip_posting_image;
      }
    }

    if (demux->prev_image_buffer) {
      gst_buffer_unref (demux->prev_image_buffer);
    }
    demux->prev_image_buffer = gst_buffer_copy(image_buffer);

    g_value_init (&codec_type, G_TYPE_STRING);
    g_value_init (&image_data, GST_TYPE_BUFFER);
    gst_value_set_buffer(&image_data, image_buffer);

    image_header = g_strndup((gchar *) image_buffer->data, 8);

    if((image_header[0] == 0xFF) && (image_header[1] == 0xD8)) {
      GST_INFO_OBJECT(demux, "Found JPEG image header");
      g_value_set_static_string (&codec_type, "image/jpeg");
      gst_tag_list_add_value(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_CODEC, &codec_type);
    } else if (!g_strcmp0(image_header, "\211PNG\015\012\032\012")) {
      GST_INFO_OBJECT(demux, "Found PNG image header");
      g_value_set_static_string (&codec_type, "image/png");
      gst_tag_list_add_value(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_CODEC, &codec_type);
    } else {
      g_value_set_static_string (&codec_type, "image/unknown");
      GST_INFO_OBJECT(demux, "Unknown image header");
      gst_tag_list_add_value(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_CODEC, &codec_type);
    }

    gst_tag_list_add_value (tag_list, GST_TAG_MERGE_APPEND, GST_TAG_IMAGE, &image_data);

    GST_INFO_OBJECT(demux, "Set value : %s", g_value_get_string (&codec_type));
    g_value_unset(&codec_type);

    tag_message = gst_message_new_tag (GST_OBJECT_CAST (demux), tag_list);
    if(!gst_element_post_message (GST_ELEMENT_CAST (demux), tag_message)) {
      GST_ERROR_OBJECT (demux, "failed to post fixed image tag");
    }

    GST_INFO_OBJECT (demux, "successfully posted image tag...");
    gst_buffer_unref (image_buffer);
    g_free (image_header);
  }

skip_posting_image:

  gst_hlsdemux2_destroy_fragment_download (demux);

  if (demux->fdownloader->remaining_data) {
    free (demux->fdownloader->remaining_data);
    demux->fdownloader->remaining_data = NULL;
  }

  demux->fdownloader->remaining_size = 0;
  demux->fdownloader->src_downloaded_size = 0;
  demux->fdownloader->queue_downloaded_size = 0;
  demux->fdownloader->download_stop_ts = demux->fdownloader->download_start_ts = 0;
  demux->fdownloader->applied_fast_switch = FALSE;
  demux->fdownloader->content_size = 0;

  if (demux->cancelled || demux->flushing) {
    GST_WARNING_OBJECT (demux, "returning false due to flushing/cancelled");
    goto error;
  } else if (demux->fdownloader->get_next_frag || demux->fdownloader->applied_fast_switch) {
    GST_INFO_OBJECT (demux, "Requesting next fragment due to error or same sequence number from different variant");
    goto exit;
  }

  demux->fdownloader->ndownloaded++;

  GST_INFO_OBJECT (demux, "number of fragments downloaded = %d", demux->fdownloader->ndownloaded);

  /* reset the count */
  demux->soup_request_fail_cnt = HLSDEMUX2_SOUP_FAILED_CNT;

exit:
  if (demux->private_stream) {
    g_free (demux->private_stream);
    demux->private_stream = NULL;
  }
  g_mutex_unlock (demux->fdownloader->lock);

  return TRUE;

error:
  if (demux->private_stream) {
    g_free (demux->private_stream);
    demux->private_stream = NULL;
  }
  g_mutex_unlock (demux->fdownloader->lock);

  return FALSE;
}

static gboolean
gst_hlsdemux2_change_playlist (GstHLSDemux2 * demux, guint max_bitrate, gboolean *is_switched)
{
  GList *previous_variant, *current_variant;
  gint old_bandwidth, new_bandwidth;

  previous_variant = demux->client->main->current_variant;
  current_variant = gst_m3u8_client_get_playlist_for_bitrate (demux->client, max_bitrate);

  old_bandwidth = GST_M3U8 (previous_variant->data)->bandwidth;
  new_bandwidth = GST_M3U8 (current_variant->data)->bandwidth;

  /* Don't do anything else if the playlist is the same */
  if (new_bandwidth == old_bandwidth) {
    if (is_switched)
      *is_switched = FALSE;
    return TRUE;
  }

  GST_M3U8_CLIENT_LOCK (demux->client);
  demux->client->main->current_variant = current_variant;
  GST_M3U8_CLIENT_UNLOCK (demux->client);

  gst_m3u8_client_set_current (demux->client, current_variant->data);

  GST_INFO_OBJECT (demux, "Client was on %dbps, max allowed is %dbps, switching"
      " to bitrate %dbps", old_bandwidth, max_bitrate, new_bandwidth);

  gst_hlsdemux2_apply_disc(demux);

  if (is_switched)
    *is_switched = TRUE;

  return TRUE;
}

static gboolean
gst_hlsdemux2_get_next_fragment (GstHLSDemux2 * demux, gboolean *is_error)
{
  const gchar *next_fragment_uri = NULL;;
  gchar *next_fragment_key_uri = NULL;
  gchar *iv = NULL;
  GstClockTime duration;
  GstClockTime timestamp;
  gboolean discont = FALSE;
  gboolean bret = TRUE;
  GstClockTime lookup_time = 0;
  guint i = 0;
  GstHLSDemux2Stream *stream = NULL;

  if (!demux->is_live) {
    for (i = 0; i< HLSDEMUX2_STREAM_NUM; i++) {
      stream = demux->streams[i];
      if (stream) {
        lookup_time = stream->lts + stream->frame_duration + (0.5 * GST_SECOND);
        GST_INFO_OBJECT (stream->pad, "Lookup time = %"GST_TIME_FORMAT, GST_TIME_ARGS(lookup_time));
        stream->frame_duration = 0;
        if (stream->type == HLSDEMUX2_STREAM_AUDIO)
          break;
      }
    }
  }

  if (!gst_m3u8_client_get_next_fragment (demux->client, lookup_time,
    &discont, &next_fragment_uri, &duration, &timestamp, &next_fragment_key_uri, &iv)) {

    GST_INFO_OBJECT (demux, "This playlist doesn't contain more fragments");

    if (demux->is_live && (demux->client->update_failed_count < DEFAULT_FAILED_COUNT)) {
      GST_WARNING_OBJECT (demux, "Could not get next fragment, try again after updating playlist...");
      *is_error = FALSE;
      if (next_fragment_uri)
        g_free ((gpointer)next_fragment_uri);
      if (next_fragment_key_uri)
        g_free ((gpointer)next_fragment_key_uri);
      if (iv)
        g_free ((gpointer)iv);
      return FALSE;
    } else {
      goto end_of_list;
    }
  }

  demux->cfrag_dur = duration;
  demux->fdownloader->cur_running_dur += duration;

  GST_LOG_OBJECT (demux, "current running duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(demux->fdownloader->cur_running_dur));

  GST_DEBUG_OBJECT (demux, "Fetching next fragment = %s & key uri = %s", next_fragment_uri, next_fragment_key_uri);

  /* when hls requests next fragment due to some error like NOT_FOUND, then also we need to set apply disc */
  if (discont || demux->fdownloader->get_next_frag)
    gst_hlsdemux2_apply_disc (demux);

  if (next_fragment_key_uri) {
    /* download key data & initialize decryption with key data & IV */
    GST_INFO_OBJECT (demux, "Fetching next fragment key %s", next_fragment_key_uri);

    if (demux->kdownloader->prev_key_uri) {
      if (strcmp (demux->kdownloader->prev_key_uri, next_fragment_key_uri)) {
        /* if previous & current key uris are different, download new one */
        if (demux->kdownloader->key) {
          gst_buffer_unref (demux->kdownloader->key);
          demux->kdownloader->key = NULL;
        }

        if (demux->kdownloader->prev_key_uri)
          g_free (demux->kdownloader->prev_key_uri);

        demux->kdownloader->prev_key_uri = g_strdup(next_fragment_key_uri);

        /* download the key data as there is a change */
        if (!gst_hlsdemux2_download_key (demux, next_fragment_key_uri)) {
          GST_ERROR_OBJECT (demux, "failed to download key...");
          goto error;
        }
      } else {
        GST_INFO_OBJECT (demux, "already having the key...no need to download again");
      }
    } else {
      if (demux->kdownloader->key) {
        gst_buffer_unref (demux->kdownloader->key);
        demux->kdownloader->key = NULL;
      }

      demux->kdownloader->prev_key_uri = g_strdup(next_fragment_key_uri);

      if (!gst_hlsdemux2_download_key (demux, next_fragment_key_uri)) {
        GST_ERROR_OBJECT (demux, "failed to download key...");
        goto error;
      }
      /* reset the count */
      demux->soup_request_fail_cnt = HLSDEMUX2_SOUP_FAILED_CNT;
    }

    if (!demux->kdownloader->key || !GST_BUFFER_DATA (demux->kdownloader->key)) {
      GST_ERROR_OBJECT (demux, "eos received without key content...");
      goto error;
    }

    if (!gst_m3u8_client_decrypt_init (demux->client, GST_BUFFER_DATA (demux->kdownloader->key), iv)) {
      GST_ERROR_OBJECT (demux, "failed to initialize AES decryption...");
      goto error;
    }

    demux->fdownloader->is_encrypted = TRUE;
  }

  if (!gst_hlsdemux2_download_fragment (demux, next_fragment_uri)) {
    GST_ERROR_OBJECT (demux, "failed to download fragment...");
    goto error;
  }

exit:
  if (next_fragment_uri)
    g_free ((gpointer)next_fragment_uri);
  if (next_fragment_key_uri)
    g_free ((gpointer)next_fragment_key_uri);
  if (iv)
    g_free ((gpointer)iv);

  return bret;

error:
  if (!demux->flushing)
    *is_error = TRUE;
  bret = FALSE;
  goto exit;

 end_of_list:
  GST_INFO_OBJECT (demux, "Reached end of playlist, sending EOS");

  gst_hlsdemux2_push_eos (demux);

  demux->end_of_playlist = TRUE;
  bret = FALSE;
  goto exit;
}


static gboolean
gst_hlsdemux2_switch_playlist (GstHLSDemux2 * demux, gboolean *is_switched)
{
  GST_M3U8_CLIENT_LOCK (demux->client);
  if (!demux->client->main->lists) {
    /* not a variant to switch */
    GST_M3U8_CLIENT_UNLOCK (demux->client);
    GST_INFO_OBJECT (demux, "not a variant to switch...");
    return TRUE;
  }
  GST_M3U8_CLIENT_UNLOCK (demux->client);

  if (demux->force_lower_bitrate && (demux->fdownloader->ndownloaded % FORCE_LOW_BITRATE_AFTER_CNT == 0)) {
    demux->fdownloader->download_rate = 0; // Using some temp lowest value
    GST_WARNING_OBJECT (demux, "resetting to lowest one bitrate = %"G_GUINT64_FORMAT, demux->fdownloader->download_rate);
  }

  return gst_hlsdemux2_change_playlist (demux, demux->fdownloader->download_rate, is_switched);
}

static void
gst_hlsdemux2_apply_disc (GstHLSDemux2 * demux)
{
  int i = 0;
  GstHLSDemux2Stream *stream = NULL;

  for (i = 0; i< HLSDEMUX2_STREAM_NUM; i++) {
    stream = demux->streams[i];

    if (stream) {
      stream->apply_disc = TRUE;
      GST_INFO_OBJECT (stream->pad, "apply discontinuity...");
    }
  }
}

static void
gst_hlsdemux2_fragment_download_loop (GstHLSDemux2 * demux)
{
  gboolean is_error = FALSE;
  gboolean bret = FALSE;
  GTimeVal tmp_update = {0, };
  GTimeVal current;
  guint64 current_time = 0;
  guint64 nextupdate_time = 0;

#ifdef TEMP_FIX_FOR_TASK_STOP
  if (demux->download_task == NULL)
  {
	GST_INFO_OBJECT (demux, "gst_hlsdemux2_fragment_download_loop : Download Task is already deleted !!!!!! RETURNING !!!!!");
	return;
  }
#endif
  /* if variant playlist, get current subplaylist first */
  if (gst_m3u8_client_has_variant_playlist(demux->client)) {
    gboolean is_error = FALSE;

    if (!gst_hlsdemux2_update_playlist (demux, FALSE, &is_error)) {
      GST_ERROR_OBJECT (demux, "failed to update playlist. uri : %s", gst_m3u8_client_get_current_uri(demux->client));
      GST_ELEMENT_ERROR (demux, RESOURCE, NOT_FOUND, ("Could not update the playlist"), (NULL));
      goto exit;
    }
  } else if (demux->is_live) {
    /* single variant live.. so schedule for starting */
    if (demux->is_live)
      gst_hlsdemux2_schedule (demux);
  }

  if (demux->fdownloader->find_mediaseq) {
    GList *walk;
    GstClockTime current_pos, target_pos;
    gint current_sequence;
    GstM3U8MediaFile *file;
    guint i = 0;
    GstHLSDemux2Stream *stream = NULL;

    GST_M3U8_CLIENT_LOCK (demux->client);
    file = GST_M3U8_MEDIA_FILE (demux->client->current->files->data);
    current_sequence = file->sequence;
    current_pos = 0 ;
    target_pos = demux->fdownloader->seeked_pos;
    for (walk = demux->client->current->files; walk; walk = walk->next) {
      file = walk->data;

      current_sequence = file->sequence;
      if (current_pos <= target_pos &&
        target_pos < current_pos + file->duration) {
        break;
      }
      current_pos += file->duration;
    }
    GST_M3U8_CLIENT_UNLOCK (demux->client);

    if (walk == NULL) {
      GST_WARNING_OBJECT (demux, "Could not find seeked fragment");
    }
    GST_M3U8_CLIENT_LOCK (demux->client);
    GST_INFO_OBJECT (demux, "seeking to sequence %d", current_sequence);
    demux->client->sequence = current_sequence;
    GST_M3U8_CLIENT_UNLOCK (demux->client);
    demux->ns_start = demux->fdownloader->cur_running_dur = current_pos;  // NON-accurate seek

    for (i = 0; i< HLSDEMUX2_STREAM_NUM; i++) {
      stream = demux->streams[i];
#ifdef LATEST_AV_SYNC
      if (stream) {
        stream->lts = stream->total_stream_time = current_pos;
        GST_INFO_OBJECT (stream->pad, "Changed stream->lts to %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->lts));
      }
#else
      if (stream && stream->type == HLSDEMUX2_STREAM_AUDIO) {
        stream->lts  = current_pos;
      }
#endif
    }

    demux->fdownloader->find_mediaseq = FALSE;
  }

  demux->target_duration = gst_m3u8_client_get_target_duration(demux->client);
  demux->total_cache_duration = DEFAULT_NUM_FRAGMENTS_CACHE * demux->target_duration;
  demux->is_live = gst_m3u8_client_is_live(demux->client);

  GST_INFO_OBJECT (demux, "Total cache duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(demux->total_cache_duration));

  while (TRUE) {

    if (demux->cancelled)
      goto exit;

    /* get the next fragement */
    bret = gst_hlsdemux2_get_next_fragment (demux, &is_error);

    if (!bret) {
      if (is_error) {
        GST_ERROR_OBJECT (demux, "error in getting next fragment");
        GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("could not download fragment"), (NULL));
        goto exit;
      } else {
        if (!demux->end_of_playlist) {
          if (demux->flushing) {
            GST_WARNING_OBJECT (demux, "SEEK is in progress.. pause the task");
            goto exit;
          } else if (demux->client->update_failed_count < DEFAULT_FAILED_COUNT) {
            GST_WARNING_OBJECT (demux, "Could not update fragment & playlist update failed count = %d",
              demux->client->update_failed_count);
          } else {
            GST_ERROR_OBJECT (demux, "Could not get next fragment..");
            GST_ELEMENT_ERROR (demux, RESOURCE, NOT_FOUND, ("Could not get next fragment..."), (NULL));
            goto exit;
          }
        } else {
          GST_INFO_OBJECT (demux, "end of fragment download, doing pause");
          goto exit;
        }
      }
    } else {
      /* reset the update failed count */
      demux->client->update_failed_count = 0;
    }

    if (demux->cancelled)
      goto exit;

    if (demux->is_live) {
      /* get the current time */
      current.tv_sec = current.tv_usec = 0;
      g_get_current_time (&current);

      current_time = (current.tv_sec * (guint64)1000000)+ current.tv_usec;
      nextupdate_time = (demux->next_update.tv_sec * (guint64)1000000)+ demux->next_update.tv_usec;
    }

    if (demux->is_live && ((current_time > nextupdate_time) || !bret)) {
      gboolean is_switched = FALSE;

      /* try to switch to another bitrate if needed */
      // TODO: take care of return value
      gst_hlsdemux2_switch_playlist (demux, &is_switched);

      g_mutex_lock (demux->pl_update_lock);

      /*  block until the next scheduled update or the exit signal */
      bret = g_cond_timed_wait (demux->pl_update_cond, demux->pl_update_lock, &demux->next_update);

      g_mutex_unlock (demux->pl_update_lock);

      GST_DEBUG_OBJECT (demux, "playlist update wait is completed. reason : %s",  bret ? "Received signal" : "time-out");

      tmp_update.tv_sec = 0;
      tmp_update.tv_usec = 0;

      g_get_current_time (&tmp_update);

      if (bret) {
        GST_DEBUG_OBJECT (demux, "Sombody signalled manifest waiting... going to exit and diff = %ld",
            ((demux->next_update.tv_sec * 1000000)+ demux->next_update.tv_usec) - ((tmp_update.tv_sec * 1000000)+ tmp_update.tv_usec));
        goto exit;
      } else if (demux->cancelled) {
        GST_WARNING_OBJECT (demux, "closing is in progress..exit now");
        goto exit;
      }

      if (!gst_hlsdemux2_update_playlist (demux, FALSE, &is_error)) {
        if (is_error) {
          GST_ERROR_OBJECT (demux, "failed to update playlist...");
          GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("failed to update playlist"), (NULL));
          goto exit;
        } else {
          if (demux->flushing) {
            GST_WARNING_OBJECT (demux, "SEEK is in progress.. pause the task");
            goto exit;
          } else if (demux->client->update_failed_count < DEFAULT_FAILED_COUNT) {
            GST_WARNING_OBJECT (demux, "Could not update playlist & playlist update failed count = %d",
              demux->client->update_failed_count);
            continue;
          } else {
            GST_ERROR_OBJECT (demux, "Could not update playlist..");
            GST_ELEMENT_ERROR (demux, RESOURCE, NOT_FOUND, ("Could not update playlist..."), (NULL));
            goto exit;
          }
        }
      }

    } else {
      gboolean is_switched = FALSE;

      /* try to switch to another bitrate if needed */
      // TODO: take care of return value

      /* when in recovery mode, do not obey download rates */
      if (demux->pldownloader->recovery_mode == HLSDEMUX2_NO_RECOVERY)
        gst_hlsdemux2_switch_playlist (demux, &is_switched);

      if (is_switched) {
        if (!gst_hlsdemux2_update_playlist (demux, FALSE, &is_error)) {
          if (is_error) {
            GST_ERROR_OBJECT (demux, "failed to update playlist...");
            GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("failed to update playlist"), (NULL));
            goto exit;
          } else {
            if (demux->flushing) {
              GST_WARNING_OBJECT (demux, "SEEK is in progress.. pause the task");
              goto exit;
            } else if (demux->client->update_failed_count < DEFAULT_FAILED_COUNT) {
              GST_WARNING_OBJECT (demux, "Could not update playlist & playlist update failed count = %d",
                demux->client->update_failed_count);
              continue;
            } else {
              GST_ERROR_OBJECT (demux, "Could not update playlist..");
              GST_ELEMENT_ERROR (demux, RESOURCE, NOT_FOUND, ("Could not update playlist..."), (NULL));
              goto exit;
            }
          }
        }
      } else if (demux->is_live) {
        GstClockTime current_time = gst_util_get_timestamp();

        if ((current_time - demux->pldownloader->download_start_ts) <
          (PLAYLIST_REFRESH_TIMEOUT - (HLSDEMUX2_FAST_CHECK_CRITICAL_TIME_FACTOR * demux->target_duration) - HLSDEMUX2_OVERHEAD)) {
          GST_INFO_OBJECT (demux, "Eligible to download next fragment w/o playlist update : "
            "elapsed time since last playlist request = %"GST_TIME_FORMAT" and max_time_limit = %"GST_TIME_FORMAT,
            GST_TIME_ARGS(current_time - demux->pldownloader->download_start_ts),
            GST_TIME_ARGS(PLAYLIST_REFRESH_TIMEOUT - (HLSDEMUX2_FAST_CHECK_CRITICAL_TIME_FACTOR * demux->target_duration) - HLSDEMUX2_OVERHEAD));
        } else {

          GST_INFO_OBJECT (demux, "NOT Eligible to download next fragment w/o playlist update : "
            "elapsed time since previous request = %"GST_TIME_FORMAT" and max_time_limit = %"GST_TIME_FORMAT,
            GST_TIME_ARGS(current_time - demux->pldownloader->download_start_ts),
            GST_TIME_ARGS(PLAYLIST_REFRESH_TIMEOUT - (HLSDEMUX2_FAST_CHECK_CRITICAL_TIME_FACTOR * demux->target_duration) - HLSDEMUX2_OVERHEAD));

          /* we are not eligible to download, next fragment.. So wait till next playlist update */
          g_mutex_lock (demux->pl_update_lock);
          /*  block until the next scheduled update or the exit signal */
          bret = g_cond_timed_wait (demux->pl_update_cond, demux->pl_update_lock, &demux->next_update);
          g_mutex_unlock (demux->pl_update_lock);

          GST_DEBUG_OBJECT (demux, "playlist update wait is completed. reason : %s",  bret ? "Received signal" : "time-out");

          tmp_update.tv_sec = 0;
          tmp_update.tv_usec = 0;

          g_get_current_time (&tmp_update);

          if (bret) {
            GST_DEBUG_OBJECT (demux, "Sombody signalled manifest waiting... going to exit and diff = %ld",
                ((demux->next_update.tv_sec * 1000000)+ demux->next_update.tv_usec) - ((tmp_update.tv_sec * 1000000)+ tmp_update.tv_usec));
            goto exit;
          } else if (demux->cancelled) {
            GST_WARNING_OBJECT (demux, "closing is in progress..exit now");
            goto exit;
          }

          if (!gst_hlsdemux2_update_playlist (demux, FALSE, &is_error)) {
            if (is_error) {
              GST_ERROR_OBJECT (demux, "failed to update playlist...");
              GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("failed to update playlist"), (NULL));
              goto exit;
            } else {
              if (demux->flushing) {
                GST_WARNING_OBJECT (demux, "SEEK is in progress.. pause the task");
                goto exit;
              } else if (demux->client->update_failed_count < DEFAULT_FAILED_COUNT) {
                GST_WARNING_OBJECT (demux, "Could not update playlist & playlist update failed count = %d",
                  demux->client->update_failed_count);
                continue;
              } else {
                GST_ERROR_OBJECT (demux, "Could not update playlist..");
                GST_ELEMENT_ERROR (demux, RESOURCE, NOT_FOUND, ("Could not update playlist..."), (NULL));
                goto exit;
              }
            }
          }
        }
      }
    }
  }

exit:
  GST_WARNING_OBJECT (demux, "Going to PAUSE download task from self");
  gst_task_pause (demux->download_task);
  return;
}

static void
gst_hlsdemux2_calculate_popped_duration (GstHLSDemux2 *demux, GstHLSDemux2Stream *stream,
    GstBuffer *outbuf)
{
  gpointer data = NULL;
  gint i = g_queue_get_length (stream->queue);

  /* get the buffer with valid timestamp from tail of the queue */
  while (i > 1) {
    data = g_queue_peek_nth (stream->queue, (i - 1));
    if (data == NULL) {
      return;
    }

    if (GST_IS_BUFFER ((GstBuffer *)data)) {
      if (GST_BUFFER_TIMESTAMP_IS_VALID(data)) {
        /* found valid timestamp & stop searching */
        GST_LOG_OBJECT (stream->pad, "data found buffer with valid ts = %"GST_TIME_FORMAT " len = %d & i = %d",
          GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(data)), g_queue_get_length (stream->queue), i);
        break;
      }
    }
    i--;
  }

  if (data == NULL) {
    return;
  }

  if (GST_BUFFER_TIMESTAMP (outbuf) <= GST_BUFFER_TIMESTAMP ((GstBuffer *)data)) {
    stream->cached_duration = GST_BUFFER_TIMESTAMP ((GstBuffer *)data) - GST_BUFFER_TIMESTAMP (outbuf);
    GST_DEBUG_OBJECT (stream->pad, "cache duration in popping : %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->cached_duration));
  } else {
    GST_ERROR_OBJECT (stream->pad, "Wrong order.. not calculating");
  }

  return;
}

static void
gst_hlsdemux2_push_loop (GstHLSDemux2Stream *stream)
{
  GstHLSDemux2 *demux = stream->parent;
  GstFlowReturn fret = GST_FLOW_OK;
  gpointer data = NULL;

  // TODO: need to take care of EOS handling....

  if (demux->cancelled || demux->flushing)
    goto error;

  g_mutex_lock (stream->queue_lock);

  if (g_queue_is_empty (stream->queue)) {
    GST_LOG_OBJECT (stream->pad,"queue is empty wait till, some buffers are available...");
    g_cond_wait (stream->queue_empty, stream->queue_lock);
    GST_LOG_OBJECT (stream->pad,"Received queue empty signal...");
  }

  if (demux->cancelled || demux->flushing) {
    GST_ERROR_OBJECT (stream->pad, "Shutting down push loop");
    g_mutex_unlock (stream->queue_lock);
    goto error;
  }

  if (g_queue_is_empty (stream->queue)) {
    GST_ERROR_OBJECT (stream->pad, "Queue empty undesired....");
    g_mutex_unlock (stream->queue_lock);
    goto error;
  }

  data = g_queue_pop_head (stream->queue);
  if (!data) {
    GST_ERROR_OBJECT (stream->pad, "Received null data...");
    GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Unhandled GstObjectType"), (NULL));
    g_mutex_unlock (stream->queue_lock);
    goto error;
  }

  /*  Calculate duration only when
    *  1. popped obj is a buffer and
    *  2. buffer has valid ts and
    *  3. queue length > 1
   */

  if (GST_IS_BUFFER(data) &&
     GST_BUFFER_TIMESTAMP_IS_VALID(data) ) {

    /* calculate buffering duration */
    gst_hlsdemux2_calculate_popped_duration (demux, stream, data);

    if (stream->cached_duration < 0) {
      GST_WARNING_OBJECT (stream->pad, "Wrong cached duration in push_loop...\n");
      stream->cached_duration = 0;
    }
  }

  g_mutex_unlock (stream->queue_lock);

  if (GST_IS_EVENT(data)) {
    GstEvent *event = (GstEvent *)data;

    switch (GST_EVENT_TYPE (event)) {
      case GST_EVENT_NEWSEGMENT: {
        GstFormat format;
        gboolean update;
        gdouble rate, arate;
        gint64 start, stop, pos;
        gst_event_parse_new_segment_full(event, &update, &rate, &arate, &format, &start, &stop, &pos);
        GST_INFO_OBJECT (stream->pad, "Pushing newsegment to downstream : rate = %0.3f, start = %"GST_TIME_FORMAT
          ", stop = %"GST_TIME_FORMAT, rate, GST_TIME_ARGS(start), GST_TIME_ARGS(stop));
      }
      break;
      case GST_EVENT_EOS:
      GST_INFO_OBJECT (stream->pad, "going to push eos event to downstream");
      stream->eos = TRUE;
      break;
      default:
      GST_ERROR_OBJECT (stream->pad, "not handled event...still pushing event");
      break;
    }
    if (!gst_pad_push_event (stream->pad, event)) {
      GST_ERROR_OBJECT (demux, "failed to push newsegment event");
      GST_ELEMENT_ERROR (demux, CORE, PAD, ("failed to push '%s' event", GST_EVENT_TYPE_NAME(event)), (NULL));
      goto error;
    }

    GST_INFO_OBJECT (stream->pad, "Pushed '%s' event...", GST_EVENT_TYPE_NAME(event));

    if (stream->eos) {
      //gst_element_post_message (GST_ELEMENT_CAST (stream->sink), gst_message_new_eos (GST_OBJECT_CAST(stream->sink)));
      GST_INFO_OBJECT (stream->pad, "Pausing the task");
      goto error;
    }
  } else if (GST_IS_BUFFER(data)) {
    GstCaps *temp_caps = NULL;
    g_cond_signal (stream->queue_full);

    GST_DEBUG_OBJECT (stream->pad, "Pushing buffer : size = %d, ts = %"GST_TIME_FORMAT", dur = %"GST_TIME_FORMAT,
      GST_BUFFER_SIZE(data), GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(data)), GST_TIME_ARGS(GST_BUFFER_DURATION(data)));

    temp_caps = gst_buffer_get_caps((GstBuffer *)data);
    temp_caps = gst_caps_make_writable (temp_caps);
    gst_caps_set_simple (temp_caps,"hls_streaming", G_TYPE_BOOLEAN, TRUE, NULL);
    gst_buffer_set_caps((GstBuffer *)data, temp_caps);
    gst_caps_unref(temp_caps);

    /* push data to downstream*/
    fret = gst_pad_push (stream->pad, data);
    if (fret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (stream->pad, "failed to push data, reason : %s", gst_flow_get_name (fret));
      goto error;
    }
  } else {
    GST_ERROR_OBJECT (stream->pad, "unhandled object type..still pushing");
    //GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Unhandled GstObjectType"), (NULL));
    //return;
    gst_object_unref (data);
  }

  if (demux->cancelled || demux->flushing) {
    goto error;
  }

  return;

error:
  {
    GST_WARNING_OBJECT (stream->pad, "Pausing the push task...");

    if (fret < GST_FLOW_UNEXPECTED) {
      GST_ERROR_OBJECT (stream->pad, "Crtical error in push loop....");
      GST_ELEMENT_ERROR (demux, CORE, PAD, ("failed to push. reason - %s", gst_flow_get_name (fret)), (NULL));
    }

    if(data) {
      gst_object_unref (data);
      data = NULL;
    }

    gst_pad_pause_task (stream->pad);
    return;
  }
}

// TODO: need to review change_playlist API

static gboolean
gst_hlsdemux2_schedule (GstHLSDemux2 * demux)
{
  gfloat update_factor;
  gint count;
  GstClockTime last_frag_duration;

  /* As defined in §6.3.4. Reloading the Playlist file:
   * "If the client reloads a Playlist file and finds that it has not
   * changed then it MUST wait for a period of time before retrying.  The
   * minimum delay is a multiple of the target duration.  This multiple is
   * 0.5 for the first attempt, 1.5 for the second, and 3.0 thereafter."
   */
  count = demux->client->update_failed_count;
  if (count < 3)
    update_factor = update_interval_factor[count];
  else
    update_factor = update_interval_factor[3];

  /* schedule the next update using the target duration field of the
   * playlist */
  demux->next_update.tv_sec = 0;
  demux->next_update.tv_usec = 0;

  last_frag_duration = gst_m3u8_client_get_last_fragment_duration (demux->client);

  GST_DEBUG_OBJECT (demux, "last fragment duration = %"GST_TIME_FORMAT" and next update = %"GST_TIME_FORMAT,
    GST_TIME_ARGS(last_frag_duration), GST_TIME_ARGS(GST_TIME_AS_USECONDS(last_frag_duration) * 1000));

  g_get_current_time (&demux->next_update);

  //g_time_val_add (&demux->next_update, last_frag_duration / GST_SECOND * G_USEC_PER_SEC * update_factor);
  g_time_val_add (&demux->next_update, GST_TIME_AS_USECONDS(last_frag_duration)* update_factor);

  return TRUE;
}

static void
gst_hlsdemux2_new_pad_added (GstElement *element, GstPad *srcpad, gpointer data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  GstPad *sinkpad = NULL;
  GstCaps *caps = NULL;
  HLSDemux2SinkBin *sinkbin = NULL;
  gchar *demuxer_name = NULL;

  GST_INFO_OBJECT (demux, "received the src_pad '%s' from mpeg2ts demuxer...", GST_PAD_NAME(srcpad));

  if (strstr (GST_PAD_NAME(srcpad), "private")) {
    /* handle private image TAG */
    gst_hlsdemux2_handle_private_pad (demux, srcpad);
    return;
  }

  caps = GST_PAD_CAPS (srcpad);
  demuxer_name = GST_ELEMENT_NAME (element);

  sinkbin = gst_hlsdemux2_create_stream (demux, demuxer_name, caps);
  if (!sinkbin) {
    return;
  }

  sinkpad = gst_element_get_pad(GST_ELEMENT(sinkbin->sinkbin), "sink");
  if (!sinkpad) {
    GST_ERROR_OBJECT (demux, "failed to get sinkpad from element  - %s", GST_PAD_NAME(sinkbin->sinkbin));
    GST_ELEMENT_ERROR (demux, CORE, PAD, ("could not get sink pad"), (NULL));
    return;
  }

  /* link demuxer srcpad & sink element's sink pad */
  if (gst_pad_link(srcpad, sinkpad) != GST_PAD_LINK_OK) {
    GST_ERROR_OBJECT (demux, "failed to link pad '%s' &  sink's sink pad", GST_PAD_NAME(srcpad));
    GST_ELEMENT_ERROR (demux, CORE, NEGOTIATION, ("Could not link pads.."), (NULL));
    gst_object_unref(sinkpad);
    return;
  }

  if (sinkbin->parser) {
    GST_INFO_OBJECT (demux, "linking parser");
    if (!gst_element_link_many (sinkbin->queue, sinkbin->parser, sinkbin->sink, NULL)) {
      GST_ERROR_OBJECT (demux, "failed to link sink bin elements");
    GST_ELEMENT_ERROR (demux, CORE, NEGOTIATION, ("Could not link pads.."), (NULL));
      gst_object_unref(sinkpad);
      return;
    }
  } else {
    if (!gst_element_link_many (sinkbin->queue, sinkbin->sink, NULL)) {
      GST_ERROR_OBJECT (demux, "failed to link sink bin elements");
      GST_ELEMENT_ERROR (demux, CORE, NEGOTIATION, ("Could not link pads.."), (NULL));
      gst_object_unref(sinkpad);
      return;
    }
  }
  demux->fdownloader->cur_stream_cnt++;

  GST_INFO_OBJECT (demux, "succesfully linked pad (%s) with stream pad and current stream cnt = %d",
    gst_pad_get_name(srcpad), demux->fdownloader->cur_stream_cnt);

  gst_element_set_state (GST_ELEMENT(sinkbin->sinkbin), GST_STATE_PLAYING);

  if (demux->gsinkbin) {
    g_free (demux->gsinkbin);
    demux->gsinkbin = NULL;
  }

  demux->gsinkbin = sinkbin;

  gst_object_unref (sinkpad);
}

static gboolean
gst_hlsdemux2_check_stream_type_exist (GstHLSDemux2 *demux, HLSDEMUX2_STREAM_TYPE stream_type)
{
  int i = 0;
  GstHLSDemux2Stream *stream = NULL;

  for (i = 0; i< HLSDEMUX2_STREAM_NUM; i++) {
    stream = demux->streams[i];

    if (stream && (stream->type == stream_type)) {
      GST_INFO_OBJECT (demux, "stream type '%d' is already present", stream_type);
      return TRUE;
    }
  }

  GST_INFO_OBJECT (demux, "received new stream type [%d]", stream_type);

  return FALSE;
}

static HLSDemux2SinkBin *
gst_hlsdemux2_create_stream (GstHLSDemux2 *demux, gchar *demuxer_name, GstCaps *caps)
{
  GstHLSDemux2Stream *stream = NULL;
  GstPad *sinkpad = NULL;
  GstPad *linkpad = NULL;
  HLSDEMUX2_STREAM_TYPE stream_type;
  gchar *element_name = NULL;
  gboolean stream_exist = FALSE;
  gulong sink_probe;
  HLSDemux2SinkBin *bin = NULL;
  GstStructure *structure = NULL;
  gchar *mime = NULL;
  gchar *stream_name = NULL;

  //stream = g_new0 (GstHLSDemux2Stream, 1);

  structure = gst_caps_get_structure (caps, 0);

  mime = gst_structure_get_name (structure);

  if (strstr (mime, "video")) {
    stream_type = HLSDEMUX2_STREAM_VIDEO;
    GST_DEBUG_OBJECT (demux, "Its a Multi-variant, Audio only stream..");
    demux->stream_config = HLSDEMUX2_MULTI_VARIANT;
    stream_name = g_strdup ("video");
  } else if (strstr (mime, "audio")) {
    stream_type = HLSDEMUX2_STREAM_AUDIO;
    stream_name = g_strdup ("audio");
  } else if (strstr (mime, "subpicture")) {
    stream_type = HLSDEMUX2_STREAM_TEXT;
    stream_name = g_strdup ("subpicture");
  } else if (strstr (mime, "private")) {
    stream_type = HLSDEMUX2_STREAM_PRIVATE;
    stream_name = g_strdup ("private");
  } else {
    GST_ELEMENT_ERROR (demux, CORE, PAD, ("Received unknown named pad"), (NULL));
    goto error;
  }

  stream_exist = gst_hlsdemux2_check_stream_type_exist (demux, stream_type);

  if (!stream_exist) {
    stream = g_new0 (GstHLSDemux2Stream, 1);
    gst_hlsdemux2_stream_init (demux, stream, stream_type, stream_name, caps);

    if (!stream->is_linked) {
      goto error;
    }
  } else {
    int i = 0;

    for (i = 0; i< HLSDEMUX2_STREAM_NUM; i++) {
      stream = demux->streams[i];
      if (stream && (stream->type == stream_type)) {
        GST_INFO_OBJECT (demux, "found the stream - '%d'", stream->type);
        break;
      }
    }
  }

  if (!stream)
    goto error;

  bin = g_new0 (HLSDemux2SinkBin, 1);

  element_name = g_strdup_printf("%s-%s", stream_name, "bin");
  bin->sinkbin = GST_BIN(gst_bin_new (element_name));
  if (!bin->sinkbin) {
    GST_ERROR_OBJECT (demux, "failed to create sink bin - %s", element_name);
    GST_ELEMENT_ERROR (demux, CORE, FAILED, ("failed to create bin - %s", element_name), (NULL));
    goto error;
  }
  g_free (element_name);
  element_name = NULL;

  /* create queue element */
  element_name = g_strdup_printf("%s-%s", stream_name, "queue");
  bin->queue =  gst_element_factory_make ("queue", element_name);
  if (!bin->queue) {
    GST_ERROR_OBJECT (demux, "failed to create queue element - %s", element_name);
    GST_ELEMENT_ERROR (demux, CORE, FAILED, ("failed to create element - %s", element_name), (NULL));
    goto error;
  }
  g_free (element_name);
  element_name = NULL;

  if ((stream->type == HLSDEMUX2_STREAM_AUDIO) &&
    g_strstr_len(demuxer_name, strlen(demuxer_name), "id3")) {
    /* create parser element */

    GST_INFO_OBJECT (stream->pad, "demuxer is id3demuxer, so create audio parser...");

    element_name = g_strdup_printf("%s-%s", stream_name, "parse");
    bin->parser =  gst_element_factory_make ("aacparse", element_name);
    if (!bin->parser) {
      GST_ERROR_OBJECT (demux, "failed to create parser element - %s", element_name);
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("failed to create element - %s", element_name), (NULL));
      goto error;
    }
    g_free (element_name);
    element_name = NULL;
    gst_bin_add (GST_BIN (bin->sinkbin), bin->parser);
  }

  /* create sink element */
  element_name = g_strdup_printf("%s-%s", stream_name, "sink");
  bin->sink =  gst_element_factory_make ("appsink", element_name);
  if (!bin->sink) {
    GST_ERROR_OBJECT (demux, "failed to create sink element - %s", element_name);
    GST_ELEMENT_ERROR (demux, CORE, FAILED, ("failed to create element - %s", element_name), (NULL));
    goto error;
  }
  g_free (element_name);
  element_name = NULL;

  g_object_set (G_OBJECT (bin->sink), "sync", FALSE, "emit-signals", TRUE, NULL);
  g_signal_connect (bin->sink, "new-buffer",  G_CALLBACK (gst_hlsdemux2_downloader_new_buffer), stream);
  g_signal_connect (bin->sink, "eos",  G_CALLBACK (gst_hlsdemux2_downloader_eos), stream);

  gst_bin_add_many (GST_BIN (bin->sinkbin), bin->queue, bin->sink, NULL);

  gst_bin_add (GST_BIN (demux->fdownloader->pipe), GST_ELEMENT(bin->sinkbin));

  /* set queue element to PLAYING state */
  gst_element_set_state (GST_ELEMENT(bin->sinkbin), GST_STATE_READY);

  sinkpad = gst_element_get_pad(bin->sink, "sink");
  if (!sinkpad) {
    GST_ERROR_OBJECT (demux, "failed to get sinkpad from element  - %s", element_name);
    GST_ELEMENT_ERROR (demux, CORE, PAD, ("Count not get sink pad"), (NULL));
    goto error;
  }

  /* adding probe to get new segment events */
  sink_probe = gst_pad_add_event_probe (sinkpad,
      G_CALLBACK (gst_hlsdemux2_sink_event_handler), stream);

  if (!stream_exist) {
    /* add stream to stream_type list */
    demux->streams[stream_type] = stream;
    demux->active_stream_cnt++;
    GST_INFO_OBJECT (demux, "number of active streams = %d", demux->active_stream_cnt);
  }

  linkpad = gst_element_get_pad(bin->queue, "sink");
  if (!linkpad) {
    GST_ERROR_OBJECT (demux, "failed to get sinkpad from element  - %s", gst_element_get_name (bin->queue));
    GST_ELEMENT_ERROR (demux, CORE, PAD, ("could not get sink pad"), (NULL));
    goto error;
  }

  gst_pad_set_active(linkpad,TRUE);

  gst_element_add_pad (GST_ELEMENT(bin->sinkbin), gst_ghost_pad_new ("sink", linkpad));

  GST_INFO_OBJECT (stream->pad, "successfully created stream...");

  demux->fdownloader->sinkbins = g_list_append (demux->fdownloader->sinkbins, bin);

  g_cond_signal (demux->post_msg_start);

  g_free (stream_name);

  return bin;

error:

  if (bin) {
    g_free (bin);
    bin = NULL;
  }

  if (stream) {
    g_free (stream);
    stream = NULL;
  }

  if (element_name)
    g_free (element_name);

  if (stream_name)
    g_free (stream_name);

  return NULL;
}

static gboolean
gst_hlsdemux2_private_image_to_vid(gpointer user_data,GstBuffer *image_buffer)
{
  GstHLSDemux2PvtStream *pvt_stream = (GstHLSDemux2PvtStream *) user_data;
  GstHLSDemux2 *demux = (GstHLSDemux2 *) pvt_stream->parent;
  GstPad *pad, *srcpad;
  GstElement *appsrc, *appsink, *dec, *videorate, *capsfilter, *enc;
  GstCaps *scale_caps, *image_caps;
  GstBus * bus;
  GstFlowReturn fret = GST_FLOW_OK;

  gchar *image_header = g_strndup((gchar *) image_buffer->data, 8);
  if(!((image_header[0] == 0xFF) && (image_header[1] == 0xD8))) {
    GST_WARNING_OBJECT(demux, "Not a valid JPEG image header : %d %d",image_header[0],image_header[1]);
    goto error;
  }

  GST_DEBUG_OBJECT(demux, "IMAGE DATA [%d,%d]",pvt_stream->image_buffer->data[0],pvt_stream->image_buffer->data[1]);

  GST_INFO_OBJECT (demux, "Creating jpeg->h264 conversion pipeline");

  pvt_stream->convert_pipe = gst_pipeline_new ("jpegdec-pipe");
  if (!pvt_stream->convert_pipe) {
    GST_ERROR_OBJECT (demux, "failed to create pipeline");
    goto error;
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (pvt_stream->convert_pipe));
  gst_bus_add_watch (bus, (GstBusFunc)gst_hlsdemux2_imagebuf_pipe_bus_cb, user_data);
  gst_object_unref (bus);

  appsrc = gst_element_factory_make ("appsrc", "imgbuf-src");
  if (!appsrc) {
    GST_ERROR_OBJECT (demux, "failed to create appSrc element");
    goto error;
  }

  appsink = gst_element_factory_make ("appsink", "imgbuf-sink");
  if (!appsink) {
    GST_ERROR_OBJECT (demux, "failed to create appSink element");
    goto error;
  }

  dec = gst_element_factory_make ("jpegdec", "jpeg-decoder");
  if (!dec) {
    GST_ERROR_OBJECT (demux, "failed to create jpeg-decoder element");
    goto error;
  }

  videorate = gst_element_factory_make ("videorate", "video-rate");
  if (!videorate) {
    GST_ERROR_OBJECT (demux, "failed to create ffmpeg-converter element");
    goto error;
  }

  capsfilter = gst_element_factory_make ("capsfilter", "caps-filter");
  if (!capsfilter) {
    GST_ERROR_OBJECT (demux, "failed to create caps-filter element");
    goto error;
  }

  scale_caps = gst_caps_new_simple ("video/x-raw-yuv",
      "width" , G_TYPE_INT, 480,
      "height", G_TYPE_INT, 270,
      "framerate",GST_TYPE_FRACTION,30,1,
      "format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
      NULL);
  g_object_set (capsfilter, "caps", scale_caps, NULL);

  enc = gst_element_factory_make ("savsenc_h264", "h264-encoder");
  if (!enc) {
    GST_ERROR_OBJECT (demux, "failed to create h264-encoder element");
    goto error;
  }

  image_caps = gst_caps_new_simple ("image/jpeg",
      "framerate", GST_TYPE_FRACTION, 30, 1,
      "width", G_TYPE_INT, 480,
      "height", G_TYPE_INT, 270,
      NULL);
  gst_buffer_set_caps (image_buffer, image_caps);

  gst_bin_add_many (GST_BIN (pvt_stream->convert_pipe), appsrc, dec, videorate, capsfilter, enc, appsink, NULL);
  if (!gst_element_link_many ( appsrc, dec, videorate, capsfilter, enc, appsink, NULL)) {
    GST_ERROR_OBJECT (demux, "failed to link elements...");
    goto error;
  }

  pad = gst_element_get_static_pad(appsink,"sink");
  gst_pad_add_buffer_probe (pad,G_CALLBACK (gst_hlsdemux2_set_video_buffer), user_data);
  gst_pad_add_event_probe  (pad,G_CALLBACK (gst_hlsdemux2_done_video_buffer), user_data);
  gst_element_set_state (pvt_stream->convert_pipe, GST_STATE_PLAYING);

  fret = gst_app_src_push_buffer ((GstAppSrc *)appsrc, pvt_stream->image_buffer);
  if(fret != GST_FLOW_OK){
    GST_ERROR_OBJECT (demux, "Push Failed; Error: %s[%d]", gst_flow_get_name(fret),fret);
    goto error;
  }

  fret = gst_app_src_end_of_stream ((GstAppSrc *)appsrc);
  if(fret != GST_FLOW_OK){
    GST_ERROR_OBJECT (demux, "Push EOS Failed; Error: %s[%d]", gst_flow_get_name(fret),fret);
    goto error;
  }
  return TRUE;

  error:
    GST_ERROR_OBJECT (demux, "ERROR ERROR ERROR: tut tut tut ");
    return FALSE;
}

static gboolean
gst_hlsdemux2_push_dummy_data (GstHLSDemux2Stream *stream)
{
  GstHLSDemux2 *demux = stream->parent;
  struct stat stat_results;
  GstBuffer *buf = NULL;
  gint iread = 0;
  gint64 percent = 0;
  GstCaps *caps = NULL;
  GstBuffer *pushbuf = NULL;
  GstClockTime stop = GST_CLOCK_TIME_NONE;
  GstClockTime next_ts = GST_CLOCK_TIME_NONE;
  gboolean first_frame = TRUE;
  gint n = 0;
  gboolean convert_success = TRUE;
  GstHLSDemux2Stream *cur_stream = NULL;
  int dummy_fp = -1;
  GstClockTime cdisc = 0;

  GST_INFO_OBJECT (demux, "START dummy video data thread");

  /* Read dummy frame */
  if(demux->has_image_buffer) {
    GstHLSDemux2PvtStream *pvt_stream = demux->private_stream;

    g_mutex_lock (pvt_stream->img_load_lock);
    if(pvt_stream->got_img_buffer == FALSE)
      g_cond_wait(pvt_stream->img_load_cond, pvt_stream->img_load_lock);
    g_mutex_unlock (pvt_stream->img_load_lock);

    GST_LOG_OBJECT (demux, "prev_image_buffer = %p and image_buffer = %p",
      demux->prev_image_buffer, pvt_stream->image_buffer);

    /* check whether it is same as previous image */
    if (demux->prev_video_buffer && demux->prev_image_buffer &&
      (GST_BUFFER_SIZE(demux->prev_image_buffer) == GST_BUFFER_SIZE(pvt_stream->image_buffer))) {
      if (!memcmp (GST_BUFFER_DATA(demux->prev_image_buffer), GST_BUFFER_DATA(pvt_stream->image_buffer),
          GST_BUFFER_SIZE(pvt_stream->image_buffer))) {
        GST_INFO_OBJECT (demux, "current & previous embedded images are same..no need to prepare video buffer");
        gst_buffer_unref(pvt_stream->image_buffer);
        pvt_stream->image_buffer = NULL;
        buf = gst_buffer_copy(demux->prev_video_buffer);
        goto start_push;
      }
    }

    if (demux->prev_image_buffer) {
      gst_buffer_unref (demux->prev_image_buffer);
    }
    demux->prev_image_buffer = gst_buffer_copy(pvt_stream->image_buffer);

    g_mutex_lock (pvt_stream->convert_lock);
    convert_success = gst_hlsdemux2_private_image_to_vid(pvt_stream, pvt_stream->image_buffer);
    if(convert_success)
      g_cond_wait(pvt_stream->convert_cond, pvt_stream->convert_lock);
    g_mutex_unlock (pvt_stream->convert_lock);

    if(pvt_stream->convert_pipe){
      pvt_stream->convert_pipe = NULL;
      gst_object_unref(pvt_stream->convert_pipe);
    }

    if (demux->prev_video_buffer) {
      gst_buffer_unref (demux->prev_video_buffer);
      demux->prev_video_buffer = NULL;
    }

    if(convert_success){
      buf = pvt_stream->video_buffer;
      pvt_stream->video_buffer = NULL;
      demux->prev_video_buffer = gst_buffer_copy(buf);
      GST_DEBUG_OBJECT (demux, "Set Image from buffer - success  [%d,%d]", buf->data[0], buf->data[1]);
      GST_INFO_OBJECT (demux, "Going to show embedded image");
    }
  }

  //load black frame if error occured while creating video buffer.
  if(demux->has_image_buffer == FALSE || convert_success == FALSE)
  {
    GST_INFO_OBJECT (demux, "Going to show fixed image");
    dummy_fp = open (PREDEFINED_VIDEO_FRAME_LOCATION, O_RDONLY);
    if (dummy_fp < 0) {
      GST_ERROR_OBJECT (stream->pad, "failed to open dummy data file : %s...", PREDEFINED_VIDEO_FRAME_LOCATION);
      GST_ELEMENT_ERROR (demux, RESOURCE, OPEN_READ, ("Failed open file '%s' for reading. reason : %s",PREDEFINED_VIDEO_FRAME_LOCATION ,g_strerror(errno)), (NULL));
      goto quit;
    }

    GST_LOG_OBJECT (stream->pad, "opened dummy video file %s succefully...", PREDEFINED_VIDEO_FRAME_LOCATION);

    if (fstat (dummy_fp, &stat_results) < 0) {
      GST_ERROR_OBJECT (stream->pad, "failed to get stats of a file...");
      GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("Failed get stats. reason : %s", g_strerror(errno)), (NULL));
      close (dummy_fp);
      goto quit;
    }

    GST_LOG_OBJECT (stream->pad, "size of the dummy file = %d\n", stat_results.st_size);

    buf = gst_buffer_new_and_alloc (stat_results.st_size);
    if (!buf) {
      GST_ERROR_OBJECT (stream->pad, "failed to allocate memory...");
      GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
      close (dummy_fp);
      goto quit;
    }

    iread = read (dummy_fp, GST_BUFFER_DATA (buf), stat_results.st_size);

    close (dummy_fp);
  }

start_push:

  for (n = 0; n < HLSDEMUX2_STREAM_NUM; n++) {
    cur_stream = demux->streams[n];

    if (cur_stream && (stream != cur_stream)) {
      cdisc = cur_stream->cdisc;
    }
  }

  stream->nts = stop = stream->fts + demux->cfrag_dur;
  stream->total_disc += cdisc;
  stream->cdisc = cdisc;
  next_ts = stream->fts;

  GST_INFO_OBJECT (stream->pad, "Dummy video start_ts = %"GST_TIME_FORMAT" and stop_ts = %"GST_TIME_FORMAT,
    GST_TIME_ARGS(stream->fts), GST_TIME_ARGS(stop));

  /* set caps on buffer */
  caps = gst_caps_new_simple ("video/x-h264",
                  "stream-format", G_TYPE_STRING, "byte-stream",
                  "alignment", G_TYPE_STRING, "nal",
                  NULL);

  gst_buffer_set_caps (buf, caps);

  if (stream->need_newsegment) {
    GST_INFO_OBJECT (stream->pad, "need to send new segment event based on audio");
    g_queue_push_tail (stream->queue, (demux->streams[HLSDEMUX2_STREAM_AUDIO])->newsegment);
    stream->need_newsegment = FALSE;
  }

  while (TRUE) {

    if (demux->cancelled || demux->flushing) {
      GST_WARNING_OBJECT (stream->pad, "Stopping dummy push task...");
      goto quit;
    }

    g_mutex_lock (stream->queue_lock);

    pushbuf = gst_buffer_copy (buf);
    gst_buffer_set_caps (pushbuf, caps);
    GST_BUFFER_TIMESTAMP (pushbuf) = stream->lts = next_ts;
    GST_BUFFER_DURATION (pushbuf) = GST_CLOCK_TIME_NONE;

    if (first_frame) {
      GST_BUFFER_FLAG_SET (pushbuf, GST_BUFFER_FLAG_DISCONT);
      first_frame = FALSE;
    }

    GST_LOG_OBJECT (stream->pad, "Pushing dummy buffer : ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_TIMESTAMP (pushbuf)));

    /* calculate cached duration */
    gst_hlsdemux2_calculate_pushed_duration (demux, stream, pushbuf);

    if (stream->cached_duration >= 0) {

      percent = (stream->cached_duration * 100) / demux->total_cache_duration;
      GST_LOG_OBJECT (stream->pad, "percent done = %"G_GINT64_FORMAT, percent);

      if (percent > 100) {
        guint64 overall_percent = 0;

        g_mutex_lock (demux->buffering_lock);
        overall_percent = demux->percent;
        g_mutex_unlock (demux->buffering_lock);

        if (overall_percent < 100) { /* otherwise, may cause blocking while buffering*/
          GST_DEBUG_OBJECT (stream->pad, "@@@@@@@@@@@ queue should not go to wait now @@@@@@@@");
        } else {
          /* update buffering & wait if space is not available */
          GST_DEBUG_OBJECT (stream->pad, "Reached more than 100 percent, queue full & wait till free");
          g_cond_wait(stream->queue_full, stream->queue_lock);
          GST_DEBUG_OBJECT (stream->pad,"Received signal to add more data...");
        }
      }
    }

    g_queue_push_tail (stream->queue, pushbuf);

    g_cond_signal (stream->queue_empty);

    g_mutex_unlock (stream->queue_lock);

    /* calculate next timestamp */
    next_ts += HLS_DEFAULT_FRAME_DURATION;

    if (next_ts > stop) {
      GST_DEBUG_OBJECT (stream->pad, "Reached Endof the fragment ....\n\n");
      break;
    }
  }

  if (demux->end_of_playlist) {
    /* end of playlist, push EOS to queue */
    GST_INFO_OBJECT (stream->pad, "pushing EOS event to stream's queue");
    g_queue_push_tail (stream->queue, gst_event_new_eos ());
  }

#ifdef LATEST_AV_SYNC
  if (demux->is_live) {
    stream->total_stream_time += (stream->lts - stream->fts);
  } else {
    stream->total_stream_time += (stream->lts - stream->fts + HLS_DEFAULT_FRAME_DURATION);
    stream->total_stream_time = stream->nts;
  }
#else
  stream->total_stream_time += (stream->lts - stream->fts);
#endif

  GST_DEBUG_OBJECT (stream->pad, "---------------- TS VALUES ----------------");
  GST_DEBUG_OBJECT (stream->pad, "valid start ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->fts));
  GST_DEBUG_OBJECT (stream->pad, "valid end ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->lts));
  GST_DEBUG_OBJECT (stream->pad, "fragment duration received = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->lts - stream->fts));
  GST_DEBUG_OBJECT (stream->pad, "total stream time = %"GST_TIME_FORMAT,GST_TIME_ARGS(stream->total_stream_time));
  GST_DEBUG_OBJECT (stream->pad, "total disc time = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->total_disc));
  GST_DEBUG_OBJECT (stream->pad, "next expected start ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->nts));
  GST_DEBUG_OBJECT (stream->pad, "---------------- DUMP VALUES END----------------");

  stream->apply_disc = FALSE;
  stream->fts = GST_CLOCK_TIME_NONE;
  stream->valid_fts_rcvd = FALSE;
  stream->prev_nts = stream->nts;

quit:
  {
    GST_INFO_OBJECT (stream->pad, "Stopping dummy data thread...");
    gst_buffer_unref (buf);
    stream->dummy_data_thread = NULL;
    return TRUE;
  }

}

static gboolean
gst_hlsdemux2_sink_event_handler (GstPad * pad, GstEvent * event, gpointer data)
{
  GstHLSDemux2Stream *stream = (GstHLSDemux2Stream *)data;
  GstHLSDemux2 *demux = stream->parent;

  if ((GST_EVENT_TYPE (event) == GST_EVENT_NEWSEGMENT) && stream->need_newsegment) {
    GstEvent *newsegment = NULL;
    GstFormat format;
    gboolean update;
    gdouble rate, arate;
    gint64 start, stop, pos;

    newsegment = gst_event_new_new_segment (FALSE, 1.0, GST_FORMAT_TIME, demux->ns_start, -1, demux->ns_start);

    GST_INFO_OBJECT (stream->pad, "Received NEWSEGMENT event...");

    g_mutex_lock (stream->queue_lock);

    gst_event_parse_new_segment_full(event, &update, &rate, &arate, &format, &start, &stop, &pos);

    if (!GST_CLOCK_TIME_IS_VALID(stream->base_ts)) {
      GST_INFO_OBJECT (stream->pad, "storing stream's base timestamp = %"GST_TIME_FORMAT, GST_TIME_ARGS(start));
      stream->base_ts = start;
    }

    g_queue_push_tail (stream->queue, newsegment);

    GST_INFO_OBJECT (stream->pad, "Pushed new segment event with start = %"GST_TIME_FORMAT" to queue..",
      GST_TIME_ARGS(demux->ns_start));

    g_mutex_unlock (stream->queue_lock);

    stream->need_newsegment = FALSE;
    stream->prev_nts = demux->ns_start;

    if (stream->newsegment)
      gst_event_unref (stream->newsegment);

    stream->newsegment = gst_event_copy (newsegment);
  }
  return TRUE;
}

static gboolean
hlsdemux2_HTTP_repeat_request (GstHLSDemux2 *demux, gchar *element_name)
{
  if (g_strrstr(element_name, "fragurisrc")) {
    guint idx = 0;
    GstBuffer *buf = NULL;

    /* clear stream queues if we already downloaded some data */
    for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
      GstHLSDemux2Stream *stream = demux->streams[idx];
      if (stream ) {
        while (!g_queue_is_empty (stream->downloader_queue)) {
          buf = g_queue_pop_head (stream->downloader_queue);
          gst_buffer_unref (buf);
        }
        GST_LOG_OBJECT (stream->pad, "cleared stream download queue...");
        g_queue_clear (stream->downloader_queue);
      }
    }

    /* request the same fragment again */
    if (demux->fragCookie)
      g_strfreev (demux->fragCookie);

    g_object_get (demux->fdownloader->urisrc, "cookies", &demux->fragCookie, NULL);
    GST_DEBUG_OBJECT (demux, "Got cookies after FRAGMENT download : %s", demux->fragCookie ? *(demux->fragCookie) : NULL);

    GST_INFO_OBJECT (demux, "========>>>>>Going to download fragment AGAIN: %s", demux->frag_uri);

    gst_hlsdemux2_destroy_fragment_download (demux);

    if (!gst_hlsdemux2_create_fragment_download (demux, demux->frag_uri)) {
      GST_ERROR_OBJECT (demux, "failed to create download pipeline");
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Failed to create pipeline"), (NULL));
      return FALSE;
    }

    /* download rate calculation : note down start time*/
    demux->fdownloader->download_start_ts = gst_util_get_timestamp();

    gst_element_set_state (demux->fdownloader->pipe, GST_STATE_PLAYING);

    return TRUE;
  } else if (g_strrstr(element_name, "playlisturisrc")) {

    if (demux->playlistCookie)
      g_strfreev (demux->playlistCookie);

    g_object_get (demux->pldownloader->urisrc, "cookies", &demux->playlistCookie, NULL);

    GST_DEBUG_OBJECT (demux, "Got cookies after PLAYLIST download : %s", demux->playlistCookie ? *(demux->playlistCookie) : NULL);

    gst_hlsdemux2_destroy_playlist_download (demux);

    GST_INFO_OBJECT (demux, "========>>>>>Going to download playlist AGAIN: %s", demux->playlist_uri);
    if (!gst_hlsdemux2_create_playlist_download (demux, demux->playlist_uri)) {
      GST_ERROR_OBJECT (demux, "failed to create download pipeline");
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Failed to create pipeline"), (NULL));
      return FALSE;
    }

    gst_element_set_state (demux->pldownloader->pipe, GST_STATE_PLAYING);
    return TRUE;
  } else if (g_strrstr(element_name, "keyurisrc")) {

    if (demux->keyCookie)
      g_strfreev (demux->keyCookie);

    g_object_get (demux->kdownloader->urisrc, "cookies", &demux->keyCookie, NULL);

    GST_DEBUG_OBJECT (demux, "Got cookies after KEY download : %s", demux->keyCookie ? *(demux->keyCookie) : NULL);

    GST_INFO_OBJECT (demux, "========>>>>>Going to download key AGAIN: %s", demux->key_uri);

    gst_hlsdemux2_destroy_key_download (demux);

    if (!gst_hlsdemux2_create_key_download (demux, demux->key_uri)) {
      GST_ERROR_OBJECT (demux, "failed to create download pipeline");
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Failed to create pipeline"), (NULL));
      return FALSE;
    }

    gst_element_set_state (demux->kdownloader->pipe, GST_STATE_PLAYING);
    return TRUE;
  }

  return FALSE;
}


static gboolean
hlsdemux2_HTTP_time_out (GstHLSDemux2 *demux, gchar *element_name)
{
  if (g_strrstr(element_name, "fragurisrc")) {
    /* as it is because of timeout, there is no point in requesting the same fragment again, so request next one */
    GST_INFO_OBJECT (demux, "signalling fragment downloader to get next fragment...");
    demux->fdownloader->get_next_frag = TRUE;
    g_cond_signal (demux->fdownloader->cond);
    return TRUE;
  } else if (g_strrstr(element_name, "playlisturisrc")) {

    if (demux->playlistCookie)
      g_strfreev (demux->playlistCookie);

    g_object_get (demux->pldownloader->urisrc, "cookies", &demux->playlistCookie, NULL);

    GST_DEBUG_OBJECT (demux, "Got cookies after PLAYLIST download : %s", demux->playlistCookie ? *(demux->playlistCookie) : NULL);

    gst_hlsdemux2_destroy_playlist_download (demux);

    GST_INFO_OBJECT (demux, "========>>>>>Going to download playlist AGAIN: %s", demux->playlist_uri);
    if (!gst_hlsdemux2_create_playlist_download (demux, demux->playlist_uri)) {
      GST_ERROR_OBJECT (demux, "failed to create download pipeline");
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Failed to create pipeline"), (NULL));
      return FALSE;
    }

    gst_element_set_state (demux->pldownloader->pipe, GST_STATE_PLAYING);
    return TRUE;
  } else if (g_strrstr(element_name, "keyurisrc")) {

    if (demux->keyCookie)
      g_strfreev (demux->keyCookie);

    g_object_get (demux->kdownloader->urisrc, "cookies", &demux->keyCookie, NULL);

    GST_DEBUG_OBJECT (demux, "Got cookies after KEY download : %s", demux->keyCookie ? *(demux->keyCookie) : NULL);

    GST_INFO_OBJECT (demux, "========>>>>>Going to download key AGAIN: %s", demux->key_uri);

    gst_hlsdemux2_destroy_key_download (demux);

    if (!gst_hlsdemux2_create_key_download (demux, demux->key_uri)) {
      GST_ERROR_OBJECT (demux, "failed to create download pipeline");
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Failed to create pipeline"), (NULL));
      return FALSE;
    }

    gst_element_set_state (demux->kdownloader->pipe, GST_STATE_PLAYING);
    return TRUE;
  }

  return FALSE;
}

static gboolean
hlsdemux2_HTTP_not_found(GstHLSDemux2 *demux, gchar *element_name)
{
  if (g_strrstr(element_name, "fragurisrc")) {
    if (demux->is_live) {
      /* request next fragment url */
      GST_INFO_OBJECT (demux, "signalling fragment downloader to get next fragment...");
      demux->fdownloader->get_next_frag = TRUE;
      g_cond_signal (demux->fdownloader->cond);
      return TRUE;
    } else {

#if 0 // In future enable
      GList *next_variant = NULL;

      if (demux->pldownloader->recovery_mode == HLSDEMUX2_NO_RECOVERY)
        demux->pldownloader->recovery_mode = HLSDEMUX2_DOWNWARD_RECOVERY;

      if (demux->pldownloader->recovery_mode == HLSDEMUX2_DOWNWARD_RECOVERY) {
        /* get next available lower variant */
        next_variant = gst_m3u8_client_get_next_lower_bw_playlist (demux->client);
        if (!next_variant) {
          GST_WARNING_OBJECT (demux, "no next lower variants.. Go Upward");
          demux->pldownloader->recovery_mode = HLSDEMUX2_UPWARD_RECOVERY;
        }
      }

      if (demux->pldownloader->recovery_mode == HLSDEMUX2_UPWARD_RECOVERY) {
        /* get next available lower variant */
        next_variant = gst_m3u8_client_get_next_higher_bw_playlist (demux->client);
        if (!next_variant) {
          GST_ERROR_OBJECT (demux, "no next higher variants.. need to exit");
          demux->pldownloader->recovery_mode = HLSDEMUX2_NO_RECOVERY;
          return FALSE;
        }
      }

      /* change variant */
      GST_M3U8_CLIENT_LOCK (demux->client);
      demux->client->main->current_variant = next_variant;
      GST_M3U8_CLIENT_UNLOCK (demux->client);

      gst_m3u8_client_set_current (demux->client, next_variant->data);

      demux->fdownloader->get_next_frag = TRUE;
      g_cond_signal (demux->fdownloader->cond);

      return TRUE;
#else
      return FALSE;
#endif
    }
  } else if (g_strrstr(element_name, "playlisturisrc")) {
    GList *next_variant = NULL;

    if (demux->pldownloader->recovery_mode == HLSDEMUX2_NO_RECOVERY)
      demux->pldownloader->recovery_mode = HLSDEMUX2_DOWNWARD_RECOVERY;

    if (demux->pldownloader->recovery_mode == HLSDEMUX2_DOWNWARD_RECOVERY) {
      /* get next available lower variant */
      next_variant = gst_m3u8_client_get_next_lower_bw_playlist (demux->client);
      if (!next_variant) {
        GST_WARNING_OBJECT (demux, "no next lower variants.. Go Upward");
        demux->pldownloader->recovery_mode = HLSDEMUX2_UPWARD_RECOVERY;
      }
    }

    if (demux->pldownloader->recovery_mode == HLSDEMUX2_UPWARD_RECOVERY) {
      /* get next available lower variant */
      next_variant = gst_m3u8_client_get_next_higher_bw_playlist (demux->client);
      if (!next_variant) {
        GST_ERROR_OBJECT (demux, "no next higher variants.. need to exit");
        demux->pldownloader->recovery_mode = HLSDEMUX2_NO_RECOVERY;
        return FALSE;
      }
    }

    /* change variant */
    GST_M3U8_CLIENT_LOCK (demux->client);
    demux->client->main->current_variant = next_variant;
    GST_M3U8_CLIENT_UNLOCK (demux->client);

    gst_m3u8_client_set_current (demux->client, next_variant->data);

    /* get new playlist uri */
    demux->playlist_uri = g_strdup (gst_m3u8_client_get_current_uri (demux->client));

    if (demux->playlistCookie)
      g_strfreev (demux->playlistCookie);

    g_object_get (demux->pldownloader->urisrc, "cookies", &demux->playlistCookie, NULL);

    GST_DEBUG_OBJECT (demux, "Got cookies after PLAYLIST download : %s", demux->playlistCookie ? *(demux->playlistCookie) : NULL);

    gst_hlsdemux2_destroy_playlist_download (demux);

    GST_INFO_OBJECT (demux, "========>>>>>Going to download recovery playlist : %s", demux->playlist_uri);
    if (!gst_hlsdemux2_create_playlist_download (demux, demux->playlist_uri)) {
      GST_ERROR_OBJECT (demux, "failed to create download pipeline");
      GST_ELEMENT_ERROR (demux, CORE, FAILED, ("Failed to create pipeline"), (NULL));
      return FALSE;
    }

    gst_element_set_state (demux->pldownloader->pipe, GST_STATE_PLAYING);

    return TRUE;

  } else if (g_strrstr(element_name, "keyurisrc")) {
    /* treating it as non-recoverable error */
    return FALSE;
  }

  return FALSE;
}

static gboolean
gst_hlsdemux2_handle_HTTP_error (GstHLSDemux2 *demux, GError *error, gchar *element_name) {
  guint i = 0;
  guint n_http_errors = sizeof (http_errors) / sizeof (http_errors[0]);

  if(!error)
    return HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE;
  for (i = 0; i < n_http_errors; i++) {
    if (G_LIKELY (!strncmp(error->message, http_errors[i].error_phrase, strlen(error->message)))) {
      if (http_errors[i].handle_error) {
        return http_errors[i].handle_error (demux, element_name);
      }
      return http_errors[i].error_type;
    }
  }
  return HLSDEMUX2_HTTP_ERROR_NONRECOVERABLE;
}

static gboolean
gst_hlsdemux2_fragment_download_bus_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  GError *error = NULL;
  gchar* debug = NULL;
  gboolean bret = TRUE;
  GstMessage *err_msg = NULL;
  gchar *ele_name = gst_element_get_name (GST_MESSAGE_SRC (msg));

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
      GST_INFO_OBJECT (demux, "received EOS on download pipe from '%s'..", ele_name);
      demux->pldownloader->recovery_mode = HLSDEMUX2_NO_RECOVERY;
      g_cond_signal(demux->fdownloader->cond);
      break;
    }
    case GST_MESSAGE_ERROR: {
      GST_ERROR_OBJECT (demux, "Error from %s\n", ele_name);

      gst_message_parse_error( msg, &error, &debug );

      demux->fdownloader->error_rcvd = TRUE;

      if (error)
        GST_ERROR_OBJECT (demux, "GST_MESSAGE_ERROR: error->message = %s and error->code = %d", error->message);

      GST_ERROR_OBJECT (demux, "GST_MESSAGE_ERROR: debug = %s", debug);

      if (g_strrstr(ele_name, "fragurisrc") && !demux->cancelled && demux->soup_request_fail_cnt) {

        /* flush the error msgs pending.. as we are going to create new pipeline */
        gst_bus_set_flushing (bus, TRUE);

        bret = gst_hlsdemux2_handle_HTTP_error (demux, error, ele_name);
        if (!bret)
          goto post_error;
        else {
          demux->soup_request_fail_cnt--;
          GST_WARNING_OBJECT (demux, "HTTP error count remaining = %d", demux->soup_request_fail_cnt);
          goto exit;
        }
      }

post_error:

      GST_ERROR_OBJECT (demux, "posting error from fragment download callback");

      err_msg = gst_message_new_error (GST_OBJECT(demux), error, debug);
      if (!gst_element_post_message (GST_ELEMENT(demux), err_msg)) {
        GST_ERROR_OBJECT (demux, "failed to post error");
        bret = FALSE;
        goto exit;
      }
      gst_hlsdemux2_stop (demux);
      break;
    }
    case GST_MESSAGE_WARNING: {
      gst_message_parse_warning(msg, &error, &debug);
      GST_WARNING_OBJECT(demux, "warning : %s\n", error->message);
      GST_WARNING_OBJECT(demux, "debug : %s\n", debug);
      break;
    }
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *s = gst_message_get_structure (msg);

      if (gst_structure_has_name (s, "cookies")) {
        const GValue *value;
        gchar **cookies = NULL;
        gchar *cookie = NULL;
        value = gst_structure_get_value (s, "cookies");
        cookie = g_strdup_value_contents(value);

        cookies = &cookie;
        GST_ERROR_OBJECT (demux, "received cookies from soup : %s", *cookies);
      }
      break;
    }
    default : {
      break;
    }
  }

exit:
  if (debug)
    g_free( debug);

  if (error)
    g_error_free( error);

  if (!bret)
    gst_hlsdemux2_stop (demux);

  g_free (ele_name);

  return bret;
}

#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
static GstBusSyncReply
gst_hlsdemux2_playlist_download_bus_sync_cb (GstBus * bus, GstMessage *msg, gpointer data)
#else
static gboolean
gst_hlsdemux2_playlist_download_bus_cb(GstBus *bus, GstMessage *msg, gpointer data)
#endif
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
  GstBusSyncReply reply = GST_BUS_DROP;
#endif
  GError *error = NULL;
  gchar *debug = NULL;
  gboolean bret = TRUE;
  GstMessage *err_msg = NULL;
  gchar *ele_name = gst_element_get_name (GST_MESSAGE_SRC (msg));

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
      GST_DEBUG_OBJECT (demux, "received EOS on playlist download pipe..");
      demux->pldownloader->recovery_mode = HLSDEMUX2_NO_RECOVERY;
      g_mutex_lock (demux->pldownloader->lock);
      g_cond_broadcast (demux->pldownloader->cond);
      g_mutex_unlock (demux->pldownloader->lock);
      break;
    }
    case GST_MESSAGE_ERROR: {
      GST_ERROR_OBJECT (demux, "Error from %s element", ele_name);

      gst_message_parse_error( msg, &error, &debug );
      if (error)
        GST_ERROR_OBJECT (demux, "GST_MESSAGE_ERROR: error= %s", error->message);

      GST_ERROR_OBJECT (demux, "GST_MESSAGE_ERROR: debug = %s", debug);

      if (g_strrstr(ele_name, "playlisturisrc") && !demux->cancelled && demux->soup_request_fail_cnt) {

        /* flush the error msgs pending.. as we are going to create new pipeline */
        gst_bus_set_flushing (bus, TRUE);

        bret = gst_hlsdemux2_handle_HTTP_error (demux, error, ele_name);
        if (!bret)
          goto post_error;
        else {
          demux->soup_request_fail_cnt--;
          GST_WARNING_OBJECT (demux, "HTTP error count remaining = %d", demux->soup_request_fail_cnt);
          goto exit;
        }
      }

post_error:

      GST_ERROR_OBJECT (demux, "posting error from playlist callback...");

      err_msg = gst_message_new_error (GST_OBJECT(demux), error, debug);
      if (!gst_element_post_message (GST_ELEMENT(demux), err_msg)) {
        GST_ERROR_OBJECT (demux, "failed to post error");
        bret = FALSE;
        goto exit;
      }

#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
/*
      Issue : Play any Clip from MySpace or some other sites , for which UA profile is not registered.
      As a result , Forbidden Error will be posted from Soup and it was resulting on the Screen Freeze .
      Handling the Freeze issue due to http://slp-info.sec.samsung.net/gerrit/#/c/662956/changes.

      Due to introduction of gst_bus_set_sync_handler in place of gst_bus_add_watch , this callback gets called
      in  same thread context as the posting object ( not with the main thread context ).
      As a result, calling gst_hlsdemux2_stop() during this context resulted in Thread Freze.
      For solving this problem , we just need to post the error message to Soup and let the error gets handled in Soup.
      Once the Error is handled in Soup , Change state from Pause to Ready will be triggered from Soup resulting in proper Main Thread Context Stop Call.
      This will solve the problem of Thread Freeze.
*/
      GST_ERROR_OBJECT (demux, "not calling gst_hlsdemux2_stop during Playlist Download Error from same thread context !!!");
      bret = TRUE;
#else
      gst_hlsdemux2_stop (demux);
#endif
      break;
    }
    case GST_MESSAGE_WARNING: {
      gst_message_parse_warning(msg, &error, &debug);
      if (error)
        GST_WARNING_OBJECT(demux, "warning : %s\n", error->message);
      GST_WARNING_OBJECT(demux, "debug : %s\n", debug);
      break;
    }
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *s = gst_message_get_structure (msg);

      if (gst_structure_has_name (s, "cookies")) {
        const GValue *value;
        gchar **cookies = NULL;
        gchar *cookie = NULL;
        value = gst_structure_get_value (s, "cookies");
        cookie = g_strdup_value_contents(value);

        cookies = &cookie;
        GST_ERROR_OBJECT (demux, "received cookies from soup : %s", *cookies);
      }
      break;
    }
    default : {
      //GST_LOG_OBJECT(demux, "unhandled message : %s\n", gst_message_type_get_name (GST_MESSAGE_TYPE (msg)));
      break;
    }
  }

exit:
  if (debug)
    g_free( debug);

  if (error)
    g_error_free( error);

  if (!bret)
    gst_hlsdemux2_stop (demux);

  g_free (ele_name);

#ifdef ENABLE_HLS_SYNC_MODE_HANDLER
  return reply;
#else
  return bret;
#endif
}

static gboolean
gst_hlsdemux2_key_download_bus_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  GError *error = NULL;
  gchar* debug = NULL;
  gboolean bret = TRUE;
  GstMessage *err_msg = NULL;
  gchar *ele_name = gst_element_get_name (GST_MESSAGE_SRC (msg));

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
      GST_DEBUG_OBJECT (demux, "received EOS on key download pipe..");
      demux->pldownloader->recovery_mode = HLSDEMUX2_NO_RECOVERY;
      g_mutex_lock (demux->kdownloader->lock);
      g_cond_signal (demux->kdownloader->cond);
      g_mutex_unlock (demux->kdownloader->lock);
      break;
    }
    case GST_MESSAGE_ERROR: {
      GST_INFO_OBJECT (demux, "Error from %s element...", ele_name);

      gst_message_parse_error(msg, &error, &debug);

      if (error)
        GST_ERROR_OBJECT (demux, "GST_MESSAGE_ERROR: error= %s", error->message);

      GST_ERROR_OBJECT (demux, "GST_MESSAGE_ERROR: debug = %s", debug);

      if (g_strrstr(ele_name, "keyurisrc") && !demux->cancelled && demux->soup_request_fail_cnt) {

        /* flush the error msgs pending.. as we are going to create new pipeline */
        gst_bus_set_flushing (bus, TRUE);

        bret = gst_hlsdemux2_handle_HTTP_error (demux, error, ele_name);
        if (!bret)
          goto post_error;
        else {
          demux->soup_request_fail_cnt--;
          GST_WARNING_OBJECT (demux, "HTTP error count remaining = %d", demux->soup_request_fail_cnt);
          goto exit;
        }
      }

post_error:
      GST_ERROR_OBJECT (demux, "Error posting from key downloader...");
      err_msg = gst_message_new_error (GST_OBJECT(demux), error, debug);

      if (!gst_element_post_message (GST_ELEMENT(demux), err_msg)) {
        GST_ERROR_OBJECT (demux, "failed to post error");
        gst_hlsdemux2_stop (demux);
        bret = FALSE;
        goto exit;
      }
      gst_hlsdemux2_stop (demux);
      break;
    }
    case GST_MESSAGE_WARNING: {
      gst_message_parse_warning(msg, &error, &debug);
      if (error)
        GST_WARNING_OBJECT(demux, "warning : %s\n", error->message);
      GST_WARNING_OBJECT(demux, "debug : %s\n", debug);
      break;
    }
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *s = gst_message_get_structure (msg);

      if (gst_structure_has_name (s, "cookies")) {
        const GValue *value;
        gchar **cookies = NULL;
        gchar *cookie = NULL;
        value = gst_structure_get_value (s, "cookies");
        cookie = g_strdup_value_contents(value);

        cookies = &cookie;
        GST_ERROR_OBJECT (demux, "received cookies from soup : %s", *cookies);
      }
     break;
    }
    default : {
      break;
    }
  }

exit:

  if (debug)
    g_free( debug);

  if (error)
    g_error_free( error);

  if (!bret)
    gst_hlsdemux2_stop (demux);

  g_free (ele_name);

  return bret;
}

static void
gst_hlsdemux2_calculate_pushed_duration (GstHLSDemux2 *demux, GstHLSDemux2Stream *stream,
    GstBuffer *inbuf)
{
  gint len = 0;
  int qidx = 0;
  gpointer data = NULL;

  // TODO: for all timestamps -1 case, we need to add max-bytes also
  len = g_queue_get_length (stream->queue);

  /* peek the head buffer having valid timestamp to calculate cached duration */
  while (qidx < len) {
    data = g_queue_peek_nth (stream->queue, qidx);

    if (GST_IS_BUFFER (data)) {
      if (GST_BUFFER_TIMESTAMP_IS_VALID(data)) {
        GST_LOG_OBJECT (stream->pad, "data found buffer with valid ts = %"GST_TIME_FORMAT " len = %d & qidx = %d",
          GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(data)), len, qidx);
        break;
      }
    }
    qidx++;
  }
  if(GST_IS_BUFFER (data)) {
    if (GST_BUFFER_TIMESTAMP (inbuf) >= GST_BUFFER_TIMESTAMP ((GstBuffer *)data)) {
      stream->cached_duration = GST_BUFFER_TIMESTAMP (inbuf) - GST_BUFFER_TIMESTAMP ((GstBuffer *)data);
      GST_LOG_OBJECT (stream->pad, "len = %d, cached duration = %"GST_TIME_FORMAT,
          g_queue_get_length (stream->queue), GST_TIME_ARGS(stream->cached_duration));
    } else {
      GST_WARNING_OBJECT (stream->pad, "Wrong order.. not calculating");
    }
  }
  return;
}

static gboolean
gst_hlsdemux2_alter_timestamps (GstHLSDemux2Stream *stream, GstBuffer *inbuf)
{
  GstHLSDemux2 *demux = stream->parent;
  GstClockTime cts = 0;

  /* set discontinuity only when there is real fragment discontinuity */
  if (GST_BUFFER_IS_DISCONT(inbuf)) {
    if (!stream->apply_disc) {
      GST_BUFFER_FLAG_UNSET (inbuf, GST_BUFFER_FLAG_DISCONT);
      GST_INFO_OBJECT (stream->pad, "unsetting discontinuity flag...");
    }
  }

  if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_TIMESTAMP (inbuf))) {
    if (GST_BUFFER_TIMESTAMP (inbuf) >= stream->base_ts) {
      cts = GST_BUFFER_TIMESTAMP (inbuf) - stream->base_ts;
    } else {
      /* this buffer should be dropped at sink */
      GST_WARNING_OBJECT (stream->pad, "input timestamp [%"GST_TIME_FORMAT"] is less than base_ts [%"GST_TIME_FORMAT"]",
        GST_TIME_ARGS(GST_BUFFER_TIMESTAMP (inbuf)), GST_TIME_ARGS(stream->base_ts));
      if (stream->type == HLSDEMUX2_STREAM_AUDIO) {
        return FALSE;
      } else {
        GST_BUFFER_TIMESTAMP (inbuf) = GST_CLOCK_TIME_NONE;
        return TRUE;
      }
    }
  } else {
    /* this buffer should be dropped at sink */
    GST_LOG_OBJECT (stream->pad, "invalid input timestamp (i.e. GST_CLOCK_TIME_NONE)");
#ifdef LATEST_AV_SYNC
    if (!demux->is_live && (stream->type == HLSDEMUX2_STREAM_AUDIO) && !stream->valid_fts_rcvd){
      GST_WARNING_OBJECT (stream->pad, "dropping invalid ts buffer, due to no valid first ts yet...");
      return FALSE;
    }
#endif
    return TRUE;
  }

  /* handle discontinuity in stream */
  if(GST_CLOCK_TIME_IS_VALID (stream->nts) && (GST_CLOCK_DIFF(stream->nts,cts) > (2 * GST_SECOND)) && stream->apply_disc) {
    GST_INFO_OBJECT (stream->pad, "cts = %"GST_TIME_FORMAT" and prev_next_ts = %"GST_TIME_FORMAT,
      GST_TIME_ARGS(cts), GST_TIME_ARGS(stream->nts));
    GST_INFO_OBJECT (stream->pad,"Received disc = %"GST_TIME_FORMAT, GST_TIME_ARGS(cts - stream->nts));
    stream->cdisc = cts - stream->nts;
    stream->total_disc += (cts - stream->nts);
    GST_INFO_OBJECT (stream->pad, "total disc = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->total_disc));
    stream->apply_disc = FALSE;
  }

  cts -= stream->cdisc;

  /* get the max last ts, required because of frame reordering */
  stream->lts = cts > stream->lts ? cts : stream->lts;

  GST_DEBUG_OBJECT (stream->pad, "modifying buffer timestamp : %"GST_TIME_FORMAT" -> %"GST_TIME_FORMAT,
    GST_TIME_ARGS(GST_BUFFER_TIMESTAMP (inbuf)), GST_TIME_ARGS(cts));

#ifndef LATEST_AV_SYNC
  /* store first valid timestamp of a fragment */
  if (stream->fts == GST_CLOCK_TIME_NONE) {
    stream->fts = cts;
    stream->nts = stream->fts + demux->cfrag_dur;
    GST_INFO_OBJECT (stream->pad, "storing first ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->fts));
    if (demux->is_live && stream->type == HLSDEMUX2_STREAM_AUDIO)
      demux->cur_audio_fts = cts;
  }
#endif

#ifdef LATEST_AV_SYNC
	/* 10/31/2013: dont want to disturb live case at this moment */
  if (!stream->valid_fts_rcvd) {
    if (stream->type == HLSDEMUX2_STREAM_AUDIO) {
      if (!demux->is_live && (GST_CLOCK_DIFF(cts, stream->prev_nts) > (10 * GST_MSECOND))) {
        GST_WARNING_OBJECT (stream->pad, "already seen audio timestamps[%"GST_TIME_FORMAT"]"
          " and previous_nts = %"GST_TIME_FORMAT"...drop this",
          GST_TIME_ARGS(cts), GST_TIME_ARGS(stream->prev_nts));
        return FALSE;
      } else {
        GST_INFO_OBJECT (stream->pad, "recevied valid audio first ts = %"GST_TIME_FORMAT,
          GST_TIME_ARGS (cts));
        demux->cur_audio_fts = cts;
      }
    }

    stream->valid_fts_rcvd = TRUE;
    stream->fts = cts;
    stream->nts = stream->fts + demux->cfrag_dur; // Approximate nts

    GST_INFO_OBJECT (stream->pad, "storing first ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->fts));
  } else if (!stream->frame_duration) {
    stream->frame_duration = cts - stream->fts;
    GST_DEBUG_OBJECT (stream->pad, "frame duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->frame_duration));
  }

#else

  /* 10/31/2013: dont want to disturb live case at this moment */
  if (!demux->is_live && stream->type == HLSDEMUX2_STREAM_AUDIO) {
    if (!stream->valid_fts_rcvd) {
      if (GST_CLOCK_DIFF(cts, stream->prev_nts) > 0) {
        GST_WARNING_OBJECT (stream->pad, "already seen audio timestamps[%"GST_TIME_FORMAT"]"
          " and previous_nts = %"GST_TIME_FORMAT"...drop this",
          GST_TIME_ARGS(cts), GST_TIME_ARGS(stream->prev_nts));
        return FALSE;
      } else {
        GST_INFO_OBJECT (stream->pad, "recevied valid audio first ts = %"GST_TIME_FORMAT,
          GST_TIME_ARGS (cts));
        demux->cur_audio_fts = cts;
        stream->valid_fts_rcvd = TRUE;
      }
    } else if (!stream->frame_duration) {
      stream->frame_duration = cts - demux->cur_audio_fts;
      GST_DEBUG_OBJECT (demux, "audio frame duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->frame_duration));
    }
  }
#endif

  GST_BUFFER_TIMESTAMP (inbuf) = cts;

  return TRUE;
}

static void
gst_hlsdemux2_downloader_new_buffer (GstElement *appsink, void *user_data)
{
  GstHLSDemux2Stream *stream = (GstHLSDemux2Stream *)user_data;
  GstHLSDemux2 *demux = stream->parent;
  GstBuffer *inbuf = NULL;
  gboolean bret = FALSE;

  if (demux->cancelled ) {
    return;
  }

  inbuf = gst_app_sink_pull_buffer ((GstAppSink *)appsink);
  if (!inbuf) {
    GST_WARNING_OBJECT (demux, "Input buffer not available...");
    return;
  }

  GST_LOG_OBJECT (stream->pad, "Received buffer with size = %d and ts = %"GST_TIME_FORMAT,
    GST_BUFFER_SIZE(inbuf), GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(inbuf)));

  if (demux->fdownloader->force_timestamps) {
    GST_BUFFER_TIMESTAMP(inbuf) += (stream->prev_nts + stream->base_ts);
    GST_DEBUG_OBJECT (stream->pad, "forced timestamp = %"GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(inbuf)));
  }

  /* alter the timestamps */
  bret = gst_hlsdemux2_alter_timestamps (stream, inbuf);
  if (!bret) {
    gst_buffer_unref (inbuf);
    return;
  }

  /* store buffer in queue */
  g_queue_push_tail (stream->downloader_queue, inbuf);

  return;
}

static gboolean
gst_hlsdemux2_imagebuf_pipe_bus_cb(GstBus *bus, GstMessage *msg, gpointer data)
{
  GstHLSDemux2PvtStream *stream = (GstHLSDemux2PvtStream *)data;
  GstHLSDemux2 *demux = (GstHLSDemux2 *)stream->parent;
  gboolean bret = TRUE;
  gchar *ele_name = gst_element_get_name (GST_MESSAGE_SRC (msg));

  GST_INFO_OBJECT(demux, "Message on BUS : %s", gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
      GST_INFO_OBJECT (demux, "received EOS on IMAGE pipe from '%s'.. Closing pipeline", ele_name);
      g_cond_signal (stream->convert_cond);
      gst_object_unref (stream->convert_pipe);
      break;
    }
    case GST_MESSAGE_ERROR: {
      GST_ERROR_OBJECT (demux, "Error Image Pipe from %s\n", ele_name);
      bret = FALSE;
      break;
    }
    default:{
      GST_DEBUG_OBJECT (demux, "Message on Image Pipe : %s from %s\n", gst_message_type_get_name(GST_MESSAGE_TYPE(msg)), ele_name);
      bret = FALSE;
      break;
    }
  }
  return bret;
}

static gboolean gst_hlsdemux2_done_video_buffer (GstPad * srcpad, GstEvent * event, gpointer user_data)
{
  GstHLSDemux2PvtStream *stream = (GstHLSDemux2PvtStream *)user_data;
  GstHLSDemux2 *demux = (GstHLSDemux2 *)stream->parent;
  GST_LOG_OBJECT(demux,"EVENT %s on pad : %s having caps : %"GST_PTR_FORMAT,GST_EVENT_TYPE_NAME (event), GST_PAD_NAME(srcpad), srcpad->caps);
  if(event->type == GST_EVENT_EOS){
    GST_DEBUG_OBJECT(demux,"EVENT : %s Size : %d Pushing to demux video buffer [%d,%d]",
    GST_EVENT_TYPE_NAME (event),GST_BUFFER_SIZE(stream->video_buffer),
        stream->video_buffer->data[0],stream->video_buffer->data[1]);
    g_cond_signal (stream->convert_cond);
  }
  return TRUE;
}

static gboolean gst_hlsdemux2_set_video_buffer (GstPad * srcpad, GstBuffer * buffer, gpointer user_data)
{
  GstHLSDemux2PvtStream *pvt_stream = (GstHLSDemux2PvtStream *)user_data;
  GstHLSDemux2 *demux = (GstHLSDemux2 *)pvt_stream->parent;
  GST_LOG_OBJECT(demux,"BUFFER size:%d on pad having caps : %"GST_PTR_FORMAT,GST_BUFFER_SIZE(buffer), srcpad->caps);
  GST_LOG_OBJECT(demux,"BUFFER DATA : %d %d",buffer->data[0],buffer->data[1]);
  pvt_stream->video_buffer = buffer;
  return TRUE;
}

static void
gst_hlsdemux2_downloader_eos (GstElement * appsink, void* user_data)
{
  GstHLSDemux2Stream *stream = (GstHLSDemux2Stream *)user_data;
  GstHLSDemux2 *demux = stream->parent;
  GstBuffer *buf = NULL;
  gint64 percent = 0;

  /* push all data to queue specific to this stream */

  if (demux->cancelled || demux->flushing) {
    GST_WARNING_OBJECT (demux, "returning due to cancelled / flushing");
    return;
  }

  if (g_queue_is_empty (stream->downloader_queue)) {
    GST_WARNING_OBJECT (demux, "EOS received without any buffer in downloader queue");
    return;
  }

  if (demux->fdownloader->force_timestamps) {
    demux->cur_audio_fts = stream->prev_nts;
  }

  if (demux->fdownloader->cur_stream_cnt < demux->active_stream_cnt) {
    gint idx = 0;

    GST_INFO_OBJECT (demux, "need to send dummy data.. check for the stream");

    /* signal queue full condition to come out */
    for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
      GstHLSDemux2Stream *cur_stream = demux->streams[idx];
      GError *error = NULL;

      if (cur_stream) {
        if (g_queue_is_empty (cur_stream->downloader_queue)) {
          GST_INFO_OBJECT (cur_stream->pad, "enable sending dummy data");

          /* take first ts of audio as cur_stream fts */
          cur_stream->fts = demux->cur_audio_fts;

          /* create thread to send dummy data */
          cur_stream->dummy_data_thread = g_thread_create ((GThreadFunc) gst_hlsdemux2_push_dummy_data,
            cur_stream, TRUE, &error);
        }
      }
    }
  }

  while (!g_queue_is_empty (stream->downloader_queue)) {

    if (demux->cancelled || demux->flushing) {
      GST_WARNING_OBJECT (stream->pad, "on cancel/flushing stopping pushing");
      break;
    }

    buf = (GstBuffer *) g_queue_pop_head (stream->downloader_queue);

    GST_DEBUG_OBJECT (stream->pad, "input buffer timestamp : %"GST_TIME_FORMAT,
      GST_TIME_ARGS(GST_BUFFER_TIMESTAMP (buf)));

    g_mutex_lock (stream->queue_lock);

/*
    if (!GST_BUFFER_TIMESTAMP_IS_VALID(buf)){
      g_queue_push_tail (stream->queue, buf);
      g_cond_signal (stream->queue_empty);
      g_mutex_unlock (stream->queue_lock);
      continue;
    }
*/
    GST_LOG_OBJECT (stream->pad, "buffer size = %d, ts = %"GST_TIME_FORMAT", dur = %"GST_TIME_FORMAT,
      GST_BUFFER_SIZE(buf), GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buf)), GST_TIME_ARGS(GST_BUFFER_DURATION(buf)));

    /* calculate cached duration */
    if (GST_BUFFER_TIMESTAMP_IS_VALID(buf))
      gst_hlsdemux2_calculate_pushed_duration (demux, stream, buf);

    if (stream->cached_duration < 0)
      stream->cached_duration = 0;

    percent = (stream->cached_duration * 100) / demux->total_cache_duration;
    GST_LOG_OBJECT (stream->pad, "percent done = %"G_GINT64_FORMAT, (gint)percent);

    if (percent > 100) {
      guint64 overall_percent = 0;

      g_mutex_lock (demux->buffering_lock);
      overall_percent = demux->percent;
      g_mutex_unlock (demux->buffering_lock);

      if (overall_percent < 100) { /* otherwise, may cause blocking while buffering*/
        if (percent > 400) {
          GST_ERROR_OBJECT (stream->pad,"having worrest buffering.. exiting");
          GST_ELEMENT_ERROR (demux, STREAM, FAILED, ("wrong buffering.. check implementation"), (NULL));
        }
        GST_INFO_OBJECT (stream->pad, "@@@@@@ queue should not go to wait now @@@@@@@@");
      } else {
        /* update buffering & wait if space is not available */
        GST_LOG_OBJECT (stream->pad, "Reached more than 100 percent, queue full & wait till free");
        g_cond_wait(stream->queue_full, stream->queue_lock);
        GST_LOG_OBJECT (stream->pad,"Received signal to add more data...");
      }
    }

    g_queue_push_tail (stream->queue, buf);

    g_cond_signal (stream->queue_empty);

    g_mutex_unlock (stream->queue_lock);
  }

  /* wait for dummy threads finish their processing */
  if (demux->fdownloader->cur_stream_cnt < demux->active_stream_cnt) {
    gint idx = 0;

    GST_INFO_OBJECT (demux, "need to close dummy threads");

    /* signal queue full condition to come out */
    for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
      GstHLSDemux2Stream *cur_stream = demux->streams[idx];

      if (cur_stream && cur_stream->dummy_data_thread) {
        GST_INFO_OBJECT (stream->pad, "waiting for dummy push thread to finish...");
        g_thread_join(cur_stream->dummy_data_thread);
        cur_stream->dummy_data_thread = NULL;
        GST_INFO_OBJECT (stream->pad, "COMPLETED DUMMY PUSH...");
      }
    }
  }

#ifdef LATEST_AV_SYNC
  if (demux->is_live) {
    stream->total_stream_time += (stream->lts - stream->fts);
  } else {
    stream->total_stream_time += (stream->lts - stream->fts + stream->frame_duration);
    stream->nts = stream->total_stream_time;
  }
#else
  stream->total_stream_time += (stream->lts - stream->fts);
#endif

  GST_DEBUG_OBJECT (stream->pad, "---------------- TS VALUES ----------------");
  GST_DEBUG_OBJECT (stream->pad, "valid start ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->fts));
  GST_DEBUG_OBJECT (stream->pad, "valid end ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->lts));
  GST_DEBUG_OBJECT (stream->pad, "fragment duration received = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->lts - stream->fts));
  GST_DEBUG_OBJECT (stream->pad, "total stream time = %"GST_TIME_FORMAT,GST_TIME_ARGS(stream->total_stream_time));
  GST_DEBUG_OBJECT (stream->pad, "total disc time = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->total_disc));
  GST_DEBUG_OBJECT (stream->pad, "next expected start ts = %"GST_TIME_FORMAT, GST_TIME_ARGS(stream->nts));
  GST_DEBUG_OBJECT (stream->pad, "---------------- DUMP VALUES END----------------");

  stream->apply_disc = FALSE;
  stream->fts = GST_CLOCK_TIME_NONE;
  stream->valid_fts_rcvd = FALSE;
  stream->prev_nts = stream->nts;
  demux->fdownloader->force_timestamps = FALSE;
  return;
}

static void
gst_hlsdemux2_push_eos (GstHLSDemux2 *demux)
{
  guint idx = 0;

  /* signal queue full condition to come out */
  for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
    GstHLSDemux2Stream *stream = demux->streams[idx];

    if (stream) {
      g_mutex_lock (stream->queue_lock);
      /* end of playlist, push EOS to queue */
      GST_INFO_OBJECT (stream->pad, "pushing EOS event to stream's queue");
      g_queue_push_tail (stream->queue, gst_event_new_eos ());
      g_cond_signal (stream->queue_empty);
      g_mutex_unlock (stream->queue_lock);
    }
  }
}

static void
gst_hlsdemux2_on_playlist_buffer (GstElement *appsink, void *data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  GstBuffer *inbuf = NULL;

  inbuf = gst_app_sink_pull_buffer ((GstAppSink *)appsink);
  if (!inbuf) {
    GST_WARNING_OBJECT (demux, "Input buffer not available...");
    return;
  }

  if (demux->pldownloader->playlist == NULL) {
    demux->pldownloader->playlist = gst_buffer_copy (inbuf);
    gst_buffer_unref (inbuf);
  } else {
    GstBuffer *buffer = NULL;
    guint size = GST_BUFFER_SIZE(demux->pldownloader->playlist) + GST_BUFFER_SIZE(inbuf);

    buffer = gst_buffer_new_and_alloc (size);
    if (!buffer) {
      GST_ERROR_OBJECT (demux, "failed allocate memory...");
      GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
      gst_buffer_unref (inbuf);
      gst_buffer_unref (demux->pldownloader->playlist);
      return;
    }

    /*copy existing playlist buffer */
    memcpy (GST_BUFFER_DATA(buffer), GST_BUFFER_DATA(demux->pldownloader->playlist), GST_BUFFER_SIZE(demux->pldownloader->playlist));

    /*copy new playlist buffer */
    memcpy (GST_BUFFER_DATA(buffer) + GST_BUFFER_SIZE(demux->pldownloader->playlist),
        GST_BUFFER_DATA(inbuf), GST_BUFFER_SIZE(inbuf));

    gst_buffer_unref (inbuf);
    gst_buffer_unref (demux->pldownloader->playlist);
    demux->pldownloader->playlist = buffer;
  }

  return;
}

static void
gst_hlsdemux2_on_key_buffer (GstElement *appsink, void *data)
{
  GstHLSDemux2 *demux = (GstHLSDemux2 *)data;
  GstBuffer *inbuf = NULL;

  inbuf = gst_app_sink_pull_buffer ((GstAppSink *)appsink);
  if (!inbuf) {
    GST_WARNING_OBJECT (demux, "Input buffer not available...");
    return;
  }

  if (demux->kdownloader->key == NULL) {
    demux->kdownloader->key = gst_buffer_copy (inbuf);
    gst_buffer_unref (inbuf);
  } else {
    GstBuffer *buffer = NULL;
    guint size = GST_BUFFER_SIZE(demux->kdownloader->key) + GST_BUFFER_SIZE(inbuf);

    buffer = gst_buffer_new_and_alloc (size);
    if (!buffer) {
      GST_ERROR_OBJECT (demux, "failed allocate memory...");
      GST_ELEMENT_ERROR (demux, RESOURCE, NO_SPACE_LEFT, ("can't allocate memory"), (NULL));
      gst_buffer_unref (inbuf);
      gst_buffer_unref (demux->kdownloader->key);
      return;
    }

    /*copy existing playlist buffer */
    memcpy (GST_BUFFER_DATA(buffer), GST_BUFFER_DATA(demux->kdownloader->key), GST_BUFFER_SIZE(demux->kdownloader->key));

    /*copy new playlist buffer */
    memcpy (GST_BUFFER_DATA(buffer) + GST_BUFFER_SIZE(demux->kdownloader->key),
        GST_BUFFER_DATA(inbuf), GST_BUFFER_SIZE(inbuf));

    gst_buffer_unref (inbuf);
    gst_buffer_unref (demux->kdownloader->key);
    demux->kdownloader->key = buffer;
  }

  return;
}

static void
gst_hlsdemux2_stop (GstHLSDemux2 *demux)
{
  GST_INFO_OBJECT (demux, "entering...");
#ifdef TEMP_FIX_FOR_TASK_STOP
  gboolean    is_task_join_success = FALSE;
#endif

  demux->cancelled = TRUE;

  g_cond_signal (demux->post_msg_start);
  g_cond_signal (demux->post_msg_exit);

  /* special case: for exiting playlist downloader created from sink event */
  if (demux->pldownloader && demux->pldownloader->cond)
    g_cond_broadcast (demux->pldownloader->cond);

  while (demux->download_task && (GST_TASK_STATE (demux->download_task) != GST_TASK_STOPPED)) {
    int idx = 0;
    GstHLSDemux2Stream *stream = NULL;

    GST_INFO_OBJECT (demux, "Closing Download task...");

    /* signal queue full conditions */
    for (idx = 0; idx < HLSDEMUX2_STREAM_NUM; idx++) {
      stream = demux->streams[idx];

      if (stream && GST_PAD_TASK(stream->pad)) {
        gst_pad_push_event (stream->pad, gst_event_new_flush_start ());
        g_cond_broadcast(stream->queue_full);
        g_cond_broadcast(stream->queue_empty);
      }
    }

    if (demux->pl_update_cond)
      g_cond_broadcast (demux->pl_update_cond);

    if (demux->fdownloader && demux->fdownloader->cond)
      g_cond_broadcast (demux->fdownloader->cond);
    if (demux->kdownloader && demux->kdownloader->cond)
      g_cond_broadcast (demux->kdownloader->cond);
    if (demux->pldownloader && demux->pldownloader->cond)
      g_cond_broadcast (demux->pldownloader->cond);

    GST_INFO_OBJECT (demux, "Waiting to acquire download lock");
    g_static_rec_mutex_lock (&demux->download_lock);
    g_static_rec_mutex_unlock (&demux->download_lock);

#ifdef TEMP_FIX_FOR_TASK_STOP
    if (demux->download_task != NULL)
    {
       GST_INFO_OBJECT (demux, "gst_hlsdemux2_stop : Trying to JOIN the task !!!!");
	is_task_join_success = gst_task_join(demux->download_task);
	if (is_task_join_success == TRUE)
	{
	       GST_INFO_OBJECT (demux, "gst_hlsdemux2_stop : JOIN Success !!!!");
	       g_thread_join(demux->buffering_posting_thread);
               GST_INFO_OBJECT (demux, "Completed closing of download monitor thread...");
	}
	else
	{
		GST_INFO_OBJECT (demux, "gst_hlsdemux2_stop : JOIN Failed : Trying Stop !!!!");
		gst_task_stop(demux->download_task);
	}

	gst_object_unref (demux->download_task);
	demux->download_task = NULL;
    }

    GST_INFO_OBJECT (demux, "Completed closing of download task...");
#else
    GST_INFO_OBJECT (demux, "Waiting download task JOIN");
    gst_task_join (demux->download_task);
    gst_object_unref (demux->download_task);
    demux->download_task = NULL;

    GST_INFO_OBJECT (demux, "Completed closing of download task...");

    g_thread_join(demux->buffering_posting_thread);
    GST_INFO_OBJECT (demux, "Completed closing of download monitor thread...");
#endif
    if (demux->fdownloader && demux->fdownloader->pipe) {
      GST_WARNING_OBJECT (demux, "Still fragment download exist");
      gst_hlsdemux2_destroy_fragment_download (demux);
    }
    if (demux->pldownloader && demux->pldownloader->pipe) {
      GST_WARNING_OBJECT (demux, "Still playlist download exist");
      gst_hlsdemux2_destroy_playlist_download (demux);
    }
    if (demux->kdownloader && demux->kdownloader->pipe) {
      GST_WARNING_OBJECT (demux, "Still key download exist");
      gst_hlsdemux2_destroy_key_download (demux);
    }
  }

  gst_hlsdemux2_stop_stream (demux);

  GST_INFO_OBJECT (demux, "leaving...");
}

static void
gst_hlsdemux2_stop_stream (GstHLSDemux2 *demux)
{
  int n = 0;
  GstHLSDemux2Stream *stream = NULL;

#ifdef TEMP_FIX_FOR_TASK_STOP
  GST_INFO_OBJECT (demux, "entering : active_stream_cnt(%d)",demux->active_stream_cnt);
  if (demux->active_stream_cnt == 0)
  {
	GST_INFO_OBJECT (demux, "gst_hlsdemux2_stop_stream : Stream Count is 0 :: RETURNING !!!");
	return;
  }
#else
  GST_INFO_OBJECT (demux, "entering");
#endif

  for (n = 0; n < HLSDEMUX2_STREAM_NUM; n++) {
    stream = demux->streams[n];

    if (stream && GST_PAD_TASK(stream->pad)) {
      GST_INFO_OBJECT (stream->pad, "stopping pad task : %s",gst_pad_get_name (stream->pad));
      gst_pad_stop_task (stream->pad);
      GST_INFO_OBJECT (stream->pad, "stopped pad task : %s", gst_pad_get_name (stream->pad));
    }
  }

  for (n = 0; n < HLSDEMUX2_STREAM_NUM; n++) {
    if (demux->streams[n]) {
      gst_hlsdemux2_stream_deinit(demux->streams[n], demux);
      demux->streams[n] = NULL;
    }
  }
  demux->active_stream_cnt = 0;
  GST_INFO_OBJECT (demux, "leaving");
}

static void
gst_hlsdemux2_stream_init (GstHLSDemux2 *demux, GstHLSDemux2Stream *stream, HLSDEMUX2_STREAM_TYPE stream_type, gchar *name, GstCaps *src_caps)
{
  stream->queue = g_queue_new ();
  stream->queue_full = g_cond_new ();
  stream->queue_empty = g_cond_new ();
  stream->queue_lock = g_mutex_new ();
  stream->parent = demux;
  stream->type = stream_type;
  stream->percent = 100;
  stream->eos = FALSE;
  stream->cached_duration = 0;
  stream->lts = 0;
  stream->dummy_data_thread = NULL;
  stream->base_ts = GST_CLOCK_TIME_NONE;
  stream->fts = GST_CLOCK_TIME_NONE;
  stream->valid_fts_rcvd = FALSE;
  stream->total_stream_time = 0;
  stream->total_disc = 0;
  stream->cdisc = 0;
  stream->nts = GST_CLOCK_TIME_NONE;
  stream->prev_nts = 0;
  stream->downloader_queue = g_queue_new();
  stream->need_newsegment = TRUE;
  stream->newsegment = NULL;
  stream->frame_duration = 0;

  if (stream_type == HLSDEMUX2_STREAM_VIDEO) {
    stream->pad = gst_pad_new_from_static_template (&hlsdemux2_videosrc_template, name);
    GST_INFO_OBJECT (demux, "Its a Multi-variant stream..");
    demux->stream_config = HLSDEMUX2_MULTI_VARIANT;
  } else if (stream_type == HLSDEMUX2_STREAM_AUDIO) {
    stream->pad = gst_pad_new_from_static_template (&hlsdemux2_audiosrc_template, name);
  } else if (stream_type == HLSDEMUX2_STREAM_TEXT) {
    stream->pad = gst_pad_new_from_static_template (&hlsdemux2_subpicture_template, name);
  } else if (stream_type == HLSDEMUX2_STREAM_PRIVATE) {
    stream->pad = gst_pad_new_from_static_template (&hlsdemux2_private_template, name);
  }

  GST_INFO_OBJECT (demux, "======>>>> created hlsdemux2 source pad : %s", name);

  GST_PAD_ELEMENT_PRIVATE (stream->pad) = stream;

  gst_pad_use_fixed_caps (stream->pad);
  gst_pad_set_event_function (stream->pad, gst_hlsdemux2_handle_src_event);
  gst_pad_set_query_function (stream->pad, gst_hlsdemux2_src_query);
  gst_pad_set_caps (stream->pad, src_caps);

  GST_DEBUG_OBJECT (demux, "adding pad %s %p to demux %p", GST_OBJECT_NAME (stream->pad), stream->pad, demux);
  gst_pad_set_active (stream->pad, TRUE);
  gst_element_add_pad (GST_ELEMENT_CAST (demux), stream->pad);

  if (!gst_pad_is_linked (stream->pad)) {
    GST_WARNING_OBJECT (stream->pad, "'%s' pad is not linked...", gst_pad_get_name (stream->pad));
    stream->is_linked = FALSE;
    return;
  }
  stream->is_linked = TRUE;

  if (!gst_pad_start_task (stream->pad, (GstTaskFunction) gst_hlsdemux2_push_loop, stream)) {
    GST_ERROR_OBJECT (demux, "failed to create stream task...");
    GST_ELEMENT_ERROR (demux, RESOURCE, FAILED, ("failed to create stream push loop"), (NULL));
    return;
  }

}

static void
gst_hlsdemux2_stream_deinit (GstHLSDemux2Stream *stream, gpointer data)
{
  GstHLSDemux2 * demux = (GstHLSDemux2 *)data;
  if (stream) {

    if (stream->queue) {
      while (!g_queue_is_empty(stream->queue)) {
        gst_buffer_unref (g_queue_pop_head (stream->queue));
      }
      g_queue_free (stream->queue);
      stream->queue = NULL;
    }

    if (stream->pad) {
      gst_element_remove_pad (GST_ELEMENT_CAST (demux), stream->pad);
      stream->pad = NULL;
    }

    if (stream->queue_lock) {
      g_mutex_free (stream->queue_lock);
      stream->queue_lock = NULL;
    }
    if (stream->queue_full) {
      g_cond_free (stream->queue_full);
      stream->queue_full = NULL;
    }
    if (stream->queue_empty) {
      g_cond_free (stream->queue_empty);
      stream->queue_empty= NULL;
    }
    g_free (stream);
  }

}

static gboolean
gst_hlsdemux2_set_location (GstHLSDemux2 * demux, const gchar * uri)
{
  if (demux->client)
    gst_m3u8_client_free (demux->client);
  demux->client = gst_m3u8_client_new (uri);
  GST_INFO_OBJECT (demux, "Changed location: %s", uri);
  return TRUE;
}

static gchar *
gst_hlsdemux2_src_buf_to_utf8_playlist (GstBuffer * buf)
{
  gint size;
  gchar *data;
  gchar *playlist;

  data = (gchar *) GST_BUFFER_DATA (buf);
  size = GST_BUFFER_SIZE (buf);

#if 0 /* giving error some times, so removing*/
  if (!g_utf8_validate (data, size, NULL))
    goto validate_error;
#endif

  /* alloc size + 1 to end with a null character */
  playlist = g_malloc0 (size + 1);
  if (!playlist) {
    GST_ERROR ("failed to create memory...");
    gst_buffer_unref (buf);
    return NULL;
  }

  memcpy (playlist, data, size);
  playlist[size] = '\0';

  gst_buffer_unref (buf);
  return playlist;
#if 0
validate_error:
  gst_buffer_unref (buf);
  GST_ERROR ("failed to validate playlist");
  return NULL;
#endif
}

static void
gst_hlsdemux2_handle_private_pad (GstHLSDemux2 *demux, GstPad *srcpad)
{
  GstHLSDemux2PvtStream *pvt_stream = NULL;
  gchar *sinkname = NULL;
  GstPad *sinkpad = NULL;

  pvt_stream = g_new0 (GstHLSDemux2PvtStream, 1);

  demux->private_stream = pvt_stream;
  pvt_stream->parent = demux;
  pvt_stream->type = HLSDEMUX2_STREAM_PRIVATE;
  pvt_stream->image_buffer = NULL;
  pvt_stream->got_img_buffer = FALSE;
  pvt_stream->video_buffer = NULL;
  pvt_stream->id3_buffer   = NULL;
  pvt_stream->convert_lock = g_mutex_new ();
  pvt_stream->convert_cond = g_cond_new ();
  pvt_stream->sink = NULL;
  pvt_stream->img_load_lock = g_mutex_new ();
  pvt_stream->img_load_cond = g_cond_new ();

  /* create sink element */
  sinkname = g_strdup_printf("%s-%s", GST_PAD_NAME(srcpad) , "sink");
  pvt_stream->sink =  gst_element_factory_make ("appsink", NULL);
  if (!pvt_stream->sink) {
    GST_ERROR_OBJECT (demux, "failed to create sink element - %s", sinkname);
    GST_ELEMENT_ERROR (demux, CORE, FAILED, ("failed to create element - %s", sinkname), (NULL));
    return;
  }

  /* set sink element to PLAYING state */
  gst_bin_add (GST_BIN (demux->fdownloader->pipe), pvt_stream->sink);
  gst_element_set_state (pvt_stream->sink, GST_STATE_READY);
  g_object_set (G_OBJECT (pvt_stream->sink), "sync", FALSE, "emit-signals", TRUE, NULL);

  sinkpad = gst_element_get_pad(pvt_stream->sink, "sink");
  if (!sinkpad) {
    GST_ERROR_OBJECT (demux, "failed to get sinkpad from element  - %s", sinkname);
    GST_ELEMENT_ERROR (demux, CORE, PAD, ("Count not get sink pad"), (NULL));
    return;
  }

  if (gst_pad_link(srcpad, sinkpad) != GST_PAD_LINK_OK) {
    GST_ERROR_OBJECT (demux, "failed to link sink bin elements");
    GST_ELEMENT_ERROR (demux, CORE, NEGOTIATION, ("Could not link pads.."), (NULL));
    gst_object_unref(sinkpad);
  }

  gst_element_set_state (pvt_stream->sink, GST_STATE_PLAYING);

  g_signal_connect (pvt_stream->sink, "new-buffer",  G_CALLBACK (gst_hlsdemux2_private_sink_on_new_buffer), pvt_stream);
  g_signal_connect (pvt_stream->sink, "eos",  G_CALLBACK (gst_hlsdemux2_private_sink_on_eos), pvt_stream);

  gst_object_unref(sinkpad);

  GST_INFO_OBJECT (demux, "successfully created private pad bin...");

  return;
}

static void
gst_hlsdemux2_private_sink_on_new_buffer (GstElement *appsink, void *user_data)
{
  GstHLSDemux2PvtStream *pvt_stream = (GstHLSDemux2PvtStream *)user_data;
  GstHLSDemux2 *demux = pvt_stream->parent;
  GstBuffer *inbuf = NULL;

  inbuf = gst_app_sink_pull_buffer ((GstAppSink *)appsink);
  if (!inbuf) {
    GST_ERROR_OBJECT (demux, "Input buffer not available....");
    return;
  }

  GST_LOG_OBJECT (demux, "Image buffer received of size = %d", GST_BUFFER_SIZE (inbuf));
  if(pvt_stream->id3_buffer==NULL)
    pvt_stream->id3_buffer = gst_buffer_copy(inbuf);
  else
    pvt_stream->id3_buffer = gst_buffer_join (pvt_stream->id3_buffer, inbuf);
  return;
}

static void
gst_hlsdemux2_private_sink_on_eos (GstElement * appsink, void* user_data)
{
  GstHLSDemux2PvtStream *pvt_stream = (GstHLSDemux2PvtStream *)user_data;
  GstHLSDemux2 *demux = pvt_stream->parent;
  GstBuffer *image_buffer = NULL;
  GstTagList *tag_list = gst_tag_list_from_id3v2_tag(pvt_stream->id3_buffer);

  GST_INFO_OBJECT (demux, "Processing image buffer");

  if(!gst_tag_list_get_buffer (tag_list, GST_TAG_IMAGE, &image_buffer)) {
    GST_ERROR_OBJECT(demux, "Failed to obtain image data.");
    return;
  }

  if(demux->stream_config == HLSDEMUX2_SINGLE_VARIANT) {
    gchar *image_header = NULL;
    GValue codec_type = G_VALUE_INIT;
    GstMessage * tag_message = NULL;

    /* check whether it is same as previous image */
    if (demux->prev_image_buffer &&
      (GST_BUFFER_SIZE(demux->prev_image_buffer) == GST_BUFFER_SIZE(image_buffer))) {
      if (!memcmp (GST_BUFFER_DATA(demux->prev_image_buffer), GST_BUFFER_DATA(image_buffer), GST_BUFFER_SIZE(image_buffer))) {
        GST_INFO_OBJECT (demux, "current & previous embedded images are same..no need to post image again");
        gst_buffer_unref (image_buffer);
        return;
      }
    }

    if (demux->prev_image_buffer) {
      gst_buffer_unref (demux->prev_image_buffer);
    }
    demux->prev_image_buffer = gst_buffer_copy(image_buffer);

    image_header = g_strndup((gchar *) image_buffer->data, 8);

    g_value_init (&codec_type, G_TYPE_STRING);

    if((image_header[0] == 0xFF) && (image_header[1] == 0xD8)) {
      GST_INFO_OBJECT(demux, "Found JPEG image header");
      g_value_set_static_string (&codec_type, "image/jpeg");
      gst_tag_list_add_value(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_CODEC, &codec_type);
    } else if (!g_strcmp0(image_header, "\211PNG\015\012\032\012")) {
      GST_INFO_OBJECT(demux, "Found PNG image header");
      g_value_set_static_string (&codec_type, "image/png");
      gst_tag_list_add_value(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_CODEC, &codec_type);
    } else {
      g_value_set_static_string (&codec_type, "image/unknown");
      GST_INFO_OBJECT(demux, "Unknown image header");
      gst_tag_list_add_value(tag_list, GST_TAG_MERGE_APPEND, GST_TAG_CODEC, &codec_type);
    }

    GST_INFO_OBJECT(demux, "Set value : %s", g_value_get_string (&codec_type));
    g_value_unset(&codec_type);

    tag_message = gst_message_new_tag (GST_OBJECT_CAST (demux), tag_list);
    if(!gst_element_post_message (GST_ELEMENT_CAST (demux), tag_message)) {
      GST_ERROR_OBJECT (demux, "failed to post image tag");
      gst_buffer_unref (image_buffer);
      g_free (image_header);
      return;
    }

    GST_INFO_OBJECT (demux, "successfully posted image tag...");
    gst_buffer_unref (image_buffer);
    g_free (image_header);
    return;
  }

  GST_INFO_OBJECT(demux, "image header : %d,%d", image_buffer->data[0],image_buffer->data[1]);

  pvt_stream->image_buffer = gst_buffer_copy(image_buffer);
  gst_buffer_unref (image_buffer);
  pvt_stream->got_img_buffer = TRUE;

  g_cond_signal (pvt_stream->img_load_cond);
  demux->has_image_buffer = TRUE;

}

static gboolean
hlsdemux2_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "hlsdemux2", GST_RANK_PRIMARY + 1, GST_TYPE_HLSDEMUX2) || FALSE)
    return FALSE;
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  "hlsdemux2",
  "HLS demux - 2 plugin",
  hlsdemux2_init,
  VERSION,
  "LGPL",
  PACKAGE_NAME,
  "http://www.samsung.com/")

