/*
    Copyright (C) 2012 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "MediaStreamCenterTizen.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaStreamDescriptor.h"
#include "MediaStreamManager.h"
#include "MediaStreamSourcesQueryClient.h"
#include "NotImplemented.h"
#include <wtf/MainThread.h>

namespace WebCore {

MediaStreamCenter& MediaStreamCenter::instance()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(MediaStreamCenterTizen, center, ());
    return center;
}

MediaStreamCenterTizen::MediaStreamCenterTizen()
{
}

MediaStreamCenterTizen::~MediaStreamCenterTizen()
{
}

void MediaStreamCenterTizen::queryMediaStreamSources(PassRefPtr<MediaStreamSourcesQueryClient> client)
{
    RefPtr<MediaStreamSourcesQueryClient> mediaClient = client;
    MediaStreamSourceVector audioSources, videoSources;
    mediaClient->didCompleteQuery(audioSources, videoSources);
}

void MediaStreamCenterTizen::didSetMediaStreamTrackEnabled(MediaStreamDescriptor*, MediaStreamComponent*)
{
    notImplemented();
}

bool MediaStreamCenterTizen::didAddMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*)
{
    notImplemented();
    return false;
}

bool MediaStreamCenterTizen::didRemoveMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*)
{
    notImplemented();
    return false;
}

void MediaStreamCenterTizen::didStopLocalMediaStream(MediaStreamDescriptor*)
{
    MediaStreamManager::instance().stopLocalMediaStream();
}

void MediaStreamCenterTizen::didCreateMediaStream(MediaStreamDescriptor*)
{
    notImplemented();
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
