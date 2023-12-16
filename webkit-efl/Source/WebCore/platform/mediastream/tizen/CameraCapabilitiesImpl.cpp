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

#include "CameraCapabilitiesImpl.h"
#include "LocalMediaServer.h"

namespace WebCore {

CameraCapabilitiesImpl::CameraCapabilitiesImpl(bool isAudioOnlyStream)
    : m_isAudioOnlyStream(isAudioOnlyStream)
{
    initialize();
}

CameraCapabilitiesImpl::~CameraCapabilitiesImpl()
{
}

void CameraCapabilitiesImpl::initialize()
{
    CameraDeviceCapabilities& cameraDeviceCapabilities = CameraDeviceCapabilities::instance();
    for (unsigned i = 0; i < cameraDeviceCapabilities.supportedCaptureResolutions().size(); i++) {
        IntSize& size = cameraDeviceCapabilities.supportedCaptureResolutions()[i];
        m_pictureSizes.append(CameraSize::create(size.width(), size.height()));
    }

    m_pictureFormats.append(String("jpeg"));
    m_pictureFormats.append(String("png"));

    m_recordingFormats.append(String("3gp"));
    if (m_isAudioOnlyStream) {
        m_recordingFormats.append(String("m4a"));
        m_recordingFormats.append(String("amr"));
        m_recordingFormats.append(String("pcm"));
        m_recordingFormats.append(String("aac"));
    }
}

Vector<RefPtr<CameraSize>>& CameraCapabilitiesImpl::pictureSizes()
{
    return m_pictureSizes;
}

Vector<String>& CameraCapabilitiesImpl::pictureFormats()
{
    return m_pictureFormats;
}

Vector<String>& CameraCapabilitiesImpl::recordingFormats()
{
    return m_recordingFormats;
}

bool CameraCapabilitiesImpl::isValidPictureSize(int width, int height)
{
    for (size_t i = 0; i < m_pictureSizes.size(); i++) {
        if (m_pictureSizes[i]->width() == width && m_pictureSizes[i]->height() == height)
            return true;
    }

    return false;
}

bool CameraCapabilitiesImpl::isValidPictureFormat(String format)
{
    for (size_t i = 0; i < m_pictureFormats.size(); i++) {
        if (m_pictureFormats[i] == format)
            return true;
    }
    return false;
}

bool CameraCapabilitiesImpl::isValidRecordingFormat(String format)
{
    for (size_t i = 0; i < m_recordingFormats.size(); i++) {
        if (m_recordingFormats[i] == format)
            return true;
    }
    return false;
}


} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

