/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(GAMEPAD) && ENABLE(TIZEN_TV_PROFILE)

#include "Document.h"
#include "DOMWindow.h"
#include "Event.h"
#include "Gamepad.h"
#include "GamepadEventController.h"
#include "Gamepads.h"
#include "Page.h"
#include "WebKitGamepadEvent.h"

namespace WebCore {

GamepadEventController::GamepadEventController()
{
}

GamepadEventController::~GamepadEventController()
{
}

void GamepadEventController::addGamepadEventListener(DOMWindow* window)
{
    if (!m_gamepadsProvider)
        m_gamepadsProvider = createGamepadProvider(this);
    m_listeners.add(window);
}

void GamepadEventController::removeGamepadEventListener(DOMWindow* window)
{
    m_listeners.remove(window);
}

void GamepadEventController::removeAllGamepadEventListeners(DOMWindow* window)
{
    m_listeners.removeAll(window);
}

PassOwnPtr<GamepadEventController> GamepadEventController::create()
{
    return adoptPtr(new GamepadEventController());
}

const char* GamepadEventController::supplementName()
{
    return "GamepadEventController";
}

void GamepadEventController::gamepadConnected(PassRefPtr<Gamepad> gamepad)
{
    dispatchGamepadEvent(WebKitGamepadEvent::createConnectedGamepad(gamepad));
}

void GamepadEventController::gamepadDisconnected(PassRefPtr<Gamepad> gamepad)
{
    dispatchGamepadEvent(WebKitGamepadEvent::createDisconnectedGamepad(gamepad));
}

void GamepadEventController::sampleGamepads(GamepadList* into)
{
    if (!m_gamepadsProvider)
        m_gamepadsProvider = createGamepadProvider(this);
    m_gamepadsProvider->sampleGamepads(into);
}

void GamepadEventController::resetState()
{
    // just destroy gamepads provider, it will be created on any request
    if (!!m_gamepadsProvider)
        m_gamepadsProvider = nullptr;
}

GamepadEventController* GamepadEventController::from(Page* page)
{
    if (!page)
        return 0;
    GamepadEventController* supplement = static_cast<GamepadEventController*>(Supplement<Page>::from(page, supplementName()));
    if (!supplement) {
        supplement = new GamepadEventController();
        provideTo(page, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

bool GamepadEventController::isActiveAt(Page* page)
{
    if (GamepadEventController* self = GamepadEventController::from(page))
        return self->isActive();
    return false;
}

void GamepadEventController::dispatchGamepadEvent(PassRefPtr<Event> prpEvent)
{
    RefPtr<Event> event = prpEvent;
    Vector<RefPtr<DOMWindow>> listenerVector;
    copyToVector(m_listeners, listenerVector);
    for (size_t i = 0; i < listenerVector.size(); ++i) {
        if (listenerVector[i]->document()
            && !listenerVector[i]->document()->activeDOMObjectsAreSuspended()
            && !listenerVector[i]->document()->activeDOMObjectsAreStopped())
        listenerVector[i]->dispatchEvent(event);
    }
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD) && ENABLE(TIZEN_TV_PROFILE)
