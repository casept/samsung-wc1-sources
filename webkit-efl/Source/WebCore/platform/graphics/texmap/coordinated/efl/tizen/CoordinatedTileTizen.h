/*
*   Copyright (C) 2012 Samsung Electronics
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

#ifndef CoordinatedTileTizen_h
#define CoordinatedTileTizen_h

#if USE(TIZEN_PLATFORM_SURFACE)
#include "CoordinatedTile.h"
#include "SharedPlatformSurfaceTizen.h"
#include "TiledBackingStoreClient.h"

namespace WebCore {
class ImageBuffer;
class TiledBackingStore;
class CoordinatedTileClient;
class SurfaceUpdateInfo;
class PlatformSurfacePoolTizen;

class CoordinatedTileTizen : public WebCore::CoordinatedTile {
public:
    static PassRefPtr<Tile> create(WebCore::CoordinatedTileClient* client, WebCore::TiledBackingStore* tiledBackingStore, const WebCore::Tile::Coordinate& tileCoordinate) { return adoptRef(new CoordinatedTileTizen(client, tiledBackingStore, tileCoordinate)); }
    ~CoordinatedTileTizen() { }

    Vector<WebCore::IntRect> updateBackBuffer();

    void resize(const WebCore::IntSize&);

private:
    bool drawPlatformSurface(bool& partialUpdate);
    CoordinatedTileTizen(WebCore::CoordinatedTileClient*, WebCore::TiledBackingStore*, const WebCore::Tile::Coordinate&);

    WebCore::IntSize m_platformSurfaceSize;
    int m_platformSurfaceID;
    int m_basePlatformSurfaceID;
#if ENABLE(TIZEN_DS5_ANNOTATOR)
    int m_updateBackBufferDepth;
#endif
};


class CoordinatedTileBackendTizen : public WebCore::TiledBackingStoreBackend {
public:
    static PassOwnPtr<WebCore::TiledBackingStoreBackend> create(WebCore::CoordinatedTileClient* client) { return adoptPtr(new CoordinatedTileBackendTizen(client)); }
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    static bool usePlatformSurface() { return s_usePlatformSurface; }
    static void setUsePlatformSurface(bool usePlatformSurface) { s_usePlatformSurface = usePlatformSurface; }
#endif
    PassRefPtr<WebCore::Tile> createTile(WebCore::TiledBackingStore*, const WebCore::Tile::Coordinate&);
    void paintCheckerPattern(WebCore::GraphicsContext*, const WebCore::FloatRect&) {}
private:
    CoordinatedTileBackendTizen(WebCore::CoordinatedTileClient*);
    WebCore::CoordinatedTileClient* m_client;

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    static bool s_usePlatformSurface;
#endif
};

}
#endif // USE(TIZEN_PLATFORM_SURFACE)

#endif

