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

#ifndef CameraMediaRecorder_h
#define CameraMediaRecorder_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "Dictionary.h"
#include "EventTarget.h"
#include "MediaRecorder.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CameraControl;
class CameraErrorCallback;
class CameraMediaRecorder;
class CameraSettingErrorCallback;
class CameraSuccessCallback;
class ScriptExecutionContext;

class CameraMediaRecorder : public ActiveDOMObject, public EventTarget, public RefCounted<CameraMediaRecorder>, public MediaRecorderClient {
public:
    static PassRefPtr<CameraMediaRecorder> create(ScriptExecutionContext* context, CameraControl* cameraControl) { return adoptRef(new CameraMediaRecorder(context, cameraControl)); }
    CameraMediaRecorder(ScriptExecutionContext*, CameraControl*);
    virtual ~CameraMediaRecorder();

    virtual const WTF::AtomicString& interfaceName() const OVERRIDE { return eventNames().interfaceForCameraMediaRecorder; }
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE { return ActiveDOMObject::scriptExecutionContext(); }
    DEFINE_ATTRIBUTE_EVENT_LISTENER(recordingstatechange);

    using RefCounted<CameraMediaRecorder>::ref;
    using RefCounted<CameraMediaRecorder>::deref;

    String state() const { return m_state; }

    void applySettings(const Dictionary& settings, PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraSettingErrorCallback>);
    void start(PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraErrorCallback>);
    void stop(PassRefPtr<CameraSuccessCallback>, PassRefPtr<CameraErrorCallback>);

    virtual void onRecordingStateChange(const String& state) OVERRIDE;

    void release();

protected:
    virtual EventTargetData* eventTargetData() OVERRIDE { return &m_eventTargetData; }
    virtual EventTargetData* ensureEventTargetData() OVERRIDE { return &m_eventTargetData; }

private:
    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }

    CameraControl* m_cameraControl;
    OwnPtr<MediaRecorder> m_recorder;
    EventTargetData m_eventTargetData;
    String m_state;
    bool m_released;
    String m_recordingFormat;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStream_h
