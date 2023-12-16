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

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "DragHandle.h"
#include "EwkView.h"
#include "ewk_util.h"
#include <Edje.h>
#include <Elementary.h>

using namespace WebCore;

namespace WebKit {

const int s_iconOffset = 90;
const int s_widthMargin = 20;
const int s_heightMargin = 10;
const int s_cancelImageXMargin = 26;
const int s_cancelImageYMargin = 36;
const double s_minIconWidth = 110;
const double s_minIconHeight = 100;
const double s_minImageLength = 90;
const double s_maxImageLength = 300;


DragHandle::DragHandle(Evas_Object* object, const String& theme, const String& path, Drag* drag)
    : m_view(object)
    , m_drag(drag)
    , m_dragJob(0)
{
    Evas_Object* topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(object));
    if (!topWidget)
        topWidget = m_view;

    m_icon = elm_layout_add(topWidget);
    m_cancelIcon = elm_layout_add(topWidget);

    if (!m_icon || !m_cancelIcon)
        return;

    if (!elm_layout_file_set(m_icon, theme.utf8().data(), path.utf8().data()) || !elm_layout_file_set(m_cancelIcon, theme.utf8().data(), "drag_cancel"))
        return;

    evas_object_smart_member_add(m_icon, m_view);
    evas_object_smart_member_add(m_cancelIcon, m_view);

    evas_object_propagate_events_set(m_icon, false);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_DOWN, mouseDown, this);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_MOVE, mouseMove, this);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_UP, mouseUp, this);
    evas_object_propagate_events_set(m_cancelIcon, false);
    evas_object_event_callback_add(m_cancelIcon, EVAS_CALLBACK_MOUSE_DOWN, cancelMouseDown, this);
    evas_object_event_callback_add(m_cancelIcon, EVAS_CALLBACK_MOUSE_UP, cancelMouseUp, this);
}

DragHandle::~DragHandle()
{
    if (m_dragJob) {
        ecore_job_del(m_dragJob);
        m_dragJob = 0;
    }

    if (m_icon) {
        evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_DOWN, mouseDown);
        evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_MOVE, mouseMove);
        evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_UP, mouseUp);
        evas_object_del(m_icon);
    }

    if (m_cancelIcon) {
        evas_object_event_callback_del(m_cancelIcon, EVAS_CALLBACK_MOUSE_DOWN, cancelMouseDown);
        evas_object_event_callback_del(m_cancelIcon, EVAS_CALLBACK_MOUSE_UP, cancelMouseUp);
        evas_object_del(m_cancelIcon);
    }
}

void DragHandle::move(const IntPoint& point)
{
    if (m_icon) {
        Evas_Coord x, y, width, height;
        evas_object_geometry_get(m_view, &x, &y, &width, &height);

        double dragX, dragY;
        if ((point.x() - m_dragImageWidth / 2) < s_cancelImageXMargin)
            dragX = s_cancelImageXMargin;
        else if ((point.x() - m_dragImageWidth / 2) > (width - m_dragImageWidth))
            dragX = width - m_dragImageWidth;
        else
            dragX = point.x() - m_dragImageWidth / 2;
        if ((point.y() - m_dragImageHeight / 2) < (y + s_cancelImageYMargin))
            dragY = y + s_cancelImageYMargin;
        else if ((point.y() - m_dragImageHeight / 2) > (y + height - m_dragImageHeight))
            dragY = y + height - m_dragImageHeight;
        else
            dragY = point.y() - m_dragImageHeight / 2;

        evas_object_move(m_icon, dragX, dragY);
        evas_object_move(m_cancelIcon, dragX - s_cancelImageXMargin, dragY - s_cancelImageYMargin);
    }
}

void DragHandle::show()
{
    if (m_icon) {
        if (m_drag->dragImage()) {
            elm_object_signal_emit(m_icon, "dragndrop,link,hide", "dragndrop");
            elm_object_signal_emit(m_icon, "dragndrop,image,show", "dragndrop");

            m_dragImageWidth = cairo_image_surface_get_width(m_drag->dragImage()->createCairoSurface().get());
            m_dragImageHeight = cairo_image_surface_get_height(m_drag->dragImage()->createCairoSurface().get());

            if (m_dragImage)
                m_dragImage = 0;

            if (m_dragImageWidth < s_minImageLength) {
                m_dragImageWidth *= 1.5;
                m_dragImageHeight *= 1.5;
            }

            if (m_dragImageHeight > s_maxImageLength) {
                m_dragImageWidth = m_dragImageWidth * (s_maxImageLength / m_dragImageHeight);
                m_dragImageHeight = s_maxImageLength;
            }
            if (m_dragImageWidth > s_maxImageLength) {
                m_dragImageHeight = m_dragImageHeight * (s_maxImageLength / m_dragImageWidth);
                m_dragImageWidth = s_maxImageLength;
            }

            if (m_dragImageWidth < s_minImageLength)
                m_dragImageWidth = s_minImageLength;
            if (m_dragImageHeight < s_minImageLength)
                m_dragImageHeight = s_minImageLength;

            m_dragImage = ewk_util_image_from_cairo_surface_add(evas_object_evas_get(m_drag->ewkView()->evasObject()), m_drag->dragImage()->createCairoSurface().get());
            evas_object_size_hint_min_set(m_dragImage, m_dragImageWidth, m_dragImageHeight);
            evas_object_resize(m_dragImage, m_dragImageWidth, m_dragImageHeight);
            evas_object_image_filled_set(m_dragImage, EINA_TRUE);
            evas_object_show(m_dragImage);

            m_dragImageWidth += s_widthMargin;
            m_dragImageHeight += s_heightMargin;

            evas_object_resize(m_icon, m_dragImageWidth, m_dragImageHeight);
            elm_object_part_content_set(m_icon, "swallow", m_dragImage);
        } else {
            elm_object_signal_emit(m_icon, "dragndrop,image,hide", "dragndrop");
            elm_object_signal_emit(m_icon, "dragndrop,link,show", "dragndrop");

            m_dragImageWidth = s_minIconWidth;
            m_dragImageHeight = s_minIconHeight;

            evas_object_resize(m_icon, m_dragImageWidth, m_dragImageHeight);
        }
        evas_object_show(m_icon);
        evas_object_show(m_cancelIcon);
    }
}

void DragHandle::hide()
{
    if (m_icon) {
        evas_object_hide(m_icon);
        evas_object_hide(m_cancelIcon);

        elm_object_signal_emit(m_icon, "dragndrop,drop,out", "dragndrop");
        elm_object_signal_emit(m_cancelIcon, "dragndrop,cancel,default", "dragndrop");
    }
}

void DragHandle::updateHandleIcon(bool isDropAble)
{
    if(!m_icon)
        return;

    if (isDropAble)
        elm_object_signal_emit(m_icon, "dragndrop,drop,in", "dragndrop");
    else
        elm_object_signal_emit(m_icon, "dragndrop,drop,out", "dragndrop");
}

// callbacks
void DragHandle::cancelMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    DragHandle* handle = static_cast<DragHandle*>(data);
    elm_object_signal_emit(handle->cancelIcon(), "dragndrop,cancel,press", "dragndrop");
}

void DragHandle::cancelMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    DragHandle* handle = static_cast<DragHandle*>(data);
    elm_object_signal_emit(handle->cancelIcon(), "dragndrop,cancel,default", "dragndrop");
    handle->hide();
}

void DragHandle::mouseDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Down* event = static_cast<Evas_Event_Mouse_Down*>(eventInfo);
    DragHandle* handle = static_cast<DragHandle*>(data);

    handle->setMousePosition(IntPoint(event->canvas.x, event->canvas.y - s_iconOffset));
}

void DragHandle::mouseMove(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Move* event = static_cast<Evas_Event_Mouse_Move*>(eventInfo);
    DragHandle* handle = static_cast<DragHandle*>(data);

    handle->setMousePosition(IntPoint(event->cur.canvas.x, event->cur.canvas.y - s_iconOffset));

    if (!handle->m_dragJob)
       handle->m_dragJob = ecore_job_add(update, data);
}

void DragHandle::mouseUp(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    DragHandle* handle = static_cast<DragHandle*>(data);
    handle->m_drag->handleMouseUp(handle);
}

// job
void DragHandle::update(void* data)
{
    DragHandle* handle = static_cast<DragHandle*>(data);
    handle->m_drag->handleMouseMove(handle);
    handle->m_dragJob = 0;
}

} // namespace WebKit

#endif // TIZEN_DRAG_SUPPORT
