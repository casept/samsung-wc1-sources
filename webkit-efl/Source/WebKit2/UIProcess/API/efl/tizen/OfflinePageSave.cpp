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
#include "OfflinePageSave.h"

#include "EwkView.h"

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include <WebCore/FileSystem.h>

using namespace WebCore;

namespace WebKit {

OfflinePageSave::OfflinePageSave(EwkView* view)
    : m_view(view)
    , m_didErrorOccured(false)
{
}

OfflinePageSave::~OfflinePageSave()
{
}

void OfflinePageSave::savePageError()
{
    m_view->smartCallback<EwkViewCallbacks::PageSaveError>().call();
}

void OfflinePageSave::savePageFinished()
{
    m_view->smartCallback<EwkViewCallbacks::PageSaveSuccess>().call();
}

void OfflinePageSave::startOfflinePageSave(String& path, String& url, String& title)
{
    if (path.isEmpty()) {
        m_view->smartCallback<EwkViewCallbacks::PageSaveError>().call();
        return;
    }

    String subresourceFolderName;
    extractProperDirectoryName(subresourceFolderName, title, url);

    m_pageSaveDirectoryPath = path;
    m_subresourceDirectoryName = subresourceFolderName;
    m_view->page()->startOfflinePageSave(subresourceFolderName);

}

void OfflinePageSave::saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData)
{
    if (m_didErrorOccured) {
        Vector<String> files = listDirectory(m_pageSaveDirectoryPath, String("*.html"));
        for (size_t i = 0; i < files.size(); ++i)
            deleteFile(files[i].utf8().data());

        savePageError();
        m_didErrorOccured = false;
        return;
    }

    String subresourceDirectory = pathByAppendingComponent(m_pageSaveDirectoryPath, m_subresourceDirectoryName);
    if (!fileExists(subresourceDirectory))
        makeAllDirectories(subresourceDirectory);

    for (size_t i = 0; i < subresourceData.size(); ++i) {
        String fullPath = pathByAppendingComponent(m_pageSaveDirectoryPath, subresourceData[i].m_fileName);
        size_t lastIndexOfSlash = fullPath.reverseFind('/');
        if (lastIndexOfSlash != notFound)
            makeAllDirectories(fullPath.substring(0, lastIndexOfSlash));

        PlatformFileHandle fileHandle = openFile(fullPath, OpenForWrite);
        if (!isHandleValid(fileHandle))
            continue;

        ASSERT(subresourceData[i].m_sharedMemory);
        if (writeToFile(fileHandle, static_cast<const char*>(subresourceData[i].m_sharedMemory->data()), subresourceData[i].m_sharedMemory->size()) == -1) {
            closeFile(fileHandle);
            deleteFile(fullPath);
            continue;
        }
        closeFile(fileHandle);
    }
    savePageFinished();
}

void OfflinePageSave::saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName)
{
    String fullPath = pathByAppendingComponent(m_pageSaveDirectoryPath, fileName);
    if (!fileExists(m_pageSaveDirectoryPath))
        makeAllDirectories(m_pageSaveDirectoryPath);

    size_t lastIndexOfSlash = fullPath.reverseFind('/');
    if (lastIndexOfSlash != notFound)
        makeAllDirectories(fullPath.substring(0, lastIndexOfSlash));

    PlatformFileHandle fileHandle = openFile(fullPath, OpenForWrite);
    if (!isHandleValid(fileHandle)) {
        m_didErrorOccured = true;
        return;
    }

    if (writeToFile(fileHandle, serializedData.utf8().data(), serializedData.length()) == -1) {
        closeFile(fileHandle);
        deleteFile(fullPath);
        m_didErrorOccured = true;
        return;
    }
    closeFile(fileHandle);
}

void OfflinePageSave::extractProperDirectoryName(String& directoryName, String& title, String& url)
{
    String subresourceFolderNameSuffix = "_files";
    String subresourceFolderName = "resource" + subresourceFolderNameSuffix;
    const char specialCharacters[] = { '\\', '/', '?', '%', '*', ':', '|', '"', '<', '>', '.', '\0'};

    if (!title.isEmpty()) {
        subresourceFolderName = title + subresourceFolderNameSuffix;
    } else if (!url.isEmpty()) {
        KURL pageURL(ParsedURLString, url);
        String lastPathComponent = pageURL.lastPathComponent();
        lastPathComponent.replace('.','\0');
        subresourceFolderName= lastPathComponent.utf8().data() + subresourceFolderNameSuffix;
    }

    for (int  i = 0; specialCharacters[i] != '\0'; i++) {
        while (subresourceFolderName.contains(specialCharacters[i]) )
            subresourceFolderName.replace(specialCharacters[i],' ');
    }

    directoryName = subresourceFolderName;
}

} // namespace WebKit

#endif // TIZEN_OFFLINE_PAGE_SAVE
