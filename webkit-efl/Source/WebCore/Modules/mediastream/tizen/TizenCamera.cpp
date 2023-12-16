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
#include "TizenCamera.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CameraControl.h"
#include "CameraError.h"
#include "CameraErrorCallback.h"
#include "CreateCameraSuccessCallback.h"
#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"
#endif

namespace WebCore {

TizenCamera::TizenCamera(ScriptExecutionContext* context)
    : m_context(context)
{
}

TizenCamera::~TizenCamera()
{
}

void TizenCamera::createCameraControl(MediaStream* stream, PassRefPtr<CreateCameraSuccessCallback> successCallback, PassRefPtr<CameraErrorCallback> errorCallback)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    bool allowed = false;
    if (TizenExtensibleAPI::extensibleAPI().cameraControl())
        allowed = true;

    if (stream->isAudioOnly() && TizenExtensibleAPI::extensibleAPI().audioRecordingControl())
        allowed = true;

    if (!allowed) {
        if (errorCallback) {
            RefPtr<CameraError> error = CameraError::create(CameraError::PERMISSION_DENIED);
            errorCallback->handleEvent(error.get());
        }
        return;
    }
#endif

    if (!successCallback)
        return;

    RefPtr<CameraControl> cameraControl = CameraControl::create(m_context, stream);

    if (!cameraControl) {
        if (errorCallback) {
            RefPtr<CameraError> error = CameraError::create(CameraError::CREATION_FAILED);
            errorCallback->handleEvent(error.get());
        }
        return;
    }
    successCallback->handleEvent(cameraControl.get());
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
