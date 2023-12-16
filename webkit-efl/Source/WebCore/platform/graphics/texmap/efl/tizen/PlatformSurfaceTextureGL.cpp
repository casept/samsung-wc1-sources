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

#include <EGL/egl.h>

#include "PlatformSurfaceTextureGL.h"

namespace WebCore {
PlatformSurfaceTextureGL::PlatformSurfaceTextureGL(int platformSurfaceID, const IntSize& size)
    : m_eglImage(0)
    , m_platformSurfaceID(platformSurfaceID)
    , m_id(0)
    , m_used(false)
    , m_textureSize(size)
{
    m_SharedPlatformSurfaceTizen =  SharedPlatformSurfaceSimpleTizen::create(m_textureSize, m_platformSurfaceID);
}

PlatformSurfaceTextureGL::~PlatformSurfaceTextureGL()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &m_id);
    evasglDestroyImage(m_eglImage);
    HandleEGLError("evasglDestroyImage");
    m_id = 0;
}

bool PlatformSurfaceTextureGL::initialize(bool useLinearFilter)
{
    if (m_platformSurfaceID == 0) {
        LOG_ERROR("ID is invalid!");
        return 0;
    }

    GLfloat filter = GL_NEAREST;
    if (useLinearFilter)
        filter = GL_LINEAR;

    m_eglImage = evasglCreateImage(EVAS_GL_NATIVE_PIXMAP, (void*)m_platformSurfaceID, 0);
    HandleEGLError("evasglCreateImage");

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glEvasGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    HandleGLError("glEvasGLImageTargetTexture2DOES initialize");

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

void PlatformSurfaceTextureGL::updateTexture()
{
    glBindTexture(GL_TEXTURE_2D, m_id);
    glEvasGLImageTargetTexture2DOES(GL_TEXTURE_2D, m_eglImage);
    HandleGLError("glEvasGLImageTargetTexture2DOES updateTexture");
}

void PlatformSurfaceTextureGL::copyRect(unsigned char* dst, unsigned char* src, const IntRect& wholeRect, const IntRect& dirtyRect)
{
    // FIXME : Now, handling row datas directly performs better than using Cairo APIs.
    // This will be replaced to Cairo APIs if using Cairo APIs perform better.
    unsigned int offset = 4 * wholeRect.width() * (dirtyRect.y() - wholeRect.y()) + 4 * (dirtyRect.x() - wholeRect.x());
    for (int i = 0; i < dirtyRect.height(); i++) {
        memcpy(dst + offset, src + (4 * i * wholeRect.width()), 4 * dirtyRect.width());
        offset += 4* wholeRect.width();
    }
}
void PlatformSurfaceTextureGL::copyPlatformSurfaceToTextureGL(const IntRect& dirtyRect, SharedPlatformSurfaceSimpleTizen* sourceTexture)
{
    IntRect wholeRect = IntRect(IntPoint(0, 0), m_textureSize);
    unsigned char* dstBuffer = (unsigned char*)m_SharedPlatformSurfaceTizen->lockTbmBuffer();
    unsigned char* srcBuffer = (unsigned char*)sourceTexture->lockTbmBuffer();

    copyRect(dstBuffer, srcBuffer, wholeRect, dirtyRect);
    m_SharedPlatformSurfaceTizen->unlockTbmBuffer();
    sourceTexture->unlockTbmBuffer();
}

void PlatformSurfaceTextureGL::HandleGLError(const char* name)
{
#if !USE(ACCELERATED_VIDEO_VAAPI)
    GLuint errorCode = glGetError();
    if (errorCode != GL_NO_ERROR) {
        TIZEN_LOGE("'%s' returned gl error (0x%x)", name, errorCode);
    }
#else
    UNUSED_PARAM(name);
#endif
}

void PlatformSurfaceTextureGL::HandleEGLError(const char* name)
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

};
#endif

