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

#if USE(TIZEN_PLATFORM_SURFACE)
#include "SharedPlatformSurfaceTizen.h"
#include "PixmapContextTizen.h"
#include "X11Helper.h"

#include "NotImplemented.h"


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <dri2/dri2.h>
#include <libdrm/drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
}

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/StdLibExtras.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>

typedef struct TBM_INFO TbmInfo;
struct TBM_INFO {
    int screen;
    int fileDescriptor;
    tbm_bufmgr bufmgr;
};

namespace WebCore {

Display* g_nativeDisplay;
int g_nativeWindow;
TbmInfo g_TbmInfo;

class PixmapContextPool {
protected:
    PixmapContextPool(void);
    virtual ~PixmapContextPool();
public:
    static inline PixmapContextPool& getInstance()
    {
        static PixmapContextPool pixmapContextPool;
        return pixmapContextPool;
    }

    PixmapContextTizen* getContext(bool isLockable, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples);

private:
    bool initializeTizenBufferManager();
    void finalizeTizenBufferManager();
    typedef HashMap<int, RefPtr<PixmapContextTizen> > PixmapContextMap;
    PixmapContextMap m_pixmapContexts;
};

PixmapContextPool::PixmapContextPool()
{
    if (!g_nativeDisplay)
        g_nativeDisplay = X11Helper::nativeDisplay();
    if (!g_nativeDisplay) {
        //TIZEN_LOGE("'XOpenDisplay' Failed to Open display in X server\n");
        exit(0);
    }

    if (!g_nativeWindow)
        g_nativeWindow = XCreateSimpleWindow(g_nativeDisplay, XDefaultRootWindow(g_nativeDisplay),
                            0, 0, 1, 1, 0,
                            BlackPixel(g_nativeDisplay, 0), WhitePixel(g_nativeDisplay, 0));

    if (!initializeTizenBufferManager()) {
        //TIZEN_LOGE("'initializeTizenBufferManager' Failed to initializeTizenBuffer Manager\n");
    }
    XFlush(g_nativeDisplay);
}

PixmapContextPool::~PixmapContextPool()
{
    PixmapContextMap::iterator end = m_pixmapContexts.end();
    for (PixmapContextMap::iterator iter = m_pixmapContexts.begin(); iter != end; ++iter) {
        RefPtr<PixmapContextTizen> context = iter->value;
        context.release();
    }
    m_pixmapContexts.clear();
    finalizeTizenBufferManager();

    if (g_nativeWindow) {
        XDestroyWindow(g_nativeDisplay, g_nativeWindow);
        g_nativeWindow = 0;
    }

    // FIXME : rebase
    // Now we use "X11Helper::nativeDisplay()" function to acquire display.
    // And this display will be closed in destructor of "DisplayConnection".
    // So we don't need to close display here.
    //XCloseDisplay(g_nativeDisplay);
}

bool PixmapContextPool::initializeTizenBufferManager()
{
    int dri2EvBase = 0;
    int dri2ErrBase = 0;
    int dri2Major = 0;
    int dri2Minor = 0;
    char* driverName=NULL;
    char* deviceName=NULL;
    drm_magic_t magic=0;
    int fileDescriptor=-1;

    g_TbmInfo.screen = XDefaultScreen(g_nativeDisplay);
    if (!DRI2QueryExtension(g_nativeDisplay, &dri2EvBase, &dri2ErrBase)) {
        //TIZEN_LOGE("'DRI2QueryExtension' Failed to get dri2 extension\n");
        goto error;
    }

    if (!DRI2QueryVersion(g_nativeDisplay, &dri2Major, &dri2Minor)) {
        //TIZEN_LOGE("'DRI2QueryVersion' Failed to get dri2 version\n");
        goto error;
    }

    if (!DRI2Connect(g_nativeDisplay, RootWindow(g_nativeDisplay, g_TbmInfo.screen), &driverName, &deviceName)) {
        //TIZEN_LOGE("'DRI2Connect' Failed to get dri2 connect\n");
        goto error;
    }

    fileDescriptor = open(deviceName, O_RDWR);
    if (fileDescriptor < 0) {
        //TIZEN_LOGE("[DRM] couldn't open : ");
        goto error;
    }

    if (drmGetMagic(fileDescriptor, &magic)) {
        //TIZEN_LOGE("'drmGetMagic' Failed to get magic\n");
        goto error;
    }

    if (!DRI2Authenticate(g_nativeDisplay, RootWindow(g_nativeDisplay, g_TbmInfo.screen), (unsigned int)magic)) {
        //TIZEN_LOGE("'DRI2Authenticate' Failed to get authentication\n");
        goto error;
    }

    g_TbmInfo.fileDescriptor = fileDescriptor;

    /* get buffer manager */
    g_TbmInfo.bufmgr = tbm_bufmgr_init (g_TbmInfo.fileDescriptor);

    free(driverName);
    free(deviceName);
    return true;

error:
    free(driverName);
    free(deviceName);
    if (fileDescriptor > -1)
        close(fileDescriptor);
    return false;
}

void PixmapContextPool::finalizeTizenBufferManager()
{
    if (g_TbmInfo.bufmgr)
        tbm_bufmgr_deinit (g_TbmInfo.bufmgr);

    if (g_TbmInfo.fileDescriptor > -1)
        close(g_TbmInfo.fileDescriptor);
}

PixmapContextTizen* PixmapContextPool::getContext(bool isLockable, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples)
{
    int contextId = ((isLockable) | (hasAlpha << 1) | (hasDepth << 2) | (hasStencil << 3) | (hasSamples << 4));
    RefPtr<PixmapContextTizen> pixmapContext = m_pixmapContexts.get(contextId);
    if (!pixmapContext) {
        pixmapContext = PixmapContextTizen::create(isLockable, hasAlpha, hasDepth, hasStencil, hasSamples);
        if (pixmapContext)
            m_pixmapContexts.add(contextId, pixmapContext);
        else
            return 0;
    }
    return pixmapContext.get();
}

PassRefPtr<PixmapContextTizen> PixmapContextTizen::create(bool isLockable, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples)
{
    RefPtr<PixmapContextTizen> context = adoptRef(new PixmapContextTizen(isLockable, hasAlpha, hasDepth, hasStencil, hasSamples));
    if (!context->initialize())
        return 0;
    return context.release();
}

void PixmapContextTizen::HandleEGLError(const char* name)
{
    static const char* const egErrorStrings[] =
    {
        "EGL_SUCCESS",
        "EGL_NOT_INITIALIZED",
        "EGL_BAD_ACCESS",
        "EGL_BAD_ALLOC",
        "EGL_BAD_ATTRIBUTE",
        "EGL_BAD_CONFIG",
        "EGL_BAD_CONTEXT",
        "EGL_BAD_CURRENT_SURFACE",
        "EGL_BAD_DISPLAY",
        "EGL_BAD_MATCH",
        "EGL_BAD_NATIVE_PIXMAP",
        "EGL_BAD_NATIVE_WINDOW",
        "EGL_BAD_PARAMETER",
        "EGL_BAD_SURFACE"
    };

    EGLint errorCode = eglGetError();

    if (errorCode != EGL_SUCCESS) {
        TIZEN_LOGE("'%s' returned egl error '%s' (0x%x)", name, egErrorStrings[errorCode - EGL_SUCCESS], errorCode);
    }
}

PixmapContextTizen::PixmapContextTizen(bool renderedByCPU, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples)
    : m_display(EGL_NO_DISPLAY)
    , m_context(EGL_NO_CONTEXT)
    , m_isRenderedByCPU(renderedByCPU)
    , m_hasAlpha(hasAlpha)
    , m_hasDepth(hasDepth)
    , m_hasStencil(hasStencil)
    , m_hasSamples(hasSamples)
{
}

PixmapContextTizen::~PixmapContextTizen()
{
    if (m_context) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(m_display, m_context);
        m_context = EGL_NO_CONTEXT;
    }
    m_display = 0;
}

bool PixmapContextTizen::initialize()
{
    if (m_isRenderedByCPU)
        return true;

    EGLint major, minor;
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    m_display = eglGetDisplay(g_nativeDisplay);
    if (m_display == EGL_NO_DISPLAY)
        HandleEGLError("eglGetDisplay");

    if (eglInitialize(m_display, &major, &minor) != EGL_TRUE)
        HandleEGLError("eglInitialize");

    EGLint configCount = 0;
    int i = 0;
    EGLint configAttribs[32];
    configAttribs[i++] = EGL_LEVEL;
    configAttribs[i++] = 0;
    // FIXME : Create pixmap surface with RGB_565 when alpha is off. Graphics driver needs to be fixed first.
    configAttribs[i++] = EGL_RED_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_GREEN_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_BLUE_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_ALPHA_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_SURFACE_TYPE;
    configAttribs[i++] = EGL_PIXMAP_BIT;
    configAttribs[i++] = EGL_DEPTH_SIZE;
    if (m_hasDepth) {
        if (m_hasSamples)
            configAttribs[i++] = 8;
        else
            configAttribs[i++] = 16;
    }
    else
        configAttribs[i++] = 0;
    configAttribs[i++] = EGL_STENCIL_SIZE;
    if (m_hasStencil)
        configAttribs[i++] = 8;
    else
        configAttribs[i++] = 0;
    configAttribs[i++] = EGL_SAMPLES;
    if (m_hasSamples)
#if ENABLE(TIZEN_CAIROGLES_MULTISAMPLE_WORKAROUND)
        configAttribs[i++] = 2;
#else
        configAttribs[i++] = 4;
#endif
    else
        configAttribs[i++] = 1;
    configAttribs[i++] = EGL_RENDERABLE_TYPE;
    configAttribs[i++] = EGL_OPENGL_ES2_BIT;
    configAttribs[i++] = EGL_NONE;

    if (eglChooseConfig(m_display, configAttribs, &m_surfaceConfig, 1, &configCount) != EGL_TRUE) {
        HandleEGLError("eglChooseConfig");
        return false;
    }
    m_context = eglCreateContext(m_display, m_surfaceConfig, EGL_NO_CONTEXT, contextAttribs);
    if( m_context == EGL_NO_CONTEXT) {
        HandleEGLError("eglCreateContext");
        return false;
    }
    return true;
}

bool PixmapContextTizen::makeCurrent(EGLSurface surface)
{
    if (eglGetCurrentSurface(EGL_DRAW) == surface && eglGetCurrentContext() == m_context)
        return true;

    if (eglMakeCurrent(m_display, surface, surface, m_context) != EGL_TRUE) {
        HandleEGLError("eglMakeCurrent");
        return false;
    }
    return true;
}

void* PixmapContextTizen::createSurface(int pixmapID)
{
    EGLSurface surface = eglCreatePixmapSurface(m_display, m_surfaceConfig, pixmapID, NULL);
    if (surface == EGL_NO_SURFACE)
        HandleEGLError("eglCreatePixmapSurface");

    return surface;
}

void PixmapContextTizen::destroySurface(EGLSurface surface)
{
    if (surface) {
        if (eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
            HandleEGLError("eglMakeCurrent");
        eglDestroySurface(m_display, surface);
    }
}

int PixmapContextTizen::createPixmap(int width, int height)
{
    // FIXME : Create 16 bit pixmap when alpha is off. Graphics driver needs to be fixed first.
    int pix = XCreatePixmap(g_nativeDisplay, g_nativeWindow, width, height, 32);
    XFlush(g_nativeDisplay);

    return pix;
}

void PixmapContextTizen::freePixmap(int pixmap)
{
    if (pixmap)
        XFreePixmap(g_nativeDisplay, pixmap);
}

void SharedPlatformSurfaceManagement::deferredRemovePlatformSurface(int platformSurfaceId)
{
    m_platformSurfacesIdToDeferredRemove.append(platformSurfaceId);
}

void SharedPlatformSurfaceManagement::doDeferredRemove()
{
    for (Vector<int>::iterator iter = m_platformSurfacesIdToDeferredRemove.begin(); iter != m_platformSurfacesIdToDeferredRemove.end(); ++iter) {
        int pixmapId = *iter;
        XFreePixmap(g_nativeDisplay, pixmapId);
    }
    m_platformSurfacesIdToDeferredRemove.clear();
}

PassOwnPtr<SharedPlatformSurfaceTizen> SharedPlatformSurfaceTizen::create(const IntSize& size, bool lockable, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples, PixmapContextTizen* pixmapContext)
{
    OwnPtr<SharedPlatformSurfaceTizen> pixmap = adoptPtr(new SharedPlatformSurfaceTizen(size, lockable, hasAlpha, hasDepth, hasStencil, hasSamples, pixmapContext));
    if (!pixmap->initialize())
        return nullptr;
    return pixmap.release();
}

SharedPlatformSurfaceTizen::SharedPlatformSurfaceTizen(const IntSize& size, bool renderedByCPU, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples, PixmapContextTizen* pixmapContext)
    : m_pixmap(0)
    , m_size(size)
    , m_surface(EGL_NO_SURFACE)
    , m_isRenderedByCPU(renderedByCPU)
    , m_hasAlpha(hasAlpha)
    , m_hasDepth(hasDepth)
    , m_hasStencil(hasStencil)
    , m_hasSamples(hasSamples)
    , m_isUsed(false)
    , m_bufferObject(0)
{
    SharedPlatformSurfaceManagement& sharedPlatformSurfaceManagement = SharedPlatformSurfaceManagement::getInstance();
    sharedPlatformSurfaceManagement.growCount();

    m_pixmapContext = pixmapContext;
}

SharedPlatformSurfaceTizen::~SharedPlatformSurfaceTizen()
{
    if (m_surface != EGL_NO_SURFACE) {
        m_pixmapContext->destroySurface(m_surface);
        m_surface = EGL_NO_SURFACE;
    }

    if (m_bufferObject)
        tbm_bo_unref(m_bufferObject);

    if ((m_isRenderedByCPU || m_hasSamples) && m_pixmap > 0)
        DRI2DestroyDrawable(g_nativeDisplay, m_pixmap);

    SharedPlatformSurfaceManagement& sharedPlatformSurfaceManagement = SharedPlatformSurfaceManagement::getInstance();
    if (m_pixmap) {
        if (m_isRenderedByCPU)
            m_pixmapContext->freePixmap(m_pixmap);
        else
            sharedPlatformSurfaceManagement.deferredRemovePlatformSurface(m_pixmap);
        m_pixmap = 0;
    }
    sharedPlatformSurfaceManagement.shrinkCount();
}

bool SharedPlatformSurfaceTizen::initialize()
{
    if (m_size.isEmpty())
        return false;

    if (!m_pixmapContext) {
        PixmapContextPool& pixmapContextPool = PixmapContextPool::getInstance();
        m_pixmapContext = pixmapContextPool.getContext(m_isRenderedByCPU, m_hasAlpha, m_hasDepth, m_hasStencil, m_hasSamples);
    }

    if (!m_pixmapContext)
        return false;

    m_pixmap = m_pixmapContext->createPixmap(m_size.width(), m_size.height());
    if (!m_pixmap)
        return false;

    if (m_isRenderedByCPU)
        DRI2CreateDrawable(g_nativeDisplay, m_pixmap);
    else {
        if (m_hasSamples)
            DRI2CreateDrawable(g_nativeDisplay, m_pixmap);
        m_surface = (void*)(m_pixmapContext->createSurface(m_pixmap));
        if (m_surface == EGL_NO_SURFACE)
            return false;
    }

    return true;
}

void* SharedPlatformSurfaceTizen::context()
{
    return m_pixmapContext->context();
}

void* SharedPlatformSurfaceTizen::display()
{
    return m_pixmapContext->display();
}

bool SharedPlatformSurfaceTizen::makeContextCurrent()
{
    return m_pixmapContext->makeCurrent(m_surface);
}

void* SharedPlatformSurfaceTizen::lockTbmBuffer()
{
    tbm_bo_handle handle;
    unsigned int attach = DRI2BufferFrontLeft;
    int num=-1, width=0, height=0;

    if (!m_bufferObject) {
        DRI2Buffer* buf = DRI2GetBuffers(g_nativeDisplay, m_pixmap, &width, &height, &attach, 1, &num);

        if (!buf || !buf->name) {
            //TIZEN_LOGE("'DRI2GetBuffers' Failed to get dri2 buffers\n");
            XFree(buf);
            return 0;
        }

        m_bufferObject = tbm_bo_import(g_TbmInfo.bufmgr, buf->name);
        if (!m_bufferObject) {
            //TIZEN_LOGE("'tbm_bo_import' Failed to import buffer\n");
            return 0;
        }
        XFree(buf);
    }

    handle = tbm_bo_map(m_bufferObject, TBM_DEVICE_CPU, TBM_OPTION_READ | TBM_OPTION_WRITE);

    if (!handle.ptr) {
        //TIZEN_LOGE("'tbm_bo_map' Failed to map tbm buffer for GPU\n");
        return 0;
    }
    return handle.ptr;
}

bool SharedPlatformSurfaceTizen::unlockTbmBuffer()
{
    if (!m_bufferObject)
        return false;

    tbm_bo_unmap(m_bufferObject);
    return true;
}

int SharedPlatformSurfaceTizen::getStride()
{
    unsigned int attach = DRI2BufferFrontLeft;
    int num = -1, width = 0, height = 0;

    DRI2Buffer* buf = DRI2GetBuffers(g_nativeDisplay, m_pixmap, &width, &height, &attach, 1, &num);
    int stride = buf->pitch;

    XFree(buf);
    return stride;
}

PassOwnPtr<SharedPlatformSurfaceSimpleTizen> SharedPlatformSurfaceSimpleTizen::create(const IntSize& size, int pixmapId)
{
    OwnPtr<SharedPlatformSurfaceSimpleTizen> pixmap = adoptPtr(new SharedPlatformSurfaceSimpleTizen(size, pixmapId));
    if (!pixmap->initialize())
        return nullptr;
    return pixmap.release();
}

SharedPlatformSurfaceSimpleTizen::SharedPlatformSurfaceSimpleTizen(const IntSize& size, int pixmapId)
    : SharedPlatformSurfaceTizen(size, true, false, false, false, false, NULL)
{
    m_pixmap = pixmapId;
}

SharedPlatformSurfaceSimpleTizen::~SharedPlatformSurfaceSimpleTizen()
{
    if (m_pixmap > 0) {
        DRI2DestroyDrawable(g_nativeDisplay, m_pixmap);
        m_pixmap = 0;
    }
}

bool SharedPlatformSurfaceSimpleTizen::initialize()
{
    if (m_size.isEmpty())
        return false;

    if (!m_pixmapContext) {
        PixmapContextPool& pixmapContextPool = PixmapContextPool::getInstance();
        m_pixmapContext = pixmapContextPool.getContext(m_isRenderedByCPU, m_hasAlpha, m_hasDepth, m_hasStencil, m_hasSamples);
    }

    if (!m_pixmapContext)
        return false;

    if (!m_pixmap)
        return false;

    DRI2CreateDrawable(g_nativeDisplay, m_pixmap);

    return true;
}

}
#endif // USE(TIZEN_PLATFORM_SURFACE)
