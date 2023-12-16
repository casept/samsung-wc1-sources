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

#ifndef WebNavigatorContentUtilsClient_h
#define WebNavigatorContentUtilsClient_h

#if ENABLE(NAVIGATOR_CONTENT_UTILS)

#include <WebCore/NavigatorContentUtilsClient.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
class WebPage;
#endif

class WebNavigatorContentUtilsClient : public WebCore::NavigatorContentUtilsClient {
public:
#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    WebNavigatorContentUtilsClient(WebPage*);
#endif
    virtual ~WebNavigatorContentUtilsClient() { }

private:
#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    virtual void registerProtocolHandler(const String&, const String&, const String&, const String&);

#if ENABLE(CUSTOM_SCHEME_HANDLER)
    virtual CustomHandlersState isProtocolHandlerRegistered(const String&, const String&, const String&);
    virtual void unregisterProtocolHandler(const String&, const String&, const String&);
#endif

    virtual void registerContentHandler(const String&, const String&, const String&, const String&);
    virtual CustomHandlersState isContentHandlerRegistered(const String&, const String&, const String&);
    virtual void unregisterContentHandler(const String&, const String&, const String&);
#else
    virtual void registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title) OVERRIDE { }

#if ENABLE(CUSTOM_SCHEME_HANDLER)
    virtual CustomHandlersState isProtocolHandlerRegistered(const String&, const String&, const String&) { return CustomHandlersDeclined; }
    virtual void unregisterProtocolHandler(const String&, const String&, const String&) { }
#endif
#endif // ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)


#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    WebPage* m_page;
#endif
};

}

#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)
#endif // WebNavigatorContentUtilsClient_h
