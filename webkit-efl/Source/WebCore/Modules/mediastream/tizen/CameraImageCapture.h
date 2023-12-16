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

#ifndef CameraImageCapture_h
#define CameraImageCapture_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "CameraImageCaptureImpl.h"
#include "Dictionary.h"
#include "EventTarget.h"

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CameraControl;
class CameraErrorCallback;
class CameraMediaRecorder;
class CameraSettingErrorCallback;
class CameraSuccessCallback;
class CameraImageCapture;
class ScriptExecutionContext;

class CameraImageCapture : public ActiveDOMObject, public EventTarget, public RefCounted<CameraImageCapture> {
public:
    static PassRefPtr<CameraImageCapture> create(ScriptExecutionContext* context, CameraControl* cameraControl) { return adoptRef(new CameraImageCapture(context, cameraControl)); }
    CameraImageCapture(ScriptExecutionContext*, CameraControl*);
    virtual ~CameraImageCapture();

    virtual const WTF::AtomicString& interfaceName() const OVERRIDE { return eventNames().interfaceForCameraImageCapture; }
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE { return ActiveDOMObject::scriptExecutionContext(); }
    DEFINE_ATTRIBUTE_EVENT_LISTENER(shutter);

    using RefCounted<CameraImageCapture>::ref;
    using RefCounted<CameraImageCapture>::deref;

    void applySettings(const Dictionary& settings, PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraSettingErrorCallback>);
    void takePicture(PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraErrorCallback>);
    void onShutter();
    void release();

protected:
    virtual EventTargetData* eventTargetData() OVERRIDE { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() OVERRIDE { return &m_eventTargetData; }

private:
    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }

    RefPtr<CameraImageCaptureImpl> m_camImageCaptureImpl;
    CameraControl* m_cameraControl;
    EventTargetData m_eventTargetData;
    bool m_released;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStream_h
