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
#include "CameraError.h"
#include "CameraErrorCallback.h"
#include "CameraMediaRecorder.h"
#include "CameraSettingErrorCallback.h"
#include "CameraSuccessCallback.h"
#include "Event.h"

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
#include "CameraShootingSound.h"
#endif

namespace WebCore {

CameraMediaRecorder::CameraMediaRecorder(ScriptExecutionContext* context, CameraControl* cameraControl)
    : ActiveDOMObject(context)
    , m_cameraControl(cameraControl)
    , m_recorder(MediaRecorder::create(this))
    , m_released(false)
{
}

CameraMediaRecorder::~CameraMediaRecorder()
{
    release();
}

void CameraMediaRecorder::applySettings(const Dictionary& settings, PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraSettingErrorCallback> errorCallback)
{
    if (m_released)
        return;

    String filename, recordingFormat;
    int maxFileSizeBytes = 0;
    RefPtr<CameraSettingErrors> settingErrors = CameraSettingErrors::create();

    CameraCapabilities* capabilities = m_cameraControl->capabilities();
    ASSERT(capabilities);

    if (settings.get("fileName", filename)) {
        if (filename.isEmpty())
            settingErrors->append(CameraSettingErrors::FILENAME_ERR);
        else
            m_recorder->setCustomFileName(filename);
    }

    if (settings.get("maxFileSizeBytes", maxFileSizeBytes)) {
        if (maxFileSizeBytes > 0)
            m_recorder->setMaxFileSizeBytes(maxFileSizeBytes);
        else
            settingErrors->append(CameraSettingErrors::MAX_FILE_SIZE_BYTES_ERR);
    }

    if (settings.get("recordingFormat", recordingFormat)) {
        if (recordingFormat.isEmpty() || !capabilities->isValidRecordingFormat(recordingFormat)
            || (m_cameraControl->descriptor()->numberOfVideoComponents() > 0 && recordingFormat == "amr"))
            settingErrors->append(CameraSettingErrors::RECORDING_FORMAT_ERR);
        else {
            m_recorder->setRecordingFormat(recordingFormat);
            m_recordingFormat = recordingFormat;
        }
    }

    if (settingErrors->codes().size() && errorCallback) {
        errorCallback->handleEvent(settingErrors.get());
        return;
    }

    if (successCallback)
        successCallback->handleEvent();
}

void CameraMediaRecorder::start(PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraErrorCallback> errorCallback)
{
    if (m_released)
        return;

    if (!m_cameraControl) {
        if (errorCallback) {
            RefPtr<CameraError> error = CameraError::create(CameraError::NO_CAMERA);
            errorCallback->handleEvent(error.get());
        }
        return;
    }

    if (!m_recorder->record(m_cameraControl->descriptor())) {
        if (errorCallback) {
            RefPtr<CameraError> error = CameraError::create(CameraError::FILE_WRITE_ERR);
            errorCallback->handleEvent(error.get());
        }
        return;
    }

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
    if (m_cameraControl->descriptor()->numberOfVideoComponents() > 0)
        m_cameraControl->playCameraShootingSound(CameraShootingSound::RecordStart);
#endif
    if (successCallback)
        successCallback->handleEvent();
}

void CameraMediaRecorder::stop(PassRefPtr<CameraSuccessCallback> successCallback, PassRefPtr<CameraErrorCallback> errorCallback)
{
    if (m_released)
        return;

    if (!m_cameraControl) {
        if (errorCallback) {
            RefPtr<CameraError> error = CameraError::create(CameraError::NO_CAMERA);
            errorCallback->handleEvent(error.get());
        }
        return;
    }

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
    if (m_cameraControl->descriptor()->numberOfVideoComponents() > 0)
        m_cameraControl->playCameraShootingSound(CameraShootingSound::RecordStop);
#endif
    // stop recording & save file
    m_recorder->save();

    if (successCallback)
        successCallback->handleEvent();
}

void CameraMediaRecorder::onRecordingStateChange(const String& state)
{
    if (m_released)
        return;

    m_state = state;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Media] state : %s", state.utf8().data());
#endif
    dispatchEvent(Event::create(WebCore::eventNames().recordingstatechangeEvent, false, false));
}

void CameraMediaRecorder::release()
{
    if (m_released)
        return;
    m_released = true;

    m_recorder->cancel();
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

