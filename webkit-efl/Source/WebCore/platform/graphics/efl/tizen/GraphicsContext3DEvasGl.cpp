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

#include "config.h"
#include "GraphicsContext3DEvasGl.h"

#include "OpenGLShims.h"

namespace WebCore {

#if ENABLE(TIZEN_DIRECT_RENDERING)

PassRefPtr<GraphicsContext3D> GraphicsContext3DEvasGl::createForCurrentGLContext()
{
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3DEvasGl(Attributes(), 0, GraphicsContext3D::RenderToCurrentGLContext));
    return context->m_private ? context.release() : 0;
}

GraphicsContext3DEvasGl::GraphicsContext3DEvasGl(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : GraphicsContext3D(attrs, hostWindow, renderStyle)
{
}

void GraphicsContext3DEvasGl::getIntegerv(GC3Denum pname, GC3Dint* value)
{
    if (pname == GraphicsContext3D::VIEWPORT) {
        for (int i = 0; i < 4; ++i)
            value[i] = m_viewport[i];
        return;
    }
    makeContextCurrent();
    ::glGetIntegerv(pname, value);
}

void GraphicsContext3DEvasGl::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    makeContextCurrent();
    // FIXME: remove the two glFlush calls when the driver bug is fixed, i.e.,
    // all previous rendering calls should be done before reading pixels.
    ::glFlush();

    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO) {
        resolveMultisamplingIfNecessary(IntRect(x, y, width, height));
        ::glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        ::glFlush();
    }

    ::glReadPixels(x, y, width, height, format, type, data);

    if (m_attrs.antialias && m_state.boundFBO == m_multisampleFBO)
        ::glBindFramebuffer(GL_FRAMEBUFFER, m_multisampleFBO);
}

void GraphicsContext3DEvasGl::readPixelsAndConvertToBGRAIfNecessary(int x, int y, int width, int height, unsigned char* pixels)
{
    makeContextCurrent();
    ::glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    static const int bytesPerPixel = 4;
    int totalBytes = width * height * bytesPerPixel;
    if (isGLES2Compliant()) {
        for (int i = 0; i < totalBytes; i += bytesPerPixel)
            std::swap(pixels[i], pixels[i + 2]); // Convert to BGRA.
    }
}

void GraphicsContext3DEvasGl::clear(GC3Dbitfield mask)
{
    makeContextCurrent();
    ::glClear(mask);
}

void GraphicsContext3DEvasGl::clearColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    makeContextCurrent();
    ::glClearColor(red, green, blue, alpha);
}

void GraphicsContext3DEvasGl::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    ::glViewport(x, y, width, height);

    m_viewport[0] = x;
    m_viewport[1] = y;
    m_viewport[2] = width;
    m_viewport[3] = height;
}

void GraphicsContext3DEvasGl::bindFramebuffer(GC3Denum target, Platform3DObject buffer)
{
    makeContextCurrent();
    GLuint fbo;
    if (buffer)
        fbo = buffer;
    else
        fbo = (m_attrs.antialias ? m_multisampleFBO : m_fbo);

    if (fbo != m_state.boundFBO) {
        ::glBindFramebuffer(target, fbo);
        m_state.boundFBO = fbo;
    }
}

void GraphicsContext3DEvasGl::scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    ::glScissor(x, y, width, height);
}

void GraphicsContext3DEvasGl::disable(GC3Denum cap)
{
    makeContextCurrent();
    ::glDisable(cap);
}

void GraphicsContext3DEvasGl::enable(GC3Denum cap)
{
    makeContextCurrent();
    ::glEnable(cap);
}

#endif // TIZEN_DIRECT_RENDERING

} // namespace WebCore
