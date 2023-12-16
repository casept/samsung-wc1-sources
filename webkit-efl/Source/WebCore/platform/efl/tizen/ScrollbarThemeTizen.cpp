/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollbarThemeTizen.h"

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
#include "Scrollbar.h"
#else
#include "NotImplemented.h"
#include "ScrollbarTheme.h"
#endif

namespace WebCore {

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    static ScrollbarThemeTizen theme;
    return &theme;
#else
    notImplemented();
    DEFINE_STATIC_LOCAL(ScrollbarTheme, theme, ());
    return &theme;
#endif
}

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

static const int cScrollbarThickness[] = { 8, 8 };

Color ScrollbarThemeTizen::m_thumbColor(102, 102, 102);
IntSize ScrollbarThemeTizen::m_thumbPositionOffset(2, 2);
IntSize ScrollbarThemeTizen::m_thumbSizeOffset(4, 4);

int ScrollbarThemeTizen::scrollbarThickness(ScrollbarControlSize controlSize)
{
    return cScrollbarThickness[controlSize];
}

IntRect ScrollbarThemeTizen::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    return scrollbar->frameRect();
}

void ScrollbarThemeTizen::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& thumbRect)
{
    if (scrollbar->enabled()) {
        IntRect paintRect = thumbRect;
        paintRect.contract(m_thumbSizeOffset);
        paintRect.move(m_thumbPositionOffset);

        context->fillRect(paintRect, m_thumbColor, ColorSpaceDeviceRGB);
    }
}

#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

} // namespace WebCore
