/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LocalMediaServer_h
#define LocalMediaServer_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "WebKitCameraSourceGStreamerTizen.h"
#include <camera.h>
#include <gst/gst.h>
#include <wtf/HashMap.h>

namespace WebCore {

#if ENABLE(TIZEN_WEARABLE_PROFILE)
#if ENABLE(TIZEN_EMULATOR)
static const guint32 cameraFormat = GST_MAKE_FOURCC ('I', '4', '2', '0');
static const int cameraWidth = 320;
static const int cameraHeight = 240;
#else
static const guint32 cameraFormat = GST_MAKE_FOURCC ('S', 'N', '1', '2');
static const int cameraWidth = 640;
static const int cameraHeight = 640;
#endif
#else
static const guint32 cameraFormat = GST_MAKE_FOURCC ('N', 'V', '1', '2');
static const int cameraWidth = 1280;
static const int cameraHeight = 720;
#endif
static const int cameraFramerate = 30;
static const int stateChangeTimeout = 300;

class CameraDeviceCapabilities {
public:
    CameraDeviceCapabilities();
    void addSupportedCaptureFormat(camera_pixel_format_e format) { m_supportedCaptureFormats.append(format); }
    void addSupportedCaptureResolution(const IntSize& size) { m_supportedCaptureResolutions.append(size); }
    Vector<int>& supportedCaptureFormats() { return m_supportedCaptureFormats; }
    Vector<IntSize>& supportedCaptureResolutions() { return m_supportedCaptureResolutions; }
    static CameraDeviceCapabilities& instance();
private:
    void initialize();
    Vector<int> m_supportedCaptureFormats;
    Vector<IntSize> m_supportedCaptureResolutions;
};

class LocalMediaServer {

public:
    enum activateState {Initiated, Preroll, Activate, Inactive};
    enum SrcType { NullSrc, CameraSrc, V4l2Src, QCameraSrc };

    ~LocalMediaServer();

    static LocalMediaServer& instance();

    virtual bool startStream();
    virtual void stopStream();
    virtual void resetStream();

    virtual void add(WebKitCameraSrc*);
    virtual void remove(WebKitCameraSrc*);
    virtual void stateChanged(WebKitCameraSrc*, GstStateChange);
    gboolean handleMessage(GstMessage*);
    GstElement* camSource() const { return m_camSource; }
    GstElement* camPipeline() const { return m_camPipeline; }
    void changeResolution(int width, int height);
    int width() const { return m_width; }
    int height() const { return m_height; }
    SrcType srcType() const { return m_srcType; }

    void clear();
#if ENABLE(TIZEN_EMULATOR)
    bool audioStreamIsAvailable() const;
    bool camPipelineStateIsPlaying(GstElement* pipeline) const;
#endif
    static gboolean queueSrcBufferProbeCallback(GstPad*, GstBuffer*, gpointer userData);
    void addAppSrc(GstElement* appSrc) { m_appSrcs.append(appSrc); }
    void removeAppSrc(GstElement* appSrc);

private:
    LocalMediaServer();

    bool createPipeline();
    void suspendIfNecessary();

    GstElement* m_camPipeline;
    GstElement* m_camSource;
    GstElement* m_capsFilter;
    GstElement* m_tcpServerSink;

    bool m_stoped;
    HashMap<WebKitCameraSrc*, activateState> m_clients;

    bool m_changingResolution;
    int m_width;
    int m_height;
    SrcType m_srcType;
    Vector<GstElement*> m_appSrcs;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // LocalMediaServer_h
