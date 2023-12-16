/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebUserMediaClient_h
#define WebUserMediaClient_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <WebCore/UserMediaClient.h>
#include <WebCore/UserMediaRequest.h>
#include <WebCore/MediaStreamSource.h>

namespace WebKit {

class WebPage;

class UserMediaPermission {
public:
    enum Permission {
        PermissionDenied, // User has explicitly denied permission
        PermissionAllowed // User has allowed media stream
    };

    virtual void decidePermission(bool) = 0;
};

class WebUserMediaClient : public WebCore::UserMediaClient, public UserMediaPermission {
public:

    WebUserMediaClient(WebPage*);
    ~WebUserMediaClient();

    virtual void pageDestroyed();
    virtual void requestUserMedia(WTF::PassRefPtr<WebCore::UserMediaRequest>, const WebCore::MediaStreamSourceVector&, const WebCore::MediaStreamSourceVector&);
    virtual void cancelUserMediaRequest(WebCore::UserMediaRequest*);

    void requestPermission(WebCore::ScriptExecutionContext*);
    virtual void decidePermission(bool);

private:
    WTF::RefPtr<WebCore::UserMediaRequest> m_userMediaRequest;
    WebCore::MediaStreamSourceVector m_responseVideoSources;
    WebCore::MediaStreamSourceVector m_responseAudioSources;

    void startUserMedia();

    WebPage* m_page;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // WebUserMediaClient_h
