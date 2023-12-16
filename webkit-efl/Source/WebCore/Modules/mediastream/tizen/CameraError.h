/*
    Copyright (C) 2014 Samsung Electronics.

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

#ifndef CameraError_h
#define CameraError_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CameraError : public RefCounted<CameraError> {
public:
    enum Code {
        CREATION_FAILED = 0,
        PERMISSION_DENIED,
        NO_CAMERA,
        PIPELINE_ERR,
        FILE_WRITE_ERR
    };
    static PassRefPtr<CameraError> create(Code code) { return adoptRef(new CameraError(code)); }

    Code code() const { return m_code; }

private:
    CameraError(Code code) : m_code(code) { }

    Code m_code;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // CameraError_h

