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

#ifndef VideoLayerTizen_h
#define VideoLayerTizen_h

#if USE(ACCELERATED_COMPOSITING) && USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)

#include "GraphicsLayer.h"
#include "HTMLMediaElement.h"
#include "SharedVideoPlatformSurfaceTizen.h"
#include "TextureMapperGL.h"
#include "TextureMapperPlatformLayer.h"

#include <gst/gst.h>

#if USE(GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

namespace WebCore {

class GraphicsContext;
class IntRect;

class VideoLayerTizen : public TextureMapperPlatformLayer, public VideoPlatformSurfaceUpdateListener
{
public:
    static PassOwnPtr<VideoLayerTizen> create(MediaPlayerClient* client)
    {
        return adoptPtr(new VideoLayerTizen(client));
    }
    ~VideoLayerTizen();

    PlatformLayer* platformLayer() const;

    // TextureMapperPlatformLayer interface
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect& /*targetRect*/, const TransformationMatrix&, float /*opacity*/) { }
    virtual Image* paintToImage() { return 0; }

    // TextureMapperPlatformLayer interface
    virtual bool swapPlatformSurfaces();
    virtual void freePlatformSurface(int);

    // VideoplatformSurfaceUpdateListener interface
    virtual void platformSurfaceUpdated();

    GstElement* createVideoSink();
    void notifySyncRequired();
    void setOverlay(IntSize);
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    void setOverlay();
#endif
#if ENABLE(TIZEN_TV_PROFILE)
    static int returnPixmapIDCallback(void*);
    int getSurfaceID();
	void resetOverlay();
#endif
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);


#if USE(GRAPHICS_SURFACE)
    virtual IntSize platformLayerSize() const;
    virtual uint32_t copyToGraphicsSurface();
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const;
    virtual GraphicsSurface::Flags graphicsSurfaceFlags() const;
#endif

private:
    VideoLayerTizen(MediaPlayerClient*);
    void syncLayer(VideoLayerTizen*);

    GstElement* m_videoSink;
    OwnPtr<SharedVideoPlatformSurfaceTizen> m_platformSurface;
    OwnPtr<SharedVideoPlatformSurfaceTizen> m_platformSurfaceToBeRemoved;

    MediaPlayerClient* m_client;
};

} // WebCore

#endif // USE(ACCELERATED_COMPOSITING) && USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)

#endif // VideoLayerTizen_h
