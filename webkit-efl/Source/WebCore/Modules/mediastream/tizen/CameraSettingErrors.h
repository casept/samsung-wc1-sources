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

#ifndef CameraSettingErrors_h
#define CameraSettingErrors_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class CameraSettingErrors : public RefCounted<CameraSettingErrors> {
public:
    enum Code {
        FOCUS_AREA_ERR = 0,
        PICTURE_SIZE_ERR,
        PICTURE_FORMAT_ERR,
        RECORDING_FORMAT_ERR,
        FILENAME_ERR,
        MAX_FILE_SIZE_BYTES_ERR
    };
    static PassRefPtr<CameraSettingErrors> create() { return adoptRef(new CameraSettingErrors()); }
    virtual ~CameraSettingErrors() { }

    const Vector<String>& codes() const { return m_codes; }
    void append(const Code& code) { m_codes.append(String::format("%d", code)); }

private:
    CameraSettingErrors() { }

    Vector<String> m_codes;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // CameraSettingErrors_h

