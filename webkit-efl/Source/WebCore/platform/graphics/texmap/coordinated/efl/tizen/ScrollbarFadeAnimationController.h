/*
*   Copyright (C) 2014 Samsung Electronics
*
*   This library is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Library General Public
*   License as published by the Free Software Foundation; either
*   version 2 of the License, or (at your option) any later version.
*
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Library General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public License
*   along with this library; see the file COPYING.LIB.  If not, write to
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
*   Boston, MA 02110-1301, USA.
*/

#ifndef ScrollbarFadeAnimationController_h
#define ScrollbarFadeAnimationController_h

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
#include "CoordinatedGraphicsState.h"
#include "TextureMapperLayer.h"
#include "Timer.h"

namespace WebCore {

class CoordinatedGraphicsScene;

class FadeAnimation {
public:
    enum FadeType {
        NoAnimation,
        FadeIn,
        FadeOut,
    };

    enum FadeTrigger {
        OverflowTouch,
        WebViewVisibilityChange
    };

    FadeAnimation(TextureMapperLayer*, FadeType, FadeTrigger);
    void startFadeAnimation(FadeType, FadeTrigger);

    void fade(double currentTime);
    FadeType fadeAnimationType() const { return m_type; }
    FadeTrigger fadeAnimationTrigger() const { return m_trigger; }

private:
    void fadeIn(double);
    void fadeOut(double);

    void adjustOpacity();

    TextureMapperLayer* m_layer;
    FadeType m_type;
    FadeTrigger m_trigger;
    double m_startTime;
    float m_opacity;
    bool m_readyToFadeOut;
};

class ScrollbarFadeAnimationController {
public:
    explicit ScrollbarFadeAnimationController(CoordinatedGraphicsScene&);
    ~ScrollbarFadeAnimationController();

    void overflowTouchStarted(TextureMapperLayer* layer) { startFadeAnimation(layer, FadeAnimation::FadeIn, FadeAnimation::OverflowTouch); }
    void overflowScrollFinished(TextureMapperLayer* layer) { startFadeAnimation(layer, FadeAnimation::FadeOut, FadeAnimation::OverflowTouch); }

    void removeAnimationIfNeeded(CoordinatedLayerID);

    void showOverflowScollbar(TextureMapperLayer* layer);

private:
    void startFadeAnimation(TextureMapperLayer*, FadeAnimation::FadeType, FadeAnimation::FadeTrigger);
    void fadeAnimationTimerFired(Timer<ScrollbarFadeAnimationController>*);
    void showingTimerFired(Timer<ScrollbarFadeAnimationController>*);

    CoordinatedGraphicsScene& m_owner;

    typedef HashMap<CoordinatedLayerID, OwnPtr<FadeAnimation> > FadeAnimationMap;
    FadeAnimationMap m_fadeAnimations;

    Timer<ScrollbarFadeAnimationController> m_timer;
    Timer<ScrollbarFadeAnimationController> m_showingTimer;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#endif // ScrollbarFadeAnimationController_h
