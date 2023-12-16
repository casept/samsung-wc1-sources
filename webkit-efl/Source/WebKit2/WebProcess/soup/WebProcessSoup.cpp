/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebProcess.h"

#define LIBSOUP_USE_UNSTABLE_REQUEST_API

#if PLATFORM(EFL)
#include "SeccompFiltersWebProcessEfl.h"
#endif

#include "WebCookieManager.h"
#include "WebProcessCreationParameters.h"
#include "WebSoupRequestManager.h"
#include <WebCore/FileSystem.h>
#include <WebCore/Language.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/PageCache.h>
#include <WebCore/ResourceHandle.h>
#include <libsoup/soup-cache.h>
#include <libsoup/soup.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(TIZEN_RESET_PATH)
#include "WebDatabaseManager.h"
#include "WebKeyValueStorageManager.h"
#include "WebPage.h"
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/LocalFileSystem.h>
#include <WebCore/StorageTracker.h>
#endif // ENABLE(TIZEN_RESET_PATH)

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include <WebCore/TizenExtensibleAPI.h>
#endif // ENABLE(TIZEN_EXTENSIBLE_API)

namespace WebKit {

static uint64_t getCacheDiskFreeSize(SoupCache* cache)
{
    ASSERT(cache);

    GOwnPtr<char> cacheDir;
    g_object_get(G_OBJECT(cache), "cache-dir", &cacheDir.outPtr(), NULL);
    if (!cacheDir)
        return 0;

    return WebCore::getVolumeFreeSizeForPath(cacheDir.get());
}

static uint64_t getMemorySize()
{
    static uint64_t kDefaultMemorySize = 512;
#if !OS(WINDOWS)
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize == -1)
        return kDefaultMemorySize;

    long physPages = sysconf(_SC_PHYS_PAGES);
    if (physPages == -1)
        return kDefaultMemorySize;

    return ((pageSize / 1024) * physPages) / 1024;
#else
    // Fallback to default for other platforms.
    return kDefaultMemorySize;
#endif
}

void WebProcess::platformSetCacheModel(CacheModel cacheModel)
{
    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;
    unsigned pageCacheCapacity = 0;

    unsigned long urlCacheMemoryCapacity = 0;
    unsigned long urlCacheDiskCapacity = 0;

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupCache* cache = SOUP_CACHE(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    uint64_t diskFreeSize = getCacheDiskFreeSize(cache) / 1024 / 1024;

    uint64_t memSize = getMemorySize();
    calculateCacheSizes(cacheModel, memSize, diskFreeSize,
                        cacheTotalCapacity, cacheMinDeadCapacity, cacheMaxDeadCapacity, deadDecodedDataDeletionInterval,
                        pageCacheCapacity, urlCacheMemoryCapacity, urlCacheDiskCapacity);

    WebCore::memoryCache()->setDisabled(cacheModel == CacheModelDocumentViewer);
    WebCore::memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    WebCore::memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
#if ENABLE(TIZEN_CACHE_DISABLE)
    WebCore::pageCache()->setCapacity(0);
#else
    WebCore::pageCache()->setCapacity(pageCacheCapacity);
#endif

#if ENABLE(TIZEN_CACHE_DISABLE)
    soup_cache_set_max_size(cache, 0);
#else
    if (urlCacheDiskCapacity > soup_cache_get_max_size(cache))
        soup_cache_set_max_size(cache, urlCacheDiskCapacity);
#endif
}

void WebProcess::platformClearResourceCaches(ResourceCachesToClear cachesToClear)
{
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    soup_cache_clear(SOUP_CACHE(soup_session_get_feature(session, SOUP_TYPE_CACHE)));
}

// This function is based on Epiphany code in ephy-embed-prefs.c.
static CString buildAcceptLanguages(Vector<String> languages)
{
    // Ignore "C" locale.
    size_t position = languages.find("c");
    if (position != notFound)
        languages.remove(position);

    // Fallback to "en" if the list is empty.
    if (languages.isEmpty())
        return "en";

    // Calculate deltas for the quality values.
    int delta;
    if (languages.size() < 10)
        delta = 10;
    else if (languages.size() < 20)
        delta = 5;
    else
        delta = 1;

    // Set quality values for each language.
    StringBuilder builder;
    for (size_t i = 0; i < languages.size(); ++i) {
        if (i)
            builder.append(", ");

        builder.append(languages[i]);

        int quality = 100 - i * delta;
        if (quality > 0 && quality < 100) {
            char buffer[8];
            g_ascii_formatd(buffer, 8, "%.2f", quality / 100.0);
            builder.append(String::format(";q=%s", buffer));
        }
    }

    return builder.toString().utf8();
}

static void setSoupSessionAcceptLanguage(Vector<String> languages)
{
    g_object_set(WebCore::ResourceHandle::defaultSession(), "accept-language", buildAcceptLanguages(languages).data(), NULL);
}

static void languageChanged(void*)
{
    setSoupSessionAcceptLanguage(WebCore::userPreferredLanguages());
}

void WebProcess::platformInitializeWebProcess(const WebProcessCreationParameters& parameters, CoreIPC::MessageDecoder&)
{

#if ENABLE(SECCOMP_FILTERS)
    {
#if PLATFORM(EFL)
        SeccompFiltersWebProcessEfl seccompFilters(parameters);
#endif
        seccompFilters.initialize();
    }
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    setCertificateFile(parameters.certificateFile);
#endif

    ASSERT(!parameters.diskCacheDirectory.isEmpty());
    GRefPtr<SoupCache> soupCache = adoptGRef(soup_cache_new(parameters.diskCacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER));
    soup_session_add_feature(WebCore::ResourceHandle::defaultSession(), SOUP_SESSION_FEATURE(soupCache.get()));
    soup_cache_load(soupCache.get());

    if (!parameters.languages.isEmpty())
        setSoupSessionAcceptLanguage(parameters.languages);

    for (size_t i = 0; i < parameters.urlSchemesRegistered.size(); i++)
        supplement<WebSoupRequestManager>()->registerURIScheme(parameters.urlSchemesRegistered[i]);

    if (!parameters.cookiePersistentStoragePath.isEmpty()) {
        supplement<WebCookieManager>()->setCookiePersistentStorage(parameters.cookiePersistentStoragePath,
            parameters.cookiePersistentStorageType);
    }
    supplement<WebCookieManager>()->setHTTPCookieAcceptPolicy(parameters.cookieAcceptPolicy);

    setIgnoreTLSErrors(parameters.ignoreTLSErrors);

    WebCore::addLanguageChangeObserver(this, languageChanged);
}

void WebProcess::platformTerminate()
{
    WebCore::removeLanguageChangeObserver(this);
}

void WebProcess::setIgnoreTLSErrors(bool ignoreTLSErrors)
{
    WebCore::ResourceHandle::setIgnoreSSLErrors(ignoreTLSErrors);
}

#if ENABLE(TIZEN_SESSION_REQUEST_CANCEL)
void WebProcess::abortSession()
{
    soup_session_abort(WebCore::ResourceHandle::defaultSession());
}
#endif

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void WebProcess::setProxyAddress(const String& proxyAddress)
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    if (!proxyAddress) {
        g_object_set(session, SOUP_SESSION_PROXY_URI, 0, NULL);
        soup_session_remove_feature_by_type(session, SOUP_TYPE_PROXY_RESOLVER);
        return;
    }

    WTF::String proxyValue = proxyAddress;
    if (!proxyAddress.startsWith("http://", false))
        proxyValue.insert("http://", 0);

    SoupURI* uri = soup_uri_new(proxyValue.utf8().data());
    if (uri) {
        g_object_set(session, SOUP_SESSION_PROXY_URI, uri, NULL);
        soup_uri_free(uri);
    }
}
#endif

#if ENABLE(TIZEN_CACHE_CONTROL)
void WebProcess::platformSetCacheDisabled(bool cacheDisabled)
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    static String cacheDirPath;

    if (cacheDisabled) {
        SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));

        GOwnPtr<char> cacheDir;
        g_object_get(G_OBJECT(cache), "cache-dir", &cacheDir.outPtr(), NULL);
        if (!cacheDir)
            return;
        cacheDirPath = String::fromUTF8(cacheDir.get());
        soup_cache_dump(cache);
        soup_session_remove_feature(session, SOUP_SESSION_FEATURE(cache));
    } else {
        SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
        if (cache)
            return;

        if (!cacheDirPath.isEmpty()) {
            cache = soup_cache_new(cacheDirPath.utf8().data(), SOUP_CACHE_SINGLE_USER);
            if (!cache)
                return;
            soup_session_add_feature(session, SOUP_SESSION_FEATURE(cache));
            soup_cache_load(cache);
            g_object_unref(cache);
        }
    }
}
#endif // ENABLE(TIZEN_CACHE_CONTROL)

#if ENABLE(TIZEN_RESET_PATH)
void WebProcess::resetStoragePath()
{
    TIZEN_LOGI("");

    String homeDirectoryPath = WebCore::homeDirectoryPath();

    WebCore::cacheStorage().setCacheDirectory(WebCore::pathByAppendingComponent(homeDirectoryPath, ".webkit/appcache"));

#if ENABLE(FILE_SYSTEM)
    String fileSystemDirectoryPath = WebCore::pathByAppendingComponent(homeDirectoryPath, ".webkit/localFileSystem");
    if (!WebCore::LocalFileSystem::initializeLocalFileSystem(fileSystemDirectoryPath))
        WebCore::LocalFileSystem::localFileSystem().changeFileSystemBasePath(fileSystemDirectoryPath);
#endif // ENABLE(FILE_SYSTEM)

#if ENABLE(TIZEN_SOUP_DATA_DIRECTORY_UNDER_EACH_APP)
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupCache* oldCache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    while (oldCache) {
        soup_cache_dump(oldCache);
        soup_session_remove_feature(session, SOUP_SESSION_FEATURE(oldCache));
        oldCache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    }
    String cacheDirectory = WebCore::pathByAppendingComponent(homeDirectoryPath, ".webkit/soupData/cache");
    SoupCache* soupCache = soup_cache_new(cacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER);
    if (soupCache) {
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(soupCache));
        soup_cache_load(soupCache);
    }

    String cookieDirectoryPath = WebCore::pathByAppendingComponent(homeDirectoryPath, ".webkit/soupData/cookie");
    if (WebCore::makeAllDirectories(cookieDirectoryPath)) {
        String cookieFilePath = WebCore::pathByAppendingComponent(homeDirectoryPath, ".webkit/soupData/cookie/.cookie.db");
        supplement<WebCookieManager>()->setCookiePersistentStorage(cookieFilePath, SoupCookiePersistentStorageSQLite);
    }
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    m_indexedDatabaseDirectory = WebCore::pathByAppendingComponent(homeDirectoryPath, ".webkit/indexedDatabases");
#endif

    HashMap<uint64_t, RefPtr<WebPage> >::iterator end = m_pageMap.end();
    for (HashMap<uint64_t, RefPtr<WebPage> >::iterator it = m_pageMap.begin(); it != end; ++it) {
        WebPage* page = it->value.get();

#if ENABLE(TIZEN_INDEXED_DATABASE)
        page->setIndexedDatabaseDirectory(m_indexedDatabaseDirectory);
#endif
    }
}
#endif // ENABLE(TIZEN_RESET_PATH)

#if ENABLE(TIZEN_EXTENSIBLE_API)
void WebProcess::setTizenExtensibleAPI(int extensibleAPI, bool enable)
{
    WebCore::TizenExtensibleAPI::extensibleAPI().setTizenExtensibleAPI(static_cast<WebCore::ExtensibleAPI>(extensibleAPI), enable);
}
#endif // ENABLE(TIZEN_EXTENSIBLE_API)

#if ENABLE(TIZEN_INDEXED_DATABASE)
void WebProcess::deleteIndexedDatabaseAll()
{
    Vector<String> paths = WebCore::listDirectory(m_indexedDatabaseDirectory, ("*"));
    size_t pathCount = paths.size();
    for (size_t i = 0; i < pathCount; ++i)
        WebCore::removeDirectory(paths[i]);
}
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void WebProcess::setCertificateFile(const WTF::String& certificateFile)
{
#if USE(SOUP)
    SoupSession* session = WebCore::ResourceHandle::defaultSession();

    TIZEN_LOGI("setCertificateFile() is called. call g_object_set().");
    g_object_set(session,
                   SOUP_SESSION_SSL_STRICT, FALSE,
                   SOUP_SESSION_CERTIFICATE_PATH, certificateFile.utf8().data(), NULL);
#endif
}
#endif

} // namespace WebKit
