/*
 * Copyright (C) 2012, 2013 Samsung Electronics. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)
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

#if USE(ACCELERATED_COMPOSITING)

#include "PageViewportControllerClientEfl.h"

#include "EwkView.h"
#include "PageViewportController.h"

using namespace WebCore;

namespace WebKit {


PageViewportControllerClientEfl::PageViewportControllerClientEfl(EwkView* view)
    : m_view(view)
    , m_controller(0)
{
    ASSERT(m_view);
}

void PageViewportControllerClientEfl::didChangeContentsSize(const WebCore::IntSize&)
{
    m_view->scheduleUpdateDisplay();
}

void PageViewportControllerClientEfl::setViewportPosition(const WebCore::FloatPoint& contentsPoint)
{
    m_contentPosition = contentsPoint;

    WKViewSetContentPosition(m_view->wkView(), WKPointMake(contentsPoint.x(), contentsPoint.y()));

    m_controller->didChangeContentsVisibility(m_contentPosition, m_controller->currentScale());

#if ENABLE(TIZEN_SCROLLBAR)
    m_view->updateScrollbar();
#endif
}

void PageViewportControllerClientEfl::setPageScaleFactor(float newScale)
{
    WKViewSetContentScaleFactor(m_view->wkView(), newScale);
}

void PageViewportControllerClientEfl::didChangeVisibleContents()
{
    m_view->scheduleUpdateDisplay();
}

void PageViewportControllerClientEfl::didChangeViewportAttributes()
{
#if ENABLE(TOUCH_EVENTS)
    m_view->didChangeViewportAttributes();
#endif
}

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
void PageViewportControllerClientEfl::didFirstFrameReady()
{
    m_view->didFirstFrameReady();
}
#endif

void PageViewportControllerClientEfl::setController(PageViewportController* controller)
{
    m_controller = controller;
}

} // namespace WebKit
#endif // USE(ACCELERATED_COMPOSITING)
