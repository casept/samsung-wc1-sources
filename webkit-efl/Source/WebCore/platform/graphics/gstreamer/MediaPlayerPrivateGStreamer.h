/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009, 2010 Igalia S.L
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
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MediaPlayerPrivateGStreamer_h
#define MediaPlayerPrivateGStreamer_h
#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include "MediaPlayerPrivateGStreamerBase.h"
#include "Timer.h"

#include <glib.h>
#include <gst/gst.h>
#include <gst/pbutils/install-plugins.h>
#include <wtf/Forward.h>

#if ENABLE(MEDIA_SOURCE)
#include "MediaSourceGStreamer.h"
#endif

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
#include "AudioSessionManagerGStreamerTizen.h"
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

typedef struct _GstBuffer GstBuffer;
typedef struct _GstMessage GstMessage;
typedef struct _GstElement GstElement;

namespace WebCore {

class AudioTrackPrivateGStreamer;
class InbandTextTrackPrivateGStreamer;
class VideoTrackPrivateGStreamer;

class MediaPlayerPrivateGStreamer : public MediaPlayerPrivateGStreamerBase
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    , public AudioSessionManagerCallbackClient
#endif
{
public:
    ~MediaPlayerPrivateGStreamer();
    static void registerMediaEngine(MediaEngineRegistrar);
    gboolean handleMessage(GstMessage*);
    void handlePluginInstallerResult(GstInstallPluginsReturn);

    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }

    void load(const String &url);
#if ENABLE(MEDIA_SOURCE)
    void load(const String& url, MediaSourcePrivateClient*);
#endif
    void commitLoad();
    void cancelLoad();

    void prepareToPlay();
    void play();
    void pause();

#if ENABLE(TIZEN_MEDIA_STREAM)
    void preparePlayForLocalMediaStream();
    bool isLocalMediaStreamPaused();
#endif

    bool paused() const;
    bool seeking() const;

    float duration() const;
#if ENABLE(TIZEN_MSE)
    virtual double durationDouble() const { return m_mediaDuration;}
#endif
    float currentTime() const;
    void seek(float);

    void setRate(float);
    void setPreservesPitch(bool);

    void setPreload(MediaPlayer::Preload);
    void fillTimerFired(Timer<MediaPlayerPrivateGStreamer>*);

    PassRefPtr<TimeRanges> buffered() const;
    float maxTimeSeekable() const;
    bool didLoadingProgress() const;
    unsigned totalBytes() const;
    float maxTimeLoaded() const;

    void loadStateChanged();
    void timeChanged();
    void didEnd();
    void durationChanged();
    void loadingFailed(MediaPlayer::NetworkState);

    void videoChanged();
    void videoCapsChanged();
    void audioChanged();

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    void getDrmElement(GstElement* element);
    void videoNeedKey(GstElement*, gpointer, guint, gpointer);
    bool generateKeyRequest(char*, unsigned char*, unsigned, unsigned char**, unsigned*);
    bool updateKey(char*, unsigned char*, unsigned, unsigned char*, unsigned);
    PassRefPtr<Uint8Array> generateKeyRequest(const String&, const String&, Uint8Array*);
    bool updateKey(const String&, const String&, Uint8Array* key, Uint8Array*);
    void elementAdded(GstElement*);
#endif

    void notifyPlayerOfVideo();
    void notifyPlayerOfVideoCaps();
    void notifyPlayerOfAudio();

#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    void textChanged();
    void notifyPlayerOfText();

    void newTextSample();
    void notifyPlayerOfNewTextSample();
#endif

    void sourceChanged();
    GstElement* audioSink() const;

    void setAudioStreamProperties(GObject*);

    void simulateAudioInterruption();

#if ENABLE(TIZEN_WEBKIT2_PROXY)
    void setProxy(GstElement* source);
#endif // ENABLE(TIZEN_WEBKIT2_PROXY)

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    MediaPlayer* player() { return m_player; }
    KURL url() { return m_url; }

    virtual void suspend();
    virtual void resume();
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

#if USE(TIZEN_PLATFORM_SURFACE)
    void xWindowIdPrepared(GstMessage*);
#endif // USE(TIZEN_PLATFORM_SURFACE)

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    MediaPlayer::OverlayType overlayType() { return m_overlayType; }
    virtual void changeOverlayType(MediaPlayer::OverlayType);
    virtual void rotateHwOverlayVideo();
#endif

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    bool asmCallbackProcessed() { return m_asmCallbackProcessed; }
    void setAsmCallbackProcessed() { m_asmCallbackProcessed = true; }
    void clearPlayStateTimerHandler() { m_playStateTimerHandler = 0; }
    void playStateChangedByASM(bool);
    virtual void notifyResumeByASMCallback();
#endif

#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    GstElement* source() { return m_source.get(); }
    bool needPauseResumeWebSrc() { return m_needPauseResumeWebSrc; }
    void setNeedPauseResumeWebSrc(bool needPauseResumeWebSrc) { m_needPauseResumeWebSrc = needPauseResumeWebSrc; }
#endif

private:
    MediaPlayerPrivateGStreamer(MediaPlayer*);

    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);

    static void getSupportedTypes(HashSet<String>&);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);

    static bool isAvailable();

    void updateAudioSink();
    void createAudioSink();

    float playbackPosition() const;

    void cacheDuration();
    void updateStates();
    void asyncStateChangeDone();

    void createGSTPlayBin();
    bool changePipelineState(GstState);

    bool loadNextLocation();
    void mediaLocationChanged(GstMessage*);

    void setDownloadBuffering();
    void processBufferingStats(GstMessage*);

    virtual String engineDescription() const { return "GStreamer"; }
#if ENABLE(TIZEN_MSE)
    virtual bool isLiveStream() const { return m_mediaSource ? false : m_isStreaming; }
    virtual IntSize naturalSize() const;
#else
    virtual bool isLiveStream() const { return m_isStreaming; }
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    virtual bool isLocalMediaStream() const;
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_MSE)
    virtual MediaPlayer::ReadyState mediasourceReadyState() const;
#endif //  ENABLE(TIZEN_MSE)

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    void setXWindowHandle();
    void setRotateForHwOverlayVideo();
#endif

#if USE(TIZEN_PLATFORM_SURFACE)
    void setPixmap();
#endif

#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    bool shouldUseWebkitWebSource();
#endif

private:
    GRefPtr<GstElement> m_playBin;
    GRefPtr<GstElement> m_source;
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    GRefPtr<GstElement> m_videoDrmBridge;
    GRefPtr<GstElement> m_audioDrmBridge;
#endif
#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    GRefPtr<GstElement> m_textAppSink;
    GRefPtr<GstPad> m_textAppSinkPad;
#endif
    float m_seekTime;
    bool m_changingRate;
    float m_endTime;
    bool m_isEndReached;
    mutable bool m_isStreaming;
    GstStructure* m_mediaLocations;
    int m_mediaLocationCurrentIndex;
    bool m_resetPipeline;
    bool m_paused;
    bool m_seeking;
    bool m_seekIsPending;
    float m_timeOfOverlappingSeek;
    bool m_canFallBackToLastFinishedSeekPositon;
    bool m_buffering;
    float m_playbackRate;
    bool m_errorOccured;
#if ENABLE(TIZEN_MSE)
    mutable double m_mediaDuration;
#else
    mutable gfloat m_mediaDuration;
#endif
    bool m_downloadFinished;
    Timer<MediaPlayerPrivateGStreamer> m_fillTimer;
    float m_maxTimeLoaded;
    int m_bufferingPercentage;
    MediaPlayer::Preload m_preload;
    bool m_delayingLoad;
    bool m_mediaDurationKnown;
    mutable float m_maxTimeLoadedAtLastDidLoadingProgress;
    bool m_volumeAndMuteInitialized;
    bool m_hasVideo;
    bool m_hasAudio;
    guint m_audioTimerHandler;
    guint m_textTimerHandler;
    guint m_videoTimerHandler;
    guint m_videoCapsTimerHandler;
    GRefPtr<GstElement> m_webkitAudioSink;
    mutable long m_totalBytes;
    KURL m_url;
    bool m_preservesPitch;
    GstState m_requestedState;
    GRefPtr<GstElement> m_autoAudioSink;
    bool m_missingPlugins;
#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    Vector<RefPtr<AudioTrackPrivateGStreamer>> m_audioTracks;
    Vector<RefPtr<InbandTextTrackPrivateGStreamer>> m_textTracks;
    Vector<RefPtr<VideoTrackPrivateGStreamer>> m_videoTracks;
    RefPtr<InbandTextTrackPrivate> m_chaptersTrack;
#endif
#if ENABLE(MEDIA_SOURCE)
    RefPtr<MediaSourcePrivateClient> m_mediaSource;
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    float m_suspendTime;
    mutable float m_lastPlaybackPosition;
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    RefPtr<AudioSessionManagerGStreamerTizen> m_audioSessionManager;
    guint m_playStateTimerHandler;
    bool  m_asmCallbackProcessed;
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    MediaPlayer::OverlayType m_overlayType;
#endif
#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    bool m_needPauseResumeWebSrc;
#endif
};
}

#endif // USE(GSTREAMER)
#endif
