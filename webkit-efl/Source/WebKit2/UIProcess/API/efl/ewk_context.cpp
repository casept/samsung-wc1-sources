/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "ewk_context.h"

#include "BatteryProvider.h"
#include "ContextHistoryClientEfl.h"
#include "DownloadManagerEfl.h"
#include "NetworkInfoProvider.h"
#include "NotImplemented.h"
#include "RequestManagerClientEfl.h"
#include "WKAPICast.h"
#include "WKContextPrivate.h"
#include "WKContextSoup.h"
#include "WKNumber.h"
#include "WKString.h"
#include "WebContext.h"
#include "WebIconDatabase.h"
#include "ewk_application_cache_manager_private.h"
#include "ewk_context_private.h"
#include "ewk_cookie_manager_private.h"
#include "ewk_database_manager_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_private.h"
#include "ewk_storage_manager_private.h"
#include "ewk_url_scheme_request_private.h"
#include <JavaScriptCore/JSContextRef.h>
#include <WebCore/FileSystem.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/Language.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

#if ENABLE(SPELLCHECK)
#include "TextCheckerClientEfl.h"
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
#include "WKApplicationCacheManager.h"
#include "WKContextPrivate.h"
#include "WKSecurityOrigin.h"
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

#if ENABLE(TIZEN_SQL_DATABASE)
#include "ewk_security_origin_private.h"
#endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
#include "ProcessLauncher.h"
#include <stdlib.h>
#endif // ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)

#if ENABLE(TIZEN_APPLICATION_CACHE)
#include "WKArray.h"
#endif

#if ENABLE(TIZEN_GEOLOCATION)
#include "GeolocationProviderTizen.h"
#endif

#if PLATFORM(TIZEN)
typedef struct Ewk_Context_Callback_Context {
    union {
#if ENABLE(TIZEN_APPLICATION_CACHE)
        Ewk_Application_Cache_Quota_Get_Callback applicationCacheQuotaCallback;
        Ewk_Application_Cache_Usage_For_Origin_Get_Callback applicationCacheUsageForOriginCallback;
        Ewk_Application_Cache_Path_Get_Callback applicationCachePathCallback;
#endif // ENABLE(TIZEN_APPLICATION_CACHE)
#if ENABLE(TIZEN_WEB_STORAGE)
        Ewk_Web_Storage_Origins_Get_Callback webStorageOriginsCallback;
        Ewk_Web_Storage_Usage_Get_Callback webStorageUsageCallback;
        Ewk_Web_Storage_Path_Get_Callback webStoragePathCallback;
#endif // ENABLE(TIZEN_WEB_STORAGE)
#if ENABLE(TIZEN_SQL_DATABASE)
        Ewk_Web_Database_Origins_Get_Callback webDatabaseOriginsCallback;
        Ewk_Web_Database_Quota_Get_Callback webDatabaseQuotaCallback;
        Ewk_Web_Database_Usage_Get_Callback webDatabaseUsageCallback;
        Ewk_Web_Database_Path_Get_Callback webDatabasePathCallback;
#endif // ENABLE(TIZEN_SQL_DATABASE)
    };
    void* userData;
} Ewk_Context_Callback_Context;
#endif // PLATFORM(TIZEN)

using namespace WebCore;
using namespace WebKit;

typedef HashMap<WKContextRef, EwkContext*> ContextMap;

static inline ContextMap& contextMap()
{
    DEFINE_STATIC_LOCAL(ContextMap, map, ());
    return map;
}

EwkContext::EwkContext(WKContextRef context)
    : m_context(context)
    , m_databaseManager(EwkDatabaseManager::create(WKContextGetDatabaseManager(context)))
    , m_storageManager(EwkStorageManager::create(WKContextGetKeyValueStorageManager(context)))
#if ENABLE(BATTERY_STATUS)
    , m_batteryProvider(BatteryProvider::create(context))
#endif
#if ENABLE(NETWORK_INFO)
    , m_networkInfoProvider(NetworkInfoProvider::create(context))
#endif
    , m_downloadManager(DownloadManagerEfl::create(context))
    , m_requestManagerClient(RequestManagerClientEfl::create(context))
    , m_historyClient(ContextHistoryClientEfl::create(context))
    , m_jsGlobalContext(0)
#if ENABLE(TIZEN_GEOLOCATION)
    , m_geolocationProvider(adoptPtr(new GeolocationProviderTizen(context)))
#endif
{
    ContextMap::AddResult result = contextMap().add(context, this);
    ASSERT_UNUSED(result, result.isNewEntry);

#if ENABLE(MEMORY_SAMPLER)
    static bool initializeMemorySampler = false;
    static const char environmentVariable[] = "SAMPLE_MEMORY";

    if (!initializeMemorySampler && getenv(environmentVariable)) {
        WKContextStartMemorySampler(context, adoptWK(WKDoubleCreate(0.0)).get());
        initializeMemorySampler = true;
    }
#endif

#if ENABLE(SPELLCHECK)
    // Load the default dictionary to show context menu spellchecking items
    // independently of checking spelling while typing setting.
    TextCheckerClientEfl::instance().ensureSpellCheckingLanguage();
#endif

    m_callbackForMessageFromInjectedBundle.callback = 0;
    m_callbackForMessageFromInjectedBundle.userData = 0;

#if ENABLE(TIZEN_SQL_DATABASE)
    m_defaultDatabaseQuota = 5 * 1024 * 1024;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

EwkContext::~EwkContext()
{
    ASSERT(contextMap().get(m_context.get()) == this);

    if (m_jsGlobalContext)
        JSGlobalContextRelease(m_jsGlobalContext);

    WKContextEnableProcessTermination(m_context.get());
    contextMap().remove(m_context.get());
}

PassRefPtr<EwkContext> EwkContext::findOrCreateWrapper(WKContextRef context)
{
    if (contextMap().contains(context))
        return contextMap().get(context);

    return adoptRef(new EwkContext(context));
}

PassRefPtr<EwkContext> EwkContext::create()
{
    return adoptRef(new EwkContext(adoptWK(WKContextCreate()).get()));
}

PassRefPtr<EwkContext> EwkContext::create(const String& injectedBundlePath)
{
    if (!fileExists(injectedBundlePath))
        return 0;

    WKRetainPtr<WKStringRef> path = adoptWK(toCopiedAPI(injectedBundlePath));

    return adoptRef(new EwkContext(adoptWK(WKContextCreateWithInjectedBundlePath(path.get())).get()));
}

EwkContext* EwkContext::defaultContext()
{
    static EwkContext* defaultInstance = create().leakRef();

    return defaultInstance;
}

EwkApplicationCacheManager* EwkContext::applicationCacheManager()
{
    if (!m_applicationCacheManager)
        m_applicationCacheManager = EwkApplicationCacheManager::create(WKContextGetApplicationCacheManager(m_context.get()));

    return m_applicationCacheManager.get();
}

EwkCookieManager* EwkContext::cookieManager()
{
    if (!m_cookieManager)
        m_cookieManager = EwkCookieManager::create(WKContextGetCookieManager(m_context.get()));

    return m_cookieManager.get();
}

EwkDatabaseManager* EwkContext::databaseManager()
{
    return m_databaseManager.get();
}

void EwkContext::ensureFaviconDatabase()
{
    if (m_faviconDatabase)
        return;

    m_faviconDatabase = EwkFaviconDatabase::create(WKContextGetIconDatabase(m_context.get()));
}

bool EwkContext::setFaviconDatabaseDirectoryPath(const String& databaseDirectory)
{
    ensureFaviconDatabase();
    // FIXME: Hole in WK2 API layering must be fixed when C API is available.
    WebIconDatabase* iconDatabase = toImpl(WKContextGetIconDatabase(m_context.get()));

    // The database path is already open so its path was
    // already set.
    if (iconDatabase->isOpen())
        return false;

    // If databaseDirectory is empty, we use the default database path for the platform.
    String databasePath = databaseDirectory.isEmpty() ? toImpl(m_context.get())->iconDatabasePath() : pathByAppendingComponent(databaseDirectory, WebCore::IconDatabase::defaultDatabaseFilename());
    toImpl(m_context.get())->setIconDatabasePath(databasePath);

    return true;
}

EwkFaviconDatabase* EwkContext::faviconDatabase()
{
    ensureFaviconDatabase();
    ASSERT(m_faviconDatabase);

    return m_faviconDatabase.get();
}

EwkStorageManager* EwkContext::storageManager() const
{
    return m_storageManager.get();
}

RequestManagerClientEfl* EwkContext::requestManager()
{
    return m_requestManagerClient.get();
}

void EwkContext::addVisitedLink(const String& visitedURL)
{
    WKContextAddVisitedLink(m_context.get(), adoptWK(toCopiedAPI(visitedURL)).get());
}

void EwkContext::setCacheModel(Ewk_Cache_Model cacheModel)
{
    WKContextSetCacheModel(m_context.get(), static_cast<WebKit::CacheModel>(cacheModel));
}

Ewk_Cache_Model EwkContext::cacheModel() const
{
    return static_cast<Ewk_Cache_Model>(WKContextGetCacheModel(m_context.get()));
}

#if ENABLE(NETSCAPE_PLUGIN_API)
void EwkContext::setAdditionalPluginPath(const String& path)
{
    // FIXME: Hole in WK2 API layering must be fixed when C API is available.
    toImpl(m_context.get())->setAdditionalPluginsDirectory(path);
}
#endif

void EwkContext::clearResourceCache()
{
    WKResourceCacheManagerClearCacheForAllOrigins(WKContextGetResourceCacheManager(m_context.get()), WKResourceCachesToClearAll);
}

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void EwkContext::setProxyAddress(const char* proxyAddress)
{
    if (m_proxyAddress != proxyAddress) {
        m_proxyAddress = proxyAddress;
        toImpl(m_context.get())->setProxyAddress(proxyAddress);
    }
}
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
static void didGetApplicationCachePath(WKStringRef path, WKErrorRef, void* context)
{
    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);

    int length = WKStringGetMaximumUTF8CStringSize(path);
    OwnArrayPtr<char> applicationCachePath = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(path, applicationCachePath.get(), length);

    // TIZEN_LOGI("path (%s)", applicationCachePath.get());
    applicationCacheContext->applicationCachePathCallback(eina_stringshare_add(applicationCachePath.get()), applicationCacheContext->userData);
    delete applicationCacheContext;
}

static void didGetApplicationCacheQuota(WKInt64Ref quota, WKErrorRef, void* context)
{
    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);
    // TIZEN_LOGI("quota (%d)", toImpl(quota)->value());
    applicationCacheContext->applicationCacheQuotaCallback(toImpl(quota)->value(), applicationCacheContext->userData);
    delete applicationCacheContext;
}

static void didGetApplicationCacheUsageForOrigin(WKInt64Ref usage, WKErrorRef, void* context)
{
    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);
    // TIZEN_LOGI("usage (%d)", toImpl(usage)->value());
    applicationCacheContext->applicationCacheUsageForOriginCallback(toImpl(usage)->value(), applicationCacheContext->userData);
    delete applicationCacheContext;
}

void EwkContext::setApplicationCachePath(const char* path)
{
    WKRetainPtr<WKStringRef> applicationCachePathRef(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetApplicationCacheDirectory(m_context.get(), applicationCachePathRef.get());
}

void EwkContext::applicationCachePath(Ewk_Application_Cache_Path_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->applicationCachePathCallback= callback;
    context->userData = userData;

    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(m_context.get());
    WKApplicationCacheManagerGetApplicationCachePath(applicationCacheRef, context, didGetApplicationCachePath);
}

void EwkContext::applicationCacheQuota(Ewk_Application_Cache_Quota_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->applicationCacheQuotaCallback = callback;
    context->userData = userData;

    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(m_context.get());
    WKApplicationCacheManagerGetApplicationCacheQuota(applicationCacheRef, context, didGetApplicationCacheQuota);
}

void EwkContext::applicationCacheUsageForOrigin(const EwkSecurityOrigin* origin, Ewk_Application_Cache_Usage_For_Origin_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->applicationCacheUsageForOriginCallback = callback;
    context->userData = userData;
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));

    WKApplicationCacheManagerRef applicationCache = WKContextGetApplicationCacheManager(m_context.get());
    WKApplicationCacheManagerGetApplicationCacheUsageForOrigin(applicationCache, context, originRef.get(), didGetApplicationCacheUsageForOrigin);
}

void EwkContext::setApplicationCacheQuota(int64_t quota)
{
    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(m_context.get());
    WKApplicationCacheManagerSetApplicationCacheQuota(applicationCacheRef, quota);
}

void EwkContext::setApplicationCacheQuotaForOrigin(const EwkSecurityOrigin* origin, int64_t quota)
{
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));

    WKApplicationCacheManagerRef applicationCache = WKContextGetApplicationCacheManager(m_context.get());
    WKApplicationCacheManagerSetApplicationCacheQuotaForOrigin(applicationCache, originRef.get(), quota);
}
#endif // ENABLE(TIZEN_APPLICATION_CACHE)

#if ENABLE(TIZEN_WEB_STORAGE)
static void didGetWebStorageOrigins(WKArrayRef origins, WKErrorRef /* error */, void* context)
{
    // TIZEN_LOGI("origin size(%d)", WKArrayGetSize(origins));
    Eina_List* originList = 0;

    for(size_t i = 0; i < WKArrayGetSize(origins); i++) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        originList = eina_list_append(originList, EwkSecurityOrigin::create(securityOriginRef).leakRef());
    }

    Ewk_Context_Callback_Context* webStorageContext = static_cast<Ewk_Context_Callback_Context*>(context);
    webStorageContext->webStorageOriginsCallback(originList, webStorageContext->userData);

    void* origin = 0;
    EINA_LIST_FREE(originList, origin)
        delete static_cast<EwkSecurityOrigin*>(origin);
    delete webStorageContext;
}

static void didGetWebStoragePath(WKStringRef path, WKErrorRef /* error */, void * context)
{
    Ewk_Context_Callback_Context* webStorageContext = static_cast<Ewk_Context_Callback_Context*>(context);

    int length = WKStringGetMaximumUTF8CStringSize(path);
    OwnArrayPtr<char> storagePath = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(path, storagePath.get(), length);

    // TIZEN_LOGI("path (%s)", storagePath.get());
    webStorageContext->webStoragePathCallback(eina_stringshare_add(storagePath.get()), webStorageContext->userData);
    delete webStorageContext;
}

static void didGetWebStorageUsage(WKInt64Ref usage, WKErrorRef /* error */, void* context)
{
    Ewk_Context_Callback_Context* webStorageContext = static_cast<Ewk_Context_Callback_Context*>(context);

    // TIZEN_LOGI("usage (%s)", toImpl(usage)->value());
    webStorageContext->webStorageUsageCallback(toImpl(usage)->value(), webStorageContext->userData);
    delete webStorageContext;
}

void EwkContext::deleteWebStorageAll()
{
    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(m_context.get());
    WKKeyValueStorageManagerDeleteAllEntries(storageManager);
}

void EwkContext::deleteWebStorageOrigin(EwkSecurityOrigin* origin)
{
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> securityOriginRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(m_context.get());
    WKKeyValueStorageManagerDeleteEntriesForOrigin(storageManager, securityOriginRef.get());
}

void EwkContext::webStorageOrigins(Ewk_Web_Storage_Origins_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webStorageOriginsCallback = callback;
    context->userData = userData;

    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(m_context.get());
    WKKeyValueStorageManagerGetKeyValueStorageOrigins(storageManager, context, didGetWebStorageOrigins);
}

void EwkContext::setWebStoragePath(const char* path)
{
    WKRetainPtr<WKStringRef> webStoragePath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetLocalStorageDirectory(m_context.get(), webStoragePath.get());
}

void EwkContext::webStoragePath(Ewk_Web_Storage_Path_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webStoragePathCallback= callback;
    context->userData = userData;

    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(m_context.get());
    WKKeyValueStorageManagerGetKeyValueStoragePath(storageManager, context, didGetWebStoragePath);
}

void EwkContext::webStorageUsageForOrigin(Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webStorageUsageCallback = callback;
    context->userData = userData;
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));

    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(m_context.get());
    WKKeyValueStorageManagerGetKeyValueStorageUsageForOrigin(storageManager, context, didGetWebStorageUsage, originRef.get());
}
#endif // ENABLE(TIZEN_WEB_STORAGE)

JSGlobalContextRef EwkContext::jsGlobalContext()
{
    if (!m_jsGlobalContext)
        m_jsGlobalContext = JSGlobalContextCreate(0);

    return m_jsGlobalContext;
}

Ewk_Application_Cache_Manager* ewk_context_application_cache_manager_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

    return const_cast<EwkContext*>(impl)->applicationCacheManager();
}

Ewk_Cookie_Manager* ewk_context_cookie_manager_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

    return const_cast<EwkContext*>(impl)->cookieManager();
}

Ewk_Database_Manager* ewk_context_database_manager_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

    return const_cast<EwkContext*>(impl)->databaseManager();
}

Eina_Bool ewk_context_favicon_database_directory_set(Ewk_Context* ewkContext, const char* directoryPath)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    return impl->setFaviconDatabaseDirectoryPath(String::fromUTF8(directoryPath));
}

Ewk_Favicon_Database* ewk_context_favicon_database_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

    return const_cast<EwkContext*>(impl)->faviconDatabase();
}

Ewk_Storage_Manager* ewk_context_storage_manager_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

    return impl->storageManager();
}

DownloadManagerEfl* EwkContext::downloadManager() const
{
    return m_downloadManager.get();
}

ContextHistoryClientEfl* EwkContext::historyClient()
{
    return m_historyClient.get();
}

static inline EwkContext* toEwkContext(const void* clientInfo)
{
    return static_cast<EwkContext*>(const_cast<void*>(clientInfo));
}

void EwkContext::didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    toEwkContext(clientInfo)->processReceivedMessageFromInjectedBundle(messageName, messageBody, 0);
}

void EwkContext::didReceiveSynchronousMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void* clientInfo)
{
    toEwkContext(clientInfo)->processReceivedMessageFromInjectedBundle(messageName, messageBody, returnData);
}

void EwkContext::setMessageFromInjectedBundleCallback(Ewk_Context_Message_From_Injected_Bundle_Cb callback, void* userData)
{
    m_callbackForMessageFromInjectedBundle.userData = userData;

    if (m_callbackForMessageFromInjectedBundle.callback == callback)
        return;

    if (!m_callbackForMessageFromInjectedBundle.callback) {
        WKContextInjectedBundleClient client;
        memset(&client, 0, sizeof(client));

        client.version = kWKContextClientCurrentVersion;
        client.clientInfo = this;
        client.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;
        client.didReceiveSynchronousMessageFromInjectedBundle = didReceiveSynchronousMessageFromInjectedBundle;

        WKContextSetInjectedBundleClient(m_context.get(), &client);
    } else if (!callback)
        WKContextSetInjectedBundleClient(m_context.get(), 0);

    m_callbackForMessageFromInjectedBundle.callback = callback;
}

void EwkContext::processReceivedMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData)
{
    if (!m_callbackForMessageFromInjectedBundle.callback)
        return;

    CString name = toImpl(messageName)->string().utf8();
    CString body;
    if (messageBody && WKStringGetTypeID() == WKGetTypeID(messageBody))
        body = toImpl(static_cast<WKStringRef>(messageBody))->string().utf8();

    if (returnData) {
        char* returnString = 0;
        m_callbackForMessageFromInjectedBundle.callback(name.data(), body.data(), &returnString, m_callbackForMessageFromInjectedBundle.userData);
        if (returnString) {
            *returnData = WKStringCreateWithUTF8CString(returnString);
            free(returnString);
        } else
            *returnData = WKStringCreateWithUTF8CString("");
    } else
        m_callbackForMessageFromInjectedBundle.callback(name.data(), body.data(), 0, m_callbackForMessageFromInjectedBundle.userData);
}

Ewk_Context* ewk_context_default_get()
{
    return EwkContext::defaultContext();
}

Ewk_Context* ewk_context_new()
{
    return EwkContext::create().leakRef();
}

Ewk_Context* ewk_context_new_with_injected_bundle_path(const char* path)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, 0);

#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
    char* wrtLaunchingPerformance = getenv("WRT_LAUNCHING_PERFORMANCE");
    if (wrtLaunchingPerformance && !strcmp(wrtLaunchingPerformance, "1")) {
        static bool firstTime = true;
        if (firstTime) {
            firstTime = false;

            if (ProcessLauncher::isInitialFork())
                ProcessLauncher::setSkipExec(true);
            else
                ProcessLauncher::setSkipExec(false);

            ProcessLauncher::forkProcess();

            if (ProcessLauncher::isParentProcess()) {
                Ewk_Context* ewkContext = ewk_context_new_with_injected_bundle_path(path);
                EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, 0);
                WKContextRef contextRef = impl->wkContext();
                toImpl(contextRef)->ensureSharedWebProcess();
                return ewkContext;
            }
            else if (ProcessLauncher::isChildProcess()) {
                ProcessLauncher::callWebProcessMain();
                exit(0);
            }

            ASSERT_NOT_REACHED();
            return 0;
        }
    }
#endif // ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)

    return EwkContext::create(String::fromUTF8(path)).leakRef();
}

Eina_Bool ewk_context_url_scheme_register(Ewk_Context* ewkContext, const char* scheme, Ewk_Url_Scheme_Request_Cb callback, void* userData)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(scheme, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    impl->requestManager()->registerURLSchemeHandler(String::fromUTF8(scheme), callback, userData);

    return true;
}

void ewk_context_history_callbacks_set(Ewk_Context* ewkContext, Ewk_History_Navigation_Cb navigate, Ewk_History_Client_Redirection_Cb clientRedirect, Ewk_History_Server_Redirection_Cb serverRedirect, Ewk_History_Title_Update_Cb titleUpdate, Ewk_History_Populate_Visited_Links_Cb populateVisitedLinks, void* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);

    impl->historyClient()->setCallbacks(navigate, clientRedirect, serverRedirect, titleUpdate, populateVisitedLinks, data);
}

void ewk_context_visited_link_add(Ewk_Context* ewkContext, const char* visitedURL)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);
    EINA_SAFETY_ON_NULL_RETURN(visitedURL);

    impl->addVisitedLink(visitedURL);
}

// Ewk_Cache_Model enum validation
COMPILE_ASSERT_MATCHING_ENUM(EWK_CACHE_MODEL_DOCUMENT_VIEWER, kWKCacheModelDocumentViewer);
COMPILE_ASSERT_MATCHING_ENUM(EWK_CACHE_MODEL_DOCUMENT_BROWSER, kWKCacheModelDocumentBrowser);
COMPILE_ASSERT_MATCHING_ENUM(EWK_CACHE_MODEL_PRIMARY_WEBBROWSER, kWKCacheModelPrimaryWebBrowser);

Eina_Bool ewk_context_cache_model_set(Ewk_Context* ewkContext, Ewk_Cache_Model cacheModel)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    impl->setCacheModel(cacheModel);

    return true;
}

Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, EWK_CACHE_MODEL_DOCUMENT_VIEWER);

    return impl->cacheModel();
}

Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    impl->setAdditionalPluginPath(String::fromUTF8(path));
    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(path);
    return false;
#endif
}

void ewk_context_resource_cache_clear(Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);

    impl->clearResourceCache();
}

void ewk_context_message_post_to_injected_bundle(Ewk_Context* ewkContext, const char* name, const char* body)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);
    EINA_SAFETY_ON_NULL_RETURN(name);
    EINA_SAFETY_ON_NULL_RETURN(body);

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString(name));
    WKRetainPtr<WKStringRef> messageBody(AdoptWK, WKStringCreateWithUTF8CString(body));
    WKContextPostMessageToInjectedBundle(impl->wkContext(), messageName.get(), messageBody.get());
}

void ewk_context_message_from_injected_bundle_callback_set(Ewk_Context* ewkContext, Ewk_Context_Message_From_Injected_Bundle_Cb callback, void* userData)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);

    impl->setMessageFromInjectedBundleCallback(callback, userData);
}

void ewk_context_preferred_languages_set(Eina_List* languages)
{
    Vector<String> preferredLanguages;
    if (languages) {
        Eina_List* l;
        void* data;
        EINA_LIST_FOREACH(languages, l, data)
            preferredLanguages.append(String::fromUTF8(static_cast<char*>(data)).lower().replace("_", "-"));
    }

    WebCore::overrideUserPreferredLanguages(preferredLanguages);
    WebCore::languageDidChange();
}

Eina_Bool ewk_context_certificate_file_set(Ewk_Context* ewkContext, const char* certificateFile)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    if (!impl->setCertificateFile(certificateFile))
        return true;

    if (fileExists(WTF::String::fromUTF8(certificateFile))) {
        long long fileSize = -1;
        getFileSize(WTF::String::fromUTF8(certificateFile), fileSize);
        TIZEN_LOGI("[Network] ewk_context_certificate_file_set certificateFile fileSize [%lld]\n", fileSize);
    } else
        TIZEN_LOGI("[Network] ewk_context_certificate_file_set certificateFile does not exist!\n");

    WKRetainPtr<WKStringRef> certificateFileRef(AdoptWK, WKStringCreateWithUTF8CString(certificateFile));
    WKContextSetCertificateFile(impl->wkContext(), certificateFileRef.get());
    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(certificateFile);
    return false;
#endif
}

const char* ewk_context_certificate_file_get(const Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    return impl->certificateFile();
#else
    UNUSED_PARAM(ewkContext);
    return 0;
#endif
}

void ewk_context_did_start_download_callback_set(Ewk_Context*, Ewk_Context_Did_Start_Download_Callback, void*)
{
    // FIXME: Need to implement
}

void ewk_context_form_password_data_clear(Ewk_Context*)
{
    // FIXME: Need to implement
}

void ewk_context_form_candidate_data_clear(Ewk_Context*)
{
    // FIXME: Need to implement
}

void ewk_context_memory_sampler_start(Ewk_Context*, double)
{
    // FIXME: Need to implement
}

void ewk_context_memory_sampler_stop(Ewk_Context*)
{
    // FIXME: Need to implement
}

void ewk_context_memory_saving_mode_set(Ewk_Context*, Eina_Bool)
{
    // FIXME: Need to implement
}

void ewk_context_network_session_requests_cancel(Ewk_Context*)
{
}

Eina_Bool ewk_context_notify_low_memory(Ewk_Context*)
{
    // FIXME: Need to implement
    return false;
}

void ewk_context_proxy_uri_set(Ewk_Context* ewkContext, const char* proxyAddress)
{
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);
    impl->setProxyAddress(proxyAddress);
#endif
}

const char* ewk_context_proxy_uri_get(const Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkContext, ewkContext, impl, 0);

    return const_cast<EwkContext*>(impl)->proxyAddress();
#else
    return 0;
#endif
}

void ewk_context_vibration_client_callbacks_set(Ewk_Context*, Ewk_Vibration_Client_Vibrate_Cb, Ewk_Vibration_Client_Vibration_Cancel_Cb, void*)
{
    // FIXME: Need to implement
}

Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_context_cache_disabled_set(Ewk_Context* ewkContext, Eina_Bool cacheDisabled)
{
#if ENABLE(TIZEN_CACHE_CONTROL)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    WKContextSetCacheDisabled(impl->wkContext(), cacheDisabled);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_cache_disabled_get(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_CACHE_CONTROL)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    return WKContextGetCacheDisabled(impl->wkContext());
#else
    return false;
#endif
}

void ewk_context_storage_path_reset(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_RESET_PATH)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl);

    WKContextResetStoragePath(impl->wkContext());
#else
    UNUSED_PARAM(ewkContext);
#endif // ENABLE(TIZEN_RESET_PATH)
}

Eina_Bool ewk_context_application_cache_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    if (!path) {
        // TIZEN_LOGE("Path value is invalid");
        return false;
    }

    // TIZEN_LOGI("path (%s)", path);
    impl->setApplicationCachePath(path);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_path_get(Ewk_Context* ewkContext, Ewk_Application_Cache_Path_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    // TIZEN_LOGI("callback (%p)", callback);
    impl->applicationCachePath(callback, userData);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_quota_get(Ewk_Context* ewkContext, Ewk_Application_Cache_Quota_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    // TIZEN_LOGI("callback (%p)", callback);
    impl->applicationCacheQuota(callback, userData);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_usage_for_origin_get(Ewk_Context* ewkContext, const EwkSecurityOrigin* origin, Ewk_Application_Cache_Usage_For_Origin_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    impl->applicationCacheUsageForOrigin(origin, callback, userData);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_quota_set(Ewk_Context* ewkContext, int64_t quota)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    if (!quota) {
        // TIZEN_LOGE("Quota value is invalid");
        return false;
    }

    // TIZEN_LOGI("quota (%d)", quota);
    impl->setApplicationCacheQuota(quota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_quota_for_origin_set(Ewk_Context* ewkContext, const EwkSecurityOrigin* origin, int64_t quota)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);
    if (!quota) {
        // TIZEN_LOGE("Quota value is invalid");
        return false;
    }

    // TIZEN_LOGI("quota (%d)", quota);
    impl->setApplicationCacheQuotaForOrigin(origin, quota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_local_file_system_path_set(Ewk_Context*, const char*)
{
    // FIXME : Remove this
    return false;
}

Eina_Bool ewk_context_local_file_system_all_delete(Ewk_Context* ewkContext)
{
    // FIXME : Remove this
    UNUSED_PARAM(ewkContext);

    return false;
}

Eina_Bool ewk_context_local_file_system_delete(Ewk_Context* ewkContext, EwkSecurityOrigin* origin)
{
    // FIXME: Remove this
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(origin);

    return false;
}

Eina_Bool ewk_context_local_file_system_origins_get(const Ewk_Context* ewkContext, Ewk_Local_File_System_Origins_Get_Callback callback, void* userData)
{
    // FIXME: Remove this
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);

    return false;
}

Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    // TIZEN_LOGI("ewkContext (%p)", ewkContext);
    impl->deleteWebStorageAll();

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

Eina_Bool ewk_context_web_storage_origin_delete(Ewk_Context* ewkContext, EwkSecurityOrigin* origin)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    impl->deleteWebStorageOrigin(origin);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

Eina_Bool ewk_context_web_storage_origins_get(Ewk_Context* ewkContext, Ewk_Web_Storage_Origins_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    // TIZEN_LOGI("callback (%p)", callback);
    impl->webStorageOrigins(callback, userData);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

Eina_Bool ewk_context_web_storage_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    // TIZEN_LOGI("path (%s)", path);
    impl->setWebStoragePath(path);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

Eina_Bool ewk_context_web_storage_path_get(Ewk_Context* ewkContext, Ewk_Web_Storage_Path_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    // TIZEN_LOGI("callback (%p)", callback);
    impl->webStoragePath(callback, userData);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}
Eina_Bool ewk_context_web_storage_usage_for_origin_get(Ewk_Context* ewkContext, Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    impl->webStorageUsageForOrigin(origin, callback, userData);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

#if ENABLE(TIZEN_SQL_DATABASE)
struct Ewk_Context_Exceeded_Quota {
    RefPtr<EwkSecurityOrigin> origin;
    const char* databaseName;
    const char* displayName;
    unsigned long long currentQuota;
    unsigned long long currentOriginUsage;
    unsigned long long currentDatabaseUsage;
    unsigned long long expectedUsage;
    unsigned long long newQuota;
};

Ewk_Context_Exceeded_Quota* ewkContextCreateExceededQuota(WKSecurityOriginRef origin, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage)
{
    Ewk_Context_Exceeded_Quota* exceededQuota = new Ewk_Context_Exceeded_Quota();

    int length = WKStringGetMaximumUTF8CStringSize(databaseName);
    OwnArrayPtr<char> databaseNameBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(databaseName, databaseNameBuffer.get(), length);

    length = WKStringGetMaximumUTF8CStringSize(displayName);
    OwnArrayPtr<char> displayNameBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(displayName, displayNameBuffer.get(), length);

    exceededQuota->origin = EwkSecurityOrigin::create(origin);
    exceededQuota->databaseName = eina_stringshare_add(databaseNameBuffer.get());
    exceededQuota->displayName = eina_stringshare_add(displayNameBuffer.get());
    exceededQuota->currentQuota = currentQuota;
    exceededQuota->currentOriginUsage = currentOriginUsage;
    exceededQuota->currentDatabaseUsage = currentDatabaseUsage;
    exceededQuota->expectedUsage = expectedUsage;

    return exceededQuota;
}

void ewkContextDeleteExceededQuota(Ewk_Context_Exceeded_Quota* exceededQuota)
{
    eina_stringshare_del(exceededQuota->databaseName);
    eina_stringshare_del(exceededQuota->displayName);
    delete exceededQuota;
}

unsigned long long ewkContextGetNewQuotaForExceededQuota(Ewk_Context* ewkContext, Ewk_Context_Exceeded_Quota* exceededQuota)
{
    if (exceededQuota->newQuota)
        return exceededQuota->newQuota + exceededQuota->currentQuota;

    if (exceededQuota->currentQuota)
        return exceededQuota->currentQuota;

    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, 0);
    return impl->defaultDatabaseQuota();
}

uint64_t ewkContextGetDatabaseQuota(Ewk_Context* ewkContext)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, 0);
    return impl->defaultDatabaseQuota();
}

static void didGetWebDatabaseOrigins(WKArrayRef origins, WKErrorRef error, void* context)
{
    // TIZEN_LOGI("origin size(%d)", WKArrayGetSize(origins));
    Eina_List* originList = 0;

    for(size_t i = 0; i < WKArrayGetSize(origins); i++) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        originList = eina_list_append(originList, EwkSecurityOrigin::create(securityOriginRef).leakRef());
    }

    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    webDatabaseContext->webDatabaseOriginsCallback(originList, webDatabaseContext->userData);

    void* origin = 0;
    EINA_LIST_FREE(originList, origin)
        delete static_cast<EwkSecurityOrigin*>(origin);
    delete webDatabaseContext;
}

static void didGetWebDatabaseQuota(WKUInt64Ref quota, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    // TIZEN_LOGI("quota (%d)", toImpl(quota)->value());
    webDatabaseContext->webDatabaseQuotaCallback(toImpl(quota)->value(), webDatabaseContext->userData);
    delete webDatabaseContext;
}

static void didGetWebDatabaseUsage(WKUInt64Ref usage, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    // TIZEN_LOGI("usage (%d)", toImpl(usage)->value());
    webDatabaseContext->webDatabaseUsageCallback(toImpl(usage)->value(), webDatabaseContext->userData);
    delete webDatabaseContext;
}

static void didGetWebDatabasePath(WKStringRef path, WKErrorRef error, void * context)
{
    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);

    int length = WKStringGetMaximumUTF8CStringSize(path);
    OwnArrayPtr<char> databasePath = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(path, databasePath.get(), length);

    // TIZEN_LOGI("path (%s)", databasePath.get());
    webDatabaseContext->webDatabasePathCallback(eina_stringshare_add(databasePath.get()), webDatabaseContext->userData);
    delete webDatabaseContext;
}

void EwkContext::setDefaultDatabaseQuota(uint64_t defaultDatabaseQuota)
{
    m_defaultDatabaseQuota = defaultDatabaseQuota;
}

void EwkContext::deleteWebDatabaseAll()
{
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(m_context.get());
    WKDatabaseManagerDeleteAllDatabases(databaseManager);
}

void EwkContext::deleteWebDatabase(Ewk_Security_Origin* origin)
{
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(m_context.get());
    WKDatabaseManagerDeleteDatabasesForOrigin(databaseManager, originRef.get());
}

void EwkContext::webDatabaseOrigins(Ewk_Web_Database_Origins_Get_Callback callback, void* userData)
{
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabaseOriginsCallback = callback;
    context->userData = userData;

    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(m_context.get());
    WKDatabaseManagerGetDatabaseOrigins(databaseManager, context, didGetWebDatabaseOrigins);
}

void EwkContext::setWebDatabasePath(const char* path)
{
    WKRetainPtr<WKStringRef> databasePath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetDatabaseDirectory(m_context.get(), databasePath.get());
}
#endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
void EwkContext::clearAllDecodedData()
{
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerClearCacheForAllOrigins(cacheManager, WKResourceCachesToClearInDecodedDataOnly);
}
#endif

Ewk_Security_Origin* ewk_context_web_database_exceeded_quota_security_origin_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    return exceededQuota->origin.get();
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

const char* ewk_context_web_database_exceeded_quota_database_name_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    // TIZEN_LOGI("name (%s)", exceededQuota->databaseName);
    return exceededQuota->databaseName;
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

const char* ewk_context_web_database_exceeded_quota_display_name_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    // TIZEN_LOGI("displayName (%s)", exceededQuota->displayName);
    return exceededQuota->displayName;
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

unsigned long long ewk_context_web_database_exceeded_quota_current_quota_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    // TIZEN_LOGI("quota (%d)", exceededQuota->currentQuota);
    return exceededQuota->currentQuota;
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

unsigned long long ewk_context_web_database_exceeded_quota_current_origin_usage_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    // TIZEN_LOGI("currentOriginUsage (%d)", exceededQuota->currentOriginUsage);
    return exceededQuota->currentOriginUsage;
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

unsigned long long ewk_context_web_database_exceeded_quota_current_database_usage_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    // TIZEN_LOGI("currentDatabaseUsage (%d)", exceededQuota->currentDatabaseUsage);
    return exceededQuota->currentDatabaseUsage;
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

unsigned long long ewk_context_web_database_exceeded_quota_expected_usage_get(Ewk_Context_Exceeded_Quota* exceededQuota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(exceededQuota, 0);

    // TIZEN_LOGI("expectedUsage (%d)", exceededQuota->expectedUsage);
    return exceededQuota->expectedUsage;
#else
    UNUSED_PARAM(exceededQuota);
    return 0;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

void ewk_context_web_database_exceeded_quota_new_quota_set(Ewk_Context_Exceeded_Quota* exceededQuota, unsigned long long quota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN(exceededQuota);

    // TIZEN_LOGI("quota (%d)", quota);
    exceededQuota->newQuota = quota;
#else
    UNUSED_PARAM(exceededQuota);
    UNUSED_PARAM(quota);
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

Eina_Bool ewk_context_web_database_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    // TIZEN_LOGI("ewkContext (%p)", ewkContext);
    impl->deleteWebDatabaseAll();

    return true;
#else
    UNUSED_PARAM(ewkContext);
    return false;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

Eina_Bool ewk_context_web_database_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    impl->deleteWebDatabase(origin);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(origin);
    return false;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

Eina_Bool ewk_context_web_database_origins_get(Ewk_Context* ewkContext, Ewk_Web_Database_Origins_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    // TIZEN_LOGI("callback (%p)", callback);
    impl->webDatabaseOrigins(callback, userData);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
    return false;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

Eina_Bool ewk_context_web_database_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    // TIZEN_LOGI("path (%s)", path);
    impl->setWebDatabasePath(path);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(path);
    return false;
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

Eina_Bool ewk_context_web_database_path_get(EwkContext* ewkContext, Ewk_Web_Database_Path_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabasePathCallback= callback;
    context->userData = userData;

    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetDatabasePath(databaseManager, context, didGetWebDatabasePath);
    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
    return false;
#endif
}

Eina_Bool ewk_context_web_database_quota_for_origin_get(EwkContext* ewkContext, Ewk_Web_Database_Quota_Get_Callback callback, void* userData, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabaseQuotaCallback = callback;
    context->userData = userData;

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetQuotaForOrigin(databaseManager, context, didGetWebDatabaseQuota, originRef.get());

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
    UNUSED_PARAM(origin);
    return false;
#endif
}

Eina_Bool ewk_context_web_database_default_quota_set(EwkContext* ewkContext, uint64_t quota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("quota (%d)", quota);
    ewkContext->setDefaultDatabaseQuota(quota);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(quota);
    return false;
#endif
}

Eina_Bool ewk_context_web_database_quota_for_origin_set(EwkContext* ewkContext, Ewk_Security_Origin* origin, uint64_t quota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerSetQuotaForOrigin(databaseManager, originRef.get(), quota);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(origin);
    UNUSED_PARAM(quota);
    return false;
#endif
}

Eina_Bool ewk_context_web_database_usage_for_origin_get(EwkContext* ewkContext, Ewk_Web_Database_Quota_Get_Callback callback, void* userData, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabaseQuotaCallback = callback;
    context->userData = userData;

    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetUsageForOrigin(databaseManager, context, didGetWebDatabaseUsage, originRef.get());

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
    UNUSED_PARAM(origin);
    return false;
#endif
}

#if ENABLE(TIZEN_EXTENSIBLE_API)
void EwkContext::setTizenExtensibleAPI(Ewk_Extensible_API extensibleAPI, Eina_Bool enable)
{
    WKContextSetTizenExtensibleAPI(m_context.get(), static_cast<WKTizenExtensibleAPI>(extensibleAPI), enable);
}
#endif // ENABLE(TIZEN_EXTENSIBLE_API)

Eina_Bool ewk_context_tizen_extensible_api_set(Ewk_Context* ewkContext,  Ewk_Extensible_API extensibleAPI, Eina_Bool enable)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    // TIZEN_LOGI("extensibleAPI (%d) enable (%d)", extensibleAPI, enable);
    impl->setTizenExtensibleAPI(extensibleAPI, enable);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(extensibleAPI);
    UNUSED_PARAM(enable);

    return false;
#endif // ENABLE(TIZEN_EXTENSIBLE_API)
}

#if ENABLE(TIZEN_INDEXED_DATABASE)
void EwkContext::deleteIndexedDatabaseAll()
{
    WKContextDeleteIndexedDatabaseAll(m_context.get());
}
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkContext, ewkContext, impl, false);

    impl->deleteIndexedDatabaseAll();

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_INDEXED_DATABASE)
}

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
bool EwkContext::setCertificateFile(const char* certificateFile)
{
    if (m_certificateFile == certificateFile)
        return false;

    m_certificateFile = certificateFile;
    return true;
}
#endif
