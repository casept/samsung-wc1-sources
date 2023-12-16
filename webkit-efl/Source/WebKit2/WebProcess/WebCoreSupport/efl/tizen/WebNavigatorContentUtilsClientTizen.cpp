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

#include "config.h"
#include "WebNavigatorContentUtilsClient.h"

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)

#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebNavigatorContentUtilsProxyTizenMessages.h"

using namespace WebCore;

namespace WebKit {

WebNavigatorContentUtilsClient::WebNavigatorContentUtilsClient(WebPage* page)
    : m_page(page)
{
}

void WebNavigatorContentUtilsClient::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
    m_page->send(Messages::WebNavigatorContentUtilsProxyTizen::RegisterProtocolHandler(scheme, baseURL, url, title), m_page->pageID());
}

#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
NavigatorContentUtilsClient::CustomHandlersState WebNavigatorContentUtilsClient::isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url)
{
    unsigned int state;
    m_page->sendSync(Messages::WebNavigatorContentUtilsProxyTizen::IsProtocolHandlerRegistered(scheme, baseURL, url), Messages::WebNavigatorContentUtilsProxyTizen::IsProtocolHandlerRegistered::Reply(state), m_page->pageID());
    return static_cast<CustomHandlersState>(state);
}

void WebNavigatorContentUtilsClient::unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url)
{
    m_page->send(Messages::WebNavigatorContentUtilsProxyTizen::UnregisterProtocolHandler(scheme, baseURL, url), m_page->pageID());
}
#endif

void WebNavigatorContentUtilsClient::registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title)
{
    m_page->send(Messages::WebNavigatorContentUtilsProxyTizen::RegisterContentHandler(mimeType, baseURL, url, title), m_page->pageID());
}

NavigatorContentUtilsClient::CustomHandlersState WebNavigatorContentUtilsClient::isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url)
{
    unsigned int state;
    m_page->sendSync(Messages::WebNavigatorContentUtilsProxyTizen::IsContentHandlerRegistered(mimeType, baseURL, url), Messages::WebNavigatorContentUtilsProxyTizen::IsContentHandlerRegistered::Reply(state), m_page->pageID());
    return static_cast<CustomHandlersState>(state);
}

void WebNavigatorContentUtilsClient::unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url)
{
    m_page->send(Messages::WebNavigatorContentUtilsProxyTizen::UnregisterContentHandler(mimeType, baseURL, url), m_page->pageID());
}

}

#endif // ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
