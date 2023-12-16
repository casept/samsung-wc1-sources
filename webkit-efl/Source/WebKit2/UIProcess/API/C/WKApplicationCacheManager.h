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

#ifndef WKApplicationCacheManager_h
#define WKApplicationCacheManager_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKApplicationCacheManagerGetTypeID();

typedef void (*WKApplicationCacheManagerGetApplicationCacheOriginsFunction)(WKArrayRef, WKErrorRef, void*);
WK_EXPORT void WKApplicationCacheManagerGetApplicationCacheOrigins(WKApplicationCacheManagerRef applicationCacheManager, void* context, WKApplicationCacheManagerGetApplicationCacheOriginsFunction function);

WK_EXPORT void WKApplicationCacheManagerDeleteEntriesForOrigin(WKApplicationCacheManagerRef applicationCacheManager, WKSecurityOriginRef origin);
WK_EXPORT void WKApplicationCacheManagerDeleteAllEntries(WKApplicationCacheManagerRef applicationCacheManager);

// #if ENABLE(TIZEN_APPLICATION_CACHE)
typedef void (*WKApplicationCacheManagerGetApplicationCachePathFunction)(WKStringRef, WKErrorRef, void*);
WK_EXPORT void WKApplicationCacheManagerGetApplicationCachePath(WKApplicationCacheManagerRef applicationCacheManager, void* context, WKApplicationCacheManagerGetApplicationCachePathFunction function);

typedef void (*WKApplicationCacheManagerGetApplicationCacheQuotaFunction)(WKInt64Ref, WKErrorRef, void*);
WK_EXPORT void WKApplicationCacheManagerGetApplicationCacheQuota(WKApplicationCacheManagerRef applicationCacheManager, void* context, WKApplicationCacheManagerGetApplicationCacheQuotaFunction function);

typedef void (*WKApplicationCacheManagerGetApplicationCacheUsageForOriginFunction)(WKInt64Ref, WKErrorRef, void*);
WK_EXPORT void WKApplicationCacheManagerGetApplicationCacheUsageForOrigin(WKApplicationCacheManagerRef applicationCacheManager, void* context, WKSecurityOriginRef originRef, WKApplicationCacheManagerGetApplicationCacheUsageForOriginFunction function);

WK_EXPORT void WKApplicationCacheManagerSetApplicationCacheQuota(WKApplicationCacheManagerRef applicationCacheManager, int64_t quota);
WK_EXPORT void WKApplicationCacheManagerSetApplicationCacheQuotaForOrigin(WKApplicationCacheManagerRef applicationCacheManagerRef, WKSecurityOriginRef originRef, int64_t quota);
// #endif // ENABLE(TIZEN_APPLICATION_CACHE)

#ifdef __cplusplus
}
#endif

#endif // WKApplicationCacheManager_h
