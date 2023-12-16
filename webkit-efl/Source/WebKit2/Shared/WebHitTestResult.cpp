/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebHitTestResult.h"

#include "WebCoreArgumentCoders.h"
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/KURL.h>
#include <WebCore/Node.h>
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#include <wtf/text/StringHash.h>
#endif
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebHitTestResult> WebHitTestResult::create(const WebHitTestResult::Data& hitTestResultData)
{
    return adoptRef(new WebHitTestResult(hitTestResultData));
}

WebHitTestResult::Data::Data()
{
}

WebHitTestResult::Data::Data(const HitTestResult& hitTestResult)
    : absoluteImageURL(hitTestResult.absoluteImageURL().string())
    , absolutePDFURL(hitTestResult.absolutePDFURL().string())
    , absoluteLinkURL(hitTestResult.absoluteLinkURL().string())
    , absoluteMediaURL(hitTestResult.absoluteMediaURL().string())
    , linkLabel(hitTestResult.textContent())
    , linkTitle(hitTestResult.titleDisplayString())
    , isContentEditable(hitTestResult.isContentEditable())
    , elementBoundingBox(elementBoundingBoxInWindowCoordinates(hitTestResult))
    , isScrollbar(hitTestResult.scrollbar())
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    , context(HitTestResultContextDocument)
    , hitTestMode(HitTestModeDefault)
    , isDragSupport(hitTestResult.isDragSupport())
#endif
{
}

WebHitTestResult::Data::~Data()
{
}

void WebHitTestResult::Data::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << absoluteImageURL;
    encoder << absolutePDFURL;
    encoder << absoluteLinkURL;
    encoder << absoluteMediaURL;
    encoder << linkLabel;
    encoder << linkTitle;
    encoder << isContentEditable;
    encoder << elementBoundingBox;
    encoder << isScrollbar;
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    encoder << context;
    encoder << hitTestMode;
    encoder << isDragSupport;
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    encoder << focusedRects;
    encoder << focusedColor;
#endif

    if (hitTestMode & HitTestModeNodeData) {
        encoder << nodeData.tagName;
        encoder << nodeData.nodeValue;
        encoder << nodeData.attributeMap;
    }

    if ((hitTestMode & HitTestModeImageData) && (context & HitTestResultContextImage)) {
        encoder << CoreIPC::DataReference(imageData.data);
        encoder << imageData.fileNameExtension;
    }
#endif
}

bool WebHitTestResult::Data::decode(CoreIPC::ArgumentDecoder& decoder, WebHitTestResult::Data& hitTestResultData)
{
    if (!decoder.decode(hitTestResultData.absoluteImageURL)
        || !decoder.decode(hitTestResultData.absolutePDFURL)
        || !decoder.decode(hitTestResultData.absoluteLinkURL)
        || !decoder.decode(hitTestResultData.absoluteMediaURL)
        || !decoder.decode(hitTestResultData.linkLabel)
        || !decoder.decode(hitTestResultData.linkTitle)
        || !decoder.decode(hitTestResultData.isContentEditable)
        || !decoder.decode(hitTestResultData.elementBoundingBox)
        || !decoder.decode(hitTestResultData.isScrollbar))
        return false;

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    if (!decoder.decode(hitTestResultData.context)
        || !decoder.decode(hitTestResultData.hitTestMode)
        || !decoder.decode(hitTestResultData.isDragSupport)
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        || !decoder.decode(hitTestResultData.focusedRects)
        || !decoder.decode(hitTestResultData.focusedColor)
#endif
        )
        return false;

    if (hitTestResultData.hitTestMode & HitTestModeNodeData) {
        if (!decoder.decode(hitTestResultData.nodeData.tagName)
            || !decoder.decode(hitTestResultData.nodeData.nodeValue)
            || !decoder.decode(hitTestResultData.nodeData.attributeMap))
            return false;
    }

    if ((hitTestResultData.hitTestMode & HitTestModeImageData) && (hitTestResultData.context & HitTestResultContextImage)) {
        CoreIPC::DataReference data;
        if (!decoder.decode(data))
            return false;
        hitTestResultData.imageData.data.append(data.data(), data.size());

        if (!decoder.decode(hitTestResultData.imageData.fileNameExtension))
            return false;
    }
#endif

    return true;
}

IntRect WebHitTestResult::Data::elementBoundingBoxInWindowCoordinates(const HitTestResult& hitTestResult)
{
    Node* node = hitTestResult.innerNonSharedNode();
    if (!node)
        return IntRect();

    Frame* frame = node->document()->frame();
    if (!frame)
        return IntRect();

    FrameView* view = frame->view();
    if (!view)
        return IntRect();

    return view->contentsToWindow(node->pixelSnappedBoundingBox());
}

} // WebKit
