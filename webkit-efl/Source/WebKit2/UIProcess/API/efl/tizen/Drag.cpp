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

#include "config.h"

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "Drag.h"

#include "EwkView.h"
#include "ewk_view.h"
#include <WebCore/DragActions.h>
#include <WebCore/DragData.h>
#include <WebCore/DragSession.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

Drag::Drag(EwkView* ewkView)
      : m_ewkView(ewkView)
      , m_isDragMode(false)
      , m_dragStorageName("Drag")
      , m_dragData(0)
{
    ASSERT(ewkView);

    m_handle = new DragHandle(m_ewkView->evasObject(), EDJE_DIR"/Drag.edj", "drag_support", this);
}

Drag::~Drag()
{
    delete m_handle;
}

WebFrameProxy* Drag::focusedOrMainFrame()
{
    WebFrameProxy* frame = m_ewkView->page()->focusedFrame();
    if(!frame)
        frame = m_ewkView->page()->mainFrame();

    return frame;
}

void Drag::setDragMode(bool isDragMode)
{
    if (!isDragMode)
        hide();

    m_isDragMode = isDragMode;
}

void Drag::setDragData(WebCore::DragData* dragdata, PassRefPtr<ShareableBitmap> dragImage)
{
    m_dragData = dragdata;
    m_dragImage = dragImage;
}

void Drag::hide()
{
    IntPoint viewPoint = m_ewkView->transformFromScene().mapPoint(m_handle->position());

    DragData* dragData = new DragData(m_dragData->platformData(), viewPoint,
        viewPoint, m_dragData->draggingSourceOperationMask(), m_dragData->flags());

    m_ewkView->page()->dragExited(dragData ,m_dragStorageName);
    m_handle->hide();
    m_ewkView->page()->dragEnded(viewPoint, viewPoint, m_dragData->draggingSourceOperationMask());
}

void Drag::show()
{
    m_handle->show();
    m_handle->move(m_savePoint);
}

// handle callbacks
void Drag::handleMouseMove(DragHandle* handle)
{
    IntPoint viewPoint = m_ewkView->transformFromScene().mapPoint(handle->position());

    DragData* dragData = new DragData(m_dragData->platformData(), viewPoint,
        viewPoint, m_dragData->draggingSourceOperationMask(), m_dragData->flags());

    m_ewkView->page()->dragUpdated(dragData ,m_dragStorageName);

    DragSession dragSession = m_ewkView->page()->dragSession();
    if (dragSession.operation != DragOperationNone)
        handle->updateHandleIcon(true);
     else
        handle->updateHandleIcon(false);

    m_handle->move(handle->position());
    delete(dragData);
}

void Drag::handleMouseUp(DragHandle* handle)
{
    IntPoint viewPoint = m_ewkView->transformFromScene().mapPoint(handle->position());

    DragData* dragData = new DragData(m_dragData->platformData(), viewPoint,
        viewPoint, m_dragData->draggingSourceOperationMask(), m_dragData->flags());

    m_ewkView->page()->dragUpdated(dragData ,m_dragStorageName);

    DragSession dragSession = m_ewkView->page()->dragSession();
    if (dragSession.operation != DragOperationNone) {
        m_ewkView->page()->performDrag(dragData, m_dragStorageName);
        setDragMode(false);
        m_ewkView->page()->dragEnded(viewPoint, viewPoint, m_dragData->draggingSourceOperationMask());
    }
    delete(dragData);
}
} // namespace WebKit

#endif // TIZEN_DRAG_SUPPORT
