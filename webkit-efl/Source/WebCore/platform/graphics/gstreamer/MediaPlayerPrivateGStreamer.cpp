/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Igalia S.L
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

#include "config.h"
#include "MediaPlayerPrivateGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GStreamerUtilities.h"
#include "GStreamerVersioning.h"
#include "KURL.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "SecurityOrigin.h"
#include "TimeRanges.h"
#include "WebKitWebSourceGStreamer.h"
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <limits>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
#include "AudioTrackPrivateGStreamer.h"
#include "InbandTextTrackPrivateGStreamer.h"
#include "TextCombinerGStreamer.h"
#include "TextSinkGStreamer.h"
#include "VideoTrackPrivateGStreamer.h"
#endif

#ifdef GST_API_VERSION_1
#include <gst/audio/streamvolume.h>
#else
#include <gst/interfaces/streamvolume.h>
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "URIUtils.h"
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_WEBKIT2_PROXY)
#include <libsoup/soup.h>
#endif // ENABLE(TIZEN_WEBKIT2_PROXY)

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include "ResourceHandle.h"
#include <power.h>
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "./mediastream/tizen/LocalMediaServer.h"
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(MEDIA_SOURCE)
#include "MediaSource.h"
#include "WebKitMediaSourceGStreamer.h"
#endif

#if ENABLE(ENCRYPTED_MEDIA)
#include "CDM.h"
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
#include "EflScreenUtilities.h"
#include <gst/interfaces/xoverlay.h>
#endif

// GstPlayFlags flags from playbin2. It is the policy of GStreamer to
// not publicly expose element-specific enums. That's why this
// GstPlayFlags enum has been copied here.
typedef enum {
    GST_PLAY_FLAG_VIDEO         = 0x00000001,
    GST_PLAY_FLAG_AUDIO         = 0x00000002,
    GST_PLAY_FLAG_TEXT          = 0x00000004,
    GST_PLAY_FLAG_VIS           = 0x00000008,
    GST_PLAY_FLAG_SOFT_VOLUME   = 0x00000010,
    GST_PLAY_FLAG_NATIVE_AUDIO  = 0x00000020,
    GST_PLAY_FLAG_NATIVE_VIDEO  = 0x00000040,
    GST_PLAY_FLAG_DOWNLOAD      = 0x00000080,
    GST_PLAY_FLAG_BUFFERING     = 0x000000100
} GstPlayFlags;

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
// DRMBridge_Mode from DRMBridge plugin.
enum DRMBridge_Mode_
{
    DRMBRIDGE_MODE_LICENSING_AND_DECRYPTION,
    DRMBRIDGE_MODE_LICENSING,
    DRMBRIDGE_MODE_DECRYPTION,
    DRMBRIDGE_MODE_ERROR = 0xffff
}DRMBridge_Mode;
#endif

// gPercentMax is used when parsing buffering ranges with
// gst_query_parse_nth_buffering_range as there was a bug in GStreamer
// 0.10 that was using 100 instead of GST_FORMAT_PERCENT_MAX. This was
// corrected in 1.0. gst_query_parse_buffering_range worked as
// expected with GST_FORMAT_PERCENT_MAX in both cases.
#ifdef GST_API_VERSION_1
static const char* gPlaybinName = "playbin";
static const gint64 gPercentMax = GST_FORMAT_PERCENT_MAX;
#else
static const char* gPlaybinName = "playbin2";
static const gint64 gPercentMax = 100;
#endif

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
extern "C" {extern void webKitMediaVideoSrcNeedKeyCb(GstElement*, gpointer initData, guint initDataLength, gpointer userData);}
static const unsigned int drmTypeClearkey = 0x61;
static const unsigned int drmTypePlayready = 0x01;
typedef struct _LicenseData {
    unsigned char* key;
    int keyLength;
}LicenseData;
#endif

GST_DEBUG_CATEGORY_EXTERN(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

using namespace std;

namespace WebCore {

static gboolean mediaPlayerPrivateMessageCallback(GstBus*, GstMessage* message, MediaPlayerPrivateGStreamer* player)
{
    return player->handleMessage(message);
}

static void mediaPlayerPrivateSourceChangedCallback(GObject*, GParamSpec*, MediaPlayerPrivateGStreamer* player)
{
    player->sourceChanged();
}

static void mediaPlayerPrivateVideoSinkCapsChangedCallback(GObject*, GParamSpec*, MediaPlayerPrivateGStreamer* player)
{
    player->videoCapsChanged();
}

static void mediaPlayerPrivateVideoChangedCallback(GObject*, MediaPlayerPrivateGStreamer* player)
{
    player->videoChanged();
}

static void mediaPlayerPrivateAudioChangedCallback(GObject*, MediaPlayerPrivateGStreamer* player)
{
    player->audioChanged();
}

static void mediaPlayerElementAddedCallback(GstBin*, GstElement* element, MediaPlayerPrivateGStreamer* player)
{
    if (String(GST_ELEMENT_NAME (element)).startsWith("decodebin", false)) {
        g_signal_connect(GST_BIN(element), "element-added",
            G_CALLBACK(mediaPlayerElementAddedCallback), player);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith("hlsdemux", false)) {
        SoupSession* session = WebCore::ResourceHandle::defaultSession();
        if (!session)
            return;

        SoupURI* proxyUri = 0;
        g_object_get(session, SOUP_SESSION_PROXY_URI, &proxyUri, NULL);
        if (!proxyUri)
            return;

        char* proxy = soup_uri_to_string(proxyUri, false);
        g_object_set(element, "hls-proxy", proxy, NULL);
        TIZEN_LOGI("Set proxy for hlsdemux element = %s", proxy);

        soup_uri_free(proxyUri);
        g_free(proxy);
    }
}


#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
static void mediaPlayerPrivateVideoNeedKeyCallback(GstElement *element, gpointer initData, guint initDataLength, guint numInitData, gpointer userData)
{
    LOG_MEDIA_MESSAGE("mediaPlayerPrivateVideoNeedKeyCallback");
    LOG_MEDIA_MESSAGE("number of initData = %d", numInitData);
    MediaPlayerPrivateGStreamer* player = reinterpret_cast<MediaPlayerPrivateGStreamer*>(userData);
    player->videoNeedKey(element,initData, initDataLength, userData);
}

static void mediaPlayerPrivateElementAddedCallback(GstBin*, GstElement* element, MediaPlayerPrivateGStreamer* player)
{
    player->elementAdded(element);
}
#endif

static gboolean mediaPlayerPrivateAudioChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
    // This is the callback of the timeout source created in ::audioChanged.
    player->notifyPlayerOfAudio();
    return FALSE;
}

#ifdef GST_API_VERSION_1
static void setAudioStreamPropertiesCallback(GstChildProxy*, GObject* object, gchar*,
    MediaPlayerPrivateGStreamer* player)
#else
static void setAudioStreamPropertiesCallback(GstChildProxy*, GObject* object, MediaPlayerPrivateGStreamer* player)
#endif
{
    player->setAudioStreamProperties(object);
}

static gboolean mediaPlayerPrivateVideoChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
    // This is the callback of the timeout source created in ::videoChanged.
    player->notifyPlayerOfVideo();
    return FALSE;
}

static gboolean mediaPlayerPrivateVideoCapsChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
    // This is the callback of the timeout source created in ::videoCapsChanged.
    player->notifyPlayerOfVideoCaps();
    return FALSE;
}

#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
static void mediaPlayerPrivateTextChangedCallback(GObject*, MediaPlayerPrivateGStreamer* player)
{
    player->textChanged();
}

static gboolean mediaPlayerPrivateTextChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
    // This is the callback of the timeout source created in ::textChanged.
    player->notifyPlayerOfText();
    return FALSE;
}

static GstFlowReturn mediaPlayerPrivateNewTextSampleCallback(GObject*, MediaPlayerPrivateGStreamer* player)
{
    player->newTextSample();
    return GST_FLOW_OK;
}
#endif

static void mediaPlayerPrivatePluginInstallerResultFunction(GstInstallPluginsReturn result, gpointer userData)
{
    MediaPlayerPrivateGStreamer* player = reinterpret_cast<MediaPlayerPrivateGStreamer*>(userData);
    player->handlePluginInstallerResult(result);
}

static GstClockTime toGstClockTime(float time)
{
    // Extract the integer part of the time (seconds) and the fractional part (microseconds). Attempt to
    // round the microseconds so no floating point precision is lost and we can perform an accurate seek.
    float seconds;
    float microSeconds = modf(time, &seconds) * 1000000;
    GTimeVal timeValue;
    timeValue.tv_sec = static_cast<glong>(seconds);
    timeValue.tv_usec = static_cast<glong>(roundf(microSeconds / 10000) * 10000);
    return GST_TIMEVAL_TO_TIME(timeValue);
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
static void mediaPlayerPrivateSourceSetupCallback(GObject*, GstElement* source, MediaPlayerPrivateGStreamer* player)
{
    // Need below logic only for 'souphttpsrc'.
    if (!player->url().protocol().startsWith("http", false))
        return;

    // FIXME : Use getRawCookies() in 'CookieJarSoup.cpp' and 'cookies' property in 'souphttpsrc'
    // to set 'domain', 'path', 'expires' of cookie.
    // Currently getRawCookies() return empty data.
    String cookie = player->player()->mediaPlayerClient()->cookieForUrl(player->url());
    GstStructure* headers = gst_structure_new("extra-headers", "Cookie", G_TYPE_STRING, cookie.utf8().data(), NULL);
    g_object_set(source, "extra-headers", headers, NULL);
    gst_structure_free(headers);

#if ENABLE(TIZEN_WEBKIT2_PROXY)
    player->setProxy(source);
#endif // ENABLE(TIZEN_WEBKIT2_PROXY)

    String userAgent = player->player()->mediaPlayerClient()->userAgentForUrl(player->url());
    if (!userAgent.isNull() || !userAgent.isEmpty())
        g_object_set(source, "user-agent", userAgent.utf8().data(), NULL);
}
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
bool MediaPlayerAudioSessionIsEventType(ASM_event_sources_t eventType)
{
    switch (eventType) {
    case ASM_EVENT_SOURCE_ALARM_END:
    case ASM_EVENT_SOURCE_CALL_END:
    case ASM_EVENT_SOURCE_EMERGENCY_END:
    case ASM_EVENT_SOURCE_NOTIFY_END:
    case ASM_EVENT_SOURCE_MEDIA:
    case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
    case ASM_EVENT_SOURCE_VR_END:
        return true;
    default:
        break;
    }

    return false;
}

bool MediaPlayerAudioSessionIsEventTypeByCallback(ASM_event_sources_t eventType)
{
    if (eventType == ASM_EVENT_SOURCE_CALL_START)
        return true;
    return false;
}

static ASM_cb_result_t MediaPlayerAudioSessionEventSourcePause(ASM_event_sources_t eventSource, void* callbackData)
{
    MediaPlayerPrivateGStreamer* player = static_cast<MediaPlayerPrivateGStreamer*>(callbackData);
    if (!player)
        return ASM_CB_RES_IGNORE;

    switch (eventSource) {
    case ASM_EVENT_SOURCE_CALL_START:
    case ASM_EVENT_SOURCE_ALARM_START:
    case ASM_EVENT_SOURCE_NOTIFY_START:
    case ASM_EVENT_SOURCE_MEDIA:
    case ASM_EVENT_SOURCE_EMERGENCY_START:
    case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
    case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
    case ASM_EVENT_SOURCE_EARJACK_UNPLUG:
    case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
    case ASM_EVENT_SOURCE_VR_START:
        if (!player->player()->url().string().contains("camera://")) {
#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
            player->setNeedPauseResumeWebSrc(MediaPlayerAudioSessionIsEventTypeByCallback(eventSource));
#endif
            player->playStateChangedByASM(false);

            if (MediaPlayerAudioSessionIsEventTypeByCallback(eventSource))
                AudioSessionManagerGStreamerTizen::addCallbackClient(player);

            // In this case, we should wait for processing pause event.
            // Otherwise, sound could be heard by speaker.
            if (eventSource == ASM_EVENT_SOURCE_EARJACK_UNPLUG) {
                const int maxWaitingCount = 30; // 300ms.
                int waitingCount = 0;
                while (!player->asmCallbackProcessed()) {
                    usleep(10000);
                    waitingCount++;
                    TIZEN_LOGI("Wait => %d", waitingCount);
                    if (waitingCount >= maxWaitingCount)
                        break;
                }
            }

            return ASM_CB_RES_PAUSE;
        }
    default:
        return ASM_CB_RES_NONE;
    }
}

static ASM_cb_result_t MediaPlayerAudioSessionEventSourcePlay(ASM_event_sources_t eventSource, void* callbackData)
{
    MediaPlayerPrivateGStreamer* player = static_cast<MediaPlayerPrivateGStreamer*>(callbackData);
    if (!player)
        return ASM_CB_RES_IGNORE;

#if ENABLE(TIZEN_EMULATOR)
    switch (eventSource) {
    case ASM_EVENT_SOURCE_ALARM_END:
        if (!player->player()->mediaPlayerClient()->isVideoElement() && !player->player()->url().string().contains("camera://")) {
            player->playStateChangedByASM(true);
            return ASM_CB_RES_PLAYING;
        }
        return ASM_CB_RES_NONE;
    default:
        return ASM_CB_RES_NONE;
    }
#else
    if (MediaPlayerAudioSessionIsEventType(eventSource)) {
        if (!player->player()->mediaPlayerClient()->isVideoElement() && !player->player()->url().string().contains("camera://")) {
            player->playStateChangedByASM(true);
            TIZEN_LOGI("Media Element has been played by ASM : %d", eventSource);
            return ASM_CB_RES_PLAYING;
        }
    }
    TIZEN_LOGI("ASM Type : %d", eventSource);
    return ASM_CB_RES_NONE;
#endif
}

static ASM_cb_result_t mediaPlayerPrivateAudioSessionNotifyCallback(int, ASM_event_sources_t eventSource, ASM_sound_commands_t command, unsigned int, void* callbackData)
{
    TIZEN_LOGI("ASM Callback => command : %d, source : %d", command, eventSource);
    if (command == ASM_COMMAND_STOP || command == ASM_COMMAND_PAUSE)
        return MediaPlayerAudioSessionEventSourcePause(eventSource, callbackData);
    if (command == ASM_COMMAND_PLAY || command == ASM_COMMAND_RESUME)
        return MediaPlayerAudioSessionEventSourcePlay(eventSource, callbackData);

    return ASM_CB_RES_NONE;
}

static gboolean mediaPlayerPrivatePlayByASMCallback(MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    if (player->needPauseResumeWebSrc()) {
        if (player->source() && WEBKIT_IS_WEB_SRC(player->source()))
            webkitWebSrcResume(WEBKIT_WEB_SRC(player->source()));
        player->setNeedPauseResumeWebSrc(false);
    }
#endif

    player->clearPlayStateTimerHandler();
    player->player()->mediaPlayerClient()->mediaPlayerPlay();
    player->setAsmCallbackProcessed();
    return FALSE;
}

static gboolean mediaPlayerPrivatePauseByASMCallback(MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    if (player->needPauseResumeWebSrc()) {
        if (player->source() && WEBKIT_IS_WEB_SRC(player->source()))
            webkitWebSrcPause(WEBKIT_WEB_SRC(player->source()));
    }
#endif

    player->clearPlayStateTimerHandler();
    player->player()->mediaPlayerClient()->mediaPlayerPause();
    player->setAsmCallbackProcessed();
    return FALSE;
}

void MediaPlayerPrivateGStreamer::playStateChangedByASM(bool play)
{
    if (m_playStateTimerHandler)
        g_source_remove(m_playStateTimerHandler);

    if (play)
        m_playStateTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivatePlayByASMCallback), this);
    else
        m_playStateTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivatePauseByASMCallback), this);

    m_asmCallbackProcessed = false;
}

void MediaPlayerPrivateGStreamer::notifyResumeByASMCallback()
{
    playStateChangedByASM(true);
}
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

void MediaPlayerPrivateGStreamer::setAudioStreamProperties(GObject* object)
{
    if (g_strcmp0(G_OBJECT_TYPE_NAME(object), "GstPulseSink"))
        return;

    const char* role = m_player->mediaPlayerClient() && m_player->mediaPlayerClient()->mediaPlayerIsVideo()
        ? "video" : "music";
    GstStructure* structure = gst_structure_new("stream-properties", "media.role", G_TYPE_STRING, role, NULL);
    g_object_set(object, "stream-properties", structure, NULL);
    gst_structure_free(structure);
    GOwnPtr<gchar> elementName(gst_element_get_name(GST_ELEMENT(object)));
    LOG_MEDIA_MESSAGE("Set media.role as %s at %s", role, elementName.get());
}

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateGStreamer::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivateGStreamer(player));
}

void MediaPlayerPrivateGStreamer::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

bool initializeGStreamerAndRegisterWebKitElements()
{
    if (!initializeGStreamer())
        return false;

    bool status = true;

#if ENABLE(TIZEN_MEDIA_STREAM)
    GRefPtr<GstElementFactory> webkitCameraSrcFactory = gst_element_factory_find("webkitcamerasrc");
    if (!webkitCameraSrcFactory) {
        status &= gst_element_register(0, "webkitcamerasrc", GST_RANK_PRIMARY + 200, WEBKIT_TYPE_CAMERA_SRC);
    }
#endif

#if !ENABLE(TIZEN_GSTREAMER_VIDEO) || ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    GRefPtr<GstElementFactory> srcFactory = gst_element_factory_find("webkitwebsrc");
    if (!srcFactory) {
        GST_DEBUG_CATEGORY_INIT(webkit_media_player_debug, "webkitmediaplayer", 0, "WebKit media player");
        status &= gst_element_register(0, "webkitwebsrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_WEB_SRC);
    }
#endif

#if ENABLE(MEDIA_SOURCE)
    GRefPtr<GstElementFactory> WebKitMediaSrcFactory = gst_element_factory_find("webkitmediasrc");
    if (!WebKitMediaSrcFactory)
        status &= gst_element_register(0, "webkitmediasrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_MEDIA_SRC);
#endif
    return status;
}

bool MediaPlayerPrivateGStreamer::isAvailable()
{
    if (!initializeGStreamerAndRegisterWebKitElements())
        return false;

    GRefPtr<GstElementFactory> factory = gst_element_factory_find(gPlaybinName);
    return factory;
}

#if USE(TIZEN_PLATFORM_SURFACE)
static GstBusSyncReply mediaPlayerPrivateSyncHandler(GstBus*, GstMessage* message, MediaPlayerPrivateGStreamer* playerPrivate)
{
    if (playerPrivate && playerPrivate->url().protocol().startsWith("http", false) &&
        playerPrivate->url().string().contains(".m3u8", false)) {
        GstState oldState, newState;

        if (String(GST_MESSAGE_SRC_NAME(message)).startsWith("uridecodebin", false)
            && GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED) {
            gst_message_parse_state_changed(message, &oldState, &newState, 0);
            if (oldState == GST_STATE_NULL && newState == GST_STATE_READY) {
                TIZEN_LOGI("Set element-added callback for uridecodebin");
                g_signal_connect(GST_BIN(GST_MESSAGE_SRC(message)), "element-added", G_CALLBACK(mediaPlayerElementAddedCallback), playerPrivate);
            }
        } else if (oldState == GST_STATE_PAUSED && newState == GST_STATE_READY) {
                g_signal_handlers_disconnect_by_func(GST_BIN(GST_MESSAGE_SRC(message)),   reinterpret_cast<gpointer>(mediaPlayerElementAddedCallback), playerPrivate);
        }
    }

    // ignore anything but 'prepare-xid' message
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_ELEMENT)
        return GST_BUS_PASS;

    if (!gst_structure_has_name (message->structure, "prepare-xid"))
        return GST_BUS_PASS;

    playerPrivate->xWindowIdPrepared(message);
    gst_message_unref(message);
    return GST_BUS_DROP;
}
#endif

MediaPlayerPrivateGStreamer::MediaPlayerPrivateGStreamer(MediaPlayer* player)
    : MediaPlayerPrivateGStreamerBase(player)
    , m_source(0)
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    , m_videoDrmBridge(0)
    , m_audioDrmBridge(0)
#endif
    , m_seekTime(0)
    , m_changingRate(false)
    , m_endTime(numeric_limits<float>::infinity())
    , m_isEndReached(false)
    , m_isStreaming(false)
    , m_mediaLocations(0)
    , m_mediaLocationCurrentIndex(0)
    , m_resetPipeline(false)
    , m_paused(true)
    , m_seeking(false)
    , m_seekIsPending(false)
    , m_timeOfOverlappingSeek(-1)
    , m_buffering(false)
    , m_playbackRate(1)
    , m_errorOccured(false)
    , m_mediaDuration(0)
    , m_downloadFinished(false)
    , m_fillTimer(this, &MediaPlayerPrivateGStreamer::fillTimerFired)
    , m_maxTimeLoaded(0)
    , m_bufferingPercentage(0)
    , m_preload(player->preload())
    , m_delayingLoad(false)
    , m_mediaDurationKnown(true)
    , m_maxTimeLoadedAtLastDidLoadingProgress(0)
    , m_volumeAndMuteInitialized(false)
    , m_hasVideo(false)
    , m_hasAudio(false)
    , m_audioTimerHandler(0)
    , m_textTimerHandler(0)
    , m_videoTimerHandler(0)
    , m_videoCapsTimerHandler(0)
    , m_webkitAudioSink(0)
    , m_totalBytes(-1)
    , m_preservesPitch(false)
    , m_requestedState(GST_STATE_VOID_PENDING)
    , m_missingPlugins(false)
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_suspendTime(0)
    , m_lastPlaybackPosition(0)
#endif
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    , m_audioSessionManager(AudioSessionManagerGStreamerTizen::createAudioSessionManager())
    , m_playStateTimerHandler(0)
    , m_asmCallbackProcessed(false)
#endif
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    , m_overlayType(MediaPlayer::Pixmap)
#endif
#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    , m_needPauseResumeWebSrc(false)
#endif
{
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    // FIXME : Need to call extensible api to check whether the flag is true
    if (m_player->mediaPlayerClient()->mediaPlayerIsFullscreen())
        m_overlayType = MediaPlayer::HwOverlay;
#endif
}

MediaPlayerPrivateGStreamer::~MediaPlayerPrivateGStreamer()
{
#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    for (size_t i = 0; i < m_audioTracks.size(); ++i)
        m_audioTracks[i]->disconnect();

    for (size_t i = 0; i < m_textTracks.size(); ++i)
        m_textTracks[i]->disconnect();

    for (size_t i = 0; i < m_videoTracks.size(); ++i)
        m_videoTracks[i]->disconnect();
#endif
    if (m_fillTimer.isActive())
        m_fillTimer.stop();

    if (m_mediaLocations) {
        gst_structure_free(m_mediaLocations);
        m_mediaLocations = 0;
    }

    if (m_autoAudioSink)
        g_signal_handlers_disconnect_by_func(G_OBJECT(m_autoAudioSink.get()),
            reinterpret_cast<gpointer>(setAudioStreamPropertiesCallback), this);

    // FIXME: We logged to ensure whether to fixe once crash. It should be removed some times later.
    TIZEN_LOGI("[%d]", !m_playBin);

    if (m_playBin) {
        GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_playBin.get()));
        ASSERT(bus);
        g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateMessageCallback), this);
        gst_bus_remove_signal_watch(bus.get());

        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateSourceChangedCallback), this);
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateVideoNeedKeyCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateElementAddedCallback), this);
#endif
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateVideoChangedCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateAudioChangedCallback), this);
#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateNewTextSampleCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateTextChangedCallback), this);
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateSourceSetupCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerElementAddedCallback), this);
#endif
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
        m_playBin.clear();
    }


    GRefPtr<GstPad> videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink.get(), "sink"));
    g_signal_handlers_disconnect_by_func(videoSinkPad.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateVideoSinkCapsChangedCallback), this);

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    AudioSessionManagerGStreamerTizen::removeCallbackClient(this);

    if (m_audioSessionManager)
        m_audioSessionManager->setSoundState(ASM_STATE_STOP);

    if (m_playStateTimerHandler)
        g_source_remove(m_playStateTimerHandler);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

    if (m_videoTimerHandler)
        g_source_remove(m_videoTimerHandler);

    if (m_audioTimerHandler)
        g_source_remove(m_audioTimerHandler);

    if (m_textTimerHandler)
        g_source_remove(m_textTimerHandler);

    if (m_videoCapsTimerHandler)
        g_source_remove(m_videoCapsTimerHandler);

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    if (m_videoDrmBridge)
        m_videoDrmBridge.clear();

    if (m_audioDrmBridge)
        m_audioDrmBridge.clear();
#endif
}

void MediaPlayerPrivateGStreamer::load(const String& url)
{
    if (!initializeGStreamerAndRegisterWebKitElements())
        return;

#if ENABLE(TIZEN_MEDIA_STREAM)
    crackURI(url);
#endif // ENABLE(TIZEN_MEDIA_STREAM)

    KURL kurl(KURL(), url);
    String cleanUrl(url);

    // Clean out everything after file:// url path.
    if (kurl.isLocalFile())
        cleanUrl = cleanUrl.substring(0, kurl.pathEnd());

    if (!m_playBin)
        createGSTPlayBin();

    ASSERT(m_playBin);

    m_url = KURL(KURL(), cleanUrl);
    g_object_set(m_playBin.get(), "uri", cleanUrl.utf8().data(), NULL);

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream())
        g_object_set(m_webkitVideoSink.get(), "sync", 0, NULL);
#endif

    LOG_MEDIA_MESSAGE("Load %s", cleanUrl.utf8().data());

    if (m_preload == MediaPlayer::None) {
        LOG_MEDIA_MESSAGE("Delaying load.");
        m_delayingLoad = true;
    }

    // Reset network and ready states. Those will be set properly once
    // the pipeline pre-rolled.
    m_networkState = MediaPlayer::Loading;
    m_player->networkStateChanged();
    m_readyState = MediaPlayer::HaveNothing;
    m_player->readyStateChanged();
    m_volumeAndMuteInitialized = false;

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager && !m_delayingLoad)
        m_audioSessionManager->setSoundState(ASM_STATE_PAUSE);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

    if (!m_delayingLoad)
        commitLoad();
}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivateGStreamer::load(const String& url, MediaSourcePrivateClient* mediaSource)
{
    String mediasourceUri = String::format("mediasource%s", url.utf8().data());
    m_mediaSource = mediaSource;
    load(mediasourceUri);
}
#endif

void MediaPlayerPrivateGStreamer::commitLoad()
{
    ASSERT(!m_delayingLoad);
    LOG_MEDIA_MESSAGE("Committing load.");

#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    WebSourceChecker::instance().setEnabled(shouldUseWebkitWebSource());
#endif
    // GStreamer needs to have the pipeline set to a paused state to
    // start providing anything useful.
    gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);

    setDownloadBuffering();
    updateStates();
}

float MediaPlayerPrivateGStreamer::playbackPosition() const
{
    if (m_isEndReached) {
        // Position queries on a null pipeline return 0. If we're at
        // the end of the stream the pipeline is null but we want to
        // report either the seek time or the duration because this is
        // what the Media element spec expects us to do.
        if (m_seeking)
            return m_seekTime;
        if (m_mediaDuration)
            return m_mediaDuration;
        return 0;
    }

    // Position is only available if no async state change is going on and the state is either paused or playing.
    gint64 position = GST_CLOCK_TIME_NONE;
    GstQuery* query= gst_query_new_position(GST_FORMAT_TIME);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // During "play -> pause -> play" sequence, time value is a little different per each state.
    // Due to this, progress bar moved in left side while playing after pausing.
    // To solve this issue, if pipeline is paused then we use cached time.
    GstState currentState;
    GstState pending;
    gst_element_get_state(m_playBin.get(), &currentState, &pending, 0);
    if ((currentState == GST_STATE_PAUSED && pending == GST_STATE_VOID_PENDING) || pending == GST_STATE_PAUSED) {
        // "start time" will be 0 after seeking.
        // We don't want to return cached time after seeking.
        GstClockTime time = gst_element_get_start_time(m_playBin.get());
        if (time)
            return m_lastPlaybackPosition;
    }
#endif

    if (gst_element_query(m_playBin.get(), query))
        gst_query_parse_position(query, 0, &position);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    float result = m_seekTime;
#else
    float result = 0.0f;
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (static_cast<GstClockTime>(position) != GST_CLOCK_TIME_NONE)
        result = static_cast<double>(position) / GST_SECOND;
    else if (m_canFallBackToLastFinishedSeekPositon)
        result = m_seekTime;

    LOG_MEDIA_MESSAGE("Position %" GST_TIME_FORMAT, GST_TIME_ARGS(position));

    gst_query_unref(query);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_lastPlaybackPosition = result;
#endif
    return result;
}

bool MediaPlayerPrivateGStreamer::changePipelineState(GstState newState)
{
    ASSERT(newState == GST_STATE_PLAYING || newState == GST_STATE_PAUSED);

    GstState currentState;
    GstState pending;

    gst_element_get_state(m_playBin.get(), &currentState, &pending, 0);
    if (currentState == newState || pending == newState) {
        LOG_MEDIA_MESSAGE("Rejected state change to %s from %s with %s pending", gst_element_state_get_name(newState),
            gst_element_state_get_name(currentState), gst_element_state_get_name(pending));
        return true;
    }

    LOG_MEDIA_MESSAGE("Changing state change to %s from %s with %s pending", gst_element_state_get_name(newState),
        gst_element_state_get_name(currentState), gst_element_state_get_name(pending));

    GstStateChangeReturn setStateResult = gst_element_set_state(m_playBin.get(), newState);
    GstState pausedOrPlaying = newState == GST_STATE_PLAYING ? GST_STATE_PAUSED : GST_STATE_PLAYING;
    if (currentState != pausedOrPlaying && setStateResult == GST_STATE_CHANGE_FAILURE) {
        loadingFailed(MediaPlayer::Empty);
        return false;
    }
    return true;
}

void MediaPlayerPrivateGStreamer::prepareToPlay()
{
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_preload = MediaPlayer::Auto;
#endif
    if (m_delayingLoad) {
        m_delayingLoad = false;
        commitLoad();
    }
}

void MediaPlayerPrivateGStreamer::play()
{
#if ENABLE(TIZEN_GSTREAMER_AUDIO) && ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_audioSessionManager->setSoundState(ASM_STATE_PLAYING) && !isLocalMediaStream())
        return;
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (changePipelineState(GST_STATE_PLAYING)) {
        m_isEndReached = false;
        m_delayingLoad = false;
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
        m_preload = MediaPlayer::Auto;
#endif
        setDownloadBuffering();
        LOG_MEDIA_MESSAGE("Play");
    }
}

void MediaPlayerPrivateGStreamer::pause()
{
#if ENABLE(TIZEN_GSTREAMER_AUDIO) && ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_audioSessionManager->setSoundState(ASM_STATE_PAUSE) && !isLocalMediaStream())
        return;
#if ENABLE(TIZEN_TV_ASM_READY)
    if (!m_audioSessionManager->deallocateResources())
        LOG_MEDIA_MESSAGE("Failed to deallocate resources.");
#endif
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
    GstState currentState, pendingState;
    gst_element_get_state(m_playBin.get(), &currentState, &pendingState, 0);
    if (currentState < GST_STATE_PAUSED && pendingState <= GST_STATE_PAUSED)
        return;

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream()) {
        changePipelineState(GST_STATE_READY);
        TIZEN_LOGI("To Ready State.");
        return;
    }
#endif

    if (changePipelineState(GST_STATE_PAUSED))
        INFO_MEDIA_MESSAGE("Pause");
}

float MediaPlayerPrivateGStreamer::duration() const
{
    if (!m_playBin)
        return 0.0f;

    if (m_errorOccured)
        return 0.0f;

    // Media duration query failed already, don't attempt new useless queries.
    if (!m_mediaDurationKnown)
        return numeric_limits<float>::infinity();

    if (m_mediaDuration)
        return m_mediaDuration;

    GstFormat timeFormat = GST_FORMAT_TIME;
    gint64 timeLength = 0;

#if ENABLE(TIZEN_MSE)
    if (m_mediaSource) {
        LOG_MEDIA_MESSAGE(" MediaPlayerPrivateGStreamer::duration() -> duration from m_mediaSource");
        m_mediaDuration = m_mediaSource->duration();
    } else
#endif
    {
#ifdef GST_API_VERSION_1
    bool failure = !gst_element_query_duration(m_playBin.get(), timeFormat, &timeLength) || static_cast<guint64>(timeLength) == GST_CLOCK_TIME_NONE;
#else
    bool failure = !gst_element_query_duration(m_playBin.get(), &timeFormat, &timeLength) || timeFormat != GST_FORMAT_TIME || static_cast<guint64>(timeLength) == GST_CLOCK_TIME_NONE;
#endif
    if (failure) {
        LOG_MEDIA_MESSAGE("Time duration query failed for %s", m_url.string().utf8().data());
        return numeric_limits<float>::infinity();
    }

    LOG_MEDIA_MESSAGE("Duration: %" GST_TIME_FORMAT, GST_TIME_ARGS(timeLength));

    m_mediaDuration = static_cast<double>(timeLength) / GST_SECOND;
    }
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Notify media duration is changed when get a new value of duration.
    if (m_mediaDuration)
        m_player->durationChanged();
#endif

    return m_mediaDuration;
    // FIXME: handle 3.14.9.5 properly
}

float MediaPlayerPrivateGStreamer::currentTime() const
{
    if (!m_playBin)
        return 0.0f;

    if (m_errorOccured)
        return 0.0f;

    if (m_seeking)
        return m_seekTime;

    // Workaround for
    // https://bugzilla.gnome.org/show_bug.cgi?id=639941 In GStreamer
    // 0.10.35 basesink reports wrong duration in case of EOS and
    // negative playback rate. There's no upstream accepted patch for
    // this bug yet, hence this temporary workaround.
    if (m_isEndReached && m_playbackRate < 0)
        return 0.0f;

    return playbackPosition();
}

void MediaPlayerPrivateGStreamer::seek(float time)
{
    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    INFO_MEDIA_MESSAGE("[Seek] seek attempt to %f secs", time);

    // Avoid useless seeking.
    if (time == currentTime())
        return;

#if ENABLE(TIZEN_MSE)
    // FIXME : This is right place? We need to check more.
    // And need to add post-processing for seeking.
    if (m_mediaSource) {
        MediaTime seekTime = MediaTime::createWithDouble(time);
        bool buffered = m_mediaSource->buffered(seekTime);

        m_mediaSource->seekToTime(seekTime);
        m_isEndReached = false;
        if (buffered)
            timeChanged();

        return;
    }
#endif

    if (isLiveStream())
        return;

    GstClockTime clockTime = toGstClockTime(time);
    INFO_MEDIA_MESSAGE("[Seek] seeking to %" GST_TIME_FORMAT " (%f)", GST_TIME_ARGS(clockTime), time);

    if (m_seeking) {
        m_timeOfOverlappingSeek = time;
        if (m_seekIsPending) {
            m_seekTime = time;
            return;
        }
    }

    GstState state;
    GstStateChangeReturn getStateResult = gst_element_get_state(m_playBin.get(), &state, 0, 0);
    if (getStateResult == GST_STATE_CHANGE_FAILURE || getStateResult == GST_STATE_CHANGE_NO_PREROLL) {
        LOG_MEDIA_MESSAGE("[Seek] cannot seek, current state change is %s", gst_element_state_change_return_get_name(getStateResult));
        return;
    }
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (getStateResult == GST_STATE_CHANGE_ASYNC || state < GST_STATE_PAUSED) {
#else
    if (getStateResult == GST_STATE_CHANGE_ASYNC || state < GST_STATE_PAUSED || m_isEndReached) {
#endif
        m_seekIsPending = true;
        if (m_isEndReached) {
            LOG_MEDIA_MESSAGE("[Seek] reset pipeline");
            m_resetPipeline = true;
            changePipelineState(GST_STATE_PAUSED);
        }
    } else {
        // We can seek now.
        if (!gst_element_seek(m_playBin.get(), m_player->rate(), GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
            GST_SEEK_TYPE_SET, clockTime, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            LOG_MEDIA_MESSAGE("[Seek] seeking to %f failed", time);
            return;
        }
    }

    m_seeking = true;
    m_seekTime = time;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_seekTime != m_mediaDuration) {
        m_isEndReached = false;
        m_lastPlaybackPosition = m_seekTime;
    }
#else
    m_isEndReached = false;
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
}

bool MediaPlayerPrivateGStreamer::paused() const
{
    if (m_isEndReached) {
        LOG_MEDIA_MESSAGE("Ignoring pause at EOS");
        return true;
    }

    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream())
        return state == GST_STATE_READY || state == GST_STATE_PAUSED;
#endif
    return state == GST_STATE_PAUSED;
}

#if ENABLE(TIZEN_MEDIA_STREAM)
void MediaPlayerPrivateGStreamer::preparePlayForLocalMediaStream()
{
    if (isLocalMediaStream() && m_playBin.get())
        gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
}

bool MediaPlayerPrivateGStreamer::isLocalMediaStreamPaused()
{
    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);

    return state == GST_STATE_READY;
}
#endif

bool MediaPlayerPrivateGStreamer::seeking() const
{
    return m_seeking;
}

void MediaPlayerPrivateGStreamer::videoChanged()
{
    if (m_videoTimerHandler)
        g_source_remove(m_videoTimerHandler);
    m_videoTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateVideoChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::videoCapsChanged()
{
    if (m_videoCapsTimerHandler)
        g_source_remove(m_videoCapsTimerHandler);
    m_videoCapsTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateVideoCapsChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfVideo()
{
    m_videoTimerHandler = 0;

    gint numTracks = 0;
    if (m_playBin)
        g_object_get(m_playBin.get(), "n-video", &numTracks, NULL);

    m_hasVideo = numTracks > 0;

#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    for (gint i = 0; i < numTracks; ++i) {
        GRefPtr<GstPad> pad;
        g_signal_emit_by_name(m_playBin.get(), "get-video-pad", i, &pad.outPtr(), NULL);
        ASSERT(pad);

        if (i < static_cast<gint>(m_videoTracks.size())) {
            RefPtr<VideoTrackPrivateGStreamer> existingTrack = m_videoTracks[i];
            existingTrack->setIndex(i);
            if (existingTrack->pad() == pad)
                continue;
        }

        RefPtr<VideoTrackPrivateGStreamer> track = VideoTrackPrivateGStreamer::create(m_playBin, i, pad);
        m_videoTracks.append(track);
        m_player->addVideoTrack(track.release());
    }

    while (static_cast<gint>(m_videoTracks.size()) > numTracks) {
        RefPtr<VideoTrackPrivateGStreamer> track = m_videoTracks.last();
        track->disconnect();
        m_videoTracks.removeLast();
        m_player->removeVideoTrack(track.release());
    }
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream()) {
        m_hasVideo = true;
        m_videoSize = IntSize(LocalMediaServer::instance().width(), LocalMediaServer::instance().height());
    }
#endif
    m_player->mediaPlayerClient()->mediaPlayerEngineUpdated(m_player);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfVideoCaps()
{
    m_videoCapsTimerHandler = 0;
    m_videoSize = IntSize();
    m_player->mediaPlayerClient()->mediaPlayerEngineUpdated(m_player);
}

void MediaPlayerPrivateGStreamer::audioChanged()
{
    if (m_audioTimerHandler)
        g_source_remove(m_audioTimerHandler);
    m_audioTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateAudioChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfAudio()
{
    m_audioTimerHandler = 0;

    gint numTracks = 0;
    if (m_playBin)
        g_object_get(m_playBin.get(), "n-audio", &numTracks, NULL);

    m_hasAudio = numTracks > 0;

#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    for (gint i = 0; i < numTracks; ++i) {
        GRefPtr<GstPad> pad;
        g_signal_emit_by_name(m_playBin.get(), "get-audio-pad", i, &pad.outPtr(), NULL);
        ASSERT(pad);

        if (i < static_cast<gint>(m_audioTracks.size())) {
            RefPtr<AudioTrackPrivateGStreamer> existingTrack = m_audioTracks[i];
            existingTrack->setIndex(i);
            if (existingTrack->pad() == pad)
                continue;
        }

        RefPtr<AudioTrackPrivateGStreamer> track = AudioTrackPrivateGStreamer::create(m_playBin, i, pad);
        m_audioTracks.insert(i, track);
        m_player->addAudioTrack(track.release());
    }

    while (static_cast<gint>(m_audioTracks.size()) > numTracks) {
        RefPtr<AudioTrackPrivateGStreamer> track = m_audioTracks.last();
        track->disconnect();
        m_audioTracks.removeLast();
        m_player->removeAudioTrack(track.release());
    }
#endif

    m_player->mediaPlayerClient()->mediaPlayerEngineUpdated(m_player);
}

#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
void MediaPlayerPrivateGStreamer::textChanged()
{
    if (m_textTimerHandler)
        g_source_remove(m_textTimerHandler);
    m_textTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateTextChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfText()
{
    m_textTimerHandler = 0;

    gint numTracks = 0;
    if (m_playBin)
        g_object_get(m_playBin.get(), "n-text", &numTracks, NULL);

    for (gint i = 0; i < numTracks; ++i) {
        GstPad* pad;
        g_signal_emit_by_name(m_playBin.get(), "get-text-pad", i, &pad, NULL);
        ASSERT(pad);

        if (i < static_cast<gint>(m_textTracks.size())) {
            RefPtr<InbandTextTrackPrivateGStreamer> existingTrack = m_textTracks[i];
            existingTrack->setIndex(i);
            if (existingTrack->pad() == pad) {
                gst_object_unref(pad);
                continue;
            }
        }

        RefPtr<InbandTextTrackPrivateGStreamer> track = InbandTextTrackPrivateGStreamer::create(i, adoptGRef(pad));
        m_textTracks.insert(i, track);
        m_player->addTextTrack(track.release());
    }

    while (static_cast<gint>(m_textTracks.size()) > numTracks) {
        RefPtr<InbandTextTrackPrivateGStreamer> track = m_textTracks.last();
        track->disconnect();
        m_textTracks.removeLast();
        m_player->removeTextTrack(track.release());
    }
}

void MediaPlayerPrivateGStreamer::newTextSample()
{
    if (!m_textAppSink)
        return;

    GRefPtr<GstEvent> streamStartEvent = adoptGRef(
        gst_pad_get_sticky_event(m_textAppSinkPad.get(), GST_EVENT_STREAM_START, 0));

    GstSample* sample;
    g_signal_emit_by_name(m_textAppSink.get(), "pull-sample", &sample, NULL);
    ASSERT(sample);

    if (streamStartEvent) {
        bool found = FALSE;
        const gchar* id;
        gst_event_parse_stream_start(streamStartEvent.get(), &id);
        for (size_t i = 0; i < m_textTracks.size(); ++i) {
            RefPtr<InbandTextTrackPrivateGStreamer> track = m_textTracks[i];
            if (track->streamId() == id) {
                track->handleSample(sample);
                found = true;
                break;
            }
        }
        if (!found)
            WARN_MEDIA_MESSAGE("Got sample with unknown stream ID.");
    } else
        WARN_MEDIA_MESSAGE("Unable to handle sample with no stream start event.");
    gst_sample_unref(sample);
}
#endif

void MediaPlayerPrivateGStreamer::setRate(float rate)
{
    // Avoid useless playback rate update.
    if (m_playbackRate == rate)
        return;

    GstState state;
    GstState pending;

    gst_element_get_state(m_playBin.get(), &state, &pending, 0);
    if ((state != GST_STATE_PLAYING && state != GST_STATE_PAUSED)
        || (pending == GST_STATE_PAUSED))
        return;

    if (isLiveStream())
        return;

    m_playbackRate = rate;
    m_changingRate = true;

    if (!rate) {
        gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
        if (m_audioSessionManager)
            m_audioSessionManager->setSoundState(ASM_STATE_PAUSE);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
        return;
    }

    float currentPosition = static_cast<float>(playbackPosition() * GST_SECOND);
    GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH);
    gint64 start, end;
    bool mute = false;

    INFO_MEDIA_MESSAGE("Set Rate to %f", rate);
    if (rate > 0) {
        // Mute the sound if the playback rate is too extreme and
        // audio pitch is not adjusted.
        mute = (!m_preservesPitch && (rate < 0.8 || rate > 2));
        start = currentPosition;
        end = GST_CLOCK_TIME_NONE;
    } else {
        start = 0;
        mute = true;

        // If we are at beginning of media, start from the end to
        // avoid immediate EOS.
        if (currentPosition <= 0)
            end = static_cast<gint64>(duration() * GST_SECOND);
        else
            end = currentPosition;
    }

    INFO_MEDIA_MESSAGE("Need to mute audio?: %d", (int) mute);

    if (!gst_element_seek(m_playBin.get(), rate, GST_FORMAT_TIME, flags,
                          GST_SEEK_TYPE_SET, start,
                          GST_SEEK_TYPE_SET, end))
        ERROR_MEDIA_MESSAGE("Set rate to %f failed", rate);
    else
        g_object_set(m_playBin.get(), "mute", mute, NULL);
}

void MediaPlayerPrivateGStreamer::setPreservesPitch(bool preservesPitch)
{
    m_preservesPitch = preservesPitch;
}

PassRefPtr<TimeRanges> MediaPlayerPrivateGStreamer::buffered() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    if (m_errorOccured || isLiveStream())
        return timeRanges.release();

#if GST_CHECK_VERSION(0, 10, 31)
    float mediaDuration(duration());
    if (!mediaDuration || std::isinf(mediaDuration))
        return timeRanges.release();

    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (!gst_element_query(m_playBin.get(), query)) {
        gst_query_unref(query);
        return timeRanges.release();
    }

    for (guint index = 0; index < gst_query_get_n_buffering_ranges(query); index++) {
        gint64 rangeStart = 0, rangeStop = 0;
        if (gst_query_parse_nth_buffering_range(query, index, &rangeStart, &rangeStop))
            timeRanges->add(static_cast<float>((rangeStart * mediaDuration) / gPercentMax),
                static_cast<float>((rangeStop * mediaDuration) / gPercentMax));
    }

    // Fallback to the more general maxTimeLoaded() if no range has
    // been found.
    if (!timeRanges->length())
        if (float loaded = maxTimeLoaded())
            timeRanges->add(0, loaded);

    gst_query_unref(query);
#else
    float loaded = maxTimeLoaded();
    if (!m_errorOccured && !isLiveStream() && loaded > 0)
        timeRanges->add(0, loaded);
#endif
    return timeRanges.release();
}

gboolean MediaPlayerPrivateGStreamer::handleMessage(GstMessage* message)
{
    GOwnPtr<GError> err;
    GOwnPtr<gchar> debug;
    MediaPlayer::NetworkState error;
    bool issueError = true;
    bool attemptNextLocation = false;
    const GstStructure* structure = gst_message_get_structure(message);
    GstState requestedState, currentState;

    m_canFallBackToLastFinishedSeekPositon = false;

    if (structure) {
        const gchar* messageTypeName = gst_structure_get_name(structure);

        // Redirect messages are sent from elements, like qtdemux, to
        // notify of the new location(s) of the media.
        if (!g_strcmp0(messageTypeName, "redirect")) {
            mediaLocationChanged(message);
            return TRUE;
        }
    }

    // We ignore state changes from internal elements. They are forwarded to playbin2 anyway.
    bool messageSourceIsPlaybin = GST_MESSAGE_SRC(message) == reinterpret_cast<GstObject*>(m_playBin.get());

    LOG_MEDIA_MESSAGE("Message %s received from element %s", GST_MESSAGE_TYPE_NAME(message), GST_MESSAGE_SRC_NAME(message));
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        if (m_resetPipeline)
            break;
        if (m_missingPlugins)
            break;
        gst_message_parse_error(message, &err.outPtr(), &debug.outPtr());
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        ERROR_MEDIA_MESSAGE("Message %s received from element %s", GST_MESSAGE_TYPE_NAME(message), GST_MESSAGE_SRC_NAME(message));
#endif
        ERROR_MEDIA_MESSAGE("Error %d: %s (url=%s)", err->code, err->message, m_url.string().utf8().data());

        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_playBin.get()), GST_DEBUG_GRAPH_SHOW_ALL, "webkit-video.error");

        error = MediaPlayer::Empty;
        if (err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND
            || err->code == GST_STREAM_ERROR_WRONG_TYPE
            || err->code == GST_STREAM_ERROR_FAILED
            || err->code == GST_CORE_ERROR_MISSING_PLUGIN
            || err->code == GST_RESOURCE_ERROR_NOT_FOUND)
            error = MediaPlayer::FormatError;
        else if (err->domain == GST_STREAM_ERROR) {
            // Let the mediaPlayerClient handle the stream error, in
            // this case the HTMLMediaElement will emit a stalled
            // event.
#if !ENABLE(TIZEN_MEDIA_STREAM)
            if (err->code == GST_STREAM_ERROR_TYPE_NOT_FOUND) {
                ERROR_MEDIA_MESSAGE("Decode error, let the Media element emit a stalled event.");
                break;
            }
#endif // !ENABLE(TIZEN_MEDIA_STREAM)
            error = MediaPlayer::DecodeError;
            attemptNextLocation = true;
        } else if (err->domain == GST_RESOURCE_ERROR)
            error = MediaPlayer::NetworkError;

        if (attemptNextLocation)
            issueError = !loadNextLocation();
        if (issueError)
            loadingFailed(error);
        break;
    case GST_MESSAGE_EOS:
        didEnd();
        break;
    case GST_MESSAGE_ASYNC_DONE:
        if (!messageSourceIsPlaybin || m_delayingLoad)
            break;
        asyncStateChangeDone();
        break;
    case GST_MESSAGE_STATE_CHANGED: {
        if (!messageSourceIsPlaybin || m_delayingLoad)
            break;
        updateStates();

        // Construct a filename for the graphviz dot file output.
        GstState newState;
        gst_message_parse_state_changed(message, &currentState, &newState, 0);
#if ENABLE(TIZEN_TV_ASM_READY)
        // When playbin finishes automatic pipeline creation it changes state from
        // GST_STATE_READY to GST_STATE_PAUSED.
        // This method shall be called from MediaPlayerPrivateGStreamer::handleMessage(...)
        // on that occasion.
        if ((currentState == GST_STATE_READY) && (newState == GST_STATE_PAUSED)) {
            LOG_MEDIA_MESSAGE("Allocating A/V resources for GST.");
            if (!m_audioSessionManager->allocateResources(m_playBin.get())) {
                LOG_MEDIA_MESSAGE("Failed to allocate resources.");
                return false;
            }
        }
#endif
        CString dotFileName = String::format("webkit-video.%s_%s", gst_element_state_get_name(currentState), gst_element_state_get_name(newState)).utf8();
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_playBin.get()), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.data());

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        if (m_player->mediaPlayerClient()->suspended() && newState == GST_STATE_PAUSED) {
            if (m_suspendTime > 0 && !(isLocalMediaStream() || m_isEndReached))
                seek(m_suspendTime);
            m_suspendTime = 0;
            if (!isLocalMediaStream())
                m_player->mediaPlayerClient()->setSuspended(false);
        }
#if USE(ACCELERATED_COMPOSITING)
        else if (newState == GST_STATE_PLAYING) {
            m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(0);
            m_player->mediaPlayerClient()->setSuspended(false);
        }
#endif

#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

        break;
    }
    case GST_MESSAGE_BUFFERING:
        processBufferingStats(message);
        break;
#ifdef GST_API_VERSION_1
    case GST_MESSAGE_DURATION_CHANGED:
#else
    case GST_MESSAGE_DURATION:
#endif
        if (messageSourceIsPlaybin)
            durationChanged();
        break;
    case GST_MESSAGE_REQUEST_STATE:
        gst_message_parse_request_state(message, &requestedState);
        gst_element_get_state(m_playBin.get(), &currentState, NULL, 250);
        if (requestedState < currentState) {
            GOwnPtr<gchar> elementName(gst_element_get_name(GST_ELEMENT(message)));
            INFO_MEDIA_MESSAGE("Element %s requested state change to %s", elementName.get(),
                gst_element_state_get_name(requestedState));
            m_requestedState = requestedState;
            changePipelineState(requestedState);
        }
        break;
    case GST_MESSAGE_ELEMENT:
        if (gst_is_missing_plugin_message(message)) {
            gchar* detail = gst_missing_plugin_message_get_installer_detail(message);
            gchar* detailArray[2] = {detail, 0};
            GstInstallPluginsReturn result = gst_install_plugins_async(detailArray, 0, mediaPlayerPrivatePluginInstallerResultFunction, this);
            m_missingPlugins = result == GST_INSTALL_PLUGINS_STARTED_OK;
            g_free(detail);
        }
        break;
    default:
        LOG_MEDIA_MESSAGE("Unhandled GStreamer message type: %s",
                    GST_MESSAGE_TYPE_NAME(message));
        break;
    }
    return TRUE;
}

void MediaPlayerPrivateGStreamer::handlePluginInstallerResult(GstInstallPluginsReturn result)
{
    m_missingPlugins = false;
    if (result == GST_INSTALL_PLUGINS_SUCCESS) {
        gst_element_set_state(m_playBin.get(), GST_STATE_READY);
        gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
    }
}

void MediaPlayerPrivateGStreamer::processBufferingStats(GstMessage* message)
{
    m_buffering = true;
    const GstStructure *structure = gst_message_get_structure(message);
    gst_structure_get_int(structure, "buffer-percent", &m_bufferingPercentage);

    LOG_MEDIA_MESSAGE("[Buffering] Buffering: %d%%.", m_bufferingPercentage);

    updateStates();
}

void MediaPlayerPrivateGStreamer::fillTimerFired(Timer<MediaPlayerPrivateGStreamer>*)
{
    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (!gst_element_query(m_playBin.get(), query)) {
        gst_query_unref(query);
        return;
    }

    gint64 start, stop;
    gdouble fillStatus = 100.0;

    gst_query_parse_buffering_range(query, 0, &start, &stop, 0);
    gst_query_unref(query);

    if (stop != -1)
        fillStatus = 100.0 * stop / GST_FORMAT_PERCENT_MAX;

    LOG_MEDIA_MESSAGE("[Buffering] Download buffer filled up to %f%%", fillStatus);

    if (!m_mediaDuration)
        durationChanged();

    // Update maxTimeLoaded only if the media duration is
    // available. Otherwise we can't compute it.
    if (m_mediaDuration) {
        if (fillStatus == 100.0)
            m_maxTimeLoaded = m_mediaDuration;
        else
            m_maxTimeLoaded = static_cast<float>((fillStatus * m_mediaDuration) / 100.0);
        LOG_MEDIA_MESSAGE("[Buffering] Updated maxTimeLoaded: %f", m_maxTimeLoaded);
    }

    m_downloadFinished = fillStatus == 100.0;
    if (!m_downloadFinished) {
        updateStates();
        return;
    }

    // Media is now fully loaded. It will play even if network
    // connection is cut. Buffering is done, remove the fill source
    // from the main loop.
    m_fillTimer.stop();
    updateStates();
}

float MediaPlayerPrivateGStreamer::maxTimeSeekable() const
{
    if (m_errorOccured)
        return 0.0f;

    LOG_MEDIA_MESSAGE("maxTimeSeekable");
    // infinite duration means live stream
    if (std::isinf(duration()))
        return 0.0f;

    return duration();
}

float MediaPlayerPrivateGStreamer::maxTimeLoaded() const
{
    if (m_errorOccured)
        return 0.0f;

    float loaded = m_maxTimeLoaded;
    if (m_isEndReached && m_mediaDuration)
        loaded = m_mediaDuration;
    LOG_MEDIA_MESSAGE("maxTimeLoaded: %f", loaded);
    return loaded;
}

bool MediaPlayerPrivateGStreamer::didLoadingProgress() const
{
    if (!m_playBin || !m_mediaDuration || !totalBytes())
        return false;
    float currentMaxTimeLoaded = maxTimeLoaded();
    bool didLoadingProgress = currentMaxTimeLoaded != m_maxTimeLoadedAtLastDidLoadingProgress;
    m_maxTimeLoadedAtLastDidLoadingProgress = currentMaxTimeLoaded;
    LOG_MEDIA_MESSAGE("didLoadingProgress: %d", didLoadingProgress);
    return didLoadingProgress;
}

unsigned MediaPlayerPrivateGStreamer::totalBytes() const
{
    if (m_errorOccured)
        return 0;

    if (m_totalBytes != -1)
        return m_totalBytes;

    if (!m_source)
        return 0;

    GstFormat fmt = GST_FORMAT_BYTES;
    gint64 length = 0;
#ifdef GST_API_VERSION_1
    if (gst_element_query_duration(m_source.get(), fmt, &length)) {
#else
    if (gst_element_query_duration(m_source.get(), &fmt, &length)) {
#endif
        INFO_MEDIA_MESSAGE("totalBytes %" G_GINT64_FORMAT, length);
        m_totalBytes = static_cast<unsigned>(length);
        m_isStreaming = !length;
        return m_totalBytes;
    }

    // Fall back to querying the source pads manually.
    // See also https://bugzilla.gnome.org/show_bug.cgi?id=638749
    GstIterator* iter = gst_element_iterate_src_pads(m_source.get());
    bool done = false;
    while (!done) {
#ifdef GST_API_VERSION_1
        GValue item = G_VALUE_INIT;
        switch (gst_iterator_next(iter, &item)) {
        case GST_ITERATOR_OK: {
            GstPad* pad = static_cast<GstPad*>(g_value_get_object(&item));
            gint64 padLength = 0;
            if (gst_pad_query_duration(pad, fmt, &padLength) && padLength > length)
                length = padLength;
            break;
        }
#else
        gpointer data;

        switch (gst_iterator_next(iter, &data)) {
        case GST_ITERATOR_OK: {
            GRefPtr<GstPad> pad = adoptGRef(GST_PAD_CAST(data));
            gint64 padLength = 0;
            if (gst_pad_query_duration(pad.get(), &fmt, &padLength) && padLength > length)
                length = padLength;
            break;
        }
#endif
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync(iter);
            break;
        case GST_ITERATOR_ERROR:
            // Fall through.
        case GST_ITERATOR_DONE:
            done = true;
            break;
        }

#ifdef GST_API_VERSION_1
        g_value_unset(&item);
#endif
    }

    gst_iterator_free(iter);

    INFO_MEDIA_MESSAGE("totalBytes %" G_GINT64_FORMAT, length);
    m_totalBytes = static_cast<unsigned>(length);
    m_isStreaming = !length;
    return m_totalBytes;
}

void MediaPlayerPrivateGStreamer::updateAudioSink()
{
    if (!m_playBin)
        return;

    GstElement* sinkPtr = 0;

    g_object_get(m_playBin.get(), "audio-sink", &sinkPtr, NULL);
    m_webkitAudioSink = adoptGRef(sinkPtr);
}

GstElement* MediaPlayerPrivateGStreamer::audioSink() const
{
    return m_webkitAudioSink.get();
}

void MediaPlayerPrivateGStreamer::sourceChanged()
{
    GstElement* srcPtr = 0;

    g_object_get(m_playBin.get(), "source", &srcPtr, NULL);
    m_source = adoptGRef(srcPtr);

    if (WEBKIT_IS_WEB_SRC(m_source.get()))
        webKitWebSrcSetMediaPlayer(WEBKIT_WEB_SRC(m_source.get()), m_player);
#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSource && WEBKIT_IS_MEDIA_SRC(m_source.get())) {
        MediaSourceGStreamer::open(m_mediaSource.get(), WEBKIT_MEDIA_SRC(m_source.get()));
        webKitMediaSrcSetPlayBin(WEBKIT_MEDIA_SRC(m_source.get()), m_playBin.get());
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
        webKitMediaSrcSetMediaPlayer(WEBKIT_MEDIA_SRC(m_source.get()), m_player);
#endif // ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    }
#endif
}

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
void MediaPlayerPrivateGStreamer::elementAdded(GstElement* element)
{
    gchar* factoryName = GST_PLUGIN_FEATURE_NAME(gst_element_get_factory(GST_ELEMENT(element)));
    GstElement* srcPtr = 0;

    g_object_get(m_playBin.get(), "source", &srcPtr, NULL);
    m_source = adoptGRef(srcPtr);

    LOG_MEDIA_MESSAGE("added factory name = %s", factoryName);

    if (strstr(factoryName, "uridecodebin") || strstr(factoryName, "decodebin2")) {
        g_signal_connect(element, "element-added", G_CALLBACK(mediaPlayerPrivateElementAddedCallback), this);

        if (m_mediaSource && WEBKIT_IS_MEDIA_SRC(m_source.get())) {
            LOG_MEDIA_MESSAGE("enable decrypt mode in decodebin2");
            g_object_set(element, "need-decrypt", TRUE, NULL);
        }
        return;
    }

    if (strstr(factoryName, "DRMBridge")) {
        LOG_MEDIA_MESSAGE("drm bridge element(%s) is added", GST_ELEMENT_NAME(element));

        if (m_mediaSource && WEBKIT_IS_MEDIA_SRC(m_source.get())) {
            LOG_MEDIA_MESSAGE("set drm mode to decryption only. %d", DRMBRIDGE_MODE_DECRYPTION);
            g_object_set(element, "drmbridge-mode", DRMBRIDGE_MODE_DECRYPTION, NULL);
        }
        else {
            getDrmElement(element);
            g_signal_connect(element, "need-key", G_CALLBACK(mediaPlayerPrivateVideoNeedKeyCallback), this);
        }
    }
}

void MediaPlayerPrivateGStreamer::getDrmElement(GstElement* element)
{
    if (!element) {
        LOG_MEDIA_MESSAGE("drm element is corrupted...");
        return;
    }

// Webkit can not distinguish between video drm element or audio drm element. generally, audio data's drm element is added before video's .
// if container has only audio data or video data, playbin2 can have one drmbridge element.
    if (!m_audioDrmBridge) {
        m_audioDrmBridge = adoptGRef(element);
        LOG_MEDIA_MESSAGE("the 1st DRMBridge is %s", GST_ELEMENT_NAME(m_audioDrmBridge.get()));
    }
    else if (m_audioDrmBridge && !m_videoDrmBridge) {
        m_videoDrmBridge = adoptGRef(element);
        LOG_MEDIA_MESSAGE("the 2nd DRMBridge is %s", GST_ELEMENT_NAME(m_videoDrmBridge.get()));
    }
}

// According to http://www.w3.org/TR/encrypted-media/#introduction, graph
// 1) get needkey event with initData(contents specific data)
// 2) generate request
// 3) get license request
// 4) provide license
void MediaPlayerPrivateGStreamer::videoNeedKey(GstElement* element, gpointer initData, guint initDataLength, gpointer /* userData */)
{
    LOG_MEDIA_MESSAGE("videoNeedKey");

    GstElement* srcPtr = 0;
    g_object_get(m_playBin.get(), "source", &srcPtr, NULL);
    m_source = adoptGRef(srcPtr);
#if ENABLE(MEDIA_SOURCE)
    if (m_mediaSource && WEBKIT_IS_MEDIA_SRC(m_source.get())) {
        // 1) needkey
        LOG_MEDIA_MESSAGE("initData = %p, len=%d", static_cast<unsigned char *>(initData), initDataLength);
        webKitMediaVideoSrcNeedKeyCb(element, initData, initDataLength, reinterpret_cast<gpointer>(WEBKIT_MEDIA_SRC(m_source.get())));
    }
#endif
}

bool MediaPlayerPrivateGStreamer::generateKeyRequest(char* keySystem, unsigned char* initData, unsigned initDataLength, unsigned char** keyRequest, unsigned* keyRequestLength)
{
    LOG_MEDIA_MESSAGE("generateKeyRequest is called!");

    if (!m_videoDrmBridge && !m_audioDrmBridge) {
        LOG_MEDIA_MESSAGE("There is no DRMBridge element...");
        return false;
    }

    if (strstr(keySystem, "webkit-org.w3.clearkey")) {
        if (m_videoDrmBridge) {
            g_object_set(m_videoDrmBridge.get(), "drmbridge-drm-type-streaming", drmTypeClearkey, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s", drmTypeClearkey, GST_ELEMENT_NAME(m_videoDrmBridge.get()));
            }
        if (m_audioDrmBridge) {
            g_object_set(m_audioDrmBridge.get(), "drmbridge-drm-type-streaming", drmTypeClearkey, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s", drmTypeClearkey, GST_ELEMENT_NAME(m_audioDrmBridge.get()));
        }
    }

    else if (strstr(keySystem, "com.youtube.playready")) {
        unsigned char* playreadyInitData = static_cast<unsigned char*>(malloc(initDataLength*sizeof(unsigned char)));
        memcpy(playreadyInitData, initData, initDataLength);
        LOG_MEDIA_MESSAGE("playreadyInitDataLen = %d", initDataLength);

        for (unsigned int i =0 ;i < initDataLength; i++ ) {
            LOG_MEDIA_MESSAGE("playreadyInitData = 0x%02x", playreadyInitData[i]);
        }

        if (m_videoDrmBridge) {
            g_object_set(m_videoDrmBridge.get(), "drmbridge-drm-type-streaming", drmTypePlayready, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s",drmTypePlayready, GST_ELEMENT_NAME(m_videoDrmBridge.get()));

            // 2) generate request
            g_object_set(m_videoDrmBridge.get(), "drmbridge-playready-challenge-data", playreadyInitData, NULL);
            g_object_set(m_videoDrmBridge.get(), "drmbridge-playready-challenge-len", initDataLength, NULL);

            LOG_MEDIA_MESSAGE("generate request is done to %s", GST_ELEMENT_NAME(m_videoDrmBridge.get()));

            // 3) get license request
            GOwnPtr<guchar> getKeyRequest;
            unsigned getKeyRequestLength = 0;
            g_object_get(m_videoDrmBridge.get(), "drmbridge-playready-key-request", &getKeyRequest.outPtr(), NULL);
            g_object_get(m_videoDrmBridge.get(), "drmbridge-playready-key-request-len", &getKeyRequestLength, NULL);

            if (!getKeyRequest || getKeyRequestLength == 0) {
                LOG_MEDIA_MESSAGE("failed to get license request...");
                return false;
            }
            else
                LOG_MEDIA_MESSAGE("getKeyRequest=%p, getKeyRequestLength=%d", getKeyRequest.get(), getKeyRequestLength);

            *keyRequestLength = getKeyRequestLength;
            *keyRequest = static_cast<unsigned char*>(malloc(getKeyRequestLength*sizeof(unsigned char)));
            memcpy(*keyRequest, getKeyRequest.get(), getKeyRequestLength);
            LOG_MEDIA_MESSAGE("complete get license request from %s", GST_ELEMENT_NAME(m_videoDrmBridge.get()));
        }

        if (m_audioDrmBridge) {
            g_object_set(m_audioDrmBridge.get(), "drmbridge-drm-type-streaming", drmTypePlayready, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s", drmTypePlayready, GST_ELEMENT_NAME(m_audioDrmBridge.get()));

            // 2) generate request
            g_object_set(m_audioDrmBridge.get(), "drmbridge-playready-challenge-data", playreadyInitData, NULL);
            g_object_set(m_audioDrmBridge.get(), "drmbridge-playready-challenge-len", initDataLength, NULL);
            LOG_MEDIA_MESSAGE("generate request is done to %s", GST_ELEMENT_NAME(m_audioDrmBridge.get()));

            // 3) get license request
            GOwnPtr<guchar> getKeyRequest;
            unsigned getKeyRequestLength = 0;
            g_object_get(m_audioDrmBridge.get(), "drmbridge-playready-key-request", &getKeyRequest.outPtr(), NULL);
            g_object_get(m_audioDrmBridge.get(), "drmbridge-playready-key-request-len", &getKeyRequestLength, NULL);

            if (!getKeyRequest || getKeyRequestLength == 0) {
                LOG_MEDIA_MESSAGE("failed to get license request...");
                return false;
            }
            else
                LOG_MEDIA_MESSAGE("getKeyRequest=%p, getKeyRequestLength=%d", getKeyRequest.get(), getKeyRequestLength);

            // if already received keyrequest data from another drm element, don't need to get it again (precondition : audio&video are encrypted by same key)
            if (!m_videoDrmBridge) {
                *keyRequestLength = getKeyRequestLength;
                *keyRequest = static_cast<unsigned char*>(malloc(getKeyRequestLength*sizeof(unsigned char)));
                memcpy(*keyRequest, getKeyRequest.get(), getKeyRequestLength);
                LOG_MEDIA_MESSAGE("complete get license request from %s", GST_ELEMENT_NAME(m_audioDrmBridge.get()));
            }
        }

        if (playreadyInitData)
            free(playreadyInitData);
    }

    return true;
}

bool MediaPlayerPrivateGStreamer::updateKey(char* keySystem, unsigned char* key, unsigned keyLength, unsigned char* /*initData*/, unsigned /*initDataLength*/)
{
    LOG_MEDIA_MESSAGE("updateKey is called!");
    LOG_MEDIA_MESSAGE("keysystem %s, key = %p, keylen=%d", keySystem, key, keyLength);

    // FIXME: property implementation should not contain any structre for parameter.
    LicenseData* licenseData = (LicenseData*)malloc(sizeof(LicenseData));
    licenseData->key = static_cast<unsigned char*>(malloc(keyLength*sizeof(unsigned char)));
    memcpy(licenseData->key, key, keyLength);
    licenseData->keyLength = static_cast<int>(keyLength);
    LOG_MEDIA_MESSAGE("licenseData = %p", static_cast<gpointer>(licenseData));

    // 4) Provide license
    if (m_videoDrmBridge) {
        if (strstr(keySystem, "webkit-org.w3.clearkey")) {
                g_object_set(m_videoDrmBridge.get(), "drmbridge-clearkey-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("clearkey provide license is done to %s",GST_ELEMENT_NAME(m_videoDrmBridge.get()));
        }
        else if (strstr(keySystem, "com.youtube.playready")) {
                g_object_set(m_videoDrmBridge.get(), "drmbridge-playready-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("playready provide license is done to %s",GST_ELEMENT_NAME(m_videoDrmBridge.get()));
        }
    }

    if (m_audioDrmBridge) {
        if (strstr(keySystem, "webkit-org.w3.clearkey")) {
                g_object_set(m_audioDrmBridge.get(), "drmbridge-clearkey-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("clearkey provide license is done to %s",GST_ELEMENT_NAME(m_audioDrmBridge.get()));
        }
        else if (strstr(keySystem, "com.youtube.playready")) {
                g_object_set(m_audioDrmBridge.get(), "drmbridge-playready-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("playready provide license is done to %s",GST_ELEMENT_NAME(m_audioDrmBridge.get()));
        }
    }

    if (licenseData)
        free(licenseData);

    return true;
}

PassRefPtr<Uint8Array> MediaPlayerPrivateGStreamer::generateKeyRequest(const String& mimeType, const String& keySystem, Uint8Array* initData)
{
    LOG_MEDIA_MESSAGE("MediaPlayerPrivateGStreamer::generateKeyRequest  %p", m_mediaSource.get());
    return m_mediaSource->generateKeyRequest(mimeType, keySystem, initData);
}

bool MediaPlayerPrivateGStreamer::updateKey(const String& mimeType, const String& keySystem, Uint8Array* key, Uint8Array* initData)
{
    return m_mediaSource->updateKey(mimeType, keySystem, key, initData);
}
#endif // ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)

void MediaPlayerPrivateGStreamer::cancelLoad()
{
    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;

    if (m_playBin)
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager)
        m_audioSessionManager->setSoundState(ASM_STATE_STOP);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
}

void MediaPlayerPrivateGStreamer::asyncStateChangeDone()
{
    if (!m_playBin || m_errorOccured)
        return;

    if (m_seeking) {
        if (m_seekIsPending)
            updateStates();
        else {
            LOG_MEDIA_MESSAGE("[Seek] seeked to %f", m_seekTime);
            m_seeking = false;
            if (m_timeOfOverlappingSeek != m_seekTime && m_timeOfOverlappingSeek != -1) {
                seek(m_timeOfOverlappingSeek);
                m_timeOfOverlappingSeek = -1;
                return;
            }
            m_timeOfOverlappingSeek = -1;

            // The pipeline can still have a pending state. In this case a position query will fail.
            // Right now we can use m_seekTime as a fallback.
            m_canFallBackToLastFinishedSeekPositon = true;
            timeChanged();
        }
    } else
        updateStates();
}

void MediaPlayerPrivateGStreamer::updateStates()
{
    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    GstState state;
    GstState pending;

    GstStateChangeReturn getStateResult = gst_element_get_state(m_playBin.get(), &state, &pending, 250 * GST_NSECOND);

    bool shouldUpdatePlaybackState = false;
    switch (getStateResult) {
    case GST_STATE_CHANGE_SUCCESS: {
        LOG_MEDIA_MESSAGE("State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));

        if (state <= GST_STATE_READY) {
            m_resetPipeline = true;
            m_mediaDuration = 0;
        } else {
            m_resetPipeline = false;
            cacheDuration();
        }

        bool didBuffering = m_buffering;

        // Update ready and network states.
        switch (state) {
        case GST_STATE_NULL:
            m_readyState = MediaPlayer::HaveNothing;
            m_networkState = MediaPlayer::Empty;
            break;
        case GST_STATE_READY:
            m_readyState = MediaPlayer::HaveMetadata;
            m_networkState = MediaPlayer::Empty;
            break;
        case GST_STATE_PAUSED:
        case GST_STATE_PLAYING:
            if (m_buffering) {
                if (m_bufferingPercentage == 100) {
                    LOG_MEDIA_MESSAGE("[Buffering] Complete.");
                    m_buffering = false;
                    m_readyState = MediaPlayer::HaveEnoughData;
                    m_networkState = m_downloadFinished ? MediaPlayer::Idle : MediaPlayer::Loading;
                } else {
                    m_readyState = MediaPlayer::HaveCurrentData;
                    m_networkState = MediaPlayer::Loading;
                }
            } else if (m_downloadFinished) {
                m_readyState = MediaPlayer::HaveEnoughData;
                m_networkState = MediaPlayer::Loaded;
            } else {
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
                // FIXME : In our gstreamer, it sometimes fails to query duration when just changed to PAUSE state.
                // It will succeed next try.
                if (m_mediaDurationKnown) {
                    if (url().protocol().startsWith("file", false)) {
                        m_readyState = MediaPlayer::HaveEnoughData;
                        m_networkState = MediaPlayer::Loaded;
                    } else {
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_MSE)
                        if (url().protocol().startsWith("mediasource", false)) {
                            m_readyState = MediaPlayer::HaveNothing;
                            m_networkState = MediaPlayer::Loading;
                        } else {
#endif // ENABLE(TIZEN_MSE)
                    m_readyState = MediaPlayer::HaveFutureData;
                    m_networkState = MediaPlayer::Loading;
                    }
#if ENABLE(TIZEN_MSE)
                    }
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
                }
#endif
            }

            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }

        // Sync states where needed.
        if (state == GST_STATE_PAUSED) {
            if (!m_webkitAudioSink)
                updateAudioSink();

            if (!m_volumeAndMuteInitialized) {
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
                TIZEN_LOGI("%d, %d", m_volumeTimerHandler, m_muteTimerHandler);
                if (m_volumeTimerHandler)
                    g_source_remove(m_volumeTimerHandler);
#endif
                notifyPlayerOfVolumeChange();

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
                if (m_muteTimerHandler)
                    g_source_remove(m_muteTimerHandler);
#endif
                notifyPlayerOfMute();
                m_volumeAndMuteInitialized = true;
            }

            if (didBuffering && !m_buffering && !m_paused) {
                LOG_MEDIA_MESSAGE("[Buffering] Restarting playback.");
                gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
                if (m_audioSessionManager)
                    m_audioSessionManager->setSoundState(ASM_STATE_PLAYING);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
            }
        } else if (state == GST_STATE_PLAYING) {
            m_paused = false;

            if (m_buffering && !isLiveStream()) {
                LOG_MEDIA_MESSAGE("[Buffering] Pausing stream for buffering.");
                gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
                if (m_audioSessionManager)
                    m_audioSessionManager->setSoundState(ASM_STATE_PAUSE);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
            }
        } else
            m_paused = true;

        if (m_changingRate) {
            m_player->rateChanged();
            m_changingRate = false;
        }

        if (m_requestedState == GST_STATE_PAUSED && state == GST_STATE_PAUSED) {
            shouldUpdatePlaybackState = true;
            LOG_MEDIA_MESSAGE("Requested state change to %s was completed", gst_element_state_get_name(state));
        }

        break;
    }
    case GST_STATE_CHANGE_ASYNC:
        LOG_MEDIA_MESSAGE("Async: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));
        // Change in progress.

        // A live stream was paused, reset the pipeline.
#if ENABLE(TIZEN_MEDIA_STREAM)
        if (state == GST_STATE_PAUSED && pending == GST_STATE_PLAYING && isLiveStream() && !isLocalMediaStream()) {
#else
        if (state == GST_STATE_PAUSED && pending == GST_STATE_PLAYING && isLiveStream()) {
#endif // ENABLE(TIZEN_MEDIA_STREAM)
            gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
            gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
            if (m_audioSessionManager)
                m_audioSessionManager->setSoundState(ASM_STATE_PLAYING);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
        }

        break;
    case GST_STATE_CHANGE_FAILURE:
        LOG_MEDIA_MESSAGE("Failure: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));
        // Change failed
        return;
    case GST_STATE_CHANGE_NO_PREROLL:
        LOG_MEDIA_MESSAGE("No preroll: State: %s, pending: %s", gst_element_state_get_name(state), gst_element_state_get_name(pending));

        // Live pipelines go in PAUSED without prerolling.
        m_isStreaming = true;
        setDownloadBuffering();

        if (state == GST_STATE_READY)
            m_readyState = MediaPlayer::HaveNothing;
        else if (state == GST_STATE_PAUSED) {
            m_readyState = MediaPlayer::HaveEnoughData;
            m_paused = true;
        } else if (state == GST_STATE_PLAYING)
            m_paused = false;

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
        if (!m_paused) {
            gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
            if (m_audioSessionManager)
                m_audioSessionManager->setSoundState(ASM_STATE_PLAYING);
        }
#else
        if (!m_paused)
            gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

        m_networkState = MediaPlayer::Loading;
        break;
    default:
        LOG_MEDIA_MESSAGE("Else : %d", getStateResult);
        break;
    }

    m_requestedState = GST_STATE_VOID_PENDING;

    if (shouldUpdatePlaybackState)
        m_player->playbackStateChanged();

    if (m_networkState != oldNetworkState) {
        LOG_MEDIA_MESSAGE("Network State Changed from %u to %u", oldNetworkState, m_networkState);
        m_player->networkStateChanged();
    }
    if (m_readyState != oldReadyState) {
        LOG_MEDIA_MESSAGE("Ready State Changed from %u to %u", oldReadyState, m_readyState);
        m_player->readyStateChanged();
    }

    if (m_seekIsPending && getStateResult == GST_STATE_CHANGE_SUCCESS && (state == GST_STATE_PAUSED || state == GST_STATE_PLAYING)) {
        LOG_MEDIA_MESSAGE("[Seek] committing pending seek to %f", m_seekTime);
        m_seekIsPending = false;
        m_seeking = gst_element_seek(m_playBin.get(), m_player->rate(), GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
            GST_SEEK_TYPE_SET, toGstClockTime(m_seekTime), GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
        if (!m_seeking)
            LOG_MEDIA_MESSAGE("[Seek] seeking to %f failed", m_seekTime);
    }
}

void MediaPlayerPrivateGStreamer::mediaLocationChanged(GstMessage* message)
{
    if (m_mediaLocations)
        gst_structure_free(m_mediaLocations);

    const GstStructure* structure = gst_message_get_structure(message);
    if (structure) {
        // This structure can contain:
        // - both a new-location string and embedded locations structure
        // - or only a new-location string.
        m_mediaLocations = gst_structure_copy(structure);
        const GValue* locations = gst_structure_get_value(m_mediaLocations, "locations");

        if (locations)
            m_mediaLocationCurrentIndex = static_cast<int>(gst_value_list_get_size(locations)) -1;

        loadNextLocation();
    }
}

bool MediaPlayerPrivateGStreamer::loadNextLocation()
{
    if (!m_mediaLocations)
        return false;

    const GValue* locations = gst_structure_get_value(m_mediaLocations, "locations");
    const gchar* newLocation = 0;

    if (!locations) {
        // Fallback on new-location string.
        newLocation = gst_structure_get_string(m_mediaLocations, "new-location");
        if (!newLocation)
            return false;
    }

    if (!newLocation) {
        if (m_mediaLocationCurrentIndex < 0) {
            m_mediaLocations = 0;
            return false;
        }

        const GValue* location = gst_value_list_get_value(locations,
                                                          m_mediaLocationCurrentIndex);
        const GstStructure* structure = gst_value_get_structure(location);

        if (!structure) {
            m_mediaLocationCurrentIndex--;
            return false;
        }

        newLocation = gst_structure_get_string(structure, "new-location");
    }

    if (newLocation) {
        // Found a candidate. new-location is not always an absolute url
        // though. We need to take the base of the current url and
        // append the value of new-location to it.
        KURL baseUrl = gst_uri_is_valid(newLocation) ? KURL() : m_url;
        KURL newUrl = KURL(baseUrl, newLocation);

        RefPtr<SecurityOrigin> securityOrigin = SecurityOrigin::create(m_url);
        if (securityOrigin->canRequest(newUrl)) {
            INFO_MEDIA_MESSAGE("New media url: %s", newUrl.string().utf8().data());

            // Reset player states.
            m_networkState = MediaPlayer::Loading;
            m_player->networkStateChanged();
            m_readyState = MediaPlayer::HaveNothing;
            m_player->readyStateChanged();

            // Reset pipeline state.
            m_resetPipeline = true;
            gst_element_set_state(m_playBin.get(), GST_STATE_READY);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
            if (m_audioSessionManager)
                m_audioSessionManager->setSoundState(ASM_STATE_NONE);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

            GstState state;
            gst_element_get_state(m_playBin.get(), &state, 0, 0);
            if (state <= GST_STATE_READY) {
                // Set the new uri and start playing.
                g_object_set(m_playBin.get(), "uri", newUrl.string().utf8().data(), NULL);
                m_url = newUrl;
                gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
                if (m_audioSessionManager)
                    m_audioSessionManager->setSoundState(ASM_STATE_PLAYING);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
                return true;
            }
        } else
            INFO_MEDIA_MESSAGE("Not allowed to load new media location: %s", newUrl.string().utf8().data());
    }
    m_mediaLocationCurrentIndex--;
    return false;
}

void MediaPlayerPrivateGStreamer::loadStateChanged()
{
    updateStates();
}

void MediaPlayerPrivateGStreamer::timeChanged()
{
    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivateGStreamer::didEnd()
{
    // Synchronize position and duration values to not confuse the
    // HTMLMediaElement. In some cases like reverse playback the
    // position is not always reported as 0 for instance.
    float now = currentTime();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (now > 0 && now != duration()) {
#else
    if (now > 0 && now <= duration() && m_mediaDuration != now) {
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
        m_mediaDurationKnown = true;
        m_mediaDuration = now;
        m_player->durationChanged();
    }

    m_isEndReached = true;
    timeChanged();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_seekTime = 0;
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

    if (!m_player->mediaPlayerClient()->mediaPlayerIsLooping()) {
        m_paused = true;
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
        gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
        if (m_audioSessionManager)
            m_audioSessionManager->setSoundState(ASM_STATE_PAUSE);
#else
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
        m_downloadFinished = false;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        if (player()->mediaPlayerClient()->isVideoElement())
            power_unlock_state(POWER_STATE_NORMAL);
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
    }
}

void MediaPlayerPrivateGStreamer::cacheDuration()
{
#if ENABLE(TIZEN_MSE)
    if (m_mediaSource) {
        m_mediaDuration = m_mediaSource->duration();
        return;
    }
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // It is meaningless to query duration for live stream.
    if (isLiveStream() || isLocalMediaStream())
        return;

    // It should retry to cache duration when called from durationChanged() or state chagned to PAUSED
    // even it was failed last time.
    if (!m_mediaDurationKnown)
        m_mediaDurationKnown = true;
#else
    if (m_mediaDuration || !m_mediaDurationKnown)
        return;
#endif

    float newDuration = duration();
    if (std::isinf(newDuration)) {
        // Only pretend that duration is not available if the the query failed in a stable pipeline state.
        GstState state;
        if (gst_element_get_state(m_playBin.get(), &state, 0, 0) == GST_STATE_CHANGE_SUCCESS && state > GST_STATE_READY)
            m_mediaDurationKnown = false;
        return;
    }

    m_mediaDuration = newDuration;
}

void MediaPlayerPrivateGStreamer::durationChanged()
{
    float previousDuration = m_mediaDuration;

    cacheDuration();
    // Avoid emiting durationchanged in the case where the previous
    // duration was 0 because that case is already handled by the
    // HTMLMediaElement.
    if (previousDuration && m_mediaDuration != previousDuration)
        m_player->durationChanged();
}

void MediaPlayerPrivateGStreamer::loadingFailed(MediaPlayer::NetworkState error)
{
    m_errorOccured = true;
    if (m_networkState != error) {
        m_networkState = error;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
}

static HashSet<String> mimeTypeCache()
{
    initializeGStreamerAndRegisterWebKitElements();

    DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (typeListInitialized)
        return cache;

    const char* mimeTypes[] = {
#if ENABLE(TIZEN_WEARABLE_PROFILE)
        "audio/aac",
        "audio/flac",
        "audio/mp3",
        "audio/mp4a-latm",
        "audio/ogg",
        "audio/x-ms-wma",
        "audio/x-amr-nb-sh",
        "audio/mpeg",
        "video/3gpp",
        "video/mp4",
#else // ENABLE(TIZEN_WEARABLE_PROFILE)
        "application/ogg",
        "application/vnd.apple.mpegurl",
        "application/vnd.rn-realmedia",
        "application/x-3gp",
        "application/x-pn-realaudio",
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        "application/x-mpegurl",
        "application/x-mpegURL",
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
        "audio/3gpp",
        "audio/aac",
        "audio/flac",
        "audio/iLBC-sh",
        "audio/midi",
        "audio/mobile-xmf",
        "audio/mp1",
        "audio/mp2",
        "audio/mp3",
        "audio/mp4",
        "audio/mpeg",
        "audio/ogg",
        "audio/opus",
        "audio/qcelp",
        "audio/riff-midi",
        "audio/speex",
        "audio/wav",
        "audio/webm",
        "audio/x-ac3",
        "audio/x-aiff",
        "audio/x-amr-wb-sh",
        "audio/x-au",
        "audio/x-ay",
        "audio/x-celt",
        "audio/x-dts",
        "audio/x-flac",
        "audio/x-gbs",
        "audio/x-gsm",
        "audio/x-gym",
        "audio/x-imelody",
        "audio/x-ircam",
        "audio/x-kss",
        "audio/x-m4a",
        "audio/x-mod",
        "audio/x-mp3",
        "audio/x-mpeg",
        "audio/x-musepack",
        "audio/x-nist",
        "audio/x-nsf",
        "audio/x-paris",
        "audio/x-sap",
        "audio/x-sbc",
        "audio/x-sds",
        "audio/x-shorten",
        "audio/x-sid",
        "audio/x-spc",
        "audio/x-speex",
        "audio/x-svx",
        "audio/x-ttafile",
        "audio/x-vgm",
        "audio/x-voc",
        "audio/x-vorbis+ogg",
        "audio/x-w64",
        "audio/x-wav",
        "audio/x-wavpack",
        "audio/x-wavpack-correction",
        "video/3gpp",
        "video/mj2",
        "video/mp4",
        "video/mpeg",
        "video/mpegts",
        "video/ogg",
        "video/quicktime",
        "video/vivo",
#if ENABLE(TIZEN_TV_PROFILE)
        "video/webm",
#endif
        "video/x-cdxa",
        "video/x-dirac",
        "video/x-dv",
        "video/x-fli",
        "video/x-flv",
        "video/x-h263",
        "video/x-ivf",
        "video/x-m4v",
        "video/x-matroska",
        "video/x-mng",
        "video/x-ms-asf",
        "video/x-msvideo",
        "video/x-mve",
        "video/x-nuv",
        "video/x-vcd"
#endif // ENABLE(TIZEN_WEARABLE_PROFILE)
    };

    for (unsigned i = 0; i < (sizeof(mimeTypes) / sizeof(*mimeTypes)); ++i)
        cache.add(String(mimeTypes[i]));

    typeListInitialized = true;
    return cache;
}

void MediaPlayerPrivateGStreamer::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamer::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.type.isNull() || parameters.type.isEmpty())
        return MediaPlayer::IsNotSupported;

#if ENABLE(ENCRYPTED_MEDIA)
    if (!parameters.keySystem.isNull())
        return CDM::keySystemSupportsMimeType(parameters.keySystem, parameters.type) ? MediaPlayer::IsSupported : MediaPlayer::IsNotSupported;
#endif

#if ENABLE(TIZEN_MSE)
    if (parameters.isMediaSource && !(parameters.type == "video/mp4" || parameters.type == "audio/mp4" || parameters.type == "video/webm" || parameters.type == "audio/webm"))
        return MediaPlayer::IsNotSupported;
#endif // ENABLE(TIZEN_MSE)

    // spec says we should not return "probably" if the codecs string is empty
    if (mimeTypeCache().contains(parameters.type))
        return parameters.codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;
    return MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivateGStreamer::setDownloadBuffering()
{
    if (!m_playBin)
        return;

    GstPlayFlags flags;
    g_object_get(m_playBin.get(), "flags", &flags, NULL);
    // We don't want to stop downloading if we already started it.
    if (flags & GST_PLAY_FLAG_DOWNLOAD && m_readyState > MediaPlayer::HaveNothing && !m_resetPipeline)
        return;

    bool shouldDownload = !isLiveStream() && m_preload == MediaPlayer::Auto;
    if (shouldDownload) {
        LOG_MEDIA_MESSAGE("Enabling on-disk buffering");
        g_object_set(m_playBin.get(), "flags", flags | GST_PLAY_FLAG_DOWNLOAD, NULL);
        m_fillTimer.startRepeating(0.2);
    } else {
        LOG_MEDIA_MESSAGE("Disabling on-disk buffering");
        g_object_set(m_playBin.get(), "flags", flags & ~GST_PLAY_FLAG_DOWNLOAD, NULL);
        m_fillTimer.stop();
    }
}

void MediaPlayerPrivateGStreamer::setPreload(MediaPlayer::Preload preload)
{
    if (preload == MediaPlayer::Auto && isLiveStream())
        return;

    m_preload = preload;
    setDownloadBuffering();

    if (m_delayingLoad && m_preload != MediaPlayer::None) {
        m_delayingLoad = false;
        commitLoad();
    }
}

void MediaPlayerPrivateGStreamer::createAudioSink()
{
    // Construct audio sink if pitch preserving is enabled.
    if (!m_preservesPitch)
        return;

    if (!m_playBin)
        return;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)

#if ENABLE(TIZEN_WEARABLE_PROFILE)
#if ENABLE(TIZEN_EMULATOR)
    GstElement* audioSink = gst_element_factory_make("avsysaudiosink", 0);
    g_object_set(audioSink, "close-handle-on-prepare", 1, NULL);
    g_object_set(audioSink, "provide-clock", !player()->mediaPlayerClient()->isVideoElement(), NULL);
#else
    GstElement* audioSink = gst_bin_new("audio-sink");
    GstElement* sink = gst_element_factory_make("avsysaudiosink", 0);
    GstElement* filter = gst_element_factory_make("soundalive", 0);

    g_object_set(sink, "close-handle-on-prepare", 1, NULL);
    g_object_set(sink, "provide-clock", !player()->mediaPlayerClient()->isVideoElement(), NULL);
    g_object_set(filter, "filter-action", 1, NULL);

    gst_bin_add_many(GST_BIN(audioSink), filter, sink, NULL);

    if (!gst_element_link_many(filter, sink, NULL)) {
        GST_WARNING("Failed to link audio sink elements");
        gst_object_unref(audioSink);
        return;
    }
    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(filter, "sink"));
    gst_element_add_pad(audioSink, gst_ghost_pad_new("sink", pad.get()));
#endif
#else
    GstElement* audioSink = gst_element_factory_make("pulsesink", 0);
    g_object_set(audioSink, "provide-clock", true, NULL);
#endif

    if (m_audioSessionManager)
        m_audioSessionManager->registerAudioSessionManager(mediaPlayerPrivateAudioSessionNotifyCallback, this);

#else // ENABLE(TIZEN_GSTREAMER_VIDEO)

    GstElement* scale = gst_element_factory_make("scaletempo", 0);
    if (!scale) {
        GST_WARNING("Failed to create scaletempo");
        return;
    }

    GstElement* convert = gst_element_factory_make("audioconvert", 0);
    GstElement* resample = gst_element_factory_make("audioresample", 0);
    GstElement* sink = gst_element_factory_make("autoaudiosink", 0);

    m_autoAudioSink = sink;

    g_signal_connect(sink, "child-added", G_CALLBACK(setAudioStreamPropertiesCallback), this);

    GstElement* audioSink = gst_bin_new("audio-sink");
    gst_bin_add_many(GST_BIN(audioSink), scale, convert, resample, sink, NULL);

    if (!gst_element_link_many(scale, convert, resample, sink, NULL)) {
        GST_WARNING("Failed to link audio sink elements");
        gst_object_unref(audioSink);
        return;
    }

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(scale, "sink"));
    gst_element_add_pad(audioSink, gst_ghost_pad_new("sink", pad.get()));
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

    g_object_set(m_playBin.get(), "audio-sink", audioSink, NULL);
}

void MediaPlayerPrivateGStreamer::createGSTPlayBin()
{
    ASSERT(!m_playBin);

    // gst_element_factory_make() returns a floating reference so
    // we should not adopt.
    m_playBin = gst_element_factory_make(gPlaybinName, "play");
    setStreamVolumeElement(GST_STREAM_VOLUME(m_playBin.get()));

    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_playBin.get()));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(mediaPlayerPrivateMessageCallback), this);

#if USE(TIZEN_PLATFORM_SURFACE)
    gst_bus_set_sync_handler(bus.get(), GstBusSyncHandler(mediaPlayerPrivateSyncHandler), this);

    GstPlayFlags flags;
    g_object_get(m_playBin.get(), "flags", &flags, NULL);

    int updateFlags = 0;
#if USE(TIZEN_PLATFORM_SURFACE)
    // Omits configuration of ffmpegcolorspace and videoscale.
    updateFlags |= GST_PLAY_FLAG_NATIVE_VIDEO;
#endif
#if ENABLE(TIZEN_TV_PROFILE)
    // Enable on-disk buffering to support pull mode playback.
    updateFlags |= GST_PLAY_FLAG_DOWNLOAD;
#endif

    g_object_set(m_playBin.get(), "flags", (flags | updateFlags), NULL);
#endif

    g_object_set(m_playBin.get(), "mute", m_player->muted(), NULL);

    g_signal_connect(m_playBin.get(), "notify::source", G_CALLBACK(mediaPlayerPrivateSourceChangedCallback), this);
    g_signal_connect(m_playBin.get(), "video-changed", G_CALLBACK(mediaPlayerPrivateVideoChangedCallback), this);
    g_signal_connect(m_playBin.get(), "audio-changed", G_CALLBACK(mediaPlayerPrivateAudioChangedCallback), this);
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    g_signal_connect(m_playBin.get(), "element-added", G_CALLBACK(mediaPlayerPrivateElementAddedCallback), this);
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    g_signal_connect(m_playBin.get(), "source-setup", G_CALLBACK(mediaPlayerPrivateSourceSetupCallback), this);
    g_signal_connect(m_playBin.get(), "element-added", G_CALLBACK(mediaPlayerElementAddedCallback),
this);
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
    if (webkitGstCheckVersion(1, 1, 2)) {
        g_signal_connect(m_playBin.get(), "text-changed", G_CALLBACK(mediaPlayerPrivateTextChangedCallback), this);

        GstElement* textCombiner = webkitTextCombinerNew();
        ASSERT(textCombiner);
        g_object_set(m_playBin.get(), "text-stream-combiner", textCombiner, NULL);

        m_textAppSink = webkitTextSinkNew();
        ASSERT(m_textAppSink);

        m_textAppSinkPad = adoptGRef(gst_element_get_static_pad(m_textAppSink.get(), "sink"));
        ASSERT(m_textAppSinkPad);

        g_object_set(m_textAppSink.get(), "emit-signals", true, "enable-last-sample", false, "caps", gst_caps_new_empty_simple("text/vtt"), NULL);
        g_signal_connect(m_textAppSink.get(), "new-sample", G_CALLBACK(mediaPlayerPrivateNewTextSampleCallback), this);

        g_object_set(m_playBin.get(), "text-sink", m_textAppSink.get(), NULL);
    }
#endif

    GstElement* videoElement = createVideoSink(m_playBin.get());

    g_object_set(m_playBin.get(), "video-sink", videoElement, NULL);
#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
    if (shouldUseWebkitWebSource())
        g_object_set(m_playBin.get(), "buffer-size", 100 * 1024, NULL);
#endif

    GRefPtr<GstPad> videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink.get(), "sink"));
    if (videoSinkPad)
        g_signal_connect(videoSinkPad.get(), "notify::caps", G_CALLBACK(mediaPlayerPrivateVideoSinkCapsChangedCallback), this);

    createAudioSink();
}

#if USE(TIZEN_PLATFORM_SURFACE)
void MediaPlayerPrivateGStreamer::xWindowIdPrepared(GstMessage* message)
{
    // It is called in streaming thread.
    // So it should be used only to set window handle to video sink.
    // And it is called just once after video src is set.
    // So video resolution change should be handled in notifyPlayerOfVideo().

    int width;
    int height;
    gst_structure_get_int(message->structure, "video-width", &width);
    gst_structure_get_int(message->structure, "video-height", &height);

#if ENABLE(TIZEN_MEDIA_STREAM)
    // FIXME : rebase
/*    int orientation = 0;
    if (isLocalMediaStream() && getFrameOrientation(&orientation)
     && (orientation == 0 || orientation == 180))
        m_videoSize = IntSize(height, width);
    else */
#endif
    m_videoSize = IntSize(width, height);

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (m_overlayType == MediaPlayer::HwOverlay) {
        setXWindowHandle();
    } else
#endif
    setPixmap();
}
#endif // USE(TIZEN_PLATFORM_SURFACE)

#if ENABLE(TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING)
bool MediaPlayerPrivateGStreamer::shouldUseWebkitWebSource()
{
    if (!player()->mediaPlayerClient()->isVideoElement() && player()->url().protocol().startsWith("http") && m_preload == MediaPlayer::MetaData)
        return true;
    return false;
}
#endif

void MediaPlayerPrivateGStreamer::simulateAudioInterruption()
{
    GstMessage* message = gst_message_new_request_state(GST_OBJECT(m_playBin.get()), GST_STATE_PAUSED);
    gst_element_post_message(m_playBin.get(), message);
}

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void MediaPlayerPrivateGStreamer::setProxy(GstElement* source)
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    if (!session)
        return;

    SoupURI* proxyUri = 0;
    g_object_get(session, SOUP_SESSION_PROXY_URI, &proxyUri, NULL);
    if (!proxyUri)
        return;

    char* proxy = soup_uri_to_string(proxyUri, false);
    g_object_set(source, "proxy", proxy, NULL);

    soup_uri_free(proxyUri);
    g_free(proxy);
}
#endif // ENABLE(TIZEN_WEBKIT2_PROXY)

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaPlayerPrivateGStreamer::suspend()
{
    if (m_player->mediaPlayerClient()->suspended())
        return;

    m_suspendTime = currentTime();
    TIZEN_LOGI("suspendTime : %f", m_suspendTime);

    // FIXME : In case localMediaStream() is true, crash occurs when pipeline is going to NULL state.
    if (isLocalMediaStream())
        gst_element_set_state(m_playBin.get(), GST_STATE_READY);
    else
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager) {
        m_audioSessionManager->setSoundState(ASM_STATE_STOP);
#if ENABLE(TIZEN_TV_ASM_READY)
        if (!m_audioSessionManager->deallocateResources())
            LOG_MEDIA_MESSAGE("Failed to deallocate resources.");
#endif
    }
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
}

void MediaPlayerPrivateGStreamer::resume()
{
    if (!m_player->mediaPlayerClient()->suspended())
        return;

    if (isLocalMediaStream()) {
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
#if ENABLE(TIZEN_TV_ASM_READY)
        if (m_audioSessionManager) {
            if (!m_audioSessionManager->allocateResources(m_playBin.get())) {
                LOG_MEDIA_MESSAGE("Failed to allocate resources.");
                return;
            }
            m_audioSessionManager->setSoundState(ASM_STATE_PLAYING);
        }
#else
        if (m_audioSessionManager)
            m_audioSessionManager->setSoundState(ASM_STATE_PLAYING);
#endif
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
        gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
    } else {
        gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
#if ENABLE(TIZEN_TV_ASM_READY)
        if (m_audioSessionManager) {
            m_audioSessionManager->setSoundState(ASM_STATE_PAUSE);
            if (!m_audioSessionManager->deallocateResources())
                LOG_MEDIA_MESSAGE("Failed to deallocate resources.");
        }
#else
        if (m_audioSessionManager)
            m_audioSessionManager->setSoundState(ASM_STATE_PAUSE);
#endif
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
    }
}

bool MediaPlayerPrivateGStreamer::isLocalMediaStream() const
{
    if (m_url.string().contains("camera://"))
        return true;
    else
        return false;
}
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_MSE)
// Returns the size of the video
IntSize MediaPlayerPrivateGStreamer::naturalSize() const
{
    if (!hasVideo() && m_mediaSource)
        return m_mediaSource->getSize();

    return MediaPlayerPrivateGStreamerBase::naturalSize();
}

MediaPlayer::ReadyState MediaPlayerPrivateGStreamer::mediasourceReadyState() const
{
    if (m_mediaSource)
        return m_mediaSource->mediasourceReadyState();

    return m_readyState;
}
#endif //  ENABLE(TIZEN_MSE)

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
void MediaPlayerPrivateGStreamer::changeOverlayType(MediaPlayer::OverlayType type)
{
    TIZEN_LOGI("Overlay Type: %d", type);

    if (m_overlayType == type)
        return;
    m_overlayType = type;

    if (type == MediaPlayer::HwOverlay)
        setXWindowHandle();
    else {
#if USE(TIZEN_PLATFORM_SURFACE)
        setPixmap();
#endif
    }
}

void MediaPlayerPrivateGStreamer::rotateHwOverlayVideo()
{
    setRotateForHwOverlayVideo();
}

void MediaPlayerPrivateGStreamer::setXWindowHandle()
{
    TIZEN_LOGI("");

    if (!m_webkitVideoSink.get() || m_overlayType != MediaPlayer::HwOverlay)
        return;

    Ecore_X_Window xWindow = 0;
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    xWindow = getXWindow();
#endif
    if (!xWindow) {
        TIZEN_LOGE("Fail to get XWindow handle");
        return;
    }

    g_object_set(m_webkitVideoSink.get(), "is-pixmap", FALSE, NULL);
    gst_x_overlay_set_window_handle(GST_X_OVERLAY(m_webkitVideoSink.get()), xWindow);
#if ENABLE(TIZEN_TV_PROFILE)
    m_videoLayer->resetOverlay();
    TIZEN_LOGI("gst_x_overlay_set_window_handle");
#endif
    setRotateForHwOverlayVideo();
}

void MediaPlayerPrivateGStreamer::setRotateForHwOverlayVideo()
{
    // We don't have rotate scenario in macro device yet.
    notImplemented();
}
#endif // ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)

#if USE(TIZEN_PLATFORM_SURFACE)
void MediaPlayerPrivateGStreamer::setPixmap()
{
    if (!m_videoLayer->graphicsSurfaceToken().isValid())
        m_videoLayer->setOverlay(m_videoSize);
    else {
        // FIXME : If video size is changed then call setOverlay(m_videoSize) with new size.
        m_videoLayer->setOverlay();
    }
}
#endif

}
#endif // USE(GSTREAMER)
