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

#include "config.h"
#include "VideoLayerTizen.h"

#if USE(ACCELERATED_COMPOSITING) && USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)

#include "GraphicsContext.h"
#include "IntRect.h"
#include "MediaPlayer.h"

#include <gst/interfaces/xoverlay.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>

namespace WebCore {

VideoLayerTizen::VideoLayerTizen(MediaPlayerClient* client)
    : m_videoSink(0)
    , m_client(client)
{
}

VideoLayerTizen::~VideoLayerTizen()
{
    syncLayer(0);

    if (m_platformSurface)
        m_platformSurface.clear();

    if (m_platformSurfaceToBeRemoved)
        m_platformSurfaceToBeRemoved.clear();

    m_videoSink = 0;
}

PlatformLayer* VideoLayerTizen::platformLayer() const
{
    return const_cast<TextureMapperPlatformLayer*>(static_cast<const TextureMapperPlatformLayer*>(this));
}

void VideoLayerTizen::syncLayer(VideoLayerTizen* /*layer*/)
{
    m_client->syncLayer();
    /*
    RenderBox* renderBox = m_media->renderBox();
    if (renderBox && renderBox->hasAcceleratedCompositing()) {
        RenderLayer* renderLayer = renderBox->layer();
        if (renderLayer && renderLayer->isComposited()) {
            GraphicsLayer* graphicsLayer = renderLayer->backing()->graphicsLayer();
            if (graphicsLayer)
                graphicsLayer->setContentsNeedsDisplay();
        }
    }
    */
}

bool VideoLayerTizen::swapPlatformSurfaces()
{
    return true;
}

void VideoLayerTizen::freePlatformSurface(int /*id*/)
{
    if (m_platformSurfaceToBeRemoved)
        m_platformSurfaceToBeRemoved.clear();
}

void VideoLayerTizen::platformSurfaceUpdated()
{
    notifySyncRequired();
}

GstElement* VideoLayerTizen::createVideoSink()
{
    GstElement* videoSink = gst_element_factory_make("xvimagesink", "xvimagesink");
    g_object_set(videoSink, "rotate", 0, NULL);
    g_object_set(videoSink, "is-pixmap", TRUE, NULL);
#if ENABLE(TIZEN_WEARABLE_PROFILE)
    g_object_set(videoSink, "pixel-aspect-ratio", "1/1", NULL);
#endif
    m_videoSink = videoSink;
    return videoSink;
}

void VideoLayerTizen::notifySyncRequired()
{
    if (!m_platformSurface)
        return;

    syncLayer(this);
}

#if ENABLE(TIZEN_TV_PROFILE)
int VideoLayerTizen::getSurfaceID()
{
    return m_platformSurface->id();
}

int VideoLayerTizen::returnPixmapIDCallback(void* data)
{
    VideoLayerTizen* videoLayer = (VideoLayerTizen*) data;
	return videoLayer->getSurfaceID();
}

void VideoLayerTizen::resetOverlay()
{
    g_object_set (m_videoSink, "pixmap-id-callback", NULL, NULL);
}
#endif


void VideoLayerTizen::setOverlay(IntSize size)
{
    if (size.isEmpty())
        return;

    if (!m_videoSink)
        return;

    // Align to 16. (If not aligned to 16, pixmap will be created again.)
    int remainder = size.width() % 16;
    if (remainder) {
        size.setHeight(size.height() + (16 - remainder) * (static_cast<float>(size.height()) / size.width()));
        size.setWidth(size.width() + (16 - remainder));
    }

    if (!m_platformSurface || m_platformSurface->size() != size) {
        if (m_platformSurface)
            m_platformSurfaceToBeRemoved = m_platformSurface.release();

        m_platformSurface = SharedVideoPlatformSurfaceTizen::create(size);
        m_platformSurface->setVideoPlatformSurfaceUpdateListener(this);

        if (m_platformSurfaceToBeRemoved)
            m_platformSurface->copySurface(m_platformSurfaceToBeRemoved.get(), SharedVideoPlatformSurfaceTizen::FitToWidth);
    }

    g_object_set(m_videoSink, "is-pixmap", TRUE, NULL);
#if ENABLE(TIZEN_TV_PROFILE)
    g_object_set (m_videoSink, "pixmap-id-callback", &returnPixmapIDCallback, NULL);
    g_object_set (m_videoSink, "pixmap-id-callback-userdata", this, NULL);
    TIZEN_LOGI("pixmap-id-callback is called with callback for setOverlay(size)");
#else
    gst_x_overlay_set_window_handle(GST_X_OVERLAY(m_videoSink), m_platformSurface->id());
#endif
}

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
void VideoLayerTizen::setOverlay()
{
    if (!m_videoSink || !m_platformSurface || !m_platformSurface->id())
        return;

    g_object_set(m_videoSink, "is-pixmap", TRUE, NULL);
#if ENABLE(TIZEN_TV_PROFILE)
    g_object_set (m_videoSink, "pixmap-id-callback", &returnPixmapIDCallback, NULL);
    g_object_set (m_videoSink, "pixmap-id-callback-userdata", this, NULL);
    TIZEN_LOGI("pixmap-id-callback is called with callback for setOverlay()");
#else

    gst_x_overlay_set_window_handle(GST_X_OVERLAY(m_videoSink), m_platformSurface->id());
#endif
}
#endif

void VideoLayerTizen::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (m_platformSurface)
        m_platformSurface->paintCurrentFrameInContext(context, rect);
}

#if USE(GRAPHICS_SURFACE)
IntSize VideoLayerTizen::platformLayerSize() const
{
    return m_platformSurface ? m_platformSurface->size() : IntSize();
}

GraphicsSurfaceToken VideoLayerTizen::graphicsSurfaceToken() const
{
    return (m_platformSurface ? GraphicsSurfaceToken(m_platformSurface->id()) : 0);
}

uint32_t VideoLayerTizen::copyToGraphicsSurface()
{
    return (m_platformSurface ? m_platformSurface->id() : 0);
}

GraphicsSurface::Flags VideoLayerTizen::graphicsSurfaceFlags() const
{
    return m_platformSurface ? m_platformSurface->graphicsSurfaceFlags() : GraphicsSurface::Is2D;
}
#endif // USE(GRAPHICS_SURFACE)

} // WebCore

#endif // USE(ACCELERATED_COMPOSITING) && USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)
