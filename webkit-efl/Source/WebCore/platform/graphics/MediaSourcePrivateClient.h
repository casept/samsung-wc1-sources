/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MediaSourcePrivateClient_h
#define MediaSourcePrivateClient_h

#if ENABLE(MEDIA_SOURCE)

#include "TimeRanges.h"
#if ENABLE(TIZEN_MSE)
#include "MediaPlayer.h"
#endif
#include <wtf/RefCounted.h>

namespace WebCore {

#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
class SourceBufferPrivate;
#endif

class MediaSourcePrivate;

class MediaSourcePrivateClient : public RefCounted<MediaSourcePrivateClient> {
public:
    virtual ~MediaSourcePrivateClient() { }

    virtual void setPrivateAndOpen(PassRefPtr<MediaSourcePrivate>) = 0;
    virtual double duration() const = 0;
    virtual PassRefPtr<TimeRanges> buffered() const = 0;
#if ENABLE(TIZEN_MSE)
    virtual bool buffered(const MediaTime&) = 0;
    virtual void seekToTime(const MediaTime&) = 0;
    virtual MediaTime seekToTime(const MediaTime&, const MediaTime&, const MediaTime&) = 0;
    virtual MediaPlayer::ReadyState mediasourceReadyState() = 0;
    virtual IntSize getSize() = 0;
#endif
#if ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
    virtual PassRefPtr<Uint8Array> generateKeyRequest(const String&, const String&, Uint8Array*) = 0;
    virtual bool updateKey(const String&, const String&, Uint8Array* key, Uint8Array*) = 0;
#endif

};

}

#endif // ENABLE(MEDIA_SOURCE)

#endif
