/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   ewk_storage_manager_product.h
 * @brief  Describes the Ewk Storage Manager API.
 *
 * Ewk Storage Manager manages web storage.
 */

#ifndef ewk_storage_manager_product_h
#define ewk_storage_manager_product_h

#include "ewk_security_origin_product.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for Ewk_Storage_Manager. */
typedef struct EwkStorageManager Ewk_Storage_Manager;

/**
 * @typedef Ewk_Storage_Origins_Async_Get_Cb Ewk_Storage_Origins_Async_Get_Cb
 * @brief Callback type for use with ewk_storage_manager_origins_async_get().
 *
 * @param origins @c Eina_List containing @c Ewk_Security_Origin elements.
 *
 * @note The @a origins should be freed like below code after use.
 *
 * @code
 *
 * static void
 * _origins_get_cb(Eina_List* origins, data)
 * {
 *    // ...
 *
 *    void *origin;
 *    EINA_LIST_FREE(origins, origin)
 *      ewk_object_unref((Ewk_Object*)origin);
 * }
 *
 * @endcode
 */
typedef void (*Ewk_Storage_Origins_Async_Get_Cb)(Eina_List *origins, void *user_data);

/**
 * Gets list of origins that are stored in storage db asynchronously.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param manager Ewk_Storage_Manager object
 * @param callback callback to get storage origins
 * @param user_data user_data will be passed when result_callback is called,
 *    -i.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_storage_manager_origins_async_get(const Ewk_Storage_Manager *manager, Ewk_Storage_Origins_Async_Get_Cb callback, void *user_data);

/**
 * Deletes all local storage.
 *
 * @param manager Ewk_Storage_Manager object
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_storage_manager_entries_clear(Ewk_Storage_Manager *manager);

/**
 * Deletes local storage for the specified origin.
 *
 * @param manager Ewk_Storage_Manager object
 * @param origin security origin
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_storage_manager_entries_for_origin_del(Ewk_Storage_Manager *manager, Ewk_Security_Origin *origin);

#ifdef __cplusplus
}
#endif
#endif // ewk_storage_manager_product_h
