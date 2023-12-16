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

#include "config.h"

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#include "ScrollbarFadeAnimationController.h"

#include "CoordinatedGraphicsScene.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

static const double delayTimeToStartFadeOut = 0.5;
static const double fadeInAnimationDuration = 0.1;
static const double fadeOutAnimationDuration = 0.5;
#if ENABLE(TIZEN_WEARABLE_PROFILE)
static const double fadeAnimationFrameRate = 30;
#else
static const double fadeAnimationFrameRate = 60;
#endif
static const double scrollbarShowingDuration = 1.0;

ScrollbarFadeAnimationController::ScrollbarFadeAnimationController(CoordinatedGraphicsScene& owner)
    : m_owner(owner)
    , m_timer(this, &ScrollbarFadeAnimationController::fadeAnimationTimerFired)
    , m_showingTimer(this, &ScrollbarFadeAnimationController::showingTimerFired)
{
}

ScrollbarFadeAnimationController::~ScrollbarFadeAnimationController()
{
    m_fadeAnimations.clear();
    m_timer.stop();
}

void ScrollbarFadeAnimationController::removeAnimationIfNeeded(CoordinatedLayerID id)
{
    m_fadeAnimations.remove(id);
}

void ScrollbarFadeAnimationController::startFadeAnimation(TextureMapperLayer* layer, FadeAnimation::FadeType type, FadeAnimation::FadeTrigger trigger)
{
    if (!m_fadeAnimations.contains(layer->id()))
        m_fadeAnimations.add(layer->id(), adoptPtr(new FadeAnimation(layer, type, trigger)));

    FadeAnimation* animation = m_fadeAnimations.get(layer->id());
    ASSERT(animation);
    animation->startFadeAnimation(type, trigger);

    if (!m_timer.isActive())
        m_timer.startRepeating(1 / fadeAnimationFrameRate);
}

void ScrollbarFadeAnimationController::fadeAnimationTimerFired(Timer<ScrollbarFadeAnimationController>*)
{
    bool bRunning = false;
    double time = currentTime();

    FadeAnimationMap::iterator end = m_fadeAnimations.end();
    for (FadeAnimationMap::iterator it = m_fadeAnimations.begin(); it != end; ++it) {
        it->value->fade(time);

        if (!bRunning && it->value->fadeAnimationType() != FadeAnimation::NoAnimation)
            bRunning = true;
    }

    if (!bRunning)
        m_timer.stop();

    m_owner.updateViewport();
}

void ScrollbarFadeAnimationController::showingTimerFired(Timer<ScrollbarFadeAnimationController>*)
{
    FadeAnimationMap::iterator end = m_fadeAnimations.end();
    for (FadeAnimationMap::iterator it = m_fadeAnimations.begin(); it != end; ++it) {
        if (it->value->fadeAnimationTrigger() == FadeAnimation::WebViewVisibilityChange) {
            it->value->startFadeAnimation(FadeAnimation::FadeOut, FadeAnimation::WebViewVisibilityChange);
        }
    }

    if (!m_timer.isActive())
        m_timer.startRepeating(1 / fadeAnimationFrameRate);
}

void ScrollbarFadeAnimationController::showOverflowScollbar(TextureMapperLayer* layer)
{
    ASSERT(layer);
    startFadeAnimation(layer, FadeAnimation::FadeIn, FadeAnimation::WebViewVisibilityChange);

    m_showingTimer.startOneShot(scrollbarShowingDuration);
}

FadeAnimation::FadeAnimation(TextureMapperLayer* layer, FadeType type, FadeTrigger trigger)
    : m_layer(layer)
    , m_type(type)
    , m_trigger(trigger)
    , m_startTime(0.0)
    , m_opacity(0.f)
    , m_readyToFadeOut(false)
{
}

void FadeAnimation::startFadeAnimation(FadeType type, FadeTrigger trigger)
{
    if (m_type == FadeIn && type == FadeOut)
        m_readyToFadeOut = true;
    else {
        m_type = type;
        m_readyToFadeOut = false;
    }

    m_trigger = trigger;
    m_startTime = currentTime();
}

void FadeAnimation::fade(double currentTime)
{
    switch (m_type) {
    case NoAnimation:
        break;
    case FadeIn: fadeIn(currentTime);
        break;
    case FadeOut: fadeOut(currentTime);
        break;
    }
}

void FadeAnimation::fadeIn(double currentTime)
{
    m_opacity += (currentTime - m_startTime) / fadeInAnimationDuration;
    if (m_opacity >= 1.0) {
        m_opacity = 1.0;
        if (m_readyToFadeOut) {
            m_type = FadeOut;
            m_readyToFadeOut = false;
        }
        else
            m_type = NoAnimation;
    }

    adjustOpacity();
}

void FadeAnimation::fadeOut(double currentTime)
{
    double elapsedTime = currentTime - m_startTime;
    if (elapsedTime < delayTimeToStartFadeOut)
        return;

    m_opacity -= ((elapsedTime - delayTimeToStartFadeOut) / fadeOutAnimationDuration);
    if (m_opacity <= 0.0) {
        m_opacity = 0.0;
        m_type = NoAnimation;
    }

    adjustOpacity();
}

void FadeAnimation::adjustOpacity()
{
    if (TextureMapperLayer* scrollbar = m_layer->horizontalScrollbarLayer())
        scrollbar->setOpacity(m_opacity);

    if (TextureMapperLayer* scrollbar = m_layer->verticalScrollbarLayer())
        scrollbar->setOpacity(m_opacity);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

