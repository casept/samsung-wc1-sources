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
#include "DeviceOrientationProviderTizen.h"

#if ENABLE(TIZEN_DEVICE_ORIENTATION)
#include "DeviceOrientationController.h"
#include "DeviceOrientationData.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

DeviceOrientationProviderTizen* DeviceOrientationProviderTizen::provider()
{
    DEFINE_STATIC_LOCAL(DeviceOrientationProviderTizen, orientationProvider, ());
    return &orientationProvider;
}

DeviceOrientationProviderTizen::DeviceOrientationProviderTizen()
    : m_listener(0)
{
    sensor_type_e type = SENSOR_ORIENTATION;
    sensor_h sensor;
    int error_code;
    bool supported;

    error_code = sensor_is_supported(type, &supported);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    if (!supported) {
        TIZEN_LOGE("Accelerometer is not supported");
        return;
    }

    error_code = sensor_get_default_sensor(type, &sensor);
    if (error_code != SENSOR_ERROR_NONE) {
        TIZEN_LOGE("Sensor error occured (%d)", error_code);
        return;
    }
    sensor_create_listener(sensor, &m_listener);
    sensor_listener_set_accuracy_cb(m_listener, onCompassNeedsCalibrationFired, this);
    sensor_listener_set_event_cb(m_listener, 50, onOrientationChanged, this);
}

DeviceOrientationProviderTizen::~DeviceOrientationProviderTizen()
{
    if (m_controllers.size()) {
        sensor_listener_unset_event_cb(m_listener);
        sensor_listener_stop(m_listener);
        sensor_destroy_listener(m_listener);
    }
}

void DeviceOrientationProviderTizen::addController(DeviceOrientationController* controller)
{
    if (!m_listener)
        return;

    m_controllers.append(controller);
    if (m_controllers.size() == 1) {
        sensor_listener_start(m_listener);
        sensor_listener_set_option(m_listener, SENSOR_OPTION_ALWAYS_ON);
    }
}

void DeviceOrientationProviderTizen::removeController(DeviceOrientationController* controller)
{
    if (!m_controllers.size())
        return;

    m_controllers.remove(m_controllers.find(controller));
    if (!m_controllers.size())
        sensor_listener_stop(m_listener);
}

void DeviceOrientationProviderTizen::onOrientationChanged(sensor_h /* sensor */, sensor_event_s* event, void* userData)
{
    DeviceOrientationProviderTizen* self = static_cast<DeviceOrientationProviderTizen*>(userData);

    self->m_lastOrientation = DeviceOrientationData::create(true, event->values[0], true, event->values[1], true, event->values[2], true, false);

    size_t size = self->m_controllers.size();
    for (size_t i = 0; i < size; ++i)
        self->m_controllers[i]->didChangeDeviceOrientation(self->m_lastOrientation.get());
}

void DeviceOrientationProviderTizen::onCompassNeedsCalibrationFired(sensor_h /* sensor */, unsigned long long /* timeStamp */, sensor_data_accuracy_e accuracy, void* userData)
{
    DeviceOrientationProviderTizen* self = static_cast<DeviceOrientationProviderTizen*>(userData);
    if (accuracy <= SENSOR_DATA_ACCURACY_BAD) {
        size_t size = self->m_controllers.size();
        for (size_t i = 0; i < size; ++i)
            self->m_controllers[i]->notifyCompassNeedsCalibration();
    }
}

}
#endif // ENABLE(TIZEN_DEVICE_ORIENTATION)
