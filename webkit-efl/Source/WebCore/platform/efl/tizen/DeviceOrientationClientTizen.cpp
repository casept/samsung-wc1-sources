/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DeviceOrientationClientTizen.h"

#if ENABLE(TIZEN_DEVICE_ORIENTATION)

#if ENABLE(DEVICE_ORIENTATION)

#include "DeviceOrientationController.h"
#include "DeviceOrientationProviderTizen.h"

namespace WebCore {

void DeviceOrientationClientTizen::deviceOrientationControllerDestroyed()
{
    delete this;
}

void DeviceOrientationClientTizen::startUpdating()
{
    if (!m_controller)
        return;

    DeviceOrientationProviderTizen::provider()->addController(m_controller);
}

void DeviceOrientationClientTizen::stopUpdating()
{
    if (!m_controller)
        return;

    DeviceOrientationProviderTizen::provider()->removeController(m_controller);
}

DeviceOrientationData* DeviceOrientationClientTizen::lastOrientation() const
{
    return DeviceOrientationProviderTizen::provider()->lastOrientation();
}

}
#endif

#endif
