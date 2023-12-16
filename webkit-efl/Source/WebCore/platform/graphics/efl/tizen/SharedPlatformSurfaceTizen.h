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

#ifndef SharedPlatformSurfaceTizen_h
#define SharedPlatformSurfaceTizen_h

#if USE(TIZEN_PLATFORM_SURFACE)

#include "IntSize.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

extern "C" {
#include <tbm_bufmgr.h>
}

namespace WebCore {

class PixmapContextTizen;

// FIXME : A process is limited to 1024 fd each.
// Each sharedPlatformSurface needs to open 3 fd for locking system.
class SharedPlatformSurfaceManagement {
protected:
    SharedPlatformSurfaceManagement(void) : m_sharedPlatformSurfaceCount(0) { }

public:
    ~SharedPlatformSurfaceManagement() {}

    static inline SharedPlatformSurfaceManagement& getInstance()
    {
        static SharedPlatformSurfaceManagement sharedPlatformSurfaceManagement;
        return sharedPlatformSurfaceManagement;
    }

    bool canCreateSharedPlatformSurface(int maxSizeInCount) { return (m_sharedPlatformSurfaceCount < maxSizeInCount) ? true : false; }
    void growCount() { m_sharedPlatformSurfaceCount++; }
    void shrinkCount() { m_sharedPlatformSurfaceCount--; }
    int count() { return m_sharedPlatformSurfaceCount; }

    void deferredRemovePlatformSurface(int platformSurfaceId);
    void doDeferredRemove();

private:
    int m_sharedPlatformSurfaceCount;
    Vector<int> m_platformSurfacesIdToDeferredRemove;
};

class SharedPlatformSurfaceTizen {
public:
    static PassOwnPtr<SharedPlatformSurfaceTizen> create(const IntSize&, bool renderedByCPU, bool hasAlpha = true, bool hasDepth = true, bool hasStencil = false, bool hasSamples = false, PixmapContextTizen* pixmapContext = NULL);
    virtual ~SharedPlatformSurfaceTizen();
    bool makeContextCurrent();

    int id() { return m_pixmap; }
    void* context();
    void* display();
    void* lockTbmBuffer();
    bool unlockTbmBuffer();
    int getStride();
    const IntSize size() const { return m_size; }
    void* eglSurface() { return m_surface; }

    bool used() { return m_isUsed; }
    void setUsed(bool isUsed) { m_isUsed = isUsed; }

protected:
    SharedPlatformSurfaceTizen(const IntSize&, bool renderedByCPU, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples, PixmapContextTizen* pixmapContext);
    bool initialize();

    int m_pixmap;
    IntSize m_size;
    PixmapContextTizen* m_pixmapContext;
    void* m_surface;
    bool m_isRenderedByCPU;
    bool m_hasAlpha;
    bool m_hasDepth;
    bool m_hasStencil;
    bool m_hasSamples;
    bool m_isUsed;

    tbm_bo m_bufferObject;
};

class SharedPlatformSurfaceSimpleTizen :public SharedPlatformSurfaceTizen {
public:
    static PassOwnPtr<SharedPlatformSurfaceSimpleTizen> create(const IntSize&, int pixmapId);
    ~SharedPlatformSurfaceSimpleTizen();

private:
    SharedPlatformSurfaceSimpleTizen(const IntSize& size, int pixmapId);
    bool initialize();
};
}
#endif

#endif // GLContextTizen_h
