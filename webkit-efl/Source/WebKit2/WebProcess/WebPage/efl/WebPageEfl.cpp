/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
#include "WebPage.h"

#include "EditorState.h"
#include "EventHandler.h"
#include "NotImplemented.h"
#include "WebEvent.h"
#include "WindowsKeyboardCodes.h"
#include <WebCore/EflKeyboardUtilities.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/RenderThemeEfl.h>
#include <WebCore/Settings.h>

#if HAVE(ACCESSIBILITY)
#include "WebPageAccessibilityObject.h"
#endif

#if ENABLE(TIZEN_DEVICE_ORIENTATION)
#include "DeviceMotionClientTizen.h"
#include "DeviceOrientationClientTizen.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#include "HitTestResult.h"
#include "NamedNodeMap.h"
#endif

#if ENABLE(TIZEN_PREFERENCE)
#include "WebPreferencesStore.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING) || ENABLE(TIZEN_SPATIAL_NAVIGATION)
#include <WebCore/HTMLAreaElement.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING) || ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
#include <WebCore/HTMLFrameOwnerElement.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include "WebCoreArgumentCoders.h"
#include <WebCore/HTMLImageElement.h>
#include <WebCore/NodeTraversal.h>
#include <WebCore/RenderLayer.h>
#endif

#if ENABLE(TIZEN_STYLE_SCOPED)
#include <WebCore/RuntimeEnabledFeatures.h>
#endif // ENABLE(TIZEN_STYLE_SCOPED)

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif // ENABLE(TIZEN_CSP)

#if ENABLE(TIZEN_MOBILE_WEB_PRINT) || ENABLE(TIZEN_SCREEN_CAPTURE)
#include "WebImage.h"
#include <WebCore/PlatformContextCairo.h>
#endif

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
#include "fontconfig/fontconfig.h"
#include <WebCore/FontCache.h>
#include <WebCore/PageCache.h>
#endif // ENABLE(TIZEN_USE_SETTINGS_FONT)

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "WebPageSerializerTizen.h"
#endif

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
#include "PluginView.h"
#endif // ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
#include <WebCore/ClipboardTizen.h>
#include <WebCore/EditorClient.h>
#endif // ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)

#if ENABLE(TIZEN_PASTEBOARD)
#include <WebCore/Pasteboard.h>
#endif // ENABLE(TIZEN_PASTEBOARD)

#if ENABLE(TIZEN_WEB_STORAGE)
#include "WebPageProxyMessages.h"
#include <WebCore/GroupSettings.h>
#include <WebCore/PageGroup.h>
#endif // ENABLE(TIZEN_WEB_STORAGE)

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
#include "SessionState.h"
#include <WTF/wtf/CurrentTime.h>
#endif

#if ENABLE(TIZEN_JAVASCRIPT_AND_RESOURCE_SUSPEND_RESUME)
#include <WebCore/AnimationController.h>
#endif

using namespace WebCore;

namespace WebKit {

void WebPage::platformInitialize()
{
#if HAVE(ACCESSIBILITY)
    m_accessibilityObject = adoptGRef(webPageAccessibilityObjectNew(this));
#endif
#if ENABLE(TIZEN_DEVICE_ORIENTATION)
    WebCore::provideDeviceMotionTo(m_page.get(), new DeviceMotionClientTizen);
    WebCore::provideDeviceOrientationTo(m_page.get(), new DeviceOrientationClientTizen);
#endif
}

#if HAVE(ACCESSIBILITY)
void WebPage::updateAccessibilityTree()
{
    if (!m_accessibilityObject)
        return;

    webPageAccessibilityObjectRefresh(m_accessibilityObject.get());
}
#endif

#if ENABLE(TIZEN_PREFERENCE)
void WebPage::platformPreferencesDidChange(const WebPreferencesStore& store)
{
    Settings* settings = m_page->settings();
    settings->setInteractiveFormValidationEnabled(store.getBoolValueForKey(WebPreferencesKey::interactiveFormValidationEnabledKey()));
#if ENABLE(TIZEN_UI_TAP_SOUND)
    settings->setLinkEffectEnabled(store.getBoolValueForKey(WebPreferencesKey::linkEffectEnabledKey()));
#endif
    settings->setUsesEncodingDetector(store.getBoolValueForKey(WebPreferencesKey::usesEncodingDetectorKey()));
#if ENABLE(TIZEN_STYLE_SCOPED)
    WebCore::RuntimeEnabledFeatures::setStyleScopedEnabled(store.getBoolValueForKey(WebPreferencesKey::styleScopedEnabledKey()));
#endif // ENABLE(TIZEN_STYLE_SCOPED)
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    settings->setLoadRemoteImages(store.getBoolValueForKey(WebPreferencesKey::loadRemoteImagesKey()));
#endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
#if ENABLE(TIZEN_WEB_AUDIO)
    settings->setWebAudioEnabled(true);
#endif // ENABLE(TIZEN_WEB_AUDIO)
    settings->setDeviceSupportsMouse(store.getBoolValueForKey(WebPreferencesKey::deviceSupportsMouseKey()));
}
#else
void WebPage::platformPreferencesDidChange(const WebPreferencesStore&)
{
    notImplemented();
}
#endif // #if ENABLE(TIZEN_PREFERENCE)

static inline void scroll(Page* page, ScrollDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->scrollRecursively(direction, granularity);
}

bool WebPage::performDefaultBehaviorForKeyEvent(const WebKeyboardEvent& keyboardEvent)
{
    if (keyboardEvent.type() != WebEvent::KeyDown && keyboardEvent.type() != WebEvent::RawKeyDown)
        return false;

    switch (keyboardEvent.windowsVirtualKeyCode()) {
    case VK_BACK:
        if (keyboardEvent.shiftKey())
            m_page->goForward();
        else
            m_page->goBack();
        break;
    case VK_SPACE:
        scroll(m_page.get(), keyboardEvent.shiftKey() ? ScrollUp : ScrollDown, ScrollByPage);
        break;
    case VK_LEFT:
        scroll(m_page.get(), ScrollLeft, ScrollByLine);
        break;
    case VK_RIGHT:
        scroll(m_page.get(), ScrollRight, ScrollByLine);
        break;
    case VK_UP:
        scroll(m_page.get(), ScrollUp, ScrollByLine);
        break;
    case VK_DOWN:
        scroll(m_page.get(), ScrollDown, ScrollByLine);
        break;
    case VK_HOME:
        scroll(m_page.get(), ScrollUp, ScrollByDocument);
        break;
    case VK_END:
        scroll(m_page.get(), ScrollDown, ScrollByDocument);
        break;
    case VK_PRIOR:
        scroll(m_page.get(), ScrollUp, ScrollByPage);
        break;
    case VK_NEXT:
        scroll(m_page.get(), ScrollDown, ScrollByPage);
        break;
    default:
        return false;
    }

    return true;
}

bool WebPage::platformHasLocalDataForURL(const KURL&)
{
    notImplemented();
    return false;
}

String WebPage::cachedResponseMIMETypeForURL(const KURL&)
{
    notImplemented();
    return String();
}

bool WebPage::platformCanHandleRequest(const ResourceRequest&)
{
    notImplemented();
    return true;
}

String WebPage::cachedSuggestedFilenameForURL(const KURL&)
{
    notImplemented();
    return String();
}

PassRefPtr<SharedBuffer> WebPage::cachedResponseDataForURL(const KURL&)
{
    notImplemented();
    return 0;
}

const char* WebPage::interpretKeyEvent(const KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    if (event->type() == eventNames().keydownEvent)
        return getKeyDownCommandName(event);

    return getKeyPressCommandName(event);
}

void WebPage::setThemePath(const String& themePath)
{
    WebCore::RenderThemeEfl* theme = static_cast<WebCore::RenderThemeEfl*>(m_page->theme());
    theme->setThemePath(themePath);
}

static Frame* targetFrameForEditing(WebPage* page)
{
    Frame* frame = page->corePage()->focusController()->focusedOrMainFrame();
    if (!frame)
        return 0;

    Editor& editor = frame->editor();
    if (!editor.canEdit())
        return 0;

    if (editor.hasComposition()) {
        // We should verify the parent node of this IME composition node are
        // editable because JavaScript may delete a parent node of the composition
        // node. In this case, WebKit crashes while deleting texts from the parent
        // node, which doesn't exist any longer.
        if (PassRefPtr<Range> range = editor.compositionRange()) {
            Node* node = range->startContainer();
            if (!node || !node->isContentEditable())
                return 0;
        }
    }

    return frame;
}

void WebPage::confirmComposition(const String& compositionString)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;
    m_isCompositing = true;
    targetFrame->editor().confirmComposition(compositionString);
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
    m_isCompositing = false;
}

void WebPage::setComposition(const String& compositionString, const Vector<WebCore::CompositionUnderline>& underlines, uint64_t cursorPosition)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;
    m_isCompositing = true;
    targetFrame->editor().setComposition(compositionString, underlines, cursorPosition, 0);
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
    m_isCompositing = false;
}

void WebPage::cancelComposition()
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;
    m_isCompositing = true;
    frame->editor().cancelComposition();
    send(Messages::WebPageProxy::EditorStateChanged(editorState()));
    m_isCompositing = false;
}

#if ENABLE(TIZEN_TEXT_CARET_HANDLING_WK2)
bool WebPage::setCaretPosition(const IntPoint& pos)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return false;

    FrameSelection* controller = frame->selection();
    if (!controller)
        return false;

    FrameView* frameView = frame->view();
    if (!frameView)
        return false;

    IntPoint point = m_page->mainFrame()->view()->windowToContents(pos);
    HitTestResult result(m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(point));
    if (result.scrollbar())
        return false;

    Node* innerNode = result.innerNode();

    if (!innerNode || !innerNode->renderer())
        return false;

    VisiblePosition visiblePos;

    // we check if content is richly editable - because those input field behave other than plain text ones
    // sometimes they may consists a node structure and they need special approach
    if (innerNode->rendererIsRichlyEditable()) {
        // point gets inner node local coordinates
        point = flooredIntPoint(result.localPoint());
        IntRect rect = innerNode->renderer()->absoluteBoundingBoxRect(true);

        // it is not the best way to do this, but it is not as slow and it works - so maybe in the future someone
        // will have a better idea how to solve it
        // here we are getting innerNode from HitTestResult - unfortunately this is a kind of high level node
        // in the code below I am trying to obtain low level node - #text - to get its coordinates and size

        // all those getting nodes rects are needed to bypass WebCore's methods of positioning caret when user
        // is clicking outside a node - and cheat WebCore telling it that actually we clicked into input field
        // node, not outside of it
        Node* deepInnerNode = innerNode->renderer()->positionForPoint(point).deepEquivalent().deprecatedNode();

        if (!deepInnerNode || !deepInnerNode->renderer())
            return false;

        // so we get a base node rectange
        IntRect deepNodeRect = deepInnerNode->renderer()->absoluteBoundingBoxRect(true);

        // we modify our local point to adjust it to base node local coordinates
        point.move(rect.x() - deepNodeRect.x(), rect.y() - deepNodeRect.y());

        // if we are outside the rect we cheat, that we are just inside of it
        if (point.y() < 0)
            point.setY(0);
        else if (point.y() >= deepNodeRect.height())
            point.setY(deepNodeRect.height() - 1);

        // visible position created - caret ready to set
        visiblePos = deepInnerNode->renderer()->positionForPoint(point);
        if (visiblePos.isNull())
            return false;
    } else {
        // for plain text input fields we can get only a caret bounding box
        if (!controller->isCaret() || !controller->caretRenderer())
            return false;

        const Node* node = controller->start().deprecatedNode();
        if (!node || !node->renderer())
            return false;

        Element* currentRootEditableElement = node->rootEditableElement();
        Element* newRootEditableElement = innerNode->rootEditableElement();
        if (currentRootEditableElement != newRootEditableElement)
            return false;

        IntRect rect = controller->caretRenderer()->absoluteBoundingBoxRect(true);

        // The below wirtten code is not correct way to implement. Presntly the is no
        // other working way. To be replaced by better logic
        // here we also cheat input field that we actually are just inside of if
        IntPoint focusedFramePoint = frame->view()->windowToContents(pos);
        IntPoint oldFocusedFramePoint = focusedFramePoint;

        const int boundariesWidth = 2;
        if (focusedFramePoint.x() < rect.x())
            focusedFramePoint.setX(rect.x());
        else if (focusedFramePoint.x() > rect.maxX())
            focusedFramePoint.setX(rect.maxX());
        if (focusedFramePoint.y() < rect.y() + boundariesWidth)
            focusedFramePoint.setY(rect.y() + boundariesWidth);
        else if (focusedFramePoint.y() >= rect.maxY() - boundariesWidth)
            focusedFramePoint.setY(rect.maxY() - boundariesWidth - 1);

        int diffX = focusedFramePoint.x() - oldFocusedFramePoint.x();
        int diffY = focusedFramePoint.y() - oldFocusedFramePoint.y();
        point.setX((point.x())+diffX);
        point.setY((point.y())+diffY);

        // hit test with fake (adjusted) coordinates
        IntPoint hitTestPoint = m_page->mainFrame()->view()->windowToContents(point);
        HitTestResult newResult(m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(hitTestPoint));

        if (!newResult.isContentEditable())
            return false;

        Node* newInnerNode = newResult.innerNode();

        if (!newInnerNode || !newInnerNode->renderer())
            return false;

        // visible position created
        visiblePos = newInnerNode->renderer()->positionForPoint(newResult.localPoint());
        if (visiblePos.isNull())
            return false;
    }

    // create visible selection from visible position
    VisibleSelection newSelection = VisibleSelection(visiblePos);
    controller->setSelection(newSelection, CharacterGranularity);
    // after setting selection caret blinking is suspended by default so we are unsuspedning it
    controller->setCaretBlinkingSuspended(false);

    return true;
}

void WebPage::getCaretPosition(IntRect& rect)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    FrameSelection* controller = frame->selection();
    if (!controller)
        return;

    Node* node = controller->start().deprecatedNode();
    if (!node || !node->renderer() || !node->isContentEditable())
        return;

    if (controller->isCaret()) {
        FrameView* frameView = frame->view();
        if (!frameView)
            return;

        rect = frameView->contentsToWindow(controller->absoluteCaretBounds());
    }
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
static LayoutPoint offsetOfFrame(Frame* frame)
{
    Frame* mainFrame = frame->page()->mainFrame();
    return mainFrame->view()->windowToContents(frame->view()->contentsToWindow(IntPoint()));
}

static IntRect addFocusedRects(Node* node, const LayoutPoint& frameOffset, Vector<IntRect>& rects)
{
    RenderObject* renderer = node->renderer();
    LayoutRect nodeRect;

    if (node->hasTagName(HTMLNames::areaTag)) {
        HTMLAreaElement* area = static_cast<HTMLAreaElement*>(node);
        HTMLImageElement* image = area->imageElement();
        if (image && image->renderer())
            nodeRect = area->computeRect(area->imageElement()->renderer());
    } else {
        if (node->isDocumentNode())
            nodeRect = static_cast<Document*>(node)->frame()->view()->visibleContentRect();
        else
            nodeRect = renderer->absoluteBoundingBoxRect();
    }
    nodeRect.moveBy(frameOffset);

    if (nodeRect.pixelSnappedX() < 0)
        nodeRect.shiftXEdgeTo(0);
    if (nodeRect.pixelSnappedY() < 0)
        nodeRect.shiftYEdgeTo(0);

    if (nodeRect.isEmpty())
        return IntRect();

    Vector<IntRect> candidateRects;
    renderer->addFocusRingRects(candidateRects, frameOffset);

    for (size_t i = 0; i < candidateRects.size(); ++i) {
        LayoutRect candidateRect = candidateRects.at(i);
        LayoutPoint originalLocation = candidateRect.location();

        candidateRect.moveBy(-frameOffset);

        RenderLayer* layer = renderer->enclosingLayer();
        RenderObject* currentRenderer = renderer;

        while (layer) {
            RenderObject* layerRenderer = layer->renderer();

            if (layerRenderer->hasOverflowClip() && layerRenderer != currentRenderer) {
                candidateRect.moveBy(LayoutPoint(currentRenderer->localToContainerPoint(FloatPoint(), toRenderBox(layerRenderer))));
                currentRenderer = layerRenderer;

                LayoutPoint baseLocation = candidateRect.location();
                candidateRect.intersect(toRenderBox(layerRenderer)->borderBoxRect());
                originalLocation.move(candidateRect.location() - baseLocation);

                if (candidateRect.isEmpty())
                    break;
            }
            layer = layer->parent();
        }

        if (!candidateRect.isEmpty()) {
            candidateRect.setLocation(originalLocation);
            FloatRect clippedRect = renderer->localToAbsoluteQuad(FloatRect(candidateRect)).boundingBox();
            clippedRect.intersect(nodeRect);
            if (!clippedRect.isEmpty())
                rects.append(enclosingIntRect(clippedRect));
        }
    }

    return enclosingIntRect(nodeRect);
}

static void calcFocusedRects(Node* node, Vector<IntRect>& rects)
{
    if (!node || !node->renderer())
        return;

    RenderObject* renderer = node->renderer();
    LayoutPoint frameOffset = offsetOfFrame(node->document()->frame());

    addFocusedRects(node, frameOffset, rects);

    Node* child = node->firstChild();
    while (child) {
        RenderObject* childRenderer = child->renderer();
        if (childRenderer && childRenderer->offsetParent() != renderer)
            addFocusedRects(child, frameOffset, rects);

        child = child->nextSibling();
    }
}
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
static bool isClickableOrFocusable(Node* focusableNode)
{

    if (!focusableNode)
        return false;
    if (focusableNode->isElementNode() && toElement(focusableNode)->isDisabledFormControl())
        return false;
    if (!focusableNode->inDocument())
        return false;
    if (!focusableNode->renderer() || focusableNode->renderer()->style()->visibility() != VISIBLE)
        return false;
    if (focusableNode->isHTMLElement() && toHTMLElement(focusableNode)->isFocusable()) {
        if (focusableNode->isLink()
            || focusableNode->hasTagName(HTMLNames::inputTag)
            || focusableNode->hasTagName(HTMLNames::selectTag)
            || focusableNode->hasTagName(HTMLNames::buttonTag))
            return true;
    }
    if ((focusableNode->isHTMLElement() && toHTMLElement(focusableNode)->supportsFocus())
        || focusableNode->hasEventListeners(eventNames().clickEvent)
        || focusableNode->hasEventListeners(eventNames().mousedownEvent)
        || focusableNode->hasEventListeners(eventNames().mouseupEvent)) {
        return true;
    }
    return false;
}

static void getFocusedRect(HitTestResult hitTestResult, Page* /* page */, Vector<IntRect>& rects)
{
    Node* node = hitTestResult.innerNode();

    if (!node)
        return;

    bool isFocusRingDrawable = false;
    Node* focusableNode = node;
    while (focusableNode) {
        RenderObject* renderer = focusableNode->renderer();
        if (renderer && (renderer->isBody() || renderer->isRenderView() || renderer->isRoot()))
            break;

        if (isClickableOrFocusable(focusableNode)) {
            isFocusRingDrawable = true;
            break;
        }

        focusableNode = focusableNode->parentNode();
    }

    // Don't draw focus ring if child is focusable or has trigger
    if (focusableNode && focusableNode->isContainerNode() && !focusableNode->isLink()) {
        WebCore::Node *child = static_cast<const ContainerNode*>(focusableNode)->firstChild();
        while (child) {
            if ((child->isHTMLElement() && toHTMLElement(child)->supportsFocus())
                || child->hasEventListeners(eventNames().clickEvent)
                || child->hasEventListeners(eventNames().mousedownEvent)
                || child->hasEventListeners(eventNames().mouseupEvent)) {
                return;
            }
            child = NodeTraversal::next(child, focusableNode);
        }
    }

    if (!isFocusRingDrawable) {
        if (!node->hasTagName(HTMLNames::imgTag))
            return;
        focusableNode = node;
    }

    calcFocusedRects(focusableNode, rects);
}
#endif

void WebPage::hitTestResultAtPoint(const IntPoint& point, int hitTestMode, WebHitTestResult::Data& hitTestResultData)
{
    Frame* frame = m_page->mainFrame();
    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    HitTestResult hitTestResult = frame->eventHandler()->hitTestResultAtPoint(frameView->windowToContents(point));
    hitTestResultData.absoluteImageURL = hitTestResult.absoluteImageURL().string();
    hitTestResultData.absoluteLinkURL = hitTestResult.absoluteLinkURL().string();
    hitTestResultData.absoluteMediaURL = hitTestResult.absoluteMediaURL().string();
    hitTestResultData.linkLabel = hitTestResult.textContent();
    hitTestResultData.linkTitle = hitTestResult.titleDisplayString();
    hitTestResultData.isContentEditable = hitTestResult.isContentEditable();
#if ENABLE(TIZEN_DRAG_SUPPORT)
    hitTestResultData.isDragSupport = hitTestResult.isDragSupport();
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

    int context = WebHitTestResult::HitTestResultContextDocument;

    if (!hitTestResult.absoluteLinkURL().isEmpty())
        context |= WebHitTestResult::HitTestResultContextLink;
    if (!hitTestResult.absoluteImageURL().isEmpty())
        context |= WebHitTestResult::HitTestResultContextImage;
    if (!hitTestResult.absoluteMediaURL().isEmpty())
        context |= WebHitTestResult::HitTestResultContextMedia;
    if (hitTestResult.isSelected())
        context |= WebHitTestResult::HitTestResultContextSelection;
    if (hitTestResult.isContentEditable())
        context |= WebHitTestResult::HitTestResultContextEditable;
    if (hitTestResult.innerNonSharedNode() && hitTestResult.innerNonSharedNode()->isTextNode())
        context |= WebHitTestResult::HitTestResultContextText;

    hitTestResultData.context = context;
    hitTestResultData.hitTestMode = hitTestMode;

#if ENABLE(TOUCH_EVENTS) && ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    getFocusedRect(hitTestResult, m_page.get(), hitTestResultData.focusedRects);

    if (hitTestResult.innerNode() && hitTestResult.innerNode()->renderer() && hitTestResult.innerNode()->renderer()->style()) {
        hitTestResultData.focusedColor = hitTestResult.innerNode()->renderer()->style()->tapHighlightColor();
        if (!hitTestResultData.focusedColor.hasAlpha())
            hitTestResultData.focusedColor = Color(hitTestResultData.focusedColor.red(), hitTestResultData.focusedColor.green(), hitTestResultData.focusedColor.blue(), RenderStyle::initialTapHighlightColor().alpha());
    }
#endif

    if (hitTestResultData.hitTestMode & WebHitTestResult::HitTestModeNodeData) {
        WebCore::Node* hitNode = hitTestResult.innerNonSharedNode();

        if (hitNode) {
            hitTestResultData.nodeData.nodeValue = hitNode->nodeValue();

            if ((hitTestResultData.context & WebHitTestResult::HitTestResultContextText) && hitNode->parentNode())
                hitNode = hitNode->parentNode(); // if hittest inner node is Text node, fill tagName with parent node's info and fill attributeMap with parent node's attributes.

            if (hitNode->isElementNode()) {
                WebCore::Element* hitElement = static_cast<WebCore::Element*>(hitNode);
                if (hitElement)
                    hitTestResultData.nodeData.tagName = hitElement->tagName();
            }

            WebCore::NamedNodeMap* namedNodeMap = hitNode->attributes();
            if (namedNodeMap) {
                for (size_t i = 0; i < namedNodeMap->length(); i++) {
                    const WebCore::Attribute* attribute = namedNodeMap->element()->attributeItem(i);
                    String key = attribute->name().toString();
                    String value = attribute->value();
                    hitTestResultData.nodeData.attributeMap.add(key, value);
                }
            }
        }
    }

    if ((hitTestResultData.hitTestMode & WebHitTestResult::HitTestModeImageData) && (hitTestResultData.context & WebHitTestResult::HitTestResultContextImage)) {
        WebCore::Image* hitImage = hitTestResult.image();
        if (hitImage && hitImage->data() && hitImage->data()->data()) {
            hitTestResultData.imageData.data.append(hitImage->data()->data(), hitImage->data()->size());
            hitTestResultData.imageData.fileNameExtension = hitImage->filenameExtension();
        }
    }
}
#endif // ENABLE(TIZEN_WEBKIT2_HIT_TEST)

#if ENABLE(TIZEN_CSP)
void WebPage::setContentSecurityPolicy(const String& policy, uint32_t headerType)
{
    // Keep the CSP with the Page, so that it can be accessed by widgets
    // or webapps who do not get the CSP using the usual HTTP header route.
    m_page->setContentPolicy(policy, static_cast<WebCore::ContentSecurityPolicy::HeaderType>(headerType));

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Document* document = frame->document();
    if (!document)
        return;

    document->contentSecurityPolicy()->didReceiveHeader(m_page->contentPolicyString(), m_page->headerType());
}
#endif // ENABLE(TIZEN_CSP)

#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
#define INCH_TO_MM 25.4
#define INCH_TO_POINTS 72.0

void WebPage::createPagesToPDF(const IntSize& surfaceSize, const String& fileName)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    IntSize contentsSize = frameView->contentsSize();
    RefPtr<WebImage> pageshotImage = WebImage::create(contentsSize, ImageOptionsShareable);
    if (!pageshotImage->bitmap())
        return;

    double pdfWidth = (double)surfaceSize.width() / INCH_TO_MM * INCH_TO_POINTS;
    double pdfHeight = (double)surfaceSize.height() / INCH_TO_MM * INCH_TO_POINTS;
    double scaleFactorPdf = 1.0;
    if (contentsSize.width() > pdfWidth)
        scaleFactorPdf = pdfWidth / (double)contentsSize.width();

    OwnPtr<WebCore::GraphicsContext> graphicsContext = pageshotImage->bitmap()->createGraphicsContextForPdfSurface(fileName, pdfWidth, pdfHeight);
    graphicsContext->scale(FloatSize(scaleFactorPdf, scaleFactorPdf));

    frameView->updateLayoutAndStyleIfNeededRecursive();

    int pageNumber = ((contentsSize.height() * scaleFactorPdf) / pdfHeight) + 1;
    float paintY = 0.0;

    PaintBehavior oldBehavior = frameView->paintBehavior();
    frameView->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
    for (int i = 0; i < pageNumber; i++) {
        IntRect paintRect(0, (int)paintY, contentsSize.width(), (int)(pdfHeight / scaleFactorPdf));

        frameView->paint(graphicsContext.get(), paintRect);
        cairo_show_page(graphicsContext->platformContext()->cr());
        graphicsContext->translate(0, -ceil(pdfHeight / scaleFactorPdf));
        paintY += (pdfHeight / scaleFactorPdf);
    }
    frameView->setPaintBehavior(oldBehavior);

    pageshotImage.release();
}
#endif // ENABLE(TIZEN_MOBILE_WEB_PRINT)

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
void WebPage::useSettingsFont()
{
    if (!WebCore::fontCache()->isFontFamliyTizen())
        return;

    int pageCapacity = WebCore::pageCache()->capacity();
    // Setting size to 0, makes all pages be released.
    WebCore::pageCache()->setCapacity(0);
    WebCore::pageCache()->setCapacity(pageCapacity);

    FcInitReinitialize();
    WebCore::fontCache()->invalidate();

    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return;

    if (Document* document = frame->document()) {
        document->dispatchWindowEvent(Event::create(eventNames().resizeEvent, false, false));
        document->recalcStyle(Node::Force);
    }
}
#endif // ENABLE(TIZEN_USE_SETTINGS_FONT)

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void WebPage::startOfflinePageSave(String subresourceFolderName)
{
    WebPageSerializerTizen::getSerializedPageContent(this, subresourceFolderName);
}
#endif

#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
void WebPage::suspendAnimationController()
{
    if (!m_suspendedAnimationController) {
        Frame* mainFrame = m_page->mainFrame();
        if (!mainFrame)
            return;

        for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
            frame->document()->suspendScriptedAnimationControllerCallbacks();

        m_suspendedAnimationController = true;
    }
}

void WebPage::resumeAnimationController()
{
    if (m_suspendedAnimationController) {
        Frame* mainFrame = m_page->mainFrame();
        if (!mainFrame)
            return;

        for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
            frame->document()->resumeScriptedAnimationControllerCallbacks();

        m_suspendedAnimationController = false;
    }
}
#endif

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
void WebPage::suspendPlugin()
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* view = frame->view();
        if (!view)
            continue;

        const HashSet<RefPtr<Widget> >* children = view->children();
        ASSERT(children);

        HashSet<RefPtr<Widget> >::const_iterator end = children->end();
        for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
            Widget* widget = (*it).get();
            if (widget->isPluginViewBase()) {
                PluginView* pluginView = static_cast<PluginView*>(widget);
                if (pluginView)
                    pluginView->suspendPlugin();
            }
        }
    }
}

void WebPage::resumePlugin()
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* view = frame->view();
        if (!view)
            continue;

        const HashSet<RefPtr<Widget> >* children = view->children();
        ASSERT(children);

        HashSet<RefPtr<Widget> >::const_iterator end = children->end();
        for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
            Widget* widget = (*it).get();
            if (widget->isPluginViewBase()) {
                PluginView* pluginView = static_cast<PluginView*>(widget);
                if (pluginView)
                    pluginView->resumePlugin();
            }
        }
    }
}
#endif // ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void WebPage::setClipboardDataForPaste(const String& data, const String& type)
{
#if ENABLE(TIZEN_PASTEBOARD)
    // FIXME: Should move to EditorClient like Clipboard
    Pasteboard::generalPasteboard()->setDataWithType(data, type);
#else
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    mainFrame->editor().client()->setClipboardDataForPaste(data, type);
#endif // ENABLE(TIZEN_PASTEBOARD)
}
#endif // ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)

#if ENABLE(TIZEN_JAVASCRIPT_AND_RESOURCE_SUSPEND_RESUME)
void WebPage::suspendJavaScriptAndResource()
{
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext()) {
        Document* document = frame->document();
        document->suspendScheduledTasks(WebCore::ActiveDOMObject::PageWillBeSuspended);
        frame->animation()->suspendAnimationsForDocument(document);
    }

    // FIXME: Need to implement. The codes to suspend all resource loaders can be added here.
}

void WebPage::resumeJavaScriptAndResource()
{
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext()) {
        Document* document = frame->document();
        document->resumeScheduledTasks(WebCore::ActiveDOMObject::PageWillBeSuspended);
        frame->animation()->resumeAnimationsForDocument(document);
    }

    // FIXME: Need to implement. The codes to resume all resource loaders can be added here.
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
void WebPage::getStorageQuotaBytes(uint64_t callbackID)
{
    uint32_t quota = m_page->group().groupSettings()->localStorageQuotaBytes();
    send(Messages::WebPageProxy::DidGetWebStorageQuotaBytes(quota, callbackID));
}

void WebPage::setStorageQuotaBytes(uint32_t quota)
{
    m_page->group().groupSettings()->setLocalStorageQuotaBytes(quota);
}
#endif // ENABLE(TIZEN_WEB_STORAGE)

#if ENABLE(TIZEN_INDEXED_DATABASE)
void WebPage::setIndexedDatabaseDirectory(const String& path)
{
    m_page->group().groupSettings()->setIndexedDBDatabasePath(path);
}
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if ENABLE(TIZEN_SCREEN_CAPTURE)
PassRefPtr<WebImage> WebPage::scaledSnapshotInViewCoordinates(const IntRect& rect, double scaleFactor, ImageOptions options)
{
    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return 0;

    IntSize size(ceil(rect.width() * scaleFactor), ceil(rect.height() * scaleFactor));
    RefPtr<WebImage> snapshot = WebImage::create(size, options);
    if (!snapshot->bitmap())
        return 0;

    OwnPtr<WebCore::GraphicsContext> graphicsContext = snapshot->bitmap()->createGraphicsContext();
    graphicsContext->scale(FloatSize(scaleFactor, scaleFactor));
    graphicsContext->translate(-rect.x(), -rect.y());

    frameView->updateLayoutAndStyleIfNeededRecursive();

    PaintBehavior oldBehavior = frameView->paintBehavior();
    frameView->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
    frameView->paint(graphicsContext.get(), rect);
    frameView->setPaintBehavior(oldBehavior);

    return snapshot.release();
}

void WebPage::createSnapshot(const IntRect rect, float scaleFactor, ShareableBitmap::Handle& snapshotHandle)
{
    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return;

    RefPtr<WebImage> snapshotImage = scaledSnapshotInViewCoordinates(rect, scaleFactor, ImageOptionsShareable);
    if (!snapshotImage || !snapshotImage->bitmap())
        return;

    snapshotImage->bitmap()->createHandle(snapshotHandle);
}
#endif // ENABLE(TIZEN_SCREEN_CAPTURE)

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
void WebPage::findScrollableArea(const IntPoint& point, bool& isScrollableVertically, bool& isScrollableHorizontally)
{
    isScrollableVertically = false;
    isScrollableHorizontally = false;
    HitTestResult result = m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(point);

    Frame* frame = m_page->mainFrame();
    if (m_startingNodeOfScrollableArea = result.innerNonSharedNode()) {
        if (m_startingNodeOfScrollableArea->isFrameOwnerElement())
            frame = static_cast<HTMLFrameOwnerElement*>(m_startingNodeOfScrollableArea.get())->contentFrame();
        else
            frame = m_startingNodeOfScrollableArea->document()->frame();
    }

    if (!frame)
        return;

    setFrameOfScrollableArea(frame);

    // Find scrollable sub frame.
    if (frame != m_page->mainFrame()) {
        if (FrameView* view = frame->view()) {
            if (view->verticalScrollbar())
                isScrollableVertically = true;
            if (view->horizontalScrollbar())
                isScrollableHorizontally = true;
            if (isScrollableVertically && isScrollableHorizontally)
                return;
        }
    }

    // Find overflow node.
    if (m_startingNodeOfScrollableArea) {
        RenderObject* renderer = m_startingNodeOfScrollableArea->renderer();
        while (renderer) {
            if (renderer->isBox()) {
                RenderBox* renderBox = toRenderBox(renderer);
                if (renderBox->scrollsOverflow() && renderBox->canBeScrolledAndHasScrollableArea()) {
                    if (renderBox->scrollHeight() != renderBox->clientHeight())
                        isScrollableVertically = true;
                    if (renderBox->scrollWidth() != renderBox->clientWidth())
                        isScrollableHorizontally = true;
                }
            }

            // FIXME: Find scrollable input box.

            renderer = renderer->parent();
        }
    }
}

void WebPage::scrollScrollableArea(const IntSize& offset, IntSize& scrolledOffset)
{
    scrolledOffset.setWidth(0);
    scrolledOffset.setHeight(0);
    Frame* frame = frameOfScrollableArea();
    if (!frame)
        return;

    bool wasScrolled = false;
    ScrollDirection verticalDirection = (offset.height() < 0) ? ScrollUp : ScrollDown;
    ScrollDirection horizontalDirection = (offset.width() < 0) ? ScrollLeft : ScrollRight;
    wasScrolled = frame->eventHandler()->scrollRecursivelyWithMultiplier(verticalDirection, ScrollByPixel, m_startingNodeOfScrollableArea.get(), abs(offset.height()));
    if (wasScrolled)
        scrolledOffset.setHeight(offset.height());
    wasScrolled = frame->eventHandler()->scrollRecursivelyWithMultiplier(horizontalDirection, ScrollByPixel, m_startingNodeOfScrollableArea.get(), abs(offset.width()));
    if (wasScrolled)
        scrolledOffset.setWidth(offset.width());
}

void WebPage::setFrameOfScrollableArea(Frame* frame)
{
    ASSERT(!frame || frame->page() == m_page);
    if (m_frameOfScrollableArea == frame)
        return;

    m_frameOfScrollableArea = frame;
}

Frame* WebPage::frameOfScrollableArea() const
{
    return m_frameOfScrollableArea ? m_frameOfScrollableArea : m_page->focusController()->focusedOrMainFrame();
}
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)

#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
void WebPage::scrollMainFrameBy(const IntSize& offset)
{
    m_page->mainFrame()->view()->scrollBy(offset);
}

void WebPage::scrollMainFrameTo(const IntPoint& position)
{
    m_page->mainFrame()->view()->setScrollPosition(position);
}

void WebPage::getScrollPosition(IntPoint& scrollPosition)
{
    scrollPosition = m_page->mainFrame()->view()->scrollPosition();
}
#endif // #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)


#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
void WebPage::setFocusedNode(Node* node)
{
    m_focusedNode = node;
    didFocusedRectsChanged();
}

void WebPage::didFocusedRectsChanged()
{
    Vector<IntRect> rects;
    calcFocusedRects(m_focusedNode.get(), rects);
    m_focusedRects = rects;
    send(Messages::WebPageProxy::DidFocusedRectsChanged(m_focusedRects));
}

void WebPage::clearFocusedNode()
{
    if (m_focusedNode)
        setFocusedNode(0);
}
#endif

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
void WebPage::setSpatialNavigationEnabled(bool enabled)
{
    m_page->settings()->setSpatialNavigationEnabled(enabled);

    if (m_focusedNode && m_focusedNode->renderer())
        m_focusedNode->renderer()->repaint();

    if (enabled) {
        Frame* frame = m_page->focusController()->focusedOrMainFrame();
        if (frame) {
            PlatformMouseEvent fakeMouseMove(IntPoint(-1, -1), IntPoint(-1, -1), NoButton, PlatformEvent::MouseMoved, 0, false, false, false, false, currentTime());
            frame->eventHandler()->mouseMoved(fakeMouseMove);
        }

        setFocusedNode(frame && frame->document() ? frame->document()->focusedElement() : 0);
        if (!m_focusedNode)
            m_page->focusController()->advanceFocus(FocusDirectionForward, 0);
    } else
        setFocusedNode(0);
}
#endif

} // namespace WebKit
