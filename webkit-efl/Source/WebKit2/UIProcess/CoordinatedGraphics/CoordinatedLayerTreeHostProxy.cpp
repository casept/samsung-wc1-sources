/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2013 Company 100, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

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

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedLayerTreeHostProxy.h"

#include "CoordinatedLayerTreeHostMessages.h"
#include "CoordinatedLayerTreeHostProxyMessages.h"
#include "DrawingAreaProxy.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/CoordinatedGraphicsState.h>

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
#include "WebFullScreenManagerProxy.h"
#endif

namespace WebKit {

using namespace WebCore;

CoordinatedLayerTreeHostProxy::CoordinatedLayerTreeHostProxy(DrawingAreaProxy* drawingAreaProxy)
    : m_drawingAreaProxy(drawingAreaProxy)
    , m_scene(adoptRef(new CoordinatedGraphicsScene(this)))
{
    m_drawingAreaProxy->page()->process()->addMessageReceiver(Messages::CoordinatedLayerTreeHostProxy::messageReceiverName(), m_drawingAreaProxy->page()->pageID(), this);
}

CoordinatedLayerTreeHostProxy::~CoordinatedLayerTreeHostProxy()
{
    m_drawingAreaProxy->page()->process()->removeMessageReceiver(Messages::CoordinatedLayerTreeHostProxy::messageReceiverName(), m_drawingAreaProxy->page()->pageID());
    m_scene->detach();
}

void CoordinatedLayerTreeHostProxy::updateViewport()
{
    m_drawingAreaProxy->updateViewport();
}

void CoordinatedLayerTreeHostProxy::dispatchUpdate(const Function<void()>& function)
{
    m_scene->appendUpdate(function);
}

void CoordinatedLayerTreeHostProxy::commitCoordinatedGraphicsState(const CoordinatedGraphicsState& graphicsState)
{
    dispatchUpdate(bind(&CoordinatedGraphicsScene::commitSceneState, m_scene.get(), graphicsState));
#if PLATFORM(TIZEN)
    m_scene->syncRemoteContent();
#endif
    updateViewport();
#if USE(TILED_BACKING_STORE)
    m_drawingAreaProxy->page()->didRenderFrame(graphicsState.contentsSize, graphicsState.coveredRect);
#endif
}

void CoordinatedLayerTreeHostProxy::setVisibleContentsRect(const FloatRect& rect, const FloatPoint& trajectoryVector)
{
    // Inform the renderer to adjust viewport-fixed layers.
    dispatchUpdate(bind(&CoordinatedGraphicsScene::setScrollPosition, m_scene.get(), rect.location()));

    if (rect == m_lastSentVisibleRect && trajectoryVector == m_lastSentTrajectoryVector)
        return;

    m_drawingAreaProxy->page()->process()->send(Messages::CoordinatedLayerTreeHost::SetVisibleContentsRect(rect, trajectoryVector), m_drawingAreaProxy->page()->pageID());
    m_lastSentVisibleRect = rect;
    m_lastSentTrajectoryVector = trajectoryVector;
}

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
void CoordinatedLayerTreeHostProxy::didSetVisibleContentsRect()
{
    m_drawingAreaProxy->page()->didSetVisibleContentsRect();
}
#endif

void CoordinatedLayerTreeHostProxy::renderNextFrame()
{
    m_drawingAreaProxy->page()->process()->send(Messages::CoordinatedLayerTreeHost::RenderNextFrame(), m_drawingAreaProxy->page()->pageID());
}

void CoordinatedLayerTreeHostProxy::purgeBackingStores()
{
#if ENABLE(TIZEN_CLEAR_BACKINGSTORE)
    coordinatedGraphicsScene()->clearBackingStores();
#endif
    m_drawingAreaProxy->page()->process()->send(Messages::CoordinatedLayerTreeHost::PurgeBackingStores(), m_drawingAreaProxy->page()->pageID());
}

void CoordinatedLayerTreeHostProxy::setBackgroundColor(const Color& color)
{
    dispatchUpdate(bind(&CoordinatedGraphicsScene::setBackgroundColor, m_scene.get(), color));
}

void CoordinatedLayerTreeHostProxy::commitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    m_drawingAreaProxy->page()->process()->send(Messages::CoordinatedLayerTreeHost::CommitScrollOffset(layerID, offset), m_drawingAreaProxy->page()->pageID());
}

#if PLATFORM(TIZEN)
bool CoordinatedLayerTreeHostProxy::makeContextCurrent()
{
    return m_drawingAreaProxy->page()->makeContextCurrent();
}
#endif

#if USE(TIZEN_PLATFORM_SURFACE)
PlatformSurfaceTexturePool* CoordinatedLayerTreeHostProxy::platformSurfaceTexturePool() const
{
    return &m_drawingAreaProxy->page()->process()->platformSurfaceTexturePool();
}

void CoordinatedLayerTreeHostProxy::freePlatformSurface(int layerId, int freePlatformSurface)
{
    m_drawingAreaProxy->page()->process()->send(Messages::CoordinatedLayerTreeHost::FreePlatformSurface(layerId, freePlatformSurface), m_drawingAreaProxy->page()->pageID());
}

void CoordinatedLayerTreeHostProxy::removePlatformSurface(CoordinatedLayerID layerID, unsigned int platformSurfaceID)
{
    m_drawingAreaProxy->page()->process()->send(Messages::CoordinatedLayerTreeHost::RemovePlatformSurface(layerID, platformSurfaceID), m_drawingAreaProxy->page()->pageID());
}
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
bool CoordinatedLayerTreeHostProxy::useGLBackend()
{
    return m_drawingAreaProxy->page()->useGLBackend();
}
#endif

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
bool CoordinatedLayerTreeHostProxy::fullScreenForVideoMode()
{
    if (m_drawingAreaProxy->page()->fullScreenManager()) {
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        if (m_drawingAreaProxy->page()->fullScreenManager()->isUsingHwVideoOverlay())
#endif
            return true;
    }
    return false;
}
#endif

#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
bool CoordinatedLayerTreeHostProxy::uiSideAnimationEnabled()
{
    return m_drawingAreaProxy->page()->uiSideAnimationEnabled();
}
#endif
}
#endif // USE(COORDINATED_GRAPHICS)
