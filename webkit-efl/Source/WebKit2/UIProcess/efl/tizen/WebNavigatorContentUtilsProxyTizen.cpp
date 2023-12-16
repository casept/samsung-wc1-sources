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
#include "WebNavigatorContentUtilsProxyTizen.h"

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)

#include "EwkView.h"
#include "WebContext.h"
#include "WebNavigatorContentUtilsProxyTizenMessages.h"
#include "WebPageProxy.h"
#include "ewk_custom_handlers_private.h"
#include <WebCore/NavigatorContentUtilsClient.h>

namespace WebKit {

WebNavigatorContentUtilsProxyTizen::WebNavigatorContentUtilsProxyTizen(WebPageProxy* page)
    : m_page(page)
{
    m_page->process()->context()->addMessageReceiver(Messages::WebNavigatorContentUtilsProxyTizen::messageReceiverName(), m_page->pageID(), this);
}

WebNavigatorContentUtilsProxyTizen::~WebNavigatorContentUtilsProxyTizen()
{
    m_page->process()->context()->removeMessageReceiver(Messages::WebNavigatorContentUtilsProxyTizen::messageReceiverName(), m_page->pageID());
}

inline EwkView* WebNavigatorContentUtilsProxyTizen::ewkView()
{
    return toEwkView(EwkView::toEvasObject(toAPI(m_page)));
}

void WebNavigatorContentUtilsProxyTizen::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
    RefPtr<EwkCustomHandlers> data = EwkCustomHandlers::create(scheme, baseURL, url, title);
    ewkView()->smartCallback<EwkViewCallbacks::ProtocolHandlerRegister>().call(data.get());
}

#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
void WebNavigatorContentUtilsProxyTizen::isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url, uint32_t& state)
{
    RefPtr<EwkCustomHandlers> data = EwkCustomHandlers::create(scheme, baseURL, url);
    ewkView()->smartCallback<EwkViewCallbacks::ProtocolHandlerIsRegistered>().call(data.get());

    state = data->state();
}

void WebNavigatorContentUtilsProxyTizen::unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url)
{
    RefPtr<EwkCustomHandlers> data = EwkCustomHandlers::create(scheme, baseURL, url);
    ewkView()->smartCallback<EwkViewCallbacks::ContentHandlerUnregister>().call(data.get());
}
#endif // ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)

void WebNavigatorContentUtilsProxyTizen::registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title)
{
    RefPtr<EwkCustomHandlers> data = EwkCustomHandlers::create(mimeType, baseURL, url, title);
    ewkView()->smartCallback<EwkViewCallbacks::ContentHandlerRegister>().call(data.get());
}

void WebNavigatorContentUtilsProxyTizen::isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url, uint32_t& state)
{
    RefPtr<EwkCustomHandlers> data = EwkCustomHandlers::create(mimeType, baseURL, url);
    ewkView()->smartCallback<EwkViewCallbacks::ContentHandlerIsRegistered>().call(data.get());

    state = data->state();
}

void WebNavigatorContentUtilsProxyTizen::unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url)
{
    RefPtr<EwkCustomHandlers> data = EwkCustomHandlers::create(mimeType, baseURL, url);
    ewkView()->smartCallback<EwkViewCallbacks::ContentHandlerUnregister>().call(data.get());
}

}

#endif // ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
