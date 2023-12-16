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

#include "config.h"
#include "WebViewEfl.h"

#include "DownloadManagerEfl.h"
#include "EwkView.h"
#include "InputMethodContextEfl.h"
#include "NativeWebMouseEvent.h"
#include "WebContextMenuProxyEfl.h"
#include "WebPopupMenuListenerEfl.h"
#include "ewk_context_private.h"
#include <WebCore/CoordinatedGraphicsScene.h>
#include <WebCore/PlatformContextCairo.h>

#if ENABLE(FULLSCREEN_API)
#include "WebFullScreenManagerProxy.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "EwkTouchEvent.h"
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "WebSubresourceTizen.h"
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
#include "TextSelection.h"
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "DragData.h"
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
#include "DrawingAreaProxyImpl.h"
#endif

#include <Elementary.h>

using namespace EwkViewCallbacks;
using namespace WebCore;

namespace WebKit {

PassRefPtr<WebView> WebView::create(WebContext* context, WebPageGroup* pageGroup)
{
    return adoptRef(new WebViewEfl(context, pageGroup));
}

WebViewEfl::WebViewEfl(WebContext* context, WebPageGroup* pageGroup)
    : WebView(context, pageGroup)
    , m_ewkView(0)
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)
    , m_showHorizontalOverflowEdgeEffect(false)
    , m_showVerticalOverflowEdgeEffect(false)
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)
{
}

void WebViewEfl::setEwkView(EwkView* ewkView)
{
    m_ewkView = ewkView;

#if ENABLE(FULLSCREEN_API)
    m_page->fullScreenManager()->setWebView(ewkView->evasObject());
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    unsigned xWindow = elm_win_xwindow_get(ewkView->evasObject());
    if (xWindow)
        toImpl(ewkView->ewkContext()->wkContext())->setXWindow(xWindow);
#endif
}

void WebViewEfl::setVisible(bool visible)
{
    WebView::setVisible(visible);
#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
    if (!isVisible())
        ewkView()->ewkContext()->clearAllDecodedData();
#endif
}

void WebViewEfl::paintToCairoSurface(cairo_surface_t* surface)
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    PlatformContextCairo context(cairo_create(surface));

    const FloatPoint& position = contentPosition();
    double effectiveScale = m_page->deviceScaleFactor() * contentScaleFactor();

    cairo_matrix_t transform = { effectiveScale, 0, 0, effectiveScale, - position.x() * m_page->deviceScaleFactor(), - position.y() * m_page->deviceScaleFactor() };
    cairo_set_matrix(context.cr(), &transform);
    scene->paintToGraphicsContext(&context);
}

PassRefPtr<WebPopupMenuProxy> WebViewEfl::createPopupMenuProxy(WebPageProxy* page)
{
    return WebPopupMenuListenerEfl::create(page);
}

#if ENABLE(CONTEXT_MENUS)
PassRefPtr<WebContextMenuProxy> WebViewEfl::createContextMenuProxy(WebPageProxy* page)
{
    return WebContextMenuProxyEfl::create(m_ewkView, page);
}
#endif

#if !ENABLE(TIZEN_WEARABLE_PROFILE)
void WebViewEfl::setCursor(const Cursor& cursor)
{
    m_ewkView->setCursor(cursor);
}
#endif

void WebViewEfl::updateTextInputState()
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    m_ewkView->textSelection()->update();
    if (m_ewkView->isTextSelectionMode() && m_ewkView->isTextSelectionHandleDowned())
        return;
#endif

    if (InputMethodContextEfl* inputMethodContext = m_ewkView->inputMethodContext())
        inputMethodContext->updateTextInputState();
}

void WebViewEfl::handleDownloadRequest(DownloadProxy* download)
{
    EwkContext* context = m_ewkView->ewkContext();
    context->downloadManager()->registerDownloadJob(toAPI(download), m_ewkView);
}

void WebViewEfl::setThemePath(const String& theme)
{
    m_page->setThemePath(theme);
}

#if ENABLE(TOUCH_EVENTS)
void WebViewEfl::sendTouchEvent(EwkTouchEvent* touchEvent)
{
    ASSERT(touchEvent);
    m_page->handleTouchEvent(NativeWebTouchEvent(touchEvent, transformFromScene()));
}
#endif

void WebViewEfl::sendMouseEvent(const Evas_Event_Mouse_Down* event)
{
#if ENABLE(TIZEN_WEBKIT2_IMF_CARET_FIX)
    if (InputMethodContextEfl* inputMethodContext = this->ewkView()->inputMethodContext())
        inputMethodContext->resetIMFContext();
#endif

    ASSERT(event);
    m_page->handleMouseEvent(NativeWebMouseEvent(event, transformFromScene(), m_userViewportTransform.toAffineTransform()));
}

void WebViewEfl::sendMouseEvent(const Evas_Event_Mouse_Up* event)
{
    ASSERT(event);
    m_page->handleMouseEvent(NativeWebMouseEvent(event, transformFromScene(), m_userViewportTransform.toAffineTransform()));
}

void WebViewEfl::sendMouseEvent(const Evas_Event_Mouse_Move* event)
{
    ASSERT(event);
    m_page->handleMouseEvent(NativeWebMouseEvent(event, transformFromScene(), m_userViewportTransform.toAffineTransform()));
}

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
bool WebViewEfl::lockOrientation(int willLockOrientation)
{
    return m_ewkView->lockOrientation(willLockOrientation);
}

void WebViewEfl::unlockOrientation()
{
    m_ewkView->unlockOrientation();
}
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
bool WebViewEfl::getStandaloneStatus()
{
    return m_ewkView->getStandaloneStatus();
}
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void WebViewEfl::saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName)
{
    m_ewkView->saveSerializedHTMLDataForMainPage(serializedData, fileName);
}

void WebViewEfl::saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData)
{
    m_ewkView->saveSubresourcesData(subresourceData);
}

void WebViewEfl::startOfflinePageSave(String& path, String& url, String& title)
{
    m_ewkView->startOfflinePageSave(path, url, title);
}
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void WebViewEfl::setClipboardData(const String& /*data*/, const String& /*type*/)
{
}

void WebViewEfl::clearClipboardData()
{
}
#endif // ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)

#if ENABLE(TIZEN_TEXT_SELECTION)
void WebViewEfl::setIsTextSelectionMode(bool isTextSelectionMode)
{
    m_ewkView->setIsTextSelectionMode(isTextSelectionMode);
}
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
void WebViewEfl::setDragPoint(const WebCore::IntPoint& point)
{
    m_ewkView->setDragPoint(point);
}
bool WebViewEfl::isDragMode()
{
    return m_ewkView->isDragMode();
}
void WebViewEfl::setDragMode(bool isDragMode)
{
    m_ewkView->setDragMode(isDragMode);
}
void WebViewEfl::startDrag(const DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    m_ewkView->startDrag(dragData, dragImage);
}
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if PLATFORM(TIZEN)
bool WebViewEfl::makeContextCurrent()
{
    return m_ewkView->makeContextCurrent();
}
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
bool WebViewEfl::useGLBackend()
{
    return m_ewkView->useGLBackend();
}
#endif

#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
void WebViewEfl::setAccelerationMode(bool openGL)
{
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene())
        scene->setAccelerationMode(openGL);
}
#endif

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
void WebViewEfl::findScrollableArea(const IntPoint& point, bool& isScrollableVertically, bool& isScrollableHorizontally)
{
#if ENABLE(TIZEN_EDGE_EFFECT)
    m_showHorizontalOverflowEdgeEffect = false;
    m_showVerticalOverflowEdgeEffect = false;
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)
    m_page->findScrollableArea(transformFromScene().mapPoint(point), isScrollableVertically, isScrollableHorizontally);
    if (isScrollableVertically || isScrollableHorizontally) {
        if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene()) {
            scene->startScrollableAreaTouch(point);
#if ENABLE(TIZEN_EDGE_EFFECT)
            if (isScrollableHorizontally && !isContentsScrollableHorizontally()
                && scene->existsHorizontalScrollbarLayerInTouchedScrollableLayer())
                m_showHorizontalOverflowEdgeEffect = true;
            if (isScrollableVertically && !isContentsScrollableVertically()
                && scene->existsVerticalScrollbarLayerInTouchedScrollableLayer())
                m_showVerticalOverflowEdgeEffect = true;
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)
        }
    }
}

IntSize WebViewEfl::scrollScrollableArea(const IntSize& offset)
{
    IntSize scrolledSize;
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene())
        scrolledSize = scene->scrollScrollableArea(offset);
#if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
    if (scrolledSize.isZero())
#endif
    scrolledSize = m_page->scrollScrollableArea(transformFromScene().mapSize(offset));
#if ENABLE(TIZEN_EDGE_EFFECT)
    updateEdgeEffect(offset, scrolledSize, m_showHorizontalOverflowEdgeEffect, m_showVerticalOverflowEdgeEffect, true);
#endif
    return scrolledSize;
}
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
void WebViewEfl::scrollFinished()
{
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene())
        scene->scrollableAreaScrollFinished();
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
#if ENABLE(TIZEN_EDGE_EFFECT)
    if (CoordinatedGraphicsScene* scene = coordinatedGraphicsScene()) {
        scene->hideVerticalEdgeEffect(true);
        scene->hideHorizontalEdgeEffect(true);
    }
#endif
}
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)

#if ENABLE(TIZEN_EDGE_EFFECT)
void WebViewEfl::updateEdgeEffect(const IntSize& movedSize, const IntSize& scrolledSize)
{
    updateEdgeEffect(movedSize, scrolledSize, isContentsScrollableHorizontally(), isContentsScrollableVertically());
}

bool WebViewEfl::isContentsScrollableHorizontally()
{
    if (m_contentsSize.width() - ceilf(m_size.width() / contentScaleFactor() / m_page->deviceScaleFactor()) > 0)
        return true;

    return false;
}

bool WebViewEfl::isContentsScrollableVertically()
{
    if (m_contentsSize.height() - ceilf(m_size.height() / contentScaleFactor() / m_page->deviceScaleFactor()) > 0)
        return true;

    return false;
}

void WebViewEfl::updateEdgeEffect(const IntSize& movedSize, const IntSize& scrolledSize, bool showHorizontalEffect, bool showVerticalEffect, bool overflow)
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    if (scrolledSize.width())
        scene->hideHorizontalEdgeEffect();
    else if (showHorizontalEffect) {
        if (movedSize.width() > 0)
            scene->showEdgeEffect(TextureMapperLayer::EdgeEffectTypeRight, overflow);
        else if (movedSize.width() < 0)
            scene->showEdgeEffect(TextureMapperLayer::EdgeEffectTypeLeft, overflow);
    }

    if (scrolledSize.height())
        scene->hideVerticalEdgeEffect();
    else if (showVerticalEffect) {
        if (movedSize.height() > 0)
            scene->showEdgeEffect(TextureMapperLayer::EdgeEffectTypeBottom, overflow);
        else if (movedSize.height() < 0)
            scene->showEdgeEffect(TextureMapperLayer::EdgeEffectTypeTop, overflow);
    }
}
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)

void WebViewEfl::setViewBackgroundColor(const WebCore::Color& color)
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    scene->setViewBackgroundColor(color);
}

WebCore::Color WebViewEfl::viewBackgroundColor()
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return Color();

    return scene->viewBackgroundColor();
}

#if ENABLE(TIZEN_DIRECT_RENDERING) || ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
void WebViewEfl::paintToCurrentGLContextWithOptions(bool flipY, int angle, bool needEffect)
#else
void WebViewEfl::paintToCurrentGLContextWithOptions(bool flipY, int angle)
#endif
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    // FIXME: We need to clean up this code as it is split over CoordGfx and Page.
    scene->setDrawsBackground(m_page->drawsBackground());

    const FloatRect& viewport = FloatRect(FloatPoint(), static_cast<FloatSize>(m_size));

    TextureMapper::PaintFlags flags = 0;
    switch (angle) {
    case 90:
        flags = TextureMapper::Painting90DegreesRotated;
        break;
    case 270:
        flags = TextureMapper::Painting270DegreesRotated;
        break;
    }

    TransformationMatrix matrix;
    if (flipY) {
        matrix.translate(0, viewSize().height());
        matrix.flipY();
    }

    matrix *= transformToUserViewport().toTransformationMatrix();
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    scene->paintToCurrentGLContext(matrix, m_opacity, viewport, flags, needEffect);
#else
    scene->paintToCurrentGLContext(matrix, m_opacity, viewport, flags);
#endif
}
#endif // ENABLE(TIZEN_DIRECT_RENDERING) || ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
void WebViewEfl::paintToBackupTexture()
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    scene->paintToBackupTextureEffect();
}

void WebViewEfl::clearBlurEffectResources()
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return;

    scene->clearBlurEffectResources();
}
#endif
#if ENABLE(TIZEN_POINTER_LOCK)
bool WebViewEfl::lockMouse()
{
    return m_ewkView->lockMouse();
}

void WebViewEfl::unlockMouse()
{
    m_ewkView->unlockMouse();
}

bool WebViewEfl::isMouseLocked()
{
    return m_ewkView->isMouseLocked();
}
#endif

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
void WebViewEfl::setIdleClockMode(bool enabled)
{
    DrawingAreaProxyImpl* drawingAreaImpl = static_cast<DrawingAreaProxyImpl*>(m_page->drawingArea());
    if (!drawingAreaImpl)
        return;

    drawingAreaImpl->setIdleClockMode(enabled);
}
#endif

#if ENABLE(TIZEN_SCROLL_SNAP)
IntSize WebViewEfl::findScrollSnapOffset(const IntSize& offset)
{
    CoordinatedGraphicsScene* scene = coordinatedGraphicsScene();
    if (!scene)
        return IntSize();

    return scene->findScrollSnapOffset(offset);
}
#endif

} // namespace WebKit
