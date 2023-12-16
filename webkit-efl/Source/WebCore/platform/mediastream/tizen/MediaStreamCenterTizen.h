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

#ifndef MediaStreamCenterTizen_h
#define MediaStreamCenterTizen_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaStreamCenter.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaStreamComponent;
class MediaStreamDescriptor;
class MediaStreamSourcesQueryClient;

class MediaStreamCenterTizen : public MediaStreamCenter {
public:
    MediaStreamCenterTizen();
    ~MediaStreamCenterTizen();

    // MediaStreamCenter
    virtual void queryMediaStreamSources(PassRefPtr<MediaStreamSourcesQueryClient>) OVERRIDE;

    virtual void didSetMediaStreamTrackEnabled(MediaStreamDescriptor*, MediaStreamComponent*) OVERRIDE;
    virtual bool didAddMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*) OVERRIDE;
    virtual bool didRemoveMediaStreamTrack(MediaStreamDescriptor*, MediaStreamComponent*) OVERRIDE;
    virtual void didStopLocalMediaStream(MediaStreamDescriptor*) OVERRIDE;
    virtual void didCreateMediaStream(MediaStreamDescriptor*) OVERRIDE;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStreamCenterTizen_h
