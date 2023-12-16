/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(GAMEPAD)

#include "NavigatorGamepad.h"

#include "Frame.h"
#include "GamepadList.h"
#include "GamepadEventController.h"
#include "Gamepads.h"
#include "Navigator.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

#if ENABLE(TIZEN_TV_PROFILE)
NavigatorGamepad::NavigatorGamepad(Navigator* navigator)
    : m_navigator(navigator)
{
}
#endif

NavigatorGamepad::~NavigatorGamepad()
{
}

const char* NavigatorGamepad::supplementName()
{
    return "NavigatorGamepad";
}

NavigatorGamepad* NavigatorGamepad::from(Navigator* navigator)
{
    NavigatorGamepad* supplement = static_cast<NavigatorGamepad*>(Supplement<Navigator>::from(navigator, supplementName()));
    if (!supplement) {
#if ENABLE(TIZEN_TV_PROFILE)
        supplement = new NavigatorGamepad(navigator);
#else
        supplement = new NavigatorGamepad();
#endif
        provideTo(navigator, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

GamepadList* NavigatorGamepad::webkitGetGamepads(Navigator* navigator)
{
    return NavigatorGamepad::from(navigator)->gamepads();
}

GamepadList* NavigatorGamepad::gamepads()
{
    if (!m_gamepads)
        m_gamepads = GamepadList::create();
#if ENABLE(TIZEN_TV_PROFILE)
    GamepadEventController::from(m_navigator->frame()->page())->sampleGamepads(m_gamepads.get());
#else
    sampleGamepads(m_gamepads.get());
#endif

    return m_gamepads.get();
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
