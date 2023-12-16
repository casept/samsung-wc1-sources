/*
 * Copyright (C) 2012 Samsung Electronics
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

#ifndef FocusRing_h
#define FocusRing_h

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

#include <Ecore.h>
#include <Evas.h>
#include <WebCore/Color.h>
#include <WebCore/IntPoint.h>
#include <WebCore/IntRect.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

class EwkView;

class FocusRing {
public:
    static PassOwnPtr<FocusRing> create(EwkView* view)
    {
        return adoptPtr(new FocusRing(view));
    }
    ~FocusRing();

    void setImage(const String&, int, int);

    void requestToShow(const WebCore::IntPoint&, bool immediately = false);
    void requestToHide(bool immediately = false);

    void show(const Vector<WebCore::IntRect>&);
    void moveBy(const WebCore::IntSize&);

    void hide(bool = true);

    bool canUpdate() { return (m_state < FocusRingHideState); }
    bool rectsChanged(const Vector<WebCore::IntRect>&);
    WebCore::IntPoint centerPointInScreen();

private:
    static const int s_showTimerTime = 100;
    static Eina_Bool showTimerCallback(void* data);

    static const int s_hideTimerTime = 20;
    static Eina_Bool hideTimerCallback(void* data);

private:
    FocusRing(EwkView* view);

    void nodeRectAtPosition(Vector<WebCore::IntRect>&);
    bool ensureFocusRingObject();
    void adjustFocusRingObject(const WebCore::IntRect&, const WebCore::IntPoint&, const WebCore::IntRect&);

    EwkView* m_ewkView;

    Evas_Object* m_focusRingObject;

    String m_imagePath;
    int m_imageOuterWidth;
    int m_imageInnerWidth;

    Ecore_Timer* m_showTimer;
    Ecore_Timer* m_hideTimer;
    WebCore::IntPoint m_position;

    Vector<WebCore::IntRect> m_rects;

    bool m_shouldHideAfterShow;

    WebCore::Color m_focusRingColor;

    enum FocusRingState {
        FocusRingShowState,
        FocusRingWaitRectState,
        FocusRingWaitRectWithContextMenuState,
        FocusRingHideState,
        FocusRingWaitHideKeypadState
    };
    FocusRingState m_state;
};

#endif // #if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

#endif // FocusRing_h
