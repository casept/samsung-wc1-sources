/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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

#ifndef GamepadsProviderTizen_h
#define GamepadsProviderTizen_h

#if ENABLE(GAMEPAD) && PLATFORM(EFL) && ENABLE(TIZEN_TV_PROFILE)

#include "Gamepads.h"

#include "GamepadDeviceLinux.h"
#include "GamepadEventController.h"
#include <Ecore.h>
#include <Eeze.h>
#include <Eina.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class Gamepad;

static const char joystickPrefix[] = "/dev/input/js";

class GamepadsProviderTizen;

class GamepadDeviceTizen : public GamepadDeviceLinux {
public:
    static PassOwnPtr<GamepadDeviceTizen> create(GamepadsProviderTizen* gamepadProvider, const String& deviceFile)
    {
        return adoptPtr(new GamepadDeviceTizen(gamepadProvider, deviceFile));
    }
    ~GamepadDeviceTizen();
    void resetFdHandler() { m_fdHandler = 0; }
    const String& deviceFile() const { return m_deviceFile; }
    void updateData(const struct js_event&);
    unsigned getIndex() { return m_index; }
    void setIndex(unsigned index);
    PassRefPtr<Gamepad> getGamepadRepresentation() { return m_gamepadRepresentation; }

private:
    GamepadDeviceTizen(GamepadsProviderTizen* , const String& deviceFile);
    static Eina_Bool readCallback(void* userData, Ecore_Fd_Handler*);

    GamepadsProviderTizen* m_gamepadProvider;
    Ecore_Fd_Handler* m_fdHandler;
    String m_deviceFile;
    RefPtr<Gamepad> m_gamepadRepresentation;
    unsigned m_index;
};

class GamepadsProviderTizen : public GamepadsProvider {
public:
    GamepadsProviderTizen(GamepadEventController* controller, size_t length);

    void registerDevice(const String& syspath);
    void unregisterDevice(const String& syspath);

    void sampleGamepads(GamepadList* into) OVERRIDE;

    ~GamepadsProviderTizen();
private:

    static void onGamePadChange(const char* syspath, Eeze_Udev_Event, void* userData, Eeze_Udev_Watch* watcher);

    Vector<OwnPtr<GamepadDeviceTizen> > m_slots;
    HashMap<String, GamepadDeviceTizen*> m_deviceMap;

    Eeze_Udev_Watch* m_gamepadsWatcher;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD) && PLATFORM(EFL) && ENABLE(TIZEN_TV_PROFILE)

#endif // GamepadsProviderTizen_h
