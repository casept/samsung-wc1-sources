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

#ifndef CoordinatedLayerTreeHostProxy_h
#define CoordinatedLayerTreeHostProxy_h

#if USE(COORDINATED_GRAPHICS)

#include "CoordinatedGraphicsArgumentCoders.h"
#include "MessageReceiver.h"
#include <WebCore/CoordinatedGraphicsScene.h>
#include <wtf/Functional.h>

namespace WebCore {
class CoordinatedGraphicsState;
class IntSize;
}

namespace WebKit {

class DrawingAreaProxy;

class CoordinatedLayerTreeHostProxy : public WebCore::CoordinatedGraphicsSceneClient, public CoreIPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(CoordinatedLayerTreeHostProxy);
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit CoordinatedLayerTreeHostProxy(DrawingAreaProxy*);
    virtual ~CoordinatedLayerTreeHostProxy();

    void commitCoordinatedGraphicsState(const WebCore::CoordinatedGraphicsState&);
    void setBackgroundColor(const WebCore::Color&);

    void setVisibleContentsRect(const WebCore::FloatRect&, const WebCore::FloatPoint& trajectoryVector);
    WebCore::CoordinatedGraphicsScene* coordinatedGraphicsScene() const { return m_scene.get(); }

    virtual void updateViewport() OVERRIDE;
    virtual void renderNextFrame() OVERRIDE;
    virtual void purgeBackingStores() OVERRIDE;

    virtual void commitScrollOffset(uint32_t layerID, const WebCore::IntSize& offset);

#if PLATFORM(TIZEN)
    virtual bool makeContextCurrent();
#endif
#if USE(TIZEN_PLATFORM_SURFACE)
    // CoordinatedGraphicsSceneClient
    virtual WebCore::PlatformSurfaceTexturePool* platformSurfaceTexturePool() const;
    virtual void freePlatformSurface(int layerId, int freePlatformSurface);
    virtual void removePlatformSurface(WebCore::CoordinatedLayerID layerID, unsigned int platformSurfaceID);
#endif
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    bool useGLBackend();
#endif
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    bool fullScreenForVideoMode();
#endif
#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
    void didSetVisibleContentsRect();
#endif
#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
    bool uiSideAnimationEnabled();
#endif
protected:
    void dispatchUpdate(const Function<void()>&);

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    DrawingAreaProxy* m_drawingAreaProxy;
    RefPtr<WebCore::CoordinatedGraphicsScene> m_scene;
    WebCore::FloatRect m_lastSentVisibleRect;
    WebCore::FloatPoint m_lastSentTrajectoryVector;
};

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)

#endif // CoordinatedLayerTreeHostProxy_h
