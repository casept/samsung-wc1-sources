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

#ifndef NavigatorTizenCamera_h
#define NavigatorTizenCamera_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "DOMWindowProperty.h"
#include "Supplementable.h"

namespace WebCore {

class Frame;
class Navigator;
class TizenCamera;

class NavigatorTizenCamera : public Supplement<Navigator>, public DOMWindowProperty {
public:
    virtual ~NavigatorTizenCamera();
    static NavigatorTizenCamera* from(Navigator*);

    static TizenCamera* tizCamera(Navigator*);
    TizenCamera* tizCamera() const;

private:
    NavigatorTizenCamera(Frame*);
    static const char* supplementName();
    mutable RefPtr<TizenCamera> m_tizenCamera;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaStream_h
