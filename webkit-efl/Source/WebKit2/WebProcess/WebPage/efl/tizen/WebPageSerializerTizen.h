/*
 * Copyright (C) 2013 Samsung Electronics All rights reserved.
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
#ifndef WebPageSerializerTizen_h
#define WebPageSerializerTizen_h

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "WebSubresourceTizen.h"

#include "WebPage.h"
#include <WebCore/HTMLElement.h>

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class Document;
class Element;
class Node;
class TextEncoding;
class KURL;
}

namespace WebKit {
class WebFrame;
class WebPageSerializerTizen {
public:
    bool serialize();
    static bool getSerializedPageContent(WebPage* page, String&);
    String generateMetaCharsetDeclaration(const String& charset);
    String generateMarkOfTheWebDeclaration(const WebCore::KURL& url);
    String generateBaseTagDeclaration(const String& baseTarget);
    WebPageSerializerTizen(WebFrame* frame, String& localDir);

private:
    struct SerializeDomParam {
        const WebCore::KURL& url;
        const WebCore::TextEncoding& textEncoding;
        WebCore::Document* document;
        bool isHTMLDocument; // document.isHTMLDocument()
        bool haveSeenDocType;
        bool haveAddedCharsetDeclaration;
        // This meta element need to be skipped when serializing DOM.
        const WebCore::Element* skipMetaElement;
        // Flag indicates we are in script or style tag.
        bool isInScriptOrStyleTag;
        bool haveAddedXMLProcessingDirective;
        // Flag indicates whether we have added additional contents before end tag.
        // This flag will be re-assigned in each call of function
        // PostActionAfterSerializeOpenTag and it could be changed in function
        // PreActionBeforeSerializeEndTag if the function adds new contents into
        // serialization stream.
        bool haveAddedContentsBeforeEnd;

        SerializeDomParam(const WebCore::KURL&, const WebCore::TextEncoding&, WebCore::Document*);
    };

    //Get All the Sub-resources
    void getAllSubResource(WebCore::Document *doc);

    // Collect all target frames which need to be serialized.
    void collectTargetFrames();

    // Before we begin serializing open tag of a element, we give the target
    // element a chance to do some work prior to add some additional data.
    WTF::String preActionBeforeSerializeOpenTag(const WebCore::Element* element,
                                                    SerializeDomParam* param,
                                                    bool* needSkip);

    // After we finish serializing open tag of a element, we give the target
    // element a chance to do some post work to add some additional data.
    WTF::String postActionAfterSerializeOpenTag(const WebCore::Element* element,
                                                    SerializeDomParam* param);

    // Before we begin serializing end tag of a element, we give the target
    // element a chance to do some work prior to add some additional data.
    WTF::String preActionBeforeSerializeEndTag(const WebCore::Element* element,
                                                   SerializeDomParam* param,
                                                   bool* needSkip);
    // After we finish serializing end tag of a element, we give the target
    // element a chance to do some post work to add some additional data.
    WTF::String postActionAfterSerializeEndTag(const WebCore::Element* element,
                                                   SerializeDomParam* param);
    // Save generated html content to data buffer.
    void saveHTMLContentToBuffer(const WTF::String& content);

    // Serialize open tag of an specified element.
    void openTagToString(WebCore::Element*,
                         SerializeDomParam* param);
    // Serialize end tag of an specified element.
    void endTagToString(WebCore::Element*,
                        SerializeDomParam* param);
    // Build content for a specified node
    void buildContentForNode(WebCore::Node*,
                             SerializeDomParam* param);

    // Specified frame which need to be serialized;
    WebFrame* m_specifiedWebFrame;

    //The current Frame that is being processed right now;
    WebFrame* m_currentFrame;

    // Data buffer for saving result of serialized DOM data.
    StringBuilder m_dataBuffer;

    // Flag indicates whether we have collected all frames which need to be
    // serialized or not;
    bool m_framesCollected;

    // Local directory name of all local resource files.
    WTF::String m_localDirectoryName;

    // Vector for saving all frames which need to be serialized.
    WTF::Vector<WebFrame*> m_frames;

    WTF::Vector<WebSubresourceTizen> m_allResource;
};

} // namespace WebKit

#endif // TIZEN_OFFLINE_PAGE_SAVE
#endif
