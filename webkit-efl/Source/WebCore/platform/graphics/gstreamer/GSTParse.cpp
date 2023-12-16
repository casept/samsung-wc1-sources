
#include "config.h"
#include "GSTParse.h"

#if ENABLE(TIZEN_MSE)
#include "GStreamerVersioning.h"
#include "SourceBufferPrivateGStreamer.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include "GStreamerUtilities.h"

using namespace std;
// FIXME :using namespace WebCore;
// namespace WebCore {

static const unsigned int drmTypeClearkey = 0x61;
static const unsigned int drmTypePlayready = 0x01;
typedef struct _LicenseData {
    unsigned char* key;
    int keyLength;
}LicenseData;
#define SIMPLE_KEY_PLAYREADY    0x9A04F079 // playready_system_id = "9A04F079-9840-4286-AB92-E65BE0885F95"
#define BOX_TYPE_PSSH   0x70737368 // pssh box type

#if ENABLE_EME
static void gstParseNeedKeyCallback(GstElement *element, gpointer initData, guint initDataLength, guint numInitData, gpointer userData)
{
    LOG_MEDIA_MESSAGE("gstParseNeedKeyCallback from %s \n", GST_ELEMENT_NAME(element));
    LOG_MEDIA_MESSAGE("the number of pssh box = %d\n", numInitData);
    GSTParse* gstParse = reinterpret_cast<GSTParse*>(userData);
    gstParse->needKey(element,initData, initDataLength, numInitData, userData);
}

static gboolean gstParserNeedKeyTimeoutCallback(GSTParse* parser)
{
    parser->appendNeedKeySourceBuffer();
    return FALSE;
}
#endif

#if !USE_EOS
static gboolean gstParserBufferAddedTimeoutCallback(GSTParse* parser)
{
    parser->notifyBufferAdded();
    return FALSE;
}
#endif

static gboolean gstParserParseCompleteCb(void* userData)
{
    GSTParse* parser = reinterpret_cast<GSTParse*>(userData);
    parser->parseComplete();
    return FALSE;
}

GSTParse::GSTParse()
{
    m_Bus_watch_id = 0;
    m_Pipeline = NULL;
    m_Source = NULL;
    m_Demuxer = NULL;
    m_Audio_sink = NULL;
    m_Video_sink = NULL;
    m_videoDrmBridge = NULL;
    m_audioDrmBridge = NULL;
    m_Audio_frames = NULL;
    m_Video_frames = NULL;
    m_Isinitseg = FALSE;
    m_Haveinitseg = FALSE;
    m_isLinkedAudioSink = FALSE;
    m_isLinkedVideoSink = FALSE;
    m_Bufoffset = 0;
    m_parseState = 0;
    m_initSegmentLen = 0;
    m_mediaSegmentLen = 0;
    m_moovBoxPosition = 0;
    m_have_videotrack = FALSE;
    m_have_audiotrack = FALSE;
    m_have_videotrack_id = 0;
    m_have_audiotrack_id = 0;
    m_next_track_ID = 0;
    m_track_count = 0;
    m_Width = 0;
    m_Height = 0;
    m_video_timescale = 0;
    m_audio_timescale = 0;
    m_video_duration = 0;
    m_audio_duration = 0;
    m_pCBinitsegment = NULL;
    m_CBinitsegmentData = NULL;
    m_Mimetype = NULL;
    m_bufferAddedTimerHandler = 0;
    m_parseCompleteTimerHandler = 0;
#if ENABLE_EME
    m_needKeyTimerHandler = 0;
#endif
    m_padAddedHandler = 0;
    m_videoNeedkeyHandler = 0;
    m_audioNeedkeyHandler = 0;
    m_videoHandOffHandler = 0;
    m_audioHandOffHandler = 0;
#if UNDERRUN
    m_underrunHandler = 0;
#endif
    m_sourceBuffer = 0;
    m_Mutex = g_mutex_new();
#if USE_EOS
    m_parseCompleteCB = NULL;
    m_parseCompleteData = NULL;
    m_Lock = g_mutex_new();
    m_Cond = g_cond_new ();
    m_ev_thread = NULL;
    m_ev_Lock = g_mutex_new();
    m_ev_Cond = g_cond_new ();
    m_ev_thread_exit = FALSE;
    m_is_eos = FALSE;
    m_parseEndType= PARSE_END_TYPE_NONE;
    m_sourceEventProbeHandler = 0;
    m_videoSinkEventProbeHandler = 0;
    m_audioSinkEventProbeHandler = 0;
#endif
    m_eMimetype = MIME_TYPE_NONE;
    m_have_videotrack_id_webm = 0;
    m_have_audiotrack_id_webm = 0;
    m_have_track_type_webm = 0;
    m_have_track_number_webm = 0;
    m_timescale_webm = 0;
    m_duration_webm = 0.0;
}

GSTParse::GSTParse(const gchar* mimetype, SourceBufferPrivateGStreamer* sourceBuffer)
{
    m_Bus_watch_id = 0;
    m_Pipeline = NULL;
    m_Source = NULL;
    m_Demuxer = NULL;
    m_Audio_sink = NULL;
    m_Video_sink = NULL;
    m_videoDrmBridge = NULL;
    m_audioDrmBridge = NULL;
    m_Audio_frames = NULL;
    m_Video_frames = NULL;
    m_Isinitseg = FALSE;
    m_Haveinitseg = FALSE;
    m_isLinkedAudioSink = FALSE;
    m_isLinkedVideoSink = FALSE;
    m_Bufoffset = 0;
    m_parseState = 0;
    m_initSegmentLen = 0;
    m_mediaSegmentLen = 0;
    m_moovBoxPosition = 0;
    m_have_videotrack = FALSE;
    m_have_audiotrack = FALSE;
    m_have_videotrack_id = 0;
    m_have_audiotrack_id = 0;
    m_next_track_ID = 0;
    m_track_count = 0;
    m_Width = 0;
    m_Height = 0;
    m_video_timescale = 0;
    m_audio_timescale = 0;
    m_video_duration = 0;
    m_audio_duration = 0;
    m_pCBinitsegment = NULL;
    m_CBinitsegmentData = NULL;
    m_Mimetype = g_strdup(mimetype);
    m_bufferAddedTimerHandler = 0;
    m_parseCompleteTimerHandler = 0;
#if ENABLE_EME
    m_needKeyTimerHandler = 0;
#endif
    m_padAddedHandler = 0;
    m_videoNeedkeyHandler = 0;
    m_audioNeedkeyHandler = 0;
    m_videoHandOffHandler = 0;
    m_audioHandOffHandler = 0;
#if UNDERRUN
    m_underrunHandler = 0;
#endif
    m_sourceBuffer = sourceBuffer;
    m_Mutex = g_mutex_new();
#if USE_EOS
    m_parseCompleteCB = NULL;
    m_parseCompleteData = NULL;
    m_Lock = g_mutex_new();
    m_Cond = g_cond_new ();
    m_ev_thread = NULL;
    m_ev_Lock = g_mutex_new();
    m_ev_Cond = g_cond_new ();
    m_ev_thread_exit = FALSE;
    m_is_eos = FALSE;
    m_parseEndType= PARSE_END_TYPE_NONE;
    m_sourceEventProbeHandler = 0;
    m_videoSinkEventProbeHandler = 0;
    m_audioSinkEventProbeHandler = 0;
#endif
    if(g_strrstr(m_Mimetype,"video/webm"))
        m_eMimetype = MIME_TYPE_WEBM_VIDEO;
    else if(g_strrstr(m_Mimetype,"audio/webm"))
        m_eMimetype = MIME_TYPE_WEBM_AUDIO;
    else if(g_strrstr(m_Mimetype,"video/mp4"))
        m_eMimetype = MIME_TYPE_MP4_VIDEO;
    else if(g_strrstr(m_Mimetype,"audio/mp4"))
        m_eMimetype = MIME_TYPE_MP4_AUDIO;
    else
        m_eMimetype = MIME_TYPE_NONE;
    m_have_videotrack_id_webm = 0;
    m_have_audiotrack_id_webm = 0;
    m_have_track_type_webm = 0;
    m_have_track_number_webm = 0;
    m_timescale_webm = 0;
    m_duration_webm = 0.0;

    setParseCompleteCB(gstParserParseCompleteCb, (void*)this);

    LOG_GSTPARSE_MESSAGE("[GSTParse] input Mimetype is %s[%d][%s]\n", m_Mimetype, m_eMimetype, mimetype);
}

GSTParse::~GSTParse()
{
    LOG_GSTPARSE_MESSAGE ("[GSTParse] [%s] GSTParse release\n", m_Mimetype);

    setParseCompleteCB(NULL, NULL);

    disconnect_signal();

    destroy_pipeline();

    LOG_GSTPARSE_MESSAGE ("[GSTParse] [%s] end GSTParse release\n", m_Mimetype);
}

void
GSTParse::disconnect_signal(void)
{
    LOG_GSTPARSE_MESSAGE ("[GSTParse] [%s] disconnect_signal\n", m_Mimetype);

    if ( g_signal_handler_is_connected ( m_Demuxer, m_padAddedHandler ) )
    {
        g_signal_handler_disconnect ( m_Demuxer, m_padAddedHandler );
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect pad added cb\n", m_eMimetype);
        m_padAddedHandler = 0;
    }
#if UNDERRUN
    if ( g_signal_handler_is_connected ( m_Demuxer, m_underrunHandler ) )
    {
        if (isSetParseCompleteCB()) {
            g_signal_handler_disconnect ( m_Demuxer, m_underrunHandler );
            LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect underrun cb\n", m_eMimetype);
            m_underrunHandler = 0;
        }
    }
#endif
    if ( g_signal_handler_is_connected ( m_videoDrmBridge, m_videoNeedkeyHandler ) )
    {
        g_signal_handler_disconnect ( m_videoDrmBridge, m_videoNeedkeyHandler );
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect video need key cb\n", m_eMimetype);
        m_videoNeedkeyHandler = 0;
    }
    if ( g_signal_handler_is_connected ( m_audioDrmBridge, m_audioNeedkeyHandler ) )
    {
        g_signal_handler_disconnect ( m_audioDrmBridge, m_audioNeedkeyHandler );
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect audio need key cb\n", m_eMimetype);
        m_audioNeedkeyHandler = 0;
    }
    if ( g_signal_handler_is_connected ( m_Video_sink, m_videoHandOffHandler ) )
    {
        g_signal_handler_disconnect ( m_Video_sink, m_videoHandOffHandler );
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect video handoff cb\n", m_eMimetype);
        m_videoHandOffHandler = 0;
    }
    if ( g_signal_handler_is_connected ( m_Audio_sink, m_audioHandOffHandler ) )
    {
        g_signal_handler_disconnect ( m_Audio_sink, m_audioHandOffHandler );
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect audio handoff cb\n", m_eMimetype);
        m_audioHandOffHandler = 0;
    }
    if (m_bufferAddedTimerHandler) {
        g_source_remove(m_bufferAddedTimerHandler);
        m_bufferAddedTimerHandler = 0;
    }
    if (m_parseCompleteTimerHandler) {
        g_source_remove(m_parseCompleteTimerHandler);
        m_parseCompleteTimerHandler = 0;
    }

#if USE_EOS
    if (m_sourceEventProbeHandler) {
        GstPad *srcpad = NULL;
        srcpad = gst_element_get_static_pad (m_Source, "src");
        g_signal_handler_disconnect(srcpad, m_sourceEventProbeHandler);
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect src event probe\n", m_eMimetype);
        gst_object_unref (srcpad);
        m_sourceEventProbeHandler = 0;
    }

    if (m_videoSinkEventProbeHandler) {
        GstPad *sinkpad = NULL;
        sinkpad = gst_element_get_static_pad (m_Video_sink, "sink");
        g_signal_handler_disconnect(sinkpad, m_videoSinkEventProbeHandler);
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect video sink event probe\n", m_eMimetype);
        gst_object_unref (sinkpad);
        m_videoSinkEventProbeHandler = 0;
    }

    if (m_audioSinkEventProbeHandler) {
        GstPad *sinkpad = NULL;
        sinkpad = gst_element_get_static_pad (m_Audio_sink, "sink");
        g_signal_handler_disconnect(sinkpad, m_audioSinkEventProbeHandler);
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] disconnect audio sink event probe\n", m_eMimetype);
        gst_object_unref (sinkpad);
        m_audioSinkEventProbeHandler = 0;
    }
#endif

    if (m_Bus_watch_id) {
        g_source_remove (m_Bus_watch_id);
        m_Bus_watch_id = 0;
    }

#if ENABLE_EME
    if (m_needKeyTimerHandler) {
        g_source_remove(m_needKeyTimerHandler);
        m_needKeyTimerHandler = 0;
    }
#endif

    GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (m_Pipeline));
    gst_bus_set_sync_handler (bus, NULL, NULL);
    gst_object_unref(bus);

}

void
GSTParse::destroy_pipeline(void)
{
    /* TODO : fill it */
    LOG_GSTPARSE_MESSAGE("[GSTParse][%d] enter destroy_pipeline\n", m_eMimetype);

#if USE_EOS
    /* release repeat thread */
    if (m_ev_Cond && m_ev_Lock && m_ev_thread) {
        m_ev_thread_exit = TRUE;
        g_cond_signal(m_ev_Cond);
        g_thread_join(m_ev_thread);
        g_mutex_free(m_ev_Lock);
        m_ev_Lock = NULL;
        g_cond_free(m_ev_Cond);
        m_ev_Cond = NULL;
        LOG_GSTPARSE_MESSAGE("[GSTParse][%d] ev thread released\n", m_eMimetype);
    }
#endif

    gst_element_set_state (m_Pipeline, GST_STATE_NULL);

    if (m_Video_sink)
        gst_object_unref(m_Video_sink);
    if (m_Audio_sink)
        gst_object_unref(m_Audio_sink);
#if ENABLE_EME
    if (m_videoDrmBridge)
        gst_object_unref(m_videoDrmBridge);
    if (m_audioDrmBridge)
        gst_object_unref(m_audioDrmBridge);
#endif
    if (m_Demuxer)
        gst_object_unref(m_Demuxer);
    if (m_Source)
        gst_object_unref(m_Source);
    if (m_Pipeline)
        gst_object_unref(m_Pipeline);
#if USE_EOS
    /* release repeat thread */
    if (m_ev_Cond && m_ev_Lock && m_ev_thread) {
        m_ev_thread_exit = TRUE;
        g_cond_signal(m_ev_Cond);
        g_thread_join(m_ev_thread);
        g_mutex_free(m_ev_Lock);
        g_cond_free(m_ev_Cond);
        LOG_GSTPARSE_MESSAGE("ev thread released\n");
    }
#endif
    if (m_Mutex) {
        g_mutex_free (m_Mutex);
        m_Mutex = NULL;
    }

    if (m_Audio_frames) {
        g_list_free (m_Audio_frames);
        m_Audio_frames = NULL;
    }
    if (m_Video_frames) {
        g_list_free (m_Video_frames);
        m_Video_frames = NULL;
    }
}

void
GSTParse::parseComplete()
{
    if (m_sourceBuffer)
        m_sourceBuffer->parseComplete();
    m_parseCompleteTimerHandler = 0;
    return;
}

void
GSTParse::buffer_add (GstElement* /* element */, GstBuffer* buffer, GstPad* /* pad */, gpointer data)
{
    GSTParse* parse = (GSTParse*) data;
    parse->addBuffer(buffer);
    return;
}

void
GSTParse::addBuffer(GstBuffer* buffer)
{
    GstCaps *buffer_caps;
    gchar *name;
    GstBuffer* new_buffer = gst_buffer_copy(buffer);
    buffer_caps = GST_BUFFER_CAPS(new_buffer);

    if (!buffer_caps) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] can't find caps\n");
        return;
    }

    name = gst_caps_to_string(buffer_caps);

    if (!name) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] can't find caps name\n");
        return;
    }

    if (g_strrstr(name, "audio")) {
        if(!m_Mutex)
            return;
        g_mutex_lock(m_Mutex);
        m_Audio_frames = g_list_append(m_Audio_frames, new_buffer);
        LOG_GSTPARSE_MESSAGE ("[GSTParse] audio buffer timestamp = %" GST_TIME_FORMAT
                            " duration = %" GST_TIME_FORMAT "\n",
                            GST_TIME_ARGS( GST_BUFFER_TIMESTAMP(new_buffer)), GST_TIME_ARGS( GST_BUFFER_DURATION(new_buffer)));
        g_mutex_unlock(m_Mutex);
    } else if(g_strrstr(name, "video")) {
        if(!m_Mutex)
            return;
        g_mutex_lock(m_Mutex);
        m_Video_frames = g_list_append(m_Video_frames, new_buffer);
        g_mutex_unlock(m_Mutex);
        LOG_GSTPARSE_MESSAGE ("[GSTParse] video buffer timestamp = %" GST_TIME_FORMAT
                            " duration = %" GST_TIME_FORMAT "\n",
                            GST_TIME_ARGS( GST_BUFFER_TIMESTAMP(new_buffer)), GST_TIME_ARGS( GST_BUFFER_DURATION(new_buffer)));
    }

    MMPARSE_FREEIF(name);

    // This function is not executed in main thread.
    // So, register timer to notify that buffer is added to main thread.
#if !USE_EOS
    // FIXME: if we use underrun signal, we will process everything after waiting for underrun, so those callbacks will add nothing.
    // and in such case, we want everything in sync, not on timeouts
    if (!m_bufferAddedTimerHandler)
        m_bufferAddedTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(gstParserBufferAddedTimeoutCallback), this);
#endif

    return;
    /* TODO : check memory leak.. */
}

void
GSTParse::notifyBufferAdded()
{
    m_bufferAddedTimerHandler = 0;
    if(m_sourceBuffer)
        m_sourceBuffer->bufferAdded();
}
#if USE_EOS
gpointer
GSTParse::eos_ev_thread(gpointer data)
{
    GSTParse* parse = (GSTParse *)data;
    GstPad* srcpad = NULL;
    GstEvent* event = NULL;

    LOG_GSTPARSE_MESSAGE("[GSTParse] eos_ev_thread start\n");

    while(!parse->m_ev_thread_exit) {

        g_cond_wait(parse->m_ev_Cond, parse->m_ev_Lock);

        if(parse->m_ev_thread_exit) {
            LOG_GSTPARSE_MESSAGE("[GSTParse] exiting ev thread\n");
            break;
        }

        LOG_GSTPARSE_MESSAGE("[GSTParse] eos_ev_thread run\n");

        srcpad = gst_element_get_static_pad(parse->m_Source, "src");

        gst_pad_set_active(srcpad, FALSE);
        gst_pad_set_active(srcpad, TRUE);
        gst_object_unref (srcpad);

        event = gst_event_new_seek(1.0,
                    GST_FORMAT_TIME,
                    (GstSeekFlags)(GST_SEEK_FLAG_FLUSH),
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE,
                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

        parse->m_is_eos = TRUE;

        if(event && parse->m_Video_sink) {
            LOG_GSTPARSE_MESSAGE("[GSTParse].... send flush seek event\n");
            gst_element_send_event(parse->m_Video_sink, event);
        }

        if(event && parse->m_Audio_sink) {
            LOG_GSTPARSE_MESSAGE("[GSTParse].... send flush seek event\n");
            gst_element_send_event(parse->m_Audio_sink, event);
        }
    }

    return NULL;
}

gboolean
GSTParse::src_event_probe_cb(GstPad* /* pad */, GstEvent* info, gpointer data)
{
    GSTParse* parse = (GSTParse*)data;

    switch (GST_EVENT_TYPE (info)) {
        case GST_EVENT_SEEK:
        {
            GstPad *pad = NULL;
            LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] GST_EVENT_SEEK detected at src\n", parse->m_eMimetype);

            if (parse->m_is_eos) {
                parse->m_is_eos = FALSE;

                pad = gst_element_get_static_pad(parse->m_Demuxer, "sink");

                gst_pad_send_event(pad, gst_event_new_flush_start ());
                gst_pad_send_event(pad, gst_event_new_flush_stop ());

                gst_object_unref(pad);

                if (parse->m_parseEndType == PARSE_END_TYPE_EOS) {
                    if (parse->isSetParseCompleteCB()) {
                        LOG_GSTPARSE_MESSAGE ("[GSTPasrse][%d] parse complete callback\n", parse->m_eMimetype);
                        parse->parseCompleteCB();
                    }
                    else {
                        LOG_GSTPARSE_MESSAGE ("[GSTPasrse][%d] send cond signal -> can feed next data\n", parse->m_eMimetype);
                        g_cond_signal(parse->m_Cond);
                    }
                }
            }
        }
        break;
        default:
        break;
    }

    return TRUE;
}

gboolean
GSTParse::sink_event_probe_cb(GstPad* /* pad */, GstEvent* info, gpointer data)
{
    GSTParse* parse = (GSTParse*)data;

    switch (GST_EVENT_TYPE (info)) {
        case GST_EVENT_EOS:
        {
            LOG_GSTPARSE_MESSAGE ("[GSTParse][%d]detected End of stream at sink\n\n", parse->m_eMimetype);
            g_cond_signal(parse->m_ev_Cond);
            return FALSE;
        }
        break;
        default:
        break;
    }

    return TRUE;
}

#endif
void
GSTParse::dynamic_pad (GstElement* element, GstPad* pad, gpointer data)
{
    GSTParse* parse = (GSTParse*)data;
    GstPad* sinkpad = NULL;
    GstCaps* caps = NULL;
    gchar* capsStr = NULL;

    enum DRMBridge_Mode_
    {
        DRMBRIDGE_MODE_LICENSING_AND_DECRYPTION,
        DRMBRIDGE_MODE_LICENSING,
        DRMBRIDGE_MODE_DECRYPTION,
        DRMBRIDGE_MODE_ERROR = 0xffff
    }/* DRMBridge_Mode */;

    /* We can now link this pad with the vorbis-decoder sink pad */
    LOG_GSTPARSE_MESSAGE ("[GSTParse] Dynamic pad created, linking demuxer/sink\n");

    return_if_fail(element && pad);
    return_if_fail(parse && parse->m_Pipeline);

    caps = gst_pad_get_caps(pad);

    if(!caps) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] get caps fail\n");
        return;
    }

    capsStr = gst_caps_to_string(caps);

    if(!capsStr) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] get caps to string faild\n");
        return;
    }

    LOG_GSTPARSE_MESSAGE ("[GSTParse] caps %s\n", capsStr);

    if (g_strrstr(capsStr, "drm")) {
        if (g_strrstr(capsStr, "video") && parse->m_isLinkedVideoSink == FALSE) {
            parse->m_videoDrmBridge = gst_element_factory_make ("DRMBridge", "video-DRMBridge");
            parse->m_Video_sink = gst_element_factory_make ("fakesink", "video-fakesink");

            if(!parse->m_videoDrmBridge && !parse->m_Video_sink) {
                LOG_GSTPARSE_MESSAGE ("[GSTParse] DRMBridge, video sink elment make failed\n");
                gst_caps_unref (caps);
                MMPARSE_FREEIF(capsStr);
                return; /* TO DO. possible memory leak here */
            }

            gst_bin_add_many(GST_BIN (parse->m_Pipeline), parse->m_videoDrmBridge, parse->m_Video_sink, NULL);

            if(!gst_element_link_many(parse->m_Demuxer, parse->m_videoDrmBridge, parse->m_Video_sink, NULL)) {
                LOG_GSTPARSE_MESSAGE ("[GSTParse] element link failed\n");
                gst_caps_unref (caps);
                MMPARSE_FREEIF(capsStr);
                return; /* TO DO. possible memory leak here */
            }

            parse->m_isLinkedVideoSink = TRUE;
            parse->m_videoNeedkeyHandler = g_signal_connect(parse->m_videoDrmBridge, "need-key", G_CALLBACK(gstParseNeedKeyCallback), (gpointer)parse);
            LOG_GSTPARSE_MESSAGE("[GSTParse]need-key signal is connected to %s \n ", GST_ELEMENT_NAME(parse->m_videoDrmBridge));
            g_object_set(parse->m_videoDrmBridge, "drmbridge-mode", DRMBRIDGE_MODE_LICENSING, NULL);
            LOG_GSTPARSE_MESSAGE("[GSTParse] drmbridge-mode property is set to %s , %d \n ", GST_ELEMENT_NAME(parse->m_videoDrmBridge), DRMBRIDGE_MODE_LICENSING);
            sinkpad = gst_element_get_static_pad (parse->m_Video_sink, "sink");
            g_object_set (parse->m_Video_sink, "signal-handoffs", TRUE, NULL);
            LOG_GSTPARSE_MESSAGE ("[GSTParse] Add video handoff signal\n");
            parse->m_videoHandOffHandler = g_signal_connect (parse->m_Video_sink, "handoff", G_CALLBACK(buffer_add), parse);
#if USE_EOS
            parse->m_videoSinkEventProbeHandler = gst_pad_add_event_probe(sinkpad, G_CALLBACK(sink_event_probe_cb), parse);
#endif
        } else if (g_strrstr(capsStr, "audio") && parse->m_isLinkedAudioSink == FALSE) {
            parse->m_audioDrmBridge = gst_element_factory_make ("DRMBridge", "audio-DRMBridge");
            parse->m_Audio_sink = gst_element_factory_make ("fakesink", "Audio-fakesink");

            if(!parse->m_audioDrmBridge && !parse->m_Audio_sink) {
                LOG_GSTPARSE_MESSAGE ("[GSTParse] DRMBridge, video sink elment make failed\n");
                gst_caps_unref (caps);
                MMPARSE_FREEIF(capsStr);
                return; /* TO DO. possible memory leak here */
            }

            gst_bin_add_many(GST_BIN (parse->m_Pipeline), parse->m_audioDrmBridge, parse->m_Audio_sink, NULL);

            if(!gst_element_link_many(parse->m_Demuxer, parse->m_audioDrmBridge, parse->m_Audio_sink, NULL)) {
                LOG_GSTPARSE_MESSAGE ("[GSTParse] element link failed\n");
                gst_caps_unref (caps);
                MMPARSE_FREEIF(capsStr);
                return; /* TO DO. possible memory leak here */
            }
            parse->m_isLinkedAudioSink = TRUE;
            parse->m_audioNeedkeyHandler = g_signal_connect(parse->m_audioDrmBridge, "need-key", G_CALLBACK(gstParseNeedKeyCallback), (gpointer)parse);
            LOG_GSTPARSE_MESSAGE("[GSTParse]need-key signal is connected to %s \n ", GST_ELEMENT_NAME(parse->m_audioDrmBridge));
            g_object_set(parse->m_audioDrmBridge, "drmbridge-mode", DRMBRIDGE_MODE_LICENSING, NULL);
            LOG_GSTPARSE_MESSAGE("[GSTParse] drmbridge-mode property is set to %s , %d \n ", GST_ELEMENT_NAME(parse->m_audioDrmBridge), DRMBRIDGE_MODE_LICENSING);
            sinkpad = gst_element_get_static_pad (parse->m_Audio_sink, "sink");
            g_object_set (parse->m_Audio_sink, "signal-handoffs", TRUE, NULL);
            LOG_GSTPARSE_MESSAGE ("[GSTParse] Add audio handoff signal\n");
            parse->m_audioHandOffHandler = g_signal_connect (parse->m_Audio_sink, "handoff", G_CALLBACK(buffer_add), parse);
#if USE_EOS
            parse->m_audioSinkEventProbeHandler = gst_pad_add_event_probe(sinkpad, G_CALLBACK(sink_event_probe_cb), parse);
#endif
        }
    } else if (g_strrstr(capsStr, "video") && parse->m_isLinkedVideoSink == FALSE ) {
        parse->m_Video_sink = gst_element_factory_make ("fakesink", "video-fakesink");
        if(!parse->m_Video_sink) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse] video sink element make failed\n");
            gst_caps_unref (caps);
            MMPARSE_FREEIF(capsStr);
            return;
        }
        if (!gst_bin_add (GST_BIN (parse->m_Pipeline), parse->m_Video_sink)) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse] video sink element bin add failed\n");
            gst_caps_unref (caps);
            MMPARSE_FREEIF(capsStr);
            return; /* TO DO. possible memory leak here */
        }

        parse->m_isLinkedVideoSink = TRUE;
        gst_element_link(parse->m_Demuxer, parse->m_Video_sink);
        sinkpad = gst_element_get_static_pad (parse->m_Video_sink, "sink");
        g_object_set (parse->m_Video_sink, "signal-handoffs", TRUE, NULL);
        LOG_GSTPARSE_MESSAGE ("[GSTParse] Add video handoff signal\n");
        parse->m_videoHandOffHandler = g_signal_connect (parse->m_Video_sink, "handoff", G_CALLBACK(buffer_add), parse);
#if USE_EOS
        parse->m_videoSinkEventProbeHandler = gst_pad_add_event_probe(sinkpad, G_CALLBACK(sink_event_probe_cb), parse);
#endif
    } else if(g_strrstr(capsStr, "audio") && parse->m_isLinkedAudioSink == FALSE) {
        parse->m_Audio_sink = gst_element_factory_make ("fakesink", "audio-fakesink");
        if(!parse->m_Audio_sink) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse] audio sink element make failed\n");
            gst_caps_unref (caps);
            MMPARSE_FREEIF(capsStr);
            return;
        }
        if(!gst_bin_add (GST_BIN (parse->m_Pipeline), parse->m_Audio_sink)) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse] audio sink element bin add failed\n");
            gst_caps_unref (caps);
            MMPARSE_FREEIF(capsStr);
            return; /* TO DO. possible memory leak here */
        }
        parse->m_isLinkedAudioSink = TRUE;
        gst_element_link(parse->m_Demuxer, parse->m_Audio_sink);
        sinkpad = gst_element_get_static_pad (parse->m_Audio_sink, "sink");
        g_object_set (parse->m_Audio_sink, "signal-handoffs", TRUE, NULL);
        LOG_GSTPARSE_MESSAGE ("[GSTParse] Add audio handoff signal\n");
        parse->m_audioHandOffHandler = g_signal_connect (parse->m_Audio_sink, "handoff", G_CALLBACK(buffer_add), parse);
#if USE_EOS
        parse->m_audioSinkEventProbeHandler = gst_pad_add_event_probe(sinkpad, G_CALLBACK(sink_event_probe_cb), parse);
#endif
    } else {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] Any element didn't linked\n");
        gst_caps_unref (caps);
        MMPARSE_FREEIF(capsStr);
        return;
    }

    gst_caps_unref (caps);
    MMPARSE_FREEIF(capsStr);
    gst_object_unref (sinkpad);

    LOG_GSTPARSE_MESSAGE ("[GSTParse] end dynamic_pad\n");

    /* Set the pipeline to "playing" state*/
    LOG_GSTPARSE_MESSAGE ("[GSTParse] Now playing\n");
    gst_element_set_state (parse->m_Pipeline, GST_STATE_PLAYING);

    return;
}
#if UNDERRUN
void
GSTParse::underrunCb(GstElement* /* element */, gpointer data)
{
    GSTParse* parse = (GSTParse*)data;
    if (parse->m_parseEndType == PARSE_END_TYPE_UNDERRUN) {
        if (parse->isSetParseCompleteCB()) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] parse complete callback\n", parse->m_eMimetype);
            parse->parseCompleteCB();
        }
        else {
            LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] send cond signal -> can feed next data\n", parse->m_eMimetype);
            g_cond_signal(parse->m_Cond);
        }
    }
    return;
}
#endif
gboolean
GSTParse::init_gstreamer()
{
    LOG_GSTPARSE_MESSAGE ("[GSTParse] start init_gstreamer\n");
    static gboolean initialized = FALSE;
    GError *err = NULL;

    if (initialized) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] gstreamer already initialized\n");
        return TRUE;
    }

    if (!gst_init_check(0, 0, &err)) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] Could not initialize Gstreamer: %s\n", err ? err->message : "unknown error occurred");
        return FALSE;
    }

    initialized = TRUE;

    LOG_GSTPARSE_MESSAGE ("[GSTParse] end init_gstreamer\n");

    return TRUE;
}

gboolean
GSTParse::bus_call (GstBus* /* bus */, GstMessage* msg, gpointer /* data */)
{
    switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
        LOG_GSTPARSE_MESSAGE ("[GSTParse] End of stream\n");
        break;

    case GST_MESSAGE_ERROR:
        gchar* debug;
        GError* error;

        gst_message_parse_error (msg, &error, &debug);
        g_free (debug);

        LOG_GSTPARSE_MESSAGE ("[GSTParse] Error: %s\n", error->message);
        g_error_free (error);
        break;

    default:
        break;
    }

    return TRUE;
}

void
GSTParse::setinitsegmentCB(initsegmentCallBack cbFunction, void* data)
{
    if (!cbFunction) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] callback function can't be NULL\n");
        return;
    }

    m_pCBinitsegment = cbFunction;
    m_CBinitsegmentData = data;
}

void
GSTParse::initsegmentCB(void* data)
{
    if(m_pCBinitsegment) {
        m_pCBinitsegment(data);
    }
}

#if USE_EOS
void
GSTParse::setParseCompleteCB(parseCompleteCallback funct, void* data)
{
    if (!funct) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] callback function can't be NULL\n");
        return;
    }

    m_parseCompleteCB = funct;
    m_parseCompleteData = data;
}

void
GSTParse::parseCompleteCB(void)
{
    if (m_parseCompleteCB) {
        if (!m_parseCompleteTimerHandler)
            m_parseCompleteTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(m_parseCompleteCB), m_parseCompleteData);
    }
}

/* temp. this funct will be removed after webkit code is ready for parse complete cb. */
gboolean
GSTParse::isSetParseCompleteCB(void)
{
    if (!m_parseCompleteCB)
        return FALSE;
    return TRUE;
}

#endif
void GSTParse::setNeedKeyCB(needKeyCallback needkeyCallbackFunction, gpointer userData)
{
//    if (!needkeyCallbackFunction)
//        return;
    m_needKeyCallbackFunction = needkeyCallbackFunction;
    m_needkeyCallbackUserData = userData;
}

gboolean
GSTParse::createPipeline(void)
{
    GstBus *bus = NULL;
#if USE_EOS
    GstPad *srcpad = NULL;
#endif
    LOG_GSTPARSE_MESSAGE ("[GSTParse] start _mmparse_create_pipeline\n");

    m_Pipeline = gst_pipeline_new ("parse-pipe-line");
    m_Source   = gst_element_factory_make ("appsrc",       "app-source");

    /* TODO : demuxer should be selected correspond to mimetype */
    if(m_eMimetype == MIME_TYPE_WEBM_AUDIO || m_eMimetype == MIME_TYPE_WEBM_VIDEO )
        m_Demuxer = gst_element_factory_make("ffdemux_matroska_webm","ff-demuxer");
    else
        m_Demuxer = gst_element_factory_make("ffdemux_mov_mp4_m4a_3gp_3g2_mj2","ff-demuxer");

    if (!m_Pipeline || !m_Source || !m_Demuxer) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] failed create pipeline or element\n");
        return FALSE;
    }
#if USE_EOS
    srcpad = gst_element_get_static_pad (m_Source, "src");
    m_sourceEventProbeHandler = gst_pad_add_event_probe(srcpad, G_CALLBACK(src_event_probe_cb), (gpointer)this);
    gst_object_unref(srcpad);
#endif
    bus = gst_pipeline_get_bus (GST_PIPELINE (m_Pipeline));
    m_Bus_watch_id = gst_bus_add_watch (bus, bus_call, (gpointer)this);
    gst_object_unref (bus);

#if USE_EOS
    m_ev_thread = g_thread_create(eos_ev_thread, (gpointer)this, TRUE, NULL);
#endif

    gst_bin_add_many (GST_BIN (m_Pipeline), m_Source, m_Demuxer, NULL);

    gst_element_link_many (m_Source, m_Demuxer, NULL);
    m_padAddedHandler = g_signal_connect (m_Demuxer, "pad-added", G_CALLBACK (dynamic_pad), this);
#if UNDERRUN
    m_underrunHandler = g_signal_connect (m_Demuxer, "underrun", G_CALLBACK (underrunCb), this);
#endif
    /* warm-up the pipeline */
    gst_element_set_state(m_Pipeline, GST_STATE_PAUSED);

    LOG_GSTPARSE_MESSAGE ("[GSTParse] end _mmparse_create_pipeline\n");

    return TRUE;

}

gboolean
GSTParse::havePipeline()
{
    return m_Pipeline ? TRUE : FALSE;
}

gboolean
GSTParse::haveMimetype()
{
    return m_Mimetype ? TRUE : FALSE;
}

#ifdef MP4_SUPPORT_ENABLED
gboolean
GSTParse::extractBoxtypeLength(const gchar* data, guint len, guint* ptype, guint* psize)
{
    /* get fourcc/length, set neededbytes */
    guint box_type = 0;
    guint box_size = 0;

    if (data == NULL)
        return FALSE;

    /* caller verifies at least 8 bytes in buf */
    if (len < 8) {
        m_parseState = PARSE_STATE_ERROR;
        return FALSE;
    }

    box_size = DATA_READ_UINT32_BE (&data[0]);
    box_type = DATA_READ_UINT32_BE (&data[4]);
    if (box_size == 0) {
        LOG_GSTPARSE_MESSAGE("[GSTParse] [WARN] BOX size is zero (%d)\n", box_size);
        m_parseState = PARSE_STATE_INVALID;
        return FALSE;
    }

    if (ptype)
        *ptype = box_type;
    if (psize)
        *psize = box_size;

    return TRUE;
}


gboolean
GSTParse::simpleParseBoxTkhd(const gchar* boxdata, guint /* len */)
{
    gint version = 0;
    gint track_id = 0;
    gint skip_byte = 0;
    guint tkhd_offset = 0;

    /* need information */
    gint width = 0;
    gint height = 0;

    /* Parse 'tkhd' box. */
    skip_byte = 0;
    version = DATA_READ_UINT8 (&boxdata[tkhd_offset+8]);
    skip_byte += (4+4+4);
    if (version) {
        skip_byte += (8+8);     //skip (creationTime,modificationTime)
        track_id = DATA_READ_UINT32_BE (&boxdata[tkhd_offset+skip_byte+0]); //trackID
        skip_byte += (4+4+8);   //skip (trackID,reserved,duration)
    } else {
        skip_byte += (4+4);
        track_id = DATA_READ_UINT32_BE (&boxdata[tkhd_offset+skip_byte+0]);
        skip_byte += (4+4+4);
    }
    skip_byte += (4+4+2+2+2+2);     //skip (reserved,reserved,layer,alternateGroup,volume,reserved)
    skip_byte += (4*9);             //skip (matrix[9])

    width = DATA_READ_UINT32_BE (&boxdata[tkhd_offset+skip_byte+0]);
    height = DATA_READ_UINT32_BE (&boxdata[tkhd_offset+skip_byte+4]);
    width = (width>>16);
    height = (height>>16);
    LOG_GSTPARSE_MESSAGE ("[GSTParse] [INFO] track_ID [%d / %d]\n", track_id, (m_next_track_ID-1));
    LOG_GSTPARSE_MESSAGE ("[GSTParse] [INFO] m_width [%4d] height [%4d] \n", width, height);

    if ((width > 0) && (height > 0)) {
        m_Width = width;
        m_Height = height;

        m_have_videotrack = 1;
        m_have_videotrack_id = track_id;
        LOG_GSTPARSE_MESSAGE ("[GSTParse] [INFO] This has Video track\n");
    } else if ((width == 0) && (height == 0)) {
        m_have_audiotrack = 1;
        m_have_audiotrack_id = track_id;
        LOG_GSTPARSE_MESSAGE ("[GSTParse] [INFO] This has Audio track\n");
    } else {
        /* error case */
        m_have_videotrack = 0;
        m_have_videotrack_id = 0;
        m_have_audiotrack = 0;
        m_have_audiotrack_id = 0;
        LOG_GSTPARSE_MESSAGE ("[GSTParse] [ERROR] This hasn't any track\n");
        return FALSE;
    }

    return TRUE;
}

gboolean
GSTParse::simpleParseBoxMdia(const gchar* boxdata, guint len)
{
    gint ret = 0;
    guint mdia_offset;
    guint mdia_type, mdia_size;
    guint node_type, node_size;
    gint version = 0;
    gint skip_byte = 0;

    /* need information */
    guint timescale = 0;
    guint64 duration = 0;

    if (!extractBoxtypeLength(boxdata, len, &mdia_type, &mdia_size))
        return FALSE;

    /* skip box type, size (8byte) */
    mdia_offset = 8;
    do {
        ret = extractBoxtypeLength(&boxdata[mdia_offset], len, &node_type, &node_size);
        if (ret) {
            switch (node_type) {
            case MAKE_TAG_BE('m','d','h','d'):
            {
                /* Parse 'mdhd' box. */
                skip_byte = 0;
                version = DATA_READ_UINT8 (&boxdata[mdia_offset+8]);
                skip_byte += (4+4+4);
                if (version) {
                    if (node_size < 38)
                    return FALSE;       //corrupt file
                    skip_byte += (8+8);     //skip (creationTime,modificationTime)
                    timescale = DATA_READ_UINT32_BE (&boxdata[mdia_offset+skip_byte+0]);
                    duration = DATA_READ_UINT64_BE (&boxdata[mdia_offset+skip_byte+4]);
                    skip_byte += (4+8);     //skip (timescale,duration)
                } else {
                    if (node_size < 30)
                        return FALSE;       //corrupt file
                    skip_byte += (4+4);
                    timescale = DATA_READ_UINT32_BE (&boxdata[mdia_offset+skip_byte+0]);
                    duration = DATA_READ_UINT32_BE (&boxdata[mdia_offset+skip_byte+4]);
                    skip_byte += (4+4);
                }
                if (timescale == 0)
                 return FALSE;      //corrupt file

                if (m_have_audiotrack) {
                    m_audio_timescale = timescale;
                    m_audio_duration = duration;
                } else {
                    m_video_timescale = timescale;
                    m_video_duration = duration;
                }
                LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] track timescale [%4d] duration [%d] \n", timescale, duration);
                break;
            }
            case MAKE_TAG_BE('h','d','l','r'):
            case MAKE_TAG_BE('m','i','n','f'):
                break;
            default:
                break;
            }
            mdia_offset += node_size;
        }
    } while(mdia_offset < mdia_size);

    if (mdia_offset != mdia_size) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] [WARN] mdia node has invalid size (%d / %d)\n", mdia_offset, mdia_size);
        return FALSE;
    }

    return TRUE;
}

gboolean
GSTParse::simpleParseBoxTrak(const gchar* boxdata, guint len)
{
    gint ret = 0;
    guint trak_offset;
    guint trak_type, trak_size;
    guint node_type, node_size;

    if (!extractBoxtypeLength(boxdata, len, &trak_type, &trak_size))
        return FALSE;

    /* skip box type, size (8byte) */
    trak_offset = 8;

    do {
        ret = extractBoxtypeLength(&boxdata[trak_offset], len, &node_type, &node_size);
        if (ret) {
            switch (node_type) {
            case MAKE_TAG_BE('t','k','h','d'):
                if (!simpleParseBoxTkhd(&boxdata[trak_offset], len))
                    return FALSE;
                break;
            case MAKE_TAG_BE('m','d','i','a'):
                if (!simpleParseBoxMdia(&boxdata[trak_offset], len))
                    return FALSE;
                break;
            default:
                break;
            }
            trak_offset += node_size;
        }
    } while(trak_offset < trak_size);

    if (trak_offset != trak_size) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] [WARN] trak node has invalid size (%d / %d)\n", trak_offset, trak_size);
        return FALSE;
    }

    return TRUE;
}


gboolean
GSTParse::simpleParseBoxMvhd(const gchar* boxdata, guint /* len */)
{
    gint version = 0;
    gint skip_byte = 0;
    guint timescale = 0;
    guint64 duration = 0;

    /* need information */
    m_next_track_ID = 0;

    /* Parse 'mvhd' box */
    version = DATA_READ_UINT8 (&boxdata[8]);
    skip_byte = (4+4+4);
    if (version) {
        skip_byte += (8+8);     //skip (creationTime,modificationTime)
        timescale = DATA_READ_UINT32_BE (&boxdata[skip_byte+0]);
        duration = DATA_READ_UINT64_BE (&boxdata[skip_byte+4]);
        skip_byte += (4+8);     //skip (timescale,duration)
    } else {
        skip_byte += (4+4);
        timescale = DATA_READ_UINT32_BE (&boxdata[skip_byte+0]);
        duration = DATA_READ_UINT32_BE (&boxdata[skip_byte+4]);
        skip_byte += (4+4);
    }
    if (timescale == 0)
        return FALSE;           //corrupt file
    LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] mvhd timescale [%4d] duration [%d] \n", timescale, duration);

    skip_byte += (4+2+2+4+4);       // (rate,volume,reserved,reserved,reserved)
    skip_byte += (4*9);             // (matrix[9])
    skip_byte += (4*6);             // (preDefined[6])

    m_next_track_ID = DATA_READ_UINT32_BE (&boxdata[skip_byte]);
    if (m_next_track_ID == 0)
        return FALSE;           //corrupt file
    LOG_GSTPARSE_MESSAGE ("[GSTParse] [INFO] next_track_ID [%4d] -> Track have %d \n", m_next_track_ID, (m_next_track_ID-1));

    return TRUE;
}


gboolean
GSTParse::simpleParseBoxMoovNode(const gchar* boxdata, guint len, guint node_type)
{
    switch (node_type) {
    case MAKE_TAG_BE('m','v','h','d'):
        if (!simpleParseBoxMvhd(boxdata, len))
            return FALSE;
        break;
    case MAKE_TAG_BE('t','r','a','k'):
        if (!simpleParseBoxTrak(boxdata, len))
            return FALSE;
        break;
    case MAKE_TAG_BE('m','v','e','x'):
    case MAKE_TAG_BE('p','s','s','h'):
        break;
    default:
        /* optional node is skip*/
        break;
    }

    return TRUE;
}

gboolean
GSTParse::simpleParseBoxFtyp(const gchar* /* data */, guint /* len */, guint box_size)
{
    /* only consider at least a sufficiently complete ftyp atom */
    if (box_size < 20)
        return FALSE;

    return TRUE;
}

gboolean
GSTParse::isMp4InitSegment(const gchar* data, guint len)
{
    gint ret = 0;
    guint box_type = 0;
    guint box_size = 0;
    gint got_ftyp, got_moov, got_free;
    gint got_styp, got_moof, got_mdat;
    got_ftyp = got_moov = got_free = 0;
    got_styp = got_moof = got_mdat = 0;

    /* Full scan for top-level BOXes check in data*/

    m_parseState = PARSE_STATE_NONE;
    m_initSegmentLen = 0;       /* FIXME - we will use futher check */
    m_mediaSegmentLen = 0;      /* FIXME - we will use futher check */
    m_moovBoxPosition = 0;
    do {
        ret = extractBoxtypeLength(&data[m_Bufoffset], len, &box_type, &box_size);
        if (ret) {
            switch (box_type) {
            case MAKE_TAG_BE('f','t','y','p'):
            {
                if (simpleParseBoxFtyp(&data[m_Bufoffset], len, box_size)) {
                    m_initSegmentLen += box_size;
                    got_ftyp = 1;
                } else {
                    got_ftyp = 0;
                }
                break;
            }
            case MAKE_TAG_BE('m','o','o','v'):
                m_moovBoxPosition = m_Bufoffset;    /* backup for need */
                m_initSegmentLen += box_size;
                got_moov = 1;
                break;
            case MAKE_TAG_BE('f','r','e','e'):
                m_initSegmentLen += box_size;
                got_free = 1;
                break;
            case MAKE_TAG_BE('s','t','y','p'):
                m_mediaSegmentLen += box_size;
                got_styp = 1;
                break;
            case MAKE_TAG_BE('m','o','o','f'):
                m_mediaSegmentLen += box_size;
                got_moof = 1;
                break;
            case MAKE_TAG_BE('m','d','a','t'):
                m_mediaSegmentLen += box_size;
                got_mdat = 1;
                break;
            case MAKE_TAG_BE('s','i','d','x'):
            case MAKE_TAG_BE('m','f','r','a'):
            case MAKE_TAG_BE('p','d','i','n'):
                /* pass-through */
                LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] Known box_type is (0x%x), so Skip\n", box_type);
                break;
            default:
                LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] Un-known box_type is (0x%x), so Skip\n", box_type);
                m_parseState = PARSE_STATE_INVALID;
                break;
            }

            if (m_parseState == PARSE_STATE_INVALID)
                break;
            m_Bufoffset += box_size;
        } else {
            LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] Invalid BOX type(0x%x) size(%d)\n", box_type, box_size);
            m_parseState = PARSE_STATE_INVALID;
            break;
        }
    } while(m_Bufoffset < len);

    /* check remaind buffer length */
    if (m_Bufoffset != len) {
        LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] m_Bufoffset(%d) isn't equal incomming len (%d)\n", m_Bufoffset, len);
        if ((got_ftyp == 1) && (got_moov == 1))
            m_parseState = PARSE_STATE_INIT_MEDIA_INCOMPLETE;
        else {
            m_parseState = PARSE_STATE_MEDIA_INCOMPLETE;
            m_Bufoffset = 0;
        }
    } else {
        /* FIXME - Parse state checker */
        if ((got_ftyp == 1) && (got_moov == 1)) {
            if ((got_moof == 1) && (got_mdat == 1)) {
                /*It is (init segment) + (media segment) stream */
                m_parseState = PARSE_STATE_INIT_MEDIA_SEGMENT;
            } else {
                /*It is init segment stream */
                m_parseState = PARSE_STATE_INIT_SEGMENT;
            }
        } else if ((got_styp == 1) || (got_moof == 1) || (got_mdat == 1)) {
            /* It is media segment stream */
            m_parseState = PARSE_STATE_MEDIA_SEGMENT;
            m_Bufoffset = 0;
        } else {
            m_parseState = PARSE_STATE_MEDIA_INCOMPLETE;
            m_Bufoffset = 0;
        }
    }

    LOG_GSTPARSE_MESSAGE("[GSTParse] [INFO] parse state is [%d]\n", m_parseState);

    switch (m_parseState) {
    case PARSE_STATE_INIT_SEGMENT:
    case PARSE_STATE_INIT_MEDIA_SEGMENT:
    case PARSE_STATE_INIT_MEDIA_INCOMPLETE:
        return TRUE;
    case PARSE_STATE_MEDIA_SEGMENT:
    case PARSE_STATE_MEDIA_INCOMPLETE:
    case PARSE_STATE_INVALID:                /* FIXME - incomplete */
        return FALSE;
    default:
        /* FIXME - case of error */
        return FALSE;
    }
}
#endif


gboolean
GSTParse::metaDataExtract_MP4(const gchar* data, guint len)
{
    gint ret = 0;
    gint found = 0;

    /* MOOV Box */
    guint moov_type, moov_size, moov_offset;
    /* Node Box - common */
    guint node_type, node_size;
    guint got_mvhd, got_mvex;
    guint got_trak1, got_trak2, got_trak3;  /* (video/audio/text)..FixMe...for futher track# */
    guint got_unknown;                      /* unknow counter */
    /* Node Box - mvhd / trak / mvex */
    guint mvhd_position = 0;
    guint trak1_position = 0;
    guint trak2_position = 0;
    guint trak_count = 0;

    /* initialize */
    m_Bufoffset = m_moovBoxPosition;
    got_mvhd = got_mvex = got_unknown = 0;
    got_trak1 = got_trak2 = got_trak3 = 0;

    /* MOOV BOX full node checker */
    moov_type = moov_size = moov_offset = 0;
    ret = extractBoxtypeLength(&data[m_Bufoffset], len, &moov_type, &moov_size);

    if(ret) {
        if (moov_type == MAKE_TAG_BE('m','o','o','v')) {
            /* skip box type, size (8byte) */
            moov_offset = 8;

            do {
                ret = extractBoxtypeLength(&data[m_Bufoffset + moov_offset], len, &node_type, &node_size);
                if (ret) {
                    switch (node_type) {
                    case MAKE_TAG_BE('m','v','h','d'):
                        mvhd_position = moov_offset;
                        got_mvhd = 1;
                        break;
                    case MAKE_TAG_BE('t','r','a','k'):
                        trak_count++;
                        if (trak_count == 1) {
                            trak1_position = moov_offset;
                            got_trak1 = 1;
                        } else if (trak_count == 2) {
                            trak2_position = moov_offset;
                            got_trak2 = 1;
                        } else if (trak_count == 3) {
                            got_trak3 = 1;
                        } else {
                            LOG_GSTPARSE_MESSAGE ("[GSTParse] [WARN] 'trak' count is bigger than 3\n");
                        }
                        break;
                    case MAKE_TAG_BE('m','v','e','x'):
                        got_mvex = 1;
                        break;
                    default:
                        /* optional node is skip*/
                        got_unknown++;
                        break;
                    }
                    moov_offset += node_size;

                } else {
                    LOG_GSTPARSE_MESSAGE ("[GSTParse] [ERROR] moov's node is invalid\n");
                    return FALSE;
                }
            } while (moov_offset < moov_size);

            if (moov_offset != moov_size) {
                LOG_GSTPARSE_MESSAGE ("[GSTParse] [WARN] moov node is invalid size (%d / %d)\n", moov_offset, moov_size);
                return FALSE;
            }
        }
    }

    /* node parser for extract infomation */
    if (got_mvhd) {
        if (simpleParseBoxMoovNode(&data[m_Bufoffset + mvhd_position], len, MAKE_TAG_BE('m','v','h','d')))
            found = 1;
    }

    if (got_trak1) {
        if (simpleParseBoxMoovNode(&data[m_Bufoffset + trak1_position], len, MAKE_TAG_BE('t','r','a','k')))
            found = 1;
    }

    if (got_trak2) {
        if (simpleParseBoxMoovNode(&data[m_Bufoffset + trak2_position], len, MAKE_TAG_BE('t','r','a','k')))
            found = 1;
    }

    m_Bufoffset = 0;    //reset
    return found;
}


gboolean
GSTParse::metaDataExtract_WebM(const gchar* data, guint len)
{
    guint elementID = 0;
    guint length = 0;
    guint Needed = 0;
    guint offset = 0;
    guint bytesForwarded = 0;
    gboolean retVal = FALSE;
    /* Traverse SEGMENT till video track or end found. */
    while(getNextElementID(&data[offset],len,&elementID,&length,&Needed) == GST_FLOW_OK)
    {
        LOG_GSTPARSE_MESSAGE("Offset:[0x%5d] ,ElementID:[0x%0X],Length:[%0d],Needed:[%0X]\n",m_Bufoffset,elementID,length,Needed);
        if(EBML_ID_TRACK_ENTRY == elementID || EBML_ID_INFO == elementID)
            parseTrackInfo_WebM(&data[offset+Needed],length);
        if(EBML_ID_TRACKS == elementID) {
            len = length;
            length = 0;
        }
        bytesForwarded = Needed + length;
        m_Bufoffset += bytesForwarded;
        offset += bytesForwarded;
        len -= bytesForwarded;
        Needed = 0;
        length = 0;
    }
    m_parseState = PARSE_STATE_INIT_MEDIA_INCOMPLETE;
    retVal = m_have_videotrack || m_have_audiotrack;
    return retVal;
}

gboolean
GSTParse::parseTrackInfo_WebM(const gchar* data, guint len){
    guint elementID = 0;
    guint length = 0;
    guint Needed = 0;
    guint offset = 0;
    guint width = 0;
    guint height = 0;
    guint64 trackUID = 0;
    gboolean retVal = FALSE;

    /* Traverse SEGMENT till video track or end found. */
    while(getNextElementID(&data[offset],len,&elementID,&length,&Needed) == GST_FLOW_OK)
    {
        LOG_GSTPARSE_MESSAGE("Offset:[0x%5d] ,ElementID:[0x%0X],Length:[%0d],Needed:[%0X]\n",m_Bufoffset,elementID,length,Needed);
        if(EBML_ID_TIMECODESCALE == elementID) /* TimeCodeScale */
                m_timescale_webm = readVarInt_Max64_BE(&data[offset+Needed],length);
        else if(EBML_ID_DURATION == elementID) /* Duration */
                m_duration_webm = readVarFloat_MaxDouble(&data[offset+Needed],length);
        else if(EBML_ID_TRACK_NUMBER == elementID) /* Track Number */
                m_have_track_number_webm = readVarInt_Max64_BE(&data[offset+Needed],length);
        else if(EBML_ID_TRACK_UID == elementID) /* Track UID */
                trackUID = readVarInt_Max64_BE(&data[offset+Needed],length);
        else if(EBML_ID_TRACK_TYPE == elementID) /* Track Type. Audio or video */
                m_have_track_type_webm = readVarInt_Max64_BE(&data[offset+Needed],length);
        else if(EBML_ID_AUDIO_TRACK == elementID) {/* if audio track fround. update audio trak info. */
                m_have_audiotrack_id_webm = trackUID;
                m_have_audiotrack = TRUE;
                retVal = TRUE;
        } else if(EBML_ID_VIDEO_TRACK == elementID) { /* if video track fround. update video trak info. */
                m_have_videotrack_id_webm = trackUID;
                offset += Needed;
                len = length;
                while(getNextElementID(&data[offset],len,&elementID,&length,&Needed) == GST_FLOW_OK) {

                    if(EBML_ID_VIDEOPIXELWIDTH == elementID || EBML_ID_VIDEODISPLAYWIDTH == elementID)
                        width = readVarInt_Max64_BE(&data[offset+Needed],length);
                    if(EBML_ID_VIDEODISPLAYHEIGHT == elementID || EBML_ID_VIDEOPIXELHEIGHT == elementID)
                        height = readVarInt_Max64_BE(&data[offset+Needed],length);
                    offset += Needed + length;
                    len -= (Needed + length);
               }
               if( width > 0 && height > 0) {
                    m_Width = width;
                    m_Height = height;
                    LOG_GSTPARSE_MESSAGE("Video Height:[%d], Video Width:[%d]\n",height,width);
                    retVal = TRUE;
                    m_have_videotrack = TRUE;
               }
        }
        offset += (Needed + length);
        len -= (Needed + length);
    }
    LOG_GSTPARSE_MESSAGE("Track Number:[0x%0X],Track Type:[[0x%X]], Track UID:[0x%X%X]\n",m_have_track_number_webm,
    m_have_track_type_webm,*(((gint*)(&trackUID))+1), trackUID);
    LOG_GSTPARSE_MESSAGE("TimeCodeScale:[%d], Duration:[%lf]\n",m_timescale_webm,m_duration_webm);
    return retVal;
}
guint64
GSTParse::readVarInt_Max64_BE(const char *data, guint dataLen)
{
    guint64 val = 0;
    guint i = 0;
    for(i = 0; i < dataLen; i++)
        val = (val << 8) | (((const g_uint8 *) (data))[i]);
    return val;
}
double
GSTParse::readVarFloat_MaxDouble(const char *data, guint dataLen)
{
    if(dataLen == 4) /* float */
    {
        guint32 val = DATA_READ_UINT32_BE(data);
        return (double)intBitsToFloat(val);
    }else{
        guint64 val = DATA_READ_UINT64_BE(data);
        return intBitsToDouble(val);
    }
};
float
GSTParse::intBitsToFloat(guint32 value)
{
    union uIntFloatBits
    {
        guint32 intBits;
        float floatBits;
    } bits;

    bits.intBits = value;
    return bits.floatBits;
}

double
GSTParse::intBitsToDouble(guint64 value)
{
    union uIntDoubleBits
    {
        guint64 intBits;
        double doubleBits;
    } bits;
    bits.intBits = value;
    return bits.doubleBits;
}


gboolean
GSTParse::metaDataExtract(const gchar* data, guint len)
{
    if(m_eMimetype == MIME_TYPE_MP4_AUDIO || m_eMimetype == MIME_TYPE_MP4_VIDEO)
        return metaDataExtract_MP4(data,len);
    else if(m_eMimetype == MIME_TYPE_WEBM_AUDIO || m_eMimetype == MIME_TYPE_WEBM_VIDEO)
        return metaDataExtract_WebM(&data[m_Bufoffset],len-m_Bufoffset);
    else
        return FALSE;
}


gboolean
GSTParse::initSegmentChecker(const gchar* data, guint len)
{
    if (data == NULL)
        return FALSE;

    if(m_eMimetype == MIME_TYPE_WEBM_AUDIO || m_eMimetype == MIME_TYPE_WEBM_VIDEO) {
        /* check for EBML ID */
        if(!isEBMLheaderPresent(data,len)) {
            m_Isinitseg = FALSE;
            m_parseState = PARSE_STATE_NONE;
        }
        else {
            if(!isEBMLSegmentPresent(&data[m_Bufoffset],len-m_Bufoffset))
                m_Isinitseg = FALSE;
            else {
                m_Isinitseg = TRUE;
                m_parseState = PARSE_STATE_INIT_SEGMENT;
            }
        }
    } else {
            /* check for MP4 */
            guint box_type = 0;
            guint box_size = 0;

            if (!extractBoxtypeLength(data, len, &box_type, &box_size)) {
                m_Isinitseg = FALSE;
                return m_Isinitseg;
            }

            if (isMp4InitSegment(data, len)) {
                m_Isinitseg = TRUE;
            } else {
                m_Isinitseg = FALSE;
            }
    }

    return m_Isinitseg;
}

gboolean
GSTParse::parse(const gchar* data, guint len)
{
    GstBuffer* appsrc_input_buffer;
    GstFlowReturn ret;
    gboolean result;
    m_Isinitseg = FALSE;
    /* TODO : need to verify incomming parameter */

    LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] parse data %d\n", m_eMimetype, len);

    m_parseEndType = PARSE_END_TYPE_NONE;

    if (!haveMimetype()) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] mimetype should be given before parsing any data\n");
        return FALSE;
    }

    result = initSegmentChecker(data, len);

    if (m_Isinitseg) {
        result = metaDataExtract(data, len);
        if (result)
        {
            m_Haveinitseg = m_Isinitseg;
            LOG_GSTPARSE_MESSAGE ("[GSTParse] It is Init segment \n");
            initsegmentCB(m_CBinitsegmentData);
        } else {
            m_Haveinitseg = FALSE;
            LOG_GSTPARSE_MESSAGE ("[GSTParse] [FAIL] It is invalid Init segment \n");
        }
    }

    if (!havePipeline()) {
        init_gstreamer();
        createPipeline();
    }
#if USE_EOS
        if ((m_parseState == PARSE_STATE_INIT_MEDIA_SEGMENT)
            || (m_parseState == PARSE_STATE_MEDIA_SEGMENT)) {
            m_parseEndType = PARSE_END_TYPE_EOS;
        } else if ((m_parseState == PARSE_STATE_MEDIA_INCOMPLETE)
                || (m_parseState == PARSE_STATE_INIT_MEDIA_INCOMPLETE)){
            m_parseEndType = PARSE_END_TYPE_UNDERRUN;
        } else if (m_parseState == PARSE_STATE_INIT_SEGMENT) {
            m_parseEndType = PARSE_END_TYPE_UNDERRUN;
        } else
            m_parseEndType = PARSE_END_TYPE_NONE;
#endif
    /* Create a new empty buffer */
    appsrc_input_buffer = createGstBufferForData(data, len);

    ret = gst_app_src_push_buffer(GST_APP_SRC(m_Source), appsrc_input_buffer);

#if USE_EOS
    if ((m_parseState == PARSE_STATE_INIT_MEDIA_SEGMENT)
        || (m_parseState == PARSE_STATE_MEDIA_SEGMENT)) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] send EOS signal\n", m_eMimetype);
        g_signal_emit_by_name (m_Source, "end-of-stream", &ret);
    }

    if ((m_parseState == PARSE_STATE_INIT_MEDIA_SEGMENT)
        || (m_parseState == PARSE_STATE_MEDIA_SEGMENT)
        || (m_parseState == PARSE_STATE_MEDIA_INCOMPLETE)
        || (m_parseState == PARSE_STATE_INIT_MEDIA_INCOMPLETE)) {
        if (!isSetParseCompleteCB()) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse][%d] no parse complete cb, wait until end parsing\n", m_eMimetype);
            g_cond_wait (m_Cond, m_Lock);
        }
    }
#endif
    if (!m_Haveinitseg)
        return FALSE;

    /* TODO : need to wait for drain of incomming data */

    return TRUE;
}

GstBuffer*
GSTParse::getNextAudioFrame()
{
    GList *buffer_list = NULL;
    GstBuffer *buffer = NULL;

    buffer_list = m_Audio_frames;
    if (!buffer_list)
        return NULL;
    buffer = (GstBuffer*)buffer_list->data;
    if (!buffer)
        return NULL;
    /* TODO : remove returned buffer */
    g_mutex_lock(m_Mutex);
    m_Audio_frames = g_list_next(m_Audio_frames);
    g_mutex_unlock(m_Mutex);
    return buffer;
}

GstBuffer*
GSTParse::getNextVideoFrame()
{
    GList *buffer_list = NULL;
    GstBuffer *buffer = NULL;

    buffer_list = m_Video_frames;
    if ( ! buffer_list )
        return NULL;

    buffer = (GstBuffer*)buffer_list->data;
    if (!buffer)
        return NULL;

    /* TODO : remove returned buffer */
    g_mutex_lock(m_Mutex);
    m_Video_frames = g_list_next(m_Video_frames);
    g_mutex_unlock(m_Mutex);
    return buffer;
}

GstBuffer*
GSTParse::getNextFrame(gboolean isVideo)
{
    return isVideo ? getNextVideoFrame() : getNextAudioFrame();
}

gboolean
GSTParse::getVideoDimension(int *width, int *height)
{
    if (!width || !height)
        // input argument check
        return FALSE;

    if (m_Width > 0 && m_Height >0) {
        *width = m_Width;
        *height = m_Height;
        return TRUE;
    } else {
        // width, height check...
        LOG_GSTPARSE_MESSAGE ("[GSTParse] demension value < 0\n");
        return FALSE;
    }
}

void
GSTParse::getMediaTrackInfo(gboolean* haveAudio, gboolean* haveVideo)
{
    if(!m_have_audiotrack || !m_have_videotrack)
        LOG_GSTPARSE_MESSAGE ("[GSTParse] getMediaTrackInfo invalid argument\n");

    *haveAudio = m_have_audiotrack;
    *haveVideo = m_have_videotrack;

    return;
}

void
GSTParse::getMediaTimescale(gboolean isVideo, guint* timescale, guint64* duration)
{
    if(!timescale || !duration)
        LOG_GSTPARSE_MESSAGE ("[GSTParse] getMediaTimescale invalid argument\n");

    if(m_eMimetype == MIME_TYPE_WEBM_AUDIO || m_eMimetype == MIME_TYPE_WEBM_VIDEO) {
        *timescale = m_timescale_webm;
        *duration = (guint64)m_duration_webm;
    } else { /* MP4 media content */
        if(isVideo){
            *timescale = m_video_timescale;
            *duration = m_video_duration;
        } else {
            *timescale = m_audio_timescale;
            *duration = m_audio_duration;
        }
    }
    LOG_GSTPARSE_MESSAGE ("[GSTParse] MediaTimescale:[%u], Duration:[%u]\n",*timescale,*duration);
    return;
}


#if ENABLE_EME
#if 0
void GSTParse::getDrmElement(GstElement* element)
{
    if (!element) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] drm element is corrupted...\n");
        return;
    }

// Webkit can not distinguish between video drm element or audio drm element. generally, audio data's drm element is added before video's .
// if container has only audio data or video data, playbin2 can have one drmbridge element.
    if (!m_audioDrmBridge) {
        m_audioDrmBridge = element;
        LOG_GSTPARSE_MESSAGE ("[GSTParse] the 1st DRMBridge is %s\n", GST_ELEMENT_NAME(m_audioDrmBridge));
    }
    else if (m_audioDrmBridge && !m_videoDrmBridge) {
        m_videoDrmBridge = element;
        LOG_GSTPARSE_MESSAGE ("[GSTParse] the 2nd DRMBridge is %s\n", GST_ELEMENT_NAME(m_videoDrmBridge));
    }
}
#endif
// According to http://www.w3.org/TR/encrypted-media/#introduction, graph
// 1) get needkey event with initData(contents specific data)
// 2) generate request
// 3) get license request
// 4) provide license

void GSTParse::needKey(GstElement* element, gpointer initData, guint initDataLength, guint numInitData, gpointer /* userData */)
{
    // 1) needkey
    LOG_MEDIA_MESSAGE("needKey initData = %p, len=%d : %s\n", static_cast<unsigned char *>(initData), initDataLength, GST_ELEMENT_NAME(element));
    if (m_needKeyCallbackFunction) {
        if (!m_needKeyTimerHandler)
            m_needKeyTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(gstParserNeedKeyTimeoutCallback), this);
        m_needKeyCallbackFunction(element, initData, initDataLength, numInitData, m_needkeyCallbackUserData);
    }
    else
        WARN_MEDIA_MESSAGE(" there is no registered needkeyCallback function \n");
}

void GSTParse::appendNeedKeySourceBuffer()
{
    m_needKeyTimerHandler = 0;
    m_sourceBuffer->appendNeedKeySourceBuffer();
}

bool GSTParse::generateKeyRequest(const char* keySystem, unsigned char* initData, unsigned initDataLength, unsigned char** keyRequest, unsigned* keyRequestLength)
{
    LOG_MEDIA_MESSAGE("generateKeyRequest is called! \n");

    if (!m_videoDrmBridge && !m_audioDrmBridge) {
        ERROR_MEDIA_MESSAGE("There is no DRMBridge element...\n");
        return false;
    }

    if (strstr(keySystem, "webkit-org.w3.clearkey")) {
        if (m_videoDrmBridge) {
            g_object_set(m_videoDrmBridge, "drmbridge-drm-type-streaming", drmTypeClearkey, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s\n", drmTypeClearkey, GST_ELEMENT_NAME(m_videoDrmBridge));
            }
        if (m_audioDrmBridge) {
            g_object_set(m_audioDrmBridge, "drmbridge-drm-type-streaming", drmTypeClearkey, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s\n", drmTypeClearkey, GST_ELEMENT_NAME(m_audioDrmBridge));
        }
    }

    else if (strstr(keySystem, "com.youtube.playready")) {

        const unsigned char* pssh = initData;
        unsigned int psshLength = initDataLength;

        unsigned int boxType = 0;
        unsigned int boxLength = 0;
        unsigned int playreadySystemId = 0;
        unsigned int offset = 0;
        unsigned int count = 0; //"pssh" count
        bool isPlayreadyPssh = false;

        unsigned char* playreadyInitData;
        unsigned int playreadyInitDataLength = 0;

        while (offset < psshLength) {
            boxLength = DATA_READ_UINT32_BE(&pssh[offset+0]);
            boxType = DATA_READ_UINT32_BE(&pssh[offset+4]);

            if (boxType != BOX_TYPE_PSSH) {
                isPlayreadyPssh = false;
                break;  //if not pssh box type, return false
            }
            else
                count++;

            playreadySystemId = DATA_READ_UINT32_BE(&pssh[offset+12]);
            if (playreadySystemId == SIMPLE_KEY_PLAYREADY) {
                LOG_MEDIA_MESSAGE("Found playready pssh, SIMPLE_KEY_PLAYREADY = (0x)9A-04-F0-79\n");
                LOG_MEDIA_MESSAGE("Playready Pssh length is (%d) in count #(%d) \n", boxLength, count);

                playreadyInitDataLength = boxLength;
                playreadyInitData = (unsigned char*)malloc(playreadyInitDataLength*sizeof(unsigned char));
                memset(playreadyInitData, 0, playreadyInitDataLength*sizeof(unsigned char));
                memcpy(playreadyInitData, &pssh[offset], playreadyInitDataLength);
                isPlayreadyPssh = true;
                break;
            }
            else {
                LOG_MEDIA_MESSAGE("#(%d)pssh is not playready's, go to next\n", count);
                offset += boxLength;
                isPlayreadyPssh = false;
            }
        }

        if(!isPlayreadyPssh) {
            ERROR_MEDIA_MESSAGE("Not found playready pssh box...\n");
            return false;
        }

        for (unsigned int i=0; i<playreadyInitDataLength; i++) {
            LOG_MEDIA_MESSAGE("playreadyInitData = 0x%02x\n", playreadyInitData[i]);
        }

        if (m_videoDrmBridge) {
            g_object_set(m_videoDrmBridge, "drmbridge-drm-type-streaming", drmTypePlayready, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s\n",drmTypePlayready, GST_ELEMENT_NAME(m_videoDrmBridge));

            // 2) generate request
            g_object_set(m_videoDrmBridge, "drmbridge-playready-challenge-data", playreadyInitData, NULL);
            g_object_set(m_videoDrmBridge, "drmbridge-playready-challenge-len", playreadyInitDataLength, NULL);

            LOG_MEDIA_MESSAGE("generate request is done to %s\n", GST_ELEMENT_NAME(m_videoDrmBridge));

            // 3) get license request
            guchar* getKeyRequest;
            unsigned getKeyRequestLength = 0;
            g_object_get(m_videoDrmBridge, "drmbridge-playready-key-request", &getKeyRequest, NULL);
            g_object_get(m_videoDrmBridge, "drmbridge-playready-key-request-len", &getKeyRequestLength, NULL);

            if (!getKeyRequest || getKeyRequestLength == 0) {
                ERROR_MEDIA_MESSAGE("failed to get license request...\n");
                return false;
            }
            else
                LOG_MEDIA_MESSAGE("getKeyRequest=%p, getKeyRequestLength=%d\n", getKeyRequest, getKeyRequestLength);

            *keyRequestLength = getKeyRequestLength;
            *keyRequest = static_cast<unsigned char*>(malloc(getKeyRequestLength*sizeof(unsigned char)));
            memcpy(*keyRequest, getKeyRequest, getKeyRequestLength);
            LOG_MEDIA_MESSAGE("complete get license request from %s\n", GST_ELEMENT_NAME(m_videoDrmBridge));
        }

        if (m_audioDrmBridge) {
            g_object_set(m_audioDrmBridge, "drmbridge-drm-type-streaming", drmTypePlayready, NULL);
            LOG_MEDIA_MESSAGE("complete set drmbridge-drm-type-streaming as 0x%02x to %s\n", drmTypePlayready, GST_ELEMENT_NAME(m_audioDrmBridge));

            // 2) generate request
            g_object_set(m_audioDrmBridge, "drmbridge-playready-challenge-data", playreadyInitData, NULL);
            g_object_set(m_audioDrmBridge, "drmbridge-playready-challenge-len", playreadyInitDataLength, NULL);
            LOG_MEDIA_MESSAGE("generate request is done to %s\n", GST_ELEMENT_NAME(m_audioDrmBridge));

            // 3) get license request
            guchar* getKeyRequest;
            unsigned getKeyRequestLength = 0;
            g_object_get(m_audioDrmBridge, "drmbridge-playready-key-request", &getKeyRequest, NULL);
            g_object_get(m_audioDrmBridge, "drmbridge-playready-key-request-len", &getKeyRequestLength, NULL);

            if (!getKeyRequest || getKeyRequestLength == 0) {
                LOG_MEDIA_MESSAGE("failed to get license request...\n");
                return false;
            }
            else
                LOG_MEDIA_MESSAGE("getKeyRequest=%p, getKeyRequestLength=%d\n", getKeyRequest, getKeyRequestLength);

            // if already received keyrequest data from another drm element, don't need to get it again (precondition : audio&video are encrypted by same key)
            if (!m_videoDrmBridge) {
                *keyRequestLength = getKeyRequestLength;
                *keyRequest = static_cast<unsigned char*>(malloc(getKeyRequestLength*sizeof(unsigned char)));
                memcpy(*keyRequest, getKeyRequest, getKeyRequestLength);
                LOG_MEDIA_MESSAGE("complete get license request from %s\n", GST_ELEMENT_NAME(m_audioDrmBridge));
            }
        }

        if (playreadyInitData)
            free(playreadyInitData);
    }

    return true;
}

bool GSTParse::updateKey(const char* keySystem, unsigned char* key, unsigned keyLength, unsigned char* /*initData*/, unsigned /*initDataLength*/)
{
    LOG_MEDIA_MESSAGE("updateKey is called!\n");
    LOG_MEDIA_MESSAGE("keysystem %s, key = %p, keylen=%d\n", keySystem, key, keyLength);

    if (!m_videoDrmBridge && !m_audioDrmBridge) {
        ERROR_MEDIA_MESSAGE("There is no DRMBridge element...\n");
        return false;
    }

    // FIXME: property implementation should not contain any structre for parameter.
    LicenseData* licenseData = (LicenseData*)malloc(sizeof(LicenseData));
    licenseData->key = static_cast<unsigned char*>(malloc(keyLength*sizeof(unsigned char)));
    memcpy(licenseData->key, key, keyLength);
    licenseData->keyLength = static_cast<int>(keyLength);
    LOG_MEDIA_MESSAGE("licenseData = %p\n", static_cast<gpointer>(licenseData));

    // 4) Provide license
    if (m_videoDrmBridge) {
        if (strstr(keySystem, "webkit-org.w3.clearkey")) {
                g_object_set(m_videoDrmBridge, "drmbridge-clearkey-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("clearkey provide license is done to %s\n",GST_ELEMENT_NAME(m_videoDrmBridge));
        }
        else if (strstr(keySystem, "com.youtube.playready")) {
                g_object_set(m_videoDrmBridge, "drmbridge-playready-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("playready provide license is done to %s\n",GST_ELEMENT_NAME(m_videoDrmBridge));
        }
    }

    if (m_audioDrmBridge) {
        if (strstr(keySystem, "webkit-org.w3.clearkey")) {
                g_object_set(m_audioDrmBridge, "drmbridge-clearkey-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("clearkey provide license is done to %s\n",GST_ELEMENT_NAME(m_audioDrmBridge));
        }
        else if (strstr(keySystem, "com.youtube.playready")) {
                g_object_set(m_audioDrmBridge, "drmbridge-playready-license-data", static_cast<gpointer>(licenseData), NULL);
                LOG_MEDIA_MESSAGE("playready provide license is done to %s\n",GST_ELEMENT_NAME(m_audioDrmBridge));
        }
    }

    if (licenseData)
        free(licenseData);

    return true;
}
#endif

gboolean
GSTParse::isEBMLheaderPresent(const gchar* data, guint dataLen){
    guint elementId = 0;
    guint dataLength = 0;
    guint elementIdLen = 0;
    if(GST_FLOW_OK != getNextElementID(data,dataLen, &elementId, &dataLength, &elementIdLen))
        return FALSE;
    if(EBML_ID_HEADER == elementId)
    {
        m_Bufoffset = dataLength + elementIdLen;
        LOG_GSTPARSE_MESSAGE("EBMLHeader:[0x%X]\n",elementId);
    }
    return TRUE;
}

gboolean
GSTParse::isEBMLSegmentPresent(const gchar* data, guint dataLen){
    guint elementId = 0;
    guint dataLength = 0;
    guint elementIdLen = 0;
    if(GST_FLOW_OK != getNextElementID(data,dataLen, &elementId, &dataLength, &elementIdLen))
        return FALSE;
    if(EBML_ID_SEGMENT == elementId)
    {
        m_Bufoffset += elementIdLen;
        LOG_GSTPARSE_MESSAGE("SEGMENTHeader:[0x%X]\n",elementId);
        return TRUE;
    }
    else
        return FALSE;
}

GstFlowReturn
GSTParse::getNextElementID(const gchar* buffer,const gint bufferLen, guint *elementId,
guint *dataLength, guint *elementIdLen){

    guint needed = 2;
    guint8 *buf = NULL;
    guint8 *sbuf = NULL;
    gint lengthMask = 0x80, read = 1, n = 1, num_ffs = 0;
    guint64 total = 0;
    GstFlowReturn ret = GST_FLOW_OK;
    /* check buffer length and buffer pointer*/
    if(bufferLen <= 0 || buffer == NULL)
        return GST_FLOW_ERROR;

    /* well ... */
    *elementId = (guint32) EBML_SIZE_UNKNOWN;
    *dataLength = (guint32)EBML_SIZE_UNKNOWN;

    /* read element id */
    sbuf = (guint8*)g_malloc(needed);
    buf = sbuf;
    ret = readBytes (buffer, needed, buf);
    if (ret != GST_FLOW_OK)
      goto peek_error;
    total = (guint64) (GST_READ_UINT8 (buf));
    while (read <= 4 && !(total & lengthMask)) {
        read++;
        lengthMask >>= 1;
    }
    if (G_UNLIKELY (read > 4))
        goto invalid_length;

    /* need id and at least something for subsequent length */
    needed = read + 1;
    sbuf = (guint8*)g_realloc(sbuf,needed);
    buf = sbuf;
    ret = readBytes (buffer, needed, buf);
    if (ret != GST_FLOW_OK)
        goto peek_error;
    while (n < read) {
        total = (total << 8) | GST_READ_UINT8 (buf + n);
        ++n;
    }
    *elementId = (guint32) total;
    /* read element length */
    total = (guint64) GST_READ_UINT8 (buf + n);
    lengthMask = 0x80;
    read = 1;
    while (read <= 8 && !(total & lengthMask)) {
        read++;
        lengthMask >>= 1;
    }
    if (G_UNLIKELY (read > 8))
        goto invalid_length;
    if ((total &= (lengthMask - 1)) == lengthMask - 1)
        num_ffs++;

    needed += read - 1;
    sbuf = (guint8*)g_realloc(sbuf,needed);
    buf = sbuf;
    ret = readBytes (buffer, needed, buf);
    if (ret != GST_FLOW_OK)
        goto peek_error;
    buf += (needed - read);
    n = 1;
    while (n < read) {
        guint8 mByte = GST_READ_UINT8 (buf + n);
        if (G_UNLIKELY (mByte == 0xff))
            num_ffs++;
        total = (total << 8) | mByte;
        ++n;
    }
    MMPARSE_FREEIF(sbuf);
    if (G_UNLIKELY (read == num_ffs))
        *dataLength = (guint32)G_MAXUINT64;
  else
        *dataLength = total;
    *elementIdLen = needed;
    return GST_FLOW_OK;

peek_error:
invalid_length:
    MMPARSE_FREEIF(sbuf);
    return GST_FLOW_ERROR;
}

GstFlowReturn
GSTParse::readBytes(const gchar* sourceBuf, guint length,guint8 *targetBuf){
    guint i;
    for(i = 0; i < length; i++)
        targetBuf[i] = sourceBuf[i];
    return GST_FLOW_OK;
}



#if 0
/* sample main for unit testing */
#define CHUNK_SIZE  4096*1024

void
Result(void* data)
{
    LOG_GSTPARSE_MESSAGE ("[GSTParse] called callback function\n");
}

int
main (int   argc,
    char *argv[])
{
    GSTParse* parse = NULL;//new GSTParse("video/x-h264");
    GstBuffer* outbuffer = NULL;
    FILE * fp = NULL;
    gchar* buf = NULL;
    guint size = 0;
    guint64 read_position = 0;
    GMainLoop *loop = NULL;
    int width = 0;
    int height = 0;
    int extOffset = 0;
    gchar *extension = NULL;

    gboolean have_audio = FALSE;
    gboolean have_video = FALSE;

    gboolean result = FALSE;
    gboolean quit_pushing = FALSE; // @

    gchar* test="test";


    loop = g_main_loop_new (NULL, FALSE);

    LOG_GSTPARSE_MESSAGE ("[GSTParse] opening file : %s\n", argv[1]);

    fp = fopen(argv[1], "rb");

    if(fp==NULL) {
        LOG_GSTPARSE_MESSAGE ("[GSTParse] file not found\n");
        return 1;
    }
    int i = 0;
    for(i= strlen(argv[1]); i>0; --i)
        if(argv[1][i] == '.')
            break;
    i++;
    extension = g_strdup(&argv[1][i]);
    if(g_strcasecmp(extension,"webm") == 0)
        parse = new GSTParse("video/webm",NULL);
    else if(g_strcasecmp(extension,"mp4") == 0)
        parse = new GSTParse("video/mp4",NULL);
    else
    {
        LOG_GSTPARSE_MESSAGE ("Unsuported file format:[%s]",argv[1]);
        fclose(fp);
        return 0;
    }
    parse->setinitsegmentCB(Result, (void*)test);
    int count = 0;
    while(!quit_pushing) {
        size = CHUNK_SIZE;

        buf = (gchar *)malloc(size);
        if(buf==NULL) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse] mem alloc failed\n");
            break;
        }
        memset(buf, 0, size);
        size  = fread(buf, 1, size, fp);

        if(size<=0) {
            LOG_GSTPARSE_MESSAGE ("[GSTParse] EOS\n");
            quit_pushing=TRUE;
            break;
        }
        read_position += size;
        /* for test app src*/
        result = parse->parse(buf, size);
    }

    result = parse->getVideoDimension(&width, &height);
    if (result)
        LOG_GSTPARSE_MESSAGE ("[GSTParse] get width = %d, height = %d\n", width, height);

    parse->getMediaTrackInfo(&have_audio, &have_video);
    LOG_GSTPARSE_MESSAGE ("[GSTParse] get have_audio = %d, have_video = %d\n", have_audio, have_video);


    fclose(fp);

    while(1) {
        outbuffer = parse->getNextVideoFrame();
        if (outbuffer == NULL) {
            continue;
        }
    }

    /* Just local test,, didn't close main */
    g_main_loop_run (loop);

    LOG_GSTPARSE_MESSAGE ("[GSTParse] End of stream\n");

}
#endif

#endif // #if ENABLE(TIZEN_MSE)