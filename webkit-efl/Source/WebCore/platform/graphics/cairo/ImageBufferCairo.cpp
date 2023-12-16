/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Holger Hans Peter Freyther <zecke@selfish.org>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ImageBuffer.h"

#include "BitmapImage.h"
#include "CairoUtilities.h"
#include "Color.h"
#include "GraphicsContext.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Pattern.h"
#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/Vector.h>
#include <wtf/text/Base64.h>
#include <wtf/text/WTFString.h>

#if ENABLE(ACCELERATED_2D_CANVAS)
#include "GLContext.h"
#include "OpenGLShims.h"
#include "TextureMapperGL.h"
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#include "PixmapContextTizen.h"
#include "SharedPlatformSurfaceTizen.h"
#include <stdio.h>
#include <Ecore_X.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#if ENABLE(ACCELERATED_2D_CANVAS) || ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#define CAIRO_HAS_GL_SURFACE 0
#define CAIRO_HAS_GLX_FUNCTIONS 0
#define CAIRO_HAS_WGL_FUNCTIONS 0
#include <cairo-gl.h>
#endif

using namespace std;

namespace WebCore {

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
extern void* g_nativeDisplay;
extern int g_nativeWindow;
void* g_egl_display = 0;
cairo_device_t* g_sharedCairoDevice = 0;
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = 0;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = 0;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = 0;
#endif

ImageBufferData::ImageBufferData(const IntSize& size)
    : m_platformContext(0)
    , m_size(size)
#if ENABLE(ACCELERATED_2D_CANVAS)
    , m_texture(0)
#endif
{
}

#if ENABLE(ACCELERATED_2D_CANVAS)
PassRefPtr<cairo_surface_t> createCairoGLSurface(const IntSize& size, uint32_t& texture)
{
    GLContext::sharingContext()->makeContextCurrent();

    // We must generate the texture ourselves, because there is no Cairo API for extracting it
    // from a pre-existing surface.
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA8, size.width(), size.height(), 0 /* border */, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    GLContext* context = GLContext::sharingContext();
    cairo_device_t* device = context->cairoDevice();

    // Thread-awareness is a huge performance hit on non-Intel drivers.
    cairo_gl_device_set_thread_aware(device, FALSE);

    return adoptRef(cairo_gl_surface_create_for_texture(device, CAIRO_CONTENT_COLOR_ALPHA, texture, size.width(), size.height()));
}
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
ImageBuffer::ImageBuffer(const IntSize& size, float /* resolutionScale */, ColorSpace, RenderingMode renderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
    , m_platformSurfaceID(0)
    , m_eglSurface(0)
    , m_bindingTexID(0)
    , m_eglImage(0)
{
    success = false;  // Make early return mean error.
    m_isLockable = (renderingMode == AcceleratedMemorySaving) ? true : false;

    bool hasDepth = true;
    bool hasStencil = true;
    bool hasSamples = true;

    if (renderingMode == Accelerated || renderingMode == AcceleratedMemorySaving) {
        m_data.m_platformSurface = SharedPlatformSurfaceTizen::create(size, false, true, hasDepth, hasStencil, hasSamples);
        m_platformSurfaceID = m_data.m_platformSurface->id();
        m_eglSurface = m_data.m_platformSurface->eglSurface();

        if (!g_sharedCairoDevice) {
            g_egl_display = m_data.m_platformSurface->display();
            g_sharedCairoDevice = cairo_egl_device_create(g_egl_display, m_data.m_platformSurface->context());
            if (cairo_device_status(g_sharedCairoDevice) != CAIRO_STATUS_SUCCESS) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGE("cairo_egl_device_create failed!\n");
#endif
                return;
            }
            cairo_gl_device_set_thread_aware(g_sharedCairoDevice, 0);
        }
    }

    if (m_isLockable || renderingMode == Unaccelerated)
        m_data.m_surface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height()));
    else {
#if !ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
        m_data.m_surface = adoptRef(cairo_gl_surface_create_for_egl(static_cast<cairo_device_t*>(g_sharedCairoDevice), m_eglSurface, size.width(), size.height()));
#endif
        if (!setBindingTexture()) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("setBindingTexture() failed!\n");
#endif
            return;
        }
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
        m_data.m_surface = adoptRef(cairo_surface_create_similar(m_data.m_pixmapSurface.get(), CAIRO_CONTENT_COLOR_ALPHA, size.width(), size.height()));
#endif

        if (m_data.m_platformSurface->lockTbmBuffer())
            m_data.m_platformSurface->unlockTbmBuffer();
        else {
            m_data.m_platformSurface->unlockTbmBuffer();
            return;
        }
    }

    if (cairo_surface_status(m_data.m_surface.get()) != CAIRO_STATUS_SUCCESS) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cairo_gl_surface_create_for_egl() failed!\n");
#endif
        return;  // create will notice we didn't set m_initialized and fail.
    }

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface.get()));
    m_data.m_platformContext.setCr(cr.get());
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    if (renderingMode == Accelerated) {
        RefPtr<cairo_t> pixmapCr = adoptRef(cairo_create(m_data.m_pixmapSurface.get()));
        m_data.m_platformContext.setPixmapCr(pixmapCr.get());
    }
#endif
    m_context = adoptPtr(new GraphicsContext(&m_data.m_platformContext));
    if (renderingMode == Accelerated || renderingMode == AcceleratedMemorySaving){
        cairo_set_tolerance(cr.get(), 0.5);
        context()->clearRect(FloatRect(0, 0, size.width(), size.height()));
    }
    success = true;
}
#else
ImageBuffer::ImageBuffer(const IntSize& size, float /* resolutionScale */, ColorSpace, RenderingMode renderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
{
    success = false;  // Make early return mean error.

#if ENABLE(ACCELERATED_2D_CANVAS)
    if (renderingMode == Accelerated)
        m_data.m_surface = createCairoGLSurface(size, m_data.m_texture);
    else
#endif
        m_data.m_surface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height()));

    if (cairo_surface_status(m_data.m_surface.get()) != CAIRO_STATUS_SUCCESS)
        return;  // create will notice we didn't set m_initialized and fail.

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface.get()));
    m_data.m_platformContext.setCr(cr.get());
    m_context = adoptPtr(new GraphicsContext(&m_data.m_platformContext));
    success = true;
}
#endif // ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)

ImageBuffer::~ImageBuffer()
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    if (m_platformSurfaceID > 0) {
        if (!m_isLockable) {
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
            if (context()) {
                cairo_t* cr = context()->platformContext()->pixmapCr();
                cairo_save(cr);
                cairo_set_source_surface(cr, m_data.m_surface.get(), 0, 0);
                cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
                cairo_paint(cr);
                cairo_surface_flush(m_data.m_pixmapSurface.get());
                glFlush();
                cairo_restore(cr);
            }
#else
            cairo_surface_flush(m_data.m_surface.get());
            glFlush();
#endif
        }
        if (m_bindingTexID)
            glDeleteTextures(1, &m_bindingTexID);
        if (m_eglImage) {
            if (!eglDestroyImageKHR)
                eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
            if (eglDestroyImageKHR(g_egl_display, m_eglImage) != EGL_TRUE) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGE("eglDestroyImageKHR failed!\n");
#endif
            }
        }
    }
#endif
}

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
bool ImageBuffer::setBindingTexture()
{
    // FIXME: workaround codes to support canvas-to-canvas copy by using cairo_gl_surface_set_binding_texture API
    // FIXME: this codes have to be removed soon!
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    m_data.m_platformSurface->makeContextCurrent();
#else
    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface.get()));
    cairo_set_source_rgba(cr.get(), 1.0, 1.0, 1.0, 1.0);
    cairo_paint(cr.get());
#endif

    glGenTextures(1, &m_bindingTexID);

    if (!eglCreateImageKHR) {
        eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        if (!eglCreateImageKHR) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot get eglCreateImageKHR\n");
#endif
            return false;
        }
    }
    if (!glEGLImageTargetTexture2DOES) {
        glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        if (!glEGLImageTargetTexture2DOES) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot get glEGLImageTargetTexture2DOES\n");
#endif
            return false;
        }
    }

    EGLint attr_pixmap[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    m_eglImage = eglCreateImageKHR(g_egl_display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, reinterpret_cast<EGLClientBuffer>(m_platformSurfaceID), attr_pixmap);
    if (!m_eglImage) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cannot create egl image\n");
#endif
        return false;
    }

    GLint boundTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);

    glBindTexture(GL_TEXTURE_2D, m_bindingTexID);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, static_cast<GLeglImageOES>(m_eglImage));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    m_data.m_pixmapSurface = adoptRef(cairo_gl_surface_create_for_texture(static_cast<cairo_device_t*>(g_sharedCairoDevice), CAIRO_CONTENT_COLOR_ALPHA, m_bindingTexID, m_size.width(), m_size.height()));
#else
    cairo_status_t status = cairo_gl_surface_set_binding_texture(m_data.m_surface.get(), m_bindingTexID);

    if (status != CAIRO_STATUS_SUCCESS) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cairo_gl_surface_set_binding_texture() failed!\n");
#endif
        return false;
    }
#endif

    glBindTexture(GL_TEXTURE_2D, boundTex);
    return true;
}

void ImageBuffer::swapPlatformSurfaces()
{
    RefPtr<cairo_surface_t> destinationSurface = querySurface();
    if (!destinationSurface)
        return;

    // XXX: This is likely inefficient and should be improved.
    // Optimally, we would write directly in GPU memory and
    // swap buffers.
    RefPtr<cairo_t> cr = adoptRef(cairo_create(destinationSurface.get()));
    cairo_set_source_surface(cr.get(), m_data.m_surface.get(), 0, 0);
    cairo_set_operator(cr.get(), CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr.get());

    m_data.m_platformSurface->unlockTbmBuffer();
}

PassRefPtr<cairo_surface_t> ImageBuffer::querySurface() const
{
    unsigned char* bitmapPtr = static_cast<unsigned char*>(m_data.m_platformSurface->lockTbmBuffer());
    int pitch = m_data.m_platformSurface->getStride();

    return adoptRef(cairo_image_surface_create_for_data(bitmapPtr, CAIRO_FORMAT_ARGB32, m_size.width(), m_size.height(), pitch));
}
#endif

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
bool ImageBuffer::makeEGLContextCurrent()
{
    if (cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        return m_data.m_platformSurface->makeContextCurrent();
    return false;
}
#endif

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, ScaleBehavior) const
{
    if (copyBehavior == CopyBackingStore)
        return BitmapImage::create(copyCairoImageSurface(m_data.m_surface.get()));

    // BitmapImage will release the passed in surface on destruction
    return BitmapImage::create(m_data.m_surface);
}

BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return DontCopyBackingStore;
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& maskRect) const
{
    context->platformContext()->pushImageMask(m_data.m_surface.get(), maskRect);
}

void ImageBuffer::draw(GraphicsContext* destinationContext, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
    CompositeOperator op, BlendMode blendMode, bool useLowQualityScale)
{
    BackingStoreCopy copyMode = destinationContext == context() ? CopyBackingStore : DontCopyBackingStore;
    RefPtr<Image> image = copyImage(copyMode);
    destinationContext->drawImage(image.get(), styleColorSpace, destRect, srcRect, op, blendMode, DoNotRespectImageOrientation, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<Image> image = copyImage(DontCopyBackingStore);
    image->drawPattern(context, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_t* src = 0;
    if (cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        src = cairo_surface_map_to_image(m_data.m_surface.get(), 0);
    else {
        ASSERT(cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_IMAGE);
        src = m_data.m_surface.get();
    }

    unsigned char* dataSrc = cairo_image_surface_get_data(src);
    int stride = cairo_image_surface_get_stride(src);
#else
    // FIXME: Enable color space conversions on accelerated canvases.
    if (cairo_surface_get_type(m_data.m_surface.get()) != CAIRO_SURFACE_TYPE_IMAGE)
        return;

    unsigned char* dataSrc = cairo_image_surface_get_data(m_data.m_surface.get());
    int stride = cairo_image_surface_get_stride(m_data.m_surface.get());
#endif
    for (int y = 0; y < m_size.height(); ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(dataSrc + stride * y);
        for (int x = 0; x < m_size.width(); x++) {
            unsigned* pixel = row + x;
            Color pixelColor = colorFromPremultipliedARGB(*pixel);
            pixelColor = Color(lookUpTable[pixelColor.red()],
                               lookUpTable[pixelColor.green()],
                               lookUpTable[pixelColor.blue()],
                               pixelColor.alpha());
            *pixel = premultipliedARGBFromColor(pixelColor);
        }
    }
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_mark_dirty_rectangle(src, 0, 0, m_size.width(), m_size.height());
    if (cairo_surface_get_type (m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        cairo_surface_unmap_image(m_data.m_surface.get(), src);
#else
    cairo_surface_mark_dirty_rectangle(m_data.m_surface.get(), 0, 0, m_size.width(), m_size.height());
#endif
}

#if !ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
static cairo_surface_t* mapSurfaceToImage(cairo_surface_t* surface, const IntSize& size)
{
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE)
        return surface;

    cairo_surface_t* imageSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height());
    RefPtr<cairo_t> cr = adoptRef(cairo_create(imageSurface));
    cairo_set_source_surface(cr.get(), surface, 0, 0);
    cairo_paint(cr.get());
    return imageSurface;
}

static void unmapSurfaceFromImage(cairo_surface_t* surface, cairo_surface_t* imageSurface, const IntRect& dirtyRectangle = IntRect())
{
    if (surface == imageSurface && dirtyRectangle.isEmpty())
        return;

    if (dirtyRectangle.isEmpty()) {
        cairo_surface_destroy(imageSurface);
        return;
    }

    if (surface == imageSurface) {
        cairo_surface_mark_dirty_rectangle(surface, dirtyRectangle.x(), dirtyRectangle.y(), dirtyRectangle.width(), dirtyRectangle.height());
        return;
    }

    RefPtr<cairo_t> cr = adoptRef(cairo_create(surface));
    cairo_set_source_surface(cr.get(), imageSurface, 0, 0);
    cairo_rectangle(cr.get(), dirtyRectangle.x(), dirtyRectangle.y(), dirtyRectangle.width(), dirtyRectangle.height());
    cairo_fill(cr.get());
    cairo_surface_destroy(imageSurface);
}
#endif

template <Multiply multiplied>
PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const ImageBufferData& data, const IntSize& size)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    unsigned char* dataSrc;

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    cairo_surface_t* src = 0;
    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL) {
        src = cairo_surface_map_to_image(data.m_surface.get(), 0);
        if (cairo_surface_status(src) != CAIRO_STATUS_SUCCESS)
            return 0;
    }
    else {
        ASSERT(cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_IMAGE);
        src = data.m_surface.get();
    }

    if (!src)
        return 0;

    dataSrc = cairo_image_surface_get_data(src);
#else
    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL) {
        cairo_surface_flush(data.m_surface.get());
        glFinish();
        dataSrc = static_cast<unsigned char*>(data.m_platformSurface->lockTbmBuffer());
    } else {
        ASSERT(cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_IMAGE);
        dataSrc = cairo_image_surface_get_data(data.m_surface.get());
    }
#endif
#else
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    cairo_surface_t* imageSurface = mapSurfaceToImage(data.m_surface.get(), size);
    unsigned char* dataSrc = cairo_image_surface_get_data(imageSurface);
#endif
    unsigned char* dataDst = result->data();

    if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > size.width() || (rect.y() + rect.height()) > size.height())
        result->zeroFill();

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    int stride;
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    stride = cairo_image_surface_get_stride(src);
#else
    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        stride = data.m_platformSurface->getStride();
    else
        stride = cairo_image_surface_get_stride(data.m_surface.get());
#endif
#else
    int stride = cairo_image_surface_get_stride(imageSurface);
#endif
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;

            // Avoid calling Color::colorFromPremultipliedARGB() because one
            // function call per pixel is too expensive.
            unsigned alpha = (*pixel & 0xFF000000) >> 24;
            unsigned red = (*pixel & 0x00FF0000) >> 16;
            unsigned green = (*pixel & 0x0000FF00) >> 8;
            unsigned blue = (*pixel & 0x000000FF);
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
            if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL && cairo_image_surface_get_format(src) == CAIRO_FORMAT_INVALID)
                swap(red, blue);
#endif

            if (multiplied == Unmultiplied) {
                if (alpha && alpha != 255) {
                    red = red * 255 / alpha;
                    green = green * 255 / alpha;
                    blue = blue * 255 / alpha;
                }
            }

            destRows[basex]     = red;
            destRows[basex + 1] = green;
            destRows[basex + 2] = blue;
            destRows[basex + 3] = alpha;
        }
        destRows += destBytesPerRow;
    }

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
        cairo_surface_unmap_image(data.m_surface.get(), src);
#else
        data.m_platformSurface->unlockTbmBuffer();
#endif
#else
    unmapSurfaceFromImage(data.m_surface.get(), imageSurface);
#endif

    return result.release();
}

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
template <Multiply multiplied>
PassRefPtr<Uint8ClampedArray> getImageDataTBM(const IntRect& rect, const ImageBufferData& data, const IntSize& size)
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    unsigned char* dataSrc;
    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL) {
        cairo_t* cr = cairo_create(data.m_pixmapSurface.get());
        cairo_save(cr);
        cairo_set_source_surface(cr, data.m_surface.get(), 0, 0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);
        cairo_surface_flush(data.m_pixmapSurface.get());
        glFlush();
        cairo_restore(cr);
        cairo_destroy(cr);
        dataSrc = static_cast<unsigned char*>(data.m_platformSurface->lockTbmBuffer());
    } else {
        ASSERT(cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_IMAGE);
        dataSrc = cairo_image_surface_get_data(data.m_surface.get());
    }
    unsigned char* dataDst = result->data();

    if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > size.width() || (rect.y() + rect.height()) > size.height())
        result->zeroFill();

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

    int stride;
    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        stride = data.m_platformSurface->getStride();
    else
        stride = cairo_image_surface_get_stride(data.m_surface.get());
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;

            // Avoid calling Color::colorFromPremultipliedARGB() because one
            // function call per pixel is too expensive.
            unsigned alpha = (*pixel & 0xFF000000) >> 24;
            unsigned red = (*pixel & 0x00FF0000) >> 16;
            unsigned green = (*pixel & 0x0000FF00) >> 8;
            unsigned blue = (*pixel & 0x000000FF);

            if (multiplied == Unmultiplied) {
                if (alpha && alpha != 255) {
                    red = red * 255 / alpha;
                    green = green * 255 / alpha;
                    blue = blue * 255 / alpha;
                }
            }

            destRows[basex] = red;
            destRows[basex + 1] = green;
            destRows[basex + 2] = blue;
            destRows[basex + 3] = alpha;
        }
        destRows += destBytesPerRow;
    }

    if (cairo_surface_get_type(data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        data.m_platformSurface->unlockTbmBuffer();

    return result.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageDataTBM(const IntRect& rect, CoordinateSystem) const
{
    return getImageDataTBM<Unmultiplied>(rect, m_data, m_size);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageDataTBM(const IntRect& rect, CoordinateSystem) const
{
    return getImageDataTBM<Premultiplied>(rect, m_data, m_size);
}
#endif

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<Unmultiplied>(rect, m_data, m_size);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<Premultiplied>(rect, m_data, m_size);
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_t* src = 0;
    if (cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        src = cairo_surface_map_to_image(m_data.m_surface.get(), 0);
    else {
        ASSERT(cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_IMAGE);
        src = m_data.m_surface.get();
    }
    unsigned char* dataDst = cairo_image_surface_get_data(src);
#else
    cairo_surface_t* imageSurface = mapSurfaceToImage(m_data.m_surface.get(), sourceSize);
    unsigned char* dataDst = cairo_image_surface_get_data(imageSurface);
#endif

    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < m_size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.maxX());

    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= m_size.width());

    int numColumns = endx - destx;

    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < m_size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.maxY());

    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= m_size.height());
    int numRows = endy - desty;

    unsigned srcBytesPerRow = 4 * sourceSize.width();
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    int stride = cairo_image_surface_get_stride(src);
#else
    int stride = cairo_image_surface_get_stride(imageSurface);
#endif

    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast_ptr<unsigned*>(dataDst + stride * (y + desty));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + destx;

            // Avoid calling Color::premultipliedARGBFromColor() because one
            // function call per pixel is too expensive.
            unsigned red = srcRows[basex];
            unsigned green = srcRows[basex + 1];
            unsigned blue = srcRows[basex + 2];
            unsigned alpha = srcRows[basex + 3];

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
            if (cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL && cairo_image_surface_get_format(src) == CAIRO_FORMAT_INVALID)
                swap(red, blue);
#endif

            if (multiplied == Unmultiplied) {
                if (alpha && alpha != 255) {
                    red = (red * alpha + 254) / 255;
                    green = (green * alpha + 254) / 255;
                    blue = (blue * alpha + 254) / 255;
                }
            }

            *pixel = (alpha << 24) | red  << 16 | green  << 8 | blue;
        }
        srcRows += srcBytesPerRow;
    }

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_mark_dirty_rectangle(src, destx, desty, numColumns, numRows);
    if (cairo_surface_get_type(m_data.m_surface.get()) == CAIRO_SURFACE_TYPE_GL)
        cairo_surface_unmap_image(m_data.m_surface.get(), src);
#else
    unmapSurfaceFromImage(m_data.m_surface.get(), imageSurface, IntRect(destx, desty, numColumns, numRows));
#endif
}

#if !PLATFORM(GTK)
static cairo_status_t writeFunction(void* output, const unsigned char* data, unsigned int length)
{
    if (!reinterpret_cast<Vector<unsigned char>*>(output)->tryAppend(data, length))
        return CAIRO_STATUS_WRITE_ERROR;
    return CAIRO_STATUS_SUCCESS;
}

static bool encodeImage(cairo_surface_t* image, const String& mimeType, Vector<char>* output)
{
    ASSERT_UNUSED(mimeType, mimeType == "image/png"); // Only PNG output is supported for now.

    return cairo_surface_write_to_png_stream(image, writeFunction, output) == CAIRO_STATUS_SUCCESS;
}

String ImageBuffer::toDataURL(const String& mimeType, const double*, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    cairo_surface_t* image = cairo_get_target(context()->platformContext()->cr());

    Vector<char> encodedImage;
    if (!image || !encodeImage(image, mimeType, &encodedImage))
        return "data:,";

    Vector<char> base64Data;
    base64Encode(encodedImage, base64Data);

    return "data:" + mimeType + ";base64," + base64Data;
}
#endif

#if ENABLE(ACCELERATED_2D_CANVAS)
void ImageBufferData::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    if (textureMapper->accelerationMode() != TextureMapper::OpenGLMode) {
        notImplemented();
        return;
    }

    ASSERT(m_texture);

    // Cairo may change the active context, so we make sure to change it back after flushing.
    GLContext* previousActiveContext = GLContext::getCurrent();
    cairo_surface_flush(m_surface.get());
    previousActiveContext->makeContextCurrent();

    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(m_texture, TextureMapperGL::ShouldBlend, m_size, targetRect, matrix, opacity);
}
#endif

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* ImageBuffer::platformLayer() const
{
#if ENABLE(ACCELERATED_2D_CANVAS)
    if (m_data.m_texture)
        return const_cast<ImageBufferData*>(&m_data);
#endif
    return 0;
}
#endif

} // namespace WebCore
