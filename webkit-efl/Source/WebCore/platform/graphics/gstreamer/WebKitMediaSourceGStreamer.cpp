/*
 *  Copyright (C) 2009, 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  Copyright (C) 2013 Collabora Ltd.
 *  Copyright (C) 2013 Orange
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "WebKitMediaSourceGStreamer.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "GStreamerVersioning.h"
#include "NotImplemented.h"
#include "TimeRanges.h"
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <wtf/Deque.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(TIZEN_MSE)
#include "SourceBufferPrivateGStreamer.h"
#endif // ENABLE(TIZEN_MSE)

typedef struct _Source {
    GstElement* appsrc;
    guint sourceid;        /* To control the GSource */
    GstPad* srcpad;
    gboolean padAdded;
#if ENABLE(TIZEN_MSE)
    gboolean shallAddPad;
#endif // ENABLE(TIZEN_MSE)

    guint64 offset;
    guint64 size;
    gboolean paused;

    guint startId;
    guint stopId;
    guint needDataId;
    guint enoughDataId;
    guint seekId;

    guint64 requestedOffset;

#if ENABLE(TIZEN_MSE)
    WebCore::SourceBufferPrivateGStreamer* buffer;
    gboolean needData;
    guint needDataCbId;
#endif // ENABLE(TIZEN_MSE)
} Source;


#define WEBKIT_MEDIA_SRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_SRC, WebKitMediaSrcPrivate))

struct _WebKitMediaSrcPrivate {
    gchar* uri;
    Source sourceVideo;
    Source sourceAudio;
    WebCore::MediaPlayer* player;
    GstElement* playbin;
    gint64 duration;
    gboolean seekable;
    gboolean noMorePad;
    // TRUE if appsrc's version is >= 0.10.27, see
    // https://bugzilla.gnome.org/show_bug.cgi?id=609423
    gboolean haveAppSrc27;
    guint nbSource;
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    Deque<RefPtr<Uint8Array> > initDataQueue;
#endif // ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
};

enum {
    PropLocation = 1,
    ProLast
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src_%u", GST_PAD_SRC, GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC(webkit_media_src_debug);
#define GST_CAT_DEFAULT webkit_media_src_debug

static void webKitMediaSrcUriHandlerInit(gpointer gIface, gpointer ifaceData);
static void webKitMediaSrcFinalize(GObject*);
static void webKitMediaSrcSetProperty(GObject*, guint propertyId, const GValue*, GParamSpec*);
static void webKitMediaSrcGetProperty(GObject*, guint propertyId, GValue*, GParamSpec*);
static GstStateChangeReturn webKitMediaSrcChangeState(GstElement*, GstStateChange);
static gboolean webKitMediaSrcQueryWithParent(GstPad*, GstObject*, GstQuery*);
#ifndef GST_API_VERSION_1
static gboolean webKitMediaSrcQuery(GstPad*, GstQuery*);
#endif

static void webKitMediaVideoSrcNeedDataCb(GstAppSrc*, guint, gpointer);
static void webKitMediaVideoSrcEnoughDataCb(GstAppSrc*, gpointer);
static gboolean webKitMediaVideoSrcSeekDataCb(GstAppSrc*, guint64, gpointer);
static void webKitMediaAudioSrcNeedDataCb(GstAppSrc*, guint, gpointer);
static void webKitMediaAudioSrcEnoughDataCb(GstAppSrc*, gpointer);
static gboolean webKitMediaAudioSrcSeekDataCb(GstAppSrc*, guint64, gpointer);
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
void webKitMediaVideoSrcNeedKeyCb(GstElement*, gpointer, guint, gpointer);
static void webKitMediaAudioSrcNeedKeyCb(GstElement*, gpointer, guint, gpointer);
#endif // ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)

static GstAppSrcCallbacks appsrcCallbacksVideo = {
    webKitMediaVideoSrcNeedDataCb,
    webKitMediaVideoSrcEnoughDataCb,
    webKitMediaVideoSrcSeekDataCb,
    { 0 }
};
static GstAppSrcCallbacks appsrcCallbacksAudio = {
    webKitMediaAudioSrcNeedDataCb,
    webKitMediaAudioSrcEnoughDataCb,
    webKitMediaAudioSrcSeekDataCb,
    { 0 }
};
#define webkit_media_src_parent_class parent_class
// We split this out into another macro to avoid a check-webkit-style error.
#define WEBKIT_MEDIA_SRC_CATEGORY_INIT GST_DEBUG_CATEGORY_INIT(webkit_media_src_debug, "webkitmediasrc", 0, "websrc element");
G_DEFINE_TYPE_WITH_CODE(WebKitMediaSrc, webkit_media_src, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, webKitMediaSrcUriHandlerInit);
    WEBKIT_MEDIA_SRC_CATEGORY_INIT);

static void webkit_media_src_class_init(WebKitMediaSrcClass* klass)
{
    GObjectClass* oklass = G_OBJECT_CLASS(klass);
    GstElementClass* eklass = GST_ELEMENT_CLASS(klass);

    oklass->finalize = webKitMediaSrcFinalize;
    oklass->set_property = webKitMediaSrcSetProperty;
    oklass->get_property = webKitMediaSrcGetProperty;

    gst_element_class_add_pad_template(eklass, gst_static_pad_template_get(&srcTemplate));

    setGstElementClassMetadata(eklass, "WebKit Media source element", "Source", "Handles Blob uris", "Stephane Jadaud <sjadaud@sii.fr>");

    /* Allows setting the uri using the 'location' property, which is used
     * for example by gst_element_make_from_uri() */
    g_object_class_install_property(oklass,
        PropLocation,
        g_param_spec_string("location", "location", "Location to read from", 0,
        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    eklass->change_state = webKitMediaSrcChangeState;

    g_type_class_add_private(klass, sizeof(WebKitMediaSrcPrivate));
}

static void webKitMediaSrcAddSrc(WebKitMediaSrc* src, GstElement* element)
{
    GstPad* ghostPad;
    WebKitMediaSrcPrivate* priv = src->priv;
    GOwnPtr<gchar> name;

    if (!gst_bin_add(GST_BIN(src), element)) {
        GST_DEBUG_OBJECT(src, "Src element not added");
        return;
    }
    GRefPtr<GstPad> targetsrc = adoptGRef(gst_element_get_static_pad(element, "src"));
    if (!targetsrc) {
        GST_DEBUG_OBJECT(src, "Pad not found");
        return;
    }

    gst_element_sync_state_with_parent(element);
    name.set(g_strdup_printf("src_%u", priv->nbSource));
    ghostPad = webkitGstGhostPadFromStaticTemplate(&srcTemplate, name.get(), targetsrc.get());
    gst_pad_set_active(ghostPad, TRUE);

    priv->nbSource++;

    if (priv->sourceVideo.appsrc == element)
        priv->sourceVideo.srcpad = ghostPad;
    else if (priv->sourceAudio.appsrc == element)
        priv->sourceAudio.srcpad = ghostPad;

#ifdef GST_API_VERSION_1
    GST_OBJECT_FLAG_SET(ghostPad, GST_PAD_FLAG_NEED_PARENT);
    gst_pad_set_query_function(ghostPad, webKitMediaSrcQueryWithParent);
#else
    gst_pad_set_query_function(ghostPad, webKitMediaSrcQuery);
#endif
}

static void webkit_media_src_init(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = WEBKIT_MEDIA_SRC_GET_PRIVATE(src);
    src->priv = priv;

    priv->sourceVideo.appsrc = gst_element_factory_make("appsrc", "videoappsrc");
    g_object_set (priv->sourceVideo.appsrc, "format", GST_FORMAT_TIME, NULL);

    gst_app_src_set_callbacks(GST_APP_SRC(priv->sourceVideo.appsrc), &appsrcCallbacksVideo, src, 0);
#if ENABLE(TIZEN_MSE)
    gst_app_src_set_stream_type(GST_APP_SRC(priv->sourceVideo.appsrc), GST_APP_STREAM_TYPE_SEEKABLE);
#endif
    webKitMediaSrcAddSrc(src, priv->sourceVideo.appsrc);

    priv->sourceAudio.appsrc = gst_element_factory_make("appsrc", "audioappsrc");
    g_object_set (priv->sourceAudio.appsrc, "format", GST_FORMAT_TIME, NULL);
    gst_app_src_set_callbacks(GST_APP_SRC(priv->sourceAudio.appsrc), &appsrcCallbacksAudio, src, 0);
#if ENABLE(TIZEN_MSE)
    gst_app_src_set_stream_type(GST_APP_SRC(priv->sourceAudio.appsrc), GST_APP_STREAM_TYPE_SEEKABLE);
#endif
    webKitMediaSrcAddSrc(src, priv->sourceAudio.appsrc);
}

static void webKitMediaSrcFinalize(GObject* object)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = src->priv;

    g_free(priv->uri);

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void webKitMediaSrcSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);
    switch (propId) {
    case PropLocation:
#ifdef GST_API_VERSION_1
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value), 0);
#else
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value));
#endif
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webKitMediaSrcGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    switch (propId) {
    case PropLocation:
        g_value_set_string(value, priv->uri);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
    GST_OBJECT_UNLOCK(src);
}

// must be called on main thread and with object unlocked
static gboolean webKitMediaVideoSrcStop(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;
    gboolean seeking;

    GST_OBJECT_LOCK(src);

    seeking = priv->sourceVideo.seekId;

    if (priv->sourceVideo.startId) {
        g_source_remove(priv->sourceVideo.startId);
        priv->sourceVideo.startId = 0;
    }

    priv->player = 0;
    priv->playbin = 0;
#if ENABLE(TIZEN_MSE)
    if (priv->sourceVideo.needDataCbId) {
        g_source_remove(priv->sourceVideo.needDataCbId);
        priv->sourceVideo.needDataCbId = 0;
    }
#endif
    if (priv->sourceVideo.needDataId)
        g_source_remove(priv->sourceVideo.needDataId);
    priv->sourceVideo.needDataId = 0;

    if (priv->sourceVideo.enoughDataId)
        g_source_remove(priv->sourceVideo.enoughDataId);
    priv->sourceVideo.enoughDataId = 0;

    if (priv->sourceVideo.seekId)
        g_source_remove(priv->sourceVideo.seekId);

    priv->sourceVideo.seekId = 0;

    priv->sourceVideo.paused = FALSE;
    priv->sourceVideo.offset = 0;
    priv->seekable = FALSE;

    priv->duration = 0;
    priv->nbSource = 0;

    priv->sourceVideo.stopId = 0;

    GST_OBJECT_UNLOCK(src);

    if (priv->sourceVideo.appsrc) {
        gst_app_src_set_caps(GST_APP_SRC(priv->sourceVideo.appsrc), 0);
        if (!seeking)
            gst_app_src_set_size(GST_APP_SRC(priv->sourceVideo.appsrc), -1);
    }

    GST_DEBUG_OBJECT(src, "Stopped request");

    return FALSE;
}

static gboolean webKitMediaAudioSrcStop(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;
    gboolean seeking;

    GST_OBJECT_LOCK(src);

    seeking = priv->sourceAudio.seekId;

    if (priv->sourceAudio.startId) {
        g_source_remove(priv->sourceAudio.startId);
        priv->sourceAudio.startId = 0;
    }

    priv->player = 0;
    priv->playbin = 0;
#if ENABLE(TIZEN_MSE)
    if (priv->sourceAudio.needDataCbId) {
        g_source_remove(priv->sourceAudio.needDataCbId);
        priv->sourceAudio.needDataCbId = 0;
    }
#endif
    if (priv->sourceAudio.needDataId)
        g_source_remove(priv->sourceAudio.needDataId);
    priv->sourceAudio.needDataId = 0;

    if (priv->sourceAudio.enoughDataId)
        g_source_remove(priv->sourceAudio.enoughDataId);
    priv->sourceAudio.enoughDataId = 0;

    if (priv->sourceAudio.seekId)
        g_source_remove(priv->sourceAudio.seekId);

    priv->sourceAudio.seekId = 0;

    priv->sourceAudio.paused = FALSE;

    priv->sourceAudio.offset = 0;

    priv->seekable = FALSE;

    priv->duration = 0;
    priv->nbSource = 0;

    priv->sourceAudio.stopId = 0;

    GST_OBJECT_UNLOCK(src);

    if (priv->sourceAudio.appsrc) {
        gst_app_src_set_caps(GST_APP_SRC(priv->sourceAudio.appsrc), 0);
        if (!seeking)
            gst_app_src_set_size(GST_APP_SRC(priv->sourceAudio.appsrc), -1);
    }

    GST_DEBUG_OBJECT(src, "Stopped request");

    return FALSE;
}

// must be called on main thread and with object unlocked
static gboolean webKitMediaVideoSrcStart(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    if (!priv->uri) {
        GST_ERROR_OBJECT(src, "No URI provided");
        GST_OBJECT_UNLOCK(src);
        webKitMediaVideoSrcStop(src);
        return FALSE;
    }

#if ENABLE(TIZEN_MSE)
    priv->sourceVideo.needData = true;
#endif // ENABLE(TIZEN_MSE)

    GST_OBJECT_UNLOCK(src);
    GST_DEBUG_OBJECT(src, "Started request");

    return FALSE;
}

// must be called on main thread and with object unlocked
static gboolean webKitMediaAudioSrcStart(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    if (!priv->uri) {
        GST_ERROR_OBJECT(src, "No URI provided");
        GST_OBJECT_UNLOCK(src);
        webKitMediaAudioSrcStop(src);
        return FALSE;
    }

#if ENABLE(TIZEN_MSE)
    priv->sourceAudio.needData = true;
#endif // ENABLE(TIZEN_MSE)

    GST_OBJECT_UNLOCK(src);
    GST_DEBUG_OBJECT(src, "Started request");

    return FALSE;
}

static GstStateChangeReturn webKitMediaSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(element);
    WebKitMediaSrcPrivate* priv = src->priv;

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!priv->sourceVideo.appsrc && !priv->sourceAudio.appsrc) {
            gst_element_post_message(element,
                gst_missing_element_message_new(element, "appsrc"));
            GST_ELEMENT_ERROR(src, CORE, MISSING_PLUGIN, (0), ("no appsrc"));
            return GST_STATE_CHANGE_FAILURE;
        }
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (G_UNLIKELY(ret == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT(src, "State change failed");
        return ret;
    }

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_OBJECT(src, "READY->PAUSED");
        GST_OBJECT_LOCK(src);
        priv->sourceVideo.startId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaVideoSrcStart, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
        g_source_set_name_by_id(priv->sourceVideo.startId, "[WebKit] webKitMediaVideoSrcStart");
        priv->sourceAudio.startId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaAudioSrcStart, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
        g_source_set_name_by_id(priv->sourceAudio.startId, "[WebKit] webKitMediaAudioSrcStart");
        GST_OBJECT_UNLOCK(src);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(src, "PAUSED->READY");
        GST_OBJECT_LOCK(src);
        priv->sourceVideo.stopId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaVideoSrcStop, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
        g_source_set_name_by_id(priv->sourceVideo.stopId, "[WebKit] webKitMediaVideoSrcStop");
        priv->sourceAudio.stopId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaAudioSrcStop, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
        g_source_set_name_by_id(priv->sourceAudio.stopId, "[WebKit] webKitMediaAudioSrcStop");
        GST_OBJECT_UNLOCK(src);
        break;
    default:
        break;
    }

    return ret;
}

static gboolean webKitMediaSrcQueryWithParent(GstPad* pad, GstObject* parent, GstQuery* query)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(GST_ELEMENT(parent));
    gboolean result = FALSE;

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_DURATION: {
        GstFormat format;
        gst_query_parse_duration(query, &format, NULL);

        GST_DEBUG_OBJECT(src, "duration query in format %s", gst_format_get_name(format));
        GST_OBJECT_LOCK(src);
        if ((format == GST_FORMAT_TIME) && (src->priv->duration > 0)) {
            gst_query_set_duration(query, format, src->priv->duration);
            result = TRUE;
        }
        GST_OBJECT_UNLOCK(src);
        break;
    }
    case GST_QUERY_URI: {
        GST_OBJECT_LOCK(src);
        gst_query_set_uri(query, src->priv->uri);
        GST_OBJECT_UNLOCK(src);
        result = TRUE;
        break;
    }
    default: {
        GRefPtr<GstPad> target = adoptGRef(gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(pad)));
        // Forward the query to the proxy target pad.
        if (target)
            result = gst_pad_query(target.get(), query);
        break;
    }
    }

    return result;
}

#ifndef GST_API_VERSION_1
static gboolean webKitMediaSrcQuery(GstPad* pad, GstQuery* query)
{
    GRefPtr<GstElement> src = adoptGRef(gst_pad_get_parent_element(pad));
    return webKitMediaSrcQueryWithParent(pad, GST_OBJECT(src.get()), query);
}
#endif

// uri handler interface
#ifdef GST_API_VERSION_1
static GstURIType webKitMediaSrcUriGetType(GType)
#else
static GstURIType webKitMediaSrcUriGetType(void)
#endif
{
    return GST_URI_SRC;
}

#ifdef GST_API_VERSION_1
const gchar* const* webKitMediaSrcGetProtocols(GType)
{
    static const char* protocols[] = {"mediasourceblob", 0 };
    return protocols;
}
#else
      gchar*      * webKitMediaSrcGetProtocols(void)
{
    static       char* protocols[] = {"mediasourceblob", 0 };
    return protocols;
}
#endif

#ifdef GST_API_VERSION_1
static gchar* webKitMediaSrcGetUri(GstURIHandler* handler)
#else
static const gchar* webKitMediaSrcGetUri(GstURIHandler* handler)
#endif
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(handler);
    gchar* ret;

    GST_OBJECT_LOCK(src);
    ret = g_strdup(src->priv->uri);
    GST_OBJECT_UNLOCK(src);
    return g_strdup(ret);
}

#ifdef GST_API_VERSION_1
static gboolean webKitMediaSrcSetUri(GstURIHandler* handler, const gchar* uri, GError** error)
#else
static gboolean webKitMediaSrcSetUri(GstURIHandler* handler, const gchar* uri)
#endif
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(handler);
    WebKitMediaSrcPrivate* priv = src->priv;
    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    GST_OBJECT_LOCK(src);
    g_free(priv->uri);
    priv->uri = 0;
    if (!uri) {
        GST_OBJECT_UNLOCK(src);
        return TRUE;
    }

    WebCore::KURL url(WebCore::KURL(), uri);

    priv->uri = g_strdup(url.string().utf8().data());
    GST_OBJECT_UNLOCK(src);
    return TRUE;
}

static void webKitMediaSrcUriHandlerInit(gpointer gIface, gpointer)
{
    GstURIHandlerInterface* iface = (GstURIHandlerInterface *) gIface;

    iface->get_type = webKitMediaSrcUriGetType;
    iface->get_protocols = webKitMediaSrcGetProtocols;
    iface->get_uri = webKitMediaSrcGetUri;
    iface->set_uri = webKitMediaSrcSetUri;
}

#if ENABLE(TIZEN_MSE)
namespace {
    void sourceNeedData(Source &s)
    {
        s.needData = true;
        if (s.buffer)
            s.buffer->sendData();

        if (s.needDataCbId) {
            g_source_remove(s.needDataCbId);
            s.needDataCbId = 0;
        }
    }
}

static void  videoNeedDataCb(WebKitMediaSrc* data) {
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(data);
    WebKitMediaSrcPrivate* priv = src->priv;

    sourceNeedData(priv->sourceVideo);
}

static void  audioNeedDataCb(WebKitMediaSrc* data) {
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(data);
    WebKitMediaSrcPrivate* priv = src->priv;

    sourceNeedData(priv->sourceAudio);
}
#endif // ENABLE(TIZEN_MSE)

// appsrc callbacks
static gboolean webKitMediaVideoSrcNeedDataMainCb(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    // already stopped
    if (!priv->sourceVideo.needDataId) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }

    priv->sourceVideo.paused = FALSE;
    priv->sourceVideo.needDataId = 0;
    GST_OBJECT_UNLOCK(src);

    return FALSE;
}

static gboolean webKitMediaAudioSrcNeedDataMainCb(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    // already stopped
    if (!priv->sourceAudio.needDataId) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }

    priv->sourceAudio.paused = FALSE;
    priv->sourceAudio.needDataId = 0;
    GST_OBJECT_UNLOCK(src);

    return FALSE;
}

static void webKitMediaVideoSrcNeedDataCb(GstAppSrc*, guint length, gpointer userData)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Need more data: %u", length);

    GST_OBJECT_LOCK(src);

#if ENABLE(TIZEN_MSE)
    if (!priv->sourceVideo.needDataCbId)
        priv->sourceVideo.needDataCbId = g_idle_add((GSourceFunc) videoNeedDataCb, src);
#endif // ENABLE(TIZEN_MSE)

    if (priv->sourceVideo.needDataId || !priv->sourceVideo.paused) {
        GST_OBJECT_UNLOCK(src);
        return;
    }

    priv->sourceVideo.needDataId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaVideoSrcNeedDataMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    g_source_set_name_by_id(priv->sourceVideo.needDataId, "[WebKit] webKitMediaVideoSrcNeedDataMainCb");
    GST_OBJECT_UNLOCK(src);
}

static void webKitMediaAudioSrcNeedDataCb(GstAppSrc*, guint length, gpointer userData)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Need more data: %u", length);

    GST_OBJECT_LOCK(src);

#if ENABLE(TIZEN_MSE)
    if (!priv->sourceAudio.needDataCbId)
        priv->sourceAudio.needDataCbId = g_idle_add((GSourceFunc) audioNeedDataCb, src);
#endif // ENABLE(TIZEN_MSE)

    if (priv->sourceAudio.needDataId || !priv->sourceAudio.paused) {
        GST_OBJECT_UNLOCK(src);
        return;
    }

    priv->sourceAudio.needDataId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaAudioSrcNeedDataMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    g_source_set_name_by_id(priv->sourceAudio.needDataId, "[WebKit] webKitMediaAudioSrcNeedDataMainCb");
    GST_OBJECT_UNLOCK(src);
}

static gboolean webKitMediaVideoSrcEnoughDataMainCb(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    // already stopped
    if (!priv->sourceVideo.enoughDataId) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }

#if ENABLE(TIZEN_MSE)
    priv->sourceVideo.needData = false;
#endif // ENABLE(TIZEN_MSE)

    priv->sourceVideo.paused = TRUE;
    priv->sourceVideo.enoughDataId = 0;
    GST_OBJECT_UNLOCK(src);

    return FALSE;
}

static gboolean webKitMediaAudioSrcEnoughDataMainCb(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    // already stopped
    if (!priv->sourceAudio.enoughDataId) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }

#if ENABLE(TIZEN_MSE)
    priv->sourceAudio.needData = false;
#endif // ENABLE(TIZEN_MSE)

    priv->sourceAudio.paused = TRUE;
    priv->sourceAudio.enoughDataId = 0;
    GST_OBJECT_UNLOCK(src);

    return FALSE;
}

static void webKitMediaVideoSrcEnoughDataCb(GstAppSrc*, gpointer userData)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Have enough data");

    GST_OBJECT_LOCK(src);
    if (priv->sourceVideo.enoughDataId || priv->sourceVideo.paused) {
        GST_OBJECT_UNLOCK(src);
        return;
    }

    priv->sourceVideo.enoughDataId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaVideoSrcEnoughDataMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    g_source_set_name_by_id(priv->sourceVideo.enoughDataId, "[WebKit] webKitMediaVideoSrcEnoughDataMainCb");
    GST_OBJECT_UNLOCK(src);
}

static void webKitMediaAudioSrcEnoughDataCb(GstAppSrc*, gpointer userData)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Have enough data");

    GST_OBJECT_LOCK(src);
    if (priv->sourceAudio.enoughDataId || priv->sourceAudio.paused) {
        GST_OBJECT_UNLOCK(src);
        return;
    }

    priv->sourceAudio.enoughDataId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaAudioSrcEnoughDataMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    g_source_set_name_by_id(priv->sourceAudio.enoughDataId, "[WebKit] webKitMediaAudioSrcEnoughDataMainCb");
    GST_OBJECT_UNLOCK(src);
}

static gboolean webKitMediaVideoSrcSeekMainCb(WebKitMediaSrc* /* src */)
{
    notImplemented();
    return FALSE;
}

static gboolean webKitMediaAudioSrcSeekMainCb(WebKitMediaSrc* /* src */)
{
    notImplemented();
    return FALSE;
}

static gboolean webKitMediaVideoSrcSeekDataCb(GstAppSrc*, guint64 offset, gpointer userData)
{
#if ENABLE(TIZEN_MSE)
    // We always want to flush appsrc when seeking.
    return TRUE;
#endif
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Seeking to offset: %" G_GUINT64_FORMAT, offset);
    GST_OBJECT_LOCK(src);
    if (offset == priv->sourceVideo.offset && priv->sourceVideo.requestedOffset == priv->sourceVideo.offset) {
        GST_OBJECT_UNLOCK(src);
        return TRUE;
    }

    if (!priv->seekable) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }
    if (offset > priv->sourceVideo.size) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }

    GST_DEBUG_OBJECT(src, "Doing range-request seek");
    priv->sourceVideo.requestedOffset = offset;

    if (priv->sourceVideo.seekId)
        g_source_remove(priv->sourceVideo.seekId);
    priv->sourceVideo.seekId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaVideoSrcSeekMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    g_source_set_name_by_id(priv->sourceVideo.seekId, "[WebKit] webKitMediaVideoSrcSeekMainCb");
    GST_OBJECT_UNLOCK(src);

    return TRUE;
}

static gboolean webKitMediaAudioSrcSeekDataCb(GstAppSrc*, guint64 offset, gpointer userData)
{
#if ENABLE(TIZEN_MSE)
    return TRUE;
#endif
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_DEBUG_OBJECT(src, "Seeking to offset: %" G_GUINT64_FORMAT, offset);
    GST_OBJECT_LOCK(src);
    if (offset == priv->sourceAudio.offset && priv->sourceAudio.requestedOffset == priv->sourceAudio.offset) {
        GST_OBJECT_UNLOCK(src);
        return TRUE;
    }

    if (!priv->seekable) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }
    if (offset > priv->sourceAudio.size) {
        GST_OBJECT_UNLOCK(src);
        return FALSE;
    }

    GST_DEBUG_OBJECT(src, "Doing range-request seek");
    priv->sourceAudio.requestedOffset = offset;

    if (priv->sourceAudio.seekId)
        g_source_remove(priv->sourceAudio.seekId);
    priv->sourceAudio.seekId = g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaAudioSrcSeekMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    g_source_set_name_by_id(priv->sourceAudio.seekId, "[WebKit] webKitMediaAudioSrcSeekMainCb");
    GST_OBJECT_UNLOCK(src);

    return TRUE;
}

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
static gboolean webKitMediaSrcNeedKeyMainCb(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    ASSERT_WITH_MESSAGE(priv->initDataQueue.size(),"initData should contain at least one element!");
    RefPtr<Uint8Array> initDataArray = priv->initDataQueue.takeFirst();
    priv->player->keyNeeded(initDataArray.get());
    GST_OBJECT_UNLOCK(src);

    return FALSE;
}

static void webKitMediaSrcNeedKeyCb(gpointer initData, guint initDataLength, gpointer userData)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(userData);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    priv->initDataQueue.append(Uint8Array::create(static_cast<const unsigned char *>(initData), initDataLength));
    g_timeout_add_full(G_PRIORITY_DEFAULT, 0, (GSourceFunc) webKitMediaSrcNeedKeyMainCb, gst_object_ref(src), (GDestroyNotify) gst_object_unref);
    GST_OBJECT_UNLOCK(src);
}

void webKitMediaSrcNeedKeyCb(GstElement*, gpointer initData, guint initDataLength, guint /* numInitData */, gpointer userData)
{
    webKitMediaSrcNeedKeyCb(initData, initDataLength, userData);
}

void webKitMediaVideoSrcNeedKeyCb(GstElement*, gpointer initData, guint initDataLength, gpointer userData)
{
    webKitMediaSrcNeedKeyCb(initData, initDataLength, userData);
}

static void webKitMediaAudioSrcNeedKeyCb(GstElement*, gpointer initData, guint initDataLength, gpointer userData)
{
    webKitMediaSrcNeedKeyCb(initData, initDataLength, userData);
}
#endif // ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)

void webKitMediaSrcSetMediaPlayer(WebKitMediaSrc* src, WebCore::MediaPlayer* player)
{
    WebKitMediaSrcPrivate* priv = src->priv;
    priv->player = player;
}

void webKitMediaSrcSetPlayBin(WebKitMediaSrc* src, GstElement* playBin)
{
    WebKitMediaSrcPrivate* priv = src->priv;
    priv->playbin = playBin;
}

namespace
{
    bool isAudio(const String &type)
    { return type.startsWith("audio"); }

    bool isVideo(const String &type)
    { return type.startsWith("video"); }
}

MediaSourceClientGstreamer::MediaSourceClientGstreamer(WebKitMediaSrc* src)
    : m_src(static_cast<WebKitMediaSrc*>(gst_object_ref(src)))
{
#if ENABLE(TIZEN_MSE)
    m_src->priv->sourceVideo.buffer = 0;
    m_src->priv->sourceVideo.needData = false;
    m_src->priv->sourceAudio.buffer = 0;
    m_src->priv->sourceAudio.needData = false;
#endif // ENABLE(TIZEN_MSE)
}

MediaSourceClientGstreamer::~MediaSourceClientGstreamer()
{
    gst_object_unref(m_src);
}

void MediaSourceClientGstreamer::didReceiveDuration(double duration)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    GST_DEBUG_OBJECT(m_src, "Received duration: %lf", duration);

    GST_OBJECT_LOCK(m_src);
    priv->duration = duration >= 0.0 ? static_cast<gint64>(duration*GST_SECOND) : 0;
    GST_OBJECT_UNLOCK(m_src);
}

#if ENABLE(TIZEN_MSE)
void MediaSourceClientGstreamer::didAddSourceBuffer(const String& type)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    if (isVideo(type))
        priv->sourceVideo.shallAddPad = TRUE;
    else if (isAudio(type))
        priv->sourceAudio.shallAddPad = TRUE;
}
#endif // ENABLE(TIZEN_MSE)

#if ENABLE(TIZEN_MSE)
void MediaSourceClientGstreamer::didReceiveData(GstBuffer* inputBuffer, String type)
#else
void MediaSourceClientGstreamer::didReceiveData(const char* data, int length, String type)
#endif
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer * buffer;

#if ENABLE(TIZEN_MSE)
    if (priv->noMorePad == FALSE) {
        if (priv->sourceVideo.shallAddPad == TRUE) {
            gst_element_add_pad(GST_ELEMENT(m_src), priv->sourceVideo.srcpad);
            priv->sourceVideo.padAdded = TRUE;
            priv->sourceVideo.shallAddPad = FALSE;
        }
        if (priv->sourceAudio.shallAddPad == TRUE) {
            gst_element_add_pad(GST_ELEMENT(m_src), priv->sourceAudio.srcpad);
            priv->sourceAudio.padAdded = TRUE;
            priv->sourceAudio.shallAddPad = FALSE;
        }
        gst_element_no_more_pads(GST_ELEMENT(m_src));
        priv->noMorePad = TRUE;
    }
#endif // ENABLE(TIZEN_MSE)

    if (isVideo(type)) {
#if !ENABLE(TIZEN_MSE)
        if (priv->noMorePad == FALSE && priv->sourceVideo.padAdded == TRUE) {
            gst_element_no_more_pads(GST_ELEMENT(m_src));
            priv->noMorePad = TRUE;
        }
        if (priv->noMorePad == FALSE && priv->sourceVideo.padAdded == FALSE) {
            gst_element_add_pad(GST_ELEMENT(m_src), priv->sourceVideo.srcpad);
            priv->sourceVideo.padAdded = TRUE;
        }
#endif // !ENABLE(TIZEN_MSE)
        GST_OBJECT_LOCK(m_src);
#if ENABLE(TIZEN_MSE)
        buffer = gst_buffer_copy(inputBuffer);
#else
        buffer = createGstBufferForData(data, length);
#endif
        GST_OBJECT_UNLOCK(m_src);

#if ENABLE(TIZEN_MSE)
        priv->sourceVideo.needData = false;
        // gst_app_src_set_caps(GST_APP_SRC(priv->sourceVideo.appsrc),buffer_caps);
#endif // ENABLE(TIZEN_MSE)
        ret = gst_app_src_push_buffer(GST_APP_SRC(priv->sourceVideo.appsrc), buffer);
    } else if (isAudio(type)) {
#if !ENABLE(TIZEN_MSE)
        if (priv->noMorePad == FALSE && priv->sourceAudio.padAdded == TRUE) {
            gst_element_no_more_pads(GST_ELEMENT(m_src));
            priv->noMorePad = TRUE;
        }
        if (priv->noMorePad == FALSE && priv->sourceAudio.padAdded == FALSE) {
            gst_element_add_pad(GST_ELEMENT(m_src), priv->sourceAudio.srcpad);
            priv->sourceAudio.padAdded = TRUE;
        }
#endif // !ENABLE(TIZEN_MSE)
        GST_OBJECT_LOCK(m_src);
#if ENABLE(TIZEN_MSE)
        buffer = gst_buffer_copy(inputBuffer);
#else
        buffer = createGstBufferForData(data, length);
#endif
        GST_OBJECT_UNLOCK(m_src);

#if ENABLE(TIZEN_MSE)
        priv->sourceAudio.needData = false;
        //gst_app_src_set_caps(GST_APP_SRC(priv->sourceAudio.appsrc),buffer_caps);
#endif // ENABLE(TIZEN_MSE)
        ret = gst_app_src_push_buffer(GST_APP_SRC(priv->sourceAudio.appsrc), buffer);
    }

#ifdef GST_API_VERSION_1
    if (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)
#else
    if (ret != GST_FLOW_OK && ret != GST_FLOW_UNEXPECTED)
#endif
        GST_ELEMENT_ERROR(m_src, CORE, FAILED, (0), (0));
}

#if ENABLE(TIZEN_MSE)
#include <math.h>
static GstClockTime toGstClockTime(float time)
{
    // Extract the integer part of the time (seconds) and the fractional part (microseconds). Attempt to
    // round the microseconds so no floating point precision is lost and we can perform an accurate seek.
    float seconds;
    float microSeconds = modff(time, &seconds) * 1000000;
    GTimeVal timeValue;
    timeValue.tv_sec = static_cast<glong>(seconds);
    timeValue.tv_usec = static_cast<glong>(roundf(microSeconds / 10000) * 10000);
    return GST_TIMEVAL_TO_TIME(timeValue);
}


void MediaSourceClientGstreamer::prepareSeeking(const MediaTime& time, const String& type)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    GstElement* src = 0;
    if (isVideo(type))
        src = GST_ELEMENT(priv->sourceVideo.appsrc);
    else
        src = GST_ELEMENT(priv->sourceAudio.appsrc);

    GstEvent* seek_event;
    GstClockTime clockTime = toGstClockTime(time.toDouble());
    /* Create the seek event */
    seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), GST_SEEK_TYPE_SET, clockTime, GST_SEEK_TYPE_NONE, 0);
    gst_element_send_event(src, seek_event);
    gst_element_send_event(priv->playbin, seek_event);
}

void MediaSourceClientGstreamer::didSeek(const MediaTime& /* time */, const String& /* type */)
{
/*
    WebKitMediaSrcPrivate* priv = m_src->priv;
    GstElement* src = priv->playbin;

    GstClockTime clockTime = toGstClockTime(time.toFloat());
    gst_element_set_start_time(priv->playbin, clockTime);
*/
}

void MediaSourceClientGstreamer::setBuffer(WebCore::SourceBufferPrivateGStreamer *buffer, const String &type)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;

    GST_OBJECT_LOCK(m_src);
    if (isVideo(type))
        priv->sourceVideo.buffer = buffer;
    else if (isAudio(type))
        priv->sourceAudio.buffer = buffer;
    GST_OBJECT_UNLOCK(m_src);
}

void MediaSourceClientGstreamer::dataReady(const String &type)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    Source *src = NULL;

    if (isVideo(type))
        src = &priv->sourceVideo;
    else if (isAudio(type))
        src = &priv->sourceAudio;

    if (!src)
        return;

    if (src->needData)
        src->buffer->sendData();
}
#endif // ENABLE(TIZEN_MSE)

void MediaSourceClientGstreamer::didFinishLoading(double)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;

    GST_DEBUG_OBJECT(m_src, "Have EOS");

    GST_OBJECT_LOCK(m_src);
    if (!priv->sourceVideo.seekId) {
        GST_OBJECT_UNLOCK(m_src);
        gst_app_src_end_of_stream(GST_APP_SRC(priv->sourceVideo.appsrc));
    } else
        GST_OBJECT_UNLOCK(m_src);

    GST_OBJECT_LOCK(m_src);
    if (!priv->sourceAudio.seekId) {
        GST_OBJECT_UNLOCK(m_src);
        gst_app_src_end_of_stream(GST_APP_SRC(priv->sourceAudio.appsrc));
    } else
        GST_OBJECT_UNLOCK(m_src);
}

void MediaSourceClientGstreamer::didFail()
{
    gst_app_src_end_of_stream(GST_APP_SRC(m_src->priv->sourceVideo.appsrc));
    gst_app_src_end_of_stream(GST_APP_SRC(m_src->priv->sourceAudio.appsrc));
}

#endif // USE(GSTREAMER)

