/*
 * Copyright (C) 2011 Andrew Wason (rectalogic@rectalogic.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#include "DrawingBuffer.h"

#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
#include "Canvas2DLayerTizen.h"
#endif

namespace WebCore {

#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)

DrawingBuffer::DrawingBuffer(GraphicsContext3D* context,
                             const IntSize&,
                             bool multisampleExtensionSupported,
                             bool packedDepthStencilExtensionSupported,
                             DrawingBuffer::PreserveDrawingBuffer,
                             DrawingBuffer::AlphaRequirement)
    : m_context(context)
    , m_size(-1, -1)
    , m_multisampleExtensionSupported(multisampleExtensionSupported)
    , m_packedDepthStencilExtensionSupported(packedDepthStencilExtensionSupported)
    , m_fbo(0)
    , m_colorBuffer(0)
    , m_depthStencilBuffer(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_multisampleFBO(0)
    , m_multisampleColorBuffer(0)
{
}

DrawingBuffer::~DrawingBuffer()
{
    clear();
}

#endif // ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* DrawingBuffer::platformLayer()
{
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    if (!m_platformLayer)
        m_platformLayer = Canvas2DLayerTizen::create(0);

    return static_cast<TextureMapperPlatformLayer*>(m_platformLayer.get());
#else
    return 0;
#endif // ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
}
#endif // USE(ACCELERATED_COMPOSITING)

} // WebCore


