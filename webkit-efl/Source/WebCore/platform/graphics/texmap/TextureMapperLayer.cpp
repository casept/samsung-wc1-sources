/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TextureMapperLayer.h"

#include "FloatQuad.h"
#include "Region.h"
#include <wtf/MathExtras.h>

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

namespace WebCore {

class TextureMapperPaintOptions {
public:
    RefPtr<BitmapTexture> surface;
    float opacity;
    TransformationMatrix transform;
    IntSize offset;
    TextureMapper* textureMapper;
    TextureMapperPaintOptions()
        : opacity(1)
        , textureMapper(0)
    { }
};

#if ENABLE(TIZEN_CLEAR_BACKINGSTORE)
void TextureMapperLayer::clearBackingStore()
{
    m_backingStore.clear();
    if (m_state.maskLayer)
        m_state.maskLayer->clearBackingStore();
}
#endif

const TextureMapperLayer* TextureMapperLayer::rootLayer() const
{
    if (m_effectTarget)
        return m_effectTarget->rootLayer();
    if (m_parent)
        return m_parent->rootLayer();
    return this;
}

void TextureMapperLayer::computeTransformsRecursive()
{
    if (m_state.size.isEmpty() && m_state.masksToBounds)
        return;

    // Compute transforms recursively on the way down to leafs.
    TransformationMatrix parentTransform;
    if (m_parent)
        parentTransform = m_parent->m_currentTransform.combinedForChildren();
    else if (m_effectTarget)
        parentTransform = m_effectTarget->m_currentTransform.combined();
    m_currentTransform.combineTransforms(parentTransform);

    m_state.visible = m_state.backfaceVisibility || !m_currentTransform.combined().isBackFaceVisible();

    if (m_parent && m_parent->m_state.preserves3D)
        m_centerZ = m_currentTransform.combined().mapPoint(FloatPoint3D(m_state.size.width() / 2, m_state.size.height() / 2, 0)).z();

    if (m_state.maskLayer)
        m_state.maskLayer->computeTransformsRecursive();
    if (m_state.replicaLayer)
        m_state.replicaLayer->computeTransformsRecursive();
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->computeTransformsRecursive();

    // Reorder children if needed on the way back up.
    if (m_state.preserves3D)
        sortByZOrder(m_children);
}

void TextureMapperLayer::paint()
{
    computeTransformsRecursive();

    TextureMapperPaintOptions options;
    options.textureMapper = m_textureMapper;
    options.textureMapper->bindSurface(0);
    paintRecursive(options);
}

static Color blendWithOpacity(const Color& color, float opacity)
{
    RGBA32 rgba = color.rgb();
    // See Color::getRGBA() to know how to extract alpha from color.
    float alpha = alphaChannel(rgba) / 255.;
    float effectiveAlpha = alpha * opacity;
    return Color(colorWithOverrideAlpha(rgba, effectiveAlpha));
}

void TextureMapperLayer::computePatternTransformIfNeeded()
{
    if (!m_patternTransformDirty)
        return;

    m_patternTransformDirty = false;
    m_patternTransform =
        TransformationMatrix::rectToRect(FloatRect(FloatPoint::zero(), m_state.contentsTileSize), m_state.contentsRect)
        .multiply(TransformationMatrix().translate(m_state.contentsTilePhase.x() / m_state.contentsRect.width(), m_state.contentsTilePhase.y() / m_state.contentsRect.height()));
}

void TextureMapperLayer::paintSelf(const TextureMapperPaintOptions& options)
{
    if (!m_state.visible || !m_state.contentsVisible)
        return;

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    if (options.textureMapper->fullScreenForVideoMode()) {
        if (!childOfFullScreenLayerForVideoIncludingSelf())
            return;
    }
#endif

    // We apply the following transform to compensate for painting into a surface, and then apply the offset so that the painting fits in the target rect.
    TransformationMatrix transform;
    transform.translate(options.offset.width(), options.offset.height());
    transform.multiply(options.transform);
    transform.multiply(m_currentTransform.combined());

    if (m_state.solidColor.isValid() && !m_state.contentsRect.isEmpty() && m_state.solidColor.alpha()) {
        options.textureMapper->drawSolidColor(m_state.contentsRect, transform, blendWithOpacity(m_state.solidColor, options.opacity));
        if (m_state.showDebugBorders)
            options.textureMapper->drawBorder(m_state.debugBorderColor, m_state.debugBorderWidth, layerRect(), transform);
        return;
    }

    options.textureMapper->setWrapMode(TextureMapper::StretchWrap);
    options.textureMapper->setPatternTransform(TransformationMatrix());

    if (m_backingStore) {
        FloatRect targetRect = layerRect();
        ASSERT(!targetRect.isEmpty());
        m_backingStore->paintToTextureMapper(options.textureMapper, targetRect, transform, options.opacity);
        if (m_state.showDebugBorders)
            m_backingStore->drawBorder(options.textureMapper, m_state.debugBorderColor, m_state.debugBorderWidth, targetRect, transform);
        // Only draw repaint count for the main backing store.
        if (m_state.showRepaintCounter)
            m_backingStore->drawRepaintCounter(options.textureMapper, m_state.repaintCount, m_state.debugBorderColor, targetRect, transform);
    }

#if ENABLE(TIZEN_EDGE_EFFECT)
    paintEdgeEffectIfNeeded(options);
#endif

    if (!m_contentsLayer)
        return;

    if (!m_state.contentsTileSize.isEmpty()) {
        computePatternTransformIfNeeded();
        options.textureMapper->setWrapMode(TextureMapper::RepeatWrap);
        options.textureMapper->setPatternTransform(m_patternTransform);
    }

    ASSERT(!layerRect().isEmpty());
    m_contentsLayer->paintToTextureMapper(options.textureMapper, m_state.contentsRect, transform, options.opacity);
    if (m_state.showDebugBorders)
        m_contentsLayer->drawBorder(options.textureMapper, m_state.debugBorderColor, m_state.debugBorderWidth, m_state.contentsRect, transform);
}

int TextureMapperLayer::compareGraphicsLayersZValue(const void* a, const void* b)
{
    TextureMapperLayer* const* layerA = static_cast<TextureMapperLayer* const*>(a);
    TextureMapperLayer* const* layerB = static_cast<TextureMapperLayer* const*>(b);
    return int(((*layerA)->m_centerZ - (*layerB)->m_centerZ) * 1000);
}

void TextureMapperLayer::sortByZOrder(Vector<TextureMapperLayer* >& array)
{
    qsort(array.data(), array.size(), sizeof(TextureMapperLayer*), compareGraphicsLayersZValue);
}

void TextureMapperLayer::paintSelfAndChildren(const TextureMapperPaintOptions& options)
{
#if ENABLE(TIZEN_FIX_PRESERVE3D_LAYER_ORDER_ISSUE)
    bool selfLayerPainted = false;
    if (!m_state.preserves3D || m_children.isEmpty()) {
        paintSelf(options);
        selfLayerPainted = true;
    }
#else
    paintSelf(options);
#endif

    if (m_children.isEmpty())
        return;

    bool shouldClip = m_state.masksToBounds && !m_state.preserves3D;
    if (shouldClip) {
        TransformationMatrix clipTransform;
        clipTransform.translate(options.offset.width(), options.offset.height());
        clipTransform.multiply(options.transform);
        clipTransform.multiply(m_currentTransform.combined());
        options.textureMapper->beginClip(clipTransform, layerRect());
    }

    for (size_t i = 0; i < m_children.size(); ++i) {
#if ENABLE(TIZEN_FIX_PRESERVE3D_LAYER_ORDER_ISSUE)
        if (!selfLayerPainted && m_children[i]->m_centerZ >= m_centerZ) {
            paintSelf(options);
            selfLayerPainted = true;
        }
#endif
        m_children[i]->paintRecursive(options);
    }

#if ENABLE(TIZEN_FIX_PRESERVE3D_LAYER_ORDER_ISSUE)
    if (!selfLayerPainted)
        paintSelf(options);
#endif

    if (shouldClip)
        options.textureMapper->endClip();
}

bool TextureMapperLayer::shouldBlend() const
{
    if (m_state.preserves3D)
        return false;

    return m_currentOpacity < 1
        || hasFilters()
        || m_state.maskLayer
        || (m_state.replicaLayer && m_state.replicaLayer->m_state.maskLayer);
}

bool TextureMapperLayer::isVisible() const
{
    if (m_state.size.isEmpty() && (m_state.masksToBounds || m_state.maskLayer || m_children.isEmpty()))
        return false;
    if (!m_state.visible && m_children.isEmpty())
        return false;
    if (!m_state.contentsVisible && m_children.isEmpty())
        return false;
    if (m_currentOpacity < 0.01)
        return false;
    return true;
}

void TextureMapperLayer::paintSelfAndChildrenWithReplica(const TextureMapperPaintOptions& options)
{
    if (m_state.replicaLayer) {
        TextureMapperPaintOptions replicaOptions(options);
        replicaOptions.transform
            .multiply(m_state.replicaLayer->m_currentTransform.combined())
            .multiply(m_currentTransform.combined().inverse());
        paintSelfAndChildren(replicaOptions);
    }

    paintSelfAndChildren(options);
}

void TextureMapperLayer::setAnimatedTransform(const TransformationMatrix& matrix)
{
    m_currentTransform.setLocalTransform(matrix);
}

void TextureMapperLayer::setAnimatedOpacity(float opacity)
{
    m_currentOpacity = opacity;
}

TransformationMatrix TextureMapperLayer::replicaTransform()
{
    return TransformationMatrix(m_state.replicaLayer->m_currentTransform.combined()).multiply(m_currentTransform.combined().inverse());
}

#if ENABLE(CSS_FILTERS)
void TextureMapperLayer::setAnimatedFilters(const FilterOperations& filters)
{
    m_currentFilters = filters;
}
#endif

static void resolveOverlaps(Region newRegion, Region& overlapRegion, Region& nonOverlapRegion)
{
    Region newOverlapRegion(newRegion);
    newOverlapRegion.intersect(nonOverlapRegion);
    nonOverlapRegion.subtract(newOverlapRegion);
    overlapRegion.unite(newOverlapRegion);
    newRegion.subtract(overlapRegion);
    nonOverlapRegion.unite(newRegion);
}

void TextureMapperLayer::computeOverlapRegions(Region& overlapRegion, Region& nonOverlapRegion, ResolveSelfOverlapMode mode)
{
    if (!m_state.visible || !m_state.contentsVisible)
        return;

    FloatRect boundingRect;
    if (m_backingStore || m_state.masksToBounds || m_state.maskLayer || hasFilters())
        boundingRect = layerRect();
    else if (m_contentsLayer || m_state.solidColor.alpha())
        boundingRect = m_state.contentsRect;

#if ENABLE(CSS_FILTERS)
    if (m_currentFilters.hasOutsets()) {
        FilterOutsets outsets = m_currentFilters.outsets();
        IntRect unfilteredTargetRect(boundingRect);
        boundingRect.move(std::max(0, -outsets.left()), std::max(0, -outsets.top()));
        boundingRect.expand(outsets.left() + outsets.right(), outsets.top() + outsets.bottom());
        boundingRect.unite(unfilteredTargetRect);
    }
#endif

    TransformationMatrix replicaMatrix;
    if (m_state.replicaLayer) {
        replicaMatrix = replicaTransform();
        boundingRect.unite(replicaMatrix.mapRect(boundingRect));
    }

    boundingRect = m_currentTransform.combined().mapRect(boundingRect);

    // Count all masks and filters as overlap layers.
    if (hasFilters() || m_state.maskLayer || (m_state.replicaLayer && m_state.replicaLayer->m_state.maskLayer)) {
        Region newOverlapRegion(enclosingIntRect(boundingRect));
        nonOverlapRegion.subtract(newOverlapRegion);
        overlapRegion.unite(newOverlapRegion);
        return;
    }

    Region newOverlapRegion;
    Region newNonOverlapRegion(enclosingIntRect(boundingRect));

    if (!m_state.masksToBounds) {
        for (size_t i = 0; i < m_children.size(); ++i) {
            TextureMapperLayer* child = m_children[i];
            child->computeOverlapRegions(newOverlapRegion, newNonOverlapRegion, ResolveSelfOverlapIfNeeded);
        }
    }

    if (m_state.replicaLayer) {
        newOverlapRegion.unite(replicaMatrix.mapRect(newOverlapRegion.bounds()));
        Region replicaRegion(replicaMatrix.mapRect(newNonOverlapRegion.bounds()));
        resolveOverlaps(replicaRegion, newOverlapRegion, newNonOverlapRegion);
    }

    if ((mode != ResolveSelfOverlapAlways) && shouldBlend()) {
        newNonOverlapRegion.unite(newOverlapRegion);
        newOverlapRegion = Region();
    }

    overlapRegion.unite(newOverlapRegion);
    resolveOverlaps(newNonOverlapRegion, overlapRegion, nonOverlapRegion);
}

void TextureMapperLayer::paintUsingOverlapRegions(const TextureMapperPaintOptions& options)
{
    Region overlapRegion;
    Region nonOverlapRegion;
    computeOverlapRegions(overlapRegion, nonOverlapRegion, ResolveSelfOverlapAlways);
    if (overlapRegion.isEmpty()) {
        paintSelfAndChildrenWithReplica(options);
        return;
    }

    // Having both overlap and non-overlap regions carries some overhead. Avoid it if the overlap area
    // is big anyway.
    if (overlapRegion.bounds().size().area() > nonOverlapRegion.bounds().size().area()) {
        overlapRegion.unite(nonOverlapRegion);
        nonOverlapRegion = Region();
    }

    nonOverlapRegion.translate(options.offset);
    Vector<IntRect> rects = nonOverlapRegion.rects();

    for (size_t i = 0; i < rects.size(); ++i) {
        IntRect rect = rects[i];
        if (!rect.intersects(options.textureMapper->clipBounds()))
            continue;

        options.textureMapper->beginClip(TransformationMatrix(), rects[i]);
        paintSelfAndChildrenWithReplica(options);
        options.textureMapper->endClip();
    }

    rects = overlapRegion.rects();
    static const size_t OverlapRegionConsolidationThreshold = 4;
    if (nonOverlapRegion.isEmpty() && rects.size() > OverlapRegionConsolidationThreshold) {
        rects.clear();
        rects.append(overlapRegion.bounds());
    }

    IntSize maxTextureSize = options.textureMapper->maxTextureSize();
    IntRect adjustedClipBounds(options.textureMapper->clipBounds());
    adjustedClipBounds.move(-options.offset);
    for (size_t i = 0; i < rects.size(); ++i) {
        IntRect rect = rects[i];
        for (int x = rect.x(); x < rect.maxX(); x += maxTextureSize.width()) {
            for (int y = rect.y(); y < rect.maxY(); y += maxTextureSize.height()) {
                IntRect tileRect(IntPoint(x, y), maxTextureSize);
                tileRect.intersect(rect);
                if (!tileRect.intersects(adjustedClipBounds))
                    continue;

                paintWithIntermediateSurface(options, tileRect);
            }
        }
    }
}

void TextureMapperLayer::applyMask(const TextureMapperPaintOptions& options)
{
    options.textureMapper->setMaskMode(true);
    paintSelf(options);
    options.textureMapper->setMaskMode(false);
}

PassRefPtr<BitmapTexture> TextureMapperLayer::paintIntoSurface(const TextureMapperPaintOptions& options, const IntSize& size)
{
    RefPtr<BitmapTexture> surface = options.textureMapper->acquireTextureFromPool(size);
    TextureMapperPaintOptions paintOptions(options);
    paintOptions.surface = surface;
    options.textureMapper->bindSurface(surface.get());
    paintSelfAndChildren(paintOptions);
    if (m_state.maskLayer)
        m_state.maskLayer->applyMask(options);
#if ENABLE(CSS_FILTERS)
    surface = surface->applyFilters(options.textureMapper, m_currentFilters);
#endif
    options.textureMapper->bindSurface(surface.get());
    return surface;
}

static void commitSurface(const TextureMapperPaintOptions& options, PassRefPtr<BitmapTexture> surface, const IntRect& rect, float opacity)
{
    options.textureMapper->bindSurface(options.surface.get());
    TransformationMatrix targetTransform;
    targetTransform.translate(options.offset.width(), options.offset.height());
    targetTransform.multiply(options.transform);
    options.textureMapper->drawTexture(*surface.get(), rect, targetTransform, opacity);
}

void TextureMapperLayer::paintWithIntermediateSurface(const TextureMapperPaintOptions& options, const IntRect& rect)
{
    RefPtr<BitmapTexture> replicaSurface;
    RefPtr<BitmapTexture> mainSurface;
    TextureMapperPaintOptions paintOptions(options);
    paintOptions.offset = -IntSize(rect.x(), rect.y());
    paintOptions.opacity = 1;
    paintOptions.transform = TransformationMatrix();
    if (m_state.replicaLayer) {
        paintOptions.transform = replicaTransform();
        replicaSurface = paintIntoSurface(paintOptions, rect.size());
        paintOptions.transform = TransformationMatrix();
        if (m_state.replicaLayer->m_state.maskLayer)
            m_state.replicaLayer->m_state.maskLayer->applyMask(paintOptions);
    }

    if (replicaSurface && options.opacity == 1) {
        commitSurface(options, replicaSurface, rect, 1);
        replicaSurface.clear();
    }

    mainSurface = paintIntoSurface(paintOptions, rect.size());
    if (replicaSurface) {
        options.textureMapper->bindSurface(replicaSurface.get());
        options.textureMapper->drawTexture(*mainSurface.get(), FloatRect(FloatPoint::zero(), rect.size()));
        mainSurface = replicaSurface;
    }

    commitSurface(options, mainSurface, rect, options.opacity);
}

void TextureMapperLayer::paintRecursive(const TextureMapperPaintOptions& options)
{
    if (!isVisible())
        return;

    TextureMapperPaintOptions paintOptions(options);
    paintOptions.opacity *= m_currentOpacity;

    if (!shouldBlend()) {
        paintSelfAndChildrenWithReplica(paintOptions);
        return;
    }

    paintUsingOverlapRegions(paintOptions);
}

TextureMapperLayer::~TextureMapperLayer()
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        m_children[i]->m_parent = 0;

    if (m_parent)
        m_parent->m_children.remove(m_parent->m_children.find(this));
}

TextureMapper* TextureMapperLayer::textureMapper() const
{
    return rootLayer()->m_textureMapper;
}

void TextureMapperLayer::setChildren(const Vector<TextureMapperLayer*>& newChildren)
{
    removeAllChildren();
    for (size_t i = 0; i < newChildren.size(); ++i)
        addChild(newChildren[i]);
}

void TextureMapperLayer::addChild(TextureMapperLayer* childLayer)
{
    ASSERT(childLayer != this);

    if (childLayer->m_parent)
        childLayer->removeFromParent();

    childLayer->m_parent = this;
    m_children.append(childLayer);
}

void TextureMapperLayer::removeFromParent()
{
    if (m_parent) {
        unsigned i;
        for (i = 0; i < m_parent->m_children.size(); i++) {
            if (this == m_parent->m_children[i]) {
                m_parent->m_children.remove(i);
                break;
            }
        }

        m_parent = 0;
    }
}

void TextureMapperLayer::removeAllChildren()
{
    while (m_children.size()) {
        TextureMapperLayer* curLayer = m_children[0];
        ASSERT(curLayer->m_parent);
        curLayer->removeFromParent();
    }
}

void TextureMapperLayer::setMaskLayer(TextureMapperLayer* maskLayer)
{
    if (maskLayer)
        maskLayer->m_effectTarget = this;
    m_state.maskLayer = maskLayer;
}

void TextureMapperLayer::setReplicaLayer(TextureMapperLayer* replicaLayer)
{
    if (replicaLayer)
        replicaLayer->m_effectTarget = this;
    m_state.replicaLayer = replicaLayer;
}

void TextureMapperLayer::setPosition(const FloatPoint& position)
{
    m_state.pos = position;
    m_currentTransform.setPosition(adjustedPosition());
}

void TextureMapperLayer::setSize(const FloatSize& size)
{
    m_state.size = size;
    m_currentTransform.setSize(size);
}

void TextureMapperLayer::setAnchorPoint(const FloatPoint3D& anchorPoint)
{
    m_state.anchorPoint = anchorPoint;
    m_currentTransform.setAnchorPoint(anchorPoint);
}

void TextureMapperLayer::setPreserves3D(bool preserves3D)
{
    m_state.preserves3D = preserves3D;
    m_currentTransform.setFlattening(!preserves3D);
}

void TextureMapperLayer::setTransform(const TransformationMatrix& transform)
{
    m_state.transform = transform;
    m_currentTransform.setLocalTransform(transform);
}

void TextureMapperLayer::setChildrenTransform(const TransformationMatrix& childrenTransform)
{
    m_state.childrenTransform = childrenTransform;
    m_currentTransform.setChildrenTransform(childrenTransform);
}

void TextureMapperLayer::setContentsRect(const IntRect& contentsRect)
{
    if (contentsRect == m_state.contentsRect)
        return;
    m_state.contentsRect = contentsRect;
    m_patternTransformDirty = true;
}

void TextureMapperLayer::setContentsTileSize(const IntSize& size)
{
    if (size == m_state.contentsTileSize)
        return;
    m_state.contentsTileSize = size;
    m_patternTransformDirty = true;
}

void TextureMapperLayer::setContentsTilePhase(const IntPoint& phase)
{
    if (phase == m_state.contentsTilePhase)
        return;
    m_state.contentsTilePhase = phase;
    m_patternTransformDirty = true;
}

void TextureMapperLayer::setMasksToBounds(bool masksToBounds)
{
    m_state.masksToBounds = masksToBounds;
}

void TextureMapperLayer::setDrawsContent(bool drawsContent)
{
    m_state.drawsContent = drawsContent;
}

void TextureMapperLayer::setContentsVisible(bool contentsVisible)
{
    m_state.contentsVisible = contentsVisible;
}

void TextureMapperLayer::setContentsOpaque(bool contentsOpaque)
{
    m_state.contentsOpaque = contentsOpaque;
}

void TextureMapperLayer::setBackfaceVisibility(bool backfaceVisibility)
{
    m_state.backfaceVisibility = backfaceVisibility;
}

void TextureMapperLayer::setOpacity(float opacity)
{
    m_state.opacity = opacity;
}

void TextureMapperLayer::setSolidColor(const Color& color)
{
    m_state.solidColor = color;
}

#if ENABLE(CSS_FILTERS)
void TextureMapperLayer::setFilters(const FilterOperations& filters)
{
    m_state.filters = filters;
}
#endif

void TextureMapperLayer::setDebugVisuals(bool showDebugBorders, const Color& debugBorderColor, float debugBorderWidth, bool showRepaintCounter)
{
    m_state.showDebugBorders = showDebugBorders;
    m_state.debugBorderColor = debugBorderColor;
    m_state.debugBorderWidth = debugBorderWidth;
    m_state.showRepaintCounter = showRepaintCounter;
}

void TextureMapperLayer::setRepaintCount(int repaintCount)
{
    m_state.repaintCount = repaintCount;
}

void TextureMapperLayer::setContentsLayer(TextureMapperPlatformLayer* platformLayer)
{
    m_contentsLayer = platformLayer;
}

void TextureMapperLayer::setAnimations(const GraphicsLayerAnimations& animations)
{
    m_animations = animations;
}

void TextureMapperLayer::setFixedToViewport(bool fixedToViewport)
{
    m_fixedToViewport = fixedToViewport;
}

void TextureMapperLayer::setBackingStore(PassRefPtr<TextureMapperBackingStore> backingStore)
{
    m_backingStore = backingStore;
}

bool TextureMapperLayer::descendantsOrSelfHaveRunningAnimations() const
{
    if (m_animations.hasRunningAnimations())
        return true;

    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->descendantsOrSelfHaveRunningAnimations())
            return true;
    }

    return false;
}

void TextureMapperLayer::applyAnimationsRecursively()
{
    syncAnimations();
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->applyAnimationsRecursively();
}

void TextureMapperLayer::syncAnimations()
{
#if ENABLE(TIZEN_UI_SIDE_ANIMATION_SYNC)
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform)) {
        if (!hasStoppedAnimation(AnimatedPropertyWebkitTransform))
            m_currentTransform.setLocalTransform(m_state.transform);
    }
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyOpacity)) {
        if (!hasStoppedAnimation(AnimatedPropertyOpacity))
            m_currentOpacity = m_state.opacity;
    }
    m_animations.apply(this);
#else
    m_animations.apply(this);
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        m_currentTransform.setLocalTransform(m_state.transform);
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyOpacity))
        m_currentOpacity = m_state.opacity;
#endif
#if ENABLE(CSS_FILTERS)
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitFilter))
        m_currentFilters = m_state.filters;
#endif
}

bool TextureMapperLayer::isAncestorFixedToViewport() const
{
    for (TextureMapperLayer* parent = m_parent; parent; parent = parent->m_parent) {
        if (parent->m_fixedToViewport)
            return true;
    }

    return false;
}

void TextureMapperLayer::setScrollPositionDeltaIfNeeded(const FloatSize& delta)
{
    // delta is the difference between the scroll offset in the ui process and the scroll offset
    // in the web process. We add this delta to the position of fixed layers, to make
    // sure that they do not move while scrolling. We need to reset this delta to fixed layers
    // that have an ancestor which is also a fixed layer, because the delta will be added to the ancestor.
    if (isAncestorFixedToViewport())
        m_scrollPositionDelta = FloatSize();
    else
        m_scrollPositionDelta = delta;
    m_currentTransform.setPosition(adjustedPosition());
}

template<class HitTestCondition> TextureMapperLayer* TextureMapperLayer::hitTest(const FloatPoint& point, HitTestCondition condition)
{
    if (!m_state.visible || !m_state.contentsVisible)
        return 0;

    TextureMapperLayer* result = 0;
    for (int i = m_children.size() - 1; !result && i >= 0; --i)
        result = m_children[i]->hitTest(point, condition);

    if (result)
        return result;

    return condition(this, point) ? this : 0;
}

bool TextureMapperLayer::scrollableLayerHitTestCondition(TextureMapperLayer* layer, const FloatPoint& point)
{
    // scrolling layer's m_parent->m_parent, the parent of the scrolling layes, is the one that defines the
    // rectangle to be used for hit testing.
    if (!layer->isScrollable() || !layer->m_parent || !layer->m_parent->m_parent)
        return false;

    TextureMapperLayer* parentLayer = layer->m_parent->m_parent;
    FloatRect rect = parentLayer->layerRect();
    return parentLayer->m_currentTransform.combined().mapQuad(rect).containsPoint(point);
}

TextureMapperLayer* TextureMapperLayer::findScrollableContentsLayerAt(const FloatPoint& point)
{
    return hitTest(point, &TextureMapperLayer::scrollableLayerHitTestCondition);
}

FloatSize TextureMapperLayer::mapScrollOffset(const FloatSize& offset)
{
    double zeroX, zeroY, offsetX, offsetY;
    TransformationMatrix transform = m_currentTransform.combined().inverse();
    transform.map(0, 0, zeroX, zeroY);
    transform.map(offset.width(), offset.height(), offsetX, offsetY);
    return FloatSize(offsetX - zeroX, offsetY - zeroY);
}

void TextureMapperLayer::commitScrollOffset(const FloatSize& offset)
{
    FloatSize fullOffset = m_accumulatedScrollOffsetFractionalPart + offset;

    int intWidth = round(fullOffset.width());
    int intHeight = round(fullOffset.height());

    // m_accumulatedScrollOffsetFractionalPart holds the fractional part of the user scroll offset that
    // has not yet been synced with the web process because the web process expects an IntSize.
    m_accumulatedScrollOffsetFractionalPart = FloatSize(fullOffset.width() - intWidth, fullOffset.height() - intHeight);

    m_scrollClient->commitScrollOffset(m_id, IntSize(intWidth, intHeight));
}

void TextureMapperLayer::scrollBy(const FloatSize& offset)
{
    if (!isScrollable() || !m_scrollClient || offset.isZero())
        return;

    FloatSize scrollOffset = mapScrollOffset(offset);

#if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
    scrollOffset = boundScrollOffset(scrollOffset);
    if (scrollOffset.isZero())
        return;
#endif

    m_userScrollOffset += scrollOffset;

    m_currentTransform.setPosition(adjustedPosition());
    commitScrollOffset(scrollOffset);
}

#if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
FloatSize TextureMapperLayer::boundScrollOffset(const FloatSize& scrollOffset)
{
    FloatSize oldPosition(adjustedPosition().toFloatSize());
    FloatSize newPosition = oldPosition - scrollOffset;
    FloatSize scrollSize = size() - m_parent->size();
    FloatSize newScrollOffset;

    if (m_verticalScrollbarLayer) {
        newPosition.setHeight(std::max(std::min(newPosition.height(), 0.f), -scrollSize.height()));
        newScrollOffset.setHeight(oldPosition.height() - newPosition.height());
    }

    if (m_horizontalScrollbarLayer) {
        newPosition.setWidth(std::max(std::min(newPosition.width(), 0.f), -scrollSize.width()));
        newScrollOffset.setWidth(oldPosition.width() - newPosition.width());
    }

    return newScrollOffset;
}
#endif

#if ENABLE(TIZEN_EDGE_EFFECT)
void TextureMapperLayer::paintEdgeEffectIfNeeded(const TextureMapperPaintOptions& options)
{
    if (m_verticalEdgeEffectTexture) {
        FloatRect clipRect = options.textureMapper->clipBounds();
        TransformationMatrix transform;
        transform.translate(clipRect.x(), clipRect.y());
        FloatSize textureSize = m_verticalEdgeEffectTexture->size();

        switch (m_verticalEdgeEffectType) {
        case EdgeEffectTypeTop:
            transform.scale(clipRect.width() / textureSize.width());
            options.textureMapper->drawTexture(*m_verticalEdgeEffectTexture, FloatRect(FloatPoint(), textureSize), transform, 1.0, 0);
            break;
        case EdgeEffectTypeBottom:
            transform.translate(0, clipRect.height());
            transform.flipY();
            transform.scale(clipRect.width() / textureSize.width());
            options.textureMapper->drawTexture(*m_verticalEdgeEffectTexture, FloatRect(FloatPoint(), textureSize), transform, 1.0, 0);
            break;
        default:
            break;
        }
    }

    if (m_horizontalEdgeEffectTexture) {
        FloatRect clipRect = options.textureMapper->clipBounds();
        TransformationMatrix transform;
        transform.translate(clipRect.x(), clipRect.y());
        FloatSize textureSize = m_horizontalEdgeEffectTexture->size();

        switch (m_horizontalEdgeEffectType) {
        case EdgeEffectTypeLeft:
            transform.translate(0, clipRect.height());
            transform.rotate3d(0.0, 0.0, 1.0, -90);
            transform.scale(clipRect.height() / textureSize.width());
            options.textureMapper->drawTexture(*m_horizontalEdgeEffectTexture, FloatRect(FloatPoint(), textureSize), transform, 1.0, 0);
            break;
        case EdgeEffectTypeRight:
            transform.translate(clipRect.width(), 0);
            transform.rotate3d(0.0, 0.0, 1.0, 90);
            transform.scale(clipRect.height() / textureSize.width());
            options.textureMapper->drawTexture(*m_horizontalEdgeEffectTexture, FloatRect(FloatPoint(), textureSize), transform, 1.0, 0);
            break;
        default:
            break;
        }
    }
}
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
void TextureMapperLayer::collectScrollableLayerRecursively(Vector<TextureMapperLayer* >& scrollableLayers)
{
    if (m_isScrollable)
        scrollableLayers.append(this);

    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->collectScrollableLayerRecursively(scrollableLayers);
}
#endif

#if ENABLE(TIZEN_UI_SIDE_ANIMATION_SYNC)
bool TextureMapperLayer::hasStoppedAnimation(AnimatedPropertyID type)
{
    Vector<GraphicsLayerAnimation>& anims = m_animations.animations();
    for (size_t i = 0; anims.size() > i; ++i) {
        if (anims[i].property() == type && anims[i].state() != GraphicsLayerAnimation::PlayingState)
            return true;
    }
    return false;
}
#endif

void TextureMapperLayer::didCommitScrollOffset(const IntSize& offset)
{
    m_userScrollOffset = FloatSize(m_userScrollOffset.width() - offset.width(), m_userScrollOffset.height() - offset.height());
    m_currentTransform.setPosition(adjustedPosition());
}

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
void TextureMapperLayer::setFullScreenLayerForVideo(bool fullScreenLayerForVideo)
{
    m_state.fullScreenLayerForVideo = fullScreenLayerForVideo;
}

bool TextureMapperLayer::childOfFullScreenLayerForVideoIncludingSelf()
{
    if (m_state.fullScreenLayerForVideo)
        return true;

    return m_parent ? m_parent->childOfFullScreenLayerForVideoIncludingSelf() : false;
}

bool TextureMapperLayer::hasFullScreenLayerForVideoIncludingSelf()
{
    if (m_state.fullScreenLayerForVideo)
        return true;

    for (int i = 0; i < int(m_children.size()); ++i) {
        if (m_children[i]->hasFullScreenLayerForVideoIncludingSelf())
            return true;
    }

    return false;
}
#endif

#if ENABLE(TIZEN_SCROLL_SNAP)
void TextureMapperLayer::setHorizontalSnapOffsets(const Vector<float>& offsets)
{
    m_state.horizontalSnapOffsets = offsets;
}

void TextureMapperLayer::setVerticalSnapOffsets(const Vector<float>& offsets)
{
    m_state.verticalSnapOffsets = offsets;
}

static float calcScrollOffset(float scrollPosition, float offset, float scrollSize, float parentSize)
{
    if (offset < 0)
        return (scrollPosition < offset) ? offset - scrollPosition : offset;
    else {
        float newScrollPosition = scrollPosition + offset;
        float possibleScrollSize = scrollSize - parentSize;
        return (newScrollPosition > possibleScrollSize) ? newScrollPosition - possibleScrollSize : offset;
    }
}

static float findScrollPositionWithSnapOffsets(float scrollPosition, float scrollOffset, const Vector<float>& snapOffsets)
{
    size_t offsetsSize = snapOffsets.size();
    if (!offsetsSize)
        return scrollOffset;

    float newScrollPosition = scrollPosition + scrollOffset;
    size_t i;
    for (i = 0; i < offsetsSize; ++i) {
        if (snapOffsets[i] > newScrollPosition)
            break;
    }

    if (i == offsetsSize || (!scrollOffset && i && snapOffsets[i - 1] == scrollPosition))
        return scrollOffset;

    if (i == 0 || (scrollOffset > 0 && snapOffsets[i - 1] <= scrollPosition))
        return snapOffsets[i] - scrollPosition;
    else if ((scrollOffset < 0 && snapOffsets[i] >= scrollPosition) || (snapOffsets[i] - newScrollPosition > newScrollPosition - snapOffsets[i - 1]))
        return snapOffsets[i - 1] - scrollPosition;
    else
        return snapOffsets[i] - scrollPosition;
}

FloatSize unmapScrollOffset(GraphicsLayerTransform& currentTransform, const FloatSize& offset)
{
    double zeroX, zeroY, offsetX, offsetY;
    TransformationMatrix transform = currentTransform.combined();
    transform.map(0, 0, zeroX, zeroY);
    transform.map(offset.width(), offset.height(), offsetX, offsetY);
    return FloatSize(offsetX - zeroX, offsetY - zeroY);
}

IntSize TextureMapperLayer::findScrollSnapOffset(const IntSize& offset)
{
    ASSERT(isScrollable());
    FloatSize originalScrollOffset = mapScrollOffset(offset);
    FloatSize scrollOffset = originalScrollOffset;
    TextureMapperLayer* layer = this;
    FloatPoint scrollPosition;
    TextureMapperLayer* parentLayer = 0;

    while (layer && layer->m_parent && layer->m_parent->m_parent) {
        scrollPosition = -layer->adjustedPosition();
        FloatSize scrollSize = layer->m_state.size;
        parentLayer = layer->m_parent->m_parent;
        FloatSize parentSize = parentLayer->m_state.size;

        FloatSize availableScrollOffset(calcScrollOffset(scrollPosition.x(), scrollOffset.width(), scrollSize.width(), parentSize.width()), calcScrollOffset(scrollPosition.y(), scrollOffset.height(), scrollSize.height(), parentSize.height()));
        if (scrollOffset == availableScrollOffset)
            break;

        scrollOffset -= availableScrollOffset;
        layer = parentLayer->m_parent;
        while (layer && !layer->isScrollable())
            layer = layer->m_parent;
    }

    if (!layer || !parentLayer)
        return IntSize();

    FloatSize adjustedOffset(findScrollPositionWithSnapOffsets(scrollPosition.x(), scrollOffset.width(), parentLayer->m_state.horizontalSnapOffsets), findScrollPositionWithSnapOffsets(scrollPosition.y(), scrollOffset.height(), parentLayer->m_state.verticalSnapOffsets));
    if (adjustedOffset == scrollOffset)
        return IntSize();

    adjustedOffset += (originalScrollOffset - scrollOffset);
    adjustedOffset = unmapScrollOffset(m_currentTransform, adjustedOffset);
    return roundedIntSize(adjustedOffset);
}
#endif

}
#endif
