/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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

#ifndef ewk_custom_handlers_private_h
#define ewk_custom_handlers_private_h

#include "WKEinaSharedString.h"
#include "ewk_custom_handlers.h"
#include "ewk_object_private.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

class EwkCustomHandlers : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkCustomHandlers)

    static PassRefPtr<EwkCustomHandlers> create(const String& target, const String& baseURL, const String& url, const String& title = String())
    {
        return adoptRef(new EwkCustomHandlers(target, baseURL, url, title));
    }

    const char* target() const { return m_target; }
    const char* baseURL() const { return m_baseURL; }
    const char* url() const { return m_url; }
    const char* title() const { return m_title; }
    Ewk_Custom_Handlers_State state() const { return m_state; }

    void setState(Ewk_Custom_Handlers_State state) { m_state = state; }

private:
    EwkCustomHandlers(const String&, const String&, const String&, const String&);

    WKEinaSharedString m_target;
    WKEinaSharedString m_baseURL;
    WKEinaSharedString m_url;
    WKEinaSharedString m_title;
    Ewk_Custom_Handlers_State m_state;
};

#endif // ewk_custom_handlers_private_h
