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

#include "config.h"
#include "WebContextMenuProxyTizen.h"

#if ENABLE(TIZEN_CONTEXT_MENU)

#include "WebContextMenuItemData.h"
#include "WebPageProxy.h"
#include "ewk_view_private.h"
#include <Elementary.h>
#include <WebCore/ContextMenu.h>
#include <WebCore/NotImplemented.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

WebContextMenuProxyTizen::WebContextMenuProxyTizen(Evas_Object* webView, WebPageProxy* page)
    : m_page(page)
    , m_popupPosition()
    , m_popup(0)
    , m_webView(webView)
    , m_items()
{
}

WebContextMenuProxyTizen::~WebContextMenuProxyTizen()
{
    if (m_popup)
        evas_object_del(m_popup);
}

void WebContextMenuProxyTizen::contextMenuItemSelectedCallback(void* data, Evas_Object* obj, void*)
{
    WebContextMenuItemData itemData = *(static_cast<WebContextMenuItemData*>(data));
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(evas_object_data_get(obj, "WebContextMenuProxyTizen"));

    menuProxy->m_page->contextMenuItemSelected(itemData);
    menuProxy->hideContextMenu();

    evas_object_del(menuProxy->m_popup);
    menuProxy->m_popup = 0;
}

void WebContextMenuProxyTizen::createEflMenu(const Vector<WebContextMenuItemData>& items)
{
    if (m_popup)
        evas_object_del(m_popup);

    Evas_Object* topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(m_webView));
    if (!topWidget)
        topWidget = m_webView;

    m_popup = elm_ctxpopup_add(topWidget);
    if (!m_popup)
        return;

    m_items = items;
    evas_object_data_set(m_popup, "WebContextMenuProxyTizen", this);
    elm_object_tree_focus_allow_set(m_popup, false);

    size_t size = m_items.size();

    for (size_t i = 0; i < size; i++) {
        if (!m_items.at(i).title().isEmpty())
            elm_ctxpopup_item_append(m_popup, m_items.at(i).title().utf8().data(), 0,
                                     contextMenuItemSelectedCallback, &(m_items.at(i)));
    }
}

void WebContextMenuProxyTizen::contextMenuPopupDismissedCallback(void*, Evas_Object*, void*)
{
}

void WebContextMenuProxyTizen::showContextMenu(const WebCore::IntPoint& position, const Vector<WebContextMenuItemData>& items)
{
    if (items.isEmpty())
        return;

    createEflMenu(items);

    if (m_popup) {
        int webViewX, webViewY;
        evas_object_geometry_get(m_webView, &webViewX, &webViewY, 0, 0);
        IntPoint popupPosition = position;
        popupPosition.setX(popupPosition.x() + webViewX);
        popupPosition.setY(popupPosition.y() + webViewY);

        evas_object_move(m_popup, popupPosition.x(), popupPosition.y());
        evas_object_show(m_popup);

        evas_object_smart_callback_add(m_popup, "dismissed", contextMenuPopupDismissedCallback, this);
    }
}

void WebContextMenuProxyTizen::hideContextMenu()
{
}

} // namespace WebKit
#endif // ENABLE(TIZEN_CONTEXT_MENU)
