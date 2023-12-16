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
#include "config.h"
#include "WebPageSerializerTizen.h"

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "Arguments.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DocumentType.h"
#include "Element.h"
#include "FrameLoader.h"
#include "HTMLAllCollection.h"
#include "HTMLElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLInputElement.h"
#include "HTMLMetaElement.h"
#include "HTMLNames.h"
#include "KURL.h"
#include "MemoryCache.h"
#include "TextEncoding.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebFrameLoaderClient.h"
#include "WebPageProxyMessages.h"
#include "markup.h"

#include <WebCore/Frame.h>
#include <WebCore/ResourceBuffer.h>
#include <wtf/DataLog.h>

using namespace WebCore;

namespace WebKit {

// Maximum length of data buffer which is used to save generated
// html content data. This is a soft limit which might be passed if a very large
// contegious string is found in the page.
static const unsigned dataBufferCapacity = 65536;

WebPageSerializerTizen::SerializeDomParam::SerializeDomParam(const KURL& url,
                                                            const TextEncoding& textEncoding,
                                                            Document* document)
    : url(url)
    , textEncoding(textEncoding)
    , document(document)
    , isHTMLDocument(document->isHTMLDocument())
    , haveSeenDocType(false)
    , haveAddedCharsetDeclaration(false)
    , skipMetaElement(0)
    , isInScriptOrStyleTag(false)
    , haveAddedXMLProcessingDirective(false)
    , haveAddedContentsBeforeEnd(false)
{
}


bool elementHasLegalLinkAttribute(const Element* element,
                                  const QualifiedName& attrName)
{
    if (attrName == HTMLNames::srcAttr) {
        // Check src attribute.
        if (element->hasTagName(HTMLNames::imgTag)
            || element->hasTagName(HTMLNames::scriptTag)
            || element->hasTagName(HTMLNames::iframeTag)
            || element->hasTagName(HTMLNames::frameTag))
            return true;
        if (element->hasTagName(HTMLNames::inputTag)) {
            const HTMLInputElement* input =
            static_cast<const HTMLInputElement*>(element);
            if (input->isImageButton())
                return true;
        }
    } else if (attrName == HTMLNames::hrefAttr) {
        // Check href attribute.
        if (element->hasTagName(HTMLNames::linkTag)
            || element->hasTagName(HTMLNames::aTag)
            || element->hasTagName(HTMLNames::areaTag))
            return true;
    } else if (attrName == HTMLNames::actionAttr) {
        if (element->hasTagName(HTMLNames::formTag))
            return true;
    } else if (attrName == HTMLNames::backgroundAttr) {
        if (element->hasTagName(HTMLNames::bodyTag)
            || element->hasTagName(HTMLNames::tableTag)
            || element->hasTagName(HTMLNames::trTag)
            || element->hasTagName(HTMLNames::tdTag))
            return true;
    } else if (attrName == HTMLNames::citeAttr) {
        if (element->hasTagName(HTMLNames::blockquoteTag)
            || element->hasTagName(HTMLNames::qTag)
            || element->hasTagName(HTMLNames::delTag)
            || element->hasTagName(HTMLNames::insTag))
            return true;
    } else if (attrName == HTMLNames::classidAttr
               || attrName == HTMLNames::dataAttr) {
        if (element->hasTagName(HTMLNames::objectTag))
            return true;
    } else if (attrName == HTMLNames::codebaseAttr) {
        if (element->hasTagName(HTMLNames::objectTag)
            || element->hasTagName(HTMLNames::appletTag))
            return true;
    }
    return false;
}

String WebPageSerializerTizen::generateMetaCharsetDeclaration(const String& charset)
{
    return makeString("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=", static_cast<const String&>(charset), "\">");
}

String WebPageSerializerTizen::generateMarkOfTheWebDeclaration(const KURL& url)
{
    return String::format("\n<!-- saved from url=(%04d)%s -->\n",
                          static_cast<int>(url.string().utf8().length()),
                          url.string().utf8().data());
}

String WebPageSerializerTizen::generateBaseTagDeclaration(const String& baseTarget)
{
    if (baseTarget.isEmpty())
        return makeString("<base href=\".\">");
    return makeString("<base href=\".\" target=\"", static_cast<const String&>(baseTarget), "\">");
}

String WebPageSerializerTizen::preActionBeforeSerializeOpenTag(const Element* element
                          , SerializeDomParam* param, bool* needSkip)
{
    StringBuilder result;
    *needSkip = false;
    if (param->isHTMLDocument) {
        // Skip the open tag of original META tag which declare charset since we
        // have overrided the META which have correct charset declaration after
        // serializing open tag of HEAD element.
        if (element->hasTagName(HTMLNames::metaTag)) {
            const HTMLMetaElement* meta = static_cast<const HTMLMetaElement*>(element);
            // Check whether the META tag has declared charset or not.
            String equiv = meta->httpEquiv();
            if (equalIgnoringCase(equiv, "content-type")) {
                String content = meta->content();
                if (content.length() && content.contains("charset", false)) {
                    // Find META tag declared charset, we need to skip it when
                    // serializing DOM.
                    param->skipMetaElement = element;
                    *needSkip = true;
                }
            }
        } else if (element->hasTagName(HTMLNames::htmlTag)) {
            // Check something before processing the open tag of HEAD element.
            // First we add doc type declaration if original document has it.
            if (!param->haveSeenDocType) {
                param->haveSeenDocType = true;
                result.append(createMarkup(param->document->doctype()));
            }

            // Add MOTW declaration before html tag.
            result.append(generateMarkOfTheWebDeclaration(param->url));
        } else if (element->hasTagName(HTMLNames::baseTag)) {
            // Comment the BASE tag when serializing dom.
            result.append("<!--");
        }
    } else {
        // Write XML declaration.
        if (!param->haveAddedXMLProcessingDirective) {
            param->haveAddedXMLProcessingDirective = true;
            // Get encoding info.
            String xmlEncoding = param->document->xmlEncoding();
            if (xmlEncoding.isEmpty())
                xmlEncoding = param->document->encoding();
            if (xmlEncoding.isEmpty())
                xmlEncoding = UTF8Encoding().name();
            result.append("<?xml version=\"");
            result.append(param->document->xmlVersion());
            result.append("\" encoding=\"");
            result.append(xmlEncoding);
            if (param->document->xmlStandalone())
                result.append("\" standalone=\"yes");
            result.append("\"?>\n");
        }
        // Add doc type declaration if original document has it.
        if (!param->haveSeenDocType) {
            param->haveSeenDocType = true;
            result.append(createMarkup(param->document->doctype()));
        }
    }
    return result.toString();
}

String WebPageSerializerTizen::postActionAfterSerializeOpenTag(
    const Element* element, SerializeDomParam* param)
{
    StringBuilder result;
    param->haveAddedContentsBeforeEnd = false;
    if (!param->isHTMLDocument)
        return result.toString();
    // Check after processing the open tag of HEAD element
    if (!param->haveAddedCharsetDeclaration
        && element->hasTagName(HTMLNames::headTag)) {
        param->haveAddedCharsetDeclaration = true;
        // Check meta element. WebKit only pre-parse the first 512 bytes
        // of the document. If the whole <HEAD> is larger and meta is the
        // end of head part, then this kind of pages aren't decoded correctly
        // because of this issue. So when we serialize the DOM, we need to
        // make sure the meta will in first child of head tag.
        // See http://bugs.webkit.org/show_bug.cgi?id=16621.
        // First we generate new content for writing correct META element.
        result.append(generateMetaCharsetDeclaration(
            String(param->textEncoding.name())));
        param->haveAddedContentsBeforeEnd = true;
        // Will search each META which has charset declaration, and skip them all
        // in PreActionBeforeSerializeOpenTag.
    } else if (element->hasTagName(HTMLNames::scriptTag)
        || element->hasTagName(HTMLNames::styleTag)) {
        param->isInScriptOrStyleTag = true;
    }
    return result.toString();
}

String WebPageSerializerTizen::preActionBeforeSerializeEndTag(
    const Element* element, SerializeDomParam* param, bool* needSkip)
{
    String result;
    *needSkip = false;
    if (!param->isHTMLDocument)
        return result;
    // Skip the end tag of original META tag which declare charset.
    // Need not to check whether it's META tag since we guarantee
    // skipMetaElement is definitely META tag if it's not 0.
    if (param->skipMetaElement == element)
        *needSkip = true;
    else if (element->hasTagName(HTMLNames::scriptTag)
        || element->hasTagName(HTMLNames::styleTag)) {
        ASSERT(param->isInScriptOrStyleTag);
        param->isInScriptOrStyleTag = false;
    }
    return result;
}

// After we finish serializing end tag of a element, we give the target
// element a chance to do some post work to add some additional data.
String WebPageSerializerTizen::postActionAfterSerializeEndTag(
    const Element* element, SerializeDomParam* param)
{
    StringBuilder result;

    if (!param->isHTMLDocument)
        return result.toString();
    // Comment the BASE tag when serializing DOM.
    if (element->hasTagName(HTMLNames::baseTag)) {
        result.append("-->");
        // Append a new base tag declaration.
        result.append(generateBaseTagDeclaration(
            param->document->baseTarget()));
    }
    return result.toString();
}

void WebPageSerializerTizen::saveHTMLContentToBuffer(
    const String& result)
{
    m_dataBuffer.append(result);
}

void WebPageSerializerTizen::openTagToString(Element* element,
                                            SerializeDomParam* param)
{
    bool needSkip;
    // Do pre action for open tag.
    StringBuilder result;
    result.append(preActionBeforeSerializeOpenTag(element, param, &needSkip));

    if (needSkip)
        return;
    // Add open tag
    result.append("<");
    result.append(element->nodeName().lower());
    // Go through all attributes and serialize them.
    if (element->hasAttributes()) {
        unsigned numAttrs = element->attributeCount();
        for (unsigned i = 0; i < numAttrs; i++) {
            result.append(" ");
            // Add attribute pair
            const WebCore::Attribute *attribute = element->attributeItem(i);
            result.append(attribute->name().toString());
            result.append("=\"");
            if (!attribute->value().isEmpty()) {
                const String& attrValue = attribute->value();
                // Check whether we need to replace some resource links
                // with local resource paths.
                const QualifiedName& attrName = attribute->name();
                if (elementHasLegalLinkAttribute(element, attrName)) {
                    // For links start with "javascript:", we do not change it.
                    if (attrValue.startsWith("javascript:", false)) {
                        result.append(attrValue);
                    } else if (element->hasTagName(HTMLNames::iframeTag)
                            || element->hasTagName(HTMLNames::frameTag)) {
                        if ((!element || !element->isFrameOwnerElement())
                            && !element->hasTagName(HTMLNames::frameTag))
                            return;
                        HTMLFrameOwnerElement* frameElement = static_cast<HTMLFrameOwnerElement*>(element);
                        Frame* frame = frameElement->contentFrame();
                        if (!frame)
                            return;
                        WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();
                        size_t frameIndex = m_frames.find(webFrame);
                        result.append("frame_");
                        result.append(String::number(frameIndex));
                        result.append("/");
                        KURL attrURL_frame(ParsedURLString, attrValue);
                        if (!attrURL_frame.lastPathComponent().isEmpty())
                            result.append(attrURL_frame.lastPathComponent());
                        else
                            result.append("index.html");
                    } else if (element->hasTagName(HTMLNames::linkTag) || element->hasTagName(HTMLNames::aTag)
                            || element->hasTagName(HTMLNames::areaTag)){
                        result.append(attrValue);
                    } else {
                        result.append(m_localDirectoryName);
                        result.append("/");
                        size_t index = attrValue.reverseFind('/');
                        if (index != WTF::notFound)
                            result.append(attrValue.substring(index + 1));
                    }
                } else
                    result.append(attrValue);
            }
            result.append("\"");
        }
    }
    String addedContents = postActionAfterSerializeOpenTag(element, param);
    // Complete the open tag for element when it has child/children.
    if (element->hasChildNodes() || param->haveAddedContentsBeforeEnd)
        result.append(">");
    // Append the added contents generate in  post action of open tag.
    result.append(addedContents);
    // Save the result to data buffer.
    saveHTMLContentToBuffer(result.toString());
}

// Serialize end tag of an specified element.
void WebPageSerializerTizen::endTagToString(Element* element,
                                           SerializeDomParam* param)
{
    bool needSkip;
    // Do pre action for end tag.
    StringBuilder result;
    result.append(preActionBeforeSerializeEndTag(element,
                                                   param,
                                                   &needSkip));
    if (needSkip)
        return;
    // Write end tag when element has child/children.
    if (element->hasChildNodes() || param->haveAddedContentsBeforeEnd) {
        result.append("</");
        result.append(element->nodeName().lower());
        result.append(">");
    } else {
        // Check whether we have to write end tag for empty element.
        if (param->isHTMLDocument) {
            result.append(">");
            // FIXME: This code is horribly wrong. WebPageSerializerTizen must die.
            if (!static_cast<const HTMLElement*>(element)->ieForbidsInsertHTML()) {
                // We need to write end tag when it is required.
                result.append("</");
                result.append(element->nodeName().lower());
                result.append(">");
            }
        } else {
            // For xml base document.
            result.append(" />");
        }
    }
    // Do post action for end tag.
    result.append(postActionAfterSerializeEndTag(element, param));
    // Save the result to data buffer.
    saveHTMLContentToBuffer(result.toString());
}

void WebPageSerializerTizen::buildContentForNode(Node* node,
                                                SerializeDomParam* param)
{
    switch (node->nodeType()) {
    case Node::ELEMENT_NODE:
        // Process open tag of element.
        openTagToString(static_cast<Element*>(node), param);
        // Walk through the children nodes and process it.
        for (Node *child = node->firstChild(); child; child = child->nextSibling())
            buildContentForNode(child, param);
        // Process end tag of element.
        endTagToString(static_cast<Element*>(node), param);
        break;
    case Node::TEXT_NODE:
        saveHTMLContentToBuffer(createMarkup(node));
        break;
    case Node::ATTRIBUTE_NODE:
    case Node::DOCUMENT_NODE:
    case Node::DOCUMENT_FRAGMENT_NODE:
        break;
    // Document type node can be in DOM?
    case Node::DOCUMENT_TYPE_NODE:
        param->haveSeenDocType = true;
    default:
        // For other type node, call default action.
        saveHTMLContentToBuffer(createMarkup(node));
        break;
    }
}

WebPageSerializerTizen::WebPageSerializerTizen(WebFrame* frame, String& localDir)
    : m_framesCollected(false)
    , m_localDirectoryName(localDir)
{
    // Must specify available webframe.
    ASSERT(frame);
    m_specifiedWebFrame = static_cast<WebFrame*>(frame);
    ASSERT(m_dataBuffer.isEmpty());
}


void WebPageSerializerTizen::collectTargetFrames()
{
    ASSERT(!m_framesCollected);
    m_framesCollected = true;
    // First, process main frame.
    m_frames.append(m_specifiedWebFrame);
    // Return now if user only needs to serialize specified frame, not including
    // all sub-frames.
    // Collect all frames inside the specified frame.
    for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
        WebFrame* currentFrame = m_frames[i];
        // Get current using document.
        Document* currentDoc = currentFrame->coreFrame()->document();
        // Go through sub-frames.
        RefPtr<HTMLCollection> all = currentDoc->all();
        for (unsigned i = 0; i < all->length(); ++i) {
            Node* node = all->item(i);
            if (!node->isHTMLElement())
                continue;
            Element* element = static_cast<Element*>(node);
            if ((element && element->isFrameOwnerElement()) || (element->hasTagName(HTMLNames::iframeTag)
                || element->hasTagName(HTMLNames::frameTag))) {
                HTMLFrameOwnerElement* frameElement = static_cast<HTMLFrameOwnerElement*>(element);
                Frame* frame = frameElement->contentFrame();
                if (!frame)
                    return;
                // Get the absolute link
                WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();
                if (webFrame)
                    m_frames.append(webFrame);
            }
        }

    }
}

bool WebPageSerializerTizen::getSerializedPageContent(WebPage* page, String& localDir)
{
    WebFrame* frame = page->mainWebFrame();
    WebPageSerializerTizen serializer(frame, localDir);

    return serializer.serialize();
}

bool WebPageSerializerTizen::serialize()
{
    if (!m_framesCollected)
        collectTargetFrames();
    bool didSerialization = false;
    KURL mainURL = m_specifiedWebFrame->coreFrame()->document()->url();
    for (unsigned i = 0; i < m_frames.size(); ++i) {
        WebFrame* webFrame = m_frames[i];
        m_currentFrame = webFrame;
        Document* document = webFrame->coreFrame()->document();
        const KURL& url = document->url();
        if (!url.isValid())
            continue;
        didSerialization = true;
        String encoding = document->encoding();
        const TextEncoding& textEncoding = encoding.isEmpty() ? UTF8Encoding() : TextEncoding(encoding);
        SerializeDomParam param(url, textEncoding, document);

        Element* documentElement = document->documentElement();
        if (documentElement)
            buildContentForNode(documentElement, &param);
        getAllSubResource(document);

        String fileName;
        String frameDirName("");

        if (m_currentFrame->isMainFrame()) {
            size_t index = m_localDirectoryName.reverseFind('_');
            if (index) {
                fileName = String(m_localDirectoryName.substring(0, index));
                fileName.append(".html");
            } else
               fileName = String("index.html");
        } else {
            frameDirName.append("frame_");
            frameDirName.append(String::number(i));
            if (!url.isEmpty() && !url.lastPathComponent().isEmpty()) {
                fileName.append(frameDirName);
                fileName.append("/");
                fileName.append(url.lastPathComponent());
                size_t index_filename = fileName.reverseFind('.');
                if (index_filename == WTF::notFound)
                    fileName.append(".html");
            } else {
                fileName = frameDirName;
                fileName.append("/index.html");
            }
        }
        m_specifiedWebFrame->page()->send(Messages::WebPageProxy::SaveSerializedHTMLDataForMainPage(m_dataBuffer.toString(), fileName));
        m_specifiedWebFrame->page()->send(Messages::WebPageProxy::SaveSubresourcesData(m_allResource));
        m_dataBuffer.clear();
        m_allResource.clear();
    }
    ASSERT(m_dataBuffer.isEmpty());
    return didSerialization;
}

void WebPageSerializerTizen::getAllSubResource(Document* document)
{
    if (!document)
        return;
    CachedResourceLoader::DocumentResourceMap subresources = document->cachedResourceLoader()->allCachedResources();
    for (CachedResourceLoader::DocumentResourceMap::iterator it = subresources.begin(); it != subresources.end(); ++it) {
        KURL url;
        url = KURL(KURL(), (it->key));
        if (!it->value || !(it->value).get() || !(it->value).get()->resourceBuffer())
            continue;
        PassRefPtr<ResourceBuffer> resourcebuf = (it->value).get()->resourceBuffer();
        String subResourceDirName("");
        if(m_currentFrame != m_specifiedWebFrame) {
            size_t pos = m_frames.find(m_currentFrame);
            subResourceDirName.append("frame_");
            subResourceDirName.append(String::number(pos));
            subResourceDirName.append("/");
            subResourceDirName.append(m_localDirectoryName);
        } else
            subResourceDirName = m_localDirectoryName;
        String name = subResourceDirName;
        name.append("/");
        name.append(url.lastPathComponent());
        WebSubresourceTizen subResource(url, static_cast<unsigned>((it->value).get()->type()), name, resourcebuf->data(), resourcebuf->size());
        m_allResource.append(subResource);
    }
}

} // namespace WebKit

#endif // TIZEN_OFFLINE_PAGE_SAVE
