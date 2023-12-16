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
 */

/**
 * @file    ewk_context.h
 * @brief   Describes the context API.
 *
 * @note ewk_context encapsulates all pages related to specific use of WebKit.
 *
 * Applications have the option of creating a context different than the default one
 * and use it for a group of pages. All pages in the same context share the same
 * preferences, visited link set, local storage, etc.
 *
 * A process model can be specified per context. The default one is the shared model
 * where the web-engine process is shared among the pages in the context. The second
 * model allows each page to use a separate web-engine process. This latter model is
 * currently not supported by WebKit2/EFL.
 *
 */

#ifndef ewk_context_h
#define ewk_context_h

#include "ewk_cookie_manager.h"
#include <Evas.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#ifndef ewk_context_type
#define ewk_context_type
/**
 * @brief Creates a type name for @a Ewk_Context.
 */
typedef struct EwkObject Ewk_Context;
#endif

/**
 * @brief Gets the cookie manager instance for this @a context.
 *
 * @param[in] context context object to query
 *
 * @return Ewk_Cookie_Manager object instance or @c NULL in case of failure
 */
EAPI Ewk_Cookie_Manager *ewk_context_cookie_manager_get(const Ewk_Context *context);

/**
 * \enum    Ewk_Cache_Model
 *
 * @brief   Contains option for cache model
 * @since_tizen 2.3
 */
enum Ewk_Cache_Model {
    /// Use the smallest cache capacity.
    EWK_CACHE_MODEL_DOCUMENT_VIEWER,
    /// Use bigger cache capacity than EWK_CACHE_MODEL_DOCUMENT_VIEWER.
    EWK_CACHE_MODEL_DOCUMENT_BROWSER,
    /// Use the biggest cache capacity.
    EWK_CACHE_MODEL_PRIMARY_WEBBROWSER
};

/**
 * @brief Creates a type name for the Ewk_Cache_Model.
 * @since_tizen 2.3
 */
typedef enum Ewk_Cache_Model Ewk_Cache_Model;

/**
 * Set @a cache_model as the cache model for @a context.
 *
 * By default, it is EWK_CACHE_MODEL_DOCUMENT_VIEWER.
 *
 * @param context context object to update.
 * @param cache_model a #Ewk_Cache_Model.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_cache_model_set(Ewk_Context *context, Ewk_Cache_Model cache_model);

/**
 * Gets the cache model for @a context.
 *
 * @param context context object to query.
 *
 * @return the cache model for the @a context.
 */
EAPI Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context *context);

/**
 * @brief Clears HTTP caches in local storage and all resources cached in memory\n
 * such as images, CSS, JavaScript, XSL, and fonts for @a context.
 *
 * @param[in] context context object to clear all resource caches
 */
EAPI void ewk_context_resource_cache_clear(Ewk_Context *context);

/**
 * @brief Sets the list of preferred languages.
 *
 * @details Sets the list of preferred langages. This list will be used to build the "Accept-Language"\n
 * header that will be included in the network requests.\n
 * Client can pass @c NULL for languages to clear what is set before.
 *
 * @param[in] languages the list of preferred languages (char* as data) or @c NULL
 *
 * @remarks all contexts will be affected.
 */
EAPI void ewk_context_preferred_languages_set(Eina_List *languages);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_context_h
