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
#include "CameraErrorCallback.h"
#include "CameraImageCapture.h"
#include "CameraSettingErrorCallback.h"
#include "CameraSuccessCallback.h"
#include "Event.h"
#include "LocalMediaServer.h"

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
#include "CameraShootingSound.h"
#endif

namespace WebCore {

CameraImageCapture::CameraImageCapture(ScriptExecutionContext* context, CameraControl* cameraControl)
    : ActiveDOMObject(context)
    , m_cameraControl(cameraControl)
    , m_released(false)
{
    m_camImageCaptureImpl = adoptRef(new CameraImageCaptureImpl(cameraControl));
}

CameraImageCapture::~CameraImageCapture()
{
    release();
}

void CameraImageCapture::applySettings(const Dictionary& settings, PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraSettingErrorCallback> errorCallback)
{
    if (m_released)
        return;

    String fileName, pictureFormat;
    Dictionary pictureSize;
    int pictureWidth = 0, pictureHeight = 0;
    RefPtr<CameraSettingErrors> settingErrors = CameraSettingErrors::create();

    CameraCapabilities* capabilities = m_cameraControl->capabilities();
    ASSERT(capabilities);

    if (settings.get("pictureSize", pictureSize)) {
        if (pictureSize.get("width", pictureWidth) && pictureSize.get("height", pictureHeight)) {
            if (capabilities->isValidPictureSize(pictureWidth, pictureHeight)) {
                TIZEN_LOGI("[Media] set pictureSize : %d X %d", pictureWidth, pictureHeight);
                m_camImageCaptureImpl->setPictureSize(IntSize(pictureWidth, pictureHeight));
            } else
                settingErrors->append(CameraSettingErrors::PICTURE_SIZE_ERR);
        } else
            settingErrors->append(CameraSettingErrors::PICTURE_SIZE_ERR);
    }

    if (settings.get("pictureFormat", pictureFormat)) {
        TIZEN_LOGI("[Media] set pictureFormat : %s", pictureFormat.utf8().data());
        if (!capabilities->isValidPictureFormat(pictureFormat)
                || !m_camImageCaptureImpl->setPictureFormat(pictureFormat))
            settingErrors->append(CameraSettingErrors::PICTURE_FORMAT_ERR);
    }

    if (settings.get("fileName", fileName)) {
        TIZEN_LOGI("[Media] set fileName : %s", fileName.utf8().data());
        if (fileName.isEmpty())
            settingErrors->append(CameraSettingErrors::FILENAME_ERR);
        else
            m_camImageCaptureImpl->setCustomFileName(fileName);
    }

    if (settingErrors->codes().size() && errorCallback) {
        errorCallback->handleEvent(settingErrors.get());
        return;
    }

    if (successCallback)
        successCallback->handleEvent();
}

void CameraImageCapture::takePicture(PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraErrorCallback> errorCallback)
{
    if (m_released)
        return;

    m_camImageCaptureImpl->takePicture(successCallback, errorCallback);
}

void CameraImageCapture::onShutter()
{
    if (m_released)
        return;

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
    m_cameraControl->playCameraShootingSound(CameraShootingSound::ImageCapture);
#endif
    dispatchEvent(Event::create(WebCore::eventNames().shutterEvent, false, false));
}

void CameraImageCapture::release()
{
    if (m_released)
        return;
    m_released = true;

    m_camImageCaptureImpl->release();
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

