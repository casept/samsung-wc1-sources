/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2012 Company 100, Inc.

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

#if USE(COORDINATED_GRAPHICS)

#include "CoordinatedGraphicsScene.h"

#include "CoordinatedBackingStore.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperGL.h"
#include "TextureMapperLayer.h"
#include <OpenGLShims.h>
#include <wtf/Atomics.h>
#include <wtf/MainThread.h>

#if ENABLE(CSS_SHADERS)
#include "CoordinatedCustomFilterOperation.h"
#include "CoordinatedCustomFilterProgram.h"
#include "CustomFilterProgram.h"
#include "CustomFilterProgramInfo.h"
#endif

#if USE(TIZEN_PLATFORM_SURFACE)
#include "GraphicsLayerTextureMapper.h"
#include "CoordinatedBackingStoreTizen.h"
#endif
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
#include "ScrollbarFadeAnimationController.h"
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
#include "EdgeEffectController.h"
#endif

#if ENABLE(TIZEN_DIRECT_RENDERING)
#include <wtf/CurrentTime.h>
#if ENABLE(TIZEN_WEARABLE_PROFILE)
#define UPDATE_INTERVAL 0.0166
#else
#define UPDATE_INTERVAL 0.015
#endif
#endif

namespace WebCore {

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
#define OFFSET1 0.0f
#define OFFSET2 1.0f
#define OFFSET3 -1.0f
#define OFFSET4 2.0f

int widthFBO = 320;
int heightFBO = 320;
int widthMaxFBO = 360;
int heightMaxFBO = 360;

const int attribVertexPosition = 0;
const int attribTextureCoord = 1;

static const GLfloat vertices[] =
{
    -1.0f,  1.0f,  0.0f,
    -1.0f, -1.0f,  0.0f,
    1.0f, -1.0f,  0.0f,
    1.0f,  1.0f,  0.0f,
};

static const GLfloat texturesInverse[] = {
    OFFSET1, OFFSET2,
    OFFSET1, OFFSET1,
    OFFSET2, OFFSET1,
    OFFSET2, OFFSET2,
};

static const GLfloat textures3X3[] = {
    OFFSET3, OFFSET4,
    OFFSET3, OFFSET3,
    OFFSET4, OFFSET3,
    OFFSET4, OFFSET4,
};
static const int numIndices = 6;

static const GLshort indicies[] =
{
    0, 1, 2,
    2, 3, 0,
};

#endif
void CoordinatedGraphicsScene::dispatchOnMainThread(const Function<void()>& function)
{
    if (isMainThread())
        function();
    else
        callOnMainThread(function);
}

static bool layerShouldHaveBackingStore(TextureMapperLayer* layer)
{
    return layer->drawsContent() && layer->contentsAreVisible() && !layer->size().isEmpty();
}

CoordinatedGraphicsScene::CoordinatedGraphicsScene(CoordinatedGraphicsSceneClient* client)
    : m_client(client)
    , m_isActive(false)
    , m_rootLayerID(InvalidCoordinatedLayerID)
    , m_backgroundColor(Color::white)
    , m_viewBackgroundColor(Color::white)
    , m_setDrawsBackground(false)
#if ENABLE(TIZEN_DIRECT_RENDERING)
    , m_updateViewportTimer(RunLoop::current(), this, &CoordinatedGraphicsScene::updateViewportFired)
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    , m_touchedScrollableLayerID(InvalidCoordinatedLayerID)
#endif
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    , m_scrollbarFadeAnimationController(adoptPtr(new ScrollbarFadeAnimationController(*this)))
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    , m_edgeEffectController(adoptPtr(new EdgeEffectController(this)))
#endif
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
     , m_initShaderAndFBO(0)
     , m_initShaderForBlurEffect(0)
     , m_fboTexture(0)
     , m_fboDepth(0)
     , m_fbo(0)
     , m_program(0)
#endif
{
    ASSERT(isMainThread());
}

CoordinatedGraphicsScene::~CoordinatedGraphicsScene()
{
}

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
static void initMatrix(float matrix[16])
{
    matrix[0] = 1.0f;
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;

    matrix[4] = 0.0f;
    matrix[5] = 1.0f;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;

    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = 1.0f;
    matrix[11] = 0.0f;

    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;

}

static void multiplyMatrix(float matrix[16], const float matrix0[16], const float matrix1[16])
{
    int i;
    int row;
    int column;
    float temp[16];

    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            temp[column * 4 + row] = 0.0f;
            for (i = 0; i < 4; i++)
                temp[column * 4 + row] += matrix0[i * 4 + row] * matrix1[column * 4 + i];
        }
    }

    for (i = 0; i < 16; i++)
        matrix[i] = temp[i];
}

int CoordinatedGraphicsScene::initBlurShaders(BlurEffect* b)
{
    if (b == (BlurEffect*)0) {
        return 0;
    }

    const char vertexShader[] =
        "uniform mat4 mvp_matrix;\n"
        "attribute vec4 vertPosition;\n"
        "attribute vec4 texCoord;\n"
        "varying vec2 v_coord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = mvp_matrix * vertPosition;\n"
        "    v_coord = texCoord.xy;\n"
        "}";

    const char fragmentShader[] =
        "precision mediump float;\n"
        "uniform sampler2D u_texture;\n"
        "varying vec2 v_coord;\n"
        "uniform float bias;\n"
        "void main()\n"
        "{\n"
        "    vec3 color = texture2D(u_texture, v_coord, bias).rgb;"
        "    gl_FragColor = vec4(color, 1.0);\n"
        "}";

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* p = vertexShader;
    glShaderSource(vertShader, 1, &p, NULL);
    glCompileShader(vertShader);

    GLint isCompiled = GL_FALSE;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &isCompiled);

    if(isCompiled == GL_FALSE) {
        TIZEN_LOGI("vertex shader is compile failed. err = %#x", isCompiled);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        return 0;
    }

    p = fragmentShader;
    glShaderSource(fragShader, 1, &p, NULL);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE) {
        TIZEN_LOGI("fragment shader is compile failed. err = %#x", isCompiled);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        return 0;
    }

    b->program = glCreateProgram();
    glAttachShader(b->program, vertShader);
    glAttachShader(b->program, fragShader);
    glLinkProgram(b->program);

    b->vertPosition = glGetAttribLocation(b->program, "vertPosition");
    b->texCoord = glGetAttribLocation(b->program, "texCoord");
    glUseProgram(b->program);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(b->program, "u_texture"), 0);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return 1;
}

void CoordinatedGraphicsScene::initShaderForEffect(BlurEffect* b, int w, int h)
{
    if (initBlurShaders(b)) {
        float view[16];
        initMatrix(view);
        view[0] = 320.0f / w * 3.0;
        view[5] = (float)w / h * view[0];
        initMatrix(b->mvpRatio);
        multiplyMatrix(b->mvpRatio, view, b->mvpRatio);
        b->bias = 4.0f;
        glUniformMatrix4fv(glGetUniformLocation(b->program, "mvp_matrix"), 1, GL_FALSE, b->mvpRatio);
        glUniform1f(glGetUniformLocation(b->program, "bias"), b->bias);
    }
}

void CoordinatedGraphicsScene::drawBlur(BlurEffect* b, int w, int h)
{
    glUseProgram(b->program);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glEnableVertexAttribArray(b->vertPosition);
    glEnableVertexAttribArray(b->texCoord);
    glUniform1f(glGetUniformLocation(b->program, "bias"), b->bias);
    glVertexAttribPointer(b->vertPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), vertices);
    glVertexAttribPointer(b->texCoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT), textures3X3);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indicies);
    glDisableVertexAttribArray(b->vertPosition);
    glDisableVertexAttribArray(b->texCoord);
}

void CoordinatedGraphicsScene::drawToSurface(int w, int h)
{
    static float mvp[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
    };

    mvp[0] = 320.0f / w;
    mvp[5] = 320.0f / h;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(m_program);
    glActiveTexture(GL_TEXTURE0);
    glViewport(0, 0, w, h);
    glEnableVertexAttribArray(attribVertexPosition);
    glEnableVertexAttribArray(attribTextureCoord);
    glVertexAttribPointer(attribVertexPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), vertices);
    glVertexAttribPointer(attribTextureCoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT), texturesInverse);
    glUniformMatrix4fv(glGetUniformLocation(m_program, "mvpMatrix"), 1, GL_FALSE, mvp);
    glUniform1i(glGetUniformLocation(m_program, "uTexture"), 0);
    glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, indicies);
    glDisableVertexAttribArray(attribVertexPosition);
    glDisableVertexAttribArray(attribTextureCoord);
    glFlush();
}

void CoordinatedGraphicsScene::initializeShaders()
{
    const char vertexShader[] =
        "uniform mat4 mvpMatrix;\n"
        "attribute vec4 aVertexPosition;\n"
        "attribute vec4 aTextureCoord;\n"
        "varying vec2 vCoordinates;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = mvpMatrix * aVertexPosition;\n"
        "    vCoordinates = aTextureCoord.xy;\n"
        "}";

    const char fragmentShader[] =
        "precision mediump float;\n"
        "uniform sampler2D uTexture;\n"
        "varying vec2 vCoordinates;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(uTexture, vCoordinates);\n"
        "}";

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* p = vertexShader;
    glShaderSource(vertShader, 1, &p, NULL);
    glCompileShader(vertShader);

    GLint bShaderCompiled = GL_FALSE;
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &bShaderCompiled);

    if(bShaderCompiled == GL_FALSE) {
        TIZEN_LOGI("vertex shader is compile failed. err = %#x", bShaderCompiled);
    }

    p = fragmentShader;
    glShaderSource(fragShader, 1, &p, NULL);
    glCompileShader(fragShader);
    bShaderCompiled = GL_FALSE;
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &bShaderCompiled);
    if(bShaderCompiled == GL_FALSE) {
        TIZEN_LOGI("fragment shader is compile failed. err = %#x", bShaderCompiled);
    }
    m_program = glCreateProgram();
    glAttachShader(m_program, vertShader);
    glAttachShader(m_program, fragShader);
    glBindAttribLocation(m_program, attribVertexPosition, "aVertexPosition");
    glBindAttribLocation(m_program, attribTextureCoord, "aTextureCoord");
    glLinkProgram(m_program);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

void CoordinatedGraphicsScene::initializeFBO(int width, int height)
{
    glGenTextures(1, &m_fboTexture);
    glBindTexture(GL_TEXTURE_2D, m_fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glGenRenderbuffers(1, &m_fboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fboTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_fboDepth);
    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        TIZEN_LOGI("opengl %s %d fbo status wrong %#x %#x\n", __FUNCTION__, __LINE__, status, glGetError());
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void CoordinatedGraphicsScene::initializeForEffect()
{
    if (m_initShaderAndFBO == 0) {
        initializeShaders();
        initializeFBO(widthFBO, heightFBO);
        m_initShaderAndFBO = 1;
    }

    if (m_initShaderForBlurEffect == 0) {
        initShaderForEffect(&m_blur, widthMaxFBO, heightMaxFBO);
        m_initShaderForBlurEffect = 1;
    }
}

void CoordinatedGraphicsScene::paintToBackupTextureEffect()
{
    if (m_initShaderAndFBO) {
        glBindTexture(GL_TEXTURE_2D, m_fboTexture);
        glGenerateMipmap(GL_TEXTURE_2D);
        drawBlur(&m_blur, widthMaxFBO, heightMaxFBO);
        glBindTexture(GL_TEXTURE_2D, m_fboTexture);
        drawToSurface(widthMaxFBO, heightMaxFBO);
    }
}

void CoordinatedGraphicsScene::clearBlurEffectResources()
{
    if (m_initShaderAndFBO && m_client->makeContextCurrent()) {
        glDeleteTextures(1, &m_fboTexture);
        glDeleteRenderbuffers(1, &m_fboDepth);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &m_fbo);
        m_initShaderAndFBO = 0;
    }
}
#endif

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
void CoordinatedGraphicsScene::paintToCurrentGLContext(const TransformationMatrix& matrix, float opacity, const FloatRect& clipRect, TextureMapper::PaintFlags PaintFlags, bool needEffect)
#else
void CoordinatedGraphicsScene::paintToCurrentGLContext(const TransformationMatrix& matrix, float opacity, const FloatRect& clipRect, TextureMapper::PaintFlags PaintFlags)
#endif
{
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    if (needEffect) {
        initializeForEffect();
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }
#endif
    if (!m_textureMapper) {
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
#if ENABLE(TIZEN_TEXTURE_MAPPER_TEXTURE_POOL_MAKE_CONTEXT_CURRENT)
        m_textureMapper->setCoordinatedGraphicsScene(this);
#endif
        static_cast<TextureMapperGL*>(m_textureMapper.get())->setEnableEdgeDistanceAntialiasing(true);
    }

    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);
    syncRemoteContent();

    adjustPositionForFixedLayers();
    TextureMapperLayer* currentRootLayer = rootLayer();
    if (!currentRootLayer) {
#if ENABLE(TIZEN_SET_INITIAL_COLOR_OF_WEBVIEW_EVAS_IMAGE)
        GraphicsContext3D* context = static_cast<TextureMapperGL*>(m_textureMapper.get())->graphicsContext3D();
        context->clearColor(m_backgroundColor.red() / 255.0f, m_backgroundColor.green() / 255.0f, m_backgroundColor.blue() / 255.0f, m_backgroundColor.alpha() / 255.0f);
        context->clear(GL_COLOR_BUFFER_BIT);
#endif
        return;
    }

#if ENABLE(TIZEN_DIRECT_RENDERING)
    static_cast<TextureMapperGL*>(m_textureMapper.get())->graphicsContext3D()->viewport(0, 0, static_cast<int>(clipRect.width()), static_cast<int>(clipRect.height()));
    static double startTime = 0;
    startTime = currentTime();
#endif

    currentRootLayer->setTextureMapper(m_textureMapper.get());
    currentRootLayer->applyAnimationsRecursively();
    m_textureMapper->beginPainting(PaintFlags);
    m_textureMapper->beginClip(TransformationMatrix(), clipRect);

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    m_textureMapper->setFullScreenForVideoMode(m_client->fullScreenForVideoMode());
    if (m_textureMapper->fullScreenForVideoMode()) {
        GraphicsContext3D* context = static_cast<TextureMapperGL*>(m_textureMapper.get())->graphicsContext3D();
        context->clearColor(0, 0, 0, 0);
        context->clear(GL_COLOR_BUFFER_BIT);
    }
    else
#endif
    {
        if (m_setDrawsBackground) {
            RGBA32 rgba = makeRGBA32FromFloats(m_backgroundColor.red(),
                m_backgroundColor.green(), m_backgroundColor.blue(),
                m_backgroundColor.alpha() * opacity);
            m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), Color(rgba));
        } else {
            GraphicsContext3D* context = static_cast<TextureMapperGL*>(m_textureMapper.get())->graphicsContext3D();
            context->clearColor(m_viewBackgroundColor.red() / 255.0f, m_viewBackgroundColor.green() / 255.0f, m_viewBackgroundColor.blue() / 255.0f, m_viewBackgroundColor.alpha() / 255.0f);
            context->clear(GL_COLOR_BUFFER_BIT);
        }
    }

    if (currentRootLayer->opacity() != opacity || currentRootLayer->transform() != matrix) {
        currentRootLayer->setOpacity(opacity);
        currentRootLayer->setTransform(matrix);
    }

    currentRootLayer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), clipRect.location(), matrix);
    m_textureMapper->endClip();
    m_textureMapper->endPainting();
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    m_textureMapper->setFullScreenForVideoMode(false);
#endif

    if (currentRootLayer->descendantsOrSelfHaveRunningAnimations()) {
#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
        if (!m_client->uiSideAnimationEnabled())
            return;
#endif

#if ENABLE(TIZEN_DIRECT_RENDERING)
        double paintingTime = currentTime() - startTime;
        m_updateViewportTimer.startOneShot(paintingTime > UPDATE_INTERVAL ? 0 : UPDATE_INTERVAL - paintingTime);
#else
        dispatchOnMainThread(bind(&CoordinatedGraphicsScene::updateViewport, this));
#endif
    }
}

void CoordinatedGraphicsScene::paintToGraphicsContext(PlatformGraphicsContext* platformContext)
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create();
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::SoftwareMode);
    syncRemoteContent();
    TextureMapperLayer* layer = rootLayer();

    if (!layer)
        return;

    GraphicsContext graphicsContext(platformContext);
    m_textureMapper->setGraphicsContext(&graphicsContext);
    m_textureMapper->beginPainting();

    IntRect clipRect = graphicsContext.clipBounds();
    if (m_setDrawsBackground)
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_backgroundColor);
#if PLATFORM(TIZEN)
    else {
        graphicsContext.clearRect(clipRect);
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_viewBackgroundColor);
    }
#else
    else
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_viewBackgroundColor);
#endif

    layer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), clipRect.location());
    m_textureMapper->endPainting();
    m_textureMapper->setGraphicsContext(0);
}

void CoordinatedGraphicsScene::setScrollPosition(const FloatPoint& scrollPosition)
{
    m_scrollPosition = scrollPosition;
}

void CoordinatedGraphicsScene::updateViewport()
{
    ASSERT(isMainThread());
    if (m_client)
        m_client->updateViewport();
}

void CoordinatedGraphicsScene::adjustPositionForFixedLayers()
{
    if (m_fixedLayers.isEmpty())
        return;

    // Fixed layer positions are updated by the web process when we update the visible contents rect / scroll position.
    // If we want those layers to follow accurately the viewport when we move between the web process updates, we have to offset
    // them by the delta between the current position and the position of the viewport used for the last layout.
    FloatSize delta = m_scrollPosition - m_renderedContentsScrollPosition;

    LayerRawPtrMap::iterator end = m_fixedLayers.end();
    for (LayerRawPtrMap::iterator it = m_fixedLayers.begin(); it != end; ++it)
        it->value->setScrollPositionDeltaIfNeeded(delta);
}

#if USE(GRAPHICS_SURFACE)
void CoordinatedGraphicsScene::createCanvasIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.canvasToken.isValid())
        return;

    RefPtr<TextureMapperSurfaceBackingStore> canvasBackingStore(TextureMapperSurfaceBackingStore::create());
    m_surfaceBackingStores.set(layer, canvasBackingStore);
    canvasBackingStore->setGraphicsSurface(GraphicsSurface::create(state.canvasSize, state.canvasSurfaceFlags, state.canvasToken));
    layer->setContentsLayer(canvasBackingStore.get());
}

void CoordinatedGraphicsScene::syncCanvasIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    ASSERT(m_textureMapper);

    if (state.canvasChanged) {
        destroyCanvasIfNeeded(layer, state);
        createCanvasIfNeeded(layer, state);
    }

    if (state.canvasShouldSwapBuffers) {
        ASSERT(m_surfaceBackingStores.contains(layer));
        SurfaceBackingStoreMap::iterator it = m_surfaceBackingStores.find(layer);
        RefPtr<TextureMapperSurfaceBackingStore> canvasBackingStore = it->value;
        canvasBackingStore->swapBuffersIfNeeded(state.canvasFrontBuffer);
    }
}

void CoordinatedGraphicsScene::destroyCanvasIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.canvasToken.isValid())
        return;

    m_surfaceBackingStores.remove(layer);
    layer->setContentsLayer(0);
}
#endif

void CoordinatedGraphicsScene::setLayerRepaintCountIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!layer->isShowingRepaintCounter() || !state.repaintCountChanged)
        return;

    layer->setRepaintCount(state.repaintCount);
}

void CoordinatedGraphicsScene::setLayerChildrenIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.childrenChanged)
        return;

    Vector<TextureMapperLayer*> children;

    for (size_t i = 0; i < state.children.size(); ++i) {
        CoordinatedLayerID childID = state.children[i];
        TextureMapperLayer* child = layerByID(childID);
        children.append(child);
    }
    layer->setChildren(children);
}

#if ENABLE(CSS_FILTERS)
void CoordinatedGraphicsScene::setLayerFiltersIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.filtersChanged)
        return;

#if ENABLE(CSS_SHADERS)
    injectCachedCustomFilterPrograms(state.filters);
#endif
    layer->setFilters(state.filters);
}
#endif

#if ENABLE(CSS_SHADERS)
void CoordinatedGraphicsScene::syncCustomFilterPrograms(const CoordinatedGraphicsState& state)
{
    for (size_t i = 0; i < state.customFiltersToCreate.size(); ++i)
        createCustomFilterProgram(state.customFiltersToCreate[i].first, state.customFiltersToCreate[i].second);

    for (size_t i = 0; i < state.customFiltersToRemove.size(); ++i)
        removeCustomFilterProgram(state.customFiltersToRemove[i]);
}

void CoordinatedGraphicsScene::injectCachedCustomFilterPrograms(const FilterOperations& filters) const
{
    for (size_t i = 0; i < filters.size(); ++i) {
        FilterOperation* operation = filters.operations().at(i).get();
        if (operation->getOperationType() != FilterOperation::CUSTOM)
            continue;

        CoordinatedCustomFilterOperation* customOperation = static_cast<CoordinatedCustomFilterOperation*>(operation);
        ASSERT(!customOperation->program());
        CustomFilterProgramMap::const_iterator iter = m_customFilterPrograms.find(customOperation->programID());
        ASSERT(iter != m_customFilterPrograms.end());
        customOperation->setProgram(iter->value.get());
    }
}

void CoordinatedGraphicsScene::createCustomFilterProgram(int id, const CustomFilterProgramInfo& programInfo)
{
    ASSERT(!m_customFilterPrograms.contains(id));
    m_customFilterPrograms.set(id, CoordinatedCustomFilterProgram::create(programInfo.vertexShaderString(), programInfo.fragmentShaderString(), programInfo.programType(), programInfo.mixSettings(), programInfo.meshType()));
}

void CoordinatedGraphicsScene::removeCustomFilterProgram(int id)
{
    CustomFilterProgramMap::iterator iter = m_customFilterPrograms.find(id);
    ASSERT(iter != m_customFilterPrograms.end());
    if (m_textureMapper)
        m_textureMapper->removeCachedCustomFilterProgram(iter->value.get());
    m_customFilterPrograms.remove(iter);
}
#endif // ENABLE(CSS_SHADERS)

void CoordinatedGraphicsScene::setLayerState(CoordinatedLayerID id, const CoordinatedGraphicsLayerState& layerState)
{
    ASSERT(m_rootLayerID != InvalidCoordinatedLayerID);
    TextureMapperLayer* layer = layerByID(id);

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    if (layerState.positionChanged) {
        layer->setPosition(layerState.pos);
        if (layer->horizontalScrollbarLayer() || layer->verticalScrollbarLayer())
            m_scrollbarFadeAnimationController->showOverflowScollbar(layer);
    }
#else
    if (layerState.positionChanged)
        layer->setPosition(layerState.pos);
#endif

    if (layerState.anchorPointChanged)
        layer->setAnchorPoint(layerState.anchorPoint);

    if (layerState.sizeChanged)
        layer->setSize(layerState.size);

    if (layerState.transformChanged)
        layer->setTransform(layerState.transform);

    if (layerState.childrenTransformChanged)
        layer->setChildrenTransform(layerState.childrenTransform);

    if (layerState.contentsRectChanged)
        layer->setContentsRect(layerState.contentsRect);

    if (layerState.contentsTilingChanged) {
        layer->setContentsTilePhase(layerState.contentsTilePhase);
        layer->setContentsTileSize(layerState.contentsTileSize);
    }

    if (layerState.opacityChanged)
        layer->setOpacity(layerState.opacity);

    if (layerState.solidColorChanged)
        layer->setSolidColor(layerState.solidColor);

    if (layerState.debugBorderColorChanged || layerState.debugBorderWidthChanged)
        layer->setDebugVisuals(layerState.showDebugBorders, layerState.debugBorderColor, layerState.debugBorderWidth, layerState.showRepaintCounter);

    if (layerState.replicaChanged)
        layer->setReplicaLayer(getLayerByIDIfExists(layerState.replica));

    if (layerState.maskChanged)
        layer->setMaskLayer(getLayerByIDIfExists(layerState.mask));

    if (layerState.imageChanged)
        assignImageBackingToLayer(layer, layerState.imageID);

    if (layerState.flagsChanged) {
        layer->setContentsOpaque(layerState.contentsOpaque);
        layer->setDrawsContent(layerState.drawsContent);
        layer->setContentsVisible(layerState.contentsVisible);
        layer->setBackfaceVisibility(layerState.backfaceVisible);

        // Never clip the root layer.
        layer->setMasksToBounds(id == m_rootLayerID ? false : layerState.masksToBounds);
        layer->setPreserves3D(layerState.preserves3D);

        bool fixedToViewportChanged = layer->fixedToViewport() != layerState.fixedToViewport;
        layer->setFixedToViewport(layerState.fixedToViewport);
        if (fixedToViewportChanged) {
            if (layerState.fixedToViewport)
                m_fixedLayers.add(id, layer);
            else
                m_fixedLayers.remove(id);
        }

        layer->setIsScrollable(layerState.isScrollable);
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
        layer->setFullScreenLayerForVideo(layerState.fullScreenLayerForVideo);
#endif
    }

    if (layerState.committedScrollOffsetChanged)
        layer->didCommitScrollOffset(layerState.committedScrollOffset);

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    if (layerState.scrollbarChanged) {
        layer->setHorizontalScrollbarLayer(layerByID(layerState.horizontalScrollbar));
        layer->setVerticalScrollbarLayer(layerByID(layerState.verticalScrollbar));
        m_scrollbarFadeAnimationController->showOverflowScollbar(layer);
    }
#endif

#if ENABLE(TIZEN_SCROLL_SNAP)
    if (layerState.horizontalSnapOffsetsChanged)
        layer->setHorizontalSnapOffsets(layerState.horizontalSnapOffsets);
    if (layerState.verticalSnapOffsetsChanged)
        layer->setVerticalSnapOffsets(layerState.verticalSnapOffsets);
#endif

    prepareContentBackingStore(layer);

    // Apply Operations.
    setLayerChildrenIfNeeded(layer, layerState);
    createTilesIfNeeded(layer, layerState);
    removeTilesIfNeeded(layer, layerState);
    updateTilesIfNeeded(layer, layerState);
#if ENABLE(CSS_FILTERS)
    setLayerFiltersIfNeeded(layer, layerState);
#endif
    setLayerAnimationsIfNeeded(layer, layerState);
#if USE(GRAPHICS_SURFACE)
    syncCanvasIfNeeded(layer, layerState);
#endif
    setLayerRepaintCountIfNeeded(layer, layerState);
}

TextureMapperLayer* CoordinatedGraphicsScene::getLayerByIDIfExists(CoordinatedLayerID id)
{
    return (id != InvalidCoordinatedLayerID) ? layerByID(id) : 0;
}

void CoordinatedGraphicsScene::createLayers(const Vector<CoordinatedLayerID>& ids)
{
    for (size_t index = 0; index < ids.size(); ++index)
        createLayer(ids[index]);
}

void CoordinatedGraphicsScene::createLayer(CoordinatedLayerID id)
{
    OwnPtr<TextureMapperLayer> newLayer = adoptPtr(new TextureMapperLayer);
    newLayer->setID(id);
    newLayer->setScrollClient(this);
    m_layers.add(id, newLayer.release());
}

void CoordinatedGraphicsScene::deleteLayers(const Vector<CoordinatedLayerID>& layerIDs)
{
    for (size_t index = 0; index < layerIDs.size(); ++index)
        deleteLayer(layerIDs[index]);

#if USE(TIZEN_PLATFORM_SURFACE)
    // FIXME : rebase
    // Added to free platform surface after clearing backing store.
    freePlatformSurfacesAfterClearingBackingStores();
#endif
}

void CoordinatedGraphicsScene::deleteLayer(CoordinatedLayerID layerID)
{
    OwnPtr<TextureMapperLayer> layer = m_layers.take(layerID);
    ASSERT(layer);

    m_backingStores.remove(layer.get());
    m_fixedLayers.remove(layerID);
#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#if USE(TIZEN_PLATFORM_SURFACE)
    SurfaceBackingStoreMap::iterator it = m_surfaceBackingStores.find(layer.get());

    if (it != m_surfaceBackingStores.end())
        m_client->freePlatformSurface(layerID, it->value->graphicsSurfaceFrontBuffer());
#endif
#endif
#if USE(GRAPHICS_SURFACE)
    m_surfaceBackingStores.remove(layer.get());
#endif
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    m_scrollbarFadeAnimationController->removeAnimationIfNeeded(layerID);
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    m_edgeEffectController->removeEffect(layer.get());
#endif
}

void CoordinatedGraphicsScene::setRootLayerID(CoordinatedLayerID layerID)
{
    ASSERT(layerID != InvalidCoordinatedLayerID);
    ASSERT(m_rootLayerID == InvalidCoordinatedLayerID);

    m_rootLayerID = layerID;

    TextureMapperLayer* layer = layerByID(layerID);
    ASSERT(m_rootLayer->children().isEmpty());
    m_rootLayer->addChild(layer);
}

void CoordinatedGraphicsScene::prepareContentBackingStore(TextureMapperLayer* layer)
{
    if (!layerShouldHaveBackingStore(layer)) {
        removeBackingStoreIfNeeded(layer);
        return;
    }

    createBackingStoreIfNeeded(layer);
    resetBackingStoreSizeToLayerSize(layer);
}

void CoordinatedGraphicsScene::createBackingStoreIfNeeded(TextureMapperLayer* layer)
{
    if (m_backingStores.contains(layer))
        return;

#if USE(TIZEN_PLATFORM_SURFACE)
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    RefPtr<CoordinatedBackingStore> backingStore;
    if (m_client->useGLBackend()) {
        backingStore = CoordinatedBackingStoreTizen::create();
        static_cast<CoordinatedBackingStoreTizen*>(backingStore.get())->setPlatformSurfaceTexturePool(m_client->platformSurfaceTexturePool());
    }
    else
        backingStore = CoordinatedBackingStore::create();
#else
    RefPtr<CoordinatedBackingStore> backingStore(CoordinatedBackingStoreTizen::create());
    static_cast<CoordinatedBackingStoreTizen*>(backingStore.get())->setPlatformSurfaceTexturePool(m_client->platformSurfaceTexturePool());
#endif
#else
    RefPtr<CoordinatedBackingStore> backingStore(CoordinatedBackingStore::create());
#endif
    m_backingStores.add(layer, backingStore);
    layer->setBackingStore(backingStore);
}

void CoordinatedGraphicsScene::removeBackingStoreIfNeeded(TextureMapperLayer* layer)
{
    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.take(layer);
    if (!backingStore)
        return;

    layer->setBackingStore(0);
}

void CoordinatedGraphicsScene::resetBackingStoreSizeToLayerSize(TextureMapperLayer* layer)
{
    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    ASSERT(backingStore);
    backingStore->setSize(layer->size());
    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::createTilesIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.tilesToCreate.isEmpty() || !layerShouldHaveBackingStore(layer))
        return;

    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    ASSERT(backingStore);

    for (size_t i = 0; i < state.tilesToCreate.size(); ++i)
        backingStore->createTile(state.tilesToCreate[i].tileID, state.tilesToCreate[i].scale);
}

void CoordinatedGraphicsScene::removeTilesIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.tilesToRemove.isEmpty())
        return;

    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    if (!backingStore)
        return;

#if USE(TIZEN_PLATFORM_SURFACE)
    for (size_t i = 0; i < state.tilesToRemove.size(); ++i) {
        if (backingStore->isBackedBySharedPlatformSurface()) {
            int platformSurfaceId = static_cast<CoordinatedBackingStoreTizen*>(backingStore.get())->tilePlatformSurfaceId(state.tilesToRemove[i]);
            if (platformSurfaceId > 0)
                m_client->freePlatformSurface(layer->id(), platformSurfaceId);
        }
        backingStore->removeTile(state.tilesToRemove[i]);
    }
#else
    for (size_t i = 0; i < state.tilesToRemove.size(); ++i)
        backingStore->removeTile(state.tilesToRemove[i]);
#endif

    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::updateTilesIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (state.tilesToUpdate.isEmpty() || !layerShouldHaveBackingStore(layer))
        return;

    RefPtr<CoordinatedBackingStore> backingStore = m_backingStores.get(layer);
    ASSERT(backingStore);

    for (size_t i = 0; i < state.tilesToUpdate.size(); ++i) {
        const TileUpdateInfo& tileInfo = state.tilesToUpdate[i];
        const SurfaceUpdateInfo& surfaceUpdateInfo = tileInfo.updateInfo;

#if USE(TIZEN_PLATFORM_SURFACE)
        if (backingStore->isBackedBySharedPlatformSurface())
            updatePlatformSurfaceTile(layer->id(), tileInfo.tileID, surfaceUpdateInfo.updateRect, tileInfo.tileRect, surfaceUpdateInfo.platformSurfaceID, surfaceUpdateInfo.platformSurfaceSize, surfaceUpdateInfo.partialUpdate);
        else {
            SurfaceMap::iterator surfaceIt = m_surfaces.find(surfaceUpdateInfo.atlasID);
            ASSERT(surfaceIt != m_surfaces.end());
            backingStore->updateTile(tileInfo.tileID, surfaceUpdateInfo.updateRect, tileInfo.tileRect, surfaceIt->value, surfaceUpdateInfo.surfaceOffset);
        }
#else
        SurfaceMap::iterator surfaceIt = m_surfaces.find(surfaceUpdateInfo.atlasID);
        ASSERT(surfaceIt != m_surfaces.end());

        backingStore->updateTile(tileInfo.tileID, surfaceUpdateInfo.updateRect, tileInfo.tileRect, surfaceIt->value, surfaceUpdateInfo.surfaceOffset);
#endif
        m_backingStoresWithPendingBuffers.add(backingStore);
    }
}

void CoordinatedGraphicsScene::syncUpdateAtlases(const CoordinatedGraphicsState& state)
{
    for (size_t i = 0; i < state.updateAtlasesToCreate.size(); ++i)
        createUpdateAtlas(state.updateAtlasesToCreate[i].first, state.updateAtlasesToCreate[i].second);

    for (size_t i = 0; i < state.updateAtlasesToRemove.size(); ++i)
        removeUpdateAtlas(state.updateAtlasesToRemove[i]);
}

void CoordinatedGraphicsScene::createUpdateAtlas(uint32_t atlasID, PassRefPtr<CoordinatedSurface> surface)
{
    ASSERT(!m_surfaces.contains(atlasID));
    m_surfaces.add(atlasID, surface);
}

void CoordinatedGraphicsScene::removeUpdateAtlas(uint32_t atlasID)
{
    ASSERT(m_surfaces.contains(atlasID));
    m_surfaces.remove(atlasID);
}

void CoordinatedGraphicsScene::syncImageBackings(const CoordinatedGraphicsState& state)
{
    for (size_t i = 0; i < state.imagesToRemove.size(); ++i)
        removeImageBacking(state.imagesToRemove[i]);

    for (size_t i = 0; i < state.imagesToCreate.size(); ++i)
        createImageBacking(state.imagesToCreate[i]);

    for (size_t i = 0; i < state.imagesToUpdate.size(); ++i)
        updateImageBacking(state.imagesToUpdate[i].first, state.imagesToUpdate[i].second);

    for (size_t i = 0; i < state.imagesToClear.size(); ++i)
        clearImageBackingContents(state.imagesToClear[i]);
}

void CoordinatedGraphicsScene::createImageBacking(CoordinatedImageBackingID imageID)
{
    ASSERT(!m_imageBackings.contains(imageID));
#if PLATFORM(TIZEN)
    if (m_imageBackings.contains(imageID))
        return;
#endif
    RefPtr<CoordinatedBackingStore> backingStore(CoordinatedBackingStore::create());
    m_imageBackings.add(imageID, backingStore.release());
}

void CoordinatedGraphicsScene::updateImageBacking(CoordinatedImageBackingID imageID, PassRefPtr<CoordinatedSurface> surface)
{
    ASSERT(m_imageBackings.contains(imageID));
#if PLATFORM(TIZEN)
    if (!m_imageBackings.contains(imageID))
        return;
#endif
    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    RefPtr<CoordinatedBackingStore> backingStore = it->value;

    // CoordinatedImageBacking is realized to CoordinatedBackingStore with only one tile in UI Process.
    backingStore->createTile(1 /* id */, 1 /* scale */);
    IntRect rect(IntPoint::zero(), surface->size());
    // See CoordinatedGraphicsLayer::shouldDirectlyCompositeImage()
    ASSERT(2000 >= std::max(rect.width(), rect.height()));
    backingStore->setSize(rect.size());
    backingStore->updateTile(1 /* id */, rect, rect, surface, rect.location());

    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::clearImageBackingContents(CoordinatedImageBackingID imageID)
{
    ASSERT(m_imageBackings.contains(imageID));
#if PLATFORM(TIZEN)
    if (!m_imageBackings.contains(imageID))
        return;
#endif
    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    RefPtr<CoordinatedBackingStore> backingStore = it->value;
    backingStore->removeAllTiles();
    m_backingStoresWithPendingBuffers.add(backingStore);
}

void CoordinatedGraphicsScene::removeImageBacking(CoordinatedImageBackingID imageID)
{
    ASSERT(m_imageBackings.contains(imageID));
#if PLATFORM(TIZEN)
    if (!m_imageBackings.contains(imageID))
        return;
#endif

    // We don't want TextureMapperLayer refers a dangling pointer.
    m_releasedImageBackings.append(m_imageBackings.take(imageID));
}

void CoordinatedGraphicsScene::assignImageBackingToLayer(TextureMapperLayer* layer, CoordinatedImageBackingID imageID)
{
#if USE(GRAPHICS_SURFACE)
    if (m_surfaceBackingStores.contains(layer))
        return;
#endif

    if (imageID == InvalidCoordinatedImageBackingID) {
        layer->setContentsLayer(0);
        return;
    }

    ImageBackingMap::iterator it = m_imageBackings.find(imageID);
    ASSERT(it != m_imageBackings.end());
    layer->setContentsLayer(it->value.get());
}

void CoordinatedGraphicsScene::removeReleasedImageBackingsIfNeeded()
{
    m_releasedImageBackings.clear();
}

void CoordinatedGraphicsScene::commitPendingBackingStoreOperations()
{
    HashSet<RefPtr<CoordinatedBackingStore> >::iterator end = m_backingStoresWithPendingBuffers.end();
    for (HashSet<RefPtr<CoordinatedBackingStore> >::iterator it = m_backingStoresWithPendingBuffers.begin(); it != end; ++it)
        (*it)->commitTileOperations(m_textureMapper.get());

    m_backingStoresWithPendingBuffers.clear();
}

void CoordinatedGraphicsScene::commitSceneState(const CoordinatedGraphicsState& state)
{
    m_renderedContentsScrollPosition = state.scrollPosition;

    createLayers(state.layersToCreate);
    deleteLayers(state.layersToRemove);

    if (state.rootCompositingLayer != m_rootLayerID)
        setRootLayerID(state.rootCompositingLayer);

    syncImageBackings(state);
    syncUpdateAtlases(state);
#if ENABLE(CSS_SHADERS)
    syncCustomFilterPrograms(state);
#endif

    for (size_t i = 0; i < state.layersToUpdate.size(); ++i)
        setLayerState(state.layersToUpdate[i].first, state.layersToUpdate[i].second);

#if USE(TIZEN_PLATFORM_SURFACE)
    freePlatformSurface();
#endif

    commitPendingBackingStoreOperations();
    removeReleasedImageBackingsIfNeeded();

    // The pending tiles state is on its way for the screen, tell the web process to render the next one.
    dispatchOnMainThread(bind(&CoordinatedGraphicsScene::renderNextFrame, this));
}

void CoordinatedGraphicsScene::renderNextFrame()
{
    if (m_client)
        m_client->renderNextFrame();
}

void CoordinatedGraphicsScene::ensureRootLayer()
{
#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
    if(!m_textureMapper) {
        if (m_isAcceleration) {
            if (m_client->makeContextCurrent()) {
                m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
#if ENABLE(TIZEN_TEXTURE_MAPPER_TEXTURE_POOL_MAKE_CONTEXT_CURRENT)
                m_textureMapper->setCoordinatedGraphicsScene(this);
#endif
                static_cast<TextureMapperGL*>(m_textureMapper.get())->setEnableEdgeDistanceAntialiasing(true);
            }
        } else
            m_textureMapper = TextureMapper::create();
    }
#endif

    if (m_rootLayer)
        return;

    m_rootLayer = adoptPtr(new TextureMapperLayer);
    m_rootLayer->setMasksToBounds(false);
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setAnchorPoint(FloatPoint3D(0, 0, 0));

    // The root layer should not have zero size, or it would be optimized out.
    m_rootLayer->setSize(FloatSize(1.0, 1.0));

    ASSERT(m_textureMapper);
    m_rootLayer->setTextureMapper(m_textureMapper.get());
}

void CoordinatedGraphicsScene::syncRemoteContent()
{
    // We enqueue messages and execute them during paint, as they require an active GL context.
    ensureRootLayer();

    Vector<Function<void()> > renderQueue;
    bool calledOnMainThread = WTF::isMainThread();
    if (!calledOnMainThread)
        m_renderQueueMutex.lock();
    renderQueue.swap(m_renderQueue);
    if (!calledOnMainThread)
        m_renderQueueMutex.unlock();

    for (size_t i = 0; i < renderQueue.size(); ++i)
        renderQueue[i]();
}

void CoordinatedGraphicsScene::purgeGLResources()
{
    m_imageBackings.clear();
    m_releasedImageBackings.clear();
#if USE(GRAPHICS_SURFACE)
    m_surfaceBackingStores.clear();
#endif
    m_surfaces.clear();

#if USE(TIZEN_PLATFORM_SURFACE)
    clearPlatformLayerPlatformSurfaces();
#endif
    m_rootLayer.clear();
    m_rootLayerID = InvalidCoordinatedLayerID;
    m_layers.clear();
    m_fixedLayers.clear();
    m_textureMapper.clear();
    m_backingStores.clear();
    m_backingStoresWithPendingBuffers.clear();

    setActive(false);
    dispatchOnMainThread(bind(&CoordinatedGraphicsScene::purgeBackingStores, this));
}

void CoordinatedGraphicsScene::dispatchCommitScrollOffset(uint32_t layerID, const IntSize& offset)
{
    m_client->commitScrollOffset(layerID, offset);
}

void CoordinatedGraphicsScene::commitScrollOffset(uint32_t layerID, const IntSize& offset)
{
#if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
    m_overflowScrollOffset = offset;
#endif
    dispatchOnMainThread(bind(&CoordinatedGraphicsScene::dispatchCommitScrollOffset, this, layerID, offset));
}

void CoordinatedGraphicsScene::purgeBackingStores()
{
    if (m_client)
        m_client->purgeBackingStores();
}

void CoordinatedGraphicsScene::setLayerAnimationsIfNeeded(TextureMapperLayer* layer, const CoordinatedGraphicsLayerState& state)
{
    if (!state.animationsChanged)
        return;

#if ENABLE(CSS_SHADERS)
    for (size_t i = 0; i < state.animations.animations().size(); ++i) {
        const KeyframeValueList& keyframes = state.animations.animations().at(i).keyframes();
        if (keyframes.property() != AnimatedPropertyWebkitFilter)
            continue;
        for (size_t j = 0; j < keyframes.size(); ++j) {
            const FilterAnimationValue& filterValue = static_cast<const FilterAnimationValue&>(keyframes.at(j));
            injectCachedCustomFilterPrograms(filterValue.value());
        }
    }
#endif
    layer->setAnimations(state.animations);
}

void CoordinatedGraphicsScene::detach()
{
    ASSERT(isMainThread());
#if ENABLE(TIZEN_CLEAR_BACKINGSTORE)
    clearBackingStores();
#endif
    m_renderQueue.clear();
    m_client = 0;
}

void CoordinatedGraphicsScene::appendUpdate(const Function<void()>& function)
{
    if (!m_isActive)
        return;

    ASSERT(isMainThread());
    MutexLocker locker(m_renderQueueMutex);
    m_renderQueue.append(function);
}

void CoordinatedGraphicsScene::setActive(bool active)
{
    if (m_isActive == active)
        return;

    // Have to clear render queue in both cases.
    // If there are some updates in queue during activation then those updates are from previous instance of paint node
    // and cannot be applied to the newly created instance.
    m_renderQueue.clear();
    m_isActive = active;
    if (m_isActive)
        dispatchOnMainThread(bind(&CoordinatedGraphicsScene::renderNextFrame, this));
}

void CoordinatedGraphicsScene::setBackgroundColor(const Color& color)
{
    m_backgroundColor = color;
}

TextureMapperLayer* CoordinatedGraphicsScene::findScrollableContentsLayerAt(const FloatPoint& point)
{
    return rootLayer() ? rootLayer()->findScrollableContentsLayerAt(point) : 0;
}

#if ENABLE(TIZEN_CLEAR_BACKINGSTORE)
void CoordinatedGraphicsScene::clearBackingStores()
{
    LayerMap::iterator end = m_layers.end();
    for(LayerMap::iterator it = m_layers.begin(); it != end; ++it)
        it->value->clearBackingStore();

#if USE(TIZEN_PLATFORM_SURFACE)
    clearPlatformLayerPlatformSurfaces();
#endif
    /* FIXME : rebase
    m_directlyCompositedImages.clear();
    */
#if ENABLE(TIZEN_EMULATOR)
    m_surfaces.clear();
#endif
    m_backingStores.clear();
    m_backingStoresWithPendingBuffers.clear();

#if USE(TIZEN_PLATFORM_SURFACE)
    freePlatformSurfacesAfterClearingBackingStores();
#endif
}
#endif

#if USE(TIZEN_PLATFORM_SURFACE)
PassRefPtr<CoordinatedBackingStore> CoordinatedGraphicsScene::getBackingStore(CoordinatedLayerID id)
{
    TextureMapperLayer* layer = layerByID(id);
    ASSERT(layer);
    RefPtr<CoordinatedBackingStoreTizen> backingStore = static_cast<CoordinatedBackingStoreTizen*>(layer->backingStore().get());
    if (!backingStore) {
        backingStore = CoordinatedBackingStoreTizen::create();
        layer->setBackingStore(backingStore.get());
        backingStore->setPlatformSurfaceTexturePool(m_client->platformSurfaceTexturePool());
    }
    ASSERT(backingStore);
    return backingStore;
}

void CoordinatedGraphicsScene::freePlatformSurface()
{
    if (!hasPlatformSurfaceToFree())
        return;

    HashSet<RefPtr<CoordinatedBackingStore> >::iterator end = m_backingStoresWithPendingBuffers.end();
    for (HashSet<RefPtr<CoordinatedBackingStore> >::iterator it = m_backingStoresWithPendingBuffers.begin(); it != end; ++it) {
        if (!(*it)->isBackedBySharedPlatformSurface())
            continue;

        CoordinatedBackingStoreTizen* backingStore = static_cast<CoordinatedBackingStoreTizen*>((*it).get());
        int layerId = backingStore->layerId();
        Vector<int> freePlatformSurface = *(backingStore->freePlatformSurfaceTiles());

        for (unsigned n = 0; n < backingStore->freePlatformSurfaceTiles()->size(); ++n)
            m_client->freePlatformSurface(layerId, freePlatformSurface[n]);
        backingStore->clearFreePlatformSurfaceTiles();
    }
    /* FIXME : rebase
    // We should clear m_backingStoresWithPendingBuffers after commitPendingBackingStoreOperations() function.
    // Do not clear here.
    m_backingStoresWithPendingBuffers.clear();
    */

    // m_freePlatformSurfaces is for platformSurfaces which are not corresponding to platformSurface tiles, like TextureMapperPlatformLayer's.
    for (unsigned n = 0; n < m_freePlatformSurfaces.size(); ++n)
        m_client->freePlatformSurface(m_freePlatformSurfaces[n].layerID, m_freePlatformSurfaces[n].platformSurfaceId) ;

    m_freePlatformSurfaces.clear();
}

bool CoordinatedGraphicsScene::hasPlatformSurfaceToFree()
{
    if (m_backingStoresWithPendingBuffers.size() > 0)
        return true;

    if (m_freePlatformSurfaces.isEmpty())
        return false;

    return true;
}

void CoordinatedGraphicsScene::updatePlatformSurfaceTile(CoordinatedLayerID layerID, int tileID, const IntRect& sourceRect, const IntRect& targetRect, int platformSurfaceId, const IntSize& platformSurfaceSize, bool partialUpdate)
{
    if (!platformSurfaceId)
        return;

    m_client->makeContextCurrent();

    RefPtr<CoordinatedBackingStoreTizen> backingStore = static_cast<CoordinatedBackingStoreTizen*>(getBackingStore(layerID).get());
    backingStore->updatePlatformSurfaceTile(tileID, sourceRect, targetRect, layerID, platformSurfaceId, platformSurfaceSize, partialUpdate, m_textureMapper.get());

    /* FIXME : rebase
    if (!m_backingStoresWithPendingBuffers.contains(backingStore))
        m_backingStoresWithPendingBuffers.add(backingStore);
    */
}

void CoordinatedGraphicsScene::freePlatformSurfacesAfterClearingBackingStores()
{
    // Before calling this function, destroying BackingStores should be done,
    // because platformSurfacesToFree is filled in ~CoordinatedBackingStore().

    PlatformSurfaceTexturePool* platformSurfaceTexturePool = m_client->platformSurfaceTexturePool();
    Vector<int> platformSurfacesToFree = platformSurfaceTexturePool->platformSurfacesToFree();

    for (unsigned n = 0; n < platformSurfacesToFree.size(); ++n)
        // Actually, m_rootLayerID is not correct for all platformSurfaces.
        // But LayerTreeCoordinator::freePlatformSurface() doesn't care layerID for HTML contents layers
        // so any dummy layerID is okay for now.
        m_client->freePlatformSurface(m_rootLayerID, platformSurfacesToFree[n]);

    platformSurfaceTexturePool->clearPlatformSurfacesToFree();
}

void CoordinatedGraphicsScene::clearPlatformLayerPlatformSurfaces()
{
    PlatformLayerPlatformSurfaceMap::iterator end = m_platformLayerPlatformSurfaces.end();
    for (PlatformLayerPlatformSurfaceMap::iterator it = m_platformLayerPlatformSurfaces.begin(); it != end; ++it)
        m_client->removePlatformSurface(it->value, it->key);

    m_platformLayerPlatformSurfaces.clear();
}
#endif // USE(TIZEN_PLATFORM_SURFACE)

#if ENABLE(TIZEN_DIRECT_RENDERING)
void CoordinatedGraphicsScene::updateViewportFired()
{
    callOnMainThread(bind(&CoordinatedGraphicsScene::updateViewport, this));
}
#endif // ENABLE(TIZEN_DIRECT_RENDERING)


#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
IntSize CoordinatedGraphicsScene::scrollScrollableArea(const IntSize& offset)
{
    ASSERT(m_touchedScrollableLayerID != InvalidCoordinatedLayerID);
    TextureMapperLayer* scrollableLayer = layerByID(m_touchedScrollableLayerID);
#if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
    // m_overflowScrollOffset will be set by scrollBy() and commitScrollOffset() functions,
    // so we have to set values to 0 before calling scrollBy().
    m_overflowScrollOffset.setWidth(0);
    m_overflowScrollOffset.setHeight(0);
    if (scrollableLayer)
        scrollableLayer->scrollBy(offset);

    return m_overflowScrollOffset;
#else
    return IntSize(0, 0);
#endif // #if ENABLE(TIZEN_UI_SIDE_OVERFLOW_SCROLLING)
}

void CoordinatedGraphicsScene::startScrollableAreaTouch(const IntPoint& point)
{
    TextureMapperLayer* scrollableLayer = findScrollableContentsLayerAt(point);
    if (!scrollableLayer) {
        m_touchedScrollableLayerID = InvalidCoordinatedLayerID;
        return;
    }

    m_touchedScrollableLayerID = scrollableLayer->id();
}

void CoordinatedGraphicsScene::scrollableAreaScrollFinished()
{
    ASSERT(m_touchedScrollableLayerID != InvalidCoordinatedLayerID);
    TextureMapperLayer* scrollableLayer = layerByID(m_touchedScrollableLayerID);
    if (!scrollableLayer)
        return;

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    if (scrollableLayer->horizontalScrollbarLayer() || scrollableLayer->verticalScrollbarLayer())
        m_scrollbarFadeAnimationController->overflowScrollFinished(scrollableLayer);
#endif
}

#if ENABLE(TIZEN_EDGE_EFFECT)
bool CoordinatedGraphicsScene::existsVerticalScrollbarLayerInTouchedScrollableLayer()
{
    ASSERT(m_touchedScrollableLayerID != InvalidCoordinatedLayerID);
    TextureMapperLayer* scrollableLayer = layerByID(m_touchedScrollableLayerID);
    if (!scrollableLayer)
        return false;

    if (scrollableLayer->verticalScrollbarLayer())
        return true;

    return false;
}

bool CoordinatedGraphicsScene::existsHorizontalScrollbarLayerInTouchedScrollableLayer()
{
    ASSERT(m_touchedScrollableLayerID != InvalidCoordinatedLayerID);
    TextureMapperLayer* scrollableLayer = layerByID(m_touchedScrollableLayerID);
    if (!scrollableLayer)
        return false;

    if (scrollableLayer->horizontalScrollbarLayer())
        return true;

    return false;
}
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)

#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
void CoordinatedGraphicsScene::showOverflowScollbars()
{
    if (!m_rootLayer)
        return;

    Vector<TextureMapperLayer*> scrollableLayers;
    m_rootLayer->collectScrollableLayerRecursively(scrollableLayers);

    size_t size = scrollableLayers.size();
    for (size_t i = 0; i < size; ++i)
        m_scrollbarFadeAnimationController->showOverflowScollbar(scrollableLayers[i]);
}
#endif

#if ENABLE(TIZEN_EDGE_EFFECT)
static TextureMapperLayer* findFirstDescendantWithContentsRecursively(TextureMapperLayer* layer)
{
    if (layerShouldHaveBackingStore(layer))
        return layer;

    for (size_t i = 0; i < layer->children().size(); ++i) {
        TextureMapperLayer* contentsLayer = findFirstDescendantWithContentsRecursively(layer->children()[i]);
        if (contentsLayer)
            return contentsLayer;
    }

    return 0;
}

void CoordinatedGraphicsScene::showEdgeEffect(TextureMapperLayer::EdgeEffectType type, bool overflow)
{
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    if (overflow && m_touchedScrollableLayerID != InvalidCoordinatedLayerID) {
        m_edgeEffectController->showEdgeEffect(layerByID(m_touchedScrollableLayerID), type);
        return;
    }
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    if (rootLayer()) {
        TextureMapperLayer* mainContentsLayer = findFirstDescendantWithContentsRecursively(rootLayer());
        if (mainContentsLayer)
            m_edgeEffectController->showEdgeEffect(mainContentsLayer, type);
    }
}

void CoordinatedGraphicsScene::hideVerticalEdgeEffect(bool withAnimation)
{
    m_edgeEffectController->hideVerticalEdgeEffect(withAnimation);
}

void CoordinatedGraphicsScene::hideHorizontalEdgeEffect(bool withAnimation)
{
    m_edgeEffectController->hideHorizontalEdgeEffect(withAnimation);
}
#endif // #if ENABLE(TIZEN_EDGE_EFFECT)

#if ENABLE(TIZEN_SCROLL_SNAP)
IntSize CoordinatedGraphicsScene::findScrollSnapOffset(const IntSize& offset)
{
    TextureMapperLayer* layer = rootLayer();
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    if (m_touchedScrollableLayerID != InvalidCoordinatedLayerID)
        layer = layerByID(m_touchedScrollableLayerID);
#endif
    if (!layer)
        return IntSize();

    return layer->findScrollSnapOffset(offset);
}
#endif

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
