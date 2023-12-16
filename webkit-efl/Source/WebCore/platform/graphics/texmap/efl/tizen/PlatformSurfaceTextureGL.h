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

#ifndef PlatformSurfaceTextureGL_h
#define PlatformSurfaceTextureGL_h

#if USE(TIZEN_PLATFORM_SURFACE)
#include "SharedPlatformSurfaceTizen.h"

#include "IntSize.h"
#include "OpenGLShims.h"
#include "TextureMapper.h"

namespace WebCore {
class SharedPlatformSurfaceSimpleTizen;
class PlatformSurfaceTextureGL : public PlatformSurfaceTexture {
public:
    ~PlatformSurfaceTextureGL();
    bool initialize(bool useLinearFilter);

    virtual uint32_t id() const { return m_id; }
    virtual int platformSurfaceId() const { return m_platformSurfaceID; }
    virtual void setUsed(bool used) { m_used = used; }
    virtual bool used() { return m_used; }
    virtual IntSize size() const { return m_textureSize; }
    virtual void updateTexture();
    void copyPlatformSurfaceToTextureGL(const IntRect&, SharedPlatformSurfaceSimpleTizen*);
    SharedPlatformSurfaceSimpleTizen* sharedPlatformSurfaceSimple() { return m_SharedPlatformSurfaceTizen.get(); }

private:
    PlatformSurfaceTextureGL(int platformSurfaceID, const IntSize& size);
    void HandleGLError(const char* name);
    void HandleEGLError(const char* name);
    void copyRect(unsigned char* dst, unsigned char* src, const IntRect& wholeRect, const IntRect& dirtyRect);

    void* m_eglImage;
    int m_platformSurfaceID;
    GLuint m_id;
    bool m_used;
    IntSize m_textureSize;
    OwnPtr<SharedPlatformSurfaceSimpleTizen> m_SharedPlatformSurfaceTizen;

    friend class TextureMapperGL;
};

}
#endif
#endif

