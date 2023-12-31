/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WebApplicationCacheManagerProxy_h
#define WebApplicationCacheManagerProxy_h

#include "APIObject.h"
#include "GenericCallback.h"
#include "ImmutableArray.h"
#include "MessageReceiver.h"
#include "WebContextSupplement.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace CoreIPC {
    class Connection;
}

namespace WebKit {

class WebSecurityOrigin;
struct SecurityOriginData;

typedef GenericCallback<WKArrayRef> ArrayCallback;
#if ENABLE(TIZEN_APPLICATION_CACHE)
typedef GenericCallback<WKStringRef, StringImpl*> AppCacheStringCallback;
typedef GenericCallback<WKInt64Ref> AppCacheInt64Callback;
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

class WebApplicationCacheManagerProxy : public TypedAPIObject<APIObject::TypeApplicationCacheManager>, public WebContextSupplement, private CoreIPC::MessageReceiver {
public:
    static const char* supplementName();

    static PassRefPtr<WebApplicationCacheManagerProxy> create(WebContext*);
    virtual ~WebApplicationCacheManagerProxy();

    void getApplicationCacheOrigins(PassRefPtr<ArrayCallback>);
    void deleteEntriesForOrigin(WebSecurityOrigin*);
    void deleteAllEntries();

#if ENABLE(TIZEN_APPLICATION_CACHE)
    void getApplicationCachePath(PassRefPtr<AppCacheStringCallback>);
    void getApplicationCacheQuota(PassRefPtr<AppCacheInt64Callback>);
    void getApplicationCacheUsageForOrigin(PassRefPtr<AppCacheInt64Callback>, WebSecurityOrigin*);
    void setApplicationCacheQuota(int64_t quota);
    void setApplicationCacheQuotaForOrigin(WebSecurityOrigin*, int64_t quota);
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

    using APIObject::ref;
    using APIObject::deref;

private:
    explicit WebApplicationCacheManagerProxy(WebContext*);

    void didGetApplicationCacheOrigins(const Vector<SecurityOriginData>&, uint64_t callbackID);

    // WebContextSupplement
    virtual void contextDestroyed() OVERRIDE;
    virtual void processDidClose(WebProcessProxy*) OVERRIDE;
    virtual bool shouldTerminate(WebProcessProxy*) const OVERRIDE;
    virtual void refWebContextSupplement() OVERRIDE;
    virtual void derefWebContextSupplement() OVERRIDE;

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

#if ENABLE(TIZEN_APPLICATION_CACHE)
    void didGetApplicationCachePath(const String& path, uint64_t callbackID);
    void didGetApplicationCacheQuota(const int64_t quota, uint64_t callbackID);
    void didGetApplicationCacheUsageForOrigin(const int64_t usage, uint64_t callbackID);
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

    HashMap<uint64_t, RefPtr<ArrayCallback>> m_arrayCallbacks;
#if ENABLE(TIZEN_APPLICATION_CACHE)
    HashMap<uint64_t, RefPtr<AppCacheStringCallback> > m_appCacheStringCallbacks;
    HashMap<uint64_t, RefPtr<AppCacheInt64Callback> > m_appCacheInt64Callbacks;
#endif // ENABLE(TIZEN_APPLICATION_CACHE)
};

} // namespace WebKit

#endif // WebApplicationCacheManagerProxy_h
