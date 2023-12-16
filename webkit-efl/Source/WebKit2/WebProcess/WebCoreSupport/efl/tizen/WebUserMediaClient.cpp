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
#include "WebUserMediaClient.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "UserMediaPermissionRequestManager.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <WebCore/MediaStreamSource.h>
#include <WebCore/UUID.h>

using namespace WebCore;

namespace WebKit {

WebUserMediaClient::WebUserMediaClient(WebPage* page)
    : m_page(page)
{
}

WebUserMediaClient::~WebUserMediaClient()
{
}

void WebUserMediaClient::pageDestroyed()
{
    delete this;
}

void WebUserMediaClient::requestUserMedia(WTF::PassRefPtr<UserMediaRequest> request, const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources)
{
    m_responseAudioSources = audioSources;
    m_responseVideoSources = videoSources;
    m_userMediaRequest = request;

    requestPermission(m_userMediaRequest.get()->scriptExecutionContext());
}

void WebUserMediaClient::cancelUserMediaRequest(UserMediaRequest*)
{
    return;
}

void WebUserMediaClient::startUserMedia()
{
    if (m_userMediaRequest.get()->audio())
        m_responseAudioSources.append(MediaStreamSource::create(createCanonicalUUIDString(), MediaStreamSource::TypeAudio, "Internal microphone"));

    if (m_userMediaRequest.get()->video()) {
        RefPtr<MediaStreamSource> source;
        source = MediaStreamSource::create(createCanonicalUUIDString(), MediaStreamSource::TypeVideo, "Self camera");
        m_responseVideoSources.append(source);
    }
}

void WebUserMediaClient::requestPermission(ScriptExecutionContext* context)
{
    m_page->userMediaPermissionRequestManager()->requestPermission(this, context->securityOrigin());
}

void WebUserMediaClient::decidePermission(bool allowed)
{
    Permission permission = allowed ? PermissionAllowed : PermissionDenied;

    switch(permission) {
    case PermissionAllowed:
        startUserMedia();
        m_userMediaRequest.get()->succeed(m_responseAudioSources, m_responseVideoSources);
        return;
    case PermissionDenied:
        m_userMediaRequest.get()->fail();
        return;
    }
}

} // namespace WebKit

#endif // ENABLE(TIZEN_MEDIA_STREAM)
