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

#ifndef GamepadEventController_h
#define GamepadEventController_h

#if ENABLE(GAMEPAD) && ENABLE(TIZEN_TV_PROFILE)

#include "Supplementable.h"

#include <wtf/HashCountedSet.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class DOMWindow;
class Event;
class Gamepad;
class GamepadList;
class GamepadsProvider;
class Page;

class GamepadEventController : public Supplement<Page> {
    WTF_MAKE_NONCOPYABLE(GamepadEventController);
public:

    void addGamepadEventListener(DOMWindow*);
    void removeGamepadEventListener(DOMWindow*);
    void removeAllGamepadEventListeners(DOMWindow*);

    bool isActive() { return !m_listeners.isEmpty(); }

    static PassOwnPtr<GamepadEventController> create();

    void gamepadConnected(PassRefPtr<Gamepad>);
    void gamepadDisconnected(PassRefPtr<Gamepad>);

    void sampleGamepads(GamepadList*);

    void resetState();

    static const char* supplementName();
    static GamepadEventController* from(Page*);
    static bool isActiveAt(Page*);

    ~GamepadEventController();
private:
    GamepadEventController();

    void dispatchGamepadEvent(PassRefPtr<Event>);

    OwnPtr<GamepadsProvider> m_gamepadsProvider;
    HashCountedSet<RefPtr<DOMWindow>> m_listeners;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD) && ENABLE(TIZEN_TV_PROFILE)

#endif // GamepadEventController_h
