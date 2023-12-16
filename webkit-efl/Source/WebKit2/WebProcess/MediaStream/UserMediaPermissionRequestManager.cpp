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
#include "UserMediaPermissionRequestManager.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebUserMediaClient.h"
#include <WebCore/SecurityOrigin.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateRequestID()
{
    static uint64_t uniqueRequestID = 1;
    return uniqueRequestID++;
}

PassRefPtr<UserMediaPermissionRequestManager> UserMediaPermissionRequestManager::create(WebPage* page)
{
    return adoptRef(new UserMediaPermissionRequestManager(page));
}

UserMediaPermissionRequestManager::UserMediaPermissionRequestManager(WebPage* page)
    : m_page(page)
{
}

void UserMediaPermissionRequestManager::requestPermission(UserMediaPermission* permission, SecurityOrigin* origin)
{
    uint64_t requestID = generateRequestID();
    m_idToPermissionMap.set(requestID, permission);
    m_page->send(Messages::WebPageProxy::RequestUserMediaPermission(requestID, origin->toString()));
}

void UserMediaPermissionRequestManager::didReceiveUserMediaPermissionDecision(uint64_t requestID, bool allowed)
{
    UserMediaPermission* permission = m_idToPermissionMap.take(requestID);
    permission->decidePermission(allowed);
}

} // namespace WebKit
#endif // ENABLE(TIZEN_MEDIA_STREAM)
