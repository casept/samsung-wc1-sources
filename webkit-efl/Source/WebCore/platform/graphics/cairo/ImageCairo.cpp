/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "Image.h"

#if USE(CAIRO)

#include "AffineTransform.h"
#include "CairoUtilities.h"
#include "Color.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "PlatformContextCairo.h"
#include <cairo.h>
#include <math.h>

#if ENABLE(TIZEN_JPEG_IMAGE_SCALE_DECODING)
#include "ImageSource.h"
#endif

namespace WebCore {

void Image::drawPattern(GraphicsContext* context, const FloatRect& tileRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, ColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode)
{
    RefPtr<cairo_surface_t> surface = nativeImageForCurrentFrame();
    if (!surface) // If it's too early we won't have an image yet.
        return;

#if ENABLE(TIZEN_JPEG_IMAGE_SCALE_DECODING)
    unsigned width = static_cast<unsigned>(size().width());
    unsigned height = static_cast<unsigned>(size().height());
    if (filenameExtension() == "jpg" && ((width * height) > ImageSource::maxPixelsPerDecodedImage())) {
        int surfaceWidth = cairo_image_surface_get_width(surface.get());
        int surfaceHeight = cairo_image_surface_get_height(surface.get());
        float scaleX = static_cast<float>(width) / static_cast<float>(surfaceWidth);
        float scaleY = static_cast<float>(height) / static_cast<float>(surfaceHeight);
        RefPtr<cairo_surface_t> expand_image = adoptRef(cairo_image_surface_create(cairo_image_surface_get_format(surface.get()), width, height));
        RefPtr<cairo_t> expand_cr = adoptRef(cairo_create(expand_image.get()));
        cairo_scale(expand_cr.get(), scaleX, scaleY);
        cairo_set_source_surface(expand_cr.get(), surface.get(), 0, 0);
        cairo_set_operator(expand_cr.get(), CAIRO_OPERATOR_SOURCE);
        cairo_paint(expand_cr.get());

        cairo_t* cr = context->platformContext()->cr();
        drawPatternToCairoContext(cr, expand_image.get(), size(), tileRect, patternTransform, phase, toCairoOperator(op), destRect);

        if (imageObserver())
            imageObserver()->didDraw(this);
        return;
    }
#endif

    cairo_t* cr = context->platformContext()->cr();
    drawPatternToCairoContext(cr, surface.get(), size(), tileRect, patternTransform, phase, toCairoOperator(op), destRect);

    if (imageObserver())
        imageObserver()->didDraw(this);
}

}

#endif // USE(CAIRO)
