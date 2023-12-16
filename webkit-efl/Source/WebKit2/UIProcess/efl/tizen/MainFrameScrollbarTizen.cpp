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
#include "MainFrameScrollbarTizen.h"

#include "ewk_view.h"
#include <Edje.h>
#include <Evas.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(TIZEN_SCROLLBAR)

static const int horizontalScrollbarID = 1;
static const int verticalScrollbarID = 1500100900;

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_TV_PROFILE)
static void handleEdjeMessage(void* userData, Evas_Object*, Edje_Message_Type messageType, int messageId, void* message)
{
    if (messageType != EDJE_MESSAGE_FLOAT)
        return;

    Edje_Message_Float* floatMessage = reinterpret_cast<Edje_Message_Float*>(message);
    MainFrameScrollbarTizen* mainFrameScrollbarTizen = reinterpret_cast<MainFrameScrollbarTizen*>(userData);

    int newScrollPosition = (mainFrameScrollbarTizen->totalSize() - mainFrameScrollbarTizen->visibleSize()) * floatMessage->val;

    if (messageId == horizontalScrollbarID)
        ewk_view_scroll_set(mainFrameScrollbarTizen->ewkView(), newScrollPosition, 0);
    else if (messageId == verticalScrollbarID)
        ewk_view_scroll_set(mainFrameScrollbarTizen->ewkView(), 0, newScrollPosition);
}
#endif

PassRefPtr<MainFrameScrollbarTizen> MainFrameScrollbarTizen::createNativeScrollbar(Evas_Object* ewkView, ScrollbarOrientation orientation)
{
    return adoptRef(new MainFrameScrollbarTizen(ewkView, orientation));
}

MainFrameScrollbarTizen::MainFrameScrollbarTizen(Evas_Object* view, ScrollbarOrientation orientation)
    : m_view(view)
    , m_position(0)
    , m_totalSize(0)
    , m_visibleSize(0)
    , m_isEnabled(false)
{
    ASSERT(m_view);

    String m_theme = ewk_view_theme_get(m_view);

    m_object = edje_object_add(evas_object_evas_get(m_view));
    if (!m_object)
        return;

    const char* group = (orientation == HorizontalScrollbar) ? "scrollbar.horizontal" : "scrollbar.vertical";
    if (!edje_object_file_set(m_object, m_theme.utf8().data(), group))
        return;

    evas_object_smart_member_add(m_object, m_view);

#if ENABLE(TIZEN_TV_PROFILE)
    edje_object_message_handler_set(m_object, handleEdjeMessage, this);
#endif
}

MainFrameScrollbarTizen::~MainFrameScrollbarTizen()
{
    evas_object_smart_member_del(m_object);
}

void MainFrameScrollbarTizen::updateThumbPositionAndProportion()
{
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[sizeof(Edje_Message_Float_Set) + sizeof(double)]);
    Edje_Message_Float_Set* message = new(buffer.get()) Edje_Message_Float_Set;
    message->count = 2;

    if (m_totalSize - m_visibleSize > 0)
        message->val[0] = static_cast<double>(m_position) / static_cast<double>(m_totalSize - m_visibleSize);
    else
        message->val[0] = 0.0f;

    if (m_totalSize > 0)
        message->val[1] = static_cast<double>(m_visibleSize) / static_cast<double>(m_totalSize);
    else
        message->val[1] = 0.0f;

    edje_object_message_send(m_object, EDJE_MESSAGE_FLOAT_SET, 0, message);

    // Process scrollbar's message queue in order to trigger scrollbar's rendering here
    // because we want to scroll contents and scrollbar at a time.
    // If we do not process message queue, scrollbar will be rendered in the next frame even though
    // scroll is already moved. We can not recognize that by seeing, but it can be recognized by
    // testing using video camera (240 FPS).
    edje_object_message_signal_process(m_object);
}

void MainFrameScrollbarTizen::setFrameRect(const WebCore::IntRect& rect)
{
    if (m_frameRect == rect)
        return;

    m_frameRect = rect;
    frameRectChanged();
}

void MainFrameScrollbarTizen::setPosition(int position)
{
    m_position = position;

    if (m_isEnabled && m_position >= 0)
        updateThumbPositionAndProportion();
}

void MainFrameScrollbarTizen::setProportion(int visibleSize, int totalSize)
{
    m_visibleSize = visibleSize;
    m_totalSize = totalSize;
}

void MainFrameScrollbarTizen::frameRectChanged()
{
    int x, y;
    evas_object_geometry_get(m_view, &x, &y, 0, 0);
    evas_object_move(m_object, x + m_frameRect.x(), y + m_frameRect.y());
    evas_object_resize(m_object, m_frameRect.width(), m_frameRect.height());
}

void MainFrameScrollbarTizen::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
        return;

    m_isEnabled = enabled;
    m_isEnabled ? evas_object_show(m_object) : evas_object_hide(m_object);
}

} // namespace WebKit
#endif // ENABLE(TIZEN_SCROLLBAR)
