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

#ifndef CoordinatedBackingStoreTizen_h
#define CoordinatedBackingStoreTizen_h

#if USE(TIZEN_PLATFORM_SURFACE)
#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedGraphicsState.h"
#include "CoordinatedSurface.h"
#endif
#include "CoordinatedBackingStore.h"
#include "PlatformSurfaceTexturePoolTizen.h"

namespace WebCore {

class CoordinatedBackingStoreTileTizen : public CoordinatedBackingStoreTile {
public:
    CoordinatedBackingStoreTileTizen(float scale = 1)
        : CoordinatedBackingStoreTile(scale)
        , m_platformSurfaceId(0)
        , m_textureId(0)
        , m_platformSurfaceTexture(0)
    {
    }

    void paintPlatformSurfaceTile(WebCore::TextureMapper*, const WebCore::TransformationMatrix&, float);
    void setPlatformSurfaceTexture(const WebCore::IntRect& sourceRect, const WebCore::IntRect& targetRect, PassRefPtr<WebCore::PlatformSurfaceTexture>);
    int platformSurfaceId() { return m_platformSurfaceId; }
    PassRefPtr<WebCore::PlatformSurfaceTexture> platformSurfaceTexture() { return m_platformSurfaceTexture; }

private:
    int m_platformSurfaceId;
    uint32_t m_textureId;
    WebCore::IntSize m_platformSurfaceSize;
    RefPtr<WebCore::PlatformSurfaceTexture> m_platformSurfaceTexture;
};

class CoordinatedBackingStoreTizen : public CoordinatedBackingStore {
public:
    void createTile(uint32_t, float);
    void removeTile(uint32_t);
    bool isBackedBySharedPlatformSurface() { return true; }
    static PassRefPtr<CoordinatedBackingStoreTizen> create() { return adoptRef(new CoordinatedBackingStoreTizen); }
    void commitTileOperations(WebCore::TextureMapper*);
    PassRefPtr<WebCore::BitmapTexture> texture() const;
    void paintToTextureMapper(WebCore::TextureMapper*, const WebCore::FloatRect&, const WebCore::TransformationMatrix&, float);

    ~CoordinatedBackingStoreTizen();
    void clearFreePlatformSurfaceTiles() { m_freePlatformSurfaces.clear(); }
    const Vector<int>* freePlatformSurfaceTiles() { return &m_freePlatformSurfaces; }
    int layerId() const { return m_layerId; }
    void updatePlatformSurfaceTile(int id, const WebCore::IntRect& sourceRect, const WebCore::IntRect& target, int layerId, int platformSurfaceId, const WebCore::IntSize& platformSurfaceSize, bool partialUpdate, WebCore::TextureMapper* textureMapper);
    int tilePlatformSurfaceId(int tileId);
    void setPlatformSurfaceTexturePool(PlatformSurfaceTexturePool*);
    void freeAllPlatformSurfacesAsFree();

private:
    CoordinatedBackingStoreTizen()
        : CoordinatedBackingStore()
        , m_layerId(0)
    { }

    HashMap<int, CoordinatedBackingStoreTileTizen> m_platformSurfaceTiles;
    Vector<int> m_freePlatformSurfaces;
    uint32_t m_layerId;
    PlatformSurfaceTexturePool* m_platformSurfaceTexturePool;
};

} // namespace WebKit

#endif // USE(TIZEN_PLATFORM_SURFACE)
#endif
