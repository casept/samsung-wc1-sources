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

#include "config.h"
#include "WKPageTizen.h"

#include "WebImage.h"
#include "WebPageProxy.h"

using namespace WebKit;

WKStringRef WKPageContextMenuCopyAbsoluteLinkURLString(WKPageRef page)
{
#if ENABLE(TIZEN_CONTEXT_MENU)
    return toCopiedAPI(toImpl(page)->contextMenuAbsoluteLinkURLString());
#else
    UNUSED_PARAM(page);
    return toCopiedAPI(String());
#endif
}

WKStringRef WKPageContextMenuCopyAbsoluteImageURLString(WKPageRef page)
{
#if ENABLE(TIZEN_CONTEXT_MENU)
    return toCopiedAPI(toImpl(page)->contextMenuAbsoluteImageURLString());
#else
    UNUSED_PARAM(page);
    return toCopiedAPI(String());
#endif
}

void WKPageReplyExceededDatabaseQuota(WKPageRef page, bool allow)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    toImpl(page)->replyExceededDatabaseQuota(allow);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(allow);
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

void WKPageReplyExceededIndexedDatabaseQuota(WKPageRef page, bool allow)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    toImpl(page)->replyExceededIndexedDatabaseQuota(allow);
#endif // ENABLE(TIZEN_INDEXED_DATABASE)
}

#if ENABLE(TIZEN_SCREEN_CAPTURE)
WKImageRef WKPageCreateSnapshot(WKPageRef page, WKRect viewArea, float scaleFactor)
{
    RefPtr<WebImage> webImage = toImpl(page)->createSnapshot(toIntRect(viewArea), scaleFactor);
    return toAPI(webImage.release().leakRef());
}
#endif

void WKPageScrollBy(WKPageRef page, WKSize offset)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    toImpl(page)->scrollMainFrameBy(toIntSize(offset));
#endif
}

void WKPageScrollTo(WKPageRef page, WKPoint position)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    toImpl(page)->scrollMainFrameTo(toIntPoint(position));
#endif
}

void WKPageUpdateScrollPosition(WKPageRef page)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    toImpl(page)->updateScrollPosition();
#endif
}

WKPoint WKPageGetScrollPosition(WKPageRef page)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    WebCore::IntPoint scrollPosition = toImpl(page)->scrollPosition();
    return WKPointMake(scrollPosition.x(), scrollPosition.y());
#else
    return WKPointMake(0, 0);
#endif
}
