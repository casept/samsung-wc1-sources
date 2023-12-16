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

#ifndef SourceBufferPrivateGStreamer_h
#define SourceBufferPrivateGStreamer_h

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "SourceBufferPrivate.h"
#include "WebKitMediaSourceGStreamer.h"

#if ENABLE(TIZEN_MSE)
typedef struct _GstBuffer GstBuffer;
class GSTParse;
#endif

namespace WebCore {

#define TIZEN_MSE_FRAME_SOURCEBUFFER_ABORT_WORKAROUND 1

#if ENABLE(TIZEN_MSE)
class Box;
class MoovBox;
class MoofBox;
class MdatBox;
#endif

class SourceBufferPrivateGStreamer FINAL : public SourceBufferPrivate {
public:
    SourceBufferPrivateGStreamer(PassRefPtr<MediaSourceClientGstreamer>, const ContentType&);
    ~SourceBufferPrivateGStreamer();

#if ENABLE(TIZEN_MSE)
    void setClient(SourceBufferPrivateClient* newClient) { sourceBufferPrivateClient = newClient; }
    void sendData();
    void bufferAdded();
#else
    void setClient(SourceBufferPrivateClient*) { }
#endif
    AppendResult append(const unsigned char*, unsigned);
    void abort();
    void removedFromMediaSource();
    MediaPlayer::ReadyState readyState() const { return m_readyState; }
    void setReadyState(MediaPlayer::ReadyState readyState) { m_readyState = readyState; }
    void evictCodedFrames() { }
    bool isFull() { return false; }
#if ENABLE(TIZEN_MSE)
    void prepareSeeking(const MediaTime&);
    void didSeek(const MediaTime&);
    void parseComplete();
#endif
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    const String getMimeType() { return m_type; }
    PassRefPtr<Uint8Array> generateKeyRequest(const String&, Uint8Array*);
    bool updateKey(const String&, Uint8Array* key, Uint8Array*);
    void appendNeedKeySourceBuffer();
#endif

private:
#if ENABLE(TIZEN_MSE)
    void handlingBox(PassRefPtr<Box>);
    void didReceiveInitializationSegment();
    void emitSamples(const MdatBox*, unsigned long long);
    void emitSample(GstBuffer*);

    unsigned long long int getTrackBaseDecodeTimeOrZero(unsigned) const;
    unsigned getTrackTimescale(unsigned) const;
    unsigned getTrackMediaType(unsigned) const;
    unsigned long long getMovieTimescale() const;
    unsigned long long getFragmentedMovieDurationOrZero() const;
    void didReceiveSample();

    unsigned long long int getBaseDecodeTimeOrZero() const;
    unsigned long long int getSamplesInFragmentTotalDuration() const;

    virtual void enqueueSample(PassRefPtr<MediaSample>, AtomicString) OVERRIDE;
    virtual bool isReadyForMoreSamples(AtomicString) OVERRIDE { return true; }
#endif
    String m_type;
    RefPtr<MediaSourceClientGstreamer> m_client;
    MediaPlayer::ReadyState m_readyState;

#if ENABLE(TIZEN_MSE)
    SourceBufferPrivateClient* sourceBufferPrivateClient;
    bool m_isMp4;
    bool m_isWebM;
    RefPtr<MoovBox> m_moov;
    RefPtr<MoofBox> m_moof;

    std::vector<unsigned char> m_prevData;
    unsigned long long m_needDataLength;

    GSTParse* m_parser;
#endif
};

}

#endif
#endif
