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
#include "MediaRecorderPrivateImpl.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "FileSystem.h"
#include "GStreamerUtilities.h"
#include "GStreamerVersioning.h"
#include "LocalMediaServer.h"
#include "MediaRecorder.h"
#include "MediaStreamComponent.h"
#include "MediaStreamDescriptor.h"
#include "MediaStreamSource.h"
#include "WebKitCameraSourceGStreamerTizen.h"

#define ACAPS "audio/x-raw-int, endianness=(int)1234, signed=(boolean)true, width=(int)16, depth=(int)16, rate=(int)8000, channels=(int)1"
#define ACAPS_HIGH "audio/x-raw-int, endianness=(int)1234, signed=(boolean)true, width=(int)16, depth=(int)16, rate=(int)16000, channels=(int)1"
#define ACAPS_AAC "audio/x-raw-int, endianness=(int)1234, signed=(boolean)true, width=(int)16, depth=(int)16, rate=(int)32000, channels=(int)1"
#define AUDIO_FILE_PATH "/opt/usr/media/Sounds/"
#define VIDEO_FILE_PATH "/opt/usr/media/Videos/"
#define MEDIA_DIR_PATH "/opt/usr/media"

namespace WebCore {

static MediaRecorderFormatManagerMap& createMediaRecorderFormatManagerMap()
{
#if ENABLE(TIZEN_EMULATOR)
    static const char* gstH263Encoder = "maru_h263penc";
    static const char* gstAmrnbEncoder = "amrnbenc";
    static const char* gstAudioBooster = "";
    static const char* gstAACEncoder = "maru_aacenc";
#else
    static const char* gstH263Encoder = "omx_h264enc";
    static const char* gstAmrnbEncoder = "savsenc_amrnb";
    static const char* gstAudioBooster = "secrecord";
    static const char* gstAACEncoder = "savsenc_aac";
#endif
    static const char* gstVideoSource = "webkitcamerasrc";
    static const char* gstAudioSource = "avsysaudiosrc";
    static const char* gstFileSink = "filesink";
    static const char* gstQueue = "queue";
    static const char* gstAudioFilter = "capsfilter";
    static const char* gstMuxerFor3GP = "ffmux_3gp";
    static const char* gstMuxerForAMR = "ffmux_amr";
    static const char* gstMuxerForMP4 = "mp4mux";
    static const char* gstMuxerForAAC = "ffmux_adts";

    static MediaRecorderFormatManagerMap formatMap;

    // Initialize 3GP file format.
    OwnPtr<MediaRecorderFormatManager> mediaRecorderFormatManager3GP = adoptPtr(new MediaRecorderFormatManager());
    mediaRecorderFormatManager3GP->initializeRecorderFormat(ACAPS,
            gstVideoSource, gstH263Encoder, gstQueue,
            gstAudioSource, gstAudioFilter, gstAudioBooster, gstAmrnbEncoder, gstQueue,
            gstMuxerFor3GP, gstFileSink);
    formatMap.set(String("3gp"), mediaRecorderFormatManager3GP.release());

    // Initialize AMR file format.
    OwnPtr<MediaRecorderFormatManager> mediaRecorderFormatManagerAMR = adoptPtr(new MediaRecorderFormatManager());
    mediaRecorderFormatManagerAMR->initializeRecorderFormat(ACAPS,
            0, 0, 0,
            gstAudioSource, gstAudioFilter, gstAudioBooster, gstAmrnbEncoder, gstQueue,
            gstMuxerForAMR, gstFileSink);
    formatMap.set(String("amr"), mediaRecorderFormatManagerAMR.release());

    // Initialize PCM file format.
    OwnPtr<MediaRecorderFormatManager> mediaRecorderFormatManagerPCM = adoptPtr(new MediaRecorderFormatManager());
    mediaRecorderFormatManagerPCM->initializeRecorderFormat(ACAPS_HIGH,
            0, 0, 0,
            gstAudioSource, gstAudioFilter, gstAudioBooster, 0, gstQueue,
            0, gstFileSink);
    formatMap.set(String("pcm"), mediaRecorderFormatManagerPCM.release());

    // Initialize M4A file format.
    OwnPtr<MediaRecorderFormatManager> mediaRecorderFormatManagerM4A = adoptPtr(new MediaRecorderFormatManager());
    mediaRecorderFormatManagerM4A->initializeRecorderFormat(ACAPS_AAC,
            0, 0, 0,
            gstAudioSource, gstAudioFilter, gstAudioBooster, gstAACEncoder, gstQueue,
            gstMuxerForMP4, gstFileSink);
    formatMap.set(String("m4a"), mediaRecorderFormatManagerM4A.release());

    OwnPtr<MediaRecorderFormatManager> mediaRecorderFormatManagerAAC = adoptPtr(new MediaRecorderFormatManager());
    mediaRecorderFormatManagerAAC->initializeRecorderFormat(ACAPS_AAC,
            0, 0, 0,
            gstAudioSource, gstAudioFilter, gstAudioBooster, gstAACEncoder, gstQueue,
            gstMuxerForAAC, gstFileSink);
    formatMap.set(String("aac"), mediaRecorderFormatManagerAAC.release());

    return formatMap;
}

static MediaRecorderFormatManager* mediaRecorderFormatManager(const String& formatName)
{
    static MediaRecorderFormatManagerMap& formatMap = createMediaRecorderFormatManagerMap();
    return formatMap.get(formatName);
}

PassOwnPtr<MediaRecorderPrivateInterface> MediaRecorderPrivate::create(MediaRecorder* recorder)
{
    return adoptPtr(new MediaRecorderPrivate(recorder));
}

MediaRecorderPrivate::MediaRecorderPrivate(MediaRecorder* recorder)
    : m_pipeline(0)
    , m_recordMode(RECORD_MODE_NONE)
    , m_maxFileSizeBytes(0)
    , m_baseTime(GST_CLOCK_TIME_NONE)
    , m_recorder(recorder)
    , m_recordingStopTimer(RunLoop::current(), this, &MediaRecorderPrivate::recordingStopTimerFired)
    , m_audioSource(0)
    , m_audioFilter(0)
    , m_audioBooster(0)
    , m_audioEncoder(0)
    , m_audioQueue(0)
    , m_videoSource(0)
    , m_videoEncoder(0)
    , m_videoQueue(0)
    , m_muxer(0)
    , m_filesink(0)
{
    if (!initializeGStreamer()) {
        TIZEN_LOGE("Fail to initialize.");
        return;
    }

    GRefPtr<GstElementFactory> srcFactory = gst_element_factory_find("webkitcamerasrc");
    if (!srcFactory) {
        if (!gst_element_register(0, "webkitcamerasrc", GST_RANK_PRIMARY + 200, WEBKIT_TYPE_CAMERA_SRC))
            TIZEN_LOGE("Fail to register webkitcamerasrc.");
    }
}

MediaRecorderPrivate::~MediaRecorderPrivate()
{
    deletePipeline();
}

void MediaRecorderPrivate::recordingStopTimerFired()
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("");
#endif
    if (m_pipeline && !m_recordedFileName.isEmpty())
        save(m_recordedFileName);
}

gboolean MediaRecorderPrivate::mediaRecorderMessageCallback(GstBus*, GstMessage* message, gpointer userData)
{
    MediaRecorderPrivate* recorderPrivate = static_cast<MediaRecorderPrivate*>(userData);
    ASSERT(recorderPrivate);
    ASSERT(recorderPrivate->m_recorder);

    gboolean handled = FALSE;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(recorderPrivate->m_pipeline)) {
            GstState newState, oldState;
            gst_message_parse_state_changed(message, &oldState, &newState, 0);

#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGI("[Media] recording state change : %s -> %s",
                    gst_element_state_get_name(oldState),
                    gst_element_state_get_name(newState));
#endif

            recorderPrivate->m_recorder->onRecordingStateChange(String(gst_element_state_get_name(newState)));
            handled = TRUE;
        }
        break;
    default:
        break;
    }

    return handled;
}

gboolean MediaRecorderPrivate::fileSinkBufferProbeCallback(GstPad*, GstBuffer*, gpointer userData)
{
    MediaRecorderPrivate* recorder = static_cast<MediaRecorderPrivate*>(userData);
    ASSERT(recorder->m_pipeline);

    if (!recorder->m_recordedFileName.isEmpty()) {
        struct stat stat_buf;
        if (stat(recorder->m_recordedFileName.utf8().data(), &stat_buf) == 0) {
            const int maxRecordedFileSize = 1024 * 1024 * 10;
            if (stat_buf.st_size > maxRecordedFileSize || (recorder->m_maxFileSizeBytes && (stat_buf.st_size > recorder->m_maxFileSizeBytes))) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGI("[Media] fileSize : %d", stat_buf.st_size);
#endif
                recorder->m_recordingStopTimer.startOneShot(0);
            }
        }
    }

    return TRUE;
}

gboolean MediaRecorderPrivate::bufferProbeCallback(GstPad*, GstBuffer* buffer, gpointer userData)
{
    MediaRecorderPrivate* recorder = static_cast<MediaRecorderPrivate*>(userData);
    GstClock* gstClock = gst_system_clock_obtain();
    gst_object_ref(gstClock);

    if (recorder->m_baseTime == GST_CLOCK_TIME_NONE)
        recorder->m_baseTime = gst_clock_get_time(gstClock);

    GST_BUFFER_TIMESTAMP(buffer) = gst_clock_get_time(gstClock) - recorder->m_baseTime;
    gst_object_unref(gstClock);

    return TRUE;
}

void MediaRecorderPrivate::createPipeline()
{
    if (m_pipeline || m_recordMode == RECORD_MODE_NONE || m_recordMode == RECORD_MODE_INVALID)
        return;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("");
#endif
    m_pipeline = gst_pipeline_new(0);

    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(mediaRecorderMessageCallback), this);

    String recorderFormat = m_recordingFormat;
    if (recorderFormat.isEmpty()) {
        if (m_recordMode == RECORD_MODE_AUDIO_ONLY)
            recorderFormat = "amr";
        else
            recorderFormat = "3gp";
    }

    m_recordedFileName = String(m_recordMode == RECORD_MODE_AUDIO_ONLY ? AUDIO_FILE_PATH : VIDEO_FILE_PATH);

    if (m_customFileName.isEmpty()) {
        GTimeZone* tz = g_time_zone_new_local();
        GDateTime* dt = g_date_time_new_now(tz);
        gchar* tstr = 0;
        tstr = g_date_time_format(dt, "%s.");
        m_recordedFileName.append(String(tstr) + m_recordingFormat);
        g_free(tstr);
        g_date_time_unref(dt);
        g_time_zone_unref(tz);
    } else
        m_recordedFileName.append(m_customFileName);

    MediaRecorderFormatManager* formatManager = mediaRecorderFormatManager(recorderFormat);

    ASSERT(formatManager);

    m_muxer = formatManager->createElement(MediaRecorderFormatManager::Muxer);
    m_filesink = formatManager->createElement(MediaRecorderFormatManager::FileSink);

    g_object_set(G_OBJECT(m_filesink), "location", m_recordedFileName.utf8().data(), NULL);
    {
        GRefPtr<GstPad> sinkpad = adoptGRef(gst_element_get_static_pad(m_filesink, "sink"));
        gst_pad_add_buffer_probe(sinkpad.get(), G_CALLBACK(fileSinkBufferProbeCallback), static_cast<gpointer>(this));
    }

    if (m_recordMode & RECORD_MODE_AUDIO_ONLY) {
        m_audioSource = formatManager->createElement(MediaRecorderFormatManager::AudioSource);
        m_audioFilter = formatManager->createElement(MediaRecorderFormatManager::AudioFilter);
        GRefPtr<GstCaps> caps = gst_caps_from_string(formatManager->audioCaps().utf8().data());

        g_object_set(G_OBJECT(m_audioFilter), "caps", caps.get(), NULL);
        m_audioEncoder = formatManager->createElement(MediaRecorderFormatManager::AudioEncoder);
        m_audioBooster = formatManager->createElement(MediaRecorderFormatManager::AudioBooster);
        m_audioQueue = formatManager->createElement(MediaRecorderFormatManager::AudioQueue);

        gst_bin_add_many(GST_BIN(m_pipeline), m_audioSource, m_audioFilter, NULL);
        gst_element_link_many(m_audioSource, m_audioFilter, NULL);

        if (m_audioBooster) {
            g_object_set(G_OBJECT(m_audioBooster), "record-filter", TRUE, NULL);
            g_object_set(G_OBJECT(m_audioBooster), "sound-booster", TRUE, NULL);

            gst_bin_add(GST_BIN(m_pipeline), m_audioBooster);
            gst_element_link(m_audioFilter, m_audioBooster);
        }

        if (!m_muxer && m_recordMode == RECORD_MODE_AUDIO_ONLY) {
            // PCM
            gst_bin_add_many(GST_BIN(m_pipeline), m_audioQueue, m_filesink, NULL);
            gst_element_link_many(m_audioBooster ? m_audioBooster : m_audioFilter, m_audioQueue, m_filesink, NULL);
            if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_READY))
                g_warning("can't set pipeline to ready");
            return;
        } else {
            // maru_aacenc fails to encode if "1024 * bitrate / sample_rate" > 6144 * channels.
            // To avoid this case, set 'bitrate' as 64000. (Default bitrate of maru_aacenc is 128000.)
            if (recorderFormat.startsWith("m4a"))
                g_object_set(G_OBJECT(m_audioEncoder), "bitrate", 64000, NULL);

            gst_bin_add_many(GST_BIN(m_pipeline), m_audioEncoder, m_audioQueue, m_muxer, NULL);
            gst_element_link_many(m_audioBooster ? m_audioBooster : m_audioFilter, m_audioEncoder, m_audioQueue, m_muxer, NULL);
        }
    }

    if (m_recordMode & RECORD_MODE_VIDEO_ONLY) {
        m_videoSource = gst_element_factory_make("webkitcamerasrc", 0);
        m_videoEncoder = formatManager->createElement(MediaRecorderFormatManager::VideoEncoder);
        g_object_set(G_OBJECT(m_videoEncoder), "bitrate", 4000000, NULL);
        g_object_set(G_OBJECT(m_videoEncoder), "state-tuning", TRUE, NULL);

        // caps Filter to make caps data from camera.
        GstCaps* caps = gst_caps_new_simple("video/x-raw-yuv",
                "format", GST_TYPE_FOURCC, cameraFormat,
                "framerate", GST_TYPE_FRACTION, cameraFramerate, 1,
                "width", G_TYPE_INT, cameraWidth,
                "height", G_TYPE_INT, cameraHeight,
                "rotate", G_TYPE_INT, 0,
                NULL);

        GstElement* m_capsFilter = gst_element_factory_make("capsfilter", 0);
        g_object_set(G_OBJECT(m_capsFilter), "caps", caps, NULL);
        gst_caps_unref(caps);

        gst_pad_set_caps(gst_element_get_static_pad(m_videoEncoder, "sink"), caps);

        m_videoQueue = formatManager->createElement(MediaRecorderFormatManager::VideoQueue);

        gst_bin_add_many(GST_BIN(m_pipeline), m_videoSource, m_videoEncoder, m_videoQueue, m_muxer, NULL);
        gst_element_link_many(m_videoSource, m_videoEncoder, m_videoQueue, m_muxer, NULL);

        GRefPtr<GstPad> sinkpad = adoptGRef(gst_element_get_static_pad(m_videoEncoder, "sink"));
        gst_pad_add_buffer_probe(sinkpad.get(), G_CALLBACK(bufferProbeCallback), static_cast<gpointer>(this));

        String cameraUrl = String::format("camera://%d", m_cameraId);
        g_object_set(G_OBJECT(m_videoSource), "location", cameraUrl.utf8().data(), NULL);
        g_object_set(G_OBJECT(m_videoSource), "is-recording", TRUE, NULL);
    }

    gst_bin_add(GST_BIN(m_pipeline), m_filesink);
    gst_element_link(m_muxer, m_filesink);

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_READY)) {
        TIZEN_LOGE("Can't set pipeline to ready");
        g_warning("can't set pipeline to ready");
    }
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "webkit-mediarecorder");
}

void MediaRecorderPrivate::deletePipeline()
{
    if (!m_pipeline)
        return;

    m_baseTime = GST_CLOCK_TIME_NONE;

    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(mediaRecorderMessageCallback), this);

    // Flush message.
    gst_bus_set_flushing(bus.get(), TRUE);
    gst_bus_set_flushing(bus.get(), FALSE);

    gst_bus_remove_signal_watch(bus.get());

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_NULL))
        g_warning("can't set pipeline to stop");

    if (GST_STATE_CHANGE_FAILURE == gst_element_get_state(m_pipeline, 0, 0, GST_CLOCK_TIME_NONE))
        g_warning("can't get pipeline status");

    gst_object_unref(m_pipeline);
    m_pipeline = 0;
    if (m_audioSource) {
        gst_object_unref(m_audioSource);
        m_audioSource = 0;
    }
    if (m_audioFilter) {
        gst_object_unref(m_audioFilter);
        m_audioFilter = 0;
    }
    if (m_audioBooster) {
        gst_object_unref(m_audioBooster);
        m_audioBooster = 0;
    }
    if (m_audioEncoder) {
        gst_object_unref(m_audioEncoder);
        m_audioEncoder = 0;
    }
    if (m_audioQueue) {
        gst_object_unref(m_audioQueue);
        m_audioQueue = 0;
    }
    if (m_videoSource) {
        gst_object_unref(m_videoSource);
        m_videoSource = 0;
    }
    if (m_videoEncoder) {
        gst_object_unref(m_videoEncoder);
        m_videoEncoder = 0;
    }
    if (m_videoQueue) {
        gst_object_unref(m_videoQueue);
        m_videoQueue = 0;
    }
    if (m_muxer) {
        gst_object_unref(m_muxer);
        m_muxer = 0;
    }
    if (m_filesink) {
        gst_object_unref(m_filesink);
        m_filesink = 0;
    }
}

bool MediaRecorderPrivate::start()
{
#if ENABLE(TIZEN_EMULATOR)
    // when recording a video type
    if (m_recordMode & RECORD_MODE_VIDEO_ONLY) {
        // check camPipeline state whether it was changed to PLAYING or not.
        if (!LocalMediaServer::instance().camPipelineStateIsPlaying(LocalMediaServer::instance().camPipeline()))
            TIZEN_LOGE("wasn't set cam pipeline to playing");
    }
#endif

    createPipeline();

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_PLAYING)) {
        g_warning("can't set pipeline to playing");
        return false;
    }

    GstState currentState;
    gst_element_get_state(m_pipeline, &currentState, 0, stateChangeTimeout * GST_MSECOND);
#if ENABLE(TIZEN_EMULATOR)
    if (m_recordMode & RECORD_MODE_VIDEO_ONLY) {
        if (LocalMediaServer::instance().camPipelineStateIsPlaying(m_pipeline))
            return true;
        else
            TIZEN_LOGE("wasn't set pipeline to playing");
    }
#else
    if (currentState == GST_STATE_PLAYING)
        return true;
#endif

    // retry
    deletePipeline();
    LocalMediaServer::instance().resetStream();
    createPipeline();

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_PLAYING)) {
        g_warning("can't set pipeline to playing");
        return false;
    }

    gst_element_get_state(m_pipeline, &currentState, 0, stateChangeTimeout * GST_MSECOND);
    return (currentState == GST_STATE_PLAYING) ? true : false;
}

void MediaRecorderPrivate::stop()
{
    deletePipeline();
}

void MediaRecorderPrivate::cancel()
{
    stop();

    if (!m_recordedFileName.isEmpty())
        deleteFile(m_recordedFileName);
}

bool MediaRecorderPrivate::record(MediaStreamDescriptor* descriptor)
{
    if (m_pipeline) {
        g_warning("recording already started");
        return false;
    }

    uint64_t maxRecordedFileSize = m_maxFileSizeBytes ? m_maxFileSizeBytes : 1024 * 1024 * 10;
    if (getVolumeFreeSizeForPath(MEDIA_DIR_PATH) < maxRecordedFileSize) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("Lack of space for recording - FreeSize : %ulld", getVolumeFreeSizeForPath(MEDIA_DIR_PATH));
#endif
        return false;
    }

    m_recordMode = RECORD_MODE_NONE;

    for (unsigned int i = 0; i < descriptor->numberOfAudioComponents(); i++)
        m_recordMode |= RECORD_MODE_AUDIO_ONLY;

    for (unsigned int i = 0; i < descriptor->numberOfVideoComponents(); i++) {
        MediaStreamSource* source = descriptor->videoComponent(i)->source();
        m_cameraId = (source->name() == "Self camera") ? 1 : 0;
        m_recordMode |= RECORD_MODE_VIDEO_ONLY;
    }

    if (m_recordMode != RECORD_MODE_NONE)
        return start();

    return false;
}

void MediaRecorderPrivate::save(String& filename)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("Filename => %s", m_recordedFileName.utf8().data());
#endif
    if (m_recordedFileName.isEmpty())
        return;

    gst_element_send_event(m_pipeline, gst_event_new_eos());
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    GstMessage* message = gst_bus_timed_pop_filtered(bus.get(), GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    if (!message)
        g_warning("Message is NULL");
    else {
        switch (message->type) {
        case GST_MESSAGE_EOS:
            gst_message_unref(message);
            break;
        default:
            g_warning("Got message but not GST_MESSAGE_EOS");
            gst_message_unref(message);
            break;
        }
    }

    m_recorder->onRecordingStateChange(String("STOP"));
    deletePipeline();
    filename.swap(m_recordedFileName);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
