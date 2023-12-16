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

#include "config.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "NavigatorTizenCamera.h"

#include "Document.h"
#include "Frame.h"
#include "Navigator.h"
#include "TizenCamera.h"

namespace WebCore {

NavigatorTizenCamera::NavigatorTizenCamera(Frame* frame)
    : DOMWindowProperty(frame)
{
}

NavigatorTizenCamera::~NavigatorTizenCamera()
{
}

const char* NavigatorTizenCamera::supplementName()
{
    return "NavigatorTizenCamera";
}

NavigatorTizenCamera* NavigatorTizenCamera::from(Navigator* navigator)
{
    NavigatorTizenCamera* supplement = static_cast<NavigatorTizenCamera*>(Supplement<Navigator>::from(navigator, supplementName()));
    if (!supplement) {
        supplement = new NavigatorTizenCamera(navigator->frame());
        provideTo(navigator, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

TizenCamera* NavigatorTizenCamera::tizCamera(Navigator* navigator)
{
   return NavigatorTizenCamera::from(navigator)->tizCamera();
}

TizenCamera* NavigatorTizenCamera::tizCamera() const
{
    if (!m_tizenCamera && frame())
        m_tizenCamera = TizenCamera::create(frame()->document());
    return m_tizenCamera.get();
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

