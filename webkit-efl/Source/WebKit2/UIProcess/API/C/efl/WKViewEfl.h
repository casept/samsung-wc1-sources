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

#ifndef WKViewEfl_h
#define WKViewEfl_h

#include <WebKit2/WKBase.h>

//#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
#include <WebKit2/WKGeometry.h>
//#endif

typedef struct _Evas_Event_Mouse_Down Evas_Event_Mouse_Down;
typedef struct _Evas_Event_Mouse_Move Evas_Event_Mouse_Move;
typedef struct _Evas_Event_Mouse_Up Evas_Event_Mouse_Up;
typedef struct _cairo_surface cairo_surface_t;

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT void WKViewPaintToCairoSurface(WKViewRef, cairo_surface_t*);

WK_EXPORT WKImageRef WKViewCreateSnapshot(WKViewRef);

WK_EXPORT void WKViewSetThemePath(WKViewRef, WKStringRef);

WK_EXPORT void WKViewSendTouchEvent(WKViewRef, WKTouchEventRef);

WK_EXPORT void WKViewSendMouseDownEvent(WKViewRef, Evas_Event_Mouse_Down*);
WK_EXPORT void WKViewSendMouseUpEvent(WKViewRef, Evas_Event_Mouse_Up*);
WK_EXPORT void WKViewSendMouseMoveEvent(WKViewRef, Evas_Event_Mouse_Move*);

//#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
WK_EXPORT void WKViewFindScrollableArea(WKViewRef, WKPoint, bool* isScrollableVertically, bool* isScrollableHorizontally);
WK_EXPORT WKSize WKViewScrollScrollableArea(WKViewRef, WKSize);
//#endif
//#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR) || ENABLE(TIZEN_EDGE_EFFECT)
WK_EXPORT void WKViewScrollFinished(WKViewRef);
//#endif

WK_EXPORT void WKViewSetBackgroundColor(WKViewRef viewRef, int red, int green, int blue, int alpha);
WK_EXPORT void WKViewGetBackgroundColor(WKViewRef viewRef, int* red, int* green, int* blue, int* alpha);

//#if ENABLE(TIZEN_DIRECT_RENDERING) || ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
void WKViewPaintToBackupTexture(WKViewRef viewRef);
void WKViewPaintToCurrentGLContextWithOptions(WKViewRef viewRef, bool flipY, int angle);
void WKViewCleanBlurEffectResources(WKViewRef viewRef);
//#endif

//#if ENABLE(TIZEN_EDGE_EFFECT)
void WKViewUpdateEdgeEffect(WKViewRef, WKSize moved, WKSize scrolled);
//#endif

//#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
WK_EXPORT void WKViewSetAccelerationMode(WKViewRef viewRef, bool openGL);
//#endif

//#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
WK_EXPORT void WKViewSetIdleClockMode(WKViewRef viewRef, bool enabled);
//#endif

//#if ENABLE(TIZEN_SCROLL_SNAP)
WK_EXPORT WKSize WKViewFindScrollSnapOffset(WKViewRef viewRef, WKSize offset);
//#endif

#ifdef __cplusplus
}
#endif

#endif /* WKViewEfl_h */
