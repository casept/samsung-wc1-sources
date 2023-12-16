/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "Screen.h"

#include "FloatRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "InspectorInstrumentation.h"
#include "PlatformScreen.h"
#include "Settings.h"
#include "Widget.h"

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
#include "Chrome.h"
#include "ChromeClient.h"
#include "Event.h"
#include "Settings.h"
#endif

namespace WebCore {

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
#if ENABLE(TIZEN_TV_PROFILE)
bool Screen::gNaturalOrientationIsPortrait = false;
#else
bool Screen::gNaturalOrientationIsPortrait = true;
#endif
#endif

Screen::Screen(Frame* frame)
    : DOMWindowProperty(frame)
#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
    , ActiveDOMObject(frame->document())
    , m_orientation(OrientationPortraitPrimary)
#endif
{
}

unsigned Screen::height() const
{
    if (!m_frame)
        return 0;
    long height = static_cast<long>(screenRect(m_frame->view()).height());
    InspectorInstrumentation::applyScreenHeightOverride(m_frame, &height);
    return static_cast<unsigned>(height);
}

unsigned Screen::width() const
{
    if (!m_frame)
        return 0;
    long width = static_cast<long>(screenRect(m_frame->view()).width());
    InspectorInstrumentation::applyScreenWidthOverride(m_frame, &width);
    return static_cast<unsigned>(width);
}

unsigned Screen::colorDepth() const
{
    if (!m_frame)
        return 0;
    return static_cast<unsigned>(screenDepth(m_frame->view()));
}

unsigned Screen::pixelDepth() const
{
    if (!m_frame)
        return 0;
    return static_cast<unsigned>(screenDepth(m_frame->view()));
}

int Screen::availLeft() const
{
    if (!m_frame)
        return 0;
    return static_cast<int>(screenAvailableRect(m_frame->view()).x());
}

int Screen::availTop() const
{
    if (!m_frame)
        return 0;
    return static_cast<int>(screenAvailableRect(m_frame->view()).y());
}

unsigned Screen::availHeight() const
{
    if (!m_frame)
        return 0;
    return static_cast<unsigned>(screenAvailableRect(m_frame->view()).height());
}

unsigned Screen::availWidth() const
{
    if (!m_frame)
        return 0;
    return static_cast<unsigned>(screenAvailableRect(m_frame->view()).width());
}

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
struct ScreenOrientationInfo {
    const char* name;
    Screen::Orientation orientation;
};

static ScreenOrientationInfo orientationMap[] = {
    {"portrait-primary", Screen::OrientationPortraitPrimary},
    {"landscape-primary", Screen::OrientationLandscapePrimary},
    {"portrait-secondary", Screen::OrientationPortraitSecondary},
    {"landscape-secondary", Screen::OrientationLandscapeSecondary},
    {"portrait", Screen::OrientationPortrait},
    {"landscape", Screen::OrientationLandscape}
};

String Screen::orientation() const
{
    for (size_t i = 0; i < sizeof(orientationMap) / sizeof(orientationMap[0]); ++i) {
        if (m_orientation == orientationMap[i].orientation)
            return orientationMap[i].name;
    }
    ASSERT_NOT_REACHED();
    return "";
}

bool Screen::lockOrientation(String orientation)
{
    Orientation willLockOrientation = InvalidOrientation;

    String lowerOrientation = orientation.lower();
    for (size_t i = 0; i < sizeof(orientationMap) / sizeof(orientationMap[0]); ++i) {
        if (lowerOrientation == orientationMap[i].name) {
            willLockOrientation = orientationMap[i].orientation;
            break;
        }
    }

    if (willLockOrientation == InvalidOrientation)
        return false;

    if (!m_frame)
        return false;

    Page* page = m_frame->page();
    if (!page)
        return false;

    if (!page->chrome().client()->lockOrientation(willLockOrientation))
        return false;

    return true;
}

void Screen::unlockOrientation()
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    page->chrome().client()->unlockOrientation();
}

void Screen::sendOrientationChangeEvent(int rawOrientation)
{
    if (!naturalOrientationIsPortrait())
        rawOrientation += 90;

    switch (rawOrientation) {
    case 0:
        m_orientation = OrientationPortraitPrimary;
        break;
    case 90:
        m_orientation = OrientationLandscapePrimary;
        break;
    case 180:
        m_orientation = OrientationPortraitSecondary;
        break;
    case 270:
    case -90:
        m_orientation = OrientationLandscapeSecondary;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    RefPtr<Event> event = Event::create(WebCore::eventNames().orientationchangeEvent, false, false);
    dispatchEvent(event);
}
#endif
} // namespace WebCore
