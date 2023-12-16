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
#include "WKViewEfl.h"

#include "EwkView.h"
#include "WKAPICast.h"
#include "WebViewEfl.h"
#include <Evas.h>
#include <WebKit2/WKImageCairo.h>

using namespace WebKit;

void WKViewPaintToCairoSurface(WKViewRef viewRef, cairo_surface_t* surface)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->paintToCairoSurface(surface);
}

WKImageRef WKViewCreateSnapshot(WKViewRef viewRef)
{
    EwkView* ewkView = static_cast<WebViewEfl*>(toImpl(viewRef))->ewkView();
    return WKImageCreateFromCairoSurface(ewkView->takeSnapshot().get(), 0 /* options */);
}

void WKViewSetThemePath(WKViewRef viewRef, WKStringRef theme)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->setThemePath(toImpl(theme)->string());
}

void WKViewSendTouchEvent(WKViewRef viewRef, WKTouchEventRef touchEventRef)
{
#if ENABLE(TOUCH_EVENTS)
    static_cast<WebViewEfl*>(toImpl(viewRef))->sendTouchEvent(toImpl(touchEventRef));
#else
    UNUSED_PARAM(viewRef);
    UNUSED_PARAM(touchEventRef);
#endif
}

void WKViewSendMouseDownEvent(WKViewRef viewRef, Evas_Event_Mouse_Down* event)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->sendMouseEvent(event);
}

void WKViewSendMouseUpEvent(WKViewRef viewRef, Evas_Event_Mouse_Up* event)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->sendMouseEvent(event);
}

void WKViewSendMouseMoveEvent(WKViewRef viewRef, Evas_Event_Mouse_Move* event)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->sendMouseEvent(event);
}

void WKViewFindScrollableArea(WKViewRef viewRef, WKPoint point, bool* isScrollableVertically, bool* isScrollableHorizontally)
{
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    static_cast<WebViewEfl*>(toImpl(viewRef))->findScrollableArea(toIntPoint(point), *isScrollableVertically, *isScrollableHorizontally);
#endif
}

WKSize WKViewScrollScrollableArea(WKViewRef viewRef, WKSize size)
{
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    const WebCore::IntSize scrolledOffset = static_cast<WebViewEfl*>(toImpl(viewRef))->scrollScrollableArea(toIntSize(size));
    return WKSizeMake(scrolledOffset.width(), scrolledOffset.height());
#else
    return WKSizeMake(0, 0);
#endif
}

void WKViewScrollFinished(WKViewRef viewRef)
{
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR) || ENABLE(TIZEN_EDGE_EFFECT)
    static_cast<WebViewEfl*>(toImpl(viewRef))->scrollFinished();
#endif
}

void WKViewSetBackgroundColor(WKViewRef viewRef, int red, int green, int blue, int alpha)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->setViewBackgroundColor(WebCore::Color(red, green, blue, alpha));
}

void WKViewGetBackgroundColor(WKViewRef viewRef, int* red, int* green, int* blue, int* alpha)
{
    WebCore::Color backgroundColor = static_cast<WebViewEfl*>(toImpl(viewRef))->viewBackgroundColor();

    if (red)
        *red = backgroundColor.red();
    if (green)
        *green = backgroundColor.green();
    if (blue)
        *blue = backgroundColor.blue();
    if (alpha)
        *alpha = backgroundColor.alpha();
}

void WKViewPaintToCurrentGLContextWithOptions(WKViewRef viewRef, bool flipY, int angle)
{
#if ENABLE(TIZEN_DIRECT_RENDERING) || ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    EwkView* ewkView = static_cast<WebViewEfl*>(toImpl(viewRef))->ewkView();
    static_cast<WebViewEfl*>(toImpl(viewRef))->paintToCurrentGLContextWithOptions(flipY, angle, ewkView->getAppNeedEffect());
#else
    static_cast<WebViewEfl*>(toImpl(viewRef))->paintToCurrentGLContextWithOptions(flipY, angle);
#endif
#endif
}
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
void WKViewPaintToBackupTexture(WKViewRef viewRef)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->paintToBackupTexture();
}

void WKViewCleanBlurEffectResources(WKViewRef viewRef)
{
    static_cast<WebViewEfl*>(toImpl(viewRef))->clearBlurEffectResources();
}
#endif
void WKViewUpdateEdgeEffect(WKViewRef viewRef, WKSize moved, WKSize scrolled)
{
#if ENABLE(TIZEN_EDGE_EFFECT)
    static_cast<WebViewEfl*>(toImpl(viewRef))->updateEdgeEffect(toIntSize(moved), toIntSize(scrolled));
#endif
}

void WKViewSetAccelerationMode(WKViewRef viewRef, bool openGL)
{
#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
    static_cast<WebViewEfl*>(toImpl(viewRef))->setAccelerationMode(openGL);
#endif
}

void WKViewSetIdleClockMode(WKViewRef viewRef, bool enabled)
{
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    static_cast<WebViewEfl*>(toImpl(viewRef))->setIdleClockMode(enabled);
#endif
}

WKSize WKViewFindScrollSnapOffset(WKViewRef viewRef, WKSize offset)
{
#if ENABLE(TIZEN_SCROLL_SNAP)
    return toAPI(static_cast<WebViewEfl*>(toImpl(viewRef))->findScrollSnapOffset(toIntSize(offset)));
#endif
}
