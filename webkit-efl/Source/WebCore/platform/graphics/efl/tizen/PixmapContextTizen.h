/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef PixmapContextTizen_h
#define PixmapContextTizen_h

#if USE(TIZEN_PLATFORM_SURFACE)

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wtf/RefCounted.h>

namespace WebCore {
class PixmapContextTizen : public RefCounted<PixmapContextTizen> {
public:
    static PassRefPtr<PixmapContextTizen> create(bool isLockable, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples = false);
    virtual ~PixmapContextTizen();
    bool makeCurrent(EGLSurface);
    void* createSurface(int);
    void destroySurface(EGLSurface);
    bool lockSurface(EGLSurface);
    bool unlockSurface(EGLSurface);
    bool querySurface(EGLSurface, EGLint*);
    int createPixmap(int, int);
    void freePixmap(int);
    void* context() { return m_context; }
    void* display() { return m_display; }
    static void HandleEGLError(const char* name);

private:
    PixmapContextTizen(bool renderedByCPU, bool hasAlpha, bool hasDepth, bool hasStencil, bool hasSamples);
    virtual bool initialize();

protected:
    EGLConfig m_surfaceConfig;
    EGLDisplay m_display;
    EGLContext m_context;

    bool m_isRenderedByCPU;
    bool m_hasAlpha;
    bool m_hasDepth;
    bool m_hasStencil;
    bool m_hasSamples;
};
}

#endif //#if USE(TIZEN_PLATFORM_SURFACE)
#endif //#ifndef PixmapContextTizen_h
