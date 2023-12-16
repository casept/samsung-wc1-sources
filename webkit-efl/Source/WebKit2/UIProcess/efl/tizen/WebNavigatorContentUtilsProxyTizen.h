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

#ifndef WebNavigatorContentUtilsProxyTizen_h
#define WebNavigatorContentUtilsProxyTizen_h

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)

#include "MessageReceiver.h"
#include <wtf/text/WTFString.h>

class EwkView;

namespace WebKit {

class WebPageProxy;

class WebNavigatorContentUtilsProxyTizen : public CoreIPC::MessageReceiver {
public:
    explicit WebNavigatorContentUtilsProxyTizen(WebPageProxy*);
    ~WebNavigatorContentUtilsProxyTizen();

private:
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    inline EwkView* ewkView();

    void registerProtocolHandler(const String&, const String&, const String&, const String&);
#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
    void isProtocolHandlerRegistered(const String&, const String&, const String&, uint32_t&);
    void unregisterProtocolHandler(const String&, const String&, const String&);
#endif

    void registerContentHandler(const String&, const String&, const String&, const String&);
    void isContentHandlerRegistered(const String&, const String&, const String&, uint32_t&);
    void unregisterContentHandler(const String&, const String&, const String&);

    WebPageProxy* m_page;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)

#endif // WebNavigatorContentUtilsProxyTizen_h