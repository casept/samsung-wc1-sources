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

#ifndef Drag_h
#define Drag_h

#if ENABLE(TIZEN_DRAG_SUPPORT)

#include "DragHandle.h"
#include "WebPageProxy.h"
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/PassOwnPtr.h>

class EwkView;

namespace WebKit {

class DragHandle;

class Drag {
public:
    static PassOwnPtr<Drag> create(EwkView* ewkView)
    {
        return adoptPtr(new Drag(ewkView));
    }
    ~Drag();

    bool isDragMode() { return m_isDragMode; }
    void setDragMode(bool isDragMode);
    void setDragData(WebCore::DragData* dragdata, PassRefPtr<ShareableBitmap> dragImage);
    void setDragPoint(const WebCore::IntPoint& point) { m_savePoint = point; }
    WebCore::IntPoint getDragPoint() { return m_savePoint; }
    PassRefPtr<ShareableBitmap> dragImage() { return m_dragImage; }
    EwkView* ewkView() { return m_ewkView; }

    // handle callback
    void handleMouseMove(DragHandle* handle);
    void handleMouseUp(DragHandle* handle);

    void hide();
    void show();

private:
    Drag(EwkView*);
    WebFrameProxy* focusedOrMainFrame();

    EwkView* m_ewkView;
    RefPtr<ShareableBitmap> m_dragImage;
    DragHandle* m_handle;

    bool m_isDragMode;
    String m_dragStorageName;
    WebCore::DragData* m_dragData;
    WebCore::IntPoint m_savePoint;
};

} // namespace WebKit

#endif // TIZEN_DRAG_SUPPORT
#endif // Drag_h
