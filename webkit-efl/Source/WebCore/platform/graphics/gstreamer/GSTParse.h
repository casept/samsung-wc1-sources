#ifndef GSTParser_h
#define GSTParser_h

#if ENABLE(TIZEN_MSE)
#include <gst/gst.h>

#define MMPARSE_FREEIF(x) \
if ( x ) \
    g_free((gpointer) x ); \
x = NULL;

#define return_if_fail(expr) \
if(!(expr)) { \
    return; \
}

#define return_val_if_fail(expr, val) \
if (!(expr)) { \
    return val; \
}

#define LOG_GSTPARSE_MESSAGE(...) do { \
    TIZEN_LOGD(__VA_ARGS__); \
} while (0)



#define MP4_SUPPORT_ENABLED 1


/**********************************
* For BOX TYPE Define
**********************************/
#define BOX_FTYP    0x66747970      /* 'ftyp' */
#define BOX_MOOV    0x6D6F6F76      /* 'moov' */
#define BOX_MVHD    0x6d766864      /* 'mvhd' */
#define BOX_TRAK    0x7472616B      /* 'trak' */
#define BOX_TKHD    0x746B6864      /* 'tkhd' */
#define BOX_MDIA    0x6D646961      /* 'mdia' */
#define BOX_MDHD    0x6D646864      /* 'mdhd' */

/**********************************
* For Read Data
**********************************/

#define g_uint8     unsigned char
#define g_uint32    unsigned int
#define g_int64     long long
#define g_uint64    unsigned long long

/**********************************
* For Read Data
**********************************/

/* Define PUT and GET functions for unaligned memory */
#define _DATA_GET(__data, __idx, __size, __shift) \
    (((g_uint##__size) (((const g_uint8 *) (__data))[__idx])) << (__shift))

/* Read an 8 bit unsigned integer value from the memory buffer */
#define DATA_READ_UINT8(data)   (_DATA_GET (data, 0,  8,  0))

/* Read a 16 bit unsigned integer value in big endian format from the memory buffer */
#define DATA_READ_UINT16_BE(data)   (_DATA_GET (data, 0, 32, 8) | \
                                    _DATA_GET (data, 1, 32, 0))

/* Read a 32 bit unsigned integer value in big endian format from the memory buffer */
#define DATA_READ_UINT32_BE(data)   (_DATA_GET (data, 0, 32, 24) | \
                                    _DATA_GET (data, 1, 32, 16) | \
                                    _DATA_GET (data, 2, 32,  8) | \
                                    _DATA_GET (data, 3, 32,  0))

/* Read a 64 bit unsigned integer value in big endian format from the memory buffer */
#define DATA_READ_UINT64_BE(data)   (_DATA_GET (data, 0, 64, 56) | \
                                    _DATA_GET (data, 1, 64, 48) | \
                                    _DATA_GET (data, 2, 64, 40) | \
                                    _DATA_GET (data, 3, 64, 32) | \
                                    _DATA_GET (data, 4, 64, 24) | \
                                    _DATA_GET (data, 5, 64, 16) | \
                                    _DATA_GET (data, 6, 64,  8) | \
                                    _DATA_GET (data, 7, 64,  0))


#define MAKE_TAG_BE(a,b,c,d) ((d) | ((c) << 8) | ((b) << 16) | ((a) << 24))

#if 0   /* Not yet */
typedef enum __GST_BOX_STATE {
    BOX_STATE_NONE,
    BOX_STATE_INITIAL,          /* Initial state (haven't got the header yet) */
    BOX_STATE_HEADER,           /* Parsing the header */
    BOX_STATE_MOVIE,            /* Parsing/Playing the media data */
    BOX_STATE_INVALID,          /* invalid and incomplete data  */
    BOX_STATE_ERROR             /* error case */
}GST_BOX_STATE;
#endif

typedef enum __GST_PARSE_STATE {
    /* common */
    PARSE_STATE_NONE,
    PARSE_STATE_INIT_SEGMENT,           /* ftyp - (...) - moov */
    PARSE_STATE_MEDIA_SEGMENT,          /* styp - (...) - moof - mdat */
    PARSE_STATE_INVALID,                /* invalid data  */
    PARSE_STATE_ERROR,                  /* error case */
    /* incomplete case */
    PARSE_STATE_INIT_INCOMPLETE,        /* moov is incomplete (or truncated?) */
    PARSE_STATE_MEDIA_INCOMPLETE,       /* mdat is incomplete (or truncated?) */
    PARSE_STATE_WAIT_SEGMENT,           /* need continuous (init or media) stream */
    /* exceptional case */
    PARSE_STATE_INIT_MEDIA_SEGMENT,     /*   complete : (init segment) + (media segment) */
    PARSE_STATE_INIT_MEDIA_INCOMPLETE   /* incomplete : (init segment) + (media segment)*/
    /* added more case */
}GST_PARSE_STATE;

typedef enum __GST_MIME_TYPE{
    MIME_TYPE_NONE = -1,
    MIME_TYPE_MP4_AUDIO = 0,
    MIME_TYPE_MP4_VIDEO,
    MIME_TYPE_WEBM_AUDIO,
    MIME_TYPE_WEBM_VIDEO,
    MIME_TYPE_END
}GST_MIME_TYPE;


/**********************************
* For EBML ID.
**********************************/

#define EBML_SUPPORT_ENABLED 1

#if EBML_SUPPORT_ENABLED
/* EBML File-IDs */
#define EBML_ID_HEADER              0x1A45DFA3

/* EBML HEADER  IDs master */
#define EBML_ID_EBMLVERSION         0x4286
#define EBML_ID_EBMLREADVERSION     0x42F7
#define EBML_ID_EBMLMAXIDLENGTH     0x42F2
#define EBML_ID_EBMLMAXSIZELENGTH   0x42F3
#define EBML_ID_DOCTYPE             0x4282
#define EBML_ID_DOCTYPEVERSION      0x4287
#define EBML_ID_DOCTYPEREADVERSION  0x4285

/* IDs in Segment */
#define EBML_ID_SEGMENT         0x18538067
#define EBML_ID_INFO            0x1549A966
#define EBML_ID_TRACKS          0x1654AE6B

/*IDs inside Tracks segment */
#define EBML_ID_TRACK_ENTRY     0xAE
#define EBML_ID_TRACK_NUMBER    0xD7
#define EBML_ID_TRACK_UID       0x73C5
#define EBML_ID_TRACK_TYPE      0x83
#define EBML_ID_AUDIO_TRACK     0xE1
#define EBML_ID_VIDEO_TRACK     0xE0

/* IDs inside Info segment */
#define EBML_ID_TIMECODESCALE    0x2AD7B1
#define EBML_ID_DURATION         0x4489

/* IDs inside VideoTrack Master */
#define EBML_ID_VIDEODISPLAYWIDTH   0x54B0
#define EBML_ID_VIDEODISPLAYHEIGHT  0x54BA
#define EBML_ID_VIDEODISPLAYUNIT    0x54B2
#define EBML_ID_VIDEOPIXELWIDTH     0xB0
#define EBML_ID_VIDEOPIXELHEIGHT    0xBA

#define EBML_SIZE_UNKNOWN           G_GINT64_CONSTANT(0x00ffffffffffffff)

#endif

typedef enum __GST_PARSE_END_TYPE{
    PARSE_END_TYPE_NONE = 0,
    PARSE_END_TYPE_EOS,
    PARSE_END_TYPE_UNDERRUN,
    PARSE_END_TYPE_NUM
}GST_PARSE_END_TYPE;


#define ENABLE_EME 1
#define USE_EOS 1
#define UNDERRUN 1

typedef void (*initsegmentCallBack)(void* data);  /* initsegment callback defined */
typedef void (*needKeyCallback)(GstElement* element, gpointer initData, guint initDataLength, guint numInitData, gpointer userData);
#if USE_EOS
typedef gboolean (*parseCompleteCallback)(void* data);
#endif

namespace WebCore {
class SourceBufferPrivateGStreamer;
}

using namespace WebCore;

class GSTParse {

public:
    GSTParse();
    GSTParse(const gchar* mimetype, SourceBufferPrivateGStreamer* sourceBuffer = NULL);
    ~GSTParse();

    gboolean parse(const gchar* data, guint len);

    /* TODO : define a enum for av_type */
    GstBuffer* getNextAudioFrame(void);
    GstBuffer* getNextVideoFrame(void);
    GstBuffer* getNextFrame(gboolean isVideo);

    /* some internal stuffs */
    void addBuffer(GstBuffer* buff);

    /* TODO : convert to callback */
    gboolean getVideoDimension(gint* width, gint* height);
    void getMediaTrackInfo(gboolean* haveAudio, gboolean* haveVideo);
    void getMediaTimescale(gboolean isVideo, guint* timescale, guint64* duration);

#if ENABLE_EME
    void getDrmElement(GstElement* element);
    void needKey(GstElement*, gpointer, guint, guint, gpointer);
    void appendNeedKeySourceBuffer();
    bool generateKeyRequest(const char*, unsigned char*, unsigned, unsigned char**, unsigned*);
    bool updateKey(const char*, unsigned char*, unsigned, unsigned char*, unsigned);
#endif

    /* init segment callback */
    void setinitsegmentCB(initsegmentCallBack cbFunction, void* data);
    void initsegmentCB(void* data);

    void setNeedKeyCB(needKeyCallback needkeyCallbackFunction, gpointer userData);

#if USE_EOS
    void setParseCompleteCB(parseCompleteCallback funct, void* data);
    void parseCompleteCB(void);
    gboolean isSetParseCompleteCB(void);  /* temp */
#endif

    bool isInitSegment() { return m_Isinitseg; }
    void notifyBufferAdded();

    void parseComplete();
private:

    /* TODO : modify to m_Aaaa  */
    guint m_Bus_watch_id;

    GstElement* m_Pipeline;
    GstElement* m_Source;
    GstElement* m_Demuxer;
    GstElement* m_Audio_sink;
    GstElement* m_Video_sink;

    GList* m_Audio_frames;
    GList* m_Video_frames;

    gboolean m_Isinitseg;
    gboolean m_Haveinitseg;
    guint m_Bufoffset;

    gboolean m_isLinkedAudioSink; /* Fix TC Number 38 and 39 */
    gboolean m_isLinkedVideoSink;

    GMutex* m_Mutex;
#if USE_EOS
    GMutex* m_Lock;
    GCond* m_Cond;

    GThread* m_ev_thread;
    GMutex* m_ev_Lock;
    GCond* m_ev_Cond;
    gboolean m_ev_thread_exit;
#endif

    guint m_parseState;
    guint m_initSegmentLen;
    guint m_mediaSegmentLen;
    guint m_moovBoxPosition;

    gboolean m_have_videotrack;
    gboolean m_have_audiotrack;
    gint m_have_videotrack_id;    /* Video track ID(#) */
    gint m_have_audiotrack_id;    /* Audio track ID(#) */

    /* from box : moov - mvhd */
    guint m_next_track_ID;        /* Max Track number */
    guint m_track_count;          /* check to find track number */

    /* from box : moov - trak - tkhd  */
    gint m_Width;
    gint m_Height;

    /* from box : moov - trak - mdia - mdhd  */
    guint m_video_timescale;
    guint m_audio_timescale;
    guint64 m_video_duration;
    guint64 m_audio_duration;

    gchar* m_Mimetype;
    guint m_eMimetype;

    initsegmentCallBack m_pCBinitsegment;
    void* m_CBinitsegmentData;

    unsigned long m_bufferAddedTimerHandler;
    unsigned long m_parseCompleteTimerHandler;
    SourceBufferPrivateGStreamer* m_sourceBuffer;

#if ENABLE_EME
    GstElement* m_videoDrmBridge;
    GstElement* m_audioDrmBridge;
    needKeyCallback m_needKeyCallbackFunction;
    gpointer m_needkeyCallbackUserData;
    unsigned long m_needKeyTimerHandler;
#endif
    guint64 m_have_videotrack_id_webm;    /* WebM Video TrackUID(#) */
    guint64 m_have_audiotrack_id_webm;    /* Webm Audio TrackUID(#) */
    guint m_have_track_type_webm; /* Track type audio or video */
    guint m_have_track_number_webm; /* Track Number */
    guint m_timescale_webm;
    double m_duration_webm;   /* 4/8/10 byte long float value */

    gulong m_padAddedHandler;
    gulong m_videoNeedkeyHandler;
    gulong m_audioNeedkeyHandler;
    gulong m_videoHandOffHandler;
    gulong m_audioHandOffHandler;
#if UNDERRUN
    gulong m_underrunHandler;
#endif
#if USE_EOS
    parseCompleteCallback m_parseCompleteCB;
    void* m_parseCompleteData;

    gboolean m_is_eos;
    gulong m_sourceEventProbeHandler;
    gulong m_audioSinkEventProbeHandler;
    gulong m_videoSinkEventProbeHandler;
    gint m_parseEndType;
#endif

private:
    /* hand-off hander callbac */
    static void buffer_add (GstElement *element, GstBuffer *buffer, GstPad *pad, gpointer data);

    /* pad-added hander callback */
    static void dynamic_pad (GstElement *element, GstPad *pad, gpointer data);

    /* bus message callback */
    static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data);
#if USE_EOS
    static gpointer eos_ev_thread(gpointer data);
    static gboolean src_event_probe_cb(GstPad * pad, GstEvent * info, gpointer data);
    static gboolean sink_event_probe_cb(GstPad * pad, GstEvent * info, gpointer data);
#endif
#if UNDERRUN
    static void underrunCb(GstElement *element, gpointer data);
#endif
    gboolean init_gstreamer(void);
    gboolean createPipeline(void);
    void disconnect_signal(void);
    void destroy_pipeline(void);

    gboolean metaDataExtract_WebM(const gchar* data, guint len);
    gboolean metaDataExtract_MP4(const gchar* data, guint len);
    gboolean metaDataExtract(const gchar* data, guint len);
    gboolean initSegmentChecker(const gchar* data, guint len);

    gboolean havePipeline();
    gboolean haveMimetype();

#ifdef MP4_SUPPORT_ENABLED
    gboolean extractBoxtypeLength(const gchar* data, guint len, guint* ptype, guint* psize);
    gboolean simpleParseBoxTkhd(const gchar* boxdata, guint len);
    gboolean simpleParseBoxMdia(const gchar* boxdata, guint len);
    gboolean simpleParseBoxTrak(const gchar* boxdata, guint len);
    gboolean simpleParseBoxMvhd(const gchar* boxdata, guint len);
    gboolean simpleParseBoxMoovNode(const gchar* boxdata, guint len, guint node_type);
    gboolean simpleParseBoxFtyp(const gchar* data, guint len, guint box_size);
    gboolean isMp4InitSegment(const gchar* data, guint len);
#endif

#if EBML_SUPPORT_ENABLED
    gboolean isEBMLheaderPresent(const gchar* data, guint dataLen);
    gboolean isEBMLSegmentPresent(const gchar* data, guint dataLen);
    GstFlowReturn getNextElementID(const gchar* buffer,const gint bufferLen, guint *elementId, guint *dataLength, guint *elementIdLen);
    GstFlowReturn readBytes(const gchar* sourceBuf, guint length,guint8 *targetBuf);
    gboolean parseTrackInfo_WebM(const gchar* data, guint len);
    guint64 readVarInt_Max64_BE(const char *data, guint lenBytes);
    double readVarFloat_MaxDouble(const char *data, guint dataLen);
    float intBitsToFloat(guint32 value);
    double intBitsToDouble(guint64 value);
#endif
};

#endif // #if ENABLE(TIZEN_MSE)

#endif // GSTParser_h