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

#include "config.h"

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#include "ScrollThumbLayerCoordinator.h"

#include "GraphicsLayer.h"
#include "ScrollableArea.h"
#include "Scrollbar.h"
#include "ScrollbarThemeTizen.h"

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
#include "BitmapImage.h"
#include "RenderLayer.h"
#endif

namespace WebCore {

ScrollThumbLayerCoodinator::~ScrollThumbLayerCoodinator()
{
    m_horizontalScrollThumbs.clear();
    m_verticalScrollThumbs.clear();
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
    m_verticalCircleScrollThumbs.clear();
    m_verticalCircleScrollThumbs.clear();
#endif
}

void ScrollThumbLayerCoodinator::createScrollThumbLayer(GraphicsLayerFactory* factory, ScrollableArea* scrollableArea, GraphicsLayer* scrollbarLayer, ScrollbarOrientation orientation)
{
    OwnPtr<GraphicsLayer> scrollThumbLayer = GraphicsLayer::create(factory, scrollbarLayer->client());

    scrollbarLayer->addChild(scrollThumbLayer.get());

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
    OwnPtr<GraphicsLayer> circleScrollThumbLayer = GraphicsLayer::create(factory, scrollbarLayer->client());
    scrollbarLayer->addChild(circleScrollThumbLayer.get());
    circularScrollThumbsInfo* info = new circularScrollThumbsInfo();
#endif

    switch (orientation) {
    case HorizontalScrollbar:
        m_horizontalScrollThumbs.add(scrollableArea, scrollThumbLayer.release());
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
        m_horizontalCircleScrollThumbs.add(scrollableArea, circleScrollThumbLayer.release());
        m_horizontalScrollThumbInfo.add(scrollableArea, info);
#endif
        break;
    case VerticalScrollbar:
        m_verticalScrollThumbs.add(scrollableArea, scrollThumbLayer.release());
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
        m_verticalCircleScrollThumbs.add(scrollableArea, circleScrollThumbLayer.release());
        m_verticalScrollThumbInfo.add(scrollableArea, info);
#endif
        break;
    }
}

void ScrollThumbLayerCoodinator::scrollBarLayerDestroyed(ScrollableArea* scrollableArea, ScrollbarOrientation orientation)
{
    switch (orientation) {
    case HorizontalScrollbar:
        m_horizontalScrollThumbs.remove(scrollableArea);
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
        m_horizontalCircleScrollThumbs.remove(scrollableArea);
        delete m_horizontalScrollThumbInfo.get(scrollableArea);
        m_horizontalScrollThumbInfo.remove(scrollableArea);
#endif
        break;
    case VerticalScrollbar:
        m_verticalScrollThumbs.remove(scrollableArea);
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
        m_verticalCircleScrollThumbs.remove(scrollableArea);
        delete m_verticalScrollThumbInfo.get(scrollableArea);
        m_verticalScrollThumbInfo.remove(scrollableArea);
#endif
        break;
    }
}

void ScrollThumbLayerCoodinator::updateScrollThumbLayer(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, IntPoint mainScrollPosition)
{
    GraphicsLayer* scrollThumbLayer = 0;
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
    GraphicsLayer* circleScrollThumbLayer = 0;
    circularScrollThumbsInfo* info = 0;
#endif
    Scrollbar* bar = 0;

    switch (orientation) {
    case HorizontalScrollbar:
        ASSERT(m_horizontalScrollThumbs.contains(scrollableArea));
        scrollThumbLayer = m_horizontalScrollThumbs.get(scrollableArea);
        bar = scrollableArea->horizontalScrollbar();
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
        circleScrollThumbLayer = m_horizontalCircleScrollThumbs.get(scrollableArea);
        info = m_horizontalScrollThumbInfo.get(scrollableArea);
#endif
        break;

    case VerticalScrollbar:
        ASSERT(m_verticalScrollThumbs.contains(scrollableArea));
        scrollThumbLayer = m_verticalScrollThumbs.get(scrollableArea);
        bar = scrollableArea->verticalScrollbar();
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
        circleScrollThumbLayer = m_verticalCircleScrollThumbs.get(scrollableArea);
        info = m_verticalScrollThumbInfo.get(scrollableArea);
#endif
        break;
    }
    ASSERT(bar);

    ScrollbarThemeTizen* theme = static_cast<ScrollbarThemeTizen*>(bar->theme());
    ASSERT(theme);

    FloatPoint thumbPosition; // = FloatPoint(0, theme->thumbPosition(bar)) + theme->thumbPositionOffset();
    if (orientation == HorizontalScrollbar)
        thumbPosition.setX(theme->thumbPosition(bar));
    else
        thumbPosition.setY(theme->thumbPosition(bar));
    thumbPosition.move(theme->thumbPositionOffset());

    IntRect thumbRect = theme->thumbRect(bar);
    thumbRect.contract(theme->thumbSizeOffset());

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SCROLLBAR)
    RenderLayer* layer = static_cast<RenderLayer*>(scrollableArea);
    bool useCircularScrollbar = toElement(layer->renderer()->node())->hasAttribute("tizen-circular-scrollbar");

    if (!scrollThumbLayer->parent()->parent() || !scrollThumbLayer->parent()->size().height()) {
        if (useCircularScrollbar)
            return;

        scrollThumbLayer->setPosition(thumbPosition);
        scrollThumbLayer->setSize(thumbRect.size());
        scrollThumbLayer->setContentsRect(IntRect(0, 0, thumbRect.width(), thumbRect.height()));
        scrollThumbLayer->setContentsToSolidColor(theme->thumbColor());
        return;
    }

    if (thumbRect.size().width() < 0 || thumbRect.size().height() < 0)
        return;

    static const int diameter = 360;
    static const int radius = diameter / 2;
    static const int margin = 10;
    static const int arcRadius = radius - margin / 2 - theme->thumbSizeOffset().width() / 2;
    static const int scrollbarAngleScope = 60;
    static const int minimumAngle = 2;
    static const int minimumThumbSize = 6;

    // Circular scroll bar
    if (useCircularScrollbar) {
        scrollThumbLayer->setOpacity(0);
        circleScrollThumbLayer->setOpacity(255);

        double angle1 = 0;
        double angle2 = 0;
        double scrollbarAreaLength = 0;
        bool shouldCreateCircularScrollbarImage = false;
        if (info->getThumbSize() != thumbRect.size() || circleScrollThumbLayer->size().isZero())
            shouldCreateCircularScrollbarImage = true;
        info->setThumbSize(thumbRect.size());

        if (orientation == VerticalScrollbar) {
            scrollbarAreaLength = scrollThumbLayer->parent()->size().height() + margin + margin;
            angle1 = scrollbarAngleScope * static_cast<double>(thumbPosition.y() + margin) / scrollbarAreaLength;
            angle2 = scrollbarAngleScope * static_cast<double>(thumbPosition.y() + thumbRect.size().height()  + margin) / scrollbarAreaLength;
        } else {
            scrollbarAreaLength = scrollThumbLayer->parent()->size().width() + margin + margin;
            angle1 = scrollbarAngleScope * static_cast<double>(thumbPosition.x() + margin) / scrollbarAreaLength;
            angle2 = scrollbarAngleScope * static_cast<double>(thumbPosition.x() + thumbRect.size().width() + margin) / scrollbarAreaLength;
        }

        if (angle2 - angle1 < minimumAngle)
            angle2 = angle1 + minimumAngle;

        double radianForCairo1 = 270 * (M_PI / 180.0);
        double radianForCairo2 = (270 + angle2 - angle1) * (M_PI / 180.0);
        double radianDiff = (angle2 - angle1) * (M_PI / 180.0);
        double radianMiddleAngle = (30 - (angle1 + (angle2 - angle1) / 2)) * (M_PI / 180.0);

        if (shouldCreateCircularScrollbarImage) {
            // Create arc scroll bar image.
            double circleThumbWidth = 0;
            double circleThumbHeight = 0;
            circleThumbHeight = arcRadius * (1 - std::cos(radianDiff));
            circleThumbWidth = arcRadius * std::sin(radianDiff);

            // Extent circle thumb size to avoid clipped.
            circleThumbWidth += (theme->thumbSizeOffset().width() / 2) * std::sin(radianDiff);
            circleThumbHeight += (theme->thumbSizeOffset().width() / 2) * (1 + std::cos(radianDiff));

            RefPtr<cairo_surface_t> cairoSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, circleThumbWidth, circleThumbHeight));
            RefPtr<cairo_t> cairoContext = adoptRef(cairo_create(cairoSurface.get()));

            const float thumbColorR = static_cast<float>(theme->thumbColor().red()) / 255;
            const float thumbColorG = static_cast<float>(theme->thumbColor().green()) / 255;
            const float thumbColorB = static_cast<float>(theme->thumbColor().blue()) / 255;
            const float thumbColorA = static_cast<float>(theme->thumbColor().alpha()) / 255;
            cairo_set_source_rgba(cairoContext.get(), thumbColorR, thumbColorG, thumbColorB, thumbColorA);
            cairo_set_line_width(cairoContext.get(), theme->thumbSizeOffset().width());
            cairo_arc(cairoContext.get(), 0, theme->thumbSizeOffset().width() / 2 + arcRadius, arcRadius, radianForCairo1, radianForCairo2);
            cairo_stroke(cairoContext.get());

            RefPtr<BitmapImage> image = BitmapImage::create(cairoSurface.release());

            // Move created image.
            FloatPoint circleThumbPosition = FloatPoint(mainScrollPosition.x() - scrollThumbLayer->parent()->position().x() - scrollThumbLayer->parent()->parent()->position().x(), mainScrollPosition.y() - scrollThumbLayer->parent()->position().y() - scrollThumbLayer->parent()->parent()->position().y());
            circleThumbPosition += FloatSize(radius, radius);

            if (orientation == VerticalScrollbar) {
                circleThumbPosition += FloatSize(arcRadius * std::cos(radianMiddleAngle), -1 * arcRadius * std::sin(radianMiddleAngle));
                circleThumbPosition -= FloatSize(arcRadius * std::sin(radianDiff / 2), arcRadius * (1 - std::cos(radianDiff / 2)));
                circleThumbPosition -= FloatSize(0, theme->thumbSizeOffset().width() / 2);
            } else {
                circleThumbPosition += FloatSize(-1 * arcRadius * std::sin(radianMiddleAngle), arcRadius * std::cos(radianMiddleAngle));
                circleThumbPosition -= FloatSize(arcRadius * std::sin(radianDiff / 2), arcRadius * (1 - std::cos(radianDiff / 2)));
                circleThumbPosition -= FloatSize(theme->thumbSizeOffset().width() / 2, theme->thumbSizeOffset().width());
            }

            // We should rotate image with center of arc. but currently it is rotated with center of image.
            // It makes error and we should revise it.
            FloatPoint centerOfImage(circleThumbWidth / 2, circleThumbHeight / 2);
            FloatPoint centerOfArc(arcRadius * std::sin(radianDiff / 2), arcRadius * (1 - std::cos(radianDiff / 2)) + (theme->thumbSizeOffset().width() / 2));
            double diffAngle = std::atan((centerOfArc.x() - centerOfImage.x()) / (centerOfImage.y() - centerOfArc.y()));
            float distance = FloatPoint(centerOfImage - centerOfArc).length();
            double a1 = 90 * (M_PI / 180.0) - diffAngle;
            double a2 = a1 - (scrollbarAngleScope + angle1) * (M_PI / 180.0);
            double diffX = distance * (std::cos(a2) - std::cos(a1));
            double diffY = distance * (std::sin(a2) - std::sin(a1));

            if (orientation == VerticalScrollbar)
                diffX *= -1;
            else
                diffY *= 2;
            circleThumbPosition += FloatSize(diffX, diffY);

            double rotationAngle = 0.0;
            if (orientation == VerticalScrollbar) {
                info->setAngle(angle1);
                rotationAngle = -info->getAbsoluteAngle() + 60 + angle1;
            } else {
                info->setAngle(angle2);
                rotationAngle = -info->getAbsoluteAngle() + 210 - angle2;
            }

            circleScrollThumbLayer->setPosition(circleThumbPosition);
            circleScrollThumbLayer->setSize(IntSize(circleThumbWidth, circleThumbHeight));
            circleScrollThumbLayer->setContentsRect(IntRect(0, 0, circleThumbWidth, circleThumbHeight));
            circleScrollThumbLayer->setContentsToImage(image.get());
            TransformationMatrix matrix = circleScrollThumbLayer->transform();
            circleScrollThumbLayer->setTransform(matrix.rotate(rotationAngle));
            info->setAbsoluteAngle(info->getAbsoluteAngle() + rotationAngle);

        } else {
            // Rotoray and move position scrollbar image.
            TransformationMatrix matrix = circleScrollThumbLayer->transform();
            FloatPoint newPosition = circleScrollThumbLayer->position();
            if (orientation == VerticalScrollbar) {
                newPosition += FloatSize(arcRadius * (std::cos(radianMiddleAngle) - std::cos(info->getMiddleAngle())), -1 * arcRadius * (std::sin(radianMiddleAngle) - std::sin(info->getMiddleAngle())));
                circleScrollThumbLayer->setTransform(matrix.rotate(angle1 - info->getAngle()));
                info->setAbsoluteAngle(info->getAbsoluteAngle() + angle1 - info->getAngle());
                info->setAngle(angle1);
            } else {
                newPosition += FloatSize(-1 * arcRadius * (std::sin(radianMiddleAngle) - std::sin(info->getMiddleAngle())), arcRadius * (std::cos(radianMiddleAngle) - std::cos(info->getMiddleAngle())));
                circleScrollThumbLayer->setTransform(matrix.rotate(info->getAngle() - angle2));
                info->setAbsoluteAngle(info->getAbsoluteAngle() + info->getAngle() - angle2);
                info->setAngle(angle2);
            }
            circleScrollThumbLayer->setPosition(newPosition);
        }
        info->setMiddleAngle(radianMiddleAngle);

        return;
    }
    circleScrollThumbLayer->setOpacity(0);

    // Inner scroll bar
    IntPoint scrollableAreaStart = IntPoint((int)scrollThumbLayer->parent()->parent()->position().x(), (int)scrollThumbLayer->parent()->parent()->position().y());
    GraphicsLayer* graphicsLayer = scrollThumbLayer->parent()->parent()->parent();
    while (graphicsLayer) {
        scrollableAreaStart = scrollableAreaStart + IntPoint((int)graphicsLayer->position().x(), (int)graphicsLayer->position().y());
        scrollableAreaStart = graphicsLayer->transform().mapPoint(scrollableAreaStart);
        graphicsLayer = graphicsLayer->parent();
    }
    IntPoint scrollableAreaEnd = IntPoint(scrollableAreaStart.x() + scrollableArea->visibleWidth(), scrollableAreaStart.y() + scrollableArea->visibleHeight());

    int deltaX = 0;
    int deltaY = 0;
    static const double halfScrollbarAngleScopeRadian = (scrollbarAngleScope / 2) * (M_PI / 180.0);
    if (orientation == VerticalScrollbar) {
        deltaX = (1 - std::cos(halfScrollbarAngleScopeRadian)) * radius;
        deltaY = std::sin(halfScrollbarAngleScopeRadian) * radius;
    } else {
        deltaX = std::sin(halfScrollbarAngleScopeRadian) * radius;
        deltaY = (1 - std::cos(halfScrollbarAngleScopeRadian)) * radius;
    }

    IntPoint innerSquareStart = mainScrollPosition + IntPoint(deltaX, deltaY);
    IntPoint innerSquareEnd = IntPoint(mainScrollPosition.x() + 2 * radius - deltaX, mainScrollPosition.y() + 2 * radius - deltaY);

    if (orientation == VerticalScrollbar && innerSquareEnd.x() < scrollableAreaEnd.x()) {
        int upperSpace = margin;
        int lowerSpace = margin;
        if (scrollableAreaStart.y() < innerSquareStart.y())
            upperSpace += std::min(innerSquareStart.y() - scrollableAreaStart.y(), deltaY);

        if (scrollableAreaEnd.y() > innerSquareEnd.y())
            lowerSpace += std::min(scrollableAreaEnd.y() - innerSquareEnd.y(), deltaY);

        float scaleValue = fabs(static_cast<float>(scrollThumbLayer->parent()->size().height() - upperSpace - lowerSpace) / static_cast<float>(scrollThumbLayer->parent()->size().height()));

        thumbPosition.move(innerSquareEnd.x() - scrollableAreaEnd.x(), 0);
        thumbPosition.setY(static_cast<float>(thumbPosition.y()) * scaleValue + upperSpace);
        int thumbHeight = static_cast<float>(thumbRect.height()) * scaleValue;
        thumbRect.setHeight(std::max(thumbHeight, minimumThumbSize));
    } else if (orientation == HorizontalScrollbar && innerSquareEnd.y() < scrollableAreaEnd.y()) {
        int leftSpace = margin;
        int rightSpace = margin;
        if (scrollableAreaStart.x() < innerSquareStart.x())
            leftSpace += std::min(innerSquareStart.x() - scrollableAreaStart.x(), deltaX);

        if (scrollableAreaEnd.x() > innerSquareEnd.x())
            rightSpace += std::min(scrollableAreaEnd.x() - innerSquareEnd.x(), deltaX);

        float scaleValue = fabs(static_cast<float>(scrollThumbLayer->parent()->size().width() - leftSpace - rightSpace) / static_cast<float>(scrollThumbLayer->parent()->size().width()));

        thumbPosition.move(0, innerSquareEnd.y() - scrollableAreaEnd.y());
        thumbPosition.setX(static_cast<float>(thumbPosition.x()) * scaleValue + leftSpace);
        int thumbWidth = static_cast<float>(thumbRect.width()) * scaleValue;
        thumbRect.setWidth(std::max(thumbWidth, minimumThumbSize));
    }
#endif
    scrollThumbLayer->setOpacity(255);
    scrollThumbLayer->setPosition(thumbPosition);
    scrollThumbLayer->setSize(thumbRect.size());
    scrollThumbLayer->setContentsRect(IntRect(0, 0, thumbRect.width(), thumbRect.height()));
    scrollThumbLayer->setContentsToSolidColor(theme->thumbColor());
}

} // namespace WebCore

#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
