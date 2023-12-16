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
#include "PlatformSurfacePoolTizen.h"
#include <wtf/HashMap.h>

namespace WebCore {

static const int KB = 1024;
static const int MB = 1024 * 1024;

static inline bool reachLowPoolUtilization(int usedSize, int totalSize)
{
    // lowPoolUtilization threshold is 70%.
    return (usedSize < static_cast<int>(static_cast<float>(totalSize) * 0.7));

}
static inline bool reachHighPoolUtilization(int usedSize, int totalSize)
{
    // highPoolUtilization threshold is 90%.
    return (usedSize > static_cast<int>(static_cast<float>(totalSize) * 0.9));
}

PlatformSurfacePoolTizen::PlatformSurfaceInfo::PlatformSurfaceInfo(const IntSize& size)
    : m_used(false)
    , m_willBeRemoved(false)
    , m_age(0)
    , m_tileID(0)
{
    m_SharedPlatformSurfaceTizen =  WebCore::SharedPlatformSurfaceTizen::create(size, true);
}

PlatformSurfacePoolTizen::PlatformSurfacePoolTizen(PlatformSurfacePoolClient* client)
    : m_sharedPlatformSurfaceManagement(SharedPlatformSurfaceManagement::getInstance())
    , m_sizeInByte(0)
#if ENABLE(TIZEN_WEARABLE_PROFILE)
    , m_maxSizeInByte(8 * MB)  // 8 MB
#else
    , m_maxSizeInByte(128 * MB)  // 128 MB
#endif
    , m_maxSizeInCount(200) // XPixmap count is limited to 1024 in system
    , m_usedSizeInByte(0)
    , m_usedSizeInCount(0)
    , m_removePlaformSurfaceTimer(this, &PlatformSurfacePoolTizen::removePlaformSurfaceTimerFired)
    , m_shrinkTimer(this, &PlatformSurfacePoolTizen::shrinkTimerFired)
    , m_needToShrink(false)
    , m_client(client)
{
}

PlatformSurfacePoolTizen::~PlatformSurfacePoolTizen()
{
    m_platformSurfacesToFree.clear();

    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        iter->value->m_SharedPlatformSurfaceTizen.release();
        iter->value.release();
    }
    m_platformSurfaces.clear();
}

SharedPlatformSurfaceTizen* PlatformSurfacePoolTizen::acquirePlatformSurface(const IntSize& size, int tileID)
{
    freePlatformSurfacesIfNeeded();
    removePlatformSurfacesIfNeeded();
    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        RefPtr<PlatformSurfaceInfo> platformSurfaceInfo = iter->value;
        if (platformSurfaceInfo->m_used || platformSurfaceInfo->m_willBeRemoved)
            continue;

        if (platformSurfaceInfo->size() != size)
            platformSurfaceInfo->m_age++;
        else {
            platformSurfaceInfo->m_used = true;
            platformSurfaceInfo->m_age = 0;
            platformSurfaceInfo->m_tileID = tileID;

            // update statistics
            m_usedSizeInByte += platformSurfaceInfo->sizeInByte();
            m_usedSizeInCount++;

#if ENABLE(TIZEN_RENDERING_LOG)
            LOG(TiledAC, "XPlatformSurface pool size : %u KB (%u tiles) used : %u KB (%u tiles) at [%s]",
                m_sizeInByte/KB, m_platformSurfaces.size(), m_usedSizeInByte/KB, m_usedSizeInCount, "PlatformSurfacePoolTizen::acquirePlatformSurface");
#endif
            return platformSurfaceInfo->m_SharedPlatformSurfaceTizen.get();
        }
    }

    if (canCreatePlatformSurface()) {
        RefPtr<PlatformSurfaceInfo> newPlatformSurfaceInfo = createPlatformSurface(size);
        if (!newPlatformSurfaceInfo) {
#if ENABLE(TIZEN_RENDERING_LOG)
            LOG(TiledAC, "can't create new platformSurface(%dx%d) at [%s]", size.width(), size.height(), "PlatformSurfacePoolTizen::acquirePlatformSurface");
#endif
            return 0;
        }
        newPlatformSurfaceInfo->m_used = true;
        newPlatformSurfaceInfo->m_tileID = tileID;

        m_usedSizeInCount++;
        m_usedSizeInByte += newPlatformSurfaceInfo->sizeInByte();

#if ENABLE(TIZEN_RENDERING_LOG)
        LOG(TiledAC, "acquire platformSurface(%dx%d) XPlatformSurface pool size : %u KB (%u tiles) used : %u KB (%u tiles) at [%s]",
            size.width(), size.height(), m_sizeInByte/KB, m_platformSurfaces.size(), m_usedSizeInByte/KB, m_usedSizeInCount, "PlatformSurfacePoolTizen::acquirePlatformSurface");
#endif
        return newPlatformSurfaceInfo->m_SharedPlatformSurfaceTizen.get();
    }

    TIZEN_LOGI("No more platformSurface allowed: can't acquire platformSurface(%dx%d)",size.width(), size.height());
#if ENABLE(TIZEN_RENDERING_LOG)
    LOG(TiledAC, "No more platformSurface allowed: can't acquire platformSurface(%dx%d) at [%s]", size.width(), size.height(), "PlatformSurfacePoolTizen::acquirePlatformSurface");
#endif
    return 0;
}

SharedPlatformSurfaceTizen* PlatformSurfacePoolTizen::acquirePlatformSurfaceByID(int platformSurfaceId)
{
    PlatformSurfaceMap::iterator foundSurface = m_platformSurfaces.find(platformSurfaceId);
    if (foundSurface == m_platformSurfaces.end())
        return 0;
    return foundSurface->value->m_SharedPlatformSurfaceTizen.get();
}

void PlatformSurfacePoolTizen::freePlatformSurfaceByTileID(int tileID)
{
    int platformSurfaceId = 0;
    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        RefPtr<PlatformSurfaceInfo> platformSurfaceInfo = iter->value;
        if (platformSurfaceInfo->m_tileID == tileID) {
            platformSurfaceId = platformSurfaceInfo->m_SharedPlatformSurfaceTizen->id();
            break;
        }
    }
    if (platformSurfaceId)
        freePlatformSurface(platformSurfaceId);
}

void PlatformSurfacePoolTizen::freePlatformSurface(int platformSurfaceId)
{
    RefPtr<PlatformSurfaceInfo> platformSurfaceInfo = m_platformSurfaces.get(platformSurfaceId);
    if (platformSurfaceInfo && platformSurfaceInfo->m_used)
        m_platformSurfacesToFree.append(platformSurfaceInfo);
#if ENABLE(TIZEN_RENDERING_LOG)
    else
        LOG(TiledAC, "WARNING: no matching or freed PlatformSurfaceInfo for %d at [%s]", platformSurfaceId, __PRETTY_FUNCTION__);
#endif
}

void PlatformSurfacePoolTizen::freePlatformSurfacesIfNeeded()
{
    if (m_platformSurfacesToFree.isEmpty())
        return;

    for (size_t index = 0; index < m_platformSurfacesToFree.size(); index++) {
        RefPtr<PlatformSurfaceInfo> platformSurfaceInfo = m_platformSurfacesToFree[index];
        platformSurfaceInfo->m_tileID = 0;
        platformSurfaceInfo->m_used = false;
        m_usedSizeInByte -= platformSurfaceInfo->sizeInByte();
        m_usedSizeInCount--;
        // update statistics
#if ENABLE(TIZEN_RENDERING_LOG)
        LOG(TiledAC, "free platformSurface(%dx%d) XPlatformSurface pool size : %u KB (%u tiles) used : %u KB (%u tiles) at [%s]",
               platformSurfaceInfo->m_SharedPlatformSurfaceTizen->size().width(),platformSurfaceInfo->m_SharedPlatformSurfaceTizen->size().height(),
               m_sizeInByte/KB, m_platformSurfaces.size(), m_usedSizeInByte/KB, m_usedSizeInCount, __PRETTY_FUNCTION__);
#endif
    }

    m_platformSurfacesToFree.clear();

    if (reachLowPoolUtilization(m_usedSizeInByte, m_sizeInByte))
        m_needToShrink = true;
}

void PlatformSurfacePoolTizen::removePlatformSurface(int platformSurfaceId)
{
    /*FIXME : platformSurfaceId must not be zero.
       But sometimes zero id was added to platformSurfaceMap.
       If this problem is fixed, it will be removed.*/
    if (!platformSurfaceId)
        return;

    PlatformSurfaceMap::iterator foundSurface = m_platformSurfaces.find(platformSurfaceId);
    if (foundSurface == m_platformSurfaces.end())
        return;

    if (foundSurface->value->m_used) {
        m_platformSurfacesIdToRemove.append(platformSurfaceId);
        foundSurface->value->m_willBeRemoved = true;
        startRemovePlaformSurfaceTimer();
        return;
    }

    // update statistics
    m_sizeInByte -= foundSurface->value->sizeInByte();
    m_platformSurfaces.remove(foundSurface);

#if ENABLE(TIZEN_RENDERING_LOG)
    int width = foundSurface->value->m_SharedPlatformSurfaceTizen->size().width();
    int height = foundSurface->value->m_SharedPlatformSurfaceTizen->size().height();
    LOG(TiledAC, "remove platformSurface(%dx%d) XPlatformSurface pool size : %u KB (%u tiles) used : %u KB (%u tiles) at [%s]",
       width,  height, m_sizeInByte/KB, m_platformSurfaces.size(), m_usedSizeInByte/KB, m_usedSizeInCount, "PlatformSurfacePoolTizen::removePlatformSurface");
#endif
}

void PlatformSurfacePoolTizen::willRemovePlatformSurface(int platformSurfaceId)
{
    m_client->willRemovePlatformSurface(platformSurfaceId);
}

bool PlatformSurfacePoolTizen::canCreatePlatformSurface()
{
    if (m_sizeInByte >= m_maxSizeInByte || !m_sharedPlatformSurfaceManagement.canCreateSharedPlatformSurface(m_maxSizeInCount))
        shrink();

    return (m_sizeInByte < m_maxSizeInByte && m_sharedPlatformSurfaceManagement.canCreateSharedPlatformSurface(m_maxSizeInCount));
}

RefPtr<PlatformSurfacePoolTizen::PlatformSurfaceInfo> PlatformSurfacePoolTizen::createPlatformSurface(const IntSize& size)
{
    RefPtr<PlatformSurfaceInfo> newPlatformSurface = adoptRef(new PlatformSurfaceInfo(size));
    if (!newPlatformSurface->m_SharedPlatformSurfaceTizen)
        return 0;

    m_platformSurfaces.add(newPlatformSurface->m_SharedPlatformSurfaceTizen->id(), newPlatformSurface);
    m_sizeInByte += newPlatformSurface->sizeInByte();

    return newPlatformSurface.get();
}

void PlatformSurfacePoolTizen::shrink()
{
    Vector<int> platformSurfacesToRemove;
    size_t sizeInByte = m_sizeInByte;
    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        RefPtr<PlatformSurfaceInfo> platformSurfaceInfo = iter->value;

        if (!platformSurfaceInfo->m_used && ++platformSurfaceInfo->m_age > 3) {
            platformSurfacesToRemove.append(platformSurfaceInfo->m_SharedPlatformSurfaceTizen->id());
            sizeInByte -= platformSurfaceInfo->sizeInByte();
        }
        if (reachLowPoolUtilization(m_usedSizeInByte, m_sizeInByte)
            && reachLowPoolUtilization(sizeInByte, m_sizeInByte)) {
            m_needToShrink = false;
            break;
        }

       if (platformSurfacesToRemove.size() > 0)
           break;
    }

    if (platformSurfacesToRemove.size() == 0)
        m_needToShrink = false;
#if ENABLE(TIZEN_RENDERING_LOG)
    else
        LOG(TiledAC, "remove %d platformSurfaces @PlatformSurfacePoolTizen::shrink", platformSurfacesToRemove.size());
#endif

    for (size_t index = 0; index < platformSurfacesToRemove.size(); index++) {
        // FIXME: we should send willRemovePlatformSurface to UIProcess before destroying xplatformSurface itself
        // to make sure UIProcess release corresponding texture first.
        // But currently there's no harm to destroying xplatformSurface first, because UIProcess will not use
        // corresponding texture any longer and will successfully release the texture.
        // Without removing platformSurfaces immediately here, we will encounter starvation of xplatformSurface too often.
        removePlatformSurface(platformSurfacesToRemove[index]);
        willRemovePlatformSurface(platformSurfacesToRemove[index]);
    }

    if (m_needToShrink)
        startShrinkTimer();
}

void PlatformSurfacePoolTizen::startRemovePlaformSurfaceTimer()
{
    if (m_removePlaformSurfaceTimer.isActive())
        return;
    m_removePlaformSurfaceTimer.startOneShot(0.01);
}

void PlatformSurfacePoolTizen::removePlaformSurfaceTimerFired(Timer<PlatformSurfacePoolTizen>*)
{
    freePlatformSurfacesIfNeeded();
    removePlatformSurfacesIfNeeded();
}

void PlatformSurfacePoolTizen::removePlatformSurfacesIfNeeded()
{
    Vector<int> platformSurfacesIdToRemove = m_platformSurfacesIdToRemove;
    m_platformSurfacesIdToRemove.clear();

    for (Vector<int>::iterator iter = platformSurfacesIdToRemove.begin(); iter != platformSurfacesIdToRemove.end(); ++iter)
        removePlatformSurface(*iter);
}

void PlatformSurfacePoolTizen::shrinkIfNeeded()
{
    if (!m_needToShrink)
        return;
    shrink();
}

void PlatformSurfacePoolTizen::startShrinkTimer()
{
    if (m_shrinkTimer.isActive())
        return;
    m_shrinkTimer.startOneShot(0.01);
}

void PlatformSurfacePoolTizen::shrinkTimerFired(Timer<PlatformSurfacePoolTizen>*)
{
    shrinkIfNeeded();
}

}
#endif

