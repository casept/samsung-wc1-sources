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
#include "TextSelection.h"

#include "EditorState.h"
#include "EwkView.h"
#include "NativeWebMouseEvent.h"
#include "ewk_view.h"
#include "ewk_view_private.h"
#include <Elementary.h>

using namespace WebCore;

namespace WebKit {

TextSelection::TextSelection(EwkView* view)
      : m_view(view)
      , m_isTextSelectionDowned(false)
      , m_isTextSelectionMode(false)
      , m_moveAnimator(0)
      , m_showTimer(0)
#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
      , m_selectedHandle(0)
#endif
      , m_handleMovingDirection(HandleMovingDirectionNormal)
      , m_isSelectionInScrollingMode(false)
{
    ASSERT(view);

    const Eina_List* defaultThemeList = elm_theme_list_get(0);

    const Eina_List* l;
    void* theme;
    EINA_LIST_FOREACH(defaultThemeList, l, theme) {
        char* themePath = elm_theme_list_item_path_get((const char*)theme, 0);

        if (themePath) {
            m_leftHandle = new TextSelectionHandle(m_view->evasObject(), themePath, "elm/entry/selection/block_handle_left", true, this);
            m_rightHandle = new TextSelectionHandle(m_view->evasObject(), themePath, "elm/entry/selection/block_handle_right", false, this);

            free(themePath);
            break;
        }
    }

    m_magnifier = new TextSelectionMagnifier(m_view);

    evas_object_event_callback_add(m_view->evasObject(), EVAS_CALLBACK_MOUSE_UP, onMouseUp, this);
}

TextSelection::~TextSelection()
{
    if (m_leftHandle)
        delete m_leftHandle;

    if (m_rightHandle)
        delete m_rightHandle;

    delete m_magnifier;

    if (m_moveAnimator) {
        ecore_animator_del(m_moveAnimator);
        m_moveAnimator = 0;
    }

    if (m_showTimer) {
        ecore_timer_del(m_showTimer);
        m_showTimer = 0;
    }
    evas_object_event_callback_del(m_view->evasObject(), EVAS_CALLBACK_MOUSE_UP, onMouseUp);
}

void TextSelection::update()
{
    EditorState editorState = m_view->page()->editorState();
    if (editorState.updateEditorRectOnly)
        return;

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    if (!editorState.shouldIgnoreCompositionSelectionChange && ewk_settings_text_style_state_enabled_get(ewk_view_settings_get(m_view->evasObject())))
        informTextStyleState();
#endif

    if (isTextSelectionMode()) {
        if (!editorState.selectionIsRange) {
            if (editorState.isContentEditable && !evas_object_focus_get(m_view->evasObject())) {
                const EditorState& editor = m_view->page()->editorState();
                IntRect caretRect;
                if (!editor.selectionIsNone && !editor.selectionIsRange)
                    caretRect = editor.selectionRect;

                if (!caretRect.isEmpty())
                    return;
            } else {
                WebCore::IntRect leftRect;
                WebCore::IntRect rightRect;
                if (isTextSelectionDowned() || m_view->page()->getSelectionHandlers(leftRect, rightRect))
                    return;

                setIsTextSelectionMode(false);
            }
        } else {
            if (!isTextSelectionDowned() && !isTextSelectionHandleDowned()) {
                updateHandlers();
                showContextMenu();
            }
        }
    }
}

void TextSelection::setIsTextSelectionMode(bool isTextSelectionMode)
{
    if (!isTextSelectionMode) {
        hide();
        clear();
        initHandlesMouseDownedStatus();
        setIsTextSelectionDowned(false);
    }

    m_isTextSelectionMode = isTextSelectionMode;
}

void TextSelection::clear()
{
    EditorState editorState = m_view->page()->editorState();
    if (!editorState.selectionIsRange)
        return;

    m_view->page()->selectionRangeClear();
}

void TextSelection::hide()
{
    hideHandlers();
    hideMagnifier();
    hideContextMenu();
}

void TextSelection::updateHandlers()
{
    WebCore::IntRect leftRect, rightRect;
    if (!m_view->page()->getSelectionHandlers(leftRect, rightRect))
        return;

    m_lastLeftHandleRect = leftRect;
    m_lastRightHandleRect = rightRect;

    AffineTransform toEvasTransform = m_view->webView()->transformToScene();
    WebCore::IntPoint leftEvasPoint = toEvasTransform.mapPoint(leftRect.minXMaxYCorner());
    WebCore::IntPoint rightEvasPoint = toEvasTransform.mapPoint(rightRect.maxXMaxYCorner());

    TextSelectionHandle* shownLeftHandle = m_leftHandle;
    TextSelectionHandle* shownRightHandle = m_rightHandle;

    if (m_handleMovingDirection == HandleMovingDirectionReverse) {
        shownLeftHandle = m_rightHandle;
        shownRightHandle = m_leftHandle;
    }

    EditorState editorState = m_view->page()->editorState();
    if (editorState.isContentEditable) {
        shownLeftHandle->hide();
        shownRightHandle->hide();

        WebCore::IntRect editorRect = editorState.editorRect;
        WebCore::IntPoint editorLeftEvasPoint = toEvasTransform.mapPoint(editorRect.location());
        WebCore::IntPoint editorRightEvasPoint = toEvasTransform.mapPoint(editorRect.maxXMaxYCorner());
        int webViewX, webViewY, webViewWidth, webViewHeight;

        evas_object_geometry_get(m_view->evasObject(), &webViewX, &webViewY, &webViewWidth, &webViewHeight);
        if ((editorLeftEvasPoint.x() <= leftEvasPoint.x() && editorLeftEvasPoint.y() <= leftEvasPoint.y())
            && (webViewX <= leftEvasPoint.x() && webViewY <= leftEvasPoint.y())) {
            shownLeftHandle->move(leftEvasPoint, m_view->webView()->transformToScene().mapRect(leftRect));
            shownLeftHandle->show();
        }

        if ((editorRightEvasPoint.x() >= rightEvasPoint.x() && editorRightEvasPoint.y() >= rightEvasPoint.y())
            && ((webViewX + webViewWidth) >= rightEvasPoint.x() && (webViewY <= rightEvasPoint.y() && (webViewY + webViewHeight) >= rightEvasPoint.y()))) {
            shownRightHandle->move(rightEvasPoint, m_view->webView()->transformToScene().mapRect(rightRect));
            shownRightHandle->show();
        }
    } else {
        shownLeftHandle->move(leftEvasPoint, m_view->webView()->transformToScene().mapRect(leftRect));
        shownLeftHandle->show();

        shownRightHandle->move(rightEvasPoint, m_view->webView()->transformToScene().mapRect(rightRect));
        shownRightHandle->show();
    }
}

void TextSelection::hideHandlers()
{
    m_leftHandle->hide();
    m_rightHandle->hide();
}

void TextSelection::showMagnifier()
{
    m_magnifier->show();
}

void TextSelection::hideMagnifier()
{
    m_magnifier->hide();
}

void TextSelection::updateMagnifier(const IntPoint& position)
{
    m_magnifier->update(position);
    m_magnifier->move(position);
}

void TextSelection::showContextMenu()
{
    if (!isEnabled())
        return;

    EditorState editorState = m_view->page()->editorState();
    if (!editorState.selectionIsRange && !editorState.isContentEditable)
        return;

    WebCore::IntPoint point;
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    bool isPresentInViewPort = false;
#endif
    if (editorState.selectionIsRange) {
        WebCore::IntRect leftRect, rightRect;
        if (!m_view->page()->getSelectionHandlers(leftRect, rightRect))
            return;

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
        // Checking if this point is in viewport area. If the calcualated
        // point/Left/Right point are in view port then draw else do not draw the
        // context menu. Only draw the selection points.
        FloatRect unscaledRect = FloatRect(m_view->visibleContentRect());
        unscaledRect.scale(1 / m_view->page()->scaleFactor());
        IntRect viewportRect = enclosingIntRect(unscaledRect);

        WebCore::IntPoint visiblePoint = leftRect.center();
        if (viewportRect.contains(visiblePoint)) {
            // First check That the modified points are present in view port
            point = visiblePoint;
            isPresentInViewPort = true;
        } else if (viewportRect.contains(leftRect.location())) {
            // else if the calculated point is not in the view port area the
            // draw context menu at left point if visible
            point = leftRect.location();
            isPresentInViewPort = true;
        } else if (viewportRect.contains(rightRect.maxXMinYCorner())) {
            // else if the calculated point is not in the view port area the
            // draw context menu at right point if visible
            point = rightRect.maxXMinYCorner();
            isPresentInViewPort = true;
        }

        if (isPresentInViewPort && editorState.isContentEditable) {
            // In case of single line editor box.
            if (leftRect == rightRect) {
                // draw context menu at center point of visible selection range in the editor box
                IntRect editorRect = editorState.editorRect;
                leftRect.intersect(editorRect);
                if (!leftRect.isEmpty())
                    point = leftRect.center();
                else {
                    // not draw context menu if there is no visible selection range in the editor box
                    isPresentInViewPort = false;
                }
            }
        }
#else
        point = leftRect.center();
#endif
    } else if (editorState.isContentEditable) {
        const EditorState& editor = m_view->page()->editorState();
        IntRect caretRect;
        if (!editor.selectionIsNone && !editor.selectionIsRange)
            caretRect = editor.selectionRect;

        if (caretRect.isEmpty())
            return;

        point = caretRect.center();
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
        isPresentInViewPort = true;
#endif
    }
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    if (!isPresentInViewPort)
        return;
#endif

    // show context menu if its in viewport else do not show the contextmenu

    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_view->evasObject()));
    if (!smartData || !smartData->api || !smartData->api->mouse_down || !smartData->api->mouse_up)
        return;

    point = m_view->webView()->transformToScene().mapPoint(point);
    Evas* evas = evas_object_evas_get(m_view->evasObject());

    // send mouse down.
    Evas_Event_Mouse_Down mouseDown;
    mouseDown.button = 3;
    mouseDown.output.x = mouseDown.canvas.x = point.x();
    mouseDown.output.y = mouseDown.canvas.y = point.y();
    mouseDown.data = 0;
    mouseDown.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseDown.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseDown.flags = EVAS_BUTTON_NONE;
    mouseDown.timestamp = ecore_time_get() * 1000;
    mouseDown.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseDown.dev = 0;
    smartData->api->mouse_down(smartData, &mouseDown);

    // send mouse up.
    Evas_Event_Mouse_Up mouseUp;
    mouseUp.button = 3;
    mouseUp.output.x = mouseUp.canvas.x = point.x();
    mouseUp.output.y = mouseUp.canvas.y = point.y();
    mouseUp.data = 0;
    mouseUp.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseUp.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseUp.flags = EVAS_BUTTON_NONE;
    mouseUp.timestamp = ecore_time_get() * 1000;
    mouseUp.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseUp.dev = 0;
    smartData->api->mouse_up(smartData, &mouseUp);
}

void TextSelection::hideContextMenu()
{
    if (!isEnabled())
        return;

    m_view->page()->hideContextMenu();
}

void TextSelection::scrollContentWithSelectionIfRequired(const IntPoint& evasPoint)
{
    // Content should get scroll irrespective whether content is editable or not.
    setSelectionScrollingStatus(true);
    IntRect unscaledVisibleContentRect = m_view->visibleContentRect();
    const int scrollClipValue = 50;
    if (evasPoint.y() >= (unscaledVisibleContentRect.height() - scrollClipValue))
        ewk_view_scroll_by(m_view->evasObject(), 0, m_rightHandle->getHandleRect().height());
    else if (evasPoint.y() <= scrollClipValue) {
        IntSize scrollSize(0, m_rightHandle->getHandleRect().height());
        scrollSize.scale(-1);
        ewk_view_scroll_by(m_view->evasObject(), scrollSize.width(), scrollSize.height());
    }

    if (evasPoint.x() >= (unscaledVisibleContentRect.width() - scrollClipValue))
        ewk_view_scroll_by(m_view->evasObject(), m_rightHandle->getHandleRect().width(), 0);
    else if (evasPoint.x() <= scrollClipValue) {
        IntSize scrollSize(m_rightHandle->getHandleRect().width(), 0);
        scrollSize.scale(-1);
        ewk_view_scroll_by(m_view->evasObject(), scrollSize.width(), scrollSize.height());
    }
    setSelectionScrollingStatus(false);
}

void TextSelection::setLeftSelectionToEvasPoint(const IntPoint& evasPoint)
{
    scrollContentWithSelectionIfRequired(evasPoint);
    int result = m_view->page()->setLeftSelection(m_view->webView()->transformFromScene().mapPoint(evasPoint), m_handleMovingDirection);
    if (result)
        m_handleMovingDirection = result;
    updateHandlers();
}

void TextSelection::setRightSelectionToEvasPoint(const IntPoint& evasPoint)
{
    scrollContentWithSelectionIfRequired(evasPoint);
    int result = m_view->page()->setRightSelection(m_view->webView()->transformFromScene().mapPoint(evasPoint), m_handleMovingDirection);
    if (result)
        m_handleMovingDirection = result;
    updateHandlers();
}

// handle callbacks
void TextSelection::handleMouseDown(TextSelectionHandle* handle, const IntPoint& /*position*/)
{
    WebCore::IntPoint basePosition;
    EditorState editorState = m_view->page()->editorState();

    if (editorState.selectionIsRange) {
        WebCore::IntRect leftRect, rightRect;
        if (!m_view->page()->getSelectionHandlers(leftRect, rightRect)) {
            clear();
            return;
        }

        if (handle->isLeft()) {
            basePosition.setX(leftRect.x());
            basePosition.setY(leftRect.y() + (leftRect.height()/2));
        } else {
            basePosition.setX(rightRect.x() + rightRect.width());
            basePosition.setY(rightRect.y() + (rightRect.height()/2));
        }

        handle->setBasePositionForMove(m_view->webView()->transformToScene().mapPoint(basePosition));
    } else
        return;

    hideContextMenu();
    updateMagnifier(handle->position());
    showMagnifier();
}

void TextSelection::handleMouseMove(TextSelectionHandle* handle, const IntPoint& position)
{
    if (handle->isLeft())
        setLeftSelectionToEvasPoint(position);
    else
        setRightSelectionToEvasPoint(position);

    updateMagnifier(handle->position());
}

void TextSelection::handleMouseUp(TextSelectionHandle* /* handle */, const IntPoint& /* position */)
{
    m_handleMovingDirection = HandleMovingDirectionNormal;

    hideMagnifier();
    updateHandlers();
    showContextMenu();
}

bool TextSelection::isTextSelectionHandleDowned()
{
    return m_leftHandle->isMouseDowned() || m_rightHandle->isMouseDowned(); 
}

bool TextSelection::isMagnifierVisible()
{
    return m_magnifier->isVisible();
}

void TextSelection::updateHandlesAndContextMenu(bool isShow, bool isScrolling)
{
    if (getSelectionScrollingStatus())
        return;

    if (isTextSelectionDowned() && !isScrolling) {
        showMagnifier();
        return;
    }

    // FixMe: apdopt to new gesutre module.
/*
    if (m_view->gestureClient->isGestureWorking()) {
        EditorState editorState = m_view->page()->editorState();
        if (!editorState.selectionIsRange && editorState.isContentEditable)
            setIsTextSelectionMode(false);
    }
*/
    if (isShow) {
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
        if (m_view->page()->isClipboardWindowOpened())
            return;
#endif
    // FixMe: apdopt to new gesutre module.
    //    if (m_view->gestureClient->isGestureWorking())
    //        return;

        updateHandlers();
        showContextMenu();
    } else {
        hideHandlers();
        hideContextMenu();
    }

    if (isScrolling && isMagnifierVisible())
        hideMagnifier();
}

void TextSelection::startMoveAnimator()
{
    if (!isEnabled() || !isTextSelectionDowned())
        return;

    if (!m_moveAnimator)
        m_moveAnimator = ecore_animator_add(moveAnimatorCallback, this);
}

void TextSelection::stopMoveAnimator()
{
    if (m_moveAnimator) {
        ecore_animator_del(m_moveAnimator);
        m_moveAnimator = 0;
    }
}

void TextSelection::onMouseUp(void* data, Evas*, Evas_Object*, void* /*eventInfo*/)
{
    static_cast<TextSelection*>(data)->textSelectionUp(IntPoint());
}

Eina_Bool TextSelection::moveAnimatorCallback(void* data)
{
    TextSelection* textSelection = static_cast<TextSelection*>(data);

    Evas_Coord_Point point;
    evas_pointer_canvas_xy_get(evas_object_evas_get(textSelection->m_view->evasObject()), &point.x, &point.y);
    textSelection->textSelectionMove(IntPoint(point.x, point.y));

    return ECORE_CALLBACK_RENEW;
}

// 'return false' means text selection is not possible for point.
// 'return true' means text selection is possible.
bool TextSelection::textSelectionDown(const IntPoint& point)
{
    // text selection should be ignored when longtap on handle from osp
    if (!isEnabled() && isTextSelectionHandleDowned())
        return false;

#if ENABLE(TIZEN_ISF_PORT)
    m_view->inputMethodContext()->resetIMFContext();
#endif
    setIsTextSelectionMode(false);

    IntPoint contentsPoint = m_view->webView()->transformFromScene().mapPoint(point);
    bool result = m_view->page()->selectClosestWord(contentsPoint);
    if (!result)
        return false;

    if (isTextSelectionMode()) {
        hide();
    } else {
        setIsTextSelectionMode(true);
    }
    setIsTextSelectionDowned(true);

    updateMagnifier(point);
    showMagnifier();

    startMoveAnimator();

    return true;
}

static int s_textSelectionMargin = 5;

void TextSelection::textSelectionMove(const IntPoint& point)
{
    // text selection should be ignored when longtap on handle from osp
    if (!isEnabled() && isTextSelectionHandleDowned())
        return;

    if (!isTextSelectionMode()) {
        stopMoveAnimator();
        return;
    }

    WebCore::IntPoint viewPoint;
    EditorState editorState = m_view->page()->editorState();
    bool isInEditablePicker = false;

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    if (editorState.isContentEditable) {
        if (editorState.inputMethodHints == "date"
            || editorState.inputMethodHints == "datetime"
            || editorState.inputMethodHints == "datetime-local"
            || editorState.inputMethodHints == "month"
            || editorState.inputMethodHints == "time"
            || editorState.inputMethodHints == "week")
            isInEditablePicker = true;
    }
#endif

    if (editorState.isContentEditable && !isInEditablePicker) {
        IntRect mapRect = m_view->webView()->transformToScene().mapRect(editorState.editorRect);
        IntPoint updatedPoint = point;
        bool scrolledY = false;
        if (point.y() < mapRect.y()) {
            updatedPoint.setY(mapRect.y() + s_textSelectionMargin);
            if (m_view->page()->scrollContentByLine(point,WebCore::DirectionBackward)) {
                scrolledY = true;
                updateMagnifier(updatedPoint);
            }
        } else if (point.y() > mapRect.maxY()) {
            updatedPoint.setY(mapRect.maxY() - s_textSelectionMargin);
            if (m_view->page()->scrollContentByLine(point,WebCore::DirectionForward)) {
                scrolledY = true;
                updateMagnifier(updatedPoint);
            }
        }

        bool scrolledX = false;
        if (point.x() < mapRect.x()) {
            updatedPoint.setX(mapRect.x() + s_textSelectionMargin);
            if (m_view->page()->scrollContentByCharacter(point,WebCore::DirectionBackward)) {
                scrolledX = true;
                updateMagnifier(updatedPoint);
            }
        } else if (point.x() > mapRect.maxX()) {
            updatedPoint.setX(mapRect.maxX() - s_textSelectionMargin);
            if (m_view->page()->scrollContentByCharacter(point,WebCore::DirectionForward)) {
                scrolledX = true;
                updateMagnifier(updatedPoint);
            }
        }

        if (!scrolledX && !scrolledY) {
            viewPoint = m_view->webView()->transformFromScene().mapPoint(updatedPoint);
            m_view->page()->selectClosestWord(viewPoint);
            updateMagnifier(updatedPoint);
        }
    } else {
        viewPoint = m_view->webView()->transformFromScene().mapPoint(point);
        m_view->page()->selectClosestWord(viewPoint);
        updateMagnifier(point);
    }
    showMagnifier();
}

void TextSelection::textSelectionUp(const IntPoint&, bool isStartedTextSelectionFromOutside)
{
    // text selection should be ignored when longtap on handle from osp
    if (!isEnabled() && isTextSelectionHandleDowned())
        return;

    stopMoveAnimator();

    if (!isTextSelectionMode() || !isTextSelectionDowned())
        return;

    setIsTextSelectionDowned(false);
    hideMagnifier();

    EditorState editorState = m_view->page()->editorState();
    if (editorState.selectionIsRange || editorState.isContentEditable) {
        if (editorState.selectionIsRange)
            updateHandlers();

        showContextMenu();
    } else if (!isStartedTextSelectionFromOutside)
        setIsTextSelectionMode(false);
}

#if ENABLE(TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
TextSelectionHandle* TextSelection::getSelectedHandle(const IntPoint& position)
{
    WebCore::IntRect leftHandleRect = m_leftHandle->getHandleRect();
    if (!leftHandleRect.isEmpty() && leftHandleRect.contains(position))
        return m_leftHandle;

    WebCore::IntRect rightHandleRect = m_rightHandle->getHandleRect();
    if (!rightHandleRect.isEmpty() && rightHandleRect.contains(position))
        return m_rightHandle;

    return 0;
}

void TextSelection::textSelectionHandleDown(const IntPoint& position)
{
    m_selectedHandle = getSelectedHandle(position);
    if (m_selectedHandle)
        m_selectedHandle->mouseDown(position);
    else
        initHandlesMouseDownedStatus();
}

void TextSelection::textSelectionHandleMove(const IntPoint& position)
{
    if (m_selectedHandle && isTextSelectionHandleDowned())
        m_selectedHandle->mouseMove(position);
}

void TextSelection::textSelectionHandleUp()
{
    if (m_selectedHandle && isTextSelectionHandleDowned()) {
        m_selectedHandle->mouseUp();
        m_selectedHandle = 0;
    }
}
#endif

bool TextSelection::isEnabled()
{
    return ewk_settings_text_selection_enabled_get(ewk_view_settings_get(m_view->evasObject()));
}

bool TextSelection::isAutomaticClearEnabled()
{
    return ewk_settings_clear_text_selection_automatically_get(ewk_view_settings_get(m_view->evasObject()));
}

void TextSelection::requestToShow()
{
    if (m_showTimer)
        ecore_timer_del(m_showTimer);
    m_showTimer = ecore_timer_loop_add((double)200.0/1000.0, showTimerCallback, this);
}

Eina_Bool TextSelection::showTimerCallback(void* data)
{
    TextSelection* textSelection = static_cast<TextSelection*>(data);
    textSelection->showHandlesAndContextMenu();

    return ECORE_CALLBACK_RENEW;
}

void TextSelection::showHandlesAndContextMenu()
{
    WebCore::IntRect leftRect, rightRect;
    if (m_view->page()->getSelectionHandlers(leftRect, rightRect)) {
        if ((leftRect == m_lastLeftHandleRect) && (rightRect == m_lastRightHandleRect)) {
            if (m_showTimer) {
                ecore_timer_del(m_showTimer);
                m_showTimer = 0;
            }

            updateHandlesAndContextMenu(true);
        }

        m_lastLeftHandleRect = leftRect;
        m_lastRightHandleRect = rightRect;
    }
}

void TextSelection::initHandlesMouseDownedStatus()
{
    m_leftHandle->setIsMouseDowned(false);
    m_rightHandle->setIsMouseDowned(false);
}

void TextSelection::changeContextMenuPosition(WebCore::IntPoint& position)
{
    if (m_leftHandle->isTop()) {
        IntRect handleRect = m_leftHandle->getHandleRect();
        position.setY(position.y() - handleRect.height());
    }
}

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
void TextSelection::informTextStyleState()
{
    WebCore::IntPoint startPoint, endPoint;
    WebCore::IntRect leftRect, rightRect;

    const EditorState& editor = m_view->page()->editorState();
    IntRect caretRect;
    if (!editor.selectionIsNone && !editor.selectionIsRange)
        caretRect = editor.selectionRect;

    if (!caretRect.isEmpty()) {
        startPoint.setX(caretRect.x());
        startPoint.setY(caretRect.y() + caretRect.height());

        endPoint.setX(caretRect.x() + caretRect.width());
        endPoint.setY(caretRect.y() + caretRect.height());
    }
    else if (m_view->page()->getSelectionHandlers(leftRect, rightRect)) {
        startPoint.setX(leftRect.x());
        startPoint.setY(leftRect.y() + leftRect.height());

        endPoint.setX(rightRect.x() + rightRect.width());
        endPoint.setY(rightRect.y() + rightRect.height());
    }

    AffineTransform toEvasTransform = m_view->webView()->transformToScene();
    WebCore::IntPoint startEvasPoint = toEvasTransform.mapPoint(startPoint);
    WebCore::IntPoint endEvasPoint = toEvasTransform.mapPoint(endPoint);

    ewkViewTextStyleState(m_view->evasObject(), startEvasPoint, endEvasPoint);
}
#endif
} // namespace WebKit

#endif // TIZEN_TEXT_SELECTION
