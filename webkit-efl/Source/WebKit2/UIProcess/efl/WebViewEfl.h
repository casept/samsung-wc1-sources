/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef WebViewEfl_h
#define WebViewEfl_h

#include "WebView.h"

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "WebSubresourceTizen.h"
#endif

class EwkView;

namespace WebKit {

#if ENABLE(TOUCH_EVENTS)
class EwkTouchEvent;
#endif

class WebViewEfl : public WebView {
public:
    void setEwkView(EwkView*);
    EwkView* ewkView() { return m_ewkView; }

    void paintToCairoSurface(cairo_surface_t*);
    void setThemePath(const String&);

    void setVisible(bool);

#if ENABLE(TOUCH_EVENTS)
    void sendTouchEvent(EwkTouchEvent*);
#endif
    void sendMouseEvent(const Evas_Event_Mouse_Down*);
    void sendMouseEvent(const Evas_Event_Mouse_Up*);
    void sendMouseEvent(const Evas_Event_Mouse_Move*);

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
    bool lockOrientation(int willLockOrientation);
    void unlockOrientation();
#endif
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    bool getStandaloneStatus();
#endif
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    void saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName);
    void saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData);
    void startOfflinePageSave(String& path, String& url, String& title);
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
    void setClipboardData(const String& data, const String& type);
    void clearClipboardData();
#endif // ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)

#if ENABLE(TIZEN_TEXT_SELECTION)
    void setIsTextSelectionMode(bool isTextSelectionMode);
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    void setDragPoint(const WebCore::IntPoint& point);
    bool isDragMode();
    void setDragMode(bool isDragMode);
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if PLATFORM(TIZEN)
    virtual bool makeContextCurrent();
#endif
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual bool useGLBackend();
#endif
#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
    void setAccelerationMode(bool openGL);
#endif

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    void findScrollableArea(const WebCore::IntPoint&, bool& isScrollableVertically, bool& isScrollableHorizontally);
    WebCore::IntSize scrollScrollableArea(const WebCore::IntSize&);
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
    void scrollFinished();
#endif

    void setViewBackgroundColor(const WebCore::Color&);
    WebCore::Color viewBackgroundColor();

#if ENABLE(TIZEN_DIRECT_RENDERING) || ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    void paintToCurrentGLContextWithOptions(bool flipY, int angle, bool needEffect);
#else
    void paintToCurrentGLContextWithOptions(bool flipY, int angle);
#endif
#endif

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    void paintToBackupTexture();
    void clearBlurEffectResources();
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    void updateEdgeEffect(const WebCore::IntSize& moved, const WebCore::IntSize& scrolled);
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    void setIdleClockMode(bool enabled);
#endif

#if ENABLE(TIZEN_SCROLL_SNAP)
    WebCore::IntSize findScrollSnapOffset(const WebCore::IntSize&);
#endif

private:
    WebViewEfl(WebContext*, WebPageGroup*);

#if !ENABLE(TIZEN_WEARABLE_PROFILE)
    void setCursor(const WebCore::Cursor&) OVERRIDE;
#endif
    PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*) OVERRIDE;

#if ENABLE(CONTEXT_MENUS)
    PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*) OVERRIDE;
#endif

    void updateTextInputState() OVERRIDE;
    void handleDownloadRequest(DownloadProxy*) OVERRIDE;

#if ENABLE(TIZEN_DRAG_SUPPORT)
    virtual void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage);
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if ENABLE(TIZEN_EDGE_EFFECT)
    bool isContentsScrollableHorizontally();
    bool isContentsScrollableVertically();
    void updateEdgeEffect(const WebCore::IntSize& moved, const WebCore::IntSize& scrolled, bool showHorizontalEffect, bool showVerticalEffect, bool overflow = false);
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)

#if ENABLE(TIZEN_POINTER_LOCK)
    bool lockMouse() OVERRIDE;
    void unlockMouse() OVERRIDE;
    bool isMouseLocked() OVERRIDE;
#endif

private:
    EwkView* m_ewkView;
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)
    bool m_showHorizontalOverflowEdgeEffect;
    bool m_showVerticalOverflowEdgeEffect;
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)

    friend class WebView;
};

} // namespace WebKit

#endif // WebViewEfl_h
