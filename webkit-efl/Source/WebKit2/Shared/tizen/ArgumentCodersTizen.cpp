/*
 * Copyright (C) 2012 Samsung Electronic.
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
#include "ArgumentCodersTizen.h"

#if ENABLE(TIZEN_DRAG_SUPPORT)

#include "NotImplemented.h"
#include "ShareableBitmap.h"
#include "WebCoreArgumentCoders.h"

#include <cairo.h>
#include <Ecore_Evas.h>
#include <WebCore/DataObjectTizen.h>
#include <WebCore/DragData.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/PlatformContextCairo.h>


using namespace WebCore;
using namespace WebKit;

namespace CoreIPC {

static void encodeDataObject(ArgumentEncoder& encoder, const DataObjectTizen* dataObject)
{
    bool hasText = dataObject->hasText();
    encoder << hasText;
    if (hasText)
        encoder << dataObject->text();

    bool hasMarkup = dataObject->hasMarkup();
    encoder << hasMarkup;
    if (hasMarkup)
        encoder << dataObject->markup();

    bool hasURL = dataObject->hasURL();
    encoder << hasURL;
    if (hasURL)
        encoder << dataObject->url().string();

    bool hasURIList = dataObject->hasURIList();
    encoder << hasURIList;
    if (hasURIList)
        encoder << dataObject->uriList();
}

static bool decodeDataObject(ArgumentDecoder& decoder, RefPtr<DataObjectTizen>& dataObject)
{
    RefPtr<DataObjectTizen> data = DataObjectTizen::create();

    bool hasText;
    if (!decoder.decode(hasText))
        return false;
    if (hasText) {
        String text;
        if (!decoder.decode(text))
            return false;
        data->setText(text);
    }

    bool hasMarkup;
    if (!decoder.decode(hasMarkup))
        return false;
    if (hasMarkup) {
        String markup;
        if (!decoder.decode(markup))
            return false;
        data->setMarkup(markup);
    }

    bool hasURL;
    if (!decoder.decode(hasURL))
        return false;
    if (hasURL) {
        String url;
        if (!decoder.decode(url))
            return false;
        data->setURL(KURL(KURL(), url), String());
    }

    bool hasURIList;
    if (!decoder.decode(hasURIList))
        return false;
    if (hasURIList) {
        String uriList;
        if (!decoder.decode(uriList))
            return false;
        data->setURIList(uriList);
    }

    dataObject = data;

    return true;
}

void ArgumentCoder<DragData>::encode(ArgumentEncoder& encoder, const DragData& dragData)
{
    encoder << dragData.clientPosition();
    encoder << dragData.globalPosition();
    encoder << static_cast<uint64_t>(dragData.draggingSourceOperationMask());
    encoder << static_cast<uint64_t>(dragData.flags());

    DataObjectTizen* platformData = dragData.platformData();
    encoder << static_cast<bool>(platformData);
    if (platformData)
        encodeDataObject(encoder, platformData);
}

bool ArgumentCoder<DragData>::decode(ArgumentDecoder& decoder, DragData& dragData)
{
    IntPoint clientPosition;
    if (!decoder.decode(clientPosition))
        return false;

    IntPoint globalPosition;
    if (!decoder.decode(globalPosition))
        return false;

    uint64_t sourceOperationMask;
    if (!decoder.decode(sourceOperationMask))
        return false;

    uint64_t flags;
    if (!decoder.decode(flags))
        return false;

    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;

    RefPtr<DataObjectTizen> platformData;
    if (hasPlatformData) {
        if (!decodeDataObject(decoder, platformData))
            return false;
    }

    dragData = DragData(platformData.release().leakRef(), clientPosition, globalPosition, static_cast<DragOperation>(sourceOperationMask), static_cast<DragApplicationFlags>(flags));

    return true;
}

}

#endif // ENABLE(TIZEN_DRAG_SUPPORT)
