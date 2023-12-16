/*
 * Copyright (C) 2012-2014 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DeviceMotionProviderTizen_h
#define DeviceMotionProviderTizen_h

#if ENABLE(TIZEN_DEVICE_ORIENTATION)
#include "DeviceMotionData.h"
#include <sensor.h>
#include <wtf/Vector.h>

namespace WebCore {

class DeviceMotionController;

class DeviceMotionProviderTizen {
public:
    ~DeviceMotionProviderTizen();
    static DeviceMotionProviderTizen* provider();

    void addController(DeviceMotionController*);
    void removeController(DeviceMotionController*);

    DeviceMotionData* lastMotionData() { return m_lastMotionData.get(); }
private:
    DeviceMotionProviderTizen();

    static void onAccelerationChanged(sensor_h sensor, sensor_event_s* event, void* userData);
    static void onGyroscopeChanged(sensor_h sensor, sensor_event_s* event, void* userData);

    static float sAlpha;
    static float sBeta;
    static float sGamma;
    static int sInterval;

    RefPtr<DeviceMotionData> m_lastMotionData;
    Vector<DeviceMotionController*> m_controllers;
    sensor_listener_h m_accelerometerListener;
    sensor_listener_h m_gyroscopeListener;
};

} // namespace WebCore
#endif // ENABLE(TIZEN_DEVICE_ORIENTATION)

#endif // DeviceMotionProviderTizen_h
