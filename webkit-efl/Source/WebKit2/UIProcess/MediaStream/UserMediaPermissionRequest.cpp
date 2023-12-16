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
#include "UserMediaPermissionRequest.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "UserMediaPermissionRequestManagerProxy.h"

namespace WebKit {

PassRefPtr<UserMediaPermissionRequest> UserMediaPermissionRequest::create(WebKit::UserMediaPermissionRequestManagerProxy *manager, uint64_t userMediaID)
{
    return adoptRef(new UserMediaPermissionRequest(manager, userMediaID));
}

UserMediaPermissionRequest::UserMediaPermissionRequest(UserMediaPermissionRequestManagerProxy* manager, uint64_t userMediaID)
    : m_manager(manager)
    , m_userMediaID(userMediaID)
{
}

void UserMediaPermissionRequest::allow()
{
    if (!m_manager)
        return;

    m_manager->didReceiveUserMediaPermissionDecision(m_userMediaID, true);
    m_manager = 0;
}

void UserMediaPermissionRequest::deny()
{
    if (!m_manager)
        return;

    m_manager->didReceiveUserMediaPermissionDecision(m_userMediaID, false);
    m_manager = 0;
}

void UserMediaPermissionRequest::invalidate()
{
    m_manager = 0;
}

} // namespace WebKit

#endif // ENABLE(TIZEN_MEDIA_STREAM)
