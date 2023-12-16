/*
 * Copyright (C) 2014 Samsung Electronics
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

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)

#include "EdgeEffectController.h"

#include "BitmapImage.h"
#include "CoordinatedGraphicsScene.h"
#include "SharedBuffer.h"
#include "TextureMapper.h"
#include "TextureMapperLayer.h"
#include "Timer.h"

namespace WebCore {

class EdgeEffectAnimationController {
public:
    static PassOwnPtr<EdgeEffectAnimationController> create(EdgeEffectController* owner)
    {
        return adoptPtr(new EdgeEffectAnimationController(owner));
    }
    ~EdgeEffectAnimationController();

    void show(TextureMapperLayer*, TextureMapperLayer::EdgeEffectType, unsigned frameSize);
    void hide(bool withAnimation);
    void clear();
    TextureMapperLayer* currentLayer() { return m_currentLayer; }

private:
    explicit EdgeEffectAnimationController(EdgeEffectController*);

    void showingTimerFired(Timer<EdgeEffectAnimationController>*);
    void hidingTimerFired(Timer<EdgeEffectAnimationController>*);

    void updateEffect(BitmapTexture*);

#if ENABLE(TIZEN_WEARABLE_PROFILE)
    static constexpr double s_frameRate = 30;
#else
    static constexpr double s_frameRate = 60;
#endif

    EdgeEffectController* m_owner;
    Timer<EdgeEffectAnimationController> m_showingTimer;
    Timer<EdgeEffectAnimationController> m_hidingTimer;
    unsigned m_currentIndex;
    TextureMapperLayer* m_currentLayer;
    TextureMapperLayer::EdgeEffectType m_effectType;
    unsigned m_frameSize;
    bool m_increaseIndexInHidingTimer;
};

EdgeEffectAnimationController::EdgeEffectAnimationController(EdgeEffectController* owner)
    : m_owner(owner)
    , m_showingTimer(this, &EdgeEffectAnimationController::showingTimerFired)
    , m_hidingTimer(this, &EdgeEffectAnimationController::hidingTimerFired)
    , m_currentIndex(0)
    , m_currentLayer(0)
    , m_effectType(TextureMapperLayer::EdgeEffectTypeNone)
    , m_frameSize(0)
    , m_increaseIndexInHidingTimer(false)
{
}

EdgeEffectAnimationController::~EdgeEffectAnimationController()
{
    m_showingTimer.stop();
    m_hidingTimer.stop();
}

void EdgeEffectAnimationController::show(TextureMapperLayer* currentLayer, TextureMapperLayer::EdgeEffectType type, unsigned frameSize)
{
    if (frameSize <= 0)
        return;

    if (!currentLayer)
        return;

    m_currentLayer = currentLayer;
    m_effectType = type;
    m_frameSize = frameSize;

    if (!m_showingTimer.isActive() && !m_currentIndex)
        m_showingTimer.startRepeating(1 / s_frameRate);
}

void EdgeEffectAnimationController::hide(bool withAnimation)
{
    if (m_showingTimer.isActive())
        m_showingTimer.stop();

    if (!withAnimation) {
        if (m_hidingTimer.isActive())
            m_hidingTimer.stop();
        m_currentIndex = 0;
        updateEffect(0);
        m_effectType = TextureMapperLayer::EdgeEffectTypeNone;
        return;
    }

    if (!m_currentLayer)
        return;

    if (!m_hidingTimer.isActive() && m_currentIndex) {
        m_hidingTimer.startRepeating(1 / s_frameRate);
        if (m_currentIndex < m_frameSize - 1)
            m_increaseIndexInHidingTimer = true;
        else
            m_increaseIndexInHidingTimer = false;
    }
}

void EdgeEffectAnimationController::showingTimerFired(Timer<EdgeEffectAnimationController>* timer)
{
    if (m_currentIndex >= m_frameSize - 1) {
        timer->stop();
        return;
    }

    m_currentIndex++;
    updateEffect(m_owner->m_images[m_currentIndex].get());
    m_owner->updateViewport();
}

void EdgeEffectAnimationController::hidingTimerFired(Timer<EdgeEffectAnimationController>* timer)
{
    if (m_currentIndex <= 0) {
        m_currentIndex = 0;
        updateEffect(0);
        m_effectType = TextureMapperLayer::EdgeEffectTypeNone;
        timer->stop();
        return;
    }

    if (m_currentIndex >= m_frameSize - 1)
        m_increaseIndexInHidingTimer = false;

    if (m_increaseIndexInHidingTimer)
        m_currentIndex++;
    else
        m_currentIndex--;

    updateEffect(m_owner->m_images[m_currentIndex].get());
    m_owner->updateViewport();
}

void EdgeEffectAnimationController::updateEffect(BitmapTexture* texture)
{
    if (!m_currentLayer)
        return;

    switch (m_effectType) {
    case TextureMapperLayer::EdgeEffectTypeTop:
    case TextureMapperLayer::EdgeEffectTypeBottom:
        m_currentLayer->setVerticalEdgeEffectTexture(texture);
        m_currentLayer->setVerticalEdgeEffectType(m_effectType);
        break;
    case TextureMapperLayer::EdgeEffectTypeLeft:
    case TextureMapperLayer::EdgeEffectTypeRight:
        m_currentLayer->setHorizontalEdgeEffectTexture(texture);
        m_currentLayer->setHorizontalEdgeEffectType(m_effectType);
        break;
    default:
        break;
    }
}

void EdgeEffectAnimationController::clear()
{
    m_currentIndex = 0;
    m_currentLayer = 0;
    m_showingTimer.stop();
    m_hidingTimer.stop();
}

EdgeEffectController::EdgeEffectController(CoordinatedGraphicsScene* scene)
    : m_owner(scene)
    , m_verticalEffectAnimation(EdgeEffectAnimationController::create(this))
    , m_horizontalEffectAnimation(EdgeEffectAnimationController::create(this))
{
}

EdgeEffectController::~EdgeEffectController()
{
    m_images.clear();
}

void EdgeEffectController::loadImages(TextureMapper* textureMapper)
{
    if (!m_images.isEmpty())
        return;

    for (int i = 0; i < 10; ++i) {
        RefPtr<SharedBuffer> buffer = SharedBuffer::createWithContentsOfFile(makeString(IMAGE_DIR"bouncing_top_0", String::number(i), ".png"));
        if (!buffer || buffer->isEmpty())
            continue;

        RefPtr<BitmapImage> image = BitmapImage::create();
        image->setData(buffer.release(), true);
        RefPtr<BitmapTexture> texture = textureMapper->createTexture();
        texture->reset(image->size(), BitmapTexture::SupportsAlpha);
        texture->updateContents(image.get(), image->rect(), IntPoint::zero(), BitmapTexture::UpdateCannotModifyOriginalImageData);
        m_images.append(texture.release());
    }
}

void EdgeEffectController::updateViewport()
{
    m_owner->updateViewport();
}

void EdgeEffectController::showEdgeEffect(TextureMapperLayer* layer, TextureMapperLayer::EdgeEffectType type)
{
    if (!layer || !layer->textureMapper())
        return;

    loadImages(layer->textureMapper());
    if (m_images.isEmpty()) {
        TIZEN_LOGE("Fail to load edge effect images.");
        return;
    }

    switch (type) {
    case TextureMapperLayer::EdgeEffectTypeTop:
    case TextureMapperLayer::EdgeEffectTypeBottom:
        m_verticalEffectAnimation->show(layer, type, m_images.size());
        break;
    case TextureMapperLayer::EdgeEffectTypeLeft:
    case TextureMapperLayer::EdgeEffectTypeRight:
        m_horizontalEffectAnimation->show(layer, type, m_images.size());
        break;
    default:
        break;
    }
}

void EdgeEffectController::hideVerticalEdgeEffect(bool withAnimation)
{
    m_verticalEffectAnimation->hide(withAnimation);
}

void EdgeEffectController::hideHorizontalEdgeEffect(bool withAnimation)
{
    m_horizontalEffectAnimation->hide(withAnimation);
}

void EdgeEffectController::removeEffect(TextureMapperLayer* layer)
{
    if (m_horizontalEffectAnimation->currentLayer() == layer)
        m_horizontalEffectAnimation->clear();
    if (m_verticalEffectAnimation->currentLayer() == layer)
        m_verticalEffectAnimation->clear();
}

} // namespace WebCore

#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) && ENABLE(TIZEN_EDGE_EFFECT)
