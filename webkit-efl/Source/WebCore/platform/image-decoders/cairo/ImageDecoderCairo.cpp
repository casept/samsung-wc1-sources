/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#include "ImageDecoder.h"

#include <cairo.h>

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
#define CAIRO_HAS_GL_SURFACE 0
#define CAIRO_HAS_GLX_FUNCTIONS 0
#define CAIRO_HAS_WGL_FUNCTIONS 0
#include <cairo-gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

namespace WebCore {

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
extern void* g_egl_display;
extern cairo_device_t* g_sharedCairoDevice;

PassNativeImagePtr ImageFrame::asNewNativeImageForGL()
{
    if (width() > CAIRO_GL_SURFACE_MAX_WIDTH || height() > CAIRO_GL_SURFACE_MAX_HEIGHT || !m_platformSurfaceID) {
        cairo_surface_t* imageSurface = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(const_cast<PixelData*>(m_bytes)),
            CAIRO_FORMAT_ARGB32, width(), height(), m_stride ? stride() : width() * sizeof(PixelData));

        float scaleX = CAIRO_GL_SURFACE_MAX_WIDTH / static_cast<float>(width());
        float scaleY = CAIRO_GL_SURFACE_MAX_HEIGHT / static_cast<float>(height());
        float scale = scaleX < scaleY ? scaleX : scaleY;

        cairo_content_t content;
        RefPtr<cairo_surface_t> glSurface;
        RefPtr<cairo_t> glSurface_cr;

        if (scale >= 1) {
            content = cairo_surface_get_content(imageSurface);
            glSurface = adoptRef(cairo_gl_surface_create(static_cast<cairo_device_t*>(g_sharedCairoDevice),
                content, cairo_image_surface_get_width(imageSurface), cairo_image_surface_get_height(imageSurface)));

            glSurface_cr = adoptRef(cairo_create(glSurface.get()));
            cairo_set_source_surface(glSurface_cr.get(), imageSurface, 0, 0);
            cairo_paint(glSurface_cr.get());
        } else {
            RefPtr<cairo_surface_t> shrink_image = adoptRef(cairo_image_surface_create(cairo_image_surface_get_format(imageSurface),
                                                      static_cast<int>(width() * scale), static_cast<int>(height() * scale)));
            RefPtr<cairo_t> shrink_cr = adoptRef(cairo_create(shrink_image.get()));
            cairo_scale(shrink_cr.get(), scale, scale);
            cairo_set_source_surface(shrink_cr.get(), imageSurface, 0, 0);
            cairo_set_operator(shrink_cr.get(), CAIRO_OPERATOR_SOURCE);
            cairo_paint(shrink_cr.get());

            content = cairo_surface_get_content(shrink_image.get());
            glSurface = adoptRef(cairo_gl_surface_create(static_cast<cairo_device_t*>(g_sharedCairoDevice),
                content, cairo_image_surface_get_width(shrink_image.get()), cairo_image_surface_get_height(shrink_image.get())));

            glSurface_cr = adoptRef(cairo_create(glSurface.get()));
            cairo_set_source_surface(glSurface_cr.get(), shrink_image.get(), 0, 0);
            cairo_paint(glSurface_cr.get());
        }

        /*
        // rebase
        m_nativeimage = new NativeImageCairo();
        m_nativeimage->setGLSurface(cairo_surface_reference(glSurface.get()));
        m_nativeimage->setImageFrame(this);
        */

        if (imageSurface) {
            cairo_surface_destroy(imageSurface);
            m_backingStore.clear();
            m_bytes = 0;
        }

        return glSurface.release();
    }

    GLint boundTex;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);

    if (m_stride) {
        PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = 0;
        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = 0;

        EGLint image_attrib[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };

        eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        if (!eglCreateImageKHR) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot get eglCreateImageKHR\n");
#endif
            return 0;
        }
        glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        if (!glEGLImageTargetTexture2DOES) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot get glEGLImageTargetTexture2DOES\n");
#endif
            return 0;
        }

        m_eglImage = eglCreateImageKHR(g_egl_display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, reinterpret_cast<EGLClientBuffer>(m_platformSurfaceID), image_attrib);
        if (!m_eglImage) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot create egl image\n");
#endif
            return 0;
        }

        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    } else {
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        lockTbmBuffer();
        glTexImage2D(GL_TEXTURE_2D, 0, 0x80E1, width(), height(), 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_bytes);
        unlockTbmBuffer();
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    cairo_surface_t* surface = cairo_gl_surface_create_for_texture(static_cast<cairo_device_t*>(g_sharedCairoDevice), CAIRO_CONTENT_COLOR_ALPHA, m_tex, width(), height());
    cairo_surface_mark_dirty(surface);

    glBindTexture(GL_TEXTURE_2D, boundTex);

    /*
    // rebase
    m_nativeimage = new NativeImageCairo();
    m_nativeimage->setGLSurface(surface);
    m_nativeimage->setImageFrame(this);
    */
    return adoptRef(surface);
}

// FIXME : rebase
// It seems not used.
/*
void ImageFrame::addImageSurfaceFromData(NativeImagePtr nativeimage)
{
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        reinterpret_cast<unsigned char*>(const_cast<PixelData*>(m_bytes)),
        CAIRO_FORMAT_ARGB32, width(), height(), m_stride ? stride() : width() * sizeof(PixelData));

    nativeimage->setImageSurface(surface);
}

void ImageFrame::addGLSurfaceFromData(NativeImagePtr nativeimage)
{
    //FIX ME : Add procedure to copy cpu memory to pixmap.

    if (width() > CAIRO_GL_SURFACE_MAX_WIDTH || height() > CAIRO_GL_SURFACE_MAX_HEIGHT) {
        cairo_surface_t* imageSurface = cairo_image_surface_create_for_data(
            reinterpret_cast<unsigned char*>(const_cast<PixelData*>(m_bytes)),
            CAIRO_FORMAT_ARGB32, width(), height(), m_stride ? stride() : width() * sizeof(PixelData));

        float scaleX = CAIRO_GL_SURFACE_MAX_WIDTH / static_cast<float>(width());
        float scaleY = CAIRO_GL_SURFACE_MAX_HEIGHT / static_cast<float>(height());
        float scale = scaleX < scaleY ? scaleX : scaleY;

        RefPtr<cairo_surface_t> shrink_image = adoptRef(cairo_image_surface_create(cairo_image_surface_get_format(imageSurface),
                                                  static_cast<int>(width() * scale), static_cast<int>(height() * scale)));
        RefPtr<cairo_t> shrink_cr = adoptRef(cairo_create(shrink_image.get()));
        cairo_scale(shrink_cr.get(), scale, scale);
        cairo_set_source_surface(shrink_cr.get(), imageSurface, 0, 0);
        cairo_set_operator(shrink_cr.get(), CAIRO_OPERATOR_SOURCE);
        cairo_paint(shrink_cr.get());

        cairo_content_t content = cairo_surface_get_content(shrink_image.get());
        RefPtr<cairo_surface_t> glSurface = adoptRef(cairo_gl_surface_create(static_cast<cairo_device_t*>(g_sharedCairoDevice), content, cairo_image_surface_get_width(shrink_image.get()), cairo_image_surface_get_height(shrink_image.get())));

        RefPtr<cairo_t> glSurface_cr = adoptRef(cairo_create(glSurface.get()));
        cairo_set_source_surface(glSurface_cr.get(), shrink_image.get(), 0, 0);
        cairo_paint(glSurface_cr.get());

        m_nativeimage->setGLSurface(cairo_surface_reference(glSurface.get()));
        m_nativeimage->setImageFrame(this);

        if (imageSurface) {
            cairo_surface_destroy(imageSurface);
            m_backingStore.clear();
            m_bytes = 0;
        }

        return;
    }

    GLint boundTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);

    if (m_stride) {
        PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = 0;
        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = 0;

        EGLint image_attrib[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };

        eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

        m_eglImage = eglCreateImageKHR(g_egl_display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, reinterpret_cast<EGLClientBuffer>(m_platformSurfaceID), image_attrib);

        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    } else {
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);

        glTexImage2D(GL_TEXTURE_2D, 0, 0x80E1, width(), height(), 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_bytes);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, boundTex);

    cairo_surface_t* surface = cairo_gl_surface_create_for_texture(static_cast<cairo_device_t*>(g_sharedCairoDevice), CAIRO_CONTENT_COLOR_ALPHA, m_tex, width(), height());

    m_nativeimage->setGLSurface(surface);
}
*/
#endif

PassNativeImagePtr ImageFrame::asNewNativeImage() const
{
    return adoptRef(cairo_image_surface_create_for_data(
        reinterpret_cast<unsigned char*>(const_cast<PixelData*>(m_bytes)),
        CAIRO_FORMAT_ARGB32, width(), height(), width() * sizeof(PixelData)));
}

} // namespace WebCore
