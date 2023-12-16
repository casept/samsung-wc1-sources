/*
   Copyright (C) 2014 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ScrollThumbLayerCoordinator_h
#define ScrollThumbLayerCoordinator_h

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#include "ScrollTypes.h"

#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
#include <IntSize.h>
#endif

namespace WebCore {

class GraphicsLayer;
class GraphicsLayerFactory;
class ScrollableArea;
class IntPoint;

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
class circularScrollThumbsInfo {
public:
    circularScrollThumbsInfo()
    : m_scrollThumbSize(IntSize())
    , m_middleAngle(0.0)
    , m_angle(0.0)
    , m_absoluteAngle(0.0)
    {}

    void setThumbSize(IntSize size) { m_scrollThumbSize = size; }
    void setMiddleAngle(double angle) { m_middleAngle = angle; }
    void setAngle(double angle) { m_angle = angle; }
    void setAbsoluteAngle(double angle) { m_absoluteAngle = angle; }

    IntSize getThumbSize() { return m_scrollThumbSize; }
    double getAngle() { return m_angle; }
    double getMiddleAngle() { return m_middleAngle; }
    double getAbsoluteAngle() { return m_absoluteAngle; }

private:
    IntSize m_scrollThumbSize;
    double m_middleAngle;
    double m_angle;
    double m_absoluteAngle;
};
#endif

class ScrollThumbLayerCoodinator {
public:
    ScrollThumbLayerCoodinator() {}
    ~ScrollThumbLayerCoodinator();

    void createScrollThumbLayer(GraphicsLayerFactory*, ScrollableArea*, GraphicsLayer*, ScrollbarOrientation);
    void scrollBarLayerDestroyed(ScrollableArea*, ScrollbarOrientation);
    void updateScrollThumbLayer(ScrollableArea*, ScrollbarOrientation, IntPoint);

private:
    typedef HashMap<ScrollableArea*, OwnPtr<GraphicsLayer> > ScrollThumbMap;
    ScrollThumbMap m_horizontalScrollThumbs;
    ScrollThumbMap m_verticalScrollThumbs;
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
    typedef HashMap<ScrollableArea*, circularScrollThumbsInfo* > ScrollThumbInfo;
    ScrollThumbMap m_horizontalCircleScrollThumbs;
    ScrollThumbMap m_verticalCircleScrollThumbs;
    ScrollThumbInfo m_horizontalScrollThumbInfo;
    ScrollThumbInfo m_verticalScrollThumbInfo;
#endif
};

} // namespace WebCore

#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#endif // ScrollThumbLayerCoordinator_h
