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

#ifndef WebHitTestResult_h
#define WebHitTestResult_h

#include "APIObject.h"
#include <WebCore/IntRect.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#include "DataReference.h"
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include <WebCore/Color.h>
#endif
#include <wtf/HashMap.h>
#endif

namespace CoreIPC {
class ArgumentDecoder;
class ArgumentEncoder;
}

namespace WebCore {
class HitTestResult;
}

namespace WebKit {

class WebFrame;

class WebHitTestResult : public TypedAPIObject<APIObject::TypeHitTestResult> {
public:
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    enum HitTestResultContext{
        HitTestResultContextDocument = 1 << 1,
        HitTestResultContextLink = 1 << 2,
        HitTestResultContextImage = 1 << 3,
        HitTestResultContextMedia = 1 << 4,
        HitTestResultContextSelection = 1 << 5,
        HitTestResultContextEditable = 1 << 6,
        HitTestResultContextText = 1 << 7,
        HitTestResultDragSupport = 1 << 8
    };

    enum HitTestMode {
        HitTestModeDefault = 1 << 1,
        HitTestModeNodeData = 1 << 2,
        HitTestModeImageData = 1 << 3,
        HitTestModeSetFocus = 1 << 4,
        HitTestModeAll = HitTestModeDefault | HitTestModeNodeData | HitTestModeImageData | HitTestModeSetFocus
    };
#endif
    struct Data {
        String absoluteImageURL;
        String absolutePDFURL;
        String absoluteLinkURL;
        String absoluteMediaURL;
        String linkLabel;
        String linkTitle;
        bool isContentEditable;
        WebCore::IntRect elementBoundingBox;
        bool isScrollbar;
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
        unsigned int context;
        unsigned int hitTestMode;
        bool isDragSupport;
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        Vector<WebCore::IntRect> focusedRects;
        WebCore::Color focusedColor;
#endif

        struct NodeData {
            String tagName;
            String nodeValue;
            typedef HashMap<String, String> AttributeMap;
            AttributeMap attributeMap;
        };
        NodeData nodeData;

        struct ImageData {
            Vector<uint8_t> data;
            String fileNameExtension;
        } ;
        ImageData imageData;
#endif // #if ENABLE(TIZEN_WEBKIT2_HIT_TEST)

        Data();
        explicit Data(const WebCore::HitTestResult&);
        ~Data();

        void encode(CoreIPC::ArgumentEncoder&) const;
        static bool decode(CoreIPC::ArgumentDecoder&, WebHitTestResult::Data&);

        WebCore::IntRect elementBoundingBoxInWindowCoordinates(const WebCore::HitTestResult&);
    };

    static PassRefPtr<WebHitTestResult> create(const WebHitTestResult::Data&);

    String absoluteImageURL() const { return m_data.absoluteImageURL; }
    String absolutePDFURL() const { return m_data.absolutePDFURL; }
    String absoluteLinkURL() const { return m_data.absoluteLinkURL; }
    String absoluteMediaURL() const { return m_data.absoluteMediaURL; }

    String linkLabel() const { return m_data.linkLabel; }
    String linkTitle() const { return m_data.linkTitle; }

    bool isContentEditable() const { return m_data.isContentEditable; }

    WebCore::IntRect elementBoundingBox() const { return m_data.elementBoundingBox; }

    bool isScrollbar() const { return m_data.isScrollbar; }
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    bool isDragSupport() const { return m_data.isDragSupport; }
#endif

private:
    explicit WebHitTestResult(const WebHitTestResult::Data& hitTestResultData)
        : m_data(hitTestResultData)
    {
    }

    Data m_data;
};

} // namespace WebKit

#endif // WebHitTestResult_h
