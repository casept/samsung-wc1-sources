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

#ifndef GraphicsContext3DEvasGl_h
#define GraphicsContext3DEvasGl_h

#include "GraphicsContext3D.h"

namespace WebCore {

#if ENABLE(TIZEN_DIRECT_RENDERING)

class GraphicsContext3DEvasGl : public GraphicsContext3D {
public:
    static PassRefPtr<GraphicsContext3D> createForCurrentGLContext();

    virtual void getIntegerv(GC3Denum pname, GC3Dint* value);
    virtual void readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data);
    virtual void readPixelsAndConvertToBGRAIfNecessary(int x, int y, int width, int height, unsigned char* pixels);
    virtual void clear(GC3Dbitfield mask);
    virtual void clearColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha);
    virtual void viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    virtual void bindFramebuffer(GC3Denum target, Platform3DObject);
    virtual void scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    virtual void disable(GC3Denum cap);
    virtual void enable(GC3Denum cap);

private:
    GraphicsContext3DEvasGl(Attributes, HostWindow*, RenderStyle = RenderOffscreen);

    GC3Dint m_viewport[4];
};

#endif // TIZEN_DIRECT_RENDERING

} // namespace WebCore

#endif // GraphicsContext3DEvasGl_h
