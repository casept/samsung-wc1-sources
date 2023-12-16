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

#ifndef UserMediaPermissionRequestManager_h
#define UserMediaPermissionRequestManager_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class SecurityOrigin;
}

namespace WebKit {

class UserMediaPermission;
class WebPage;

class UserMediaPermissionRequestManager : public RefCounted<UserMediaPermissionRequestManager> {
public:
    static PassRefPtr<UserMediaPermissionRequestManager> create(WebPage*);
    void requestPermission(UserMediaPermission*, WebCore::SecurityOrigin*);
    void didReceiveUserMediaPermissionDecision(uint64_t userMediaID, bool allowed);

private:
    UserMediaPermissionRequestManager(WebPage*);
    WebPage* m_page;
    HashMap<uint64_t, UserMediaPermission*> m_idToPermissionMap;
};

} // namespace WebKit

#endif // #if ENABLE(TIZEN_MEDIA_STREAM)

#endif // UserMediaPermissionRequestManager_h
