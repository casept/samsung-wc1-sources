/*
 * Copyright (C) 2013 Samsung Electronics. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ClipboardTizen.h"

#if ENABLE(TIZEN_CLIPBOARD)

#include "CachedImage.h"
#include "DataTransferItemList.h"
#include "DragData.h"
#include "Editor.h"
#include "EditorClient.h"
#include "FileList.h"
#include "Frame.h"
#include "Image.h"
#include "NotImplemented.h"
#include "RenderImage.h"
#include "markup.h"
#include <cairo.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

PassRefPtr<Clipboard> Editor::newGeneralClipboard(ClipboardAccessPolicy policy, Frame* frame)
{
    return ClipboardTizen::create(policy, Clipboard::CopyAndPaste, frame);
}

PassRefPtr<Clipboard> Clipboard::create(ClipboardAccessPolicy policy, DragData* dragData, Frame* frame)
{
    return ClipboardTizen::create(policy, dragData->platformData(), DragAndDrop, frame);
}

ClipboardTizen::ClipboardTizen(ClipboardAccessPolicy policy, ClipboardType clipboardType, Frame* frame)
    : Clipboard(policy, clipboardType)
    , m_dataObject(DataObjectTizen::sharedDataObject())
    , m_frame(frame)
{
}

ClipboardTizen::ClipboardTizen(ClipboardAccessPolicy policy, PassRefPtr<DataObjectTizen> dataObject, ClipboardType clipboardType, Frame* frame)
    : Clipboard(policy, clipboardType)
    , m_dataObject(dataObject)
    , m_frame(frame)
{
    makeImageDirectory();
}

ClipboardTizen::~ClipboardTizen()
{
}

static String getDataTypeFromString(const String& rawType)
{
    String type(rawType.stripWhiteSpace());

    if (type == "Text" || type == "text")
        return "PlainText";
    if (type == "URL" || type == "url")
        return "URL";

    // From the Mac port: Ignore any trailing charset - JS strings are
    // Unicode, which encapsulates the charset issue.
    if (type == "text/plain" || type.startsWith("text/plain;"))
        return "PlainText";
    if (type == "text/html" || type.startsWith("text/html;"))
        return "Markup";
    if (type == "Files" || type == "text/uri-list" || type.startsWith("text/uri-list;"))
        return "URIList";

    // Not a known type, so just default to using the text portion.
    return "Unknown";
}

void ClipboardTizen::clearData(const String& typeString)
{
    if (policy() != ClipboardWritable)
        return;

    String type = getDataTypeFromString(typeString);
    if (type == "PlainText")
        m_dataObject->clearText();
    else if(type == "Markup")
        m_dataObject->clearMarkup();
    else if(type == "URIList" || type == "URL")
        m_dataObject->clearURIList();
    else if(type == "Image")
        m_dataObject->clearImage();
    else if(type == "Unknown")
        m_dataObject->clearAll();

    if (!isForDragAndDrop())
        m_frame->editor().client()->clearClipboardData();
}

void ClipboardTizen::clearData()
{
}

void ClipboardTizen::clearAllData()
{
    if (policy() != ClipboardWritable)
        return;

    // We do not clear filenames. According to the spec: "The clearData() method
    // does not affect whether any files were included in the drag, so the types
    // attribute's list might still not be empty after calling clearData() (it would
    // still contain the "Files" string if any files were included in the drag)."
    m_dataObject->clearAllExceptFilenames();

    if (!isForDragAndDrop())
        m_frame->editor().client()->clearClipboardData();
}

String ClipboardTizen::getData(const String& typeString) const
{
    if (policy() != ClipboardReadable)
        return String();

    String type = getDataTypeFromString(typeString);
    if (type == "PlainText")
        return m_dataObject->text();
    if (type == "Markup")
        return m_dataObject->markup();
    if (type == "URIList")
        return m_dataObject->uriList();
    if (type == "URL")
        return m_dataObject->url();

    return String();
}

void ClipboardTizen::setData(const String& typeString, const String& data)
{
    if (policy() != ClipboardWritable)
        return;

    bool success = false;
    String type = getDataTypeFromString(typeString);
    if (type == "PlainText") {
        m_dataObject->setText(data);
        success = true;
    } else if(type == "Markup") {
        m_dataObject->setMarkup(data);
        success = true;
    } else if(type == "URIList" || type == "URL") {
        m_dataObject->setURIList(data);
        success = true;
    }

    if(success && !isForDragAndDrop())
        m_frame->editor().client()->setClipboardData(data, type);
}

ListHashSet<String> ClipboardTizen::types() const
{
    if (policy() != ClipboardReadable && policy() != ClipboardTypesReadable)
        return ListHashSet<String>();

    ListHashSet<String> types;
    if (m_dataObject->hasText()) {
        types.add("text/plain");
        types.add("Text");
        types.add("text");
    }

    if (m_dataObject->hasMarkup())
        types.add("text/html");

    if (m_dataObject->hasURIList()) {
        types.add("text/uri-list");
        types.add("URL");
        types.add("url");
    }

    if (m_dataObject->hasFilenames())
        types.add("Files");

    return types;
}

PassRefPtr<FileList> ClipboardTizen::files() const
{
    RefPtr<FileList> files = FileList::create();
    if (policy() != ClipboardReadable)
        return files.release();

    const Vector<String>& filenames = m_dataObject->filenames();
    for (size_t i = 0; i < filenames.size(); i++)
        files->append(File::create(filenames[i]));

    return files.release();
}

void ClipboardTizen::setDragImage(CachedImage* image, const IntPoint& location)
{
    setDragImage(image, 0, location);
}

void ClipboardTizen::setDragImageElement(Node* element, const IntPoint& location)
{
    setDragImage(0, element, location);
}

void ClipboardTizen::setDragImage(CachedImage* image, Node* element, const IntPoint& location)
{
    if (policy() != ClipboardImageWritable && policy() != ClipboardWritable)
        return;

    if (m_dragImage)
        m_dragImage->removeClient(this);
    m_dragImage = image;
    if (m_dragImage)
        m_dragImage->addClient(this);

    m_dragLoc = location;
    m_dragImageElement = element;
}

DragImageRef ClipboardTizen::createDragImage(IntPoint& location) const
{
    location = m_dragLoc;

    if (m_dragImage)
        return createDragImageFromImage(m_dragImage->image());
    if (m_dragImageElement && m_frame)
        return m_frame->nodeImage(m_dragImageElement.get());

    return 0;
}

void ClipboardTizen::makeImageDirectory()
{
    m_imageDirectory = pathByAppendingComponent(homeDirectoryPath(), ".webkit/copiedImages/");
    makeAllDirectories(m_imageDirectory);

    // Delete all png files in the image directory because they are temporary files.
    Vector<String> files = listDirectory(m_imageDirectory, String("*.png"));
    for (size_t i = 0, size = files.size(); i < size; ++i)
        deleteFile(files[i]);
}

static CachedImage* getCachedImage(Element* element)
{
    // Attempt to pull CachedImage from element
    ASSERT(element);
    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isImage())
        return 0;

    RenderImage* image = static_cast<RenderImage*>(renderer);
    if (image->cachedImage() && !image->cachedImage()->errorOccurred())
        return image->cachedImage();

    return 0;
}

void ClipboardTizen::declareAndWriteDragImage(Element* element, const KURL& url, const String& label, Frame* frame)
{
    CachedImage* cachedImage = getCachedImage(element);
    if (!cachedImage || !cachedImage->imageForRenderer(element->renderer()) || !cachedImage->isLoaded())
        return;

    String imagePath = "";
    String uniqueString = String::number(time(0));

    imagePath = m_imageDirectory;
    imagePath.append(uniqueString);
    imagePath.append(".png");

    RefPtr<cairo_surface_t> surface = cachedImage->imageForRenderer(element->renderer())->nativeImageForCurrentFrame();
    if (!surface)
        return;

    cairo_surface_write_to_png(surface.get(), imagePath.utf8().data());

    imagePath.insert("file://", 0);

    m_dataObject->setURL(url, label);
    m_dataObject->setMarkup(createMarkup(element, IncludeNode, 0, ResolveAllURLs));
    m_dataObject->setImage(imagePath);

    if (!isForDragAndDrop())
        frame->editor().client()->setClipboardData(imagePath, String("Image"));
}

void ClipboardTizen::writeURL(const KURL& url, const String& label, Frame*)
{
    m_dataObject->setURL(url, label);

    if (!isForDragAndDrop())
        m_frame->editor().client()->setClipboardData(url, String("URL"));
}

void ClipboardTizen::writeRange(Range* range, Frame* frame)
{
    String markup = createMarkup(range, 0, AnnotateForInterchange, false, ResolveNonLocalURLs);
    m_dataObject->setText(frame->editor().selectedText());
    m_dataObject->setMarkup(markup);

    if (!isForDragAndDrop())
        m_frame->editor().client()->setClipboardData(markup, String("Markup"));
}

void ClipboardTizen::writePlainText(const String& text)
{
    m_dataObject->setText(text);

    if (!isForDragAndDrop())
        m_frame->editor().client()->setClipboardData(text, String("PlainText"));
}

bool ClipboardTizen::hasData()
{
    return m_dataObject->hasText() || m_dataObject->hasMarkup()
        || m_dataObject->hasURIList() || m_dataObject->hasImage();
}

#if ENABLE(DATA_TRANSFER_ITEMS)
PassRefPtr<DataTransferItemList> ClipboardTizen::items()
{
    notImplemented();
    return 0;
}
#endif


}

#endif // ENABLE(TIZEN_CLIPBOARD)
