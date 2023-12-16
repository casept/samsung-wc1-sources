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
#include "MediaStreamPlayer.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaStreamComponent.h"
#include "MediaStreamDescriptor.h"
#include "MediaStreamSource.h"
#include <vconf.h>

#define FAKE_CAMERA_CAPS_0 "video/x-raw-yuv, format=(fourcc)I420, width = (int) 320, height = (int) 240, framerate=(fraction)30/1"
#define FAKE_CAMERA_CAPS_1 "video/x-raw-rgb, bpp=(int)32, depth=(int)24, endianness=(int)4321, red_mask=(int)65280, green_mask=(int)16711680, blue_mask=(int)-16777216, format=(fourcc)BGRx, width = (int) 320, height = (int) 240, framerate=(fraction)30/1"
#define CAMERA0_CAPS_0 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 240, height = (int) 320, framerate=(fraction)30/1, rotate=90"
#define CAMERA0_CAPS_90 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 320, height = (int) 240, framerate=(fraction)30/1, rotate=0"
#define CAMERA0_CAPS_180 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 240, height = (int) 320, framerate=(fraction)30/1, rotate=270"
#define CAMERA0_CAPS_270 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 320, height = (int) 240, framerate=(fraction)30/1, rotate=180"
#define CAMERA1_CAPS_0 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 240, height = (int) 320, framerate=(fraction)30/1, rotate=270"
#define CAMERA1_CAPS_90 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 320, height = (int) 240, framerate=(fraction)30/1, rotate=0"
#define CAMERA1_CAPS_180 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 240, height = (int) 320, framerate=(fraction)30/1, rotate=90"
#define CAMERA1_CAPS_270 "video/x-raw-yuv, format=(fourcc)SN12, width = (int) 320, height = (int) 240, framerate=(fraction)30/1, rotate=180"
#define ACAPS "audio/x-raw-int, endianness=(int)1234, signed=(boolean)true, width=(int)16, depth=(int)16, rate=(int)44100, channels=(int)2"

#define RADIAN_VALUE (57.2957)

enum {
    ROTATE_0,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270,
    ROTATE_ERROR
};

namespace WebCore {

MediaStreamPlayer& MediaStreamPlayer::instance()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(MediaStreamPlayer, player, ());
    return player;
}

MediaStreamPlayer::MediaStreamPlayer()
    : m_callback(0)
    , m_listener(0)
    , m_pipeline(0)
    , m_vcaps(0)
    , m_cameraId(0)
    , m_rotation(ROTATE_0)
    , m_previewMode(PREVIEW_MODE_NONE)
    , m_audioMuted(true)
    , m_isCaptured(false)
    , m_isRecording(false)
{
    static bool initGst = false;
    if (!initGst)
        initGst = gst_init_check(0, 0, 0);
    registerRotationCallback(onRotationChanged);
}

MediaStreamPlayer::~MediaStreamPlayer()
{
    unregisterRotationCallback();
    deletePipeline();
}

void MediaStreamPlayer::onRotationChanged(sensor_h, sensor_event_s* event, void* userData)
{
    MediaStreamPlayer* that = static_cast<MediaStreamPlayer*>(userData);
    if ((!that->m_pipeline) || (that->m_isRecording))
        return;

    int autoRotationLock = 0;
    vconf_get_bool(VCONFKEY_SETAPPL_ROTATE_LOCK_BOOL, &autoRotationLock);
    if (autoRotationLock)
        return;

    int rotation = that->rotation();
    if (rotation == ROTATE_ERROR || rotation == that->m_rotation)
        return;

    that->m_rotation = rotation;
    that->deletePipeline();
    that->createPipeline();

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(that->m_pipeline, GST_STATE_PLAYING))
        g_warning("can't set pipeline to playing");

}

gboolean MediaStreamPlayer::onBusCallbackCalled(GstBus* bus, GstMessage* message, void* data)
{
    MediaStreamPlayer* that = static_cast<MediaStreamPlayer*>(data);
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError* err;
        gchar* debug;

        gst_message_parse_error(message, &err, &debug);
        g_print("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);
        break;
    }
    case GST_MESSAGE_EOS:
        if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(that->m_pipeline, GST_STATE_NULL))
            g_warning("can't set pipeline to null");

        gst_object_unref(that->m_pipeline);
        that->m_pipeline = 0;
        that->startCapture();
      break;
    default:
      break;
    }

    return TRUE;
}

void MediaStreamPlayer::onCameraVideoStreamReceived(GstElement* sink, GstBuffer* buffer, GstPad* pad, MediaStreamPlayer* that)
{
    if (!that->m_callback)
        return;

    gint width, height;
    guint32 format;
    gdouble rate;

    GstCaps* caps = gst_pad_get_negotiated_caps(pad);
    GstStructure* str = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(str, "width", &width);
    gst_structure_get_int(str, "height", &height);
    gst_structure_get_fourcc(str, "format", &format);
    gst_structure_get_double(str, "framerate", &rate);
    gst_caps_unref(caps);

    that->m_callback->OnVideoCaptured((unsigned char*)buffer->data, buffer->size, width, height);
}

void MediaStreamPlayer::registerRotationCallback()
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
    sensor_listener_set_event_cb(m_listener, onRotationChanged, this);
    sensor_listener_start(m_listener);
}

void MediaStreamPlayer::unregisterRotationCallback()
{
    sensor_listener_unset_event_cb(m_listener);
    sensor_listener_stop(m_listener);
    sensor_destroy_listener(m_listener);
}

void MediaStreamPlayer::updateVideoCaps()
{
    if (m_vcaps)
        free(m_vcaps);

    switch (m_rotation) {
    case ROTATION_EVENT_0:
        m_vcaps = m_cameraId ? strdup(CAMERA1_CAPS_0) : strdup(CAMERA0_CAPS_0);
        break;
    case ROTATION_EVENT_90:
        m_vcaps = m_cameraId ? strdup(CAMERA1_CAPS_90) : strdup(CAMERA0_CAPS_90);
        break;
    case ROTATION_EVENT_180:
        m_vcaps = m_cameraId ? strdup(CAMERA1_CAPS_180) : strdup(CAMERA0_CAPS_180);
        break;
    case ROTATION_EVENT_270:
        m_vcaps = m_cameraId ? strdup(CAMERA1_CAPS_270) : strdup(CAMERA0_CAPS_270);
        break;
    default:
        break;
    }
}

void MediaStreamPlayer::createPipeline()
{
    if (m_pipeline)
        return;

    gchar* nVsrc = 0;
    GstCaps* caps = 0;
    m_pipeline = gst_pipeline_new(0);
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_add_watch(bus, onBusCallbackCalled, this);
    gst_object_unref(bus);
    updateVideoCaps();

    if (!m_isRecording) {
        if (m_previewMode && PREVIEW_MODE_VIDEO_ONLY) {
            m_vsrc = gst_element_factory_make("camerasrc", "real-camera");
            if (!m_vsrc) {
                m_vsrc = gst_element_factory_make("videotestsrc", "fake-camera");
                m_vconvert = gst_element_factory_make("queue", 0);
            } else {
                m_vconvert = gst_element_factory_make("fimcconvert", 0);

                g_object_set(G_OBJECT(m_vsrc), "camera-id", m_cameraId, NULL);
                g_object_set(G_OBJECT(m_vsrc), "hflip", m_cameraId, NULL);
            }
            m_vfilter = gst_element_factory_make("capsfilter", "filter");
            m_vsink = gst_element_factory_make("fakesink", 0);
            g_object_set(m_vsink, "sync", false, NULL);
            g_object_set(G_OBJECT(m_vsink), "async", false, NULL);
            g_signal_connect(m_vsink, "handoff", G_CALLBACK(onCameraVideoStreamReceived), this);

            nVsrc = gst_element_get_name(m_vsrc);
            if (g_str_equal(nVsrc, "real-camera"))
                caps = gst_caps_from_string(m_vcaps);
            else
                caps = gst_caps_from_string(FAKE_CAMERA_CAPS_1);
            g_free(nVsrc);

            g_object_set(G_OBJECT(m_vfilter), "caps", caps, NULL);
            gst_caps_unref(caps);
            gst_bin_add_many(GST_BIN(m_pipeline), m_vsrc, m_vfilter, m_vconvert, m_vsink, NULL);
            gst_element_link_many(m_vsrc, m_vfilter, m_vconvert, m_vsink, NULL);
        }

        if (m_previewMode && PREVIEW_MODE_AUDIO_ONLY) {
            m_asrc = gst_element_factory_make("pulsesrc", 0);
            m_afilter = gst_element_factory_make("capsfilter", 0);
            m_asink = gst_element_factory_make("pulsesink", 0);

            caps = gst_caps_from_string(ACAPS);
            g_object_set(G_OBJECT(m_afilter), "caps", caps, NULL);
            gst_caps_unref(caps);

            g_object_set(G_OBJECT(m_asink), "mute", true, NULL);

            gst_bin_add_many(GST_BIN(m_pipeline), m_asrc, m_afilter, m_asink, NULL);
            gst_element_link_many(m_asrc, m_afilter, m_asink, NULL);
        }
    } else {
            GstPad* pvt, *pte, *pt0, *pt1, *pq0, *pq1, *pq1_, *pae, *pma, *pmv, *pve, *pve_;
            GDateTime* dt;
            GTimeZone* tz;
            gchar* tstr;

            m_vsrc = gst_element_factory_make("camerasrc", "real-camera");
            m_vfilter = gst_element_factory_make("capsfilter", "filter");
            if (!m_vsrc) {
                m_vsrc = gst_element_factory_make("videotestsrc", "fake-camera");
                m_vconvert = gst_element_factory_make("ffmpegcolorspace", 0);
                caps = gst_caps_from_string(FAKE_CAMERA_CAPS_0);
            } else {
                g_object_set(G_OBJECT(m_vsrc), "camera-id", m_cameraId, "hflip", m_cameraId, NULL);
                m_vconvert = gst_element_factory_make("fimcconvert", 0);
                caps = gst_caps_from_string(m_vcaps);
            }
            g_object_set(G_OBJECT(m_vfilter), "caps", caps, NULL);
            gst_caps_unref(caps);
            m_vsink = gst_element_factory_make("fakesink", 0);
            g_object_set(G_OBJECT(m_vsink), "sync", false, "async", false, NULL);
            g_signal_connect(m_vsink, "handoff", G_CALLBACK(onCameraVideoStreamReceived), this);

            gst_bin_add_many(GST_BIN(m_pipeline), m_vsrc, m_vfilter, m_vconvert, m_vsink, NULL);
            gst_element_link_many(m_vsrc, m_vfilter, NULL);

            m_asrc = gst_element_factory_make("pulsesrc", 0);
            m_afilter = gst_element_factory_make("capsfilter", 0);

            caps = gst_caps_from_string(ACAPS);
            g_object_set(G_OBJECT(m_afilter), "caps", caps, NULL);
            gst_caps_unref(caps);

            gst_bin_add_many(GST_BIN(m_pipeline), m_asrc, m_afilter, NULL);
            gst_element_link_many(m_asrc, m_afilter, NULL);

            nVsrc = gst_element_get_name(m_vsrc);
            if (g_str_equal(nVsrc, "real-camera")) {
                m_venc = gst_element_factory_make("omx_h264enc", 0);
                m_aenc = gst_element_factory_make("savsenc_aac", 0);
            } else {
                m_venc = gst_element_factory_make("ffenc_mpeg4", 0);
                m_aenc = gst_element_factory_make("faac", 0);
            }

            m_tee = gst_element_factory_make("tee", 0);
            m_q0 = gst_element_factory_make("queue", 0);
            m_q1 = gst_element_factory_make("queue", 0);
            m_mux = gst_element_factory_make("ffmux_mp4", 0);
            m_fsink = gst_element_factory_make("filesink", 0);
            tz = g_time_zone_new_local();
            dt = g_date_time_new_now(tz);
            tstr = g_date_time_format(dt, "/opt/media/Videos/%s.mp4");
            m_recordedFileName = String(tstr);
            g_object_set(G_OBJECT(m_fsink), "location", tstr, NULL);
            g_date_time_unref(dt);
            g_time_zone_unref(tz);
            g_free(tstr);

            gst_bin_add_many(GST_BIN(m_pipeline), m_venc, m_aenc, m_tee, m_q0, m_q1, m_mux, m_fsink, NULL);
            gst_element_link(m_afilter, m_aenc);

            pvt = gst_element_get_static_pad(m_vfilter, "src");
            pte = gst_element_get_static_pad(m_tee, "sink");
            pt0 = gst_element_get_request_pad(m_tee, "src%d");
            pt1 = gst_element_get_request_pad(m_tee, "src%d");
            pq0 = gst_element_get_static_pad(m_q0, "sink");
            pq1 = gst_element_get_static_pad(m_q1, "sink");
            pq1_ = gst_element_get_static_pad(m_q1, "src");
            pve = gst_element_get_static_pad(m_venc, "sink");
            pve_ = gst_element_get_static_pad(m_venc, "src");
            pae = gst_element_get_static_pad(m_aenc, "src");
            pma = gst_element_get_request_pad(m_mux, "audio_0");
            pmv = gst_element_get_request_pad(m_mux, "video_0");

            gst_pad_link(pvt, pte);
            gst_pad_link(pt0, pq0);
            gst_pad_link(pt1, pq1);

            if (g_str_equal(nVsrc, "real-camera"))
                gst_element_link_many(m_q0, m_vconvert, m_vsink, NULL);
            else  {
                caps = gst_caps_from_string(FAKE_CAMERA_CAPS_1);
                gst_element_link(m_q0, m_vconvert);
                gst_element_link_filtered(m_vconvert, m_vsink, caps);
                gst_caps_unref(caps);
            }
            g_free(nVsrc);

            gst_pad_link(pae, pma);
            gst_pad_link(pq1_, pve);
            gst_pad_link(pve_, pmv);
            gst_element_link(m_mux, m_fsink);

            gst_object_unref(pvt);
            gst_object_unref(pte);
            gst_object_unref(pt0);
            gst_object_unref(pt1);
            gst_object_unref(pq0);
            gst_object_unref(pq1);
            gst_object_unref(pq1_);
            gst_object_unref(pve);
            gst_object_unref(pve_);
            gst_object_unref(pae);
            gst_object_unref(pma);
            gst_object_unref(pmv);
    }

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_READY))
        g_warning("can't set pipeline to ready");
}

void MediaStreamPlayer::deletePipeline()
{
    if (!m_pipeline)
        return;

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_NULL))
        g_warning("can't set pipeline to null");

    gst_object_unref(m_pipeline);
    m_pipeline = 0;
}

void MediaStreamPlayer::startCapture()
{
    createPipeline();

    g_object_set(G_OBJECT(m_vsink), "signal-handoffs", TRUE, NULL);
    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_PLAYING)) {
        g_warning("can't set audio pipeline to playing");
        return;
    }
    m_isCaptured = true;
}

void MediaStreamPlayer::stopCapture()
{
    if (!m_pipeline)
        return;

    g_object_set(G_OBJECT(m_vsink), "signal-handoffs", FALSE, NULL);
    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_NULL)) {
        g_warning("can't set audio pipeline to null");
        return;
    }
    m_isCaptured = false;

    if (GST_STATE_CHANGE_FAILURE == gst_element_get_state(m_pipeline, 0, 0, 0))
        g_warning("can't get pipeline status");

    if (m_isRecording) {
        gst_object_unref(m_pipeline);
        m_pipeline = 0;
    }
}

void MediaStreamPlayer::addCallback(MediaStreamCallback* callback)
{
    m_callback = callback;
}

void MediaStreamPlayer::start(MediaStreamDescriptor* descriptor)
{
    for (unsigned int i = 0; i < descriptor->numberOfAudioComponents(); i++)
        m_previewMode |= PREVIEW_MODE_AUDIO_ONLY;

    for (unsigned int i = 0; i < descriptor->numberOfVideoComponents(); i++) {
        MediaStreamSource* source = descriptor->videoComponent(i)->source();
        m_cameraId = (source->name() == "Self camera") ? 1 : 0;
        m_previewMode |= PREVIEW_MODE_VIDEO_ONLY;
    }

    if (m_previewMode != PREVIEW_MODE_NONE)
        startCapture();
}

void MediaStreamPlayer::stop()
{
    if (m_isCaptured)
        stopCapture();
}

void MediaStreamPlayer::setAudioMute(bool muted)
{
    if ((!m_pipeline) || (!m_asink))
        return;

    m_audioMuted = muted;
    if (m_audioMuted)
        g_object_set(G_OBJECT(m_asink), "mute", true, NULL);
    else
        g_object_set(G_OBJECT(m_asink), "mute", false, NULL);
}

void MediaStreamPlayer::toggleCamera()
{
    if ((!m_pipeline) || (m_isRecording))
        return;

    m_cameraId = m_cameraId ? 0 : 1;
    deletePipeline();
    createPipeline();

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_PLAYING))
        g_warning("can't set pipeline to playing");
}

void MediaStreamPlayer::record(MediaStreamDescriptor* descriptor)
{
    for (unsigned int i = 0; i < descriptor->numberOfAudioComponents(); i++)
        m_recordMode |= RECORD_MODE_AUDIO_ONLY;

    for (unsigned int i = 0; i < descriptor->numberOfVideoComponents(); i++) {
        MediaStreamSource* source = descriptor->videoComponent(i)->source();
        m_cameraId = (source->name() == "Self camera") ? 1 : 0;
        m_recordMode |= RECORD_MODE_VIDEO_ONLY;
    }

    m_isRecording = true;
    if (m_recordMode != RECORD_MODE_NONE)
        startCapture();

}

void MediaStreamPlayer::save(String& filename)
{
    gst_element_send_event(m_pipeline, gst_event_new_eos());
    filename.swap(m_recordedFileName);
}

int MediaStreamPlayer::rotation(float x, float y, float z)
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


} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
