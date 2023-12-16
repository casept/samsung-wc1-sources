/*
    Copyright (C) 2012 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "WebKitCameraSourceGStreamerTizen.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "LocalMediaServer.h"
#include <sensor.h>
#include <vconf.h>
#include <wtf/gobject/GRefPtr.h>

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"
#endif // ENABLE(TIZEN_EXTENSIBLE_API)

#define RADIAN_VALUE (57.2957)

#define HOST_ADDRESS "127.0.0.1"
#define HOST_PORT 8888

enum {
    ROTATE_0,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270,
    ROTATE_ERROR
};

using namespace WebCore;

class RotationManager {
    WTF_MAKE_NONCOPYABLE(RotationManager);
    public:
        RotationManager(WebKitCameraSrc*);
        virtual ~RotationManager();

        static void onRotationChanged(sensor_h sensor, sensor_event_s *event, void *userData);
        void registerRotationCallback();
        void unregisterRotationCallback();
        void updateMethod();
        int rotation(float x, float y, float z);

    private:
        WebKitCameraSrc* m_src;
        sensor_listener_h m_listener;
};

#define WEBKIT_CAMERA_SRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_CAMERA_SRC, WebKitCameraSrcPrivate))
struct _WebKitCameraSrcPrivate {
    GstElement* appSrc;
    GstElement* capsFilter;

    GstPad* srcpad;
    gchar* uri;
    gboolean isRecording;

    gint cameraId;
    gint rotation;
    gint method;

    RotationManager* rotationManager;
};

enum {
    PROP_LOCATION = 1,
    PROP_IS_RECORDING = 2
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC(webkit_camera_src_debug);
#define GST_CAT_DEFAULT webkit_camera_src_debug

static void webKitCameraSrcUriHandlerInit(gpointer gIface, gpointer ifaceData);

static void webKitCameraSrcFinalize(GObject*);
static void webKitCameraSrcSetProperty(GObject*, guint propertyID, const GValue*, GParamSpec*);
static void webKitCameraSrcGetProperty(GObject*, guint propertyID, GValue*, GParamSpec*);
static GstStateChangeReturn webKitCameraSrcChangeState(GstElement*, GstStateChange);

static void webKitCameraSrcStop(WebKitCameraSrc*, bool);

#define webkit_camera_src_parent_class parent_class
// We split this out into another macro to avoid a check-webkit-style error.
#define WEBKIT_CAMERA_SRC_CATEGORY_INIT GST_DEBUG_CATEGORY_INIT(webkit_camera_src_debug, "webkitcamerasrc", 0, "camerasrc element");
G_DEFINE_TYPE_WITH_CODE(WebKitCameraSrc, webkit_camera_src, GST_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, webKitCameraSrcUriHandlerInit);
                         WEBKIT_CAMERA_SRC_CATEGORY_INIT);

static void webkit_camera_src_class_init(WebKitCameraSrcClass* klass)
{
    GObjectClass* oklass = G_OBJECT_CLASS(klass);
    GstElementClass* eklass = GST_ELEMENT_CLASS(klass);

    oklass->finalize = webKitCameraSrcFinalize;
    oklass->set_property = webKitCameraSrcSetProperty;
    oklass->get_property = webKitCameraSrcGetProperty;

    gst_element_class_add_pad_template(eklass,
                                       gst_static_pad_template_get(&srcTemplate));
#ifdef GST_API_VERSION_1
    gst_element_class_set_metadata(eklass,
#else
    gst_element_class_set_details_simple(eklass,
#endif
                                         (gchar*) "WebKit Camera source element",
                                         (gchar*) "Source",
                                         (gchar*) "Handles Camera uris",
                                         (gchar*) "Dong-gwan Kim <donggwan.kim@samsung.com>");

    /* Allows setting the uri using the 'location' property, which is used
     * for example by gst_element_make_from_uri() */
    g_object_class_install_property(oklass,
                                    PROP_LOCATION,
                                    g_param_spec_string("location",
                                                        "location",
                                                        "Location to read from",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(oklass,
                                    PROP_IS_RECORDING,
                                    g_param_spec_boolean("is-recording",
                                                        "is-recording",
                                                        "Is recording",
                                                        FALSE,
                                                        (GParamFlags) (G_PARAM_READWRITE)));
    eklass->change_state = webKitCameraSrcChangeState;

    g_type_class_add_private(klass, sizeof(WebKitCameraSrcPrivate));
}

static void newPadCallback(GstElement*, GstPad* pad, WebKitCameraSrc* src)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("");
#endif

    WebKitCameraSrcPrivate* priv = WEBKIT_CAMERA_SRC_GET_PRIVATE(src);
    gst_pad_link(pad, gst_element_get_static_pad(priv->capsFilter, "sink"));
}

static void webkit_camera_src_init(WebKitCameraSrc* src)
{
    GRefPtr<GstPadTemplate> padTemplate = adoptGRef(gst_static_pad_template_get(&srcTemplate));
    WebKitCameraSrcPrivate* priv = WEBKIT_CAMERA_SRC_GET_PRIVATE(src);
    src->priv = priv;

    LocalMediaServer::instance().add(src);
    priv->rotationManager = new RotationManager(src);

    priv->appSrc = gst_element_factory_make("appsrc", 0);
    LocalMediaServer::instance().addAppSrc(priv->appSrc);
    g_object_set(priv->appSrc, "is-live", 1, NULL);
    g_object_set(priv->appSrc, "max-bytes", 0, NULL);
    g_object_set(priv->appSrc, "do-timestamp", true, NULL);

    GstCaps* caps = gst_caps_new_simple("video/x-raw-yuv",
            "format", GST_TYPE_FOURCC, cameraFormat,
            "framerate", GST_TYPE_FRACTION, cameraFramerate, 1,
            "width", G_TYPE_INT, LocalMediaServer::instance().width(),
            "height", G_TYPE_INT, LocalMediaServer::instance().height(),
            "rotate", G_TYPE_INT, 0,
            NULL);

    priv->capsFilter = gst_element_factory_make("capsfilter", 0);
    g_object_set(G_OBJECT(priv->capsFilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(src), priv->appSrc, priv->capsFilter, NULL);
    g_signal_connect(priv->capsFilter, "pad-added", G_CALLBACK(newPadCallback), src);

    GRefPtr<GstPad> targetPad = adoptGRef(gst_element_get_static_pad(GST_ELEMENT(priv->capsFilter), "src"));
    priv->srcpad = gst_ghost_pad_new_from_template("src", targetPad.get(), padTemplate.get());
    gst_pad_set_active(priv->srcpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(src), priv->srcpad);

    gst_element_link(priv->appSrc, priv->capsFilter);

    webKitCameraSrcStop(src, false);
}

static void webKitCameraSrcFinalize(GObject* object)
{
    WebKitCameraSrc* src = WEBKIT_CAMERA_SRC(object);
    WebKitCameraSrcPrivate* priv = src->priv;

    LocalMediaServer::instance().remove(src);
    LocalMediaServer::instance().removeAppSrc(priv->appSrc);

    delete priv->rotationManager;
    g_free(priv->uri);

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, ((GObject* )(src)));
}

static void webKitCameraSrcSetProperty(GObject* object, guint propID, const GValue* value, GParamSpec* pspec)
{
    WebKitCameraSrc* src = WEBKIT_CAMERA_SRC(object);
    WebKitCameraSrcPrivate* priv = src->priv;

    switch (propID) {
    case PROP_LOCATION:
#ifdef GST_API_VERSION_1
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value), 0);
#else
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value));
#endif
        break;
    case PROP_IS_RECORDING:
        priv->isRecording = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propID, pspec);
        break;
    }
}

static void webKitCameraSrcGetProperty(GObject* object, guint propID, GValue* value, GParamSpec* pspec)
{
    WebKitCameraSrc* src = WEBKIT_CAMERA_SRC(object);
    WebKitCameraSrcPrivate* priv = src->priv;

    switch (propID) {
    case PROP_LOCATION:
        g_value_set_string(value, priv->uri);
        break;
    case PROP_IS_RECORDING:
        g_value_set_boolean(value, priv->isRecording);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propID, pspec);
        break;
    }
}


static void webKitCameraSrcStop(WebKitCameraSrc* src, bool)
{
    WebKitCameraSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);

    priv->cameraId = 0;
    priv->rotation = 0;

    GST_OBJECT_UNLOCK(src);

    GST_DEBUG_OBJECT(src, "Stopped request");
}

static bool webKitCameraSrcStart(WebKitCameraSrc* src)
{
    WebKitCameraSrcPrivate* priv = src->priv;

    if (!priv->uri) {
        GST_ERROR_OBJECT(src, "No URI provided");
        return false;
    }

    GST_OBJECT_LOCK(src);

    priv->cameraId = 0;
    priv->rotation = 0;

    GST_OBJECT_UNLOCK(src);

    GST_DEBUG_OBJECT(src, "Started request");

    return true;
}

static GstStateChangeReturn webKitCameraSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    WebKitCameraSrc* src = WEBKIT_CAMERA_SRC(element);

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (G_UNLIKELY(ret == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT(src, "State change failed");
        return ret;
    }

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_OBJECT(src, "READY->PAUSED");
        if (!webKitCameraSrcStart(src))
            ret = GST_STATE_CHANGE_FAILURE;
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(src, "PAUSED->READY");
        webKitCameraSrcStop(src, false);
        break;
    default:
        break;
    }

    return ret;
}

// uri handler interface

#ifdef GST_API_VERSION_1
static GstURIType webKitCameraSrcUriGetType(GType)
{
    return GST_URI_SRC;
}

const gchar* const* webKitCameraSrcGetProtocols(GType)
{
    static const char* protocols[] = {"camera", 0 };
    return protocols;
}

static gchar* webKitCameraSrcGetUri(GstURIHandler* handler)
{
    return g_strdup(WEBKIT_CAMERA_SRC(handler)->priv->uri);
}

static gboolean webKitCameraSrcSetUri(GstURIHandler* handler, const gchar* uri, GError** error)
{
    WebKitCameraSrc* src = WEBKIT_CAMERA_SRC(handler);
    WebKitCameraSrcPrivate* priv = src->priv;

    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    g_free(priv->uri);
    priv->uri = 0;

    if (!uri)
        return TRUE;

    int cameraId = 0;
    sscanf(uri, "camera://%d", &cameraId);

    priv->uri = g_strdup(uri);
    priv->cameraId = cameraId;

    return TRUE;
}

#else
static GstURIType webKitCameraSrcUriGetType(void)
{
    return GST_URI_SRC;
}

static gchar** webKitCameraSrcGetProtocols(void)
{
    static gchar* protocols[] = {(gchar*) "camera", 0 };
    return protocols;
}

static const gchar* webKitCameraSrcGetUri(GstURIHandler* handler)
{
    return g_strdup(WEBKIT_CAMERA_SRC(handler)->priv->uri);
}

static gboolean webKitCameraSrcSetUri(GstURIHandler* handler, const gchar* uri)
{
    WebKitCameraSrc* src = WEBKIT_CAMERA_SRC(handler);
    WebKitCameraSrcPrivate* priv = src->priv;

    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    g_free(priv->uri);
    priv->uri = 0;

    if (!uri)
        return TRUE;

    int cameraId = 0;
    sscanf(uri, "camera://%d", &cameraId);

    priv->uri = g_strdup(uri);
    priv->cameraId = cameraId;

    return TRUE;
}
#endif

static void webKitCameraSrcUriHandlerInit(gpointer gIface, gpointer)
{
    GstURIHandlerInterface* iface = (GstURIHandlerInterface *) gIface;

    iface->get_type = webKitCameraSrcUriGetType;
    iface->get_protocols = webKitCameraSrcGetProtocols;
    iface->get_uri = webKitCameraSrcGetUri;
    iface->set_uri = webKitCameraSrcSetUri;
}

RotationManager::RotationManager(WebKitCameraSrc* src) : m_src(src), m_listener(0)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
#if !ENABLE(TIZEN_WEARABLE_PROFILE)
    if (TizenExtensibleAPI::extensibleAPI().rotateCameraView())
        registerRotationCallback();
#endif
#endif // ENABLE(TIZEN_EXTENSIBLE_API)
}

RotationManager::~RotationManager()
{
    if (m_listener)
        unregisterRotationCallback();
}

void RotationManager::onRotationChanged(sensor_h, sensor_event_s* event, void* userData)
{
    RotationManager* manager = static_cast<RotationManager*>(userData);
    if (!manager->m_src)
        return;

    WebKitCameraSrcPrivate* priv = manager->m_src->priv;
    if (priv->isRecording)
        return;

    int autoRotateScreen = 0;
    vconf_get_bool(VCONFKEY_SETAPPL_AUTO_ROTATE_SCREEN_BOOL, &autoRotateScreen);
    if (!autoRotateScreen)
        return;

    int rotation = manager->rotation(event->values[0], event->values[1], event->values[2]);
    if (rotation == ROTATE_ERROR || rotation == priv->rotation)
        return;

    priv->rotation = rotation;
    manager->updateMethod();

    //FIXME: Rotate video using 'rotate' property of xvimagesink.
}

void RotationManager::registerRotationCallback()
{
    sensor_type_e type = SENSOR_ACCELEROMETER;
    sensor_h sensor;
    int error_code;
    bool supported;

    error_code = sensor_is_supported(type, &supported);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    if (!supported) {
        TIZEN_LOGE("Accelerometer is not supported");
        return;
    }

    error_code = sensor_get_default_sensor(type, &sensor);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    sensor_create_listener(sensor, &m_listener);
    sensor_listener_set_event_cb(m_listener, 0, onRotationChanged, this);
    sensor_listener_start(m_listener);
}

void RotationManager::unregisterRotationCallback()
{
    sensor_listener_unset_event_cb(m_listener);
    sensor_listener_stop(m_listener);
    sensor_destroy_listener(m_listener);
}

void RotationManager::updateMethod()
{
    WebKitCameraSrcPrivate* priv = m_src->priv;

    if (gst_element_factory_find("camerasrc")) {
        switch (priv->rotation) {
        case ROTATE_0:
            priv->method = 0;
            break;
        case ROTATE_90:
            priv->method = 1;
            break;
        case ROTATE_180:
            priv->method = 2;
            break;
        case ROTATE_270:
            priv->method = 3;
            break;
        default:
            break;
        }
    } else if (gst_element_factory_find("qcamerasrc")) {
        switch (priv->rotation) {
        case ROTATE_0:
            priv->method = 3;
            break;
        case ROTATE_90:
            priv->method = 2;
            break;
        case ROTATE_180:
            priv->method = 1;
            break;
        case ROTATE_270:
            priv->method = 0;
            break;
        default:
            break;
        }
    }
}

int RotationManager::rotation(float x, float y, float z)
{
    double atanV, normZ, rawZ;
    int accTheta, accPitch;
    int rotation;

    atanV = atan2(y, x);
    accTheta = (int)(atanV * (RADIAN_VALUE) + 270) % 360;
    rawZ = (double)(z / (0.004 * 9.81));

    if (rawZ > 250)
        normZ = 1.0;
    else if (rawZ < -250)
        normZ = -1.0;
    else
        normZ = ((double)rawZ) / 250;

    accPitch = (int)(acos(normZ) * (RADIAN_VALUE));

    if ((accPitch > 35) && (accPitch < 145)) {
        if ((accTheta >= 315 && accTheta <= 359) || (accTheta >= 0 && accTheta < 45))
            rotation = ROTATE_0;
        else if (accTheta >= 45 && accTheta < 135)
            rotation = ROTATE_90;
        else if (accTheta >= 135 && accTheta < 225)
            rotation = ROTATE_180;
        else if (accTheta >= 225 && accTheta < 315)
            rotation = ROTATE_270;
        else
            rotation = ROTATE_ERROR;
    } else
        rotation = ROTATE_ERROR;

    return rotation;
}

#endif // ENABLE(TIZEN_MEDIA_STREAM)

