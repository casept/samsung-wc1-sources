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
#include "PlatformSurfaceTexturePoolTizen.h"

namespace WebCore {

PlatformSurfaceTexturePool::~PlatformSurfaceTexturePool()
{
    clear();
}

RefPtr<WebCore::PlatformSurfaceTexture> PlatformSurfaceTexturePool::acquireTextureFromPool(TextureMapper* textureMapper, int platformSurfaceId, const IntSize& platformSurfaceSize)
{
    HashMap<int, RefPtr<WebCore::PlatformSurfaceTexture> >::iterator foundTexture = m_platformSurfaceTextures.find(platformSurfaceId);
    if (foundTexture == m_platformSurfaceTextures.end()) {
        RefPtr<WebCore::PlatformSurfaceTexture> newPlatformSurfaceTexture = textureMapper->createPlatformSurfaceTexture(platformSurfaceId, platformSurfaceSize);
        foundTexture = m_platformSurfaceTextures.add(platformSurfaceId, newPlatformSurfaceTexture).iterator;
    }

    foundTexture->value->setUsed(true);
    return m_platformSurfaceTextures.get(platformSurfaceId);
}

void PlatformSurfaceTexturePool::returnTextureToPool(int platformSurfaceId)
{
    // FIXME : PlatformSurfaceTextures in PlatformSurfacesToFree of CoordinatedBackingStore can be removed in removeUnusedPlatformSurfaceTextures().
    // If swapping platformSurfaceTexture moves updatePlatformSurfaceTile() to swapBuffers(), it will be fixed.
    HashMap<int, RefPtr<WebCore::PlatformSurfaceTexture> >::iterator foundTexture = m_platformSurfaceTextures.find(platformSurfaceId);
    if (foundTexture == m_platformSurfaceTextures.end())
        return;

    foundTexture->value->setUsed(false);
}

Vector<int> PlatformSurfaceTexturePool::platformSurfacesToFree()
{
    return m_platformSurfacesToFree;
}

void PlatformSurfaceTexturePool::addPlatformSurfacesToFree(int platformSurfaceId)
{
    m_platformSurfacesToFree.append(platformSurfaceId);
}

void PlatformSurfaceTexturePool::clearPlatformSurfacesToFree()
{
    m_platformSurfacesToFree.clear();
}

void PlatformSurfaceTexturePool::clear()
{
    m_platformSurfacesToFree.clear();
    m_platformSurfaceTextures.clear();
}

void PlatformSurfaceTexturePool::platformSurfacesToRemove(Vector<int>& removes)
{
    HashMap<int, RefPtr<WebCore::PlatformSurfaceTexture> >::iterator end = m_platformSurfaceTextures.end();
    for (HashMap<int, RefPtr<WebCore::PlatformSurfaceTexture> >::iterator iter = m_platformSurfaceTextures.begin(); iter != end; ++iter) {
        if (!iter->value->used()) {
            removes.append(iter->key);
            iter->value.release();
        }
    }

    size_t size = removes.size();
    for (size_t index = 0; index < size; index++)
        m_platformSurfaceTextures.remove(removes[index]);
}

void PlatformSurfaceTexturePool::removePlatformSurfaceTexture(int platformSurfaceId)
{
    HashMap<int, RefPtr<WebCore::PlatformSurfaceTexture> >::iterator foundTexture = m_platformSurfaceTextures.find(platformSurfaceId);
    if (foundTexture != m_platformSurfaceTextures.end()) {
        foundTexture->value.release();
        m_platformSurfaceTextures.remove(foundTexture);
    }
}

}

#endif
