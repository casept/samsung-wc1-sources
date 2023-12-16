/*
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebTizenClient.h"

#include "WebPageProxy.h"

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "UserMediaPermissionRequest.h"
#endif // ENABLE(TIZEN_MEDIA_STREAM)

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_MEDIA_STREAM)
bool WebTizenClient::decidePolicyForUserMediaPermissionRequest(WebPageProxy* page, UserMediaPermissionRequest* permissionRequest, WebSecurityOrigin* origin)
{
    if (!m_client.decidePolicyForUserMediaPermissionRequest)
        return false;

    m_client.decidePolicyForUserMediaPermissionRequest(toAPI(page), toAPI(permissionRequest), toAPI(origin),  m_client.clientInfo);
    return true;
}
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_INDEXED_DATABASE)
bool WebTizenClient::exceededIndexedDatabaseQuota(WebPageProxy* page, WebSecurityOrigin* origin, long long currentUsage, WebFrameProxy* frame)
{
    if (!m_client.exceededIndexedDatabaseQuota)
        return currentUsage;

    return m_client.exceededIndexedDatabaseQuota(toAPI(page), toAPI(origin), currentUsage, toAPI(frame), m_client.clientInfo);
}
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
bool WebTizenClient::decidePolicyForCertificateError(WebPageProxy* page, const String& url, const String& certificate, int error)
{
    if (!m_client.decidePolicyForCertificateError)
        return false;

    return m_client.decidePolicyForCertificateError(toAPI(page), toCopiedAPI(url), toCopiedAPI(certificate), error, m_client.clientInfo);
}
#endif

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
bool WebTizenClient::decidePolicyForWebSpeechPermissionRequest(WebPageProxy* page, WebSpeechPermissionRequestProxy* permissionRequest,
    const String& host, const String& protocol, uint32_t port)
{
    if (!m_client.decidePolicyForWebSpeechPermissionRequest)
        return false;

    m_client.decidePolicyForWebSpeechPermissionRequest(toAPI(page), toAPI(permissionRequest), toCopiedAPI(host), toCopiedAPI(protocol), port, m_client.clientInfo);
    return true;
}
#endif
} // namespace WebKit
