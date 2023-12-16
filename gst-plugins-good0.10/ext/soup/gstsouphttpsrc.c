/* GStreamer
 * Copyright (C) 2007-2008 Wouter Cloetens <wouter@mind.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more
 */

/**
 * SECTION:element-souphttpsrc
 *
 * This plugin reads data from a remote location specified by a URI.
 * Supported protocols are 'http', 'https'.
 *
 * An HTTP proxy must be specified by its URL.
 * If the "http_proxy" environment variable is set, its value is used.
 * If built with libsoup's GNOME integration features, the GNOME proxy
 * configuration will be used, or failing that, proxy autodetection.
 * The #GstSoupHTTPSrc:proxy property can be used to override the default.
 *
 * In case the #GstSoupHTTPSrc:iradio-mode property is set and the location is
 * an HTTP resource, souphttpsrc will send special Icecast HTTP headers to the
 * server to request additional Icecast meta-information.
 * If the server is not an Icecast server, it will behave as if the
 * #GstSoupHTTPSrc:iradio-mode property were not set. If it is, souphttpsrc will
 * output data with a media type of application/x-icy, in which case you will
 * need to use the #ICYDemux element as follow-up element to extract the Icecast
 * metadata and to determine the underlying media type.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v souphttpsrc location=https://some.server.org/index.html
 *     ! filesink location=/home/joe/server.html
 * ]| The above pipeline reads a web page from a server using the HTTPS protocol
 * and writes it to a local file.
 * |[
 * gst-launch -v souphttpsrc user-agent="FooPlayer 0.99 beta"
 *     automatic-redirect=false proxy=http://proxy.intranet.local:8080
 *     location=http://music.foobar.com/demo.mp3 ! mad ! audioconvert
 *     ! audioresample ! alsasink
 * ]| The above pipeline will read and decode and play an mp3 file from a
 * web server using the HTTP protocol. If the server sends redirects,
 * the request fails instead of following the redirect. The specified
 * HTTP proxy server is used. The User-Agent HTTP request header
 * is set to a custom string instead of "GStreamer souphttpsrc."
 * |[
 * gst-launch -v souphttpsrc location=http://10.11.12.13/mjpeg
 *     do-timestamp=true ! multipartdemux
 *     ! image/jpeg,width=640,height=480 ! matroskamux
 *     ! filesink location=mjpeg.mkv
 * ]| The above pipeline reads a motion JPEG stream from an IP camera
 * using the HTTP protocol, encoded as mime/multipart image/jpeg
 * parts, and writes a Matroska motion JPEG file. The width and
 * height properties are set in the caps to provide the Matroska
 * multiplexer with the information to set this in the header.
 * Timestamps are set on the buffers as they arrive from the camera.
 * These are used by the mime/multipart demultiplexer to emit timestamps
 * on the JPEG-encoded video frame buffers. This allows the Matroska
 * multiplexer to timestamp the frames in the resulting file.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>             /* atoi() */
#endif
#include <gst/gstelement.h>
#include <gst/gst-i18n-plugin.h>
#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif
#include "gstsouphttpsrc.h"

#include <gst/tag/tag.h>

#define SEEK_CHANGES

GST_DEBUG_CATEGORY_STATIC (souphttpsrc_debug);
#define GST_CAT_DEFAULT souphttpsrc_debug

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_IS_LIVE,
  PROP_USER_AGENT,
  PROP_AUTOMATIC_REDIRECT,
  PROP_PROXY,
  PROP_USER_ID,
  PROP_USER_PW,
  PROP_PROXY_ID,
  PROP_PROXY_PW,
  PROP_COOKIES,
  PROP_IRADIO_MODE,
  PROP_IRADIO_NAME,
  PROP_IRADIO_GENRE,
  PROP_IRADIO_URL,
  PROP_IRADIO_TITLE,
  PROP_TIMEOUT,
#ifdef GST_EXT_SOUP_MODIFICATION
  PROP_CONTENT_SIZE,
  PROP_AHS_STREAMING,
#endif
#ifdef SEEK_CHANGES
  PROP_EXTRA_HEADERS,
  PROP_RANGESIZE,
#else
  PROP_EXTRA_HEADERS,
#endif
  PROP_SOUPCOOKIES
};

#define DEFAULT_USER_AGENT           "GStreamer souphttpsrc "

#ifdef GST_EXT_SOUP_MODIFICATION
#define DEFAULT_RETRY_TIMEOUT -1
#define DEFAULT_SESSION_TIMEOUT 20
#define MIN_SESSION_TIMEOUT 5

#define DLNA_OP_TIMED_SEEK  0x02
#define DLNA_OP_BYTE_SEEK   0x01
#endif

static void gst_soup_http_src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);
static void gst_soup_http_src_finalize (GObject * gobject);

static void gst_soup_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_soup_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_soup_http_src_create (GstPushSrc * psrc,
    GstBuffer ** outbuf);
static gboolean gst_soup_http_src_start (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_stop (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_get_size (GstBaseSrc * bsrc, guint64 * size);
static gboolean gst_soup_http_src_is_seekable (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_do_seek (GstBaseSrc * bsrc,
    GstSegment * segment);
static gboolean gst_soup_http_src_query (GstBaseSrc * bsrc, GstQuery * query);
#ifdef USE_SAMSUNG_LINK
static gboolean gst_soup_http_src_event (GstBaseSrc * bsrc, GstEvent *event);
#endif

static gboolean gst_soup_http_src_unlock (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_unlock_stop (GstBaseSrc * bsrc);
static gboolean gst_soup_http_src_set_location (GstSoupHTTPSrc * src,
    const gchar * uri);
static gboolean gst_soup_http_src_set_proxy (GstSoupHTTPSrc * src,
    const gchar * uri);
static char *gst_soup_http_src_unicodify (const char *str);
static gboolean gst_soup_http_src_build_message (GstSoupHTTPSrc * src);
static void gst_soup_http_src_cancel_message (GstSoupHTTPSrc * src);
static void gst_soup_http_src_queue_message (GstSoupHTTPSrc * src);
#ifdef GST_EXT_SOUP_MODIFICATION
static gboolean gst_soup_http_src_add_range_header (GstSoupHTTPSrc * src,
    guint64 offset , gchar *range);
#else
static gboolean gst_soup_http_src_add_range_header (GstSoupHTTPSrc * src,
    guint64 offset)
#endif
static void gst_soup_http_src_session_unpause_message (GstSoupHTTPSrc * src);
static void gst_soup_http_src_session_pause_message (GstSoupHTTPSrc * src);
static void gst_soup_http_src_session_close (GstSoupHTTPSrc * src);
static void gst_soup_http_src_parse_status (SoupMessage * msg,
    GstSoupHTTPSrc * src);
static void gst_soup_http_src_chunk_free (gpointer gstbuf);
static SoupBuffer *gst_soup_http_src_chunk_allocator (SoupMessage * msg,
    gsize max_len, gpointer user_data);
static void gst_soup_http_src_got_chunk_cb (SoupMessage * msg,
    SoupBuffer * chunk, GstSoupHTTPSrc * src);
static void gst_soup_http_src_response_cb (SoupSession * session,
    SoupMessage * msg, GstSoupHTTPSrc * src);
static void gst_soup_http_src_got_headers_cb (SoupMessage * msg,
    GstSoupHTTPSrc * src);
static void gst_soup_http_src_got_body_cb (SoupMessage * msg,
    GstSoupHTTPSrc * src);
static void gst_soup_http_src_finished_cb (SoupMessage * msg,
    GstSoupHTTPSrc * src);
static void gst_soup_http_src_authenticate_cb (SoupSession * session,
    SoupMessage * msg, SoupAuth * auth, gboolean retrying,
    GstSoupHTTPSrc * src);

static void
_do_init (GType type)
{
  static const GInterfaceInfo urihandler_info = {
    gst_soup_http_src_uri_handler_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (type, GST_TYPE_URI_HANDLER, &urihandler_info);

  GST_DEBUG_CATEGORY_INIT (souphttpsrc_debug, "souphttpsrc", 0,
      "SOUP HTTP src");
}

GST_BOILERPLATE_FULL (GstSoupHTTPSrc, gst_soup_http_src, GstPushSrc,
    GST_TYPE_PUSH_SRC, _do_init);

static void
gst_soup_http_src_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class, &srctemplate);

  gst_element_class_set_details_simple (element_class, "HTTP client source",
      "Source/Network",
      "Receive data as a client over the network via HTTP using SOUP",
      "Wouter Cloetens <wouter@mind.be>");
}

static void
gst_soup_http_src_class_init (GstSoupHTTPSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_soup_http_src_set_property;
  gobject_class->get_property = gst_soup_http_src_get_property;
  gobject_class->finalize = gst_soup_http_src_finalize;

  g_object_class_install_property (gobject_class,
      PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location to read from", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_USER_AGENT,
      g_param_spec_string ("user-agent", "User-Agent",
          "Value of the User-Agent HTTP request header field",
          DEFAULT_USER_AGENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_AUTOMATIC_REDIRECT,
      g_param_spec_boolean ("automatic-redirect", "automatic-redirect",
          "Automatically follow HTTP redirects (HTTP Status Code 3xx)",
          TRUE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_PROXY,
      g_param_spec_string ("proxy", "Proxy",
          "HTTP proxy server URI", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_USER_ID,
      g_param_spec_string ("user-id", "user-id",
          "HTTP location URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USER_PW,
      g_param_spec_string ("user-pw", "user-pw",
          "HTTP location URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY_ID,
      g_param_spec_string ("proxy-id", "proxy-id",
          "HTTP proxy URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY_PW,
      g_param_spec_string ("proxy-pw", "proxy-pw",
          "HTTP proxy URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COOKIES,
      g_param_spec_boxed("cookies", "Cookies", "HTTP request cookies",
          G_TYPE_STRV, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SOUPCOOKIES,
      g_param_spec_pointer("soupcookies", "soupcookies", "web HTTP request cookies",
          G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "is-live", "Act like a live source",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#ifdef GST_EXT_SOUP_MODIFICATION
  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_int ("timeout", "timeout",
          "Value in seconds to timeout (0 = No timeout, -1 = default timeout + infinite retry).", -1,
          G_MAXINT, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#else
  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_uint ("timeout", "timeout",
          "Value in seconds to timeout a blocking I/O (0 = No timeout).", 0,
          3600, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif
  g_object_class_install_property (gobject_class, PROP_EXTRA_HEADERS,
      g_param_spec_boxed ("extra-headers", "Extra Headers",
          "Extra headers to append to the HTTP request",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

#ifdef SEEK_CHANGES
  g_object_class_install_property (gobject_class, PROP_RANGESIZE,
      g_param_spec_int64 ("rangesize", "rangesize",
          "Size of each buffer downloaded from libsoup",
          -1, G_MAXUINT, 4096, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif

#ifdef GST_EXT_SOUP_MODIFICATION
  g_object_class_install_property (gobject_class, PROP_CONTENT_SIZE,
      g_param_spec_uint64 ("content-size", "contentsize",
          "Total size of the content under download",
          0, G_MAXUINT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_AHS_STREAMING,
      g_param_spec_boolean ("ahs-streaming", "adaptive HTTP Streaming",
          "set whether adaptive HTTP streaming(HLS/SS/DASH) or not ",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif

  /* icecast stuff */
  g_object_class_install_property (gobject_class,
      PROP_IRADIO_MODE,
      g_param_spec_boolean ("iradio-mode",
          "iradio-mode",
          "Enable internet radio mode (extraction of shoutcast/icecast metadata)",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_IRADIO_NAME,
      g_param_spec_string ("iradio-name",
          "iradio-name", "Name of the stream", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_IRADIO_GENRE,
      g_param_spec_string ("iradio-genre",
          "iradio-genre", "Genre of the stream", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_IRADIO_URL,
      g_param_spec_string ("iradio-url",
          "iradio-url",
          "Homepage URL for radio stream", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
      PROP_IRADIO_TITLE,
      g_param_spec_string ("iradio-title",
          "iradio-title",
          "Name of currently playing song", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_soup_http_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_soup_http_src_stop);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_soup_http_src_unlock);
  gstbasesrc_class->unlock_stop =
      GST_DEBUG_FUNCPTR (gst_soup_http_src_unlock_stop);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_soup_http_src_get_size);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (gst_soup_http_src_is_seekable);
#ifdef USE_SAMSUNG_LINK
  gstbasesrc_class->event = GST_DEBUG_FUNCPTR (gst_soup_http_src_event);
#endif
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (gst_soup_http_src_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_soup_http_src_query);

  gstpushsrc_class->create = GST_DEBUG_FUNCPTR (gst_soup_http_src_create);
}

static void
gst_soup_http_src_reset (GstSoupHTTPSrc * src)
{
  src->interrupted = FALSE;
  src->retry = FALSE;
  src->have_size = FALSE;
  src->seekable = FALSE;
  src->read_position = 0;
  src->request_position = 0;
  src->content_size = 0;
  src->have_body = FALSE;
#ifdef GST_EXT_SOUP_MODIFICATION
  src->content_len = 0;
  src->timeout = DEFAULT_RETRY_TIMEOUT;
  src->session_timeout = DEFAULT_SESSION_TIMEOUT;
  src->op_code = 0;
#endif
#ifdef SEEK_CHANGES
  src->file_size = 0;
#endif
  gst_caps_replace (&src->src_caps, NULL);
  g_free (src->iradio_name);
  src->iradio_name = NULL;
  g_free (src->iradio_genre);
  src->iradio_genre = NULL;
  g_free (src->iradio_url);
  src->iradio_url = NULL;
  g_free (src->iradio_title);
  src->iradio_title = NULL;
}

static void
gst_soup_http_src_init (GstSoupHTTPSrc * src, GstSoupHTTPSrcClass * g_class)
{
  const gchar *proxy;

  src->location = NULL;
  src->automatic_redirect = TRUE;
  src->user_agent = g_strdup (DEFAULT_USER_AGENT);
  src->user_id = NULL;
  src->user_pw = NULL;
  src->proxy_id = NULL;
  src->proxy_pw = NULL;
  src->cookies = NULL;
  src->iradio_mode = FALSE;
  src->loop = NULL;
  src->context = NULL;
  src->session = NULL;
  src->msg = NULL;
#ifdef SEEK_CHANGES
  src->file_size = 0;
  src->range_size = 0;
#endif
  proxy = g_getenv ("http_proxy");
  if (proxy && !gst_soup_http_src_set_proxy (src, proxy)) {
    GST_WARNING_OBJECT (src,
        "The proxy in the http_proxy env var (\"%s\") cannot be parsed.",
        proxy);
  }
#ifdef GST_EXT_SOUP_MODIFICATION
  src->is_ahs_streaming = FALSE;
  src->cookie_jar  = NULL;
  src->cookie_list = NULL;
  src->op_code = 0;
#endif

  gst_soup_http_src_reset (src);
}

static void
gst_soup_http_src_finalize (GObject * gobject)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (gobject);

  GST_DEBUG_OBJECT (src, "finalize");

  g_free (src->location);
  g_free (src->user_agent);
  if (src->proxy != NULL) {
    soup_uri_free (src->proxy);
  }
  g_free (src->user_id);
  g_free (src->user_pw);
  g_free (src->proxy_id);
  g_free (src->proxy_pw);
  if (src->cookie_list != NULL) {
    g_slist_free (src->cookie_list);
    src->cookie_list = NULL;
  }

  g_strfreev (src->cookies);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
gst_soup_http_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
    {
      const gchar *location;

      location = g_value_get_string (value);

      if (location == NULL) {
        GST_WARNING ("location property cannot be NULL");
        goto done;
      }
      if (!gst_soup_http_src_set_location (src, location)) {
        GST_WARNING ("badly formatted location");
        goto done;
      }
      break;
    }
    case PROP_USER_AGENT:
      if (src->user_agent)
        g_free (src->user_agent);
      src->user_agent = g_value_dup_string (value);
      break;
    case PROP_IRADIO_MODE:
      src->iradio_mode = g_value_get_boolean (value);
      break;
    case PROP_AUTOMATIC_REDIRECT:
      src->automatic_redirect = g_value_get_boolean (value);
      break;
    case PROP_PROXY:
    {
      const gchar *proxy;

      proxy = g_value_get_string (value);

      if (proxy == NULL) {
        GST_WARNING ("proxy property cannot be NULL");
        goto done;
      }
      if (!gst_soup_http_src_set_proxy (src, proxy)) {
        GST_WARNING ("badly formatted proxy URI");
        goto done;
      }
      break;
    }
    case PROP_COOKIES:
    {
#ifdef GST_EXT_SOUP_MODIFICATION
      char **array;
      SoupURI *base_uri;

#endif
      g_strfreev (src->cookies);
      src->cookies = g_strdupv (g_value_get_boxed (value));
#ifdef GST_EXT_SOUP_MODIFICATION
      if (src->cookie_jar && ((array = src->cookies) != NULL)) {
	  	base_uri = soup_uri_new (src->location);
        GST_INFO_OBJECT (src, "request to set cookies...");
        while (*array != NULL) {
          soup_cookie_jar_add_cookie (src->cookie_jar,
              soup_cookie_parse (*array++, base_uri));
        }
        soup_uri_free (base_uri);
      } else {
        GST_INFO_OBJECT (src, "set cookies after session creation");
      }
#endif
      break;
    }
#ifdef GST_EXT_SOUP_MODIFICATION
    case PROP_SOUPCOOKIES:
    {
      GSList *walk, *new_cookies;
      SoupCookie* cookie;

      new_cookies = (GSList*)g_value_get_pointer(value);

      /* remove old list and alloc new */
      if (src->cookie_list)
      {
        g_slist_free (src->cookie_list);
      }
      src->cookie_list = g_slist_alloc();

      if ( new_cookies )
      {
        walk = new_cookies;

        /* check if cookie_jar is created */
        if ( src->cookie_jar )
        {
          GST_INFO_OBJECT (src, "cookie jar exist. adding it to jar");
          for ( ; walk; walk = walk->next)
          {
            cookie = (SoupCookie*)walk->data;
            if ( cookie )
            {
              GST_INFO_OBJECT (src, "adding cookie to jar : %s - %s",
              cookie->name, cookie->value);
              soup_cookie_jar_add_cookie (src->cookie_jar, cookie);
            }
          }
        }
        else
        /* cooke jar not created yet. new cookies will be stored in temporal list
        * and will be added to jar when start() has called */
        {
          GST_INFO_OBJECT (src, "cookie jar not yet created. adding to temporal list");
          for ( ; walk; walk = walk->next )
          {
            src->cookie_list = g_slist_prepend(src->cookie_list,
            soup_cookie_copy(walk->data));
          }
        }
      }
      break;
    }
#endif
#ifdef GST_EXT_SOUP_MODIFICATION
    case PROP_AHS_STREAMING:
      src->is_ahs_streaming = g_value_get_boolean (value);
      break;
#endif
    case PROP_IS_LIVE:
      gst_base_src_set_live (GST_BASE_SRC (src), g_value_get_boolean (value));
      break;
    case PROP_USER_ID:
      if (src->user_id)
        g_free (src->user_id);
      src->user_id = g_value_dup_string (value);
      break;
    case PROP_USER_PW:
      if (src->user_pw)
        g_free (src->user_pw);
      src->user_pw = g_value_dup_string (value);
      break;
    case PROP_PROXY_ID:
      if (src->proxy_id)
        g_free (src->proxy_id);
      src->proxy_id = g_value_dup_string (value);
      break;
    case PROP_PROXY_PW:
      if (src->proxy_pw)
        g_free (src->proxy_pw);
      src->proxy_pw = g_value_dup_string (value);
      break;
    case PROP_TIMEOUT:
#ifdef GST_EXT_SOUP_MODIFICATION
      src->timeout = g_value_get_int (value);
#else
      src->timeout = g_value_get_uint (value);
#endif
      break;
    case PROP_EXTRA_HEADERS:{
      const GstStructure *s = gst_value_get_structure (value);

      if (src->extra_headers)
        gst_structure_free (src->extra_headers);

      src->extra_headers = s ? gst_structure_copy (s) : NULL;
      break;
    }
#ifdef SEEK_CHANGES
    case PROP_RANGESIZE:{
      src->range_size = g_value_get_int64 (value);
      break;
    }
#endif

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
done:
  return;
}

static void
gst_soup_http_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, src->location);
      break;
    case PROP_USER_AGENT:
      g_value_set_string (value, src->user_agent);
      break;
    case PROP_AUTOMATIC_REDIRECT:
      g_value_set_boolean (value, src->automatic_redirect);
      break;
    case PROP_PROXY:
      if (src->proxy == NULL)
        g_value_set_static_string (value, "");
      else {
        char *proxy = soup_uri_to_string (src->proxy, FALSE);

        g_value_set_string (value, proxy);
        g_free (proxy);
      }
      break;
    case PROP_COOKIES:
    {
#ifdef GST_EXT_SOUP_MODIFICATION
      GSList *cookie_list, *c;
      gchar **cookies, **array;

      cookies = NULL;
      if ((cookie_list = soup_cookie_jar_all_cookies (src->cookie_jar)) != NULL) {
        cookies = g_new0 (gchar *, g_slist_length(cookie_list) + 1);
        array = cookies;
        for (c = cookie_list; c; c = c->next) {
          *array++ = soup_cookie_to_set_cookie_header ((SoupCookie *)(c->data));
        }
        soup_cookies_free (cookie_list);
      }
      g_value_set_boxed (value, cookies);
#else
      g_value_set_boxed (value, g_strdupv (src->cookies));
#endif
      break;
    }
    case PROP_IS_LIVE:
      g_value_set_boolean (value, gst_base_src_is_live (GST_BASE_SRC (src)));
      break;
    case PROP_IRADIO_MODE:
      g_value_set_boolean (value, src->iradio_mode);
      break;
#ifdef GST_EXT_SOUP_MODIFICATION
    case PROP_AHS_STREAMING:
      g_value_set_boolean (value, src->is_ahs_streaming);
      break;
#endif
    case PROP_IRADIO_NAME:
      g_value_set_string (value, src->iradio_name);
      break;
    case PROP_IRADIO_GENRE:
      g_value_set_string (value, src->iradio_genre);
      break;
    case PROP_IRADIO_URL:
      g_value_set_string (value, src->iradio_url);
      break;
    case PROP_IRADIO_TITLE:
      g_value_set_string (value, src->iradio_title);
      break;
    case PROP_USER_ID:
      g_value_set_string (value, src->user_id);
      break;
    case PROP_USER_PW:
      g_value_set_string (value, src->user_pw);
      break;
    case PROP_PROXY_ID:
      g_value_set_string (value, src->proxy_id);
      break;
    case PROP_PROXY_PW:
      g_value_set_string (value, src->proxy_pw);
      break;
    case PROP_TIMEOUT:
#ifdef GST_EXT_SOUP_MODIFICATION
      g_value_set_int (value, src->timeout);
#else
      g_value_set_uint (value, src->timeout);
#endif
      break;
    case PROP_EXTRA_HEADERS:
      gst_value_set_structure (value, src->extra_headers);
      break;
#ifdef SEEK_CHANGES
    case PROP_RANGESIZE:
	  g_value_set_int64 (value, src->range_size);
	  break;
#endif
#ifdef GST_EXT_SOUP_MODIFICATION
    case PROP_CONTENT_SIZE:
      g_value_set_uint64 (value, src->content_len);
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gchar *
gst_soup_http_src_unicodify (const gchar * str)
{
  const gchar *env_vars[] = { "GST_ICY_TAG_ENCODING",
    "GST_TAG_ENCODING", NULL
  };

  return gst_tag_freeform_string_to_utf8 (str, -1, env_vars);
}

static void
gst_soup_http_src_cancel_message (GstSoupHTTPSrc * src)
{
  if (src->msg != NULL) {
    src->session_io_status = GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_CANCELLED;
    soup_session_cancel_message (src->session, src->msg, SOUP_STATUS_CANCELLED);
  }
  src->session_io_status = GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_IDLE;
  src->msg = NULL;
}

static void
gst_soup_http_src_queue_message (GstSoupHTTPSrc * src)
{

  soup_session_queue_message (src->session, src->msg,
      (SoupSessionCallback) gst_soup_http_src_response_cb, src);
  src->session_io_status = GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_QUEUED;
}

#ifdef GST_EXT_SOUP_MODIFICATION
static gboolean
gst_soup_http_src_add_range_header (GstSoupHTTPSrc * src, guint64 offset ,gchar *range)
#else
static gboolean
gst_soup_http_src_add_range_header (GstSoupHTTPSrc * src, guint64 offset)
#endif
{
  gchar buf[64];

  gint rc;

  soup_message_headers_remove (src->msg->request_headers, "Range");

#ifdef GST_EXT_SOUP_MODIFICATION

  if ((range != NULL) && src->is_ahs_streaming) {
    guint len, pos;
    gchar *str = NULL;
    guint64 first_byte_pos = 0, last_byte_pos = 0;

    str = range +6 ;//remove 'range='
    len = strlen(range);
    GST_DEBUG(" mediaRange : %s", str);

    /* read "-" */
    pos = strcspn (str, "-");
    if (pos >= len) {
      GST_TRACE ("pos %d >= len %d", pos, len);
      GST_WARNING ("pos >= len !");
      return FALSE;
    }
    /* read first_byte_pos */
    if (pos != 0) {
      if (sscanf (str, "%llu", &first_byte_pos) != 1) {
        GST_WARNING ("can not get first_byte_pos");
        return FALSE;
      }
    }
    /* read last_byte_pos */
    if (pos < (len - 1)) {
      if (sscanf (str + pos + 1, "%llu", &last_byte_pos) != 1) {
        GST_WARNING ("can not get last_byte_pos");
        return FALSE;
      }
    }

    GST_DEBUG_OBJECT(src,"- requestRange: %" G_GUINT64_FORMAT "-%" G_GUINT64_FORMAT,
        first_byte_pos, last_byte_pos);

    rc = g_snprintf (buf, sizeof (buf), "bytes=%" G_GUINT64_FORMAT "-%" G_GUINT64_FORMAT, first_byte_pos, last_byte_pos);
      if (rc > sizeof (buf) || rc < 0)
          return FALSE;
    soup_message_headers_append (src->msg->request_headers, "Range", buf);
	}

 else {

  if (!src->is_ahs_streaming) {
    /* Note : Some http server could not handle Range header in the middle of playing.
     *    Need to add Range header at first for seeking properly.
     */
    rc = g_snprintf (buf, sizeof (buf), "bytes=%" G_GUINT64_FORMAT "-", offset);
    if (rc > sizeof (buf) || rc < 0)
      return FALSE;
    soup_message_headers_append (src->msg->request_headers, "Range", buf);
  } else {
    if (offset) {
      rc = g_snprintf (buf, sizeof (buf), "bytes=%" G_GUINT64_FORMAT "-", offset);
      if (rc > sizeof (buf) || rc < 0)
        return FALSE;
      soup_message_headers_append (src->msg->request_headers, "Range", buf);
    }
  }
}
#else
  if (offset) {
    rc = g_snprintf (buf, sizeof (buf), "bytes=%" G_GUINT64_FORMAT "-", offset);
    if (rc > sizeof (buf) || rc < 0)
      return FALSE;
    soup_message_headers_append (src->msg->request_headers, "Range", buf);
  }
#endif
  src->read_position = offset;
  return TRUE;
}

static gboolean
_append_extra_header (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (user_data);
  const gchar *field_name = g_quark_to_string (field_id);
  gchar *field_content = NULL;

  if (G_VALUE_TYPE (value) == G_TYPE_STRING) {
    field_content = g_value_dup_string (value);
  } else {
    GValue dest = { 0, };

    g_value_init (&dest, G_TYPE_STRING);
    if (g_value_transform (value, &dest)) {
      field_content = g_value_dup_string (&dest);
    }
  }

  if (field_content == NULL) {
    GST_ERROR_OBJECT (src, "extra-headers field '%s' contains no value "
        "or can't be converted to a string", field_name);
    return FALSE;
  }

  GST_DEBUG_OBJECT (src, "Appending extra header: \"%s: %s\"", field_name,
      field_content);
  soup_message_headers_append (src->msg->request_headers, field_name,
      field_content);
#ifdef GST_EXT_SOUP_MODIFICATION
  if (!g_ascii_strcasecmp(field_name, "Cookie")) {
    SoupURI *uri = NULL;
    SoupCookie *cookie_parsed = NULL;

    if (strlen(field_content) > 0) {
      gchar *tmp_field = NULL;

      uri = soup_uri_new (src->location);

      tmp_field = strtok (field_content, ";");

      while (tmp_field != NULL) {
        GST_DEBUG_OBJECT (src, "field_content = %s", tmp_field);

        cookie_parsed = soup_cookie_parse(tmp_field, uri);
        GST_DEBUG_OBJECT (src, "cookie parsed = %p", cookie_parsed);

        if (src->cookie_jar)
          soup_cookie_jar_add_cookie (src->cookie_jar, cookie_parsed);

        tmp_field = strtok (NULL, ";");
      }
      soup_uri_free (uri);
    }
  }
#endif

  g_free (field_content);

  return TRUE;
}

static gboolean
_append_extra_headers (GQuark field_id, const GValue * value,
    gpointer user_data)
{
  if (G_VALUE_TYPE (value) == GST_TYPE_ARRAY) {
    guint n = gst_value_array_get_size (value);
    guint i;

    for (i = 0; i < n; i++) {
      const GValue *v = gst_value_array_get_value (value, i);

      if (!_append_extra_header (field_id, v, user_data))
        return FALSE;
    }
  } else if (G_VALUE_TYPE (value) == GST_TYPE_LIST) {
    guint n = gst_value_list_get_size (value);
    guint i;

    for (i = 0; i < n; i++) {
      const GValue *v = gst_value_list_get_value (value, i);

      if (!_append_extra_header (field_id, v, user_data))
        return FALSE;
    }
  } else {
    return _append_extra_header (field_id, value, user_data);
  }

  return TRUE;
}


static gboolean
gst_soup_http_src_add_extra_headers (GstSoupHTTPSrc * src)
{
  if (!src->extra_headers)
    return TRUE;

  return gst_structure_foreach (src->extra_headers, _append_extra_headers, src);
}


static void
gst_soup_http_src_session_unpause_message (GstSoupHTTPSrc * src)
{
  soup_session_unpause_message (src->session, src->msg);
}

static void
gst_soup_http_src_session_pause_message (GstSoupHTTPSrc * src)
{
  soup_session_pause_message (src->session, src->msg);
}

static void
gst_soup_http_src_session_close (GstSoupHTTPSrc * src)
{
  if (src->session) {
    soup_session_abort (src->session);  /* This unrefs the message. */
    g_object_unref (src->session);
    src->session = NULL;
    src->msg = NULL;
  }
}

static void
gst_soup_http_src_authenticate_cb (SoupSession * session, SoupMessage * msg,
    SoupAuth * auth, gboolean retrying, GstSoupHTTPSrc * src)
{
  if (!retrying) {
    /* First time authentication only, if we fail and are called again with retry true fall through */
    if (msg->status_code == SOUP_STATUS_UNAUTHORIZED) {
      if (src->user_id && src->user_pw)
        soup_auth_authenticate (auth, src->user_id, src->user_pw);
    } else if (msg->status_code == SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED) {
      if (src->proxy_id && src->proxy_pw)
        soup_auth_authenticate (auth, src->proxy_id, src->proxy_pw);
    }
  }
}

static void
gst_soup_http_src_headers_foreach (const gchar * name, const gchar * val,
    gpointer src)
{
  GST_DEBUG_OBJECT (src, " %s: %s", name, val);

#ifdef GST_EXT_SOUP_MODIFICATION
  if (g_ascii_strcasecmp (name, "Set-Cookie") == 0)
  {
    if (val)
    {
      gboolean bret = FALSE;
      GstStructure *s = NULL;
      GstSoupHTTPSrc * tmp = src;
      SoupURI *uri;

      uri =  soup_uri_new (tmp->location);

      /* post current bandwith & uri to application */
      s = gst_structure_new ("cookies",
            "updated-cookie", G_TYPE_STRING, val,
            "updated-url", G_TYPE_STRING, tmp->location, NULL);
      bret = gst_element_post_message (GST_ELEMENT_CAST (src), gst_message_new_element (GST_OBJECT_CAST (src), s));
      soup_cookie_jar_set_cookie (tmp->cookie_jar,  uri, val);
      soup_uri_free (uri);

      GST_INFO_OBJECT (src, "request url [%s], posted cookies [%s] msg and returned = %d", tmp->location, val, bret);
    }
  }
#endif
}

static void
gst_soup_http_src_got_headers_cb (SoupMessage * msg, GstSoupHTTPSrc * src)
{
  const char *value;
  GstTagList *tag_list;
  GstBaseSrc *basesrc;
  guint64 newsize;
#ifdef SEEK_CHANGES
  goffset start = 0, end = 0, total_length = 0;
#endif
  GHashTable *params = NULL;

  GST_DEBUG_OBJECT (src, "got headers:");

#ifdef GST_EXT_SOUP_MODIFICATION
  src->retry_timestamp = 0;
#endif

  soup_message_headers_foreach (msg->response_headers,
      gst_soup_http_src_headers_foreach, src);

  if (msg->status_code == 407 && src->proxy_id && src->proxy_pw)
    return;

  if (src->automatic_redirect && SOUP_STATUS_IS_REDIRECTION (msg->status_code)) {
#ifdef GST_EXT_SOUP_MODIFICATION
    value = soup_message_headers_get (msg->response_headers, "Location");
    gst_soup_http_src_set_location (src, value);
    GST_DEBUG_OBJECT (src, "%u redirect to \"%s\"", msg->status_code, value);
#else
    GST_DEBUG_OBJECT (src, "%u redirect to \"%s\"", msg->status_code,
        soup_message_headers_get (msg->response_headers, "Location"));
#endif
    return;
  }

  if (msg->status_code == SOUP_STATUS_UNAUTHORIZED)
    return;

  src->session_io_status = GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING;

#ifdef GST_EXT_SOUP_MODIFICATION
  if ((value =
          soup_message_headers_get (msg->response_headers,
              "contentFeatures.dlna.org")) != NULL) {
    gchar **token = NULL;
    gchar **ptr = NULL;

    token = g_strsplit (value, ";", 0);

    for (ptr = token ; *ptr ; ptr++)
    {
      if (strlen(*ptr) > 0)
      {
        if (strstr(g_ascii_strup(*ptr, strlen(*ptr)), "DLNA.ORG_OP") != NULL)
        {
          gchar *op_code = NULL;

          op_code = strchr(*ptr, '=');
          if (op_code)
          {
            op_code++;

            src->op_code = (atoi(op_code)/10 <<1) | (atoi(op_code)%10);
            GST_DEBUG_OBJECT (src, "dlna op code: %s (0x%X)", op_code, src->op_code);
          }
        }
      }
    }
    g_strfreev (token);
  }

  if ((value = soup_message_headers_get (msg->response_headers, "Content-Duration")) != NULL) {
    src->duration = atoi(value) * G_GINT64_CONSTANT (1000000);
    GST_DEBUG_OBJECT (src, "Content Duration is %"GST_TIME_FORMAT, GST_TIME_ARGS (src->duration));
  }

#endif

#ifdef USE_SAMSUNG_LINK
  if ((value = soup_message_headers_get (msg->response_headers, "X-ASP-DURATION-TIME")) != NULL) {
    src->is_x_asp = TRUE;
    src->duration = atoi(value) * G_GINT64_CONSTANT (1000000);
    GST_DEBUG_OBJECT (src, "X-ASP-DURATION-TIME is %"GST_TIME_FORMAT, GST_TIME_ARGS (src->duration));
  }
#endif

  /* Parse Content-Length. */
  if (soup_message_headers_get_encoding (msg->response_headers) ==
      SOUP_ENCODING_CONTENT_LENGTH) {
    newsize = src->request_position +
        soup_message_headers_get_content_length (msg->response_headers);
    if (!src->have_size || (src->content_size != newsize)) {
      src->content_size = newsize;
#ifdef SEEK_CHANGES
    if(!src->file_size)
      src->file_size = newsize;
#endif
      src->have_size = TRUE;
      src->seekable = TRUE;
      GST_DEBUG_OBJECT (src, "content size = %" G_GUINT64_FORMAT, src->content_size);
#ifdef GST_EXT_SOUP_MODIFICATION
      src->content_len = src->content_size;
#endif
      basesrc = GST_BASE_SRC_CAST (src);
      gst_segment_set_duration (&basesrc->segment, GST_FORMAT_BYTES,
          src->content_size);
      gst_element_post_message (GST_ELEMENT (src),
          gst_message_new_duration (GST_OBJECT (src), GST_FORMAT_BYTES,
              src->content_size));
    }
#ifdef SEEK_CHANGES
    soup_message_headers_get_content_range(msg->response_headers, &start, &end, &total_length);
    if(total_length > 0)
    {
      src->file_size = total_length;
      GST_DEBUG_OBJECT (src, "total size = %" G_GUINT64_FORMAT, src->file_size);
      basesrc = GST_BASE_SRC_CAST (src);
      gst_segment_set_duration (&basesrc->segment, GST_FORMAT_BYTES,
          src->file_size);
      gst_element_post_message (GST_ELEMENT (src),
          gst_message_new_duration (GST_OBJECT (src), GST_FORMAT_BYTES,
              src->file_size));
    }
#endif
  }
#ifdef GST_EXT_SOUP_MODIFICATION
  else if ((soup_message_headers_get_encoding (msg->response_headers) ==
            SOUP_ENCODING_CHUNKED) && (src->op_code & DLNA_OP_BYTE_SEEK)) {
    soup_message_headers_get_content_range(msg->response_headers, &start, &end, &total_length);
    if(total_length > 0)
    {
      src->have_size = TRUE;
      src->seekable = TRUE;

      src->content_size = src->content_len = src->file_size = total_length;

      GST_DEBUG_OBJECT (src, "total size = %" G_GUINT64_FORMAT, src->file_size);
      basesrc = GST_BASE_SRC_CAST (src);
      gst_segment_set_duration (&basesrc->segment, GST_FORMAT_BYTES,
          src->file_size);
      gst_element_post_message (GST_ELEMENT (src),
          gst_message_new_duration (GST_OBJECT (src), GST_FORMAT_BYTES,
              src->file_size));
    }
  }
#endif

  /* Icecast stuff */
  tag_list = gst_tag_list_new ();

  if ((value =
          soup_message_headers_get (msg->response_headers,
              "icy-metaint")) != NULL) {
    gint icy_metaint = atoi (value);

    GST_DEBUG_OBJECT (src, "icy-metaint: %s (parsed: %d)", value, icy_metaint);
    if (icy_metaint > 0) {
      if (src->src_caps)
        gst_caps_unref (src->src_caps);

      src->src_caps = gst_caps_new_simple ("application/x-icy",
          "metadata-interval", G_TYPE_INT, icy_metaint, NULL);
    }
  }
#ifdef SEEK_CHANGES
  if ((msg->status_code == 200) && (value=soup_message_headers_get (msg->response_headers,"Accept-Ranges")) != NULL) {
    GST_DEBUG_OBJECT (src, "Accept-ranges: %s ", value);
    if(strstr(value,"none")!=NULL) {
      src->seekable=FALSE;
      GST_DEBUG_OBJECT (src, "server is not seekable");
    }
    else
      src->seekable=TRUE;
  }
  else if(msg->status_code == 200) {
    src->seekable=FALSE;
    GST_DEBUG_OBJECT (src, "server is not seekable");
  }
#endif
  if ((value =
          soup_message_headers_get_content_type (msg->response_headers,
              &params)) != NULL) {
    GST_DEBUG_OBJECT (src, "Content-Type: %s", value);

    if (g_ascii_strcasecmp (value, "audio/L16") == 0) {
      gint channels = 2;
      gint rate = 44100;
      char *param;

      if (src->src_caps)
        gst_caps_unref (src->src_caps);

      param = g_hash_table_lookup (params, "channels");
      if (param != NULL)
        channels = atol (param);

      param = g_hash_table_lookup (params, "rate");
      if (param != NULL)
        rate = atol (param);

      src->src_caps = gst_caps_new_simple ("audio/x-raw-int",
          "channels", G_TYPE_INT, channels,
          "rate", G_TYPE_INT, rate,
          "width", G_TYPE_INT, 16,
          "depth", G_TYPE_INT, 16,
          "signed", G_TYPE_BOOLEAN, TRUE,
          "endianness", G_TYPE_INT, G_BIG_ENDIAN, NULL);
    } else {
      /* Set the Content-Type field on the caps */
      if (src->src_caps)
        gst_caps_set_simple (src->src_caps, "content-type", G_TYPE_STRING,
            value, NULL);
    }
  }

  if (params != NULL)
    g_hash_table_destroy (params);

  if ((value =
          soup_message_headers_get (msg->response_headers,
              "icy-name")) != NULL) {
    g_free (src->iradio_name);
    src->iradio_name = gst_soup_http_src_unicodify (value);
    if (src->iradio_name) {
      g_object_notify (G_OBJECT (src), "iradio-name");
      gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE, GST_TAG_ORGANIZATION,
          src->iradio_name, NULL);
    }
  }
  if ((value =
          soup_message_headers_get (msg->response_headers,
              "icy-genre")) != NULL) {
    g_free (src->iradio_genre);
    src->iradio_genre = gst_soup_http_src_unicodify (value);
    if (src->iradio_genre) {
      g_object_notify (G_OBJECT (src), "iradio-genre");
      gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE, GST_TAG_GENRE,
          src->iradio_genre, NULL);
    }
  }
  if ((value = soup_message_headers_get (msg->response_headers, "icy-url"))
      != NULL) {
    g_free (src->iradio_url);
    src->iradio_url = gst_soup_http_src_unicodify (value);
    if (src->iradio_url) {
      g_object_notify (G_OBJECT (src), "iradio-url");
      gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE, GST_TAG_LOCATION,
          src->iradio_url, NULL);
    }
  }
  if (!gst_tag_list_is_empty (tag_list)) {
    GST_DEBUG_OBJECT (src,
        "calling gst_element_found_tags with %" GST_PTR_FORMAT, tag_list);
    gst_element_found_tags (GST_ELEMENT_CAST (src), tag_list);
  } else {
    gst_tag_list_free (tag_list);
  }

  /* Handle HTTP errors. */
  gst_soup_http_src_parse_status (msg, src);

#ifdef GST_EXT_SOUP_MODIFICATION
  if(src->is_ahs_streaming && msg->status_code == 404){
    GST_WARNING_OBJECT (src,"Received 404 error while HLS streaming.");
    src->content_len = 0;
  }
#endif
  /* Check if Range header was respected. */
  if (src->ret == GST_FLOW_CUSTOM_ERROR &&
      src->read_position && msg->status_code != SOUP_STATUS_PARTIAL_CONTENT) {
    src->seekable = FALSE;
    GST_ELEMENT_ERROR (src, RESOURCE, SEEK,
        (_("Server does not support seeking.")),
        ("Server does not accept Range HTTP header, URL: %s", src->location));
    src->ret = GST_FLOW_ERROR;
  }
}

/* Have body. Signal EOS. */
static void
gst_soup_http_src_got_body_cb (SoupMessage * msg, GstSoupHTTPSrc * src)
{
  if (G_UNLIKELY (msg != src->msg)) {
    GST_DEBUG_OBJECT (src, "got body, but not for current message");
    return;
  }
  if (G_UNLIKELY (src->session_io_status !=
          GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING)) {
    /* Probably a redirect. */
    return;
  }
  GST_DEBUG_OBJECT (src, "got body");
  src->ret = GST_FLOW_UNEXPECTED;
  src->have_body = TRUE;

  /* no need to interrupt the message here, we do it on the
   * finished_cb anyway if needed. And getting the body might mean
   * that the connection was hang up before finished. This happens when
   * the pipeline is stalled for too long (long pauses during playback).
   * Best to let it continue from here and pause because it reached the
   * final bytes based on content_size or received an out of range error */
}

/* Finished. Signal EOS. */
static void
gst_soup_http_src_finished_cb (SoupMessage * msg, GstSoupHTTPSrc * src)
{
  if (G_UNLIKELY (msg != src->msg)) {
    GST_DEBUG_OBJECT (src, "finished, but not for current message");
    return;
  }

  GST_WARNING_OBJECT (src, "finished : %d", src->session_io_status);
  src->ret = GST_FLOW_UNEXPECTED;
  if (src->session_io_status == GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_CANCELLED) {
    /* gst_soup_http_src_cancel_message() triggered this; probably a seek
     * that occurred in the QUEUEING state; i.e. before the connection setup
     * was complete. Do nothing */
  } else if (src->session_io_status ==
      GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING && src->read_position > 0 &&
      (src->have_size || src->read_position < src->content_size)) {
    /* The server disconnected while streaming. Reconnect and seeking to the
     * last location. */
    src->retry = TRUE;
    src->ret = GST_FLOW_CUSTOM_ERROR;
  } else if (G_UNLIKELY (src->session_io_status !=
          GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING)) {
#ifdef GST_EXT_SOUP_MODIFICATION
    if (msg->status_code != SOUP_STATUS_OK) {
      if ((src->read_position <= 0) || (src->timeout == 0)) {
        /* FIXME: reason_phrase is not translated, add proper error message */
        GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND,
            ("%s", msg->reason_phrase),
            ("libsoup status code %d", msg->status_code));
      } else {
        GstClockTime current = 0, elapsed = 0;

        GST_WARNING_OBJECT(src, "session timout:%d, timeout:%d", src->session_timeout, src->timeout);

        /* Assumption : soup error will be occurred after session timeout. */
        if (src->retry_timestamp == 0) {
          GstClockTime retry_timestamp = 0;
          retry_timestamp = gst_util_get_timestamp();
          src->retry_timestamp = retry_timestamp;
        }

        current = gst_util_get_timestamp ();
        elapsed = GST_CLOCK_DIFF (src->retry_timestamp, current);

        if ((src->timeout == DEFAULT_RETRY_TIMEOUT) || (elapsed <= (src->timeout*GST_SECOND))) {

          if (src->timeout != DEFAULT_RETRY_TIMEOUT) {
            guint new_session_timeout = src->timeout - (elapsed/GST_SECOND);

            if (new_session_timeout < MIN_SESSION_TIMEOUT)
              new_session_timeout = MIN_SESSION_TIMEOUT;
            else if (new_session_timeout > DEFAULT_SESSION_TIMEOUT)
              new_session_timeout = DEFAULT_SESSION_TIMEOUT;

            if (new_session_timeout != src->session_timeout) {
              src->session_timeout = new_session_timeout;
              g_object_set(src->session, SOUP_SESSION_TIMEOUT, src->session_timeout, NULL);
            }
          }

          GST_WARNING_OBJECT(src, "retry. session timeout : %d", src->session_timeout);

          src->retry = TRUE;
          src->ret = GST_FLOW_CUSTOM_ERROR;
        } else {
          src->timeout = 0;
          GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND,
              ("%s", msg->reason_phrase),
              ("libsoup status code %d", msg->status_code));
        }
      }
    }
#else
    if (msg->status_code != SOUP_STATUS_OK){
      /* FIXME: reason_phrase is not translated, add proper error message */
      GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND,
          ("%s", msg->reason_phrase),
          ("libsoup status code %d", msg->status_code));
    }
#endif
  }
  if (src->loop)
    g_main_loop_quit (src->loop);
}

/* Buffer lifecycle management.
 *
 * gst_soup_http_src_create() runs the GMainLoop for this element, to let
 * Soup take control.
 * A GstBuffer is allocated in gst_soup_http_src_chunk_allocator() and
 * associated with a SoupBuffer.
 * Soup reads HTTP data in the GstBuffer's data buffer.
 * The gst_soup_http_src_got_chunk_cb() is then called with the SoupBuffer.
 * That sets gst_soup_http_src_create()'s return argument to the GstBuffer,
 * increments its refcount (to 2), pauses the flow of data from the HTTP
 * source to prevent gst_soup_http_src_got_chunk_cb() from being called
 * again and breaks out of the GMainLoop.
 * Because the SOUP_MESSAGE_OVERWRITE_CHUNKS flag is set, Soup frees the
 * SoupBuffer and calls gst_soup_http_src_chunk_free(), which decrements the
 * refcount (to 1).
 * gst_soup_http_src_create() returns the GstBuffer. It will be freed by a
 * downstream element.
 * If Soup fails to read HTTP data, it does not call
 * gst_soup_http_src_got_chunk_cb(), but still frees the SoupBuffer and
 * calls gst_soup_http_src_chunk_free(), which decrements the GstBuffer's
 * refcount to 0, freeing it.
 */

static void
gst_soup_http_src_chunk_free (gpointer gstbuf)
{
  gst_buffer_unref (GST_BUFFER_CAST (gstbuf));
}

static SoupBuffer *
gst_soup_http_src_chunk_allocator (SoupMessage * msg, gsize max_len,
    gpointer user_data)
{
  GstSoupHTTPSrc *src = (GstSoupHTTPSrc *) user_data;
  GstBaseSrc *basesrc = GST_BASE_SRC_CAST (src);
  GstBuffer *gstbuf;
  SoupBuffer *soupbuf;
  gsize length;
  GstFlowReturn rc;

  if (max_len)
    length = MIN (basesrc->blocksize, max_len);
  else
    length = basesrc->blocksize;
  GST_DEBUG_OBJECT (src, "alloc %" G_GSIZE_FORMAT " bytes <= %" G_GSIZE_FORMAT,
      length, max_len);


  rc = gst_pad_alloc_buffer (GST_BASE_SRC_PAD (basesrc),
      GST_BUFFER_OFFSET_NONE, length,
      src->src_caps ? src->src_caps :
      GST_PAD_CAPS (GST_BASE_SRC_PAD (basesrc)), &gstbuf);
  if (G_UNLIKELY (rc != GST_FLOW_OK)) {
    /* Failed to allocate buffer. Stall SoupSession and return error code
     * to create(). */
    src->ret = rc;
    g_main_loop_quit (src->loop);
    return NULL;
  }

  soupbuf = soup_buffer_new_with_owner (GST_BUFFER_DATA (gstbuf), length,
      gstbuf, gst_soup_http_src_chunk_free);

  return soupbuf;
}

static void
gst_soup_http_src_got_chunk_cb (SoupMessage * msg, SoupBuffer * chunk,
    GstSoupHTTPSrc * src)
{
  GstBaseSrc *basesrc;
  guint64 new_position;

  if (G_UNLIKELY (msg != src->msg)) {
    GST_DEBUG_OBJECT (src, "got chunk, but not for current message");
    return;
  }

  src->have_body = FALSE;
  if (G_UNLIKELY (src->session_io_status !=
          GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING)) {
    /* Probably a redirect. */
    return;
  }
  basesrc = GST_BASE_SRC_CAST (src);
  GST_DEBUG_OBJECT (src, "got chunk of %" G_GSIZE_FORMAT " bytes",
      chunk->length);

  /* Extract the GstBuffer from the SoupBuffer and set its fields. */
  *src->outbuf = GST_BUFFER_CAST (soup_buffer_get_owner (chunk));

  GST_BUFFER_SIZE (*src->outbuf) = chunk->length;
  GST_BUFFER_OFFSET (*src->outbuf) = basesrc->segment.last_stop;

  gst_buffer_set_caps (*src->outbuf,
      (src->src_caps) ? src->src_caps :
      GST_PAD_CAPS (GST_BASE_SRC_PAD (basesrc)));

  gst_buffer_ref (*src->outbuf);

  new_position = src->read_position + chunk->length;
  if (G_LIKELY (src->request_position == src->read_position))
    src->request_position = new_position;
  src->read_position = new_position;

  src->ret = GST_FLOW_OK;
  g_main_loop_quit (src->loop);
  gst_soup_http_src_session_pause_message (src);
}

static void
gst_soup_http_src_response_cb (SoupSession * session, SoupMessage * msg,
    GstSoupHTTPSrc * src)
{
  if (G_UNLIKELY (msg != src->msg)) {
    GST_DEBUG_OBJECT (src, "got response %d: %s, but not for current message",
        msg->status_code, msg->reason_phrase);
    return;
  }
  if (G_UNLIKELY (src->session_io_status !=
          GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING)
      && SOUP_STATUS_IS_REDIRECTION (msg->status_code)) {
    /* Ignore redirections. */
    return;
  }
  GST_WARNING_OBJECT (src, "got response %d: %s", msg->status_code,
      msg->reason_phrase);
  if (src->session_io_status == GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING &&
      src->read_position > 0) {
    /* The server disconnected while streaming. Reconnect and seeking to the
     * last location. */
    src->retry = TRUE;
  } else
    gst_soup_http_src_parse_status (msg, src);
  /* The session's SoupMessage object expires after this callback returns. */
  src->msg = NULL;
  g_main_loop_quit (src->loop);
}

#define SOUP_HTTP_SRC_ERROR(src,soup_msg,cat,code,error_message)     \
  GST_ELEMENT_ERROR ((src), cat, code, ("%s", error_message),        \
      ("%s (%d), URL: %s", (soup_msg)->reason_phrase,                \
          (soup_msg)->status_code, (src)->location));

static void
gst_soup_http_src_parse_status (SoupMessage * msg, GstSoupHTTPSrc * src)
{
  if (SOUP_STATUS_IS_TRANSPORT_ERROR (msg->status_code)) {
    switch (msg->status_code) {
      case SOUP_STATUS_CANT_RESOLVE:
      case SOUP_STATUS_CANT_RESOLVE_PROXY:
#ifdef GST_EXT_SOUP_MODIFICATION
        if (src->retry != TRUE)
#endif
        {
          SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, NOT_FOUND,
              _("Could not resolve server name."));
          src->ret = GST_FLOW_ERROR;
        }
        break;
      case SOUP_STATUS_CANT_CONNECT:
      case SOUP_STATUS_CANT_CONNECT_PROXY:
#ifdef GST_EXT_SOUP_MODIFICATION
        if (src->retry != TRUE)
#endif
        {
          SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, OPEN_READ,
              _("Could not establish connection to server."));
          src->ret = GST_FLOW_ERROR;
        }
        break;
      case SOUP_STATUS_SSL_FAILED:
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, OPEN_READ,
            _("Secure connection setup failed."));
        src->ret = GST_FLOW_ERROR;
        break;
      case SOUP_STATUS_IO_ERROR:
#ifdef GST_EXT_SOUP_MODIFICATION
        if (src->retry != TRUE)
#endif
        {
          SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, READ,
              _("A network error occured, or the server closed the connection "
                  "unexpectedly."));
          src->ret = GST_FLOW_ERROR;
        }
        break;
      case SOUP_STATUS_MALFORMED:
        SOUP_HTTP_SRC_ERROR (src, msg, RESOURCE, READ,
            _("Server sent bad data."));
        src->ret = GST_FLOW_ERROR;
        break;
      case SOUP_STATUS_CANCELLED:
        /* No error message when interrupted by program. */
        break;
    }
  } else if (SOUP_STATUS_IS_CLIENT_ERROR (msg->status_code) ||
      SOUP_STATUS_IS_REDIRECTION (msg->status_code) ||
      SOUP_STATUS_IS_SERVER_ERROR (msg->status_code)) {
    /* Report HTTP error. */
    /* when content_size is unknown and we have just finished receiving
     * a body message, requests that go beyond the content limits will result
     * in an error. Here we convert those to EOS */
    if (msg->status_code == SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE &&
        src->have_body && src->have_size) {
      GST_WARNING_OBJECT (src, "Requested range out of limits and received full "
          "body, returning EOS");
      src->ret = GST_FLOW_UNEXPECTED;
      return;
    }

    /* FIXME: reason_phrase is not translated and not suitable for user
     * error dialog according to libsoup documentation.
     * FIXME: error code (OPEN_READ vs. READ) should depend on http status? */
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ,
        ("%s", msg->reason_phrase),
        ("%s (%d), URL: %s", msg->reason_phrase, msg->status_code,
            src->location));
    src->ret = GST_FLOW_ERROR;
  }
}

static gboolean
gst_soup_http_src_build_message (GstSoupHTTPSrc * src)
{
  gchar *user_agent = DEFAULT_USER_AGENT;

  src->msg = soup_message_new (SOUP_METHOD_GET, src->location);
  if (!src->msg) {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ,
        ("Error parsing URL."), ("URL: %s", src->location));
    return FALSE;
  }
  src->session_io_status = GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_IDLE;
  soup_message_headers_append (src->msg->request_headers, "Connection",
      "close");
  if (src->iradio_mode) {
    soup_message_headers_append (src->msg->request_headers, "icy-metadata",
        "1");
  }

  if (src->user_agent) {
    user_agent = src->user_agent;
    GST_DEBUG_OBJECT(src, "Set User-Agent = %s", user_agent);
  }

  soup_message_headers_append (src->msg->request_headers, "User-Agent",
        user_agent);

#ifdef SEEK_CHANGES
  soup_message_headers_append (src->msg->request_headers, "Accept-Ranges","bytes");
#endif


#ifdef GST_EXT_SOUP_MODIFICATION
  if (src->cookie_jar) {
    GSList *cookie_list, *c;
    gchar **cookies;
    gchar *header;

    SoupURI *uri = NULL;
    SoupCookie *cookie;
    uri = soup_uri_new (src->location);

    if ((cookie_list = soup_cookie_jar_all_cookies (src->cookie_jar)) != NULL) {
      cookies = g_new0 (gchar *, g_slist_length(cookie_list) + 1);
      for (c = cookie_list; c; c = c->next) {
        cookie = (SoupCookie *)c->data;
        if (soup_cookie_applies_to_uri(cookie, uri)) {
          header = soup_cookie_to_cookie_header (cookie);
          soup_message_headers_append (src->msg->request_headers, "Cookie",
          header);
          g_free (header);
        }
      }
    }
    soup_cookies_free (cookie_list);
  }
#else
  gchar **cookie;
  if (src->cookies) {
    for (cookie = src->cookies; *cookie != NULL; cookie++) {
      soup_message_headers_append (src->msg->request_headers, "Cookie",
        *cookie);
    }
  }
#endif

  soup_message_headers_append (src->msg->request_headers,
      "transferMode.dlna.org", "Streaming");
  src->retry = FALSE;

  g_signal_connect (src->msg, "got_headers",
      G_CALLBACK (gst_soup_http_src_got_headers_cb), src);
  g_signal_connect (src->msg, "got_body",
      G_CALLBACK (gst_soup_http_src_got_body_cb), src);
  g_signal_connect (src->msg, "finished",
      G_CALLBACK (gst_soup_http_src_finished_cb), src);
  g_signal_connect (src->msg, "got_chunk",
      G_CALLBACK (gst_soup_http_src_got_chunk_cb), src);
  soup_message_set_flags (src->msg, SOUP_MESSAGE_OVERWRITE_CHUNKS |
      (src->automatic_redirect ? 0 : SOUP_MESSAGE_NO_REDIRECT));
  soup_message_set_chunk_allocator (src->msg,
      gst_soup_http_src_chunk_allocator, src, NULL);

#ifdef USE_SAMSUNG_LINK
  if(src->is_x_asp && src->seek_time_position >= 0)
  {
    gchar seek_time[64];
    gint rc = 0;
    rc = g_snprintf(seek_time, 64,  "%"G_GINT64_FORMAT, src->seek_time_position);
    if (rc > 0)
    {
      soup_message_headers_append (src->msg->request_headers,
      "X-ASP-SEEK-TIME", seek_time);
    }
    src->seek_time_position = (gint64)-1;
  }
#endif

#ifdef SEEK_CHANGES
  //gst_soup_http_src_add_range_header (src, src->request_position);
  if(src->range_size > 0)
    soup_message_headers_set_range(src->msg->request_headers, src->request_position, (src->request_position+src->range_size-1));
  else {
#ifdef GST_EXT_SOUP_MODIFICATION
    gchar *dash_mediaRange = NULL;
    dash_mediaRange = strstr (src->location, "range=");
      if (dash_mediaRange)
        gst_soup_http_src_add_range_header (src, src->request_position,dash_mediaRange);
      else
        gst_soup_http_src_add_range_header (src, src->request_position,NULL);
#else
        gst_soup_http_src_add_range_header (src, src->request_position);
#endif
    gst_soup_http_src_add_extra_headers (src);
    GST_DEBUG_OBJECT (src, "request headers:");
    soup_message_headers_foreach (src->msg->request_headers,gst_soup_http_src_headers_foreach, src);
  }
#else

#ifdef GST_EXT_SOUP_MODIFICATION
  gst_soup_http_src_add_range_header (src, src->request_position,NULL);
#else
  gst_soup_http_src_add_range_header (src, src->request_position);
#endif

  gst_soup_http_src_add_extra_headers (src);

  GST_DEBUG_OBJECT (src, "request headers:");
  soup_message_headers_foreach (src->msg->request_headers,
      gst_soup_http_src_headers_foreach, src);
#endif

  return TRUE;
}

static GstFlowReturn
gst_soup_http_src_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (psrc);

  if (src->msg && (src->request_position != src->read_position)) {
#ifdef SEEK_CHANGES
    if (src->file_size != 0 && src->request_position >= src->file_size) {
#else
    if (src->content_size != 0 && src->request_position >= src->content_size) {
#endif
      GST_WARNING_OBJECT (src, "Seeking behind the end of file -- EOS");
      return GST_FLOW_UNEXPECTED;
    } else if (src->session_io_status ==
        GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_IDLE) {
      #ifdef GST_EXT_SOUP_MODIFICATION
      gst_soup_http_src_add_range_header (src, src->request_position,NULL);
	  #else
      gst_soup_http_src_add_range_header (src, src->request_position);
	  #endif
    } else {
      GST_DEBUG_OBJECT (src, "Seek from position %" G_GUINT64_FORMAT
          " to %" G_GUINT64_FORMAT ": requeueing connection request",
          src->read_position, src->request_position);
#ifndef SEEK_CHANGES
      gst_soup_http_src_cancel_message (src);
#endif
    }
  }
#ifdef SEEK_CHANGES
  if(src->msg  && src->seeked) {
    GST_DEBUG_OBJECT (src, "seeking to offset start %llu end %llu", src->request_position, (src->request_position+src->range_size-1));
    if(src->msg) {
      soup_session_cancel_message (src->session, src->msg, SOUP_STATUS_OK);
      src->msg = NULL;
      if (!gst_soup_http_src_build_message (src))
        return GST_FLOW_ERROR;
    }
    soup_session_queue_message (src->session, src->msg, (SoupSessionCallback) gst_soup_http_src_response_cb, src);
    src->session_io_status = GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_QUEUED;
    src->read_position = src->request_position;
    if(src->range_size > 0) src->content_size = src->request_position+src->range_size-1;
    else src->content_size = src->file_size;
  }
  src->seeked = FALSE;
#endif
  if (!src->msg)
    if (!gst_soup_http_src_build_message (src))
      return GST_FLOW_ERROR;

  src->ret = GST_FLOW_CUSTOM_ERROR;
  src->outbuf = outbuf;
  do {
    if (src->interrupted) {
      GST_DEBUG_OBJECT (src, "interrupted");
      break;
    }
    if (src->retry) {
      GST_WARNING_OBJECT (src, "Reconnecting");
#ifdef GST_EXT_SOUP_MODIFICATION
      g_usleep (1000000);
#endif
      if (!gst_soup_http_src_build_message (src))
        return GST_FLOW_ERROR;
      src->retry = FALSE;
      continue;
    }
    if (!src->msg) {
      GST_DEBUG_OBJECT (src, "EOS reached");
      break;
    }

    switch (src->session_io_status) {
      case GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_IDLE:
        GST_DEBUG_OBJECT (src, "Queueing connection request");
        gst_soup_http_src_queue_message (src);
        break;
      case GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_QUEUED:
        break;
      case GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_RUNNING:
        gst_soup_http_src_session_unpause_message (src);
        break;
      case GST_SOUP_HTTP_SRC_SESSION_IO_STATUS_CANCELLED:
        /* Impossible. */
        break;
    }

    if (src->ret == GST_FLOW_CUSTOM_ERROR)
      g_main_loop_run (src->loop);
  } while (src->ret == GST_FLOW_CUSTOM_ERROR);

  if (src->ret == GST_FLOW_CUSTOM_ERROR)
    src->ret = GST_FLOW_UNEXPECTED;
  return src->ret;
}

static gboolean
gst_soup_http_src_start (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);
#ifdef GST_EXT_SOUP_MODIFICATION
  char **array = NULL;
  SoupURI *base_uri;
#endif

  GST_DEBUG_OBJECT (src, "start(\"%s\")", src->location);

#ifdef GST_EXT_SOUP_MODIFICATION
/* For videohub DRM dash uri which has custom data */
  gchar *has_custom_data = strstr(src->location,"[]<>");
  gchar *has_mpd= strstr(src->location,".mpd");

  if (src->videohub_dash_uri){
    g_free(src->videohub_dash_uri);
    src->videohub_dash_uri = NULL;
  }

  if ( has_custom_data !=NULL && has_mpd !=NULL ) {
    GST_DEBUG_OBJECT(src,"The uri has custom data..");

    /* keep uri which includes custom data for forwarding it to qtdemux */
    src->videohub_dash_uri = g_strdup(src->location);

    /* make normal uri without custom data for requesting to server */
    gchar *videohub_dash_normal_uri = NULL;
    videohub_dash_normal_uri = strtok(src->location,"[]<>");
    src->location = NULL;
    src->location = videohub_dash_normal_uri;
    GST_DEBUG_OBJECT (src, "Videohub normal uri(\"%s\")", src->location);
  }
#endif

  if (!src->location) {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (_("No URL set.")),
        ("Missing location property"));
    return FALSE;
  }

  src->context = g_main_context_new ();

  src->loop = g_main_loop_new (src->context, TRUE);
  if (!src->loop) {
    GST_ELEMENT_ERROR (src, LIBRARY, INIT,
        (NULL), ("Failed to start GMainLoop"));
    g_main_context_unref (src->context);
    return FALSE;
  }

#ifdef GST_EXT_SOUP_MODIFICATION
  if (src->timeout == 0)
    src->session_timeout = 0;
  else if ((src->timeout > 0) && (src->timeout < DEFAULT_SESSION_TIMEOUT))
    src->session_timeout = src->timeout;

  GST_DEBUG_OBJECT (src, "session timeout: %d", src->session_timeout);

  src->duration = GST_CLOCK_TIME_NONE;
#endif

  if (src->proxy == NULL) {
    src->session =
        soup_session_async_new_with_options (SOUP_SESSION_ASYNC_CONTEXT,
        src->context, SOUP_SESSION_USER_AGENT, src->user_agent,
#ifdef GST_EXT_SOUP_MODIFICATION
        SOUP_SESSION_TIMEOUT, src->session_timeout,
#else
        SOUP_SESSION_TIMEOUT, src->timeout,
#endif
#ifdef HAVE_LIBSOUP_GNOME
        SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_GNOME,
#endif
        NULL);
  } else {
    src->session =
        soup_session_async_new_with_options (SOUP_SESSION_ASYNC_CONTEXT,
        src->context, SOUP_SESSION_PROXY_URI, src->proxy,
#ifdef GST_EXT_SOUP_MODIFICATION
        SOUP_SESSION_TIMEOUT, src->session_timeout,
#else
        SOUP_SESSION_TIMEOUT, src->timeout,
#endif
        SOUP_SESSION_USER_AGENT, src->user_agent, NULL);
  }

  if (!src->session) {
    GST_ELEMENT_ERROR (src, LIBRARY, INIT,
        (NULL), ("Failed to create async session"));
    return FALSE;
  }

#ifdef GST_EXT_SOUP_MODIFICATION
  SoupCookie *soup_cookie = NULL;
  GSList *c_list = NULL;

  soup_session_add_feature_by_type (src->session, SOUP_TYPE_COOKIE_JAR);
  src->cookie_jar = SOUP_COOKIE_JAR (soup_session_get_feature (src->session, SOUP_TYPE_COOKIE_JAR));
  if ((array = src->cookies) != NULL) {
    base_uri = soup_uri_new (src->location);
    while (*array != NULL) {
      soup_cookie = soup_cookie_parse (*array++, base_uri);
      if (soup_cookie != NULL) {
        GST_INFO_OBJECT (src, "adding cookies..");
        soup_cookie_jar_add_cookie (src->cookie_jar, soup_cookie);
      }
    }
    soup_uri_free (base_uri);
  }

  /* check if temporal cookie store exist. add them to jar */
  if ( src->cookie_list ) {
    for ( c_list = src->cookie_list; c_list; c_list = c_list->next) {
      soup_cookie = (SoupCookie *)c_list->data;
      if ( soup_cookie ) {
        soup_cookie_jar_add_cookie (src->cookie_jar, soup_cookie);
        c_list->data = NULL;
        GST_INFO_OBJECT (src, "adding cookies..");
      }
    }
    /* freeing list since all cookies are stolen by jar */
    g_slist_free(src->cookie_list);
    src->cookie_list = NULL;
  }

#endif

  g_signal_connect (src->session, "authenticate",
      G_CALLBACK (gst_soup_http_src_authenticate_cb), src);
  return TRUE;
}

static gboolean
gst_soup_http_src_stop (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);
  GST_DEBUG_OBJECT (src, "stop()");
  gst_soup_http_src_session_close (src);
  if (src->loop) {
    g_main_loop_unref (src->loop);
    g_main_context_unref (src->context);
    src->loop = NULL;
    src->context = NULL;
  }
  if (src->extra_headers) {
    gst_structure_free (src->extra_headers);
    src->extra_headers = NULL;
  }

  gst_soup_http_src_reset (src);
  return TRUE;
}

/* Interrupt a blocking request. */
static gboolean
gst_soup_http_src_unlock (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);
  GST_DEBUG_OBJECT (src, "unlock()");

  src->interrupted = TRUE;
  if (src->loop)
    g_main_loop_quit (src->loop);
  return TRUE;
}

/* Interrupt interrupt. */
static gboolean
gst_soup_http_src_unlock_stop (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);
  GST_DEBUG_OBJECT (src, "unlock_stop()");

  src->interrupted = FALSE;
  return TRUE;
}

static gboolean
gst_soup_http_src_get_size (GstBaseSrc * bsrc, guint64 * size)
{
  GstSoupHTTPSrc *src;

  src = GST_SOUP_HTTP_SRC (bsrc);

  if (src->have_size) {
#ifdef SEEK_CHANGES
    GST_DEBUG_OBJECT (src, "get_size() = %" G_GUINT64_FORMAT,
        src->file_size);
    *size = src->file_size;
#else
    GST_DEBUG_OBJECT (src, "get_size() = %" G_GUINT64_FORMAT,
        src->content_size);
    *size = src->content_size;
#endif
    return TRUE;
  }
  GST_DEBUG_OBJECT (src, "get_size() = FALSE");
  return FALSE;
}

static gboolean
gst_soup_http_src_is_seekable (GstBaseSrc * bsrc)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);

  return src->seekable;
}

static gboolean
gst_soup_http_src_do_seek (GstBaseSrc * bsrc, GstSegment * segment)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);

  GST_DEBUG_OBJECT (src, "do_seek(%" G_GUINT64_FORMAT ")", segment->start);

#ifdef SEEK_CHANGES
  src->seeked = TRUE;
#endif
  if (src->read_position == segment->start) {
    GST_DEBUG_OBJECT (src, "Seeking to current read position");

#ifdef SEEK_CHANGES
  if (src->request_position >= src->file_size)
    src->request_position = segment->start;
#endif
    return TRUE;
  }

  if (!src->seekable) {
    GST_WARNING_OBJECT (src, "Not seekable");
    return FALSE;
  }

  if (segment->rate < 0.0 || segment->format != GST_FORMAT_BYTES) {
    GST_WARNING_OBJECT (src, "Invalid seek segment");
    return FALSE;
  }

#ifdef SEEK_CHANGES
  if (src->content_size != 0 && segment->start >= src->file_size) {
#else
  if (src->content_size != 0 && segment->start >= src->content_size) {
#endif
    GST_WARNING_OBJECT (src, "Seeking behind end of file, will go to EOS soon");
  }

  /* Wait for create() to handle the jump in offset. */
  src->request_position = segment->start;
  return TRUE;
}

static gboolean
gst_soup_http_src_query (GstBaseSrc * bsrc, GstQuery * query)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);
  gboolean ret;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
#ifdef GST_EXT_SOUP_MODIFICATION
    if(src->videohub_dash_uri)
      gst_query_set_uri (query, src->videohub_dash_uri);
    else
#endif
      gst_query_set_uri (query, src->location);
      ret = TRUE;
      break;
#ifdef GST_EXT_SOUP_MODIFICATION
    case GST_QUERY_CUSTOM: {
        GstStructure *s;
        s = gst_query_get_structure (query);
        if (gst_structure_has_name (s, "HTTPCookies")) {
          GSList *cookie_list, *c;
          gchar **cookies, **array;
          GValue value = { 0, { { 0 } } };

          g_value_init (&value, G_TYPE_STRV);
          cookies = NULL;
          if ((cookie_list = soup_cookie_jar_all_cookies (src->cookie_jar)) != NULL) {
            cookies = g_new0 (gchar *, g_slist_length(cookie_list) + 1);
            array = cookies;
            for (c = cookie_list; c; c = c->next) {
              *array++ = soup_cookie_to_set_cookie_header ((SoupCookie *)(c->data));
            }
            soup_cookies_free (cookie_list);
          }
          g_value_set_boxed (&value, cookies);

          GST_INFO_OBJECT (src, "Received supported custom query");
          gst_structure_set_value (s, "cookies", &value);
          ret = TRUE;
        }
#ifdef USE_SAMSUNG_LINK
        else if (gst_structure_has_name (s, "X-ASP")){
          GST_INFO_OBJECT (src, "Received X-ASP custom query, X-ASP is %d", src->is_x_asp);
          GValue value = { 0, };
          g_value_init (&value, G_TYPE_BOOLEAN);
          g_value_set_boolean (&value, src->is_x_asp);
          gst_structure_set_value (s, "x-asp", &value);
          ret = TRUE;
        }
#endif
        else if (gst_structure_has_name(s, "HTTPUserAgent")) {
          GValue value = { 0, };
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, src->user_agent);
          GST_INFO_OBJECT (src, "Received HTTPUserAgent custom query");
          gst_structure_set_value (s, "user-agent", &value);
          ret = TRUE;
        }

        else {
          GST_WARNING_OBJECT (src,"Unsupported query");
          ret = FALSE;
        }
      }
      break;
    case GST_QUERY_DURATION: {

      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      if (format == GST_FORMAT_TIME) {
        if (src->duration != GST_CLOCK_TIME_NONE) {
          gst_query_set_duration (query, GST_FORMAT_TIME, src->duration);
          GST_DEBUG_OBJECT (src,"query duration time is %" GST_TIME_FORMAT,
          GST_TIME_ARGS(src->duration));
          ret = TRUE;
        } else {
          GST_WARNING_OBJECT (src,"Unsupported query");
          ret = FALSE;
        }

      } else if (format == GST_FORMAT_BYTES) {
        gst_query_set_duration (query, GST_FORMAT_BYTES, src->file_size);
        GST_DEBUG_OBJECT (src,"query duration bytes is %" G_GUINT64_FORMAT, src->file_size);
        ret = TRUE;
      } else {
        GST_WARNING_OBJECT (src,"Unsupported query");
        ret = FALSE;
      }
    }
      break;
#endif

    default:
      ret = FALSE;
      break;
  }

  if (!ret)
    ret = GST_BASE_SRC_CLASS (parent_class)->query (bsrc, query);

  return ret;
}

#ifdef USE_SAMSUNG_LINK
static gboolean
gst_soup_http_src_event (GstBaseSrc * bsrc, GstEvent *event)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (bsrc);
  gboolean ret;

  GST_DEBUG_OBJECT (src, "handling %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)){
    case GST_EVENT_SEEK:
      if (src->is_x_asp) {
        gdouble rate;
        GstFormat format;
        GstSeekFlags flags;
        GstSeekType start_type, stop_type;
        gint64 start, stop;

        gst_event_parse_seek (event, &rate, &format, &flags, &start_type, &start,
                                     &stop_type, &stop);
        if (format == GST_FORMAT_TIME) {
          src->seek_time_position = start / G_GINT64_CONSTANT (1000000);
          GST_DEBUG_OBJECT (src,"seek position time is %" G_GINT64_FORMAT"(ms)", src->seek_time_position);
        }
      }
      ret = GST_BASE_SRC_CLASS (parent_class)->event (bsrc, event);
      break;
    default:
      ret = GST_BASE_SRC_CLASS (parent_class)->event (bsrc, event);
      break;
  }

  return ret;
}
#endif


static gboolean
gst_soup_http_src_set_location (GstSoupHTTPSrc * src, const gchar * uri)
{
  if (src->location) {
    g_free (src->location);
    src->location = NULL;
  }
  src->location = g_strdup (uri);

  return TRUE;
}

static gboolean
gst_soup_http_src_set_proxy (GstSoupHTTPSrc * src, const gchar * uri)
{

  GST_WARNING_OBJECT (src, "gst_soup_http_src_set_proxy = %s",uri);

  if (src->proxy) {
    soup_uri_free (src->proxy);
    src->proxy = NULL;
  }
  if (g_str_has_prefix (uri, "http://")) {
    src->proxy = soup_uri_new (uri);
  } else {
    gchar *new_uri = g_strconcat ("http://", uri, NULL);

    src->proxy = soup_uri_new (new_uri);
    g_free (new_uri);
  }

  return TRUE;
}

static guint
gst_soup_http_src_uri_get_type (void)
{
  return GST_URI_SRC;
}

static gchar **
gst_soup_http_src_uri_get_protocols (void)
{
  static const gchar *protocols[] = { "http", "https", NULL };
  return (gchar **) protocols;
}

static const gchar *
gst_soup_http_src_uri_get_uri (GstURIHandler * handler)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (handler);

  return src->location;
}

static gboolean
gst_soup_http_src_uri_set_uri (GstURIHandler * handler, const gchar * uri)
{
  GstSoupHTTPSrc *src = GST_SOUP_HTTP_SRC (handler);

  return gst_soup_http_src_set_location (src, uri);
}

static void
gst_soup_http_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_soup_http_src_uri_get_type;
  iface->get_protocols = gst_soup_http_src_uri_get_protocols;
  iface->get_uri = gst_soup_http_src_uri_get_uri;
  iface->set_uri = gst_soup_http_src_uri_set_uri;
}
