/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef WebKitGamepadEvent_h
#define WebKitGamepadEvent_h

#if ENABLE(GAMEPAD)
#if PLATFORM(EFL)

#include "Event.h"
#include "Gamepad.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class WebKitGamepadEvent : public Event {
public:
    static PassRefPtr<WebKitGamepadEvent> create();
    virtual ~WebKitGamepadEvent();

    static PassRefPtr<WebKitGamepadEvent> createConnectedGamepad(PassRefPtr<Gamepad> gamepad);
    static PassRefPtr<WebKitGamepadEvent> createDisconnectedGamepad(PassRefPtr<Gamepad> gamepad);

    Gamepad* gamepad() const { return m_gamepad.get(); }

    // Event
    virtual const AtomicString& interfaceName() const OVERRIDE;

private:
    WebKitGamepadEvent();
    WebKitGamepadEvent(const AtomicString& eventName, PassRefPtr<Gamepad> gamepad);

    RefPtr<Gamepad> m_gamepad;
};

} // namespace WebCore

#endif // PLATFORM(EFL)
#endif // ENABLE(GAMEPAD)

#endif // WebKitGamepadEvent_h
