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

#ifndef MediaRecorderPrivateImpl_h
#define MediaRecorderPrivateImpl_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorderPrivate.h"
#include "RunLoop.h"
#include "Timer.h"
#include <gst/gst.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaRecorder;
class MediaStreamDescriptor;

enum {
    RECORD_MODE_NONE = 0x0000,
    RECORD_MODE_AUDIO_ONLY = 0x0001,
    RECORD_MODE_VIDEO_ONLY = 0x0010,
    RECORD_MODE_AUDIO_VIDEO = 0x0011,
    RECORD_MODE_INVALID = 0x0100
};

class MediaRecorderFormatManager {
public:
    enum ElementType {
        VideoSource = 0,
        VideoEncoder,
        VideoQueue,
        AudioSource,
        AudioFilter,
        AudioBooster,
        AudioEncoder,
        AudioQueue,
        Muxer,
        FileSink
    };

    MediaRecorderFormatManager()
    {
        m_elementNames.resize(static_cast<int>(FileSink) + 1);
    }

    void initializeRecorderFormat(const char* audioCaps, ...)
    {
        int index = 0;
        m_audioCaps = audioCaps;

        va_list args;
        va_start(args, audioCaps);
        while (index <= FileSink)
            m_elementNames[index++] = va_arg(args, char*);
        va_end(args);
    }

    GstElement* createElement(ElementType type)
    {
        const String& elementName = m_elementNames[static_cast<int>(type)];
        if (!gst_element_factory_find(elementName.utf8().data()))
            return 0;

        return gst_element_factory_make(elementName.utf8().data(), 0);
    }

    const String& elementName(ElementType type) { return m_elementNames[static_cast<int>(type)]; }
    const String& audioCaps() { return m_audioCaps; }

private:
    Vector<String> m_elementNames;
    String m_audioCaps;
};

typedef HashMap<String, OwnPtr<MediaRecorderFormatManager>, StringHash> MediaRecorderFormatManagerMap;

class MediaRecorderPrivate : public MediaRecorderPrivateInterface {
public:
    static PassOwnPtr<MediaRecorderPrivateInterface> create(MediaRecorder*);
    virtual ~MediaRecorderPrivate();

    virtual bool record(MediaStreamDescriptor*) OVERRIDE;
    virtual void save(String& filename) OVERRIDE;
    virtual void cancel() OVERRIDE;

    static gboolean mediaRecorderMessageCallback(GstBus*, GstMessage*, gpointer userData);
    static gboolean bufferProbeCallback(GstPad*, GstBuffer*, gpointer userData);
    static gboolean fileSinkBufferProbeCallback(GstPad*, GstBuffer*, gpointer userData);
    virtual void setCustomFileName(const String& fileName) OVERRIDE { m_customFileName = fileName; }
    virtual void setMaxFileSizeBytes(int maxFileSizeBytes) OVERRIDE { m_maxFileSizeBytes = maxFileSizeBytes; }
    virtual void setRecordingFormat(const String& format) OVERRIDE { m_recordingFormat = format; }
private:
    MediaRecorderPrivate(MediaRecorder*);

    void recordingStopTimerFired();

    bool start();
    void stop();
    void createPipeline();
    void deletePipeline();

    GstElement* m_pipeline;
    int m_cameraId;
    int m_recordMode;
    String m_recordedFileName;
    String m_customFileName;
    String m_recordingFormat;
    int m_maxFileSizeBytes;

    GstClockTime m_baseTime;
    MediaRecorder* m_recorder;
    RunLoop::Timer<MediaRecorderPrivate> m_recordingStopTimer;
    GstElement* m_audioSource;
    GstElement* m_audioFilter;
    GstElement* m_audioBooster;
    GstElement* m_audioEncoder;
    GstElement* m_audioQueue;
    GstElement* m_videoSource;
    GstElement* m_videoEncoder;
    GstElement* m_videoQueue;
    GstElement* m_muxer;
    GstElement* m_filesink;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaRecorderPrivateImpl_h
