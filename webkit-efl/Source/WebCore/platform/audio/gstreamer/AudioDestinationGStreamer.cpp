/*
 *  Copyright (C) 2011, 2012 Igalia S.L
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

#if ENABLE(WEB_AUDIO)

#include "AudioDestinationGStreamer.h"

#include "AudioChannel.h"
#include "AudioSourceProvider.h"
#include <wtf/gobject/GOwnPtr.h>
#include "GRefPtrGStreamer.h"
#include "GStreamerVersioning.h"
#include "Logging.h"
#include "WebKitWebAudioSourceGStreamer.h"
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

namespace WebCore {

// Size of the AudioBus for playback. The webkitwebaudiosrc element
// needs to handle this number of frames per cycle as well.
const unsigned framesToPull = 128;

gboolean messageCallback(GstBus*, GstMessage* message, AudioDestinationGStreamer* destination)
{
    return destination->handleMessage(message);
}

PassOwnPtr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, const String&, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
{
    // FIXME: make use of inputDeviceId as appropriate.

    // FIXME: Add support for local/live audio input.
    if (numberOfInputChannels)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled input channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    // FIXME: Add support for multi-channel (> stereo) output.
    if (numberOfOutputChannels != 2)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled output channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    return adoptPtr(new AudioDestinationGStreamer(callback, sampleRate));
}

float AudioDestination::hardwareSampleRate()
{
    return 44100;
}

unsigned long AudioDestination::maxChannelCount()
{
    // FIXME: query the default audio hardware device to return the actual number
    // of channels of the device. Also see corresponding FIXME in create().
    return 0;
}

#ifndef GST_API_VERSION_1
static void onGStreamerWavparsePadAddedCallback(GstElement*, GstPad* pad, AudioDestinationGStreamer* destination)
{
    destination->finishBuildingPipelineAfterWavParserPadReady(pad);
}
#endif

#if ENABLE(TIZEN_GSTREAMER_AUDIO)
static ASM_cb_result_t AudioDestinationAudioSessionEventSourcePause(ASM_event_sources_t eventSource, void* callbackData)
{
    AudioDestinationGStreamer* pDestination = static_cast<AudioDestinationGStreamer*>(callbackData);
    if (!pDestination)
        return ASM_CB_RES_IGNORE;

    switch (eventSource) {
    case ASM_EVENT_SOURCE_ALARM_START:
    case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
        pDestination->stop();
        return ASM_CB_RES_PAUSE;
    default:
        return ASM_CB_RES_NONE;
    }
}

static ASM_cb_result_t AudioDestinationAudioSessionEventSourcePlay(ASM_event_sources_t eventSource, void* callbackData)
{
    AudioDestinationGStreamer* pDestination = static_cast<AudioDestinationGStreamer*>(callbackData);
    if (!pDestination)
        return ASM_CB_RES_IGNORE;

    switch (eventSource) {
    case ASM_EVENT_SOURCE_ALARM_END:
    case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
        pDestination->start();
        return ASM_CB_RES_PLAYING;
    default:
        return ASM_CB_RES_NONE;
    }
}

static ASM_cb_result_t AudioDestinationAudioSessionNotifyCallback(int, ASM_event_sources_t eventSource, ASM_sound_commands_t command, unsigned int, void* callbackData)
{
    if (command == ASM_COMMAND_STOP || command == ASM_COMMAND_PAUSE)
        return AudioDestinationAudioSessionEventSourcePause(eventSource, callbackData);
    if (command == ASM_COMMAND_PLAY || command == ASM_COMMAND_RESUME)
        return AudioDestinationAudioSessionEventSourcePlay(eventSource, callbackData);

    return ASM_CB_RES_NONE;
}
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

AudioDestinationGStreamer::AudioDestinationGStreamer(AudioIOCallback& callback, float sampleRate)
    : m_callback(callback)
    , m_renderBus(AudioBus::create(2, framesToPull, false))
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    , m_audioSessionManager(AudioSessionManagerGStreamerTizen::createAudioSessionManager())
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
{
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager)
        m_audioSessionManager->registerAudioSessionManager(AudioDestinationAudioSessionNotifyCallback, this);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
    m_pipeline = gst_pipeline_new("play");
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    ASSERT(bus);
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(messageCallback), this);

    GstElement* webkitAudioSrc = reinterpret_cast<GstElement*>(g_object_new(WEBKIT_TYPE_WEB_AUDIO_SRC,
                                                                            "rate", sampleRate,
                                                                            "bus", m_renderBus.get(),
                                                                            "provider", &m_callback,
                                                                            "frames", framesToPull, NULL));

    GstElement* wavParser = gst_element_factory_make("wavparse", 0);

    m_wavParserAvailable = wavParser;
    ASSERT_WITH_MESSAGE(m_wavParserAvailable, "Failed to create GStreamer wavparse element");
    if (!m_wavParserAvailable)
        return;

#ifndef GST_API_VERSION_1
    g_signal_connect(wavParser, "pad-added", G_CALLBACK(onGStreamerWavparsePadAddedCallback), this);
#endif
    gst_bin_add_many(GST_BIN(m_pipeline), webkitAudioSrc, wavParser, NULL);
    gst_element_link_pads_full(webkitAudioSrc, "src", wavParser, "sink", GST_PAD_LINK_CHECK_NOTHING);

#ifdef GST_API_VERSION_1
    GRefPtr<GstPad> srcPad = adoptGRef(gst_element_get_static_pad(wavParser, "src"));
    finishBuildingPipelineAfterWavParserPadReady(srcPad.get());
#endif
}

AudioDestinationGStreamer::~AudioDestinationGStreamer()
{
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    ASSERT(bus);
    g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(messageCallback), this);
    gst_bus_remove_signal_watch(bus.get());

    gst_element_set_state(m_pipeline, GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager)
        m_audioSessionManager->setSoundState(ASM_STATE_STOP);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
    gst_object_unref(m_pipeline);
}

void AudioDestinationGStreamer::finishBuildingPipelineAfterWavParserPadReady(GstPad* pad)
{
    ASSERT(m_wavParserAvailable);

    GRefPtr<GstElement> audioSink = gst_element_factory_make("autoaudiosink", 0);
    m_audioSinkAvailable = audioSink;

    if (!audioSink) {
        LOG_ERROR("Failed to create GStreamer autoaudiosink element");
        return;
    }

    // Autoaudiosink does the real sink detection in the GST_STATE_NULL->READY transition
    // so it's best to roll it to READY as soon as possible to ensure the underlying platform
    // audiosink was loaded correctly.
    GstStateChangeReturn stateChangeReturn = gst_element_set_state(audioSink.get(), GST_STATE_READY);
    if (stateChangeReturn == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR("Failed to change autoaudiosink element state");
        gst_element_set_state(audioSink.get(), GST_STATE_NULL);
        m_audioSinkAvailable = false;
        return;
    }

#if ENABLE(TIZEN_WEB_AUDIO)
    GValue value = {0, {{0}}};
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, true);
    gst_child_proxy_set_property(GST_OBJECT(audioSink.get()), "close-handle-on-prepare", &value);
#endif // ENABLE(TIZEN_WEB_AUDIO)

    GstElement* audioConvert = gst_element_factory_make("audioconvert", 0);
    gst_bin_add_many(GST_BIN(m_pipeline), audioConvert, audioSink.get(), NULL);

    // Link wavparse's src pad to audioconvert sink pad.
    GRefPtr<GstPad> sinkPad = adoptGRef(gst_element_get_static_pad(audioConvert, "sink"));
    gst_pad_link_full(pad, sinkPad.get(), GST_PAD_LINK_CHECK_NOTHING);

    // Link audioconvert to audiosink and roll states.
    gst_element_link_pads_full(audioConvert, "src", audioSink.get(), "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_sync_state_with_parent(audioConvert);
    gst_element_sync_state_with_parent(audioSink.leakRef());
}

gboolean AudioDestinationGStreamer::handleMessage(GstMessage* message)
{
    GOwnPtr<GError> error;
    GOwnPtr<gchar> debug;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(message, &error.outPtr(), &debug.outPtr());
        g_warning("Warning: %d, %s. Debug output: %s", error->code,  error->message, debug.get());
        break;
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &error.outPtr(), &debug.outPtr());
        g_warning("Error: %d, %s. Debug output: %s", error->code,  error->message, debug.get());
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
        if (m_audioSessionManager)
            m_audioSessionManager->setSoundState(ASM_STATE_STOP);
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
        m_isPlaying = false;
        break;
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    case GST_MESSAGE_EOS:
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        if (m_audioSessionManager)
            m_audioSessionManager->setSoundState(ASM_STATE_STOP);
        break;
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)
    default:
        break;
    }
    return TRUE;
}

void AudioDestinationGStreamer::start()
{
    ASSERT(m_wavParserAvailable);
    if (!m_wavParserAvailable)
        return;
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager && !m_audioSessionManager->setSoundState(ASM_STATE_PLAYING))
        return;
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    m_isPlaying = true;
}

void AudioDestinationGStreamer::stop()
{
    ASSERT(m_wavParserAvailable && m_audioSinkAvailable);
    if (!m_wavParserAvailable || !m_audioSinkAvailable)
        return;
#if ENABLE(TIZEN_GSTREAMER_AUDIO)
    if (m_audioSessionManager && !m_audioSessionManager->setSoundState(ASM_STATE_PAUSE))
       return;
#endif // ENABLE(TIZEN_GSTREAMER_AUDIO)

    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    m_isPlaying = false;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
