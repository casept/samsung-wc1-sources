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

#ifndef PlatformSurfaceTexturePoolTizen_h
#define PlatformSurfaceTexturePoolTizen_h

#if USE(TIZEN_PLATFORM_SURFACE)
#include "PlatformSurfaceTextureGL.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class TextureMapper;
class PlatformSurfaceTexture;

class PlatformSurfaceTexturePool {
public:
    PlatformSurfaceTexturePool() { }
    ~PlatformSurfaceTexturePool();
    RefPtr<PlatformSurfaceTexture> acquireTextureFromPool(TextureMapper*, int, const IntSize&);
    void returnTextureToPool(int platformSurfaceId);

    Vector<int> platformSurfacesToFree();
    void addPlatformSurfacesToFree(int platformSurfaceId);
    void clearPlatformSurfacesToFree();

    void clear();

    void platformSurfacesToRemove(Vector<int>&);
    void removePlatformSurfaceTexture(int);
private:
    HashMap<int, RefPtr<PlatformSurfaceTexture> > m_platformSurfaceTextures;
    Vector<int> m_platformSurfacesToFree;

};

}

#endif
#endif

