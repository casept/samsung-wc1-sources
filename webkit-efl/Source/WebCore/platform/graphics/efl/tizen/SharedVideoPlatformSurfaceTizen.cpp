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

#if USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)

#include "SharedVideoPlatformSurfaceTizen.h"
#include "GraphicsContextPlatformPrivateCairo.h"
#include "IntRect.h"
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

namespace WebCore {

class VideoPlatformSurface {
public:
    static PassOwnPtr<VideoPlatformSurface> create(IntSize size);
    ~VideoPlatformSurface();

    int id() const { return m_platformSurface; }
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);
    void copySurface(VideoPlatformSurface* other, SharedVideoPlatformSurfaceTizen::CopySurfaceType type);
    bool hasAlpha() { return m_hasAlpha; }
    IntSize size() const { return m_size; }

    Display* display() { return m_nativeDisplay; }

private:
    VideoPlatformSurface();

    bool initialize(IntSize size);
    bool createPlatformSurface(IntSize size);
    bool destroyPlatformSurface();
    void clearPlatformSurface();

    int m_platformSurface;
    int m_nativeWindow;
    Display* m_nativeDisplay;

    IntSize m_size;
    bool m_hasAlpha;
};

PassOwnPtr<VideoPlatformSurface> VideoPlatformSurface::create(IntSize size)
{
    OwnPtr<VideoPlatformSurface> videoPlatformSurface = adoptPtr(new VideoPlatformSurface());
    if (!videoPlatformSurface->initialize(size))
        return nullptr;
    return videoPlatformSurface.release();
}

VideoPlatformSurface::VideoPlatformSurface()
    : m_platformSurface(0)
    , m_nativeWindow(0)
    , m_nativeDisplay(0)
    , m_size(IntSize())
    , m_hasAlpha(true)
{
}

VideoPlatformSurface::~VideoPlatformSurface()
{
    destroyPlatformSurface();
}

void VideoPlatformSurface::paintCurrentFrameInContext(GraphicsContext* context, const IntRect&)
{
    if (context->paintingDisabled())
        return;

    if (m_size.isEmpty())
        return;

    cairo_surface_t* surface = cairo_xlib_surface_create(m_nativeDisplay, m_platformSurface, DefaultVisual(m_nativeDisplay, DefaultScreen(m_nativeDisplay)), m_size.width(), m_size.height());
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        return;

    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);

    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_restore(cr);
    cairo_surface_destroy(surface);
}

void VideoPlatformSurface::copySurface(VideoPlatformSurface* other, SharedVideoPlatformSurfaceTizen::CopySurfaceType type)
{
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_xlib_surface_create(m_nativeDisplay, m_platformSurface, DefaultVisual(m_nativeDisplay, DefaultScreen(m_nativeDisplay)), m_size.width(), m_size.height()));
    RefPtr<cairo_t> context = adoptRef(cairo_create(surface.get()));
    OwnPtr<WebCore::GraphicsContext> graphicsContext = adoptPtr(new GraphicsContext(context.get()));

    float xScale = static_cast<float>(m_size.width()) / other->m_size.width();
    float yScale = static_cast<float>(m_size.height()) / other->m_size.height();
    float xPosition = 0;
    float yPosition = 0;

    if (xScale != yScale) {
        switch (type) {
        case SharedVideoPlatformSurfaceTizen::FitToWidth:
            yPosition = (m_size.height() - other->m_size.height() * xScale) / 2;
            yScale = xScale;
            break;
        case SharedVideoPlatformSurfaceTizen::FitToHeight:
            xPosition = (m_size.width() - other->m_size.width() * yScale) / 2;
            xScale = yScale;
            break;
        default:
            break;
        }
    }
    graphicsContext->translate(xPosition, yPosition);
    graphicsContext->scale(FloatSize(xScale, yScale));
    other->paintCurrentFrameInContext(graphicsContext.get(), IntRect());
}

bool VideoPlatformSurface::initialize(IntSize size)
{
    if (!createPlatformSurface(size))
        return false;

    m_size = size;

    clearPlatformSurface();

    return true;
}

bool VideoPlatformSurface::createPlatformSurface(IntSize size)
{
    if (size.isEmpty())
        return false;

    if (!m_nativeDisplay) {
        ecore_x_init(0);
        m_nativeDisplay = (Display*)ecore_x_display_get();
    }

    if (!m_nativeWindow)
        m_nativeWindow = XCreateSimpleWindow(m_nativeDisplay, DefaultRootWindow(m_nativeDisplay), 0, 0, 1, 1, 0, BlackPixel(m_nativeDisplay, 0), WhitePixel(m_nativeDisplay, 0));

    XSync(m_nativeDisplay, false);

    int visualDepth = XDefaultDepth(m_nativeDisplay, DefaultScreen(m_nativeDisplay));
    m_platformSurface = XCreatePixmap(m_nativeDisplay, m_nativeWindow, size.width(), size.height(), visualDepth);

    XFlush(m_nativeDisplay);
    XSync(m_nativeDisplay, false);

    m_hasAlpha = visualDepth == 32 ?  true : false;

    return true;
}

bool VideoPlatformSurface::destroyPlatformSurface()
{
    if (m_platformSurface) {
        XFreePixmap(m_nativeDisplay, m_platformSurface);
        m_platformSurface = 0;
    }

    if (m_nativeWindow) {
        XDestroyWindow(m_nativeDisplay, m_nativeWindow);
        m_nativeWindow = 0;
    }

    m_nativeDisplay = 0;

    return true;
}

void VideoPlatformSurface::clearPlatformSurface()
{
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_xlib_surface_create(m_nativeDisplay, m_platformSurface, DefaultVisual(m_nativeDisplay, DefaultScreen(m_nativeDisplay)), m_size.width(), m_size.height()));
    if (cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS)
        return;

    RefPtr<cairo_t> cr = adoptRef(cairo_create(surface.get()));
    cairo_set_source_surface(cr.get(), surface.get(), 0, 0);
    cairo_set_source_rgb(cr.get(), 0, 0, 0);
    cairo_rectangle(cr.get(), 0, 0, m_size.width(), m_size.height());
    cairo_fill(cr.get());
}

PassOwnPtr<SharedVideoPlatformSurfaceTizen> SharedVideoPlatformSurfaceTizen::create(IntSize size)
{
    return adoptPtr(new SharedVideoPlatformSurfaceTizen(size));
}

SharedVideoPlatformSurfaceTizen::SharedVideoPlatformSurfaceTizen(IntSize size)
    : m_damage(0)
    , m_damageHandler(0)
    , m_listener(0)
    , m_platformSurface(VideoPlatformSurface::create(size))
{
    if (!m_platformSurface)
        return;

    registerDamageHandler();
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    m_flags = GraphicsSurface::Is2D | GraphicsSurface::UseLinearFilter | GraphicsSurface::IsVideo;
#else
    m_flags = GraphicsSurface::Is2D | GraphicsSurface::UseLinearFilter;
#endif
    if (m_platformSurface->hasAlpha())
        m_flags |= GraphicsSurface::Alpha;

}

SharedVideoPlatformSurfaceTizen::~SharedVideoPlatformSurfaceTizen()
{
    m_listener = 0;

    unregisterDamageHandler();
}

static Eina_Bool notifyDamageUpdated(void* data, int, void* param)
{
    if (!data || !param)
        return ECORE_CALLBACK_PASS_ON;

    struct _Ecore_X_Event_Damage* damage = (_Ecore_X_Event_Damage*)param;
    SharedVideoPlatformSurfaceTizen* sharedVideoPlatformSurface = reinterpret_cast<SharedVideoPlatformSurfaceTizen*>(data);
    if (sharedVideoPlatformSurface->id() == damage->drawable)
        sharedVideoPlatformSurface->platformSurfaceUpdated();

    return ECORE_CALLBACK_PASS_ON;
}

int SharedVideoPlatformSurfaceTizen::id() const
{
    return m_platformSurface ? m_platformSurface->id() : 0;
}

IntSize SharedVideoPlatformSurfaceTizen::size()
{
    return m_platformSurface ? m_platformSurface->size() : IntSize();
}

void SharedVideoPlatformSurfaceTizen::platformSurfaceUpdated()
{
    if (m_listener)
        m_listener->platformSurfaceUpdated();
}

void SharedVideoPlatformSurfaceTizen::setVideoPlatformSurfaceUpdateListener(VideoPlatformSurfaceUpdateListener* listener)
{
    m_listener = listener;
}

void SharedVideoPlatformSurfaceTizen::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (m_platformSurface)
        m_platformSurface->paintCurrentFrameInContext(context, rect);
}

void SharedVideoPlatformSurfaceTizen::copySurface(SharedVideoPlatformSurfaceTizen* other, CopySurfaceType type)
{
    if (m_platformSurface)
        m_platformSurface->copySurface(other->m_platformSurface.get(), type);
}

void SharedVideoPlatformSurfaceTizen::registerDamageHandler()
{
    if (!m_platformSurface)
        return;

    m_damage = ecore_x_damage_new(m_platformSurface->id(), ECORE_X_DAMAGE_REPORT_RAW_RECTANGLES);
    m_damageHandler = ecore_event_handler_add(ECORE_X_EVENT_DAMAGE_NOTIFY, notifyDamageUpdated, this);
    XSync(m_platformSurface->display(), false);
}

void SharedVideoPlatformSurfaceTizen::unregisterDamageHandler()
{
    if (m_damage) {
        ecore_x_damage_free(m_damage);
        m_damage = 0;
    }

    if (m_damageHandler) {
        ecore_event_handler_del(m_damageHandler);
        m_damageHandler = 0;
    }
}

int SharedVideoPlatformSurfaceTizen::graphicsSurfaceFlags() const
{
    return m_flags;
}

} // WebCore

#endif // USE(TIZEN_PLATFORM_SURFACE) && ENABLE(VIDEO)
