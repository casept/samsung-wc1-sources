/*
*   Copyright (C) 2011 Samsung Electronics
*
*   This library is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Library General Public
*   License as published by the Free Software Foundation; either
*   version 2 of the License, or (at your option) any later version.
*
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Library General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public License
*   along with this library; see the file COPYING.LIB.  If not, write to
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
*   Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "CoordinatedTileTizen.h"

#if USE(TIZEN_PLATFORM_SURFACE)
#include "CairoUtilities.h"
#include "PlatformContextCairo.h"
#include "PlatformSurfacePoolTizen.h"
#include "SurfaceUpdateInfo.h"
#include <wtf/RefPtr.h>
#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#endif

#if ENABLE(TIZEN_DS5_ANNOTATOR)
#include <WebCore/DS5Annotator.h>
#endif

namespace WebCore {

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
bool CoordinatedTileBackendTizen::s_usePlatformSurface = true;
#endif

CoordinatedTileBackendTizen::CoordinatedTileBackendTizen(CoordinatedTileClient* client)
    : m_client(client)
{
}

PassRefPtr<WebCore::Tile> CoordinatedTileBackendTizen::createTile(WebCore::TiledBackingStore* tiledBackingStore, const WebCore::Tile::Coordinate& tileCoordinate)
{
    return CoordinatedTileTizen::create(m_client, tiledBackingStore, tileCoordinate);
}

CoordinatedTileTizen::CoordinatedTileTizen(CoordinatedTileClient* client, TiledBackingStore* tiledBackingStore, const Coordinate& tileCoordinate)
    : CoordinatedTile(client, tiledBackingStore, tileCoordinate)
    , m_platformSurfaceID(0)
    , m_basePlatformSurfaceID(0)
#if ENABLE(TIZEN_DS5_ANNOTATOR)
    , m_updateBackBufferDepth(0)
#endif
{
}

static inline bool needUpdateBackBufferPartially(const IntRect& entireRect, const IntRect& dirtyRect)
{
    // FIXME: apply partial update only for tiles whoes size is larger than 384*384.
    // because otherwise, i.e. for small tiles, partial update takes even longer time than repainting whole tile.
    // we need to tune the threshold tile size through more testing.
    if (entireRect.width() * entireRect.height() < 147456)
        return false;

    if (entireRect.size() != dirtyRect.size())
        return true;

    return false;
}

Vector<IntRect> CoordinatedTileTizen::updateBackBuffer()
{
#if ENABLE(TIZEN_DS5_ANNOTATOR)
    if (m_updateBackBufferDepth++ == 0)
        ANNOTATE_CHANNEL_COLOR(AnnotateChannelUpdateBackBuffer, ANNOTATE_LTGRAY, __FUNCTION__);
#endif
    if (!isDirty())
        return Vector<IntRect>();

    SurfaceUpdateInfo updateInfo;

    static int id = 0;
    bool needToCreateTile = false;
    if (!m_ID) {
        m_ID = ++id;
        needToCreateTile = true;
    }

    bool partialUpdate = false;
    if (!drawPlatformSurface(partialUpdate))
        return Vector<IntRect>();

    updateInfo.updateRect = m_dirtyRect;
    updateInfo.updateRect.move(-m_rect.x(), -m_rect.y());

    updateInfo.scaleFactor = m_tiledBackingStore->contentsScale();
    updateInfo.platformSurfaceID = m_platformSurfaceID;
    updateInfo.platformSurfaceSize = m_platformSurfaceSize;
    updateInfo.partialUpdate = partialUpdate;

    if (needToCreateTile)
        m_client->createTile(m_ID, updateInfo, m_rect);
    else
        m_client->updateTile(m_ID, updateInfo, m_rect);

    m_dirtyRect = IntRect();

    Vector<IntRect> paintedRects;
    paintedRects.append(m_rect);
#if ENABLE(TIZEN_DS5_ANNOTATOR)
    if (--m_updateBackBufferDepth == 0)
        ANNOTATE_CHANNEL_END(AnnotateChannelUpdateBackBuffer);
#endif
    return paintedRects;
}

void CoordinatedTileTizen::resize(const IntSize& newSize)
{
    m_rect = IntRect(m_rect.location(), newSize);
    m_dirtyRect = m_rect;
}

bool CoordinatedTileTizen::drawPlatformSurface(bool& partialUpdate)
{
    PlatformSurfacePoolTizen* platformSurfacePool = m_client->platformSurfacePool();
    if(!platformSurfacePool)
        return false;

    SharedPlatformSurfaceTizen* sharedPlatformSurface = platformSurfacePool->acquirePlatformSurface(m_tiledBackingStore->tileSize(), m_ID);
    if (!sharedPlatformSurface)
        return false;

    unsigned char* dstBuffer;
    RefPtr<cairo_surface_t> dstSurface;
    RefPtr<cairo_t> dstBitmapContext;
    OwnPtr<WebCore::GraphicsContext> graphicsContext;

    dstBuffer = (unsigned char*)sharedPlatformSurface->lockTbmBuffer();
    if (!dstBuffer) {
        platformSurfacePool->freePlatformSurface(sharedPlatformSurface->id());
        return false;
    }

    dstSurface = adoptRef(cairo_image_surface_create_for_data(dstBuffer, CAIRO_FORMAT_ARGB32,
        sharedPlatformSurface->size().width(), sharedPlatformSurface->size().height(), 4 * sharedPlatformSurface->size().width()));

    if (m_basePlatformSurfaceID > 0 && needUpdateBackBufferPartially(m_rect, m_dirtyRect)) {
        partialUpdate = true;
    } else {
        m_dirtyRect = m_rect;
        partialUpdate = false;
    }

    dstBitmapContext = adoptRef(cairo_create(dstSurface.get()));
    graphicsContext = adoptPtr(new GraphicsContext(dstBitmapContext.get()));
    graphicsContext->clearRect(WebCore::FloatRect(WebCore::FloatPoint(0, 0), sharedPlatformSurface->size()));

#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    double startTime = 0;
    if (m_tiledBackingStore->client()->drawTileInfo()) {
        graphicsContext->save();
        startTime = currentTime();
    }
#endif

    paintToSurfaceContext(graphicsContext.get());

    m_platformSurfaceID = sharedPlatformSurface->id();
    m_platformSurfaceSize = sharedPlatformSurface->size();

    if (!partialUpdate)
        m_basePlatformSurfaceID = sharedPlatformSurface->id();

#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    if (m_tiledBackingStore->client()->drawTileInfo()) {
        graphicsContext->restore();

        String tileInfoString = String::format("[%d] %.1fms (%d,%d/%dX%d)",
            m_platformSurfaceID, (currentTime() - startTime) * 1000, m_rect.x(), m_rect.y(),
            m_platformSurfaceSize.width(), m_platformSurfaceSize.height());

        cairo_set_source_rgb(dstBitmapContext.get(), 0.0, 0.0, 1.0);
        cairo_select_font_face(dstBitmapContext.get(), "Samsung Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

        static int tileInfoFontSize = 15;
        cairo_set_font_size(dstBitmapContext.get(), tileInfoFontSize);
        cairo_move_to(dstBitmapContext.get(), 0, tileInfoFontSize);

        cairo_show_text(dstBitmapContext.get(), tileInfoString.utf8().data());
    }
#endif
    dstBitmapContext.release();
    graphicsContext.release();

    if (!sharedPlatformSurface->unlockTbmBuffer())
        return false;

    return true;
}



} // namespace WebKit

#endif // USE(TIZEN_PLATFORM_SURFACE)
