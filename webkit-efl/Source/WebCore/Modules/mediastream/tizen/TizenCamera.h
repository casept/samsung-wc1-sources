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

#ifndef TizenCamera_h
#define TizenCamera_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CameraControl;
class CameraErrorCallback;
class CreateCameraSuccessCallback;
class MediaStream;
class ScriptExecutionContext;

class TizenCamera : public RefCounted<TizenCamera> {
public:
    static PassRefPtr<TizenCamera> create(ScriptExecutionContext* context) { return adoptRef(new TizenCamera(context)); }
    TizenCamera(ScriptExecutionContext*);
    virtual ~TizenCamera();
    void createCameraControl(MediaStream*, PassRefPtr<CreateCameraSuccessCallback> successCallback, PassRefPtr<CameraErrorCallback> errorCallback);

private:
    ScriptExecutionContext* m_context;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStream_h
