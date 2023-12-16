/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

/**
 * @file    ewk_cookie_manager_product.h
 * @brief   Describes the Ewk Cookie Manager API.
 */

#ifndef ewk_cookie_manager_product_h
#define ewk_cookie_manager_product_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \enum    Ewk_Cookie_Persistent_Storage
 *
 * @brief   Enum values to denote cookies persistent storage type.
 */
enum Ewk_Cookie_Persistent_Storage {
    /// Cookies are stored in a text file in the Mozilla "cookies.txt" format.
    EWK_COOKIE_PERSISTENT_STORAGE_TEXT,
    /// Cookies are stored in a SQLite file in the current Mozilla format.
    EWK_COOKIE_PERSISTENT_STORAGE_SQLITE
};

/// Creates a type name for the Ewk_Cookie_Persistent_Storage.
typedef enum Ewk_Cookie_Persistent_Storage Ewk_Cookie_Persistent_Storage;

/**
 * @typedef Ewk_Cookie_Manager_Policy_Async_Get_Cb Ewk_Cookie_Manager_Policy_Async_Get_Cb
 * @brief Callback type for use with ewk_cookie_manager_accept_policy_async_get
 */
typedef void (*Ewk_Cookie_Manager_Policy_Async_Get_Cb)(Ewk_Cookie_Accept_Policy policy, void *event_info);

/**
 * @typedef Ewk_Cookie_Manager_Hostnames_Async_Get_Cb Ewk_Cookie_Manager_Hostnames_Async_Get_Cb
 * @brief Callback type for use with ewk_cookie_manager_hostnames_with_cookies_async_get
 *
 * @note The @a hostnames list items are guaranteed to be eina_stringshare. Whenever possible
 * save yourself some cpu cycles and use eina_stringshare_ref() instead of eina_stringshare_add()
 * or strdup().
 */
typedef void (*Ewk_Cookie_Manager_Hostnames_Async_Get_Cb)(Eina_List *hostnames, void *event_info);

/**
 * @typedef Ewk_Cookie_Manager_Changes_Watch_Cb Ewk_Cookie_Manager_Changes_Watch_Cb
 * @brief Callback type for use with ewk_cookie_manager_changes_watch()
 */
typedef void (*Ewk_Cookie_Manager_Changes_Watch_Cb)(void *event_info);

/**
 * Set the @a filename where non-session cookies are stored persistently using @a storage as the format to read/write the cookies.
 *
 * Cookies are initially read from @filename to create an initial set of cookies.
 * Then, non-session cookies will be written to @filename.
 *
 * By default, @a manager doesn't store the cookies persistenly, so you need to call this
 * method to keep cookies saved across sessions.
 *
 * @param cookie_manager The cookie manager to update.
 * @param filename the filename to read to/write from.
 * @param storage the type of storage.
 */
EAPI void ewk_cookie_manager_persistent_storage_set(Ewk_Cookie_Manager *manager, const char *filename, Ewk_Cookie_Persistent_Storage storage);

/**
 * Asynchronously get the cookie acceptance policy of @a manager.
 *
 * By default, only cookies set by the main document loaded are accepted.
 *
 * @param manager The cookie manager to query.
 * @param callback The function to call when the policy is received.
 * @param data User data (may be @c NULL).
 */
EAPI void ewk_cookie_manager_accept_policy_async_get(const Ewk_Cookie_Manager *manager, Ewk_Cookie_Manager_Policy_Async_Get_Cb callback, void *data);

/**
 * Asynchronously get the list of host names for which @a manager contains cookies.
 *
 * @param manager The cookie manager to query.
 * @param callback The function to call when the host names have been received.
 * @param data User data (may be @c NULL).
 */
EAPI void ewk_cookie_manager_hostnames_with_cookies_async_get(const Ewk_Cookie_Manager *manager, Ewk_Cookie_Manager_Hostnames_Async_Get_Cb callback, void *data);

/**
 * Remove all cookies of @a manager for the given @a hostname.
 *
 * @param manager The cookie manager to update.
 * @param hostname A host name.
 */
EAPI void ewk_cookie_manager_hostname_cookies_clear(Ewk_Cookie_Manager *manager, const char *hostname);

/**
 * Watch for cookies changes in @a manager.
 *
 * Pass @c NULL as value for @a callback to stop watching for changes.
 *
 * @param manager The cookie manager to watch.
 * @param callback function that will be called every time cookies are added, removed or modified.
 * @param data User data (may be @c NULL).
 */
EAPI void ewk_cookie_manager_changes_watch(Ewk_Cookie_Manager *manager, Ewk_Cookie_Manager_Changes_Watch_Cb callback, void *data);

#ifdef __cplusplus
}
#endif

#endif // ewk_cookie_manager_product_h
