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

#include "config.h"
#include "DeviceMotionProviderTizen.h"

#if ENABLE(TIZEN_DEVICE_ORIENTATION)
#include "DeviceMotionController.h"
#include "DeviceMotionData.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

int DeviceMotionProviderTizen::sInterval = 50;
float DeviceMotionProviderTizen::sAlpha = 0;
float DeviceMotionProviderTizen::sBeta = 0;
float DeviceMotionProviderTizen::sGamma = 0;

DeviceMotionProviderTizen* DeviceMotionProviderTizen::provider()
{
    DEFINE_STATIC_LOCAL(DeviceMotionProviderTizen, orientationProvider, ());
    return &orientationProvider;
}

DeviceMotionProviderTizen::DeviceMotionProviderTizen()
    : m_accelerometerListener(0)
    , m_gyroscopeListener(0)
{
    sensor_type_e type_accelerometer = SENSOR_ACCELEROMETER;
    sensor_type_e type_gyroscope = SENSOR_GYROSCOPE;
    sensor_h accelerometer_sensor;
    sensor_h gyroscope_sensor;
    int error_code;
    bool supported;

    // accelerometer sensor
    error_code = sensor_is_supported(type_accelerometer, &supported);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    if (!supported) {
        TIZEN_LOGE("Accelerometer is not supported");
        return;
    }

    error_code = sensor_get_default_sensor(type_accelerometer, &accelerometer_sensor);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    sensor_create_listener(accelerometer_sensor, &m_accelerometerListener);
    sensor_listener_set_event_cb(m_accelerometerListener, 50, onAccelerationChanged, this);

    // gyroscope sensor
    error_code = sensor_is_supported(type_gyroscope, &supported);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    if (!supported) {
        TIZEN_LOGE("Gyroscope is not supported");
        return;
    }
    error_code = sensor_get_default_sensor(type_gyroscope, &gyroscope_sensor);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    sensor_create_listener(gyroscope_sensor, &m_gyroscopeListener);
    sensor_listener_set_event_cb(m_gyroscopeListener, 50, onGyroscopeChanged, this);
}

DeviceMotionProviderTizen::~DeviceMotionProviderTizen()
{
    if (m_controllers.size()) {
        sensor_listener_unset_event_cb(m_accelerometerListener);
        sensor_listener_unset_event_cb(m_gyroscopeListener);
        sensor_listener_stop(m_accelerometerListener);
        sensor_listener_stop(m_gyroscopeListener);
        sensor_destroy_listener(m_accelerometerListener);
        sensor_destroy_listener(m_gyroscopeListener);
    }
}

void DeviceMotionProviderTizen::addController(DeviceMotionController* controller)
{
    if (!m_accelerometerListener)
        return;

    m_controllers.append(controller);

    if (m_controllers.size() == 1) {
        sensor_listener_start(m_accelerometerListener);
        sensor_listener_start(m_gyroscopeListener);
        sensor_listener_set_option(m_accelerometerListener, SENSOR_OPTION_ALWAYS_ON);
        sensor_listener_set_option(m_gyroscopeListener, SENSOR_OPTION_ALWAYS_ON);
    }
}

void DeviceMotionProviderTizen::removeController(DeviceMotionController* controller)
{
    if (!m_controllers.size())
        return;

    m_controllers.remove(m_controllers.find(controller));

    if (!m_controllers.size()) {
        sensor_listener_stop(m_accelerometerListener);
        sensor_listener_stop(m_gyroscopeListener);
    }
}

void DeviceMotionProviderTizen::onGyroscopeChanged(sensor_h /* sensor */, sensor_event_s* event, void* /* userData */)
{
    sAlpha = event->values[0];
    sBeta = event->values[1];
    sGamma = event->values[2];
}

void DeviceMotionProviderTizen::onAccelerationChanged(sensor_h /* sensor */, sensor_event_s* event, void* userData)
{
    DeviceMotionProviderTizen* self = static_cast<DeviceMotionProviderTizen*>(userData);

    float gravityX = event->values[0] * 0.2;
    float gravityY = event->values[1] * 0.2;
    float gravityZ = event->values[2] * 0.2;
    bool accelerationAvailable = false;

    DeviceMotionData* motion = self->lastMotionData();
    if (motion) {
        gravityX += (motion->accelerationIncludingGravity()->x() - motion->acceleration()->x()) * 0.8;
        gravityY += (motion->accelerationIncludingGravity()->y() - motion->acceleration()->y()) * 0.8;
        gravityZ += (motion->accelerationIncludingGravity()->z() - motion->acceleration()->z()) * 0.8;
        accelerationAvailable = true;
    }

    RefPtr<DeviceMotionData::Acceleration> accelerationIncludingGravity = DeviceMotionData::Acceleration::create(true, event->values[0], true, event->values[1], true, event->values[2]);

    RefPtr<DeviceMotionData::Acceleration> acceleration = DeviceMotionData::Acceleration::create(
            accelerationAvailable, event->values[0] - gravityX,
            accelerationAvailable, event->values[1] - gravityY,
            accelerationAvailable, event->values[2] - gravityZ);

    bool rotationRateAvailable = true;

    RefPtr<DeviceMotionData::RotationRate> rotationRate = DeviceMotionData::RotationRate::create(
            rotationRateAvailable, sAlpha,
            rotationRateAvailable, sBeta,
            rotationRateAvailable, sGamma);

    self->m_lastMotionData= DeviceMotionData::create(
            acceleration,
            accelerationIncludingGravity,
            rotationRate,
            true,
            sInterval);
    size_t size = self->m_controllers.size();
    for (size_t i = 0; i < size; ++i)
        self->m_controllers[i]->didChangeDeviceMotion(self->m_lastMotionData.get());
}

}
#endif // ENABLE(TIZEN_DEVICE_ORIENTATION)
