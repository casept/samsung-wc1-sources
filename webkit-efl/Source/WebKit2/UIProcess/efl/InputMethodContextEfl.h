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

#ifndef InputMethodContextEfl_h
#define InputMethodContextEfl_h

#include <Ecore_IMF.h>
#include <Evas.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(TIZEN_IMF_SETTING)
#include "EditorState.h"
#endif

class EwkView;

namespace WebKit {

class WebPageProxy;

class InputMethodContextEfl {
public:
#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    static PassOwnPtr<InputMethodContextEfl> create(EwkView* viewImpl)
    {
        return adoptPtr(new InputMethodContextEfl(viewImpl));
    }
#else
    static PassOwnPtr<InputMethodContextEfl> create(EwkView* viewImpl, Evas* canvas)
    {
        OwnPtr<Ecore_IMF_Context> context = createIMFContext(canvas);
        if (!context)
            return nullptr;

        return adoptPtr(new InputMethodContextEfl(viewImpl, context.release()));
    }
#endif
    ~InputMethodContextEfl();

    void handleMouseUpEvent(const Evas_Event_Mouse_Up* upEvent);
    void handleKeyDownEvent(const Evas_Event_Key_Down* downEvent, bool* isFiltered);
    void handleKeyUpEvent(const Evas_Event_Key_Up* upEvent);
    void updateTextInputState();

#if ENABLE(TIZEN_WEBKIT2_IMF_CARET_FIX)
    void resetIMFContext();
#endif
#if ENABLE(TIZEN_WEBKIT2_IMF_CONTEXT_INPUT_PANEL)
    void showInputPanel();
    void hideInputPanel();
#endif

private:
#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    InputMethodContextEfl(EwkView*);
#else
    InputMethodContextEfl(EwkView*, PassOwnPtr<Ecore_IMF_Context>);
#endif
#if ENABLE(TIZEN_IMF_SETTING)
    void setIMFContextProperties(const EditorState&);
#endif

#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    PassOwnPtr<Ecore_IMF_Context> createIMFContext(Evas* canvas);
    bool setContext();
#else
    static PassOwnPtr<Ecore_IMF_Context> createIMFContext(Evas* canvas);
#endif
    static void onIMFInputSequenceComplete(void* data, Ecore_IMF_Context*, void* eventInfo);
    static void onIMFPreeditSequenceChanged(void* data, Ecore_IMF_Context*, void* eventInfo);
#if ENABLE(TIZEN_CALL_SIGNAL_FOR_WRT) || ENABLE(TIZEN_WEBKIT2_IMF_REVEAL_SELECTION)
    static void onIMFInputPanelStateChanged(void* data, Ecore_IMF_Context*, int);
    static void onIMFInputPanelGeometryChanged(void* data, Ecore_IMF_Context*, int);
#endif
#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    static Eina_Bool onIMFRetrieveSurrounding(void*, Ecore_IMF_Context*, char**, int*);
    static void onIMFDeleteSurrounding(void*, Ecore_IMF_Context*, void*);
#endif
#if ENABLE(TIZEN_WEBKIT2_IMF_ADDITIONAL_CALLBACK)
    static void onIMFInputPanelDeleteSurrounding(void*, Ecore_IMF_Context*, void*);
    static void onIMFInputPanelSelectionSet(void*, Ecore_IMF_Context*, void*);
#endif

    EwkView* m_view;
    OwnPtr<Ecore_IMF_Context> m_context;
    bool m_focused;
#if ENABLE(TIZEN_SURROUNDING_TEXT_CALLBACK)
    String m_approximateSurroundingText;
    unsigned m_approximateCursorPosition;
    bool m_isLastKeyEventFiltered;
    int m_lastNotifiedCursorPosition;
    bool m_isSelectionRange;
    bool m_shouldIgnoreCursorPositionSet;
#endif
#if ENABLE(TIZEN_WEBKIT2_IMF_ADDITIONAL_CALLBACK)
    int m_selectedLength;
#endif
};

} // namespace WebKit

#endif // InputMethodContextEfl_h
