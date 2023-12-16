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

#include "config.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "CameraCapabilities.h"
#include <wtf/RefPtr.h>

namespace WebCore {

CameraCapabilities::CameraCapabilities(bool isAudioOnlyStream)
{
    m_cameraCapImpl = adoptRef(new CameraCapabilitiesImpl(isAudioOnlyStream));
}

CameraCapabilities::~CameraCapabilities()
{
}

Vector<RefPtr<CameraSize>>& CameraCapabilities::pictureSizes()
{
    return m_cameraCapImpl->pictureSizes();
}

Vector<String> CameraCapabilities::pictureFormats()
{
    return m_cameraCapImpl->pictureFormats();
}

Vector<String> CameraCapabilities::recordingFormats()
{
    return m_cameraCapImpl->recordingFormats();
}

bool CameraCapabilities::isValidPictureSize(int width, int height)
{
    return m_cameraCapImpl->isValidPictureSize(width, height);
}

bool CameraCapabilities::isValidPictureFormat(String format)
{
    return m_cameraCapImpl->isValidPictureFormat(format);
}

bool CameraCapabilities::isValidRecordingFormat(String format)
{
    return m_cameraCapImpl->isValidRecordingFormat(format);
}

} // namespace WebCore

#endif // TIZEN_MEDIA_STREAM

