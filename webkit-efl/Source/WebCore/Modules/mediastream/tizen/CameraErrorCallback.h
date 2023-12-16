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

#ifndef CameraErrorCallback_h
#define CameraErrorCallback_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class CameraError;

class CameraErrorCallback : public RefCounted<CameraErrorCallback> {
public:
    virtual ~CameraErrorCallback() { }
    virtual bool handleEvent(CameraError* error) = 0;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // CameraErrorCallback_h