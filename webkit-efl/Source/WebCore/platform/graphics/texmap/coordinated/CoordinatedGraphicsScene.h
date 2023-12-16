/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef CoordinatedGraphicsScene_h
#define CoordinatedGraphicsScene_h

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedGraphicsState.h"
#include "CoordinatedSurface.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerAnimation.h"
#include "GraphicsSurface.h"
#include "IntRect.h"
#include "IntSize.h"
#include "RunLoop.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperFPSCounter.h"
#include "TextureMapperLayer.h"
#include "Timer.h"
#include <wtf/Functional.h>
#include <wtf/HashSet.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/Vector.h>
#include <OpenGLShims.h>
#if USE(GRAPHICS_SURFACE)
#include "TextureMapperSurfaceBackingStore.h"
#endif

namespace WebCore {

typedef struct Blur
{
    GLuint program;
    GLuint vertPosition;
    GLuint texCoord;
    GLfloat bias;
    float mvpRatio[16];

    Blur()
    {
        program = 0;
        vertPosition = 0;
        texCoord = 0;
        bias = 0.0;
        for(int i = 0; i< 16;i++)
        {
          mvpRatio[i] = 0.0;
        }
    }
} BlurEffect;

class CoordinatedBackingStore;
class CustomFilterProgram;
class CustomFilterProgramInfo;
#if USE(TIZEN_PLATFORM_SURFACE)
class PlatformSurfaceTexturePool;
#endif

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
class ScrollbarFadeAnimationController;
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
class EdgeEffectController;
#endif

class CoordinatedGraphicsSceneClient {
public:
    virtual ~CoordinatedGraphicsSceneClient() { }
    virtual void purgeBackingStores() = 0;
    virtual void renderNextFrame() = 0;
    virtual void updateViewport() = 0;
    virtual void commitScrollOffset(uint32_t layerID, const IntSize& offset) = 0;
#if PLATFORM(TIZEN)
    virtual bool makeContextCurrent() = 0;
#endif
#if USE(TIZEN_PLATFORM_SURFACE)
    virtual PlatformSurfaceTexturePool* platformSurfaceTexturePool() const = 0;
    virtual void freePlatformSurface(int layerId, int freePlatformSurface) = 0;
    virtual void removePlatformSurface(CoordinatedLayerID layerID, unsigned int platformSurfaceID) = 0;
#endif
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual bool useGLBackend() = 0;
#endif
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    virtual bool fullScreenForVideoMode() = 0;
#endif
#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
    virtual bool uiSideAnimationEnabled() = 0;
#endif
};

class CoordinatedGraphicsScene : public ThreadSafeRefCounted<CoordinatedGraphicsScene>, public TextureMapperLayer::ScrollingClient {
public:
    explicit CoordinatedGraphicsScene(CoordinatedGraphicsSceneClient*);
    virtual ~CoordinatedGraphicsScene();
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    void paintToCurrentGLContext(const TransformationMatrix&, float, const FloatRect&, TextureMapper::PaintFlags = 0, bool needEffect = 0);
    void paintToBackupTextureEffect();
    void initializeForEffect();
    void clearBlurEffectResources();
    void initializeFBO(int , int );
    int initBlurShaders(BlurEffect*);
    void initShaderForEffect(BlurEffect* , int , int );
    void drawBlur(BlurEffect* , int , int );
    void drawToSurface(int , int );
    void initializeShaders();
#else
    void paintToCurrentGLContext(const TransformationMatrix&, float, const FloatRect&, TextureMapper::PaintFlags = 0);
#endif
    void paintToGraphicsContext(PlatformGraphicsContext*);
    void setScrollPosition(const FloatPoint&);
    void detach();
    void appendUpdate(const Function<void()>&);

    WebCore::TextureMapperLayer* findScrollableContentsLayerAt(const WebCore::FloatPoint&);

    virtual void commitScrollOffset(uint32_t layerID, const IntSize& offset);

    // The painting thread must lock the main thread to use below two methods, because two methods access members that the main thread manages. See m_client.
    // Currently, QQuickWebPage::updatePaintNode() locks the main thread before calling both methods.
    void purgeGLResources();
    void setActive(bool);

    void commitSceneState(const CoordinatedGraphicsState&);

    void setBackgroundColor(const Color&);
    void setDrawsBackground(bool enable) { m_setDrawsBackground = enable; }
    void setViewBackgroundColor(const Color& color) { m_viewBackgroundColor = color; }
    Color viewBackgroundColor() const { return m_viewBackgroundColor; }

#if ENABLE(TIZEN_CLEAR_BACKINGSTORE)
    virtual void clearBackingStores();
#endif

#if USE(TIZEN_PLATFORM_SURFACE)
    virtual PassRefPtr<CoordinatedBackingStore> getBackingStore(CoordinatedLayerID);

    virtual void freePlatformSurface();
    virtual bool hasPlatformSurfaceToFree();

    virtual void updatePlatformSurfaceTile(CoordinatedLayerID, int tileID, const WebCore::IntRect& sourceRect, const WebCore::IntRect& targetRect, int platformSurfaceID, const WebCore::IntSize& platformSurfaceSize, bool partialUpdate);
#endif

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    void startScrollableAreaTouch(const IntPoint&);
    void scrollableAreaScrollFinished();
#if ENABLE(TIZEN_EDGE_EFFECT)
    bool existsVerticalScrollbarLayerInTouchedScrollableLayer();
    bool existsHorizontalScrollbarLayerInTouchedScrollableLayer();
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)
    IntSize scrollScrollableArea(const IntSize&);
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    void showOverflowScollbars();
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    void showEdgeEffect(TextureMapperLayer::EdgeEffectType, bool overflow = false);
    void hideVerticalEdgeEffect(bool withAnimation = false);
    void hideHorizontalEdgeEffect(bool withAnimation = false);
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)
#if PLATFORM(TIZEN)
    void syncRemoteContent();
#endif
#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
    void setAccelerationMode(bool openGL) { m_isAcceleration = openGL; }
#endif

#if ENABLE(TIZEN_SCROLL_SNAP)
    WebCore::IntSize findScrollSnapOffset(const WebCore::IntSize&);
#endif

#if ENABLE(TIZEN_TEXTURE_MAPPER_TEXTURE_POOL_MAKE_CONTEXT_CURRENT)
    bool makeContextCurrent()
    {
        if (m_client)
            m_client->makeContextCurrent();
    }
#endif

private:
    void setRootLayerID(CoordinatedLayerID);
    void createLayers(const Vector<CoordinatedLayerID>&);
    void deleteLayers(const Vector<CoordinatedLayerID>&);
    void setLayerState(CoordinatedLayerID, const CoordinatedGraphicsLayerState&);
    void setLayerChildrenIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void updateTilesIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void createTilesIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void removeTilesIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#if ENABLE(CSS_FILTERS)
    void setLayerFiltersIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#endif
    void setLayerAnimationsIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#if USE(GRAPHICS_SURFACE)
    void createCanvasIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void syncCanvasIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
    void destroyCanvasIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);
#endif
    void setLayerRepaintCountIfNeeded(TextureMapperLayer*, const CoordinatedGraphicsLayerState&);

    void syncUpdateAtlases(const CoordinatedGraphicsState&);
    void createUpdateAtlas(uint32_t atlasID, PassRefPtr<CoordinatedSurface>);
    void removeUpdateAtlas(uint32_t atlasID);

    void syncImageBackings(const CoordinatedGraphicsState&);
    void createImageBacking(CoordinatedImageBackingID);
    void updateImageBacking(CoordinatedImageBackingID, PassRefPtr<CoordinatedSurface>);
    void clearImageBackingContents(CoordinatedImageBackingID);
    void removeImageBacking(CoordinatedImageBackingID);

#if ENABLE(CSS_SHADERS)
    void syncCustomFilterPrograms(const CoordinatedGraphicsState&);
    void injectCachedCustomFilterPrograms(const FilterOperations& filters) const;
    void createCustomFilterProgram(int id, const CustomFilterProgramInfo&);
    void removeCustomFilterProgram(int id);
#endif

    TextureMapperLayer* layerByID(CoordinatedLayerID id)
    {
        ASSERT(m_layers.contains(id));
        ASSERT(id != InvalidCoordinatedLayerID);
        return m_layers.get(id);
    }
    TextureMapperLayer* getLayerByIDIfExists(CoordinatedLayerID);
    TextureMapperLayer* rootLayer() { return m_rootLayer.get(); }

#if !PLATFORM(TIZEN)
    void syncRemoteContent();
#endif
    void adjustPositionForFixedLayers();

    void dispatchOnMainThread(const Function<void()>&);
    void updateViewport();
    void renderNextFrame();
    void purgeBackingStores();

    void createLayer(CoordinatedLayerID);
    void deleteLayer(CoordinatedLayerID);

    void assignImageBackingToLayer(TextureMapperLayer*, CoordinatedImageBackingID);
    void removeReleasedImageBackingsIfNeeded();
    void ensureRootLayer();
    void commitPendingBackingStoreOperations();

    void prepareContentBackingStore(TextureMapperLayer*);
    void createBackingStoreIfNeeded(TextureMapperLayer*);
    void removeBackingStoreIfNeeded(TextureMapperLayer*);
    void resetBackingStoreSizeToLayerSize(TextureMapperLayer*);

    void dispatchCommitScrollOffset(uint32_t layerID, const IntSize& offset);

#if ENABLE(TIZEN_DIRECT_RENDERING)
    void updateViewportFired();
#endif

    // Render queue can be accessed ony from main thread or updatePaintNode call stack!
    Vector<Function<void()> > m_renderQueue;
    Mutex m_renderQueueMutex;

    OwnPtr<TextureMapper> m_textureMapper;

    typedef HashMap<CoordinatedImageBackingID, RefPtr<CoordinatedBackingStore> > ImageBackingMap;
    ImageBackingMap m_imageBackings;
    Vector<RefPtr<CoordinatedBackingStore> > m_releasedImageBackings;

    typedef HashMap<TextureMapperLayer*, RefPtr<CoordinatedBackingStore> > BackingStoreMap;
    BackingStoreMap m_backingStores;

    HashSet<RefPtr<CoordinatedBackingStore> > m_backingStoresWithPendingBuffers;

#if USE(GRAPHICS_SURFACE)
    typedef HashMap<TextureMapperLayer*, RefPtr<TextureMapperSurfaceBackingStore> > SurfaceBackingStoreMap;
    SurfaceBackingStoreMap m_surfaceBackingStores;
#endif

    typedef HashMap<uint32_t /* atlasID */, RefPtr<CoordinatedSurface> > SurfaceMap;
    SurfaceMap m_surfaces;

    // Below two members are accessed by only the main thread. The painting thread must lock the main thread to access both members.
    CoordinatedGraphicsSceneClient* m_client;
    bool m_isActive;

    OwnPtr<TextureMapperLayer> m_rootLayer;

    typedef HashMap<CoordinatedLayerID, OwnPtr<TextureMapperLayer> > LayerMap;
    LayerMap m_layers;
    typedef HashMap<CoordinatedLayerID, TextureMapperLayer*> LayerRawPtrMap;
    LayerRawPtrMap m_fixedLayers;
    CoordinatedLayerID m_rootLayerID;
    FloatPoint m_scrollPosition;
    FloatPoint m_renderedContentsScrollPosition;
    Color m_backgroundColor;
    Color m_viewBackgroundColor;
    bool m_setDrawsBackground;

#if ENABLE(CSS_SHADERS)
    typedef HashMap<int, RefPtr<CustomFilterProgram> > CustomFilterProgramMap;
    CustomFilterProgramMap m_customFilterPrograms;
#endif

    TextureMapperFPSCounter m_fpsCounter;

#if USE(TIZEN_PLATFORM_SURFACE)
    void clearPlatformLayerPlatformSurfaces();
    void freePlatformSurfacesAfterClearingBackingStores();

    /*
     * FIXME : rebase
    virtual void renderNextFrame();
    virtual void purgeBackingStores();
    */

    struct FreePlatformSurfaceData {
        unsigned int platformSurfaceId;
        CoordinatedLayerID layerID;
    };
    Vector<FreePlatformSurfaceData> m_freePlatformSurfaces;

    typedef HashMap<unsigned int, CoordinatedLayerID > PlatformLayerPlatformSurfaceMap;
    PlatformLayerPlatformSurfaceMap m_platformLayerPlatformSurfaces;
#endif

#if ENABLE(TIZEN_DIRECT_RENDERING)
    WebCore::RunLoop::Timer<CoordinatedGraphicsScene> m_updateViewportTimer;
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    CoordinatedLayerID m_touchedScrollableLayerID;
#endif

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    friend class ScrollbarFadeAnimationController;
    OwnPtr<ScrollbarFadeAnimationController> m_scrollbarFadeAnimationController;
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    friend class EdgeEffectController;
    OwnPtr<EdgeEffectController> m_edgeEffectController;
#endif
#if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
    IntSize m_overflowScrollOffset;
#endif
#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
    bool m_isAcceleration;
#endif
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    int m_initShaderAndFBO;
    int m_initShaderForBlurEffect;
    GLuint m_fboTexture;
    GLuint m_fboDepth;
    GLuint m_fbo;
    GLuint m_program;
    BlurEffect m_blur;
#endif
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // CoordinatedGraphicsScene_h


