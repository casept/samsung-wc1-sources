/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013 Orange
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SourceBufferPrivateGStreamer.h"

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "ContentType.h"

#if ENABLE(TIZEN_MSE)
#include "GSTParse.h"
#include "MP4Parser.h"
#include "MediaDescription.h"
#include "MediaSample.h"
#include "SourceBufferPrivateClient.h"
#include <limits>
#include <wtf/text/CString.h>
#endif
#include "NotImplemented.h"

namespace WebCore {

#if ENABLE(TIZEN_MSE)
class MediaSampleGStreamer FINAL : public MediaSample {
public:
    static PassRefPtr<MediaSampleGStreamer> create(MediaTime pts, MediaTime dts, MediaTime dur, bool isSync, RefPtr<MoovBox> moov, RefPtr<MoofBox> moof, PassOwnPtr<Vector<unsigned char>> data)
    {
        return adoptRef(new MediaSampleGStreamer(pts, dts, dur, isSync, moov, moof, data));
    }

    static PassRefPtr<MediaSampleGStreamer> create(MediaTime pts, MediaTime dts, MediaTime dur, bool isSync, GstBuffer* buffer)
    {
        return adoptRef(new MediaSampleGStreamer(pts, dts, dur, isSync, buffer));
    }

    virtual ~MediaSampleGStreamer()
    {
        if (m_buffer)
            gst_buffer_unref(m_buffer);
    }

    virtual MediaTime presentationTime() const OVERRIDE { return m_presentationTimestamp; }
    virtual MediaTime decodeTime() const OVERRIDE { return m_decodeTimestamp; }
    virtual MediaTime duration() const OVERRIDE { return m_duration; }
    virtual AtomicString trackID() const OVERRIDE { return AtomicString(""); }
    virtual void addTimestamp(const MediaTime& timestamp) OVERRIDE
    {
        m_presentationTimestamp += timestamp;
        m_decodeTimestamp += timestamp;
        m_buffer = gst_buffer_make_metadata_writable(m_buffer);
        const long long unsigned kNanosecond = 1000LL * 1000LL * 1000LL;
        GST_BUFFER_TIMESTAMP(m_buffer) += timestamp.toDouble() * kNanosecond;
    }

    // purpose of this part is yet to be determined
    virtual SampleFlags flags() const OVERRIDE { return isSyncSample ? IsSync : None; }
    virtual PlatformSample platformSample() OVERRIDE
    {
        PlatformSample platformSample = { PlatformSample::GstBufferType, { 0 } };
        platformSample.sample.gstBuffer = m_buffer;
        return platformSample;
    }
    virtual const size_t size() const OVERRIDE { return GST_BUFFER_SIZE(m_buffer); }

protected:
    MediaSampleGStreamer(MediaTime pts, MediaTime dts, MediaTime dur, bool isSync, RefPtr<MoovBox> moov, RefPtr<MoofBox> moof, PassOwnPtr<Vector<unsigned char>> data)
        : m_presentationTimestamp(pts)
        , m_decodeTimestamp(dts)
        , m_duration(dur)
        , isSyncSample(isSync)
        , m_moov(moov)
        , m_moof(moof)
        , m_rawData(data)
        , m_buffer(0)
    {
    }

    MediaSampleGStreamer(MediaTime pts, MediaTime dts, MediaTime dur, bool isSync, GstBuffer* buffer)
        : m_presentationTimestamp(pts)
        , m_decodeTimestamp(dts)
        , m_duration(dur)
        , isSyncSample(isSync)
        , m_buffer(buffer)
    {
    }

    MediaTime m_presentationTimestamp;
    MediaTime m_decodeTimestamp;
    MediaTime m_duration;
    bool isSyncSample;
    RefPtr<MoovBox> m_moov;
    RefPtr<MoofBox> m_moof;
    OwnPtr<Vector<unsigned char>> m_rawData;
    GstBuffer* m_buffer;
};

class SimpleMediaDescription FINAL : public MediaDescription {
public:
    static RefPtr<SimpleMediaDescription> create(String codec, int mediaType) { return adoptRef(new SimpleMediaDescription(codec, mediaType)); }

    virtual AtomicString codec() const OVERRIDE { return m_string; }
    virtual bool isVideo() const OVERRIDE { return m_type & 1; }
    virtual bool isAudio() const OVERRIDE { return m_type & 2; }
    virtual bool isText()  const OVERRIDE { return m_type & 4; }

protected:
    SimpleMediaDescription(String codec, int mediaType)
        : m_string(codec)
        , m_type(mediaType)
    {
    }
    String m_string;
    int m_type;
};
#endif

SourceBufferPrivateGStreamer::SourceBufferPrivateGStreamer(PassRefPtr<MediaSourceClientGstreamer> client, const ContentType& contentType)
    : m_readyState(MediaPlayer::HaveNothing)
#if ENABLE(TIZEN_MSE)
    , m_needDataLength(0)
#endif
{
    m_client = client;
    m_type = contentType.type();
#if ENABLE(TIZEN_MSE)
    m_client->didAddSourceBuffer(m_type);
    m_isMp4 = (equalIgnoringCase(m_type, "audio/mp4") || equalIgnoringCase(m_type, "video/mp4"));
    m_isWebM = (equalIgnoringCase(m_type, "audio/webm") || equalIgnoringCase(m_type, "video/webm"));
    m_client->setBuffer(this, m_type);
    m_parser = new GSTParse(reinterpret_cast_ptr<const gchar*>(m_type.ascii().data()), this);
    m_parser->setNeedKeyCB(webKitMediaSrcNeedKeyCb, m_client->getWebKitMediaSrc());
#endif
}

SourceBufferPrivateGStreamer::~SourceBufferPrivateGStreamer()
{
#if ENABLE(TIZEN_MSE)
    m_client->setBuffer(NULL, m_type);
    m_parser->setNeedKeyCB(NULL, NULL);
    delete m_parser;
#endif // ENABLE(TIZEN_MSE)
}

void SourceBufferPrivateGStreamer::handlingBox(PassRefPtr<Box> box)
{
    switch (box->type()) {
    case BoxTypeAsIntT<'m', 'o', 'o', 'v'>::Val:
        m_moov = adoptRef(static_cast<MoovBox*>(box.leakRef()));
        didReceiveInitializationSegment();
        break;
    case BoxTypeAsIntT<'m', 'o', 'o', 'f'>::Val:
        m_moof = adoptRef(static_cast<MoofBox*>(box.leakRef()));
        break;
    case BoxTypeAsIntT<'m', 'd', 'a', 't'>::Val:
        emitSamples(static_cast<MdatBox*>(box.get()), box->size() - MP4BoxHeaderSize);
        break;
    default:
        break;
    }
}

#if ENABLE(TIZEN_MSE)
void SourceBufferPrivateGStreamer::parseComplete()
{
    // 1. Init Segment
    if (m_parser->isInitSegment()) {
        SourceBufferPrivateClient::InitializationSegment segment;
        segment.duration = MediaTime::positiveInfiniteTime();
        if (!m_parser->getVideoDimension((int*)&segment.m_width, (int*)&segment.m_height)) {
            segment.m_width = 0;
            segment.m_height = 0;
        }

        // FIXME : Need to check type.
        if (m_type.startsWith("video")) {
            SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
            info.track = VideoTrackPrivate::create();
            info.description = SimpleMediaDescription::create("", 1); // FIXME: for future usage need proper codec name?
            segment.videoTracks.append(info);
        } else {
            SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
            info.track = AudioTrackPrivate::create();
            info.description = SimpleMediaDescription::create("", 2); // FIXME: for future usage need proper codec name?
            segment.audioTracks.append(info);
        }
        sourceBufferPrivateClient->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
    }

    // 2. Media Segment
    bufferAdded();

    ASSERT(m_client);
    m_client->dataReady(m_type);

    sourceBufferPrivateClient->parseComplete();
}
#endif

SourceBufferPrivate::AppendResult SourceBufferPrivateGStreamer::append(const unsigned char* data, unsigned length)
{
#if ENABLE(TIZEN_MSE)
    if (m_isMp4 || m_isWebM) {
        if (m_parser->parse(reinterpret_cast_ptr<const char*>(data), length))
            return AppendSucceeded;
    }
    return ParsingFailed;
#else
    AppendResult result = AppendSucceeded;
    ASSERT(m_client);
    // TODO: Remove didReceiveData when sendData will be implemented.
    m_client->didReceiveData(reinterpret_cast_ptr<const char*>(data), length, m_type);
#endif
}

#if ENABLE(TIZEN_MSE)

void SourceBufferPrivateGStreamer::prepareSeeking(const MediaTime& mediaTime)
{
    m_client->prepareSeeking(mediaTime,m_type);
}

void SourceBufferPrivateGStreamer::didSeek(const MediaTime& mediaTime)
{
    m_client->didSeek(mediaTime,m_type);
}

void SourceBufferPrivateGStreamer::bufferAdded()
{
    bool isVideo = m_type.startsWith("video");
    GstBuffer* buffer = 0;
    while (buffer = m_parser->getNextFrame(isVideo))
        emitSample(buffer);
}

void SourceBufferPrivateGStreamer::sendData()
{
    ASSERT(m_client);

    // TODO: implement here sending data from MediaSampleMap.
    // m_client->didReceiveData(data, data.size(), m_type);
}

void SourceBufferPrivateGStreamer::didReceiveInitializationSegment()
{
    if (!sourceBufferPrivateClient)
        return;

    ASSERT(m_moov);
    ASSERT(m_moov->m_trak.size());

    unsigned type = getTrackMediaType(0);

    SourceBufferPrivateClient::InitializationSegment segment;

    unsigned long long duration = getFragmentedMovieDurationOrZero();
    unsigned long long timescale = getMovieTimescale();
    segment.duration = (duration && timescale) ? MediaTime(duration, timescale) : MediaTime::positiveInfiniteTime();

    if (type == BoxTypeAsIntT<'v', 'i', 'd', 'e'>::Val) {
        SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
        info.track = VideoTrackPrivate::create();
        info.description = SimpleMediaDescription::create("", 1); // FIXME: for future usage need proper codec name?
        segment.videoTracks.append(info);
    } else if (type == BoxTypeAsIntT<'s', 'o', 'u', 'n'>::Val) {
        SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
        info.track = AudioTrackPrivate::create();
        info.description = SimpleMediaDescription::create("", 2); // FIXME: for future usage need proper codec name?
        segment.audioTracks.append(info);
    }

    sourceBufferPrivateClient->sourceBufferPrivateDidReceiveInitializationSegment(this, segment);
}

unsigned long long int SourceBufferPrivateGStreamer::getTrackBaseDecodeTimeOrZero(unsigned trackID) const
{
    if (!m_moof || !(trackID < m_moof->m_traf.size()) || !(m_moof->m_traf[trackID]->m_tfdt))
        return 0;

    return m_moof->m_traf[trackID]->m_tfdt->m_baseMediaDecodeTime;
}

void SourceBufferPrivateGStreamer::emitSample(GstBuffer* buffer)
{
    if (!sourceBufferPrivateClient)
        return;

    // FIXME: need to set proper timescale.
    const unsigned timescale = 1000 * 1000;
    MediaTime pts = MediaTime(GST_BUFFER_TIMESTAMP(buffer) / 1000, timescale);
    MediaTime dts = pts; // FIXME: need to include proper offset if exist
    MediaTime duration = MediaTime(GST_BUFFER_DURATION(buffer) / 1000, timescale);

    bool isSyncSample = !GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    sourceBufferPrivateClient->sourceBufferPrivateDidReceiveSample(this, MediaSampleGStreamer::create(pts, dts, duration, isSyncSample, buffer));
}

void SourceBufferPrivateGStreamer::emitSamples(const MdatBox* mdat, unsigned long long mdat_bytes)
{
    if (!sourceBufferPrivateClient)
        return;

    if (!m_moof || !m_moov || !mdat_bytes) {
        ASSERT_WITH_MESSAGE(m_moov, "Should be used with media fragment, need moof box.");
        ASSERT_WITH_MESSAGE(m_moof, "Should be used after initialization segment, need moov box.");
        return;
    }

    // FIXME: need to handle multiple tracks
    const unsigned timescale = getTrackTimescale(0);

    MediaTime pts = MediaTime(getTrackBaseDecodeTimeOrZero(0), timescale);
    MediaTime dts = pts; // FIXME: need to include proper offset if exist

    const Vector<RefPtr<TrafBox>> & traf = m_moof->m_traf;
    if (traf.size()) {
        ASSERT(traf.size() == 1); //  other cases need to be rethinked
        ASSERT(traf[0]->m_trun.size() == 1);

        TfhdBox* tfhd = traf[0]->m_tfhd.get();
        TrunBox& trun = *(traf[0]->m_trun[0]);

        if (!trun.populated())
            trun.populateDefaultsForSamples(m_moov.get(), tfhd);

        unsigned long long leftData = mdat_bytes;

        const unsigned char* mdat_data = mdat->getInternalData() + MP4BoxHeaderSize;

        Vector<OwnPtr<Vector<unsigned char>>> SampleAuxiliaryInformation;

        if (traf[0]->m_saiz.size()) {
            SaizBox* saiz = traf[0]->m_saiz[0].get();

            if (leftData < saiz->m_totalSize) // partial information gives us nothing
                return;

            for (unsigned sample_it = 0; sample_it < saiz->m_sample_count; sample_it++) {
                unsigned char current_sample_size = saiz->m_sample_info_size[sample_it];
                SampleAuxiliaryInformation.append(adoptPtr(new Vector<unsigned char>()));
                SampleAuxiliaryInformation.last()->append(mdat_data, current_sample_size);
                mdat_data += current_sample_size;
            }
            leftData -= saiz->m_totalSize;
            //mdat_data += saiz->m_totalSize;
        }

        for (unsigned i = 0; i < trun.m_sampleCount; i++) {
            if (leftData < trun.m_samples[i].m_sampleSize )
                return;

            // emit sample
            MediaTime duration = MediaTime(trun.m_samples[i].m_sampleDuration, timescale);
            const unsigned kSampleIsDifferenceSampleFlagMask = 0x10000;
            bool isSyncSample = !(trun.m_samples[i].m_sampleFlags & kSampleIsDifferenceSampleFlagMask);
            PassOwnPtr<Vector<unsigned char>> sampleData = adoptPtr(new Vector<unsigned char>());
            sampleData->append(mdat_data, trun.m_samples[i].m_sampleSize);
            sourceBufferPrivateClient->sourceBufferPrivateDidReceiveSample(this, MediaSampleGStreamer::create(pts, dts, duration, isSyncSample, m_moov, m_moof, sampleData));

            // increase next sample offsets
            leftData -= trun.m_samples[i].m_sampleSize;
            mdat_data += trun.m_samples[i].m_sampleSize;
            pts += duration;
            dts = pts;
        }
    }
}

unsigned SourceBufferPrivateGStreamer::getTrackTimescale(unsigned trackID) const
{
    if (!m_moov || !(trackID < m_moov->m_trak.size()))
        return 0;

    return m_moov->m_trak[trackID]->m_mdia->m_mdhd->timeScale();
}

unsigned SourceBufferPrivateGStreamer::getTrackMediaType(unsigned trackID) const
{
    if (!m_moov || !(trackID < m_moov->m_trak.size()))
        return 0;

    return m_moov->m_trak[trackID]->m_mdia->m_hdlr->handlerType();
}

unsigned long long SourceBufferPrivateGStreamer::getMovieTimescale() const
{
    if (!m_moov || !(m_moov->m_mvhd))
        return 0;

    return m_moov->m_mvhd->timescale();
}

unsigned long long SourceBufferPrivateGStreamer::getFragmentedMovieDurationOrZero() const
{
   if (!m_moov || !(m_moov->m_mvex) || !(m_moov->m_mvex->m_mehd))
        return 0;

    return m_moov->m_mvex->m_mehd->fragmentDuration();
}

void SourceBufferPrivateGStreamer::enqueueSample(PassRefPtr<MediaSample> sample, AtomicString)
{
    if (!m_client || !sample)
        return;

    m_client->didReceiveData(sample->platformSample().sample.gstBuffer, m_type);
}
#endif

void SourceBufferPrivateGStreamer::abort()
{
#if ENABLE(TIZEN_MSE)
    m_prevData.clear();
    m_needDataLength = 0;
#if TIZEN_MSE_FRAME_SOURCEBUFFER_ABORT_WORKAROUND
    if (m_parser) {
        m_parser->setNeedKeyCB(NULL, NULL);
        delete m_parser;
        m_parser = new GSTParse(reinterpret_cast_ptr<const gchar*>(m_type.ascii().data()), this);
        m_parser->setNeedKeyCB(webKitMediaSrcNeedKeyCb, m_client->getWebKitMediaSrc());
    }
#endif
#endif // ENABLE(TIZEN_MSE)
    notImplemented();
}

void SourceBufferPrivateGStreamer::removedFromMediaSource()
{
    notImplemented();
}

}

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
void SourceBufferPrivateGStreamer::appendNeedKeySourceBuffer()
{
    sourceBufferPrivateClient->appendNeedKeySourceBuffer();
}

PassRefPtr<Uint8Array> SourceBufferPrivateGStreamer::generateKeyRequest(const String& keySystem, Uint8Array* initData)
{
    unsigned char* keyRequest = 0;
    unsigned keyLength = 0;
    RefPtr<Uint8Array> outKeyRequest;
    if (m_parser->generateKeyRequest(reinterpret_cast<const char*>(keySystem.characters8()),
                initData->data(), initData->length(), &keyRequest, &keyLength)) {
        if (keySystem.contains("org.w3.clearkey",false))
            outKeyRequest = Uint8Array::create(initData->data(), initData->length());
        else
            outKeyRequest = Uint8Array::create(keyRequest, keyLength);
        if (keyRequest)
            free(keyRequest);
    } else {
        outKeyRequest = Uint8Array::create(NULL, 0);
    }
    return outKeyRequest.release();
}

bool SourceBufferPrivateGStreamer::updateKey(const String& keySystem, Uint8Array* key, Uint8Array* initData)
{
    return m_parser->updateKey(reinterpret_cast<const char*>(keySystem.characters8()),
                key->data(), key->length(),
                initData->data(), initData->length());
}
#endif

#endif
