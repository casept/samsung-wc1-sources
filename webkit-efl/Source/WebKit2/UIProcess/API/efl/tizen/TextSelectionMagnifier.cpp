/*
 * Copyright (C) 2012 Samsung Electronics
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

#if ENABLE(TIZEN_TEXT_SELECTION)
#include "TextSelectionMagnifier.h"

#include "WebImage.h"
#include <Elementary.h>
#include <WebCore/CairoUtilitiesEfl.h>
#include <WebCore/IntRect.h>
#include <cairo.h>

using namespace WebCore;

namespace WebKit {

TextSelectionMagnifier::TextSelectionMagnifier(EwkView* view)
    : m_view(view)
    , m_image(0)
    , m_magnifier(0)
    , m_width(0)
    , m_height(0)
    , m_edjeWidth(0)
    , m_edjeHeight(0)
{
}

TextSelectionMagnifier::~TextSelectionMagnifier()
{
    if (m_image)
        evas_object_del(m_image);
    evas_object_del(m_magnifier);
}

static float s_widtheOffset = 32;
static float s_heightOffset = 60;
static int s_magnifierMargin = 10;

void TextSelectionMagnifier::update(const IntPoint& point)
{
    if (!m_magnifier) {
        Evas_Object* topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(m_view->evasObject()));
        if (!topWidget)
            topWidget = m_view->evasObject();

        m_magnifier = elm_layout_add(topWidget);
        elm_layout_file_set(m_magnifier, EDJE_DIR"/Magnifier.edj", "magnifier");

        edje_object_part_geometry_get(elm_layout_edje_get(m_magnifier), "bg", 0, 0, &m_edjeWidth, &m_edjeHeight);

        m_width = m_edjeWidth - s_widtheOffset;
        m_height = m_edjeHeight - s_heightOffset;
    }

    int viewX, viewY, viewWidth, viewHeight;
    evas_object_geometry_get(m_view->evasObject(), &viewX, &viewY, &viewWidth, &viewHeight);

    float zoomLevel = 1.5f;
    IntRect rect(point.x() - viewX - (ceil(m_width/zoomLevel)/2), point.y() - viewY - (ceil(m_height/zoomLevel)/2), ceil(m_width/zoomLevel), ceil(m_height/zoomLevel));

    if (rect.x() < 0)
        rect.setX(0);
    if (rect.y() < 0)
        rect.setY(0);

    int overSize = rect.maxX() - (viewX + viewWidth);
    if (overSize > 0)
        rect.setX(rect.x() - overSize);
    overSize = rect.maxY() - (viewY + viewHeight);
    if (overSize > 0)
        rect.setY(rect.y() - overSize);

    // Check if the touch point is very near to top or bottom.
    // Adjust the rect to get the proper image in magnifier
    if ((rect.y() < m_height/2) && (rect.y() > s_magnifierMargin))
        rect.setY(rect.y() - (s_magnifierMargin));
    if (rect.y() > (viewHeight - m_height))
        rect.setY(rect.y() + (s_magnifierMargin));

    // FixMe: implement createSnapShot.
    //RefPtr<WebImage> webImage = m_view->page()->createSnapshot(rect, zoomLevel);
    RefPtr<WebImage> webImage;
    if (!webImage || !webImage->bitmap())
        return;
    RefPtr<cairo_surface_t> cairoSurface = webImage->bitmap()->createCairoSurface();
    if (!cairoSurface)
        return;

    if (m_image)
        evas_object_del(m_image);

    m_image = evasObjectFromCairoImageSurface(evas_object_evas_get(m_view->evasObject()), cairoSurface.get()).leakRef();
    evas_object_size_hint_min_set(m_image, m_width, m_height);
    evas_object_resize(m_image, m_width, m_height);
    evas_object_image_filled_set(m_image, true);
    evas_object_show(m_image);

    elm_object_part_content_set(m_magnifier, "swallow", m_image);
    evas_object_pass_events_set(m_image, EINA_TRUE);
    evas_object_clip_set(m_image, m_magnifier);

    evas_object_layer_set(m_magnifier, EVAS_LAYER_MAX);
    evas_object_layer_set(m_image, EVAS_LAYER_MAX);
}

void TextSelectionMagnifier::show()
{
    if (isVisible())
        return;

    evas_object_show(m_magnifier);
    evas_object_smart_callback_call(m_view->evasObject(), "magnifier,show", 0);
}

void TextSelectionMagnifier::hide()
{
    if (!isVisible())
        return;

    evas_object_hide(m_magnifier);
    evas_object_smart_callback_call(m_view->evasObject(), "magnifier,hide", 0);
}

static int s_magnifierOffsetY = 220;
static int s_defaultEdjeHeight = 177;

void TextSelectionMagnifier::move(const IntPoint& point)
{
    int viewX, viewY, viewWidth, viewHeight;
    evas_object_geometry_get(m_view->evasObject(), &viewX, &viewY, &viewWidth, &viewHeight);

    int xPosition = point.x();
    if (xPosition < viewX + (m_width / 2))
        xPosition = viewX + m_width / 2 + s_magnifierMargin;
    if (xPosition > viewX + viewWidth - (m_width / 2))
        xPosition = viewX + viewWidth - (m_width / 2) - s_magnifierMargin;

    int yPosition = point.y() - ((float)s_magnifierOffsetY * ((float)m_edjeHeight / s_defaultEdjeHeight));
    if (yPosition < viewY)
        yPosition = viewY;
    if (yPosition > viewY + viewHeight)
        yPosition = viewY + viewHeight;

    evas_object_move(m_magnifier, xPosition, yPosition);
}

bool TextSelectionMagnifier::isVisible()
{
    return evas_object_visible_get(m_magnifier);
}

} // namespace WebKit

#endif // TIZEN_TEXT_SELECTION
