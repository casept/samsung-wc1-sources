/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WKKeyValueStorageManager.h"

#include "WKAPICast.h"
#include "WebKeyValueStorageManager.h"

using namespace WebKit;

WKTypeID WKKeyValueStorageManagerGetTypeID()
{
    return toAPI(WebKeyValueStorageManager::APIType);
}

void WKKeyValueStorageManagerGetKeyValueStorageOrigins(WKKeyValueStorageManagerRef keyValueStorageManagerRef, void* context, WKKeyValueStorageManagerGetKeyValueStorageOriginsFunction callback)
{
    toImpl(keyValueStorageManagerRef)->getKeyValueStorageOrigins(ArrayCallback::create(context, callback));
}

void WKKeyValueStorageManagerDeleteEntriesForOrigin(WKKeyValueStorageManagerRef keyValueStorageManagerRef, WKSecurityOriginRef originRef)
{
    toImpl(keyValueStorageManagerRef)->deleteEntriesForOrigin(toImpl(originRef));
}

void WKKeyValueStorageManagerDeleteAllEntries(WKKeyValueStorageManagerRef keyValueStorageManagerRef)
{
    toImpl(keyValueStorageManagerRef)->deleteAllEntries();
}

void WKKeyValueStorageManagerGetKeyValueStoragePath(WKKeyValueStorageManagerRef keyValueStorageManagerRef, void* context, WKKeyValueStorageManagerGetKeyValueStoragePathFunction callback)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    toImpl(keyValueStorageManagerRef)->getKeyValueStoragePath(WebStorageStringCallback::create(context, callback));
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

void WKKeyValueStorageManagerGetKeyValueStorageUsageForOrigin(WKKeyValueStorageManagerRef keyValueStorageManagerRef, void* context, WKKeyValueStorageManagerGetKeyValueStorageUsageForOriginFunction callback, WKSecurityOriginRef originRef)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    toImpl(keyValueStorageManagerRef)->getKeyValueStorageUsageForOrigin(WebStorageInt64Callback::create(context, callback), toImpl(originRef));
#endif // ENABLE(TIZEN_WEB_STORAGE)
}