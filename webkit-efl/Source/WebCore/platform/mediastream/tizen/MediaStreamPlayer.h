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

#ifndef MediaStreamPlayer_h
#define MediaStreamPlayer_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <gst/gst.h>
#include <sensor.h>

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaStreamDescriptor;
class MediaStreamSource;


enum {
    PREVIEW_MODE_NONE = 0x0000,
    PREVIEW_MODE_AUDIO_ONLY = 0x0001,
    PREVIEW_MODE_VIDEO_ONLY = 0x0010,
    PREVIEW_MODE_AUDIO_VIDEO = 0x0011,
    PREVIEW_MODE_INVALID = 0x0100
};

enum {
    RECORD_MODE_NONE = 0x0000,
    RECORD_MODE_AUDIO_ONLY = 0x0001,
    RECORD_MODE_VIDEO_ONLY = 0x0010,
    RECORD_MODE_AUDIO_VIDEO = 0x0011,
    RECORD_MODE_INVALID = 0x0100
};

class MediaStreamCallback {
public:
    virtual void OnVideoCaptured(unsigned char* frame, int size, int width, int height) = 0;

protected:
    virtual ~MediaStreamCallback() { }
};

class MediaStreamPlayer {
    WTF_MAKE_NONCOPYABLE(MediaStreamPlayer);
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~MediaStreamPlayer();

    static MediaStreamPlayer& instance();

    void addCallback(MediaStreamCallback*);

    void start(MediaStreamDescriptor*);
    void stop();

    void setAudioMute(bool muted);
    void toggleCamera();

    void record(MediaStreamDescriptor*);
    void save(String& filename);

private:
    MediaStreamPlayer();

    static void onRotationChanged(sensor_h, sensor_event_s*, void*);
    static void onCameraVideoStreamReceived(GstElement* sink, GstBuffer* buffer, GstPad* pad, MediaStreamPlayer* that);
    static gboolean onBusCallbackCalled(GstBus* bus, GstMessage* message, void* data);

    void registerRotationCallback();
    void unregisterRotationCallback();

    void updateVideoCaps();

    void createPipeline();
    void deletePipeline();

    void startCapture();
    void stopCapture();

    int rotation(float x, float y, float z);

    MediaStreamCallback* m_callback;
    sensor_listener_h m_listener;

    GstElement *m_pipeline;
    GstElement *m_vsrc, *m_vsink, *m_vfilter, *m_vconvert, *m_venc;
    GstElement *m_asrc, *m_afilter, *m_asink, *m_aenc;
    GstElement *m_tee, *m_q0, *m_q1, *m_mux, *m_fsink;
    gchar *m_vcaps;

    int m_cameraId;
    int m_rotation;
    int m_previewMode;
    int m_recordMode;

    bool m_audioMuted;
    bool m_isCaptured;
    bool m_isRecording;

    String m_recordedFileName;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStreamPlayer_h
