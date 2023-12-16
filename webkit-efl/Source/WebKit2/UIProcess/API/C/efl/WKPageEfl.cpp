/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THE IMPLIED WARRANTIES OF MERCHANTAwBILITY AND FITNESS FOR A PARTICULAR
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
#include "WKPageEfl.h"

#include "WKAPICast.h"
#include "WebPageProxy.h"

using namespace WebKit;

void WKPageSetUIPopupMenuClient(WKPageRef pageRef, const WKPageUIPopupMenuClient* wkClient)
{
    toImpl(pageRef)->initializeUIPopupMenuClient(wkClient);
}

void WKPageSetPageTizenClient(WKPageRef pageRef, const WKPageTizenClient* wkClient)
{
#if PLATFORM(TIZEN)
    toImpl(pageRef)->initializeTizenClient(wkClient);
#endif // PLATFORM(TIZEN)
}

void WKPageGetSnapshotPdfFile(WKPageRef page, WKSize surfaceSize, WKStringRef fileName)
{
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
    toImpl(page)->createPagesToPDF(toIntSize(surfaceSize), toImpl(fileName)->string());
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(surfaceSize);
    UNUSED_PARAM(fileName);
#endif // ENABLE(TIZEN_MOBILE_WEB_PRINT)
}

void WKPageGetWebAppCapable(WKPageRef page, void* context, WKPageGetWebAppCapableFunction callback)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    toImpl(page)->getWebAppCapable(BooleanCallback::create(context, callback));
#endif
}

void WKPageGetWebAppIconURL(WKPageRef page, void* context, WKPageGetWebAppIconURLFunction callback)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    toImpl(page)->getWebAppIconURL(StringCallback::create(context, callback));
#endif
}

void WKPageGetWebAppIconURLs(WKPageRef page, void* context, WKPageGetWebAppIconURLsFunction callback)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    toImpl(page)->getWebAppIconURLs(DictionaryCallback::create(context, callback));
#endif
}

void WKPageGetWebStorageQuota(WKPageRef page, void* context, WKPageGetWebStorageQuotaFunction callback)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    toImpl(page)->getWebStorageQuotaBytes(WebStorageQuotaCallback::create(context, callback));
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

void WKPageSetWebStorageQuota(WKPageRef page, uint32_t quota)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    toImpl(page)->setWebStorageQuotaBytes(quota);
#endif // ENABLE(TIZEN_WEB_STORAGE)
}
