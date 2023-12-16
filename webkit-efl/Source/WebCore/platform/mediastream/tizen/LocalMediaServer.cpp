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
#include "LocalMediaServer.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "GStreamerVersioning.h"
#include <wtf/text/CString.h>

#define HOST_ADDRESS "127.0.0.1"
#define HOST_PORT 8888

namespace WebCore {

static gboolean localMediaServerMessageCallback(GstBus*, GstMessage* message, LocalMediaServer* mediaServer)
{
    return mediaServer->handleMessage(message);
}

CameraDeviceCapabilities::CameraDeviceCapabilities()
{
    initialize();
}

CameraDeviceCapabilities& CameraDeviceCapabilities::instance()
{
    static CameraDeviceCapabilities capabilities;
    return capabilities;
}

bool cameraSupportedCaptureFormatCallback(camera_pixel_format_e format, void* userData)
{
    CameraDeviceCapabilities* capabilities = static_cast<CameraDeviceCapabilities*>(userData);
    capabilities->addSupportedCaptureFormat(format);
    return true;
}

bool cameraSupportedCaptureResolutionCallback(int width, int height, void* userData)
{
    CameraDeviceCapabilities* capabilities = static_cast<CameraDeviceCapabilities*>(userData);
    capabilities->addSupportedCaptureResolution(IntSize(width, height));
    return true;
}

void CameraDeviceCapabilities::initialize()
{
    camera_h camera = 0;
    camera_create(CAMERA_DEVICE_CAMERA1, &camera);
    if (camera) {
        camera_foreach_supported_capture_format(camera, cameraSupportedCaptureFormatCallback, this);
        camera_foreach_supported_capture_resolution(camera, cameraSupportedCaptureResolutionCallback, this);
        camera_destroy(camera);
    }
}

LocalMediaServer::LocalMediaServer()
    : m_camPipeline(0)
    , m_camSource(0)
    , m_capsFilter(0)
    , m_tcpServerSink(0)
    , m_stoped(true)
    , m_changingResolution(false)
    , m_width(cameraWidth)
    , m_height(cameraHeight)
    , m_srcType(NullSrc)
{
    if (!gst_is_initialized())
        gst_init_check(0, 0, 0);
}

LocalMediaServer::~LocalMediaServer()
{
    clear();
}

void LocalMediaServer::clear()
{
    if (m_camPipeline) {
        TIZEN_LOGI("#START");
        stopStream();
        gst_object_unref(GST_OBJECT(m_camPipeline));
        m_camPipeline = 0;
        TIZEN_LOGI("#END");
    }
}

bool LocalMediaServer::createPipeline()
{
    m_camPipeline = gst_pipeline_new(0);
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_camPipeline));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(localMediaServerMessageCallback), this);

    GstElement* src = 0;

    if (gst_element_factory_find("camerasrc")) {
        CameraDeviceCapabilities& cameraDeviceCapabilities = CameraDeviceCapabilities::instance();
        IntSize maxCaptureSize = cameraDeviceCapabilities.supportedCaptureResolutions().last();

        src = gst_element_factory_make("camerasrc", 0);
        g_object_set(src, "capture-width", maxCaptureSize.width(), NULL);
        g_object_set(src, "capture-height", maxCaptureSize.height(), NULL);
        g_object_set(src, "video-width", cameraWidth, NULL);
        g_object_set(src, "video-Height", cameraHeight, NULL);
        m_srcType = CameraSrc;
    } else if (gst_element_factory_find("qcamerasrc")) {
        src = gst_element_factory_make("qcamerasrc", 0);
        m_srcType = QCameraSrc;
    } else if (gst_element_factory_find("v4l2src")) {
        src = gst_element_factory_make("v4l2src", 0);
        m_srcType = V4l2Src;
    }

    if (!src) {
        TIZEN_LOGE("Fail to create src element for camera");
        return false;
    }

    m_camSource = src;
    g_object_set(src, "camera-id", 1, NULL);

    // caps Filter to make caps data from camera.
    GstCaps* caps = gst_caps_new_simple("video/x-raw-yuv",
            "format", GST_TYPE_FOURCC, cameraFormat,
            "framerate", GST_TYPE_FRACTION, cameraFramerate, 1,
            "width", G_TYPE_INT, cameraWidth,
            "height", G_TYPE_INT, cameraHeight,
            "rotate", G_TYPE_INT, 0,
            NULL);

    m_capsFilter = gst_element_factory_make("capsfilter", 0);
    g_object_set(G_OBJECT(m_capsFilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    GstElement* videoSink = gst_element_factory_make("fakesink", "fakesink");
    g_object_set(videoSink, "rotate", 0, NULL);
    m_tcpServerSink = videoSink;

    GstElement* queue = gst_element_factory_make("queue", 0);

    gst_bin_add_many(GST_BIN(m_camPipeline), src, m_capsFilter, queue, m_tcpServerSink, NULL);
    gst_element_link_many(src, m_capsFilter, queue, m_tcpServerSink, NULL);
    if (gst_element_set_state(m_camPipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        TIZEN_LOGE("Fail to set state as PLAYING");
        gst_element_set_state(m_camPipeline, GST_STATE_NULL);
        return false;
    }

    GRefPtr<GstPad> srcpad = adoptGRef(gst_element_get_static_pad(queue, "src"));
    gst_pad_add_buffer_probe(srcpad.get(), G_CALLBACK(queueSrcBufferProbeCallback), static_cast<gpointer>(this));
    return true;
}

gboolean LocalMediaServer::queueSrcBufferProbeCallback(GstPad*, GstBuffer* buffer, gpointer userData)
{
    LocalMediaServer* localMediaServer = static_cast<LocalMediaServer*>(userData);
    GstFlowReturn ret;

    for (unsigned i = 0; i < localMediaServer->m_appSrcs.size(); i++) {
        GstState currentState;
        gst_element_get_state(localMediaServer->m_appSrcs[i], &currentState, 0, 0);
        if (currentState == GST_STATE_PLAYING)
            g_signal_emit_by_name(localMediaServer->m_appSrcs[i], "push-buffer", buffer, &ret);
    }

    return true;
}

void LocalMediaServer::removeAppSrc(GstElement* appSrc)
{
    if (m_appSrcs.contains(appSrc))
        m_appSrcs.remove(m_appSrcs.find(appSrc));
}

LocalMediaServer& LocalMediaServer::instance()
{
    DEFINE_STATIC_LOCAL(LocalMediaServer, mediaServer, ());
    return static_cast<LocalMediaServer&>(mediaServer);
}

bool LocalMediaServer::startStream()
{
    if (!m_camPipeline) {
        if (!createPipeline()) {
            gst_object_unref(GST_OBJECT(m_camPipeline));
            m_camPipeline = 0;
            return false;
        }
    } else
        gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);

    m_stoped = false;
    return true;
}

void LocalMediaServer::stopStream()
{
    m_stoped = true;

    if (m_camPipeline) {
        // Stop sink first to release buffer.
        // If not, pipeline's transition will take long time.
        gst_element_set_state(m_tcpServerSink, GST_STATE_NULL);
        gst_element_set_state(m_camPipeline, GST_STATE_NULL);
    }
}

void LocalMediaServer::resetStream()
{
    if (!m_camPipeline)
        return;

    TIZEN_LOGI("RESET STREAM.");

    gst_element_set_state(m_tcpServerSink, GST_STATE_NULL);
    gst_element_set_state(m_camPipeline, GST_STATE_NULL);

    // Flush message.
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_camPipeline));
    gst_bus_set_flushing(bus.get(), TRUE);
    gst_bus_set_flushing(bus.get(), FALSE);

    gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);

    // Waiting for state change.
    GstState currentState;
    gst_element_get_state(m_camPipeline, &currentState, 0, stateChangeTimeout * GST_MSECOND);
    if (currentState != GST_STATE_PLAYING) {
        TIZEN_LOGE("Fail to change state.");
    }

    HashMap<WebKitCameraSrc*, activateState> clients = m_clients;
    HashMap<WebKitCameraSrc*, activateState>::iterator iter = clients.begin();
    for (; iter != clients.end(); ++iter) {
        GstElement* current = GST_ELEMENT(iter->key);
        GstElement* parent = (GstElement*)gst_element_get_parent(current);

        if (!parent)
            continue;

        while(parent) {
            current = parent;
            parent = (GstElement*)gst_element_get_parent(current);
        }
        gst_element_set_state(current, GST_STATE_NULL);
        gst_element_set_state(current, GST_STATE_PLAYING);
    }
}

void LocalMediaServer::add(WebKitCameraSrc* client)
{
    m_clients.add(client, Initiated);
}

void LocalMediaServer::remove(WebKitCameraSrc* client)
{
    m_clients.remove(client);
    /* FIXME : If suspended here, LocalMediaServer will not be started before "startStream" function is called.
    suspendIfNecessary();
    */
}

void LocalMediaServer::stateChanged(WebKitCameraSrc* client, GstStateChange stateChange)
{
    if (stateChange == GST_STATE_CHANGE_NULL_TO_READY) {
        m_clients.set(client, Preroll);
        if (!m_stoped)
            gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);
    } else if (stateChange == GST_STATE_CHANGE_READY_TO_PAUSED) {
        m_clients.set(client, Activate);
    } else if (stateChange == GST_STATE_CHANGE_PAUSED_TO_READY) {
        m_clients.set(client, Inactive);
        suspendIfNecessary();
    }
}

void LocalMediaServer::suspendIfNecessary()
{
    if (!m_camPipeline)
        return;

    HashMap<WebKitCameraSrc*, activateState>::iterator iter = m_clients.begin();
    for (; iter != m_clients.end(); ++iter) {
        if (iter->value != Inactive)
            return;
    }
    gst_element_set_state(m_camPipeline, GST_STATE_NULL);
}

gboolean LocalMediaServer::handleMessage(GstMessage* message)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        TIZEN_LOGI("Stream ERROR.");
        resetStream();
        break;
    case GST_MESSAGE_STATE_CHANGED:
        // Ignore state changes from internal elements.
        // They are forwarded to pipeline anyway.
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_camPipeline)) {
            GstState newState, oldState;
            gst_message_parse_state_changed(message, &oldState, &newState, 0);

            CString dotFileName = String::format("webkit-localMediaServer.%s_%s",
                                                 gst_element_state_get_name(oldState),
                                                 gst_element_state_get_name(newState)).utf8();

            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_camPipeline), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.data());
            if (newState == GST_STATE_PLAYING) {
                HashMap<WebKitCameraSrc*, activateState>::iterator iter = m_clients.begin();
                for (; iter != m_clients.end(); ++iter)
                    if (iter->value == Preroll)
                        gst_element_set_state(GST_ELEMENT(iter->key), GST_STATE_PLAYING);
            } else if (m_srcType == V4l2Src
                    && newState == GST_STATE_READY
                    && m_changingResolution) {
                GstCaps* caps = gst_caps_new_simple("video/x-raw-yuv",
                        "format", GST_TYPE_FOURCC, cameraFormat,
                        "framerate", GST_TYPE_FRACTION, cameraFramerate, 1,
                        "width", G_TYPE_INT, m_width,
                        "height", G_TYPE_INT, m_height,
                        NULL);

                g_object_set(G_OBJECT(m_capsFilter), "caps", caps, NULL);
                gst_caps_unref(caps);
                gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);
                m_changingResolution = false;
            }
        }
        break;
    default:
        break;
    }

    return TRUE;
}

void LocalMediaServer::changeResolution(int width, int height)
{
    if (m_srcType != V4l2Src)
        return;

    if (m_width == width && m_height == height)
        return;

    GstState currentState;
    gst_element_get_state(m_camPipeline, &currentState, 0, 0);
    if (currentState != GST_STATE_NULL)
        gst_element_set_state(m_camPipeline, GST_STATE_READY);

    m_changingResolution = true;
    m_width = width;
    m_height = height;
}

#if ENABLE(TIZEN_EMULATOR)
bool LocalMediaServer::audioStreamIsAvailable() const
{
    GstElement* pipeline = gst_pipeline_new(0);
    if (!pipeline)
        return false;

    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(pipeline));
    gst_bus_add_signal_watch(bus.get());

    GstElement* audioSrc = gst_element_factory_make("avsysaudiosrc", 0);
    GstElement* fakesink = gst_element_factory_make("fakesink", 0);
    gst_bin_add_many(GST_BIN(pipeline), audioSrc, fakesink, NULL);
    gst_element_link(audioSrc, fakesink);

    gst_element_set_state(pipeline, GST_STATE_READY);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstState currentState;
    gst_element_get_state(pipeline, &currentState, 0, stateChangeTimeout * GST_MSECOND);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_bus_remove_signal_watch(bus.get());
    gst_object_unref(pipeline);

    return currentState == GST_STATE_PLAYING;
}

bool LocalMediaServer::camPipelineStateIsPlaying(GstElement* pipeline) const
{
    if (!pipeline)
        return false;

    GstState currentState;
    size_t count = 0;

    // re-check camPipeline state three times unless it is PLAYING.
    while (count < 3) {
        gst_element_get_state(pipeline, &currentState, 0, stateChangeTimeout * GST_MSECOND);
        if (currentState == GST_STATE_PLAYING)
            return true;

        count++;
    }
    return false;
}

#endif // #if ENABLE(TIZEN_EMULATOR)

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
