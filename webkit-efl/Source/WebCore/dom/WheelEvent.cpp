/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2008, 2010, 2013 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WheelEvent.h"

#include "Clipboard.h"
#include "EventDispatcher.h"
#include "EventNames.h"

#include <wtf/MathExtras.h>

namespace WebCore {

inline static unsigned determineDeltaMode(const PlatformWheelEvent& event)
{
    return event.granularity() == ScrollByPageWheelEvent ? WheelEvent::DOM_DELTA_PAGE : WheelEvent::DOM_DELTA_PIXEL;
}

WheelEventInit::WheelEventInit()
    : wheelDeltaX(0)
    , wheelDeltaY(0)
    , deltaMode(WheelEvent::DOM_DELTA_PIXEL)
{
}

WheelEvent::WheelEvent()
    : m_deltaMode(DOM_DELTA_PIXEL)
    , m_directionInvertedFromDevice(false)
{
}

WheelEvent::WheelEvent(const AtomicString& type, const WheelEventInit& initializer)
    : MouseEvent(type, initializer)
    , m_wheelDelta(IntPoint(initializer.wheelDeltaX, initializer.wheelDeltaY))
    , m_deltaMode(initializer.deltaMode)
{
}

WheelEvent::WheelEvent(const PlatformWheelEvent& event, PassRefPtr<AbstractView> view)
    : MouseEvent(eventNames().mousewheelEvent, true, true, event.timestamp(), view, 0, event.globalPosition().x(), event.globalPosition().y(), event.position().x(), event.position().y()
#if ENABLE(POINTER_LOCK)
                , 0, 0
#endif
                , event.ctrlKey(), event.altKey(), event.shiftKey(), event.metaKey(), 0, 0, 0, false)
    , m_wheelDelta(IntPoint(static_cast<int>(event.wheelTicksX() * TickMultiplier), static_cast<int>(event.wheelTicksY() * TickMultiplier)))
    , m_rawDelta(roundedIntPoint(FloatPoint(event.deltaX(), event.deltaY())))
    , m_deltaMode(determineDeltaMode(event))
    , m_directionInvertedFromDevice(event.directionInvertedFromDevice())
    , m_wheelEvent(adoptPtr(new PlatformWheelEvent(event)))
{
}

void WheelEvent::initWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView> view, int screenX, int screenY, int pageX, int pageY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
{
    if (dispatched())
        return;
    
    initUIEvent(eventNames().mousewheelEvent, true, true, view, 0);
    
    m_screenLocation = IntPoint(screenX, screenY);
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;
    
    // Normalize to the Windows 120 multiple
    m_wheelDelta = IntPoint(rawDeltaX * TickMultiplier, rawDeltaY * TickMultiplier);
    
    m_rawDelta = IntPoint(rawDeltaX, rawDeltaY);
    m_deltaMode = DOM_DELTA_PIXEL;
    m_directionInvertedFromDevice = false;

    initCoordinates(IntPoint(pageX, pageY));
}

void WheelEvent::initWebKitWheelEvent(int rawDeltaX, int rawDeltaY, PassRefPtr<AbstractView> view, int screenX, int screenY, int pageX, int pageY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey)
{
    initWheelEvent(rawDeltaX, rawDeltaY, view, screenX, screenY, pageX, pageY, ctrlKey, altKey, shiftKey, metaKey);
}

const AtomicString& WheelEvent::interfaceName() const
{
    return eventNames().interfaceForWheelEvent;
}

bool WheelEvent::isMouseEvent() const
{
    return false;
}

PassRefPtr<WheelEventDispatchMediator> WheelEventDispatchMediator::create(const PlatformWheelEvent& event, PassRefPtr<AbstractView> view)
{
    return adoptRef(new WheelEventDispatchMediator(event, view));
}

WheelEventDispatchMediator::WheelEventDispatchMediator(const PlatformWheelEvent& event, PassRefPtr<AbstractView> view)
{
    if (!(event.deltaX() || event.deltaY()))
        return;

    setEvent(WheelEvent::create(event, view));
}

WheelEvent* WheelEventDispatchMediator::event() const
{
    return static_cast<WheelEvent*>(EventDispatchMediator::event());
}

bool WheelEventDispatchMediator::dispatchEvent(EventDispatcher* dispatcher) const
{
    ASSERT(event());
    return EventDispatchMediator::dispatchEvent(dispatcher) && !event()->defaultHandled();
}

} // namespace WebCore
