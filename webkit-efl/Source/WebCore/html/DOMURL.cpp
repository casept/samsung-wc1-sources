/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Motorola Mobility Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(BLOB)

#include "DOMURL.h"

#include "ActiveDOMObject.h"
#include "Blob.h"
#include "BlobURL.h"
#include "KURL.h"
#include "MemoryCache.h"
#include "PublicURLManager.h"
#include "ResourceRequest.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include <wtf/MainThread.h>

namespace WebCore {

String DOMURL::createObjectURL(ScriptExecutionContext* scriptExecutionContext, Blob* blob)
{
    if (!scriptExecutionContext || !blob)
        return String();
    return createPublicURL(scriptExecutionContext, blob);
}

String DOMURL::createPublicURL(ScriptExecutionContext* scriptExecutionContext, URLRegistrable* registrable)
{
    KURL publicURL = BlobURL::createPublicURL(scriptExecutionContext->securityOrigin());
    if (publicURL.isEmpty())
        return String();

    scriptExecutionContext->publicURLManager().registerURL(scriptExecutionContext->securityOrigin(), publicURL, registrable);

    return publicURL.string();
}

void DOMURL::revokeObjectURL(ScriptExecutionContext* scriptExecutionContext, const String& urlString)
{
    if (!scriptExecutionContext)
        return;

    KURL url(KURL(), urlString);
    ResourceRequest request(url);
#if ENABLE(CACHE_PARTITIONING)
    request.setCachePartition(scriptExecutionContext->topOrigin()->cachePartition());
#endif
    MemoryCache::removeRequestFromCache(scriptExecutionContext, request);

    scriptExecutionContext->publicURLManager().revoke(url);
}

} // namespace WebCore

#endif // ENABLE(BLOB)
