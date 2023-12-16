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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef Canvas2DLayerTizen_h
#define Canvas2DLayerTizen_h

#if USE(ACCELERATED_COMPOSITING) && ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
#include "TextureMapper.h"
#include "TextureMapperPlatformLayer.h"
#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#include <wtf/Uint8ClampedArray.h>
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
#include "Timer.h"
#endif


namespace WebCore {

class CanvasRenderingContext2D;

class Canvas2DLayerTizen : public TextureMapperPlatformLayer
#if !ENABLE(TIZEN_INHERIT_REF_COUNT_TO_TEXTURE_MAPPER_LAYER)
    , public RefCounted<Canvas2DLayerTizen>
#endif
{
public:
    static PassRefPtr<Canvas2DLayerTizen> create(Canvas2DLayerTizen* owner);
    virtual ~Canvas2DLayerTizen();
    virtual bool swapPlatformSurfaces();
    virtual void removePlatformSurface(int);
    virtual void flushPlatformSurfaces() { swapPlatformSurfaces(); }
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float) { }
    virtual void swapBuffers() { }
    virtual void setContext(CanvasRenderingContext2D*);
    virtual int contentType() { return Canvas2DContentType; }
    virtual void flushSurface();

#if USE(GRAPHICS_SURFACE)
    // FIXME : rebase
    //virtual uint32_t copyToGraphicsSurface();
    //virtual uint64_t graphicsSurfaceToken() const;
    //virtual int graphicsSurfaceFlags() const { return GraphicsSurface::Is2D | GraphicsSurface::Alpha | GraphicsSurface::UseLinearFilter; }
    virtual uint32_t copyToGraphicsSurface();
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const;
    virtual GraphicsSurface::Flags graphicsSurfaceFlags() const { return GraphicsSurface::SupportsAlpha | GraphicsSurface::SupportsTextureTarget | GraphicsSurface::SupportsSharing; }
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    virtual void increaseDrawCount() { m_countDidDraw++; }
#endif
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    virtual IntSize platformLayerSize() const;
#endif

private:
    explicit Canvas2DLayerTizen(Canvas2DLayerTizen* owner);

    CanvasRenderingContext2D* m_renderingContext;
    int m_platformSurfaceID;

    enum ContentType { HTMLContentType, DirectImageContentType, ColorContentType, MediaContentType, Canvas3DContentType, Canvas2DContentType };

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    void restorePlatformSurface();
    RefPtr<Uint8ClampedArray> m_platformSurfaceImage;
    GraphicsContextState m_savedState;
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    void canvasDashModeFired(Timer<Canvas2DLayerTizen>*) { };
    int m_countDidDraw;
    Timer<Canvas2DLayerTizen> m_canvasDashModeTimer;
#endif
};

} // namespace WebCore

#endif

#endif // Canvas2DLayerTizen_h
