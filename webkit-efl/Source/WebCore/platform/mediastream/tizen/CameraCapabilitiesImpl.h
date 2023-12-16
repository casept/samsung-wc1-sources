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

#ifndef CameraCapabilitiesImpl_h
#define CameraCapabilitiesImpl_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CameraSize.h"

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>

namespace WebCore {

class CameraCapabilitiesImpl : public RefCounted<CameraCapabilitiesImpl> {
public:
    static PassRefPtr<CameraCapabilitiesImpl> create(bool isAudioOnlyStream) { return adoptRef(new CameraCapabilitiesImpl(isAudioOnlyStream)); }
    CameraCapabilitiesImpl(bool);
    virtual ~CameraCapabilitiesImpl();

    Vector<RefPtr<CameraSize>>& pictureSizes();
    Vector<String>& pictureFormats();
    Vector<String>& recordingFormats();

    bool isValidPictureSize(int width, int height);
    bool isValidPictureFormat(String format);
    bool isValidRecordingFormat(String format);

private:
    void initialize();
    Vector<RefPtr<CameraSize>> m_pictureSizes;
    Vector<String>     m_pictureFormats;
    Vector<String>     m_recordingFormats;
    bool m_isAudioOnlyStream;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // CameraCapabilitiesImpl_h

