/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(COORDINATED_GRAPHICS)

#include "ScrollingCoordinatorCoordinatedGraphics.h"

#include "CoordinatedGraphicsLayer.h"
#include "FrameView.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "ScrollingConstraints.h"
#include "ScrollingStateFixedNode.h"
#include "ScrollingStateScrollingNode.h"
#include "ScrollingStateStickyNode.h"
#include "ScrollingStateTree.h"
#include "Settings.h"
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
#include "Chrome.h"
#include "ChromeClient.h"
#include "ScrollThumbLayerCoordinator.h"
#endif
#if ENABLE(TIZEN_SCROLL_SNAP)
#include "RenderLayerCompositor.h"
#include "RenderView.h"
#endif

namespace WebCore {

ScrollingCoordinatorCoordinatedGraphics::ScrollingCoordinatorCoordinatedGraphics(Page* page)
    : ScrollingCoordinator(page)
    , m_scrollingStateTree(ScrollingStateTree::create())
{
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    m_scrollThumbLayerCoodinater = adoptPtr(new ScrollThumbLayerCoodinator);
#endif
}

ScrollingCoordinatorCoordinatedGraphics::~ScrollingCoordinatorCoordinatedGraphics()
{
}

ScrollingNodeID ScrollingCoordinatorCoordinatedGraphics::attachToStateTree(ScrollingNodeType nodeType, ScrollingNodeID newNodeID, ScrollingNodeID parentID)
{
    return m_scrollingStateTree->attachNode(nodeType, newNodeID, parentID);
}

void ScrollingCoordinatorCoordinatedGraphics::detachFromStateTree(ScrollingNodeID nodeID)
{
    ScrollingStateNode* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (node && node->isFixedNode())
        toCoordinatedGraphicsLayer(node->graphicsLayer())->setFixedToViewport(false);

    m_scrollingStateTree->detachNode(nodeID);
}

void ScrollingCoordinatorCoordinatedGraphics::clearStateTree()
{
    m_scrollingStateTree->clear();
}

void ScrollingCoordinatorCoordinatedGraphics::updateViewportConstrainedNode(ScrollingNodeID nodeID, const ViewportConstraints& constraints, GraphicsLayer* graphicsLayer)
{
    ASSERT(supportsFixedPositionLayers());

    ScrollingStateNode* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (!node)
        return;

    switch (constraints.constraintType()) {
    case ViewportConstraints::FixedPositionConstaint: {
        toCoordinatedGraphicsLayer(graphicsLayer)->setFixedToViewport(true); // FIXME : Use constraints!
        ScrollingStateFixedNode* fixedNode = toScrollingStateFixedNode(node);
        fixedNode->setScrollLayer(graphicsLayer);
        break;
    }
    case ViewportConstraints::StickyPositionConstraint:
        break; // FIXME : Support sticky elements.
    default:
        ASSERT_NOT_REACHED();
    }
}

void ScrollingCoordinatorCoordinatedGraphics::scrollableAreaScrollLayerDidChange(ScrollableArea* scrollableArea)
{
    CoordinatedGraphicsLayer* layer = toCoordinatedGraphicsLayer(scrollLayerForScrollableArea(scrollableArea));
    if (!layer)
        return;

    layer->setScrollableArea(scrollableArea);

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    if (CoordinatedGraphicsLayer* scrollbarLayer = toCoordinatedGraphicsLayer(horizontalScrollbarLayerForScrollableArea(scrollableArea)))
        layer->setHorizontalScrollbarLayer(scrollbarLayer->id());

    if (CoordinatedGraphicsLayer* scrollbarLayer = toCoordinatedGraphicsLayer(verticalScrollbarLayerForScrollableArea(scrollableArea)))
        layer->setVerticalScrollbarLayer(scrollbarLayer->id());

    notifyScrollLayerPositionChanged(scrollableArea);
#endif
}

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
void ScrollingCoordinatorCoordinatedGraphics::scrollableAreaScrollbarLayerDidChange(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, bool overflowLayer)
{
    if (!overflowLayer)
        return;

    GraphicsLayer* scrollbarLayer = 0;
    switch (orientation) {
    case HorizontalScrollbar:
        scrollbarLayer = horizontalScrollbarLayerForScrollableArea(scrollableArea);
        break;
    case VerticalScrollbar:
        scrollbarLayer = verticalScrollbarLayerForScrollableArea(scrollableArea);
        break;
    }

    if (scrollbarLayer) {
        GraphicsLayerFactory* graphicsLayerFactory = m_page->chrome().client()->graphicsLayerFactory();
        m_scrollThumbLayerCoodinater->createScrollThumbLayer(graphicsLayerFactory, scrollableArea, scrollbarLayer, orientation);
        scrollbarLayer->setDrawsContent(false);
        scrollbarLayer->setOpacity(0.f);
    }
    else
        m_scrollThumbLayerCoodinater->scrollBarLayerDestroyed(scrollableArea, orientation);
}

void ScrollingCoordinatorCoordinatedGraphics::notifyScrollLayerPositionChanged(ScrollableArea* scrollableArea)
{
    IntPoint mainScrollPosition = m_page->mainFrame()->view()->scrollPosition();
    if (horizontalScrollbarLayerForScrollableArea(scrollableArea))
        m_scrollThumbLayerCoodinater->updateScrollThumbLayer(scrollableArea, HorizontalScrollbar, mainScrollPosition);

    if (verticalScrollbarLayerForScrollableArea(scrollableArea))
        m_scrollThumbLayerCoodinater->updateScrollThumbLayer(scrollableArea, VerticalScrollbar, mainScrollPosition);
}
#endif // ENABLE(TIZEN_OVERFLOW_SCROLLBAR)

void ScrollingCoordinatorCoordinatedGraphics::willDestroyScrollableArea(ScrollableArea* scrollableArea)
{
    CoordinatedGraphicsLayer* layer = toCoordinatedGraphicsLayer(scrollLayerForScrollableArea(scrollableArea));
    if (!layer)
        return;

    layer->setScrollableArea(0);
}

#if ENABLE(TIZEN_SCROLL_SNAP)
void ScrollingCoordinatorCoordinatedGraphics::frameViewLayoutUpdated(FrameView* frameView)
{
    frameView->updateSnapOffsets();
    RenderView* renderView = frameView->frame()->contentRenderer();
    if (GraphicsLayer* rootLayer = renderView ? renderView->compositor()->rootGraphicsLayer() : 0) {
        rootLayer->setHorizontalSnapOffsets(frameView->horizontalSnapOffsets());
        rootLayer->setVerticalSnapOffsets(frameView->verticalSnapOffsets());
    }
}
#endif

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
