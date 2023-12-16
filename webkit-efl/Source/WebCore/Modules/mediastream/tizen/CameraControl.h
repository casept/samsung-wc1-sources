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

#ifndef CameraControl_h
#define CameraControl_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "Dictionary.h"
#include "MediaStream.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
#include "CameraShootingSound.h"
#endif

namespace WebCore {

class CameraCapabilities;
class CameraControlImpl;
class CameraMediaRecorder;
class CameraImageCapture;
class CameraSettingErrorCallback;
class CameraSuccessCallback;
class ScriptExecutionContext;

class CameraControl : public ActiveDOMObject, public RefCounted<CameraControl> {
public:
    static PassRefPtr<CameraControl> create(ScriptExecutionContext*, MediaStream*);
    CameraControl(ScriptExecutionContext*, MediaStream*);
    virtual ~CameraControl();

    using RefCounted<CameraControl>::ref;
    using RefCounted<CameraControl>::deref;

    CameraMediaRecorder* recorder() { return m_recorder.get(); }
    CameraImageCapture* image() { return m_image.get(); }
    CameraCapabilities* capabilities() { return m_capabilities.get(); }
#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
    void playCameraShootingSound(CameraShootingSound::type);
#endif

    void applySettings(const Dictionary& settings, PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraSettingErrorCallback>);
    bool autoFocus();
    void release();

    MediaStreamDescriptor* descriptor() const { return m_mediaStream->descriptor(); }

private:
    RefPtr<CameraMediaRecorder> m_recorder;
    RefPtr<CameraImageCapture> m_image;
    RefPtr<CameraCapabilities> m_capabilities;
    RefPtr<MediaStream> m_mediaStream;
    RefPtr<CameraControlImpl> m_cameraControlImpl;
#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
    CameraShootingSound m_cameraShootingSound;
#endif
    bool m_released;
    static int s_count;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStream_h
