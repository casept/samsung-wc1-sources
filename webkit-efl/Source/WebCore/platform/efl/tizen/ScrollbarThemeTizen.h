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

#ifndef ScrollbarThemeTizen_h
#define ScrollbarThemeTizen_h

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#include "ScrollbarThemeComposite.h"

namespace WebCore {

class ScrollbarThemeTizen : public ScrollbarThemeComposite {
public:
    virtual ~ScrollbarThemeTizen() {}

    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar);

    virtual bool usesOverlayScrollbars() const { return true; }

    virtual bool hasButtons(ScrollbarThemeClient*) { return false; }
    virtual bool hasThumb(ScrollbarThemeClient*) { return true; }

    virtual IntRect backButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool /*painting*/ = false) { return IntRect(); }
    virtual IntRect forwardButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool /*painting*/ = false) { return IntRect(); }
    virtual IntRect trackRect(ScrollbarThemeClient*, bool painting = false);

    virtual void paintThumb(GraphicsContext*, ScrollbarThemeClient*, const IntRect&);

    IntSize thumbPositionOffset() { return m_thumbPositionOffset; }
    IntSize thumbSizeOffset() { return m_thumbSizeOffset; }
    Color thumbColor() { return m_thumbColor; }

private:
    static Color m_thumbColor;
    static IntSize m_thumbPositionOffset;
    static IntSize m_thumbSizeOffset;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

#endif // ScrollbarThemeTizen_h
