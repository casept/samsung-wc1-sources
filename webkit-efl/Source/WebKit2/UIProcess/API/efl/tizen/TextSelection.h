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

#ifndef TextSelection_h
#define TextSelection_h

#if ENABLE(TIZEN_TEXT_SELECTION)

#include "TextSelectionHandle.h"
#include "TextSelectionMagnifier.h"
#include "WebPageProxy.h"
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

class EwkView;

namespace WebKit {

class TextSelectionHandle;
class TextSelectionMagnifier;

class TextSelection {
public:
    static PassOwnPtr<TextSelection> create(EwkView* view)
    {
        return adoptPtr(new TextSelection(view));
    }
    ~TextSelection();

    enum HandleMovingDirection {
        HandleMovingDirectionNone = 0,
        HandleMovingDirectionNormal,
        HandleMovingDirectionReverse,
    };

    void update();
    bool isTextSelectionDowned() { return m_isTextSelectionDowned; }
    void setIsTextSelectionDowned(bool isTextSelectionDowned) { m_isTextSelectionDowned = isTextSelectionDowned; }
    bool isTextSelectionMode() { return m_isTextSelectionMode; }
    void setIsTextSelectionMode(bool isTextSelectionMode);
    bool isMagnifierVisible();
    void updateHandlesAndContextMenu(bool isShow, bool isScrolling = false);
    bool isEnabled();
    bool isAutomaticClearEnabled();
    bool textSelectionDown(const WebCore::IntPoint& point);
    void textSelectionMove(const WebCore::IntPoint& point);
    void textSelectionUp(const WebCore::IntPoint& point, bool isStaredTextSelectionFromOutside = false);

    // handle callback
    void handleMouseDown(WebKit::TextSelectionHandle* handle, const WebCore::IntPoint& position);
    void handleMouseMove(WebKit::TextSelectionHandle* handle, const WebCore::IntPoint& position);
    void handleMouseUp(WebKit::TextSelectionHandle* handle, const WebCore::IntPoint& position);

    //bool isTextSelectionHandleDowned() { return m_leftHandle->isMouseDowned() || m_rightHandle->isMouseDowned(); }
    bool isTextSelectionHandleDowned();
#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    void textSelectionHandleDown(const WebCore::IntPoint& position);
    void textSelectionHandleMove(const WebCore::IntPoint& position);
    void textSelectionHandleUp();
#endif

    void requestToShow();
    void initHandlesMouseDownedStatus();

    void changeContextMenuPosition(WebCore::IntPoint& position);

    friend class EwkView; // to allow hideHandlers() call while zooming
private:
    TextSelection(EwkView*);
    void clear();
    void hide();
    void updateHandlers();
    void hideHandlers();
    void reloadMagnifier();
    void showMagnifier();
    void hideMagnifier();
    void updateMagnifier(const WebCore::IntPoint& position);
    void showContextMenu();
    void hideContextMenu();
    void setLeftSelectionToEvasPoint(const WebCore::IntPoint& evasPoint);
    void setRightSelectionToEvasPoint(const WebCore::IntPoint& evasPoint);

    void startMoveAnimator();
    void stopMoveAnimator();
    static void onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo);
    static Eina_Bool moveAnimatorCallback(void*);

#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    TextSelectionHandle* getSelectedHandle(const WebCore::IntPoint& position);
#endif

    static Eina_Bool showTimerCallback(void* data);
    void showHandlesAndContextMenu();
    void scrollContentWithSelectionIfRequired(const WebCore::IntPoint& evasPoint);
    int getSelectionScrollingStatus() { return m_isSelectionInScrollingMode; }
    void setSelectionScrollingStatus(bool status) { m_isSelectionInScrollingMode = status; }

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    void informTextStyleState();
#endif

private:
    EwkView* m_view;
    TextSelectionHandle* m_leftHandle;
    TextSelectionHandle* m_rightHandle;
    TextSelectionMagnifier* m_magnifier;
    bool m_isTextSelectionDowned;
    bool m_isTextSelectionMode;
    Ecore_Animator* m_moveAnimator;
    Ecore_Timer* m_showTimer;
    WebCore::IntRect m_lastLeftHandleRect;
    WebCore::IntRect m_lastRightHandleRect;
#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    TextSelectionHandle* m_selectedHandle;
#endif
    int m_handleMovingDirection;
    bool m_isSelectionInScrollingMode;
};

} // namespace WebKit

#endif // TIZEN_TEXT_SELECTION
#endif // TextSelection_h
