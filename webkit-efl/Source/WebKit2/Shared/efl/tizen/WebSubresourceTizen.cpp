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
#include "WebSubresourceTizen.h"

#include "Arguments.h"
#include "WebCoreArgumentCoders.h"

using namespace WebCore;

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
namespace WebKit {

WebSubresourceTizen::WebSubresourceTizen()
{
}

WebSubresourceTizen::WebSubresourceTizen(const WebCore::KURL& url, const unsigned mimeType, WTF::String fileName, const char* subResourcesData, unsigned size)
    : m_url(url)
    , m_mimeType(mimeType)
    , m_fileName(fileName)
    , m_sharedMemory(0)
    , m_size(size)
{
    m_sharedMemory = SharedMemory::create(m_size);
    memcpy(m_sharedMemory->data(), subResourcesData, m_size);
}

void WebSubresourceTizen::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    SharedMemory::Handle hd;
    m_sharedMemory->createHandle(hd, SharedMemory::ReadOnly);
    encoder << m_url;
    encoder << m_mimeType;
    encoder << m_fileName;
    encoder << hd;
    encoder << m_size;
}

bool WebSubresourceTizen::decode(CoreIPC::ArgumentDecoder& decoder, WebSubresourceTizen& output)
{
    SharedMemory::Handle hd;
    if (!decoder.decode(output.m_url))
        return false;
    if (!decoder.decode(output.m_mimeType))
        return false;
    if (!decoder.decode(output.m_fileName))
        return false;
    if (!decoder.decode(hd))
        return false;
    if (!decoder.decode(output.m_size))
        return false;

    output.m_sharedMemory = SharedMemory::create(hd, SharedMemory::ReadOnly);
    return true;
}

} // end of WebKit namespace

#endif // TIZEN_OFFLINE_PAGE_SAVE
