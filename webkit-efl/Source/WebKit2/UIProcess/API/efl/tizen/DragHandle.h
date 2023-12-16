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

#ifndef DragHandle_h
#define DragHandle_h

#if ENABLE(TIZEN_DRAG_SUPPORT)

#include "Drag.h"
#include <Ecore.h>
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class Drag;

class DragHandle {
public:
    DragHandle(Evas_Object* object, const String& theme, const String& path, Drag* drag);
    ~DragHandle();

    void move(const WebCore::IntPoint& point);
    void show();
    void hide();
    void updateHandleIcon(bool isDropAble);
    const WebCore::IntPoint position() const { return m_mousePosition; }
    Evas_Object* cancelIcon() { return m_cancelIcon; }

private:
    // callbacks
    static void cancelMouseDown(void*, Evas*, Evas_Object*, void*);
    static void cancelMouseUp(void*, Evas*, Evas_Object*, void*);
    static void mouseDown(void*, Evas*, Evas_Object*, void*);
    static void mouseMove(void*, Evas*, Evas_Object*, void*);
    static void mouseUp(void*, Evas*, Evas_Object*, void*);
    static void update(void*);

    void setMousePosition(const WebCore::IntPoint& position) { m_mousePosition = position; }

private:
    Evas_Object* m_view;
    Evas_Object* m_icon;
    Evas_Object* m_cancelIcon;
    Evas_Object* m_dragImage;
    double m_dragImageWidth;
    double m_dragImageHeight;
    WebCore::IntPoint m_mousePosition;
    Drag* m_drag;
    Ecore_Job* m_dragJob;
};

} // namespace WebKit

#endif // TIZEN_DRAG_SUPPORT
#endif // DragHandle_h
