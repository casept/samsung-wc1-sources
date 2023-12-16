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
#include "TextSelectionHandle.h"

#include <Edje.h>

using namespace WebCore;

namespace WebKit {

Ecore_Job* TextSelectionHandle::s_job = 0;

TextSelectionHandle::TextSelectionHandle(Evas_Object* object, const char* themePath, const char* part, bool isLeft, TextSelection* textSelection)
    : m_widget(object)
    , m_isLeft(isLeft)
    , m_textSelection(textSelection)
    , m_position(IntPoint(0, 0))
    , m_isMouseDowned(false)
    , m_isTop(false)
{
    Evas* evas = evas_object_evas_get(object);
    m_icon = edje_object_add(evas);

    if (!m_icon)
        return;

    if (!edje_object_file_set(m_icon, themePath, part))
        return;

    edje_object_signal_emit(m_icon, "edje,focus,in", "edje");
    edje_object_signal_emit(m_icon, "elm,state,bottom", "elm");
    evas_object_smart_member_add(m_icon, object);

    evas_object_propagate_events_set(m_icon, false);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_DOWN, onMouseDown, this);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_MOVE, onMouseMove, this);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_UP, onMouseUp, this);
}

TextSelectionHandle::~TextSelectionHandle()
{
    evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_DOWN, onMouseDown);
    evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_MOVE, onMouseMove);
    evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_UP, onMouseUp);
    evas_object_del(m_icon);
    if (s_job) {
        ecore_job_del(s_job);
        s_job = 0;
    }
}

void TextSelectionHandle::move(const IntPoint& point, const IntRect& selectionRect)
{
    IntPoint movePoint = point;
    const int reverseMargin = 32;
    int x, y, deviceWidth, deviceHeight;
    int handleHeight;

    edje_object_part_geometry_get(m_icon, "handle", 0, 0, 0, &handleHeight);
    evas_object_geometry_get(m_widget, &x, &y, &deviceWidth, &deviceHeight);

    if ((m_isLeft && (movePoint.x() <= reverseMargin)) || (!m_isLeft && (movePoint.x() >= deviceWidth - reverseMargin))) {
        if ((movePoint.y() + handleHeight) > (y + deviceHeight)) {
            movePoint.setY(movePoint.y() - selectionRect.height());
            edje_object_signal_emit(m_icon, "elm,state,top,reversed", "elm");
            m_isTop = true;
        } else {
            edje_object_signal_emit(m_icon, "elm,state,bottom,reversed", "elm");
            m_isTop = false;
        }
    }
    else {
        if ((movePoint.y() + handleHeight) > (y + deviceHeight)) {
            movePoint.setY(movePoint.y() - selectionRect.height());
            edje_object_signal_emit(m_icon, "elm,state,top", "elm");
            m_isTop = true;
        } else {
            edje_object_signal_emit(m_icon, "elm,state,bottom", "elm");
            m_isTop = false;
        }
    }

    evas_object_move(m_icon, movePoint.x(), movePoint.y());
    m_position = movePoint;

}

void TextSelectionHandle::show()
{
    evas_object_show(m_icon);
}

void TextSelectionHandle::hide()
{
    evas_object_hide(m_icon);
}

#if 1//ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
const IntRect TextSelectionHandle::getHandleRect()
{
    if (!evas_object_visible_get(m_icon))
        return IntRect();

    int x, y;
    evas_object_geometry_get(m_icon, &x, &y, 0, 0);

    int partX, partY, partWidth, partHeight;
    edje_object_part_geometry_get(m_icon, "handle", &partX, &partY, &partWidth, &partHeight);

    WebCore::IntRect handleRect(x + partX, y + partY, partWidth, partHeight);
    return handleRect;
}
#endif

// callbacks
void TextSelectionHandle::mouseDown(const IntPoint& point)
{
#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    edje_object_signal_emit(m_icon, "mouse,down,1", "handle");
#endif

    setIsMouseDowned(true);
    setFirstDownMousePosition(point);
    setMousePosition(point);
    m_textSelection->handleMouseDown(this, m_mousePosition);
}

void TextSelectionHandle::mouseMove(const IntPoint& point)
{
    if (!m_textSelection->isMagnifierVisible())
        return;

    setMousePosition(point);

    if (!s_job)
       s_job = ecore_job_add(update, this);
}

void TextSelectionHandle::mouseUp()
{
#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    edje_object_signal_emit(m_icon, "mouse,up,1", "handle");
#endif

    if (m_textSelection->isEnabled())
        setIsMouseDowned(false);

    m_textSelection->handleMouseUp(this, m_mousePosition);
}

void TextSelectionHandle::onMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Down* event = static_cast<Evas_Event_Mouse_Down*>(eventInfo);
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    handle->mouseDown(IntPoint(event->canvas.x, event->canvas.y));
}

void TextSelectionHandle::onMouseMove(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Move* event = static_cast<Evas_Event_Mouse_Move*>(eventInfo);
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    handle->mouseMove(IntPoint(event->cur.canvas.x, event->cur.canvas.y));
}

void TextSelectionHandle::onMouseUp(void* data, Evas*, Evas_Object*, void* /*eventInfo*/)
{
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    handle->mouseUp();
}

// job
void TextSelectionHandle::update(void* data)
{
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);
    int deltaX = handle->m_mousePosition.x() - handle->m_firstDownMousePosition.x();
    int deltaY = handle->m_mousePosition.y() - handle->m_firstDownMousePosition.y();

    if (deltaX || deltaY) {
        WebCore::IntPoint movePosition;

        movePosition.setX(handle->m_basePositionForMove.x() + deltaX);
        movePosition.setY(handle->m_basePositionForMove.y() + deltaY);

        handle->m_textSelection->handleMouseMove(handle, movePosition);
    }
    TextSelectionHandle::s_job = 0;
}

} // namespace WebKit

#endif // TIZEN_TEXT_SELECTION
