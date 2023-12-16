/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
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

#ifndef WKPageTizen_h
#define WKPageTizen_h

#include <Eina.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WK2 ContextMenu Additional API */
WK_EXPORT WKStringRef WKPageContextMenuCopyAbsoluteLinkURLString(WKPageRef page);
WK_EXPORT WKStringRef WKPageContextMenuCopyAbsoluteImageURLString(WKPageRef page);

// #if ENABLE(TIZEN_SQL_DATABASE)
WK_EXPORT void WKPageReplyExceededDatabaseQuota(WKPageRef page, bool allow);
// #endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_SCREEN_CAPTURE)
/* Tizen Experimental feature */
WK_EXPORT WKImageRef WKPageCreateSnapshot(WKPageRef page, WKRect viewArea, float scaleFactor);
#endif

// #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
WK_EXPORT void WKPageScrollBy(WKPageRef, WKSize);
WK_EXPORT void WKPageScrollTo(WKPageRef, WKPoint);
WK_EXPORT void WKPageUpdateScrollPosition(WKPageRef);
WK_EXPORT WKPoint WKPageGetScrollPosition(WKPageRef);
// #endif // #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)

#ifdef __cplusplus
}
#endif

#endif /* WKPageTizen_h */
