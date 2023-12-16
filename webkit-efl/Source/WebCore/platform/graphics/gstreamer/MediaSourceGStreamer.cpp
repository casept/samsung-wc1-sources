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
#include "MediaSourceGStreamer.h"

#if ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "SourceBufferPrivateGStreamer.h"
#include "WebKitMediaSourceGStreamer.h"
#include <wtf/gobject/GRefPtr.h>

namespace WebCore {

void MediaSourceGStreamer::open(MediaSourcePrivateClient* mediaSource, WebKitMediaSrc* src)
{
    mediaSource->setPrivateAndOpen(adoptRef(new MediaSourceGStreamer(src)));
}

MediaSourceGStreamer::MediaSourceGStreamer(WebKitMediaSrc* src)
    : m_client(adoptRef(new MediaSourceClientGstreamer(src)))
#if ENABLE(TIZEN_MSE)
    , m_duration(std::numeric_limits<double>::quiet_NaN())
#else
    , m_duration(0.0)
#endif // ENABLE(TIZEN_MSE)
    , m_readyState(MediaPlayer::HaveNothing)
{
}

MediaSourceGStreamer::~MediaSourceGStreamer()
{
}

MediaSourceGStreamer::AddStatus MediaSourceGStreamer::addSourceBuffer(const ContentType& contentType, RefPtr<SourceBufferPrivate>& sourceBufferPrivate)
{
    sourceBufferPrivate = adoptRef(new SourceBufferPrivateGStreamer(m_client.get(), contentType));
    return MediaSourceGStreamer::Ok;
}

void MediaSourceGStreamer::setDuration(double duration)
{
    ASSERT(m_client);
    m_duration = duration;
    m_client->didReceiveDuration(duration);
}

void MediaSourceGStreamer::markEndOfStream(EndOfStreamStatus)
{
    ASSERT(m_client);
    m_client->didFinishLoading(0);
}

void MediaSourceGStreamer::unmarkEndOfStream()
{
    ASSERT(m_client);
}

}
#endif
