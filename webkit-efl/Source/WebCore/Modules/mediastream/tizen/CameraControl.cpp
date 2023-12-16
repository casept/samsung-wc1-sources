/*
    Copyright (C) 2013 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CameraCapabilities.h"
#include "CameraControl.h"
#include "CameraControlImpl.h"
#include "CameraImageCapture.h"
#include "CameraMediaRecorder.h"
#include "CameraSettingErrors.h"
#include "CameraSettingErrorCallback.h"
#include "CameraSuccessCallback.h"

#include <wtf/Vector.h>

namespace WebCore {

int CameraControl::s_count = 0;

PassRefPtr<CameraControl> CameraControl::create(ScriptExecutionContext* context, MediaStream* mediaStream)
{
    if (CameraControl::s_count > 0)
        return 0;

    CameraControl::s_count++;
    return adoptRef(new CameraControl(context, mediaStream));
}

CameraControl::CameraControl(ScriptExecutionContext* context, MediaStream* mediaStream)
    : ActiveDOMObject(context)
    , m_recorder(CameraMediaRecorder::create(context, this))
    , m_capabilities(CameraCapabilities::create(mediaStream->isAudioOnly()))
    , m_mediaStream(mediaStream)
    , m_cameraControlImpl(CameraControlImpl::create(this))
    , m_released(false)
{
    if (!mediaStream->isAudioOnly())
        m_image = CameraImageCapture::create(context, this);
}

CameraControl::~CameraControl()
{
    release();
}

void CameraControl::applySettings(const Dictionary& settings, PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraSettingErrorCallback> errorCallback)
{
    Dictionary focusArea;
    RefPtr<CameraSettingErrors> settingErrors = CameraSettingErrors::create();
    int top = 0, left = 0, bottom = 0, right = 0;

    if (settings.get("focusArea", focusArea)) {
        if (focusArea.get("top", top)
                && focusArea.get("left", left)
                && focusArea.get("bottom", bottom)
                && focusArea.get("right", right)) {
            m_cameraControlImpl->setFocusArea(top, left, bottom, right);
            if (successCallback)
                successCallback->handleEvent();
            return;
        } else
            settingErrors->append(CameraSettingErrors::FOCUS_AREA_ERR);
    }


    if (settingErrors->codes().size() && errorCallback)
        errorCallback->handleEvent(settingErrors.get());
}

bool CameraControl::autoFocus()
{
    return m_cameraControlImpl->autoFocus();
}

void CameraControl::release()
{
    if (m_released)
        return;
    m_released = true;

    m_recorder->release();
    if (m_image.get())
        m_image->release();

    CameraControl::s_count--;
}

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
void CameraControl::playCameraShootingSound(CameraShootingSound::type type)
{
    m_cameraShootingSound.playCameraShootingSound(type);
}
#endif

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
