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

#include "config.h"

#if USE(TIZEN_PLATFORM_SURFACE)
#include "Logging.h"
#include "CoordinatedBackingStoreTizen.h"

namespace WebCore {

void CoordinatedBackingStoreTileTizen::paintPlatformSurfaceTile(WebCore::TextureMapper* textureMapper, const WebCore::TransformationMatrix& transform, float opacity)
{
    FloatRect targetRect(m_tileRect);
    targetRect.scale(1. / m_scale);
    if (targetRect != rect())
        setRect(targetRect);

    IntSize textureSize = m_tileRect.size();

    // FIXME : rebase
    // ShouldBlend | ShouldScaleTexture
    textureMapper->drawTexture(m_textureId, 0x01 | 0x10 , textureSize, rect(), transform, opacity, m_platformSurfaceSize);

#if ENABLE(TIZEN_RENDERING_LOG)
    LOG(AcceleratedCompositing, "[UI ]   coord (%d, %d), size (%d, %d), platformSurface %u @CoordinatedBackingStoreTile::paintPlatformSurfaceTile \n", m_tileRect.x(), m_tileRect.y(), m_platformSurfaceSize.width(), m_platformSurfaceSize.height(), m_platformSurfaceId);
#endif
}

void CoordinatedBackingStoreTileTizen::setPlatformSurfaceTexture(const IntRect& sourceRect, const IntRect& targetRect, PassRefPtr<PlatformSurfaceTexture> platformSurfaceTexture)
{
    m_sourceRect = sourceRect;
    m_tileRect = targetRect;
    m_platformSurfaceId = platformSurfaceTexture->platformSurfaceId();
    m_textureId = platformSurfaceTexture->id();
    m_platformSurfaceSize = platformSurfaceTexture->size();
    m_platformSurfaceTexture = platformSurfaceTexture;
}

CoordinatedBackingStoreTizen::~CoordinatedBackingStoreTizen()
{
    freeAllPlatformSurfacesAsFree();
}

void CoordinatedBackingStoreTizen::createTile(uint32_t id, float scale)
{
    m_platformSurfaceTiles.add(id, CoordinatedBackingStoreTileTizen(scale));
    m_scale = scale;
}

void CoordinatedBackingStoreTizen::removeTile(uint32_t id)
{
    if (!m_platformSurfaceTiles.contains(id))
        return;

    HashMap<int, CoordinatedBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.find(id);
    if (it->value.platformSurfaceId() > 0)
        m_platformSurfaceTexturePool->returnTextureToPool(it->value.platformSurfaceId());
    m_platformSurfaceTiles.remove(id);
}

void CoordinatedBackingStoreTizen::commitTileOperations(TextureMapper* textureMapper)
{
    HashMap<int, CoordinatedBackingStoreTileTizen>::iterator end = m_platformSurfaceTiles.end();
    for (HashMap<int, CoordinatedBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.begin(); it != end; ++it)
        it->value.swapBuffers(textureMapper);
}

PassRefPtr<BitmapTexture> CoordinatedBackingStoreTizen::texture() const
{
    HashMap<int, CoordinatedBackingStoreTileTizen>::const_iterator end = m_platformSurfaceTiles.end();
    for (HashMap<int, CoordinatedBackingStoreTileTizen>::const_iterator it = m_platformSurfaceTiles.begin(); it != end; ++it) {
        RefPtr<BitmapTexture> texture = it->value.texture();
        if (texture)
            return texture;
    }

    return PassRefPtr<BitmapTexture>();
}

static bool shouldShowTileDebugVisualsTizen()
{
#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    return getenv("TIZEN_WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS") ? (atoi(getenv("TIZEN_WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS")) == 1) : false;
#endif
    return false;
}

void CoordinatedBackingStoreTizen::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& /*targetRect*/, const TransformationMatrix& transform, float opacity)
{
    Vector<CoordinatedBackingStoreTileTizen*> tilesToPaint;

    // We have to do this every time we paint, in case the opacity has changed.
    HashMap<int, CoordinatedBackingStoreTileTizen>::iterator end = m_platformSurfaceTiles.end();

    FloatRect coveredRect;
    for (HashMap<int, CoordinatedBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.begin(); it != end; ++it) {
        if (!it->value.platformSurfaceId())
            continue;

        if (it->value.scale() == m_scale) {
            tilesToPaint.append(&it->value);
            coveredRect.unite(it->value.rect());
            continue;
        }

        // Only show the previous tile if the opacity is high, otherwise effect looks like a bug.
        // We show the previous-scale tile anyway if it doesn't intersect with any current-scale tile.
        if (opacity < 0.95 && coveredRect.intersects(it->value.rect()))
            continue;

        tilesToPaint.insert(0, &it->value);
        coveredRect.unite(it->value.rect());
    }

#if ENABLE(TIZEN_RENDERING_LOG)
    LOG(AcceleratedCompositing, "[UI ] layer id(%u) paint %d tiles (of %d tiles) @%s\n",
        m_layerId, tilesToPaint.size(), m_tiles.size(), "CoordinatedBackingStoreTizen::paintToTextureMapper");
#endif

    static bool shouldDebug = shouldShowTileDebugVisualsTizen();
    for (size_t i = 0; i < tilesToPaint.size(); ++i) {
        tilesToPaint[i]->paintPlatformSurfaceTile(textureMapper, transform, opacity);

        if (!shouldDebug)
            continue;

        textureMapper->drawBorder(Color(0xFF, 0, 0), 2, tilesToPaint[i]->rect(), transform);
        // FIXME : rebase
        // Below line is not essential. (fix build break.)
        //textureMapper->drawNumber(static_cast<CoordinatedBackingStoreTile*>(tilesToPaint[i])->repaintCount(), 8, tilesToPaint[i]->rect().location(), transform);
    }
}

void CoordinatedBackingStoreTizen::updatePlatformSurfaceTile(int id, const IntRect& sourceRect, const WebCore::IntRect& targetRect, int layerId, int platformSurfaceId, const WebCore::IntSize& platformSurfaceSize, bool partialUpdate, TextureMapper* textureMapper)
{
    if (!m_layerId)
        m_layerId = layerId;

    if (!m_platformSurfaceTiles.contains(id))
        createTile(id, m_scale);

    HashMap<int, CoordinatedBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.find(id);
    ASSERT(it != m_platformSurfaceTiles.end());

    if (partialUpdate && it->value.platformSurfaceTexture()) {
        RefPtr<PlatformSurfaceTexture> sourceTexture = m_platformSurfaceTexturePool->acquireTextureFromPool(textureMapper, platformSurfaceId, platformSurfaceSize);
        it->value.platformSurfaceTexture()->copyPlatformSurfaceToTextureGL(sourceRect, sourceTexture->sharedPlatformSurfaceSimple());
        m_freePlatformSurfaces.append(platformSurfaceId);
        m_platformSurfaceTexturePool->returnTextureToPool(platformSurfaceId);
    } else {
        RefPtr<PlatformSurfaceTexture> platformSurfaceTexture = m_platformSurfaceTexturePool->acquireTextureFromPool(textureMapper, platformSurfaceId, platformSurfaceSize);
        if (it->value.platformSurfaceId() > 0) {
            m_freePlatformSurfaces.append(it->value.platformSurfaceId());
            m_platformSurfaceTexturePool->returnTextureToPool(it->value.platformSurfaceId());
        }
        it->value.setPlatformSurfaceTexture(sourceRect, targetRect, platformSurfaceTexture);
        it->value.setTexture(static_cast<BitmapTexture*>(platformSurfaceTexture.get()));
    }
}

int CoordinatedBackingStoreTizen::tilePlatformSurfaceId(int tileId)
{
    if (!m_platformSurfaceTiles.contains(tileId))
        return 0;

    HashMap<int, CoordinatedBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.find(tileId);
    return it->value.platformSurfaceId();
}

void CoordinatedBackingStoreTizen::setPlatformSurfaceTexturePool(PlatformSurfaceTexturePool* texturePool)
{
    m_platformSurfaceTexturePool = texturePool;
}

void CoordinatedBackingStoreTizen::freeAllPlatformSurfacesAsFree()
{
    // When destroying BackingStore, all contained tiles are set to free.
    HashMap<int, CoordinatedBackingStoreTileTizen>::iterator end = m_platformSurfaceTiles.end();
    for (HashMap<int, CoordinatedBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.begin(); it != end; ++it) {
        if (it->value.platformSurfaceId() > 0) {
            m_platformSurfaceTexturePool->returnTextureToPool(it->value.platformSurfaceId());
            m_platformSurfaceTexturePool->addPlatformSurfacesToFree(it->value.platformSurfaceId());
        }
    }
    for (unsigned i = 0; i < m_freePlatformSurfaces.size(); ++i)
        m_platformSurfaceTexturePool->addPlatformSurfacesToFree(m_freePlatformSurfaces[i]);
    m_platformSurfaceTiles.clear();
}
}
#endif // USE(TIZEN_PLATFORM_SURFACE)

