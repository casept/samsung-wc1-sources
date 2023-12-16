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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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

#ifndef WebContextMenuProxyTizen_h
#define WebContextMenuProxyTizen_h

#if ENABLE(TIZEN_CONTEXT_MENU)

#include "WebContextMenuProxy.h"
#include <WebCore/IntPoint.h>

namespace WebKit {

class WebContextMenuItemData;
class WebPageProxy;

class WebContextMenuProxyTizen : public WebContextMenuProxy {
public:
    static PassRefPtr<WebContextMenuProxyTizen> create(Evas_Object* webView, WebPageProxy* page)
    {
        return adoptRef(new WebContextMenuProxyTizen(webView, page));
    }
    ~WebContextMenuProxyTizen();

    virtual void showContextMenu(const WebCore::IntPoint&, const Vector<WebContextMenuItemData>&);
    virtual void hideContextMenu();

private:
    WebContextMenuProxyTizen(Evas_Object*, WebPageProxy*);
    static void contextMenuItemSelectedCallback(void* data, Evas_Object* obj, void* eventInfo);
    static void contextMenuPopupDismissedCallback(void* data, Evas_Object* obj, void* eventInfo);

    void createEflMenu(const Vector<WebContextMenuItemData>&);
    WebPageProxy* m_page;
    WebCore::IntPoint m_popupPosition;

    Evas_Object* m_popup;
    Evas_Object* m_webView;
    Vector<WebContextMenuItemData> m_items;

};


} // namespace WebKit

#endif // ENABLE(TIZEN_CONTEXT_MENU)
#endif // WebContextMenuProxyTizen_h
