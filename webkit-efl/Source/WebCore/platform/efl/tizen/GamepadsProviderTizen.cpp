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

#include "config.h"

#if ENABLE(GAMEPAD) && PLATFORM(EFL) && ENABLE(TIZEN_TV_PROFILE)

#include "GamepadsProviderTizen.h"

#include "GamepadList.h"
#include "Logging.h"
#include <unistd.h>

namespace WebCore {

GamepadDeviceTizen::~GamepadDeviceTizen()
{
    if (m_fdHandler)
        ecore_main_fd_handler_del(m_fdHandler);
}

void GamepadDeviceTizen::updateData(const struct js_event& event)
{
    GamepadDeviceLinux::updateForEvent(event);
    if (connected()) {
        m_gamepadRepresentation->id(id());
        m_gamepadRepresentation->index(m_index);
        m_gamepadRepresentation->timestamp(timestamp());
        m_gamepadRepresentation->axes(axesCount(), axesData());
        m_gamepadRepresentation->buttons(buttonsCount(), buttonsData());

    }
}

void GamepadDeviceTizen::setIndex(unsigned index)
{
    m_index = index;
    m_gamepadRepresentation->index(m_index);
}

GamepadDeviceTizen::GamepadDeviceTizen(GamepadsProviderTizen* gamepadProvider, const String& deviceFile)
    : GamepadDeviceLinux(deviceFile)
    , m_gamepadProvider(gamepadProvider)
    , m_fdHandler(0)
    , m_deviceFile(deviceFile)
    , m_gamepadRepresentation(Gamepad::create())
    , m_index(0)
{
    if (m_fileDescriptor < 0)
        return;

    m_fdHandler = ecore_main_fd_handler_add(m_fileDescriptor, ECORE_FD_READ, readCallback, this, 0, 0);
    if (!m_fdHandler)
        LOG_ERROR("Failed to create the Ecore_Fd_Handler.");
}

Eina_Bool GamepadDeviceTizen::readCallback(void *userData, Ecore_Fd_Handler *fdHandler)
{
    GamepadDeviceTizen* gamepadDevice = static_cast<GamepadDeviceTizen*>(userData);

    if (ecore_main_fd_handler_active_get(fdHandler, ECORE_FD_ERROR)) {
        LOG_ERROR("An error occurred while watching the joystick file descriptor at %s, aborting.", gamepadDevice->deviceFile().utf8().data());
        gamepadDevice->resetFdHandler();
        return ECORE_CALLBACK_CANCEL;
    }

    int fdDevice = ecore_main_fd_handler_fd_get(fdHandler);
    struct js_event event;
    const ssize_t len = read(fdDevice, &event, sizeof(event));

    if (len <= 0) {
        LOG_ERROR("Failed to read joystick file descriptor at %s, aborting.", gamepadDevice->deviceFile().utf8().data());
        gamepadDevice->resetFdHandler();
        return ECORE_CALLBACK_CANCEL;
    }
    if (len != sizeof(event)) {
        LOG_ERROR("Wrong js_event size read on file descriptor at %s, ignoring.", gamepadDevice->deviceFile().utf8().data());
        return ECORE_CALLBACK_RENEW;
    }

    const auto wasConnected = gamepadDevice->connected();
    gamepadDevice->updateData(event);
    if (!wasConnected && gamepadDevice->connected())
        gamepadDevice->m_gamepadProvider->getGamepadController()->gamepadConnected(gamepadDevice->getGamepadRepresentation());

    return ECORE_CALLBACK_RENEW;
}

void GamepadsProviderTizen::onGamePadChange(const char* syspath, Eeze_Udev_Event event, void* userData, Eeze_Udev_Watch*)
{
    GamepadsProviderTizen* gamepadsTizen = static_cast<GamepadsProviderTizen*>(userData);

    switch (event) {
    case EEZE_UDEV_EVENT_ADD:
        gamepadsTizen->registerDevice(String::fromUTF8(syspath));
        break;
    case EEZE_UDEV_EVENT_REMOVE:
        gamepadsTizen->unregisterDevice(String::fromUTF8(syspath));
        break;
    default:
        break;
    }
}

GamepadsProviderTizen::GamepadsProviderTizen(GamepadEventController* controller, size_t length)
    : GamepadsProvider(controller)
    , m_slots(length)
    , m_gamepadsWatcher(0)
{
    if (eeze_init() < 0) {
        LOG_ERROR("Failed to initialize eeze library.");
        return;
    }

    // Watch for gamepads additions / removals.
    m_gamepadsWatcher = eeze_udev_watch_add(EEZE_UDEV_TYPE_JOYSTICK, (EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE), onGamePadChange, this);

    // List available gamepads.
    Eina_List* gamepads = eeze_udev_find_by_type(EEZE_UDEV_TYPE_JOYSTICK, 0);
    void* data;
    EINA_LIST_FREE(gamepads, data) {
        char* syspath = static_cast<char*>(data);
        registerDevice(String::fromUTF8(syspath));
        eina_stringshare_del(syspath);
    }
}

GamepadsProviderTizen::~GamepadsProviderTizen()
{
    if (m_gamepadsWatcher)
        eeze_udev_watch_del(m_gamepadsWatcher);
    eeze_shutdown();
}

void GamepadsProviderTizen::registerDevice(const String& syspath)
{
    if (m_deviceMap.contains(syspath))
        return;

    // Make sure it is a valid joystick.
    const char* deviceFile = eeze_udev_syspath_get_devpath(syspath.utf8().data());
    if (!deviceFile || !eina_str_has_prefix(deviceFile, joystickPrefix))
        return;

    LOG(Gamepad, "Registering gamepad at %s", deviceFile);

    const size_t slotCount = m_slots.size();
    for (size_t index = 0; index < slotCount; ++index) {
        if (!m_slots[index]) {
            m_slots[index] = GamepadDeviceTizen::create(this, String::fromUTF8(deviceFile));
            m_slots[index]->setIndex(index);
            LOG(Gamepad, "Gamepad device name is %s", m_slots[index]->id().utf8().data());
            m_deviceMap.add(syspath, m_slots[index].get());
            break;
        }
    }
}

void GamepadsProviderTizen::unregisterDevice(const String& syspath)
{
    if (!m_deviceMap.contains(syspath))
        return;

    GamepadDeviceTizen* gamepadDevice = m_deviceMap.take(syspath);
    LOG(Gamepad, "Unregistering gamepad at %s", gamepadDevice->deviceFile().utf8().data());
    const size_t index = m_slots.find(gamepadDevice);
    ASSERT(index != notFound);

    auto releasedPad = m_slots[index].release();

    if (releasedPad->connected())
        m_eventController->gamepadDisconnected(releasedPad->getGamepadRepresentation());
}

void GamepadsProviderTizen::sampleGamepads(GamepadList* into)
{
    ASSERT(m_slots.size() == into->length());

    const size_t slotCount = m_slots.size();
    for (size_t i = 0; i < slotCount; ++i) {
        if (m_slots[i].get() && m_slots[i]->connected())
            into->set(i, m_slots[i]->getGamepadRepresentation());
        else
            into->set(i, 0);
    }
}

PassOwnPtr<GamepadsProvider> createGamepadProvider(GamepadEventController *controller)
{
    return adoptPtr(new GamepadsProviderTizen(controller, GamepadList::maximumGamepads));
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD) && PLATFORM(EFL) && ENABLE(TIZEN_TV_PROFILE)
