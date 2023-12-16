/*
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "InputMethodContextEfl.h"

#include "EwkView.h"
#include "WebPageProxy.h"
#include <Ecore_Evas.h>
#include <Ecore_IMF_Evas.h>
#include <WebCore/Editor.h>

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_WEBKIT2_SUPPORT_JAPANESE_IME)
unsigned getUTF8CharacterIndex(const char* string, unsigned byteIndex)
{
    unsigned index = 0;
    const char* end = string + byteIndex;

    while (*string && string < end) {
        unsigned offset;

        if ((*string & 0x80) == 0x00)
            offset = 1;
        else if ((*string & 0xe0) == 0xc0)
            offset = 2;
        else if ((*string & 0xf0) == 0xe0)
            offset = 3;
        else if ((*string & 0xf8) == 0xf0)
            offset = 4;
        else if ((*string & 0xfc) == 0xf8)
            offset = 5;
        else if ((*string & 0xfe) == 0xfc)
            offset = 6;
        else
            offset = 1;

        ++index;
        while (*string && offset--)
            ++string;
    }

    return index;
}
#endif // ENABLE(TIZEN_WEBKIT2_SUPPORT_JAPANESE_IME)

#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
InputMethodContextEfl::InputMethodContextEfl(EwkView* view)
    : m_view(view)
    , m_focused(false)
#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    , m_lastNotifiedCursorPosition(0)
    , m_shouldIgnoreCursorPositionSet(false)
    , m_isSelectionRange(false)
#endif
{
}
#else
InputMethodContextEfl::InputMethodContextEfl(EwkView* view, PassOwnPtr<Ecore_IMF_Context> context)
    : m_view(view)
    , m_context(context)
    , m_focused(false)
{
    ASSERT(m_context);
    ecore_imf_context_event_callback_add(m_context.get(), ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onIMFPreeditSequenceChanged, this);
    ecore_imf_context_event_callback_add(m_context.get(), ECORE_IMF_CALLBACK_COMMIT, onIMFInputSequenceComplete, this);
}
#endif

InputMethodContextEfl::~InputMethodContextEfl()
{
}

#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
Eina_Bool InputMethodContextEfl::onIMFRetrieveSurrounding(void* data, Ecore_IMF_Context*, char** text, int* offset)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!inputMethodContext->m_view->page()->focusedFrame() || !inputMethodContext->m_focused || (!text && !offset))
        return false;

    const EditorState& editor = inputMethodContext->m_view->page()->editorState();

    if (text) {
        CString utf8Text;
        utf8Text = editor.surroundingText.utf8();

        size_t length = utf8Text.length();

        *text = static_cast<char*>(malloc((length + 1) * sizeof(char)));
        if (!(*text))
            return false;

        if (length)
            strncpy(*text, utf8Text.data(), length);
        (*text)[length] = 0;
    }

    if (offset)
        *offset = editor.cursorPosition;

    return true;
}
#endif

void InputMethodContextEfl::onIMFInputSequenceComplete(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!eventInfo || !inputMethodContext->m_focused)
        return;

#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    String completeString = String::fromUTF8(static_cast<char*>(eventInfo));
    inputMethodContext->m_view->page()->confirmComposition(completeString);
    inputMethodContext->updateTextInputState();
#else
    inputMethodContext->m_view->page()->confirmComposition(String::fromUTF8(static_cast<char*>(eventInfo)));
#endif
}

void InputMethodContextEfl::onIMFPreeditSequenceChanged(void* data, Ecore_IMF_Context* context, void*)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    if (!inputMethodContext->m_view->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

#if ENABLE(TIZEN_WEBKIT2_SUPPORT_JAPANESE_IME)
    char* buffer = 0;
    Eina_List* preeditAttrs = 0;
    int cursorPosition = 0;
    ecore_imf_context_preedit_string_with_attributes_get(context, &buffer, &preeditAttrs, &cursorPosition);
    if (!buffer)
        return;

    String preeditString = String::fromUTF8(buffer);
    Vector<CompositionUnderline> underlines;
    if (preeditAttrs) {
        void* item = 0;
        Eina_List* listIterator = 0;
        EINA_LIST_FOREACH(preeditAttrs, listIterator, item) {
            Ecore_IMF_Preedit_Attr* preeditAttr = static_cast<Ecore_IMF_Preedit_Attr*>(item);

            unsigned startIndex = getUTF8CharacterIndex(buffer, preeditAttr->start_index);
            unsigned endIndex = getUTF8CharacterIndex(buffer, preeditAttr->end_index);
            switch (preeditAttr->preedit_type) {
            case ECORE_IMF_PREEDIT_TYPE_SUB1:
                underlines.append(CompositionUnderline(startIndex, endIndex, Color(0, 0, 0), false));
                break;
            case ECORE_IMF_PREEDIT_TYPE_SUB2:
            case ECORE_IMF_PREEDIT_TYPE_SUB3:
                underlines.append(CompositionUnderline(startIndex, endIndex, Color(0, 0, 0), Color(255, 255, 255), false));
                break;
            case ECORE_IMF_PREEDIT_TYPE_SUB4:
                underlines.append(CompositionUnderline(startIndex, endIndex, Color(0, 0, 0), Color(46, 168, 255), false));
                break;
            case ECORE_IMF_PREEDIT_TYPE_SUB5:
                underlines.append(CompositionUnderline(startIndex, endIndex, Color(0, 0, 0), Color(153, 98, 195), false));
                break;
            case ECORE_IMF_PREEDIT_TYPE_SUB6:
                underlines.append(CompositionUnderline(startIndex, endIndex, Color(0, 0, 0), Color(118, 222, 55), false));
                break;
            case ECORE_IMF_PREEDIT_TYPE_SUB7:
                underlines.append(CompositionUnderline(startIndex, endIndex, Color(0, 0, 0), Color(153, 153, 153), false));
                break;
            default:
                break;
            }
        }
        EINA_LIST_FREE(preeditAttrs, item)
            free(item);
    }
    if (underlines.isEmpty())
        underlines.append(CompositionUnderline(0, preeditString.length(), Color(0, 0, 0), false));

    free(buffer);
#else
    char* buffer = 0;
#if ENABLE(TIZEN_WEBKIT2_IMF_CARET_FIX)
    int cursorPosition = 0;
    ecore_imf_context_preedit_string_get(context, &buffer, &cursorPosition);
#else
    ecore_imf_context_preedit_string_get(context, &buffer, 0);
#endif
    if (!buffer)
        return;

    String preeditString = String::fromUTF8(buffer);
    free(buffer);
    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, preeditString.length(), Color(0, 0, 0), false));
#endif // ENABLE(TIZEN_WEBKIT2_SUPPORT_JAPANESE_IME)
#if ENABLE(TIZEN_WEBKIT2_IMF_CARET_FIX)
    inputMethodContext->m_view->page()->setComposition(preeditString, underlines, cursorPosition);
#else
    inputMethodContext->m_view->page()->setComposition(preeditString, underlines, 0);
#endif
}

#if ENABLE(TIZEN_CALL_SIGNAL_FOR_WRT) || ENABLE(TIZEN_WEBKIT2_IMF_REVEAL_SELECTION)
void InputMethodContextEfl::onIMFInputPanelStateChanged(void* data, Ecore_IMF_Context*, int state)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        inputMethodContext->m_focused = false;
        inputMethodContext->resetIMFContext();
        evas_object_smart_callback_call(inputMethodContext->m_view->evasObject(), "editorclient,ime,closed", 0);
    }
    else if (state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        inputMethodContext->m_focused = true;
        evas_object_smart_callback_call(inputMethodContext->m_view->evasObject(), "editorclient,ime,opened", 0);
#if ENABLE(TIZEN_WEBKIT2_IMF_REVEAL_SELECTION)
        inputMethodContext->m_view->page()->centerSelectionInVisibleArea();
#endif
    }
}

void InputMethodContextEfl::onIMFInputPanelGeometryChanged(void* data, Ecore_IMF_Context*, int)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!inputMethodContext->m_context)
        return;

    Eina_Rectangle rect;
    ecore_imf_context_input_panel_geometry_get(inputMethodContext->m_context.get(), &rect.x, &rect.y, &rect.w, &rect.h);
    evas_object_smart_callback_call(inputMethodContext->m_view->evasObject(), "inputmethod,changed", &rect);
}
#endif

PassOwnPtr<Ecore_IMF_Context> InputMethodContextEfl::createIMFContext(Evas* canvas)
{
    const char* defaultContextID = ecore_imf_context_default_id_get();
    if (!defaultContextID)
        return nullptr;

    OwnPtr<Ecore_IMF_Context> imfContext = adoptPtr(ecore_imf_context_add(defaultContextID));
    if (!imfContext)
        return nullptr;

    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(canvas);
    ecore_imf_context_client_window_set(imfContext.get(), reinterpret_cast<void*>(ecore_evas_window_get(ecoreEvas)));
    ecore_imf_context_client_canvas_set(imfContext.get(), canvas);

#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    ecore_imf_context_event_callback_add(imfContext.get(), ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onIMFPreeditSequenceChanged, this);
    ecore_imf_context_event_callback_add(imfContext.get(), ECORE_IMF_CALLBACK_COMMIT, onIMFInputSequenceComplete, this);
#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    ecore_imf_context_retrieve_surrounding_callback_set(imfContext.get(), onIMFRetrieveSurrounding, this);
#endif
#endif
#if ENABLE(TIZEN_CALL_SIGNAL_FOR_WRT)
    ecore_imf_context_input_panel_event_callback_add(imfContext.get(), ECORE_IMF_INPUT_PANEL_STATE_EVENT, onIMFInputPanelStateChanged, this);
    ecore_imf_context_input_panel_event_callback_add(imfContext.get(), ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, onIMFInputPanelGeometryChanged, this);
#endif
#if ENABLE(TIZEN_WEBKIT2_IMF_ADDITIONAL_CALLBACK)
    ecore_imf_context_event_callback_add(imfContext.get(), ECORE_IMF_CALLBACK_DELETE_SURROUNDING, onIMFInputPanelDeleteSurrounding, this);
    ecore_imf_context_event_callback_add(imfContext.get(), ECORE_IMF_CALLBACK_SELECTION_SET, onIMFInputPanelSelectionSet, this);
#endif

    return imfContext.release();
}

#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
bool InputMethodContextEfl::setContext()
{
    if (m_context)
        return true;

    OwnPtr<Ecore_IMF_Context> context;
    context = createIMFContext(evas_object_evas_get(m_view->evasObject()));
    if (!context)
        return false;

    m_context = context.release();
    return true;
}
#endif

void InputMethodContextEfl::handleMouseUpEvent(const Evas_Event_Mouse_Up*)
{
#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    if (!setContext())
        return;
#endif
    ecore_imf_context_reset(m_context.get());
}

void InputMethodContextEfl::handleKeyDownEvent(const Evas_Event_Key_Down* downEvent, bool* isFiltered)
{
#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    if (!setContext())
        return;
#endif

#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    const EditorState& state = m_view->page()->editorState();

    // FIXME: When we press keyboard after selection set callback is called, text contents will be replaced.
    // At this time, cursor position is set to 0 for a while. To avoid calling ecore_imf_context_cursor_position_set
    // as wrong cursor position we use m_shouldIgnoreCursorPositionSet.
    if (m_isSelectionRange)
        m_shouldIgnoreCursorPositionSet = true;
#endif

    Ecore_IMF_Event inputMethodEvent;
    ecore_imf_evas_event_key_down_wrap(const_cast<Evas_Event_Key_Down*>(downEvent), &inputMethodEvent.key_down);

    *isFiltered = ecore_imf_context_filter_event(m_context.get(), ECORE_IMF_EVENT_KEY_DOWN, &inputMethodEvent);
#if ENABLE(TIZEN_WEBKIT2_IMF_CONTEXT_INPUT_PANEL)
    if (!strcmp(downEvent->keyname, "Return")) {
        const EditorState& editor = m_view->page()->editorState();
        if ((editor.autocapitalType != ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE) && !editor.isContentRichlyEditable) {
            m_focused = false;
            ecore_imf_context_reset(m_context.get());
            ecore_imf_context_focus_out(m_context.get());
        }
    }
#endif
}

void InputMethodContextEfl::handleKeyUpEvent(const Evas_Event_Key_Up* upEvent)
{
    if (!m_context)
        return;

    Ecore_IMF_Event inputMethodEvent;
    ecore_imf_evas_event_key_up_wrap(const_cast<Evas_Event_Key_Up*>(upEvent), &inputMethodEvent.key_up);
    ecore_imf_context_filter_event(m_context.get(), ECORE_IMF_EVENT_KEY_UP, &inputMethodEvent);
}

void InputMethodContextEfl::updateTextInputState()
{
#if !ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    if (!m_context)
        return;
#endif

    const EditorState& editor = m_view->page()->editorState();

#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    if (editor.cursorPosition != m_lastNotifiedCursorPosition) {
        if (!m_shouldIgnoreCursorPositionSet) {
            ecore_imf_context_cursor_position_set(m_context.get(), editor.cursorPosition);
            m_lastNotifiedCursorPosition = editor.cursorPosition;
        } else
            m_shouldIgnoreCursorPositionSet = false;
    }
    m_isSelectionRange = editor.selectionIsRange;
#endif

    if (editor.isContentEditable) {
        if (m_focused)
            return;

#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
        if (!setContext())
            return;
#endif

        ecore_imf_context_reset(m_context.get());
#if ENABLE(TIZEN_IMF_SETTING)
        setIMFContextProperties(editor);
#endif
        m_focused = true;
        ecore_imf_context_focus_in(m_context.get());
        ecore_imf_context_input_panel_show(m_context.get());
    } else {
        if (!m_focused)
            return;

        if (editor.hasComposition)
            m_view->page()->cancelComposition();

        m_focused = false;
        ecore_imf_context_reset(m_context.get());
        ecore_imf_context_focus_out(m_context.get());
    }
}

#if ENABLE(TIZEN_WEBKIT2_IMF_CARET_FIX)
void InputMethodContextEfl::resetIMFContext()
{
    if (!m_context)
        return;

    ecore_imf_context_reset(m_context.get());
}
#endif

#if ENABLE(TIZEN_WEBKIT2_IMF_CONTEXT_INPUT_PANEL)
void InputMethodContextEfl::showInputPanel()
{
    if (!m_context)
        return;

    m_focused = true;
    ecore_imf_context_focus_in(m_context.get());
    ecore_imf_context_input_panel_show(m_context.get());
}

void InputMethodContextEfl::hideInputPanel()
{
    if (!m_context)
        return;

    ecore_imf_context_focus_out(m_context.get());
    ecore_imf_context_input_panel_hide(m_context.get());
}
#endif

#if ENABLE(TIZEN_IMF_SETTING)
void InputMethodContextEfl::setIMFContextProperties(const EditorState& editor)
{
    ecore_imf_context_input_panel_layout_set(m_context.get(), static_cast<Ecore_IMF_Input_Panel_Layout>(editor.inputPanelLayout));
    if (editor.inputPanelLayout == ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY)
        ecore_imf_context_input_panel_layout_variation_set(m_context.get(), editor.inputPanelLayoutNumberonlyVariation);

    ecore_imf_context_input_panel_return_key_type_set(m_context.get(), static_cast<Ecore_IMF_Input_Panel_Return_Key_Type>(editor.inputPanelReturnKeyType));
    ecore_imf_context_prediction_allow_set(m_context.get(), editor.allowsPrediction);
    ecore_imf_context_autocapital_type_set(m_context.get(), static_cast<Ecore_IMF_Autocapital_Type>(editor.autocapitalType));
}
#endif // #if ENABLE(TIZEN_ISF_SETTING)

#if ENABLE(TIZEN_WEBKIT2_IMF_ADDITIONAL_CALLBACK)
void InputMethodContextEfl::onIMFInputPanelDeleteSurrounding(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    if (!inputMethodContext->m_view->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

    Ecore_IMF_Event_Delete_Surrounding* ev = static_cast<Ecore_IMF_Event_Delete_Surrounding*>(eventInfo);

    const EditorState& state = inputMethodContext->m_view->page()->editorState();
    int start = ev->offset + state.cursorPosition;
    int extent = ev->n_chars;

    if (start < 0 || (extent < 0 || extent + start) < 0)
        return;

#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    // FIXME: When we deleteSurroundingText, text contents will be replaced.
    // At this time, cursor position is set to 0 for a while. To avoid calling ecore_imf_context_cursor_position_set
    // as wrong cursor position we use m_shouldIgnoreCursorPositionSet.
    inputMethodContext->m_shouldIgnoreCursorPositionSet = true;
#endif
    inputMethodContext->m_view->page()->deleteSurroundingText(start, extent);
}

void InputMethodContextEfl::onIMFInputPanelSelectionSet(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    Ecore_IMF_Event_Selection* ev = static_cast<Ecore_IMF_Event_Selection*>(eventInfo);
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    int start = ev->start;
    int end = ev->end;

    if (!inputMethodContext->m_view->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

    if (start < 0)
        start = 0;

    if (end < 0) {
        if (start + end > 0) {
            int temp = end;
            end = start;
            start += temp;
        } else {
            end = start;
            start = 0;
        }
    }
    inputMethodContext->m_selectedLength = end - start;
    inputMethodContext->m_view->page()->setSelectedText(start, end);
}
#endif

}
