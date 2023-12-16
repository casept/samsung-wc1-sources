/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformSurfacePoolTizen_h
#define PlatformSurfacePoolTizen_h

#if USE(TIZEN_PLATFORM_SURFACE)

#include "IntSize.h"
#include "PlatformSurfacePoolClientTizen.h"
#include "SharedPlatformSurfaceTizen.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>

using namespace WTF;

namespace WebCore {

class SharedPlatformSurfaceTizen;
class PlatformSurfacePoolTizen {
    WTF_MAKE_NONCOPYABLE(PlatformSurfacePoolTizen);

public :
    PlatformSurfacePoolTizen(PlatformSurfacePoolClient* client);
    ~PlatformSurfacePoolTizen();
    SharedPlatformSurfaceTizen* acquirePlatformSurface(const WebCore::IntSize&, int tileID);
    SharedPlatformSurfaceTizen* acquirePlatformSurfaceByID(int platformSurfaceId);
    void freePlatformSurface(int platformSurfaceId);
    void freePlatformSurfaceByTileID(int tileID);
    void removePlatformSurface(int platformSurfaceId);
    void willRemovePlatformSurface(int platformSurfaceId);
    void shrinkIfNeeded();

private :
    class PlatformSurfaceInfo : public RefCounted<PlatformSurfaceInfo> {
    public :
        PlatformSurfaceInfo(const IntSize&);
        const IntSize size() const { return m_SharedPlatformSurfaceTizen ? m_SharedPlatformSurfaceTizen->size() : IntSize(); }
        size_t sizeInByte() { return size().width() * size().height() * 4; }
        bool m_used;
        bool m_willBeRemoved;
        OwnPtr<SharedPlatformSurfaceTizen> m_SharedPlatformSurfaceTizen;
        int m_age;
        int m_tileID;
    };

    bool canCreatePlatformSurface();
    RefPtr<PlatformSurfaceInfo> createPlatformSurface(const IntSize&);
    void freePlatformSurfacesIfNeeded();
    void shrink();
    void startRemovePlaformSurfaceTimer();
    void removePlaformSurfaceTimerFired(Timer<PlatformSurfacePoolTizen>*);
    void removePlatformSurfacesIfNeeded();

    void startShrinkTimer();
    void shrinkTimerFired(Timer<PlatformSurfacePoolTizen>*);

    typedef HashMap<int, RefPtr<PlatformSurfaceInfo> > PlatformSurfaceMap;
    PlatformSurfaceMap m_platformSurfaces;

    Vector<RefPtr<PlatformSurfaceInfo> > m_platformSurfacesToFree;
    Vector<int> m_platformSurfacesIdToRemove;

    SharedPlatformSurfaceManagement& m_sharedPlatformSurfaceManagement;

    size_t m_sizeInByte;
    size_t m_maxSizeInByte;
    size_t m_maxSizeInCount;
    size_t m_usedSizeInByte;
    size_t m_usedSizeInCount;

    Timer<PlatformSurfacePoolTizen> m_removePlaformSurfaceTimer;
    Timer<PlatformSurfacePoolTizen> m_shrinkTimer;
    bool m_needToShrink;

    PlatformSurfacePoolClient* m_client;
};
}
#endif
#endif
