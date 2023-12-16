/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ewk_context_private_h
#define ewk_context_private_h

#include "WKEinaSharedString.h"
#include "ewk_context.h"
#include "ewk_object_private.h"
#include <JavaScriptCore/JSContextRef.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_APPLICATION_CACHE)
#include "ewk_security_origin_private.h"
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

using namespace WebKit;

class EwkApplicationCacheManager;
class EwkCookieManager;
class EwkFaviconDatabase;

namespace WebKit {
class ContextHistoryClientEfl;
class DownloadManagerEfl;
class RequestManagerClientEfl;
#if ENABLE(BATTERY_STATUS)
class BatteryProvider;
#endif
#if ENABLE(NETWORK_INFO)
class NetworkInfoProvider;
#endif
#if ENABLE(TIZEN_GEOLOCATION)
class GeolocationProviderTizen;
#endif
}

class EwkContext : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkContext)

    static PassRefPtr<EwkContext> findOrCreateWrapper(WKContextRef context);
    static PassRefPtr<EwkContext> create();
    static PassRefPtr<EwkContext> create(const String& injectedBundlePath);

    static EwkContext* defaultContext();

    ~EwkContext();

    EwkApplicationCacheManager* applicationCacheManager();
    EwkCookieManager* cookieManager();
    EwkDatabaseManager* databaseManager();

    bool setFaviconDatabaseDirectoryPath(const String& databaseDirectory);
    EwkFaviconDatabase* faviconDatabase();

    EwkStorageManager* storageManager() const;

    WebKit::RequestManagerClientEfl* requestManager();

    void addVisitedLink(const String& visitedURL);

    void setCacheModel(Ewk_Cache_Model);

    Ewk_Cache_Model cacheModel() const;

    WKContextRef wkContext() const { return m_context.get(); }

    WebKit::DownloadManagerEfl* downloadManager() const;

    WebKit::ContextHistoryClientEfl* historyClient();

#if ENABLE(NETSCAPE_PLUGIN_API)
    void setAdditionalPluginPath(const String&);
#endif

    void clearResourceCache();

    JSGlobalContextRef jsGlobalContext();

    static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo);
    static void didReceiveSynchronousMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void* clientInfo);
    void setMessageFromInjectedBundleCallback(Ewk_Context_Message_From_Injected_Bundle_Cb, void*);
    void processReceivedMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData);

#if ENABLE(TIZEN_WEBKIT2_PROXY)
    void setProxyAddress(const char*);
    const char* proxyAddress() const { return m_proxyAddress; }
#endif

#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
    void clearAllDecodedData();
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
    void setApplicationCachePath(const char* path);
    void applicationCachePath(Ewk_Application_Cache_Path_Get_Callback callback, void* userData);
    void applicationCacheQuota(Ewk_Application_Cache_Quota_Get_Callback callback, void* userData);
    void applicationCacheUsageForOrigin(const EwkSecurityOrigin* origin, Ewk_Application_Cache_Usage_For_Origin_Get_Callback callback, void* userData);
    void setApplicationCacheQuota(int64_t quota);
    void setApplicationCacheQuotaForOrigin(const EwkSecurityOrigin* origin, int64_t quota);
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

#if ENABLE(TIZEN_WEB_STORAGE)
    void deleteWebStorageAll();
    void deleteWebStorageOrigin(EwkSecurityOrigin* origin);
    void webStorageOrigins(Ewk_Web_Storage_Origins_Get_Callback callback, void* userData);
    void setWebStoragePath(const char* path);
    void webStoragePath(Ewk_Web_Storage_Path_Get_Callback callback, void* userData);
    void webStorageUsageForOrigin(Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* userData);
#endif // ENABLE(TIZEN_WEB_STORAGE)

#if ENABLE(TIZEN_SQL_DATABASE)
    uint64_t defaultDatabaseQuota() const { return m_defaultDatabaseQuota; }
    void setDefaultDatabaseQuota(uint64_t);

    void deleteWebDatabaseAll();
    void deleteWebDatabase(Ewk_Security_Origin* origin);
    void webDatabaseOrigins(Ewk_Web_Database_Origins_Get_Callback callback, void* userData);
    void setWebDatabasePath(const char* path);
#endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_EXTENSIBLE_API)
    void setTizenExtensibleAPI(Ewk_Extensible_API extensibleAPI, Eina_Bool enable);
#endif // ENABLE(TIZEN_EXTENSIBLE_API)

#if ENABLE(TIZEN_INDEXED_DATABASE)
    void deleteIndexedDatabaseAll();
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    const char* certificateFile() const { return m_certificateFile; }
    bool setCertificateFile(const char*);
#endif

private:
    explicit EwkContext(WKContextRef);

    void ensureFaviconDatabase();

    WKRetainPtr<WKContextRef> m_context;

    OwnPtr<EwkApplicationCacheManager> m_applicationCacheManager;
    OwnPtr<EwkCookieManager> m_cookieManager;
    OwnPtr<EwkDatabaseManager> m_databaseManager;
    OwnPtr<EwkFaviconDatabase> m_faviconDatabase;
    OwnPtr<EwkStorageManager> m_storageManager;
#if ENABLE(BATTERY_STATUS)
    RefPtr<WebKit::BatteryProvider> m_batteryProvider;
#endif
#if ENABLE(NETWORK_INFO)
    RefPtr<WebKit::NetworkInfoProvider> m_networkInfoProvider;
#endif
    OwnPtr<WebKit::DownloadManagerEfl> m_downloadManager;
    OwnPtr<WebKit::RequestManagerClientEfl> m_requestManagerClient;

    OwnPtr<WebKit::ContextHistoryClientEfl> m_historyClient;

    JSGlobalContextRef m_jsGlobalContext;

    struct {
        Ewk_Context_Message_From_Injected_Bundle_Cb callback;
        void* userData;
    } m_callbackForMessageFromInjectedBundle;

    WKEinaSharedString m_proxyAddress;

#if ENABLE(TIZEN_SQL_DATABASE)
    uint64_t m_defaultDatabaseQuota;
#endif // ENABLE(TIZEN_SQL_DATABASE)
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    WKEinaSharedString m_certificateFile;
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    OwnPtr<GeolocationProviderTizen> m_geolocationProvider;
#endif
};

#endif // ewk_context_private_h
