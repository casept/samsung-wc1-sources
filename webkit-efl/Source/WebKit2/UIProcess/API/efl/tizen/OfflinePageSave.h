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

#ifndef OfflinePageSave_h
#define OfflinePageSave_h

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)

#include "WebPageProxy.h"
#include <wtf/PassOwnPtr.h>
#include "KURL.h"

class EwkView;

namespace WebKit {

class OfflinePageSave {
public:
    static PassOwnPtr<OfflinePageSave> create(EwkView* viewImpl)
    {
        return adoptPtr(new OfflinePageSave(viewImpl));
    }
    ~OfflinePageSave();

    void startOfflinePageSave(String& path, String& url, String& title);
    void saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName);
    void saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData);

private:
    explicit OfflinePageSave(EwkView*);

    void savePageFinished();
    void savePageError();

    void extractProperDirectoryName(String& directoryName, String& title, String& url);

    EwkView* m_view;

    String m_pageSaveDirectoryPath;
    String m_subresourceDirectoryName;
    bool m_didErrorOccured;
};

} // namespace WebKit

#endif // TIZEN_OFFLINE_PAGE_SAVE
#endif // OfflinePageSave_h
