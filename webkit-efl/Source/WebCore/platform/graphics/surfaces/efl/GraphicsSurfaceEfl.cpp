/*
    Copyright (C) 2012 Samsung Electronics

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
#include "GraphicsSurface.h"

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#if !USE(TIZEN_PLATFORM_SURFACE)
#include "TextureMapperGL.h"

#include "NotImplemented.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace WebCore {

static Display *g_display = 0;
struct GraphicsSurfacePrivate {
    GraphicsSurfacePrivate()
        : m_textureIsYInverted(false)
        , m_hasAlpha(false)
        , m_xImage(0)
        , m_platformSurfaceID(0)
    {
        if (!g_display)
            g_display = XOpenDisplay(0);
    }

    ~GraphicsSurfacePrivate()
    {
        if (m_xImage) {
            XShmDetach(g_display, &(m_shmInfo));
            XDestroyImage(m_xImage);
            shmdt(m_shmInfo.shmaddr);
            shmctl(m_shmInfo.shmid, IPC_RMID, 0);
            m_xImage = NULL;
        }
    }

    uint32_t createSurface(const IntSize& size)
    {
        return 0;
    }

    void createPlatformSurface(uint32_t winId)
    {
    }

    void createImage(const IntSize& size, uint32_t platformSurfaceID)
    {
        m_platformSurfaceID = platformSurfaceID;
        int x=0, y=0;
        unsigned int w=0, h=0, bw=0, d=0;
        Window root_win = 0;

        if (!g_display) {
            fprintf(stderr, "Can not open display\n");
            return;
        }

        XGetGeometry(g_display, platformSurfaceID, &root_win, &x, &y, &w, &h, &bw, &d);
        m_size = IntSize(w, h);

        Screen* screen = DefaultScreenOfDisplay(g_display);
        m_xImage = XShmCreateImage(g_display, DefaultVisualOfScreen(screen), 24, ZPixmap, NULL, &(m_shmInfo), m_size.width(), m_size.height());
        if (!m_xImage) {
            fprintf(stderr, "Error creating the shared image\n");
            return;
        }
        m_shmInfo.shmid = shmget(IPC_PRIVATE, m_xImage->bytes_per_line * m_xImage->height, IPC_CREAT | 0666);
        if (m_shmInfo.shmid == -1) {
            XDestroyImage(m_xImage);
            m_xImage = 0;
            fprintf(stderr, "Error getting the shared image info\n");
            return;
        }
        m_shmInfo.readOnly = false;
        m_shmInfo.shmaddr = (char*)shmat(m_shmInfo.shmid, 0, 0);
        m_xImage->data = (char*)m_shmInfo.shmaddr;

        if ((m_xImage->data == (char *)-1) || (!m_xImage->data)) {
            shmdt(m_shmInfo.shmaddr);
            shmctl(m_shmInfo.shmid, IPC_RMID, 0);
            XDestroyImage(m_xImage);
            m_xImage = 0;
            return;
        }

        XShmAttach(g_display, &m_shmInfo);
    }

    void copyImage()
    {
        if (!g_display) {
            fprintf(stderr, "Can not open display\n");
            return;
        }

        if (!m_xImage)
            return;

        XGrabServer(g_display);
        if (!XShmGetImage(g_display, (Drawable)m_platformSurfaceID, m_xImage, 0, 0, 0xffffffff))
            fprintf(stderr, "Error Getting ShmGetImage\n");
        XUngrabServer(g_display);
        XSync(g_display, false);

        // Convert to RGBA
        int totalBytes = m_size.width() * m_size.height() * 4;
        for (int i = 0; i < totalBytes; i += 4)
            std::swap(m_xImage->data[i], m_xImage->data[i + 2]);
    }

    void* xImageData() { return m_xImage ? m_xImage->data : 0; }
    void swapBuffers() { copyImage(); }
    IntSize size() const { return m_size; }

private:
    IntSize m_size;
    bool m_textureIsYInverted;
    bool m_hasAlpha;
    XImage* m_xImage;
    XShmSegmentInfo m_shmInfo;
    int m_platformSurfaceID;
};

GraphicsSurfaceToken GraphicsSurface::platformExport()
{
    return GraphicsSurfaceToken(m_platformSurface);
}

uint32_t GraphicsSurface::platformGetTextureID()
{
    if (!m_texture) {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (m_texture) {
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_size.width(), m_size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_private->xImageData());
    }
    return m_texture;
}

void GraphicsSurface::platformPaintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(platformGetTextureID(), 0, m_size, targetRect, transform, opacity);
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(const IntSize& , Flags, const PlatformGraphicsContext3D)
{
    return 0;
}

#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(uint64_t platformSurfaceID)
{
    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(IntSize(), 0));
    surface->m_private = new GraphicsSurfacePrivate();
    surface->m_platformSurface = (uint64_t)platformSurfaceID;
    return surface;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, uint64_t token, Flags canvasFlags)
{
    // X11 does not support CopyToTexture, so we do not create a GraphicsSurface if this is requested.
    // GraphicsSurfaceGLX uses an XWindow as native surface. This one always has a front and a back buffer.
    // Therefore single buffered GraphicsSurfaces are not supported.
    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate();
    surface->m_platformSurface = token;
    surface->m_canvasFlags = canvasFlags;

    surface->m_private->createImage(IntSize(), token);
    surface->m_size = surface->m_private->size();

    return surface;
}
#endif

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, const GraphicsSurfaceToken& token)
{
    // X11 does not support CopyToTexture, so we do not create a GraphicsSurface if this is requested.
    // GraphicsSurfaceGLX uses an XWindow as native surface. This one always has a front and a back buffer.
    // Therefore single buffered GraphicsSurfaces are not supported.
    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate();
    surface->m_platformSurface = token.frontBufferHandle;

    surface->m_private->createImage(IntSize(), token.frontBufferHandle);
    surface->m_size = surface->m_private->size();

    return surface;
}

void GraphicsSurface::platformDestroy()
{
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    if (m_private) {
        delete m_private;
        m_private = 0;
    }
}

void GraphicsSurface::platformCopyToGLTexture(uint32_t target, uint32_t id, const IntRect& targetRect, const IntPoint& offset)
{
    // This is not supported by GLX/Xcomposite.
}

void GraphicsSurface::platformCopyFromTexture(uint32_t textureId, const IntRect&)
{
    notImplemented();
}

uint32_t GraphicsSurface::platformFrontBuffer() const
{
    return 0;
}

uint32_t GraphicsSurface::platformSwapBuffers()
{
    m_private->swapBuffers();
    return 0;
}

IntSize GraphicsSurface::platformSize() const
{
    return m_private->size();
}

#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
uint32_t GraphicsSurface::platformSwapBuffers(uint32_t)
{
    return 0;
}
#endif

char* GraphicsSurface::platformLock(const IntRect& rect, int* outputStride, LockOptions lockOptions)
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
    return 0;
}

void GraphicsSurface::platformUnlock()
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
}

PassOwnPtr<GraphicsContext> GraphicsSurface::platformBeginPaint(const IntSize& size, char* bits, int stride)
{
    notImplemented();
    return nullptr;
}

PassRefPtr<Image> GraphicsSurface::createReadOnlyImage(const IntRect& rect)
{
    notImplemented();
    return 0;
}
}
#endif
#endif
