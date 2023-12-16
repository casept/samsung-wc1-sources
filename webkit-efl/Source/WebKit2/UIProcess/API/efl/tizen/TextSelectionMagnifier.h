/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TextSelectionMagnifier_h
#define TextSelectionMagnifier_h

#if ENABLE(TIZEN_TEXT_SELECTION)

#include "EwkView.h"
#include "WebPageProxy.h"

#include <Evas.h>
#include <WebCore/IntPoint.h>

class EwkView;

namespace WebKit {

class TextSelectionMagnifier {
public:
    TextSelectionMagnifier(EwkView*);
    ~TextSelectionMagnifier();

    void show();
    void hide();
    void move(const WebCore::IntPoint& point);
    void update(const WebCore::IntPoint& point);
    bool isVisible();

private:
    EwkView* m_view;
    Evas_Object* m_image;
    Evas_Object* m_magnifier;
    int m_width;
    int m_height;
    int m_edjeWidth;
    int m_edjeHeight;
};

} // namespace WebKit

#endif // TIZEN_TEXT_SELECTION
#endif // TextSelectionMagnifier_h
