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

#ifndef TextSelectionHandle_h
#define TextSelectionHandle_h

#if ENABLE(TIZEN_TEXT_SELECTION)

#include "TextSelection.h"
#include <Ecore.h>
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <WebCore/IntRect.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class TextSelection;

class TextSelectionHandle {
public:
    TextSelectionHandle(Evas_Object* object, const char* path, const char* part, bool isLeft, TextSelection* textSelection);
    ~TextSelectionHandle();

    void move(const WebCore::IntPoint& point, const WebCore::IntRect& selectionRect);
    void show();
    void hide();
    bool isLeft() const { return m_isLeft; }
    bool isRight() const { return !m_isLeft; }
    const WebCore::IntPoint position() const { return m_position; }
    void setBasePositionForMove(const WebCore::IntPoint& position) { m_basePositionForMove = position; }
    bool isMouseDowned() { return m_isMouseDowned; }
    bool setIsMouseDowned(bool isMouseDowned) { return m_isMouseDowned = isMouseDowned; }
#if 1//ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    const WebCore::IntRect getHandleRect();
#endif
    bool isTop() const { return m_isTop; }

    void mouseDown(const WebCore::IntPoint& point);
    void mouseMove(const WebCore::IntPoint& point);
    void mouseUp();

private:
    // callbacks
    static void onMouseDown(void*, Evas*, Evas_Object*, void*);
    static void onMouseMove(void*, Evas*, Evas_Object*, void*);
    static void onMouseUp(void*, Evas*, Evas_Object*, void*);
    static void update(void*);

    void setFirstDownMousePosition(const WebCore::IntPoint& position) { m_firstDownMousePosition = position; }
    void setMousePosition(const WebCore::IntPoint& position) { m_mousePosition = position; }

private:
    Evas_Object* m_icon;
    Evas_Object* m_widget;
    WebCore::IntPoint m_mousePosition;
    static Ecore_Job* s_job;
    bool m_isLeft;
    TextSelection* m_textSelection;
    WebCore::IntPoint m_position;
    WebCore::IntPoint m_firstDownMousePosition;
    WebCore::IntPoint m_basePositionForMove;
    bool m_isMouseDowned;
    bool m_isTop;
};

} // namespace WebKit

#endif // TIZEN_TEXT_SELECTION
#endif // TextSelectionHandle_h
