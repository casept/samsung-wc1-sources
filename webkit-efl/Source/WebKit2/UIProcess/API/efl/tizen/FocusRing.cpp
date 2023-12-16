/*
 * Copyright (C) 2012 Samsung Electronics
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
#include "FocusRing.h"

#include "EwkView.h"
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include "WebView.h"

using namespace WebKit;
using namespace WebCore;

FocusRing::FocusRing(EwkView* ewkView)
    : m_ewkView(ewkView)
    , m_focusRingObject(0)
    , m_imageOuterWidth(0)
    , m_imageInnerWidth(0)
    , m_showTimer(0)
    , m_hideTimer(0)
    , m_shouldHideAfterShow(false)
    , m_state(FocusRingHideState)
{
}

FocusRing::~FocusRing()
{
    if (m_showTimer)
        ecore_timer_del(m_showTimer);

    if (m_hideTimer)
        ecore_timer_del(m_hideTimer);

    if (m_focusRingObject)
        evas_object_del(m_focusRingObject);
}

Eina_Bool FocusRing::showTimerCallback(void* data)
{
    FocusRing* focusRing = static_cast<FocusRing*>(data);
    Vector<IntRect> rects;
    focusRing->nodeRectAtPosition(rects);
    focusRing->show(rects);

    focusRing->m_showTimer = 0;

    if (focusRing->m_shouldHideAfterShow) {
        focusRing->m_hideTimer = ecore_timer_add((double)focusRing->s_hideTimerTime/1000.0, focusRing->hideTimerCallback, focusRing);
        focusRing->m_shouldHideAfterShow = false;
    }

    return ECORE_CALLBACK_CANCEL;
}

void FocusRing::setImage(const String& path, int outerWidth, int innerWidth)
{
    if (m_imagePath == path && m_imageOuterWidth == outerWidth && m_imageInnerWidth == innerWidth)
        return;

    m_imagePath = path;
    m_imageOuterWidth = outerWidth;
    m_imageInnerWidth = innerWidth;

    if (m_focusRingObject) {
        evas_object_del(m_focusRingObject);
        m_focusRingObject = 0;
    }
    m_state = FocusRingHideState;
}

void FocusRing::requestToShow(const IntPoint& position, bool immediately)
{
    if (!m_imagePath.isNull())
        return;

    if (immediately) {
        if (m_state == FocusRingShowState)
            return;

        if (m_showTimer) {
            ecore_timer_del(m_showTimer);
            m_showTimer = 0;
        } else
            return;

        m_position = position;

        Vector<IntRect> rects;
        nodeRectAtPosition(rects);
        show(rects);

        return;
    }

    m_position = position;

    if (m_showTimer) {
        ecore_timer_del(m_showTimer);
        m_shouldHideAfterShow = false;
    }
    m_showTimer = ecore_timer_add(static_cast<double>(s_showTimerTime) / 1000, showTimerCallback, this);
}

void FocusRing::show(const Vector<IntRect>& rects)
{
    Vector<IntRect> sceneRects;
    for (size_t i = 0; i < rects.size(); ++i)
        sceneRects.append(m_ewkView->webView()->transformToScene().mapRect(rects[i]));
    if (m_state == FocusRingShowState && m_rects == sceneRects)
        return;

    m_rects = sceneRects;

    Evas_Coord_Rectangle viewGeometry;
    evas_object_geometry_get(m_ewkView->evasObject(), &viewGeometry.x, &viewGeometry.y, &viewGeometry.w, &viewGeometry.h);
    IntRect viewRect(viewGeometry.x, viewGeometry.y, viewGeometry.w, viewGeometry.h);

    IntRect rect(unionRect(sceneRects));
    IntPoint origin(rect.location());
    rect.intersect(viewRect);
    if (rect.isEmpty()) {
        hide(false);
        return;
    }

    if (!ensureFocusRingObject())
        return;

    adjustFocusRingObject(rect, origin, viewRect);

    evas_object_show(m_focusRingObject);

    m_state = FocusRingShowState;
}

void FocusRing::moveBy(const IntSize& diff)
{
    if (m_state != FocusRingShowState)
        return;

    Evas_Coord x, y;
    evas_object_geometry_get(m_focusRingObject, &x, &y, 0, 0);

    evas_object_move(m_focusRingObject, x + diff.width(), y + diff.height());
}

void FocusRing::nodeRectAtPosition(Vector<IntRect>& rects)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    IntPoint contentsPosition = m_ewkView->webView()->transformFromScene().mapPoint(m_position);

    WebHitTestResult::Data hitTestResultData = m_ewkView->page()->hitTestResultAtPoint(contentsPosition, WebHitTestResult::HitTestModeDefault | WebHitTestResult::HitTestModeSetFocus);

    if (hitTestResultData.focusedRects.isEmpty())
        return;

    if (!hitTestResultData.absoluteImageURL.isEmpty() && hitTestResultData.absoluteLinkURL.isEmpty())
        return;

    rects = hitTestResultData.focusedRects;
    m_focusRingColor = hitTestResultData.focusedColor;
#endif // #if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
}

bool FocusRing::ensureFocusRingObject()
{
    if (m_focusRingObject)
        return true;

    m_focusRingObject = evas_object_image_filled_add(evas_object_evas_get(m_ewkView->evasObject()));
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    if (!m_ewkView->getAppNeedEffect())
        evas_object_smart_member_add(m_focusRingObject, m_ewkView->evasObject());
#else
        evas_object_smart_member_add(m_focusRingObject, m_ewkView->evasObject());
#endif
    evas_object_repeat_events_set(m_focusRingObject, true);

    if (m_imagePath.isNull()) {
        evas_object_image_colorspace_set(m_focusRingObject, EVAS_COLORSPACE_ARGB8888);
        evas_object_image_alpha_set(m_focusRingObject, true);
    } else {
        evas_object_image_file_set(m_focusRingObject, m_imagePath.utf8().data(), 0);
        int border = m_imageOuterWidth + m_imageInnerWidth;
        evas_object_image_border_set(m_focusRingObject, border, border, border, border);
    }

    return true;
}

void FocusRing::adjustFocusRingObject(const IntRect& rect, const IntPoint& origin, const IntRect& viewRect)
{
    if (m_imagePath.isNull()) {
        evas_object_move(m_focusRingObject, rect.x(), rect.y());
        evas_object_image_size_set(m_focusRingObject, rect.width(), rect.height());
        evas_object_resize(m_focusRingObject, rect.width(), rect.height());

        unsigned char* data = static_cast<unsigned char*>(evas_object_image_data_get(m_focusRingObject, true));
        if (!data)
            return;

        RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, rect.width(), rect.height(), cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, rect.width())));
        RefPtr<cairo_t> context = adoptRef(cairo_create(surface.get()));

        cairo_set_operator(context.get(), CAIRO_OPERATOR_CLEAR);
        cairo_paint(context.get());

        float red, green, blue, alpha;
        m_focusRingColor.getRGBA(red, green, blue, alpha);
        cairo_set_source_rgba(context.get(), red, green, blue, alpha);

        cairo_set_operator(context.get(), CAIRO_OPERATOR_OVER);
        for (size_t i = 0; i < m_rects.size(); ++i) {
            IntRect subRect(m_rects[i]);
            subRect.moveBy(-origin);
            cairo_rectangle(context.get(), subRect.x(), subRect.y(), subRect.width(), subRect.height());
        }
        cairo_fill(context.get());

        evas_object_image_data_set(m_focusRingObject, data);
    } else {
        IntRect actualRect(rect);
        IntRect actualViewRect(viewRect);
        actualViewRect.inflate(-m_imageOuterWidth);

        if (actualRect.intersects(actualViewRect))
            actualRect.intersect(actualViewRect);
        else {
            if (actualViewRect.x() > actualRect.x()) {
                actualRect.setX(actualViewRect.x());
                actualRect.setWidth(1);
            } else if (actualViewRect.maxX() < actualRect.maxX()) {
                actualRect.setX(actualViewRect.maxX());
                actualRect.setWidth(1);
            }
            if (actualViewRect.y() > actualRect.y()) {
                actualRect.setY(actualViewRect.y());
                actualRect.setHeight(1);
            } else if (actualViewRect.maxY() < actualRect.maxY()) {
                actualRect.setY(actualViewRect.maxY());
                actualRect.setHeight(1);
            }
        }

        evas_object_move(m_focusRingObject, actualRect.x() - m_imageOuterWidth, actualRect.y() - m_imageOuterWidth);

        int width = actualRect.width() + m_imageOuterWidth * 2;
        int height = actualRect.height() + m_imageOuterWidth * 2;
        evas_object_resize(m_focusRingObject, width, height);
    }
}

void FocusRing::requestToHide(bool immediately)
{
    if (immediately) {
        hide();
        return;
    }

    if (m_showTimer) {
        m_shouldHideAfterShow = true;
        return;
    }

    hide();
}

Eina_Bool FocusRing::hideTimerCallback(void* data)
{
    FocusRing* focusRing = static_cast<FocusRing*>(data);

    if (focusRing->m_showTimer)
        return ECORE_CALLBACK_CANCEL;

    focusRing->hide();
    focusRing->m_hideTimer = 0;

    return ECORE_CALLBACK_CANCEL;
}

void FocusRing::hide(bool onlyColorDrawing)
{
    if (!m_imagePath.isNull() && onlyColorDrawing)
        return;

    if (m_showTimer) {
        ecore_timer_del(m_showTimer);
        m_showTimer = 0;
    }

    if (m_hideTimer) {
        ecore_timer_del(m_hideTimer);
        m_hideTimer = 0;
    }

    if (m_focusRingObject && evas_object_visible_get(m_focusRingObject))
        evas_object_hide(m_focusRingObject);

    m_rects.clear();
    m_state = FocusRingHideState;
}

IntPoint FocusRing::centerPointInScreen()
{
    if (m_rects.isEmpty())
        return IntPoint(-1, -1);

    Evas_Coord x, y, width, height;
    evas_object_geometry_get(m_ewkView->evasObject(), &x, &y, &width, &height);
    IntRect viewRect(x, y, width, height);

    IntRect rect(m_rects[0]);
    rect.intersect(viewRect);

    return rect.center();
}

bool FocusRing::rectsChanged(const Vector<IntRect>& newRects)
{
     return (m_state == FocusRingShowState && unionRect(m_rects) != m_ewkView->transformToScene().mapRect(unionRect(newRects)));
}
#endif // #if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
