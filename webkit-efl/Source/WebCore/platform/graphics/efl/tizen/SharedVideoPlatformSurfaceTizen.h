/*
    Copyright (C) 2012 Samsung Electronics.

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

#ifndef SharedVideoPlatformSurfaceTizen_h
#define SharedVideoPlatformSurfaceTizen_h

#if USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)

#include "IntSize.h"
#include <Ecore.h>
#include <Ecore_X.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class GraphicsContext;
class IntRect;
class VideoPlatformSurface;

class VideoPlatformSurfaceUpdateListener {
public:
    virtual void platformSurfaceUpdated() = 0;
};

class SharedVideoPlatformSurfaceTizen {
public:
    enum CopySurfaceType {
        FitToWidth,
        FitToHeight,
        FitToRect
    };

    static PassOwnPtr<SharedVideoPlatformSurfaceTizen> create(IntSize size);
    ~SharedVideoPlatformSurfaceTizen();

    int id() const;
    IntSize size();
    void platformSurfaceUpdated();
    void setVideoPlatformSurfaceUpdateListener(VideoPlatformSurfaceUpdateListener* listener);
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);
    void copySurface(SharedVideoPlatformSurfaceTizen*, CopySurfaceType);
    int graphicsSurfaceFlags() const;

private:
    SharedVideoPlatformSurfaceTizen(IntSize size);

    void registerDamageHandler();
    void unregisterDamageHandler();

    Ecore_X_Damage m_damage;
    Ecore_Event_Handler* m_damageHandler;
    VideoPlatformSurfaceUpdateListener* m_listener;
    OwnPtr<VideoPlatformSurface> m_platformSurface;
    int m_flags;
};

} // WebCore

#endif // USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)

#endif // SharedVideoPlatformSurfaceTizen_h
