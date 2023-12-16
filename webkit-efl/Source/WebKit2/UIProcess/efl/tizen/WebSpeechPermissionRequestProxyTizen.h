/*
 * Copyright (C) 2014 Samsung Electronics. All right reserved.
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

#ifndef WebSpeechPermissionRequestProxy_h
#define WebSpeechPermissionRequestProxy_h

#include "APIObject.h"
#include <wtf/PassRefPtr.h>

#if ENABLE(TIZEN_SCRIPTED_SPEECH)

namespace WebKit {

class WebSpeechPermissionRequestManagerProxy;

class WebSpeechPermissionRequestProxy : public TypedAPIObject<APIObject::TypeWebSpeechPermissionRequest> {
public:
    static PassRefPtr<WebSpeechPermissionRequestProxy> create(WebSpeechPermissionRequestManagerProxy* manager, uint64_t webspeechID)
    {
        return adoptRef(new WebSpeechPermissionRequestProxy(manager, webspeechID));
    }

    void allow();
    void deny();

    void invalidate();

private:
    WebSpeechPermissionRequestProxy(WebSpeechPermissionRequestManagerProxy*, uint64_t webspeechID);

    WebSpeechPermissionRequestManagerProxy* m_manager;
    uint64_t m_webspeechID;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_SCRIPTED_SPEECH)
#endif // WebSpeechPermissionRequestProxy_h
