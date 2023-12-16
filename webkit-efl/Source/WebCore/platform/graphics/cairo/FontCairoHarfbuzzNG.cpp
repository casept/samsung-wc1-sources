/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Intel Corporation
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
#include "Font.h"

#include "GraphicsContext.h"
#include "HarfBuzzShaper.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "PlatformContextCairo.h"
#include "SimpleFontData.h"
#include <cairo.h>

namespace WebCore {

#if ENABLE(TIZEN_APPLY_COMPLEX_TEXT_ELLIPSIS)
void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
#else
void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int, int) const
#endif
{
    GlyphBuffer glyphBuffer;
    HarfBuzzShaper shaper(this, run);
#if ENABLE(TIZEN_APPLY_COMPLEX_TEXT_ELLIPSIS)
    shaper.setDrawRange(from, to);
#endif
    if (shaper.shape(&glyphBuffer))
        drawGlyphBuffer(context, run, glyphBuffer, point);
    else
        LOG_ERROR("Shaper couldn't shape glyphBuffer.");
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    notImplemented();
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>*, GlyphOverflow*) const
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
        return shaper.totalWidth();
    LOG_ERROR("Shaper couldn't shape text run.");
    return 0;
}

#if ENABLE(TIZEN_APPLY_COMPLEX_TEXT_ELLIPSIS)
int Font::offsetForPositionForComplexText(const TextRun& run, float x, bool, bool forEllipsis) const
#else
int Font::offsetForPositionForComplexText(const TextRun& run, float x, bool) const
#endif
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
#if ENABLE(TIZEN_APPLY_COMPLEX_TEXT_ELLIPSIS)
        return shaper.offsetForPosition(x, forEllipsis);
#else
        return shaper.offsetForPosition(x);
#endif
    LOG_ERROR("Shaper couldn't shape text run.");
    return 0;
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int h, int from, int to) const
{
    HarfBuzzShaper shaper(this, run);
    if (shaper.shape())
        return shaper.selectionRect(point, h, from, to);
    LOG_ERROR("Shaper couldn't shape text run.");
    return FloatRect();
}

}
