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
#include "CameraControlImpl.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "LocalMediaServer.h"

namespace WebCore {

CameraControlImpl::CameraControlImpl(CameraControl* cameraControl)
    : m_cameraControl(cameraControl)
{
}

CameraControlImpl::~CameraControlImpl()
{
}

GstElement* CameraControlImpl::cameraSource() const
{
    return LocalMediaServer::instance().camSource();
}

void CameraControlImpl::setFocusArea(int top, int left, int bottom, int right)
{
    GstCameraControl* control = GST_CAMERA_CONTROL(cameraSource());
    ASSERT(control);
    m_focusArea.x = top;
    m_focusArea.y = left;
    m_focusArea.width = bottom - top;
    m_focusArea.height = right - left;

    if (control)
        gst_camera_control_set_auto_focus_area(control, m_focusArea);
}

bool CameraControlImpl::autoFocus()
{
    if (LocalMediaServer::instance().srcType() == LocalMediaServer::V4l2Src)
        return true;

    GstCameraControl* control = GST_CAMERA_CONTROL(cameraSource());
    if (!control)
        return false;

    TIZEN_LOGI("[Media] start auto focus");
    return gst_camera_control_start_auto_focus(control);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
