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

#include "ewk_application_cache_manager.h"
#include "ewk_cookie_manager.h"
#include "ewk_database_manager.h"
#include "ewk_favicon_database.h"
#include "ewk_navigation_data.h"
#include "ewk_storage_manager.h"
#include "ewk_url_scheme_request.h"
#include "ewk_security_origin.h"
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Context as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Context;

/**
 * \enum    Ewk_Cache_Model
 *
 * @brief   Contains option for cache model
 */
enum Ewk_Cache_Model {
    /// Use the smallest cache capacity.
    EWK_CACHE_MODEL_DOCUMENT_VIEWER,
    /// Use bigger cache capacity than EWK_CACHE_MODEL_DOCUMENT_VIEWER.
    EWK_CACHE_MODEL_DOCUMENT_BROWSER,
    /// Use the biggest cache capacity.
    EWK_CACHE_MODEL_PRIMARY_WEBBROWSER
};

/// Creates a type name for the Ewk_Cache_Model.
typedef enum Ewk_Cache_Model Ewk_Cache_Model;

/**
 * @typedef Ewk_Url_Scheme_Request_Cb Ewk_Url_Scheme_Request_Cb
 * @brief Callback type for use with ewk_context_url_scheme_register().
 */
typedef void (*Ewk_Url_Scheme_Request_Cb) (Ewk_Url_Scheme_Request *request, void *user_data);

/**
 * @typedef Ewk_History_Navigation_Cb Ewk_History_Navigation_Cb
 * @brief Type definition for a function that will be called back when @a view did navigation (loaded new URL).
 */
typedef void (*Ewk_History_Navigation_Cb)(const Evas_Object *view, Ewk_Navigation_Data *navigation_data, void *user_data);

/**
 * @typedef Ewk_History_Client_Redirection_Cb Ewk_History_Client_Redirection_Cb
 * @brief Type definition for a function that will be called back when @a view performed a client redirect.
 */
typedef void (*Ewk_History_Client_Redirection_Cb)(const Evas_Object *view, const char *source_url, const char *destination_url, void *user_data);

/**
 * @typedef Ewk_History_Server_Redirection_Cb Ewk_History_Server_Redirection_Cb
 * @brief Type definition for a function that will be called back when @a view performed a server redirect.
 */
typedef void (*Ewk_History_Server_Redirection_Cb)(const Evas_Object *view, const char *source_url, const char *destination_url, void *user_data);

/**
 * @typedef Ewk_History_Title_Update_Cb Ewk_History_Title_Update_Cb
 * @brief Type definition for a function that will be called back when history title is updated.
 */
typedef void (*Ewk_History_Title_Update_Cb)(const Evas_Object *view, const char *title, const char *url, void *user_data);

/**
 * @typedef Ewk_Context_History_Client_Visited_Links_Populate_Cb Ewk_Context_History_Client_Visited_Links_Populate_Cb
 * @brief Type definition for a function that will be called back when client is asked to provide visited links from a client-managed storage.
 *
 * @see ewk_context_visited_link_add
 */
typedef void (*Ewk_History_Populate_Visited_Links_Cb)(void *user_data);

/**
 * Callback for didReceiveMessageFromInjectedBundle and didReceiveSynchronousMessageFromInjectedBundle
 *
 * User should allocate new string for return_data before setting it.
 * The return_data string will be freed on WebKit side.
 *
 * @param name name of message from injected bundle
 * @param body body of message from injected bundle
 * @param return_data return_data string from application
 * @param user_data user_data will be passsed when receiving message from injected bundle
 */
typedef void (*Ewk_Context_Message_From_Injected_Bundle_Cb)(const char *name, const char *body, char **return_data, void *user_data);

/**
 * Gets default Ewk_Context instance.
 *
 * The returned Ewk_Context object @b should not be unref'ed if application
 * does not call ewk_context_ref() for that.
 *
 * @return Ewk_Context object.
 */
EAPI Ewk_Context *ewk_context_default_get(void);

/**
 * Creates a new Ewk_Context.
 *
 * The returned Ewk_Context object @b should be unref'ed after use.
 *
 * @return Ewk_Context object on success or @c NULL on failure
 *
 * @see ewk_object_unref
 * @see ewk_context_new_with_injected_bundle_path
 */
EAPI Ewk_Context *ewk_context_new(void);

/**
 * Creates a new Ewk_Context.
 *
 * The returned Ewk_Context object @b should be unref'ed after use.
 *
 * @param path path of injected bundle library
 *
 * @return Ewk_Context object on success or @c NULL on failure
 *
 * @see ewk_object_unref
 * @see ewk_context_new
 */
EAPI Ewk_Context *ewk_context_new_with_injected_bundle_path(const char *path);

/**
 * Gets the application cache manager instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Cookie_Manager object instance or @c NULL in case of failure.
 */
EAPI Ewk_Application_Cache_Manager *ewk_context_application_cache_manager_get(const Ewk_Context *context);

/**
 * Gets the cookie manager instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Cookie_Manager object instance or @c NULL in case of failure.
 */
EAPI Ewk_Cookie_Manager *ewk_context_cookie_manager_get(const Ewk_Context *context);

/**
 * Gets the database manager instance for this @a context.
 *
 * @param context context object to query
 *
 * @return Ewk_Database_Manager object instance or @c NULL in case of failure
 */
EAPI Ewk_Database_Manager *ewk_context_database_manager_get(const Ewk_Context *context);

/**
 * Sets the favicon database directory for this @a context.
 *
 * Sets the directory path to be used to store the favicons database
 * for @a context on disk. Passing @c NULL as @a directory_path will
 * result in using the default directory for the platform.
 *
 * Calling this method also means enabling the favicons database for
 * its use from the applications, it is therefore expected to be
 * called only once. Further calls for the same instance of
 * @a context will not have any effect.
 *
 * @param context context object to update
 * @param directory_path database directory path to set
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_context_favicon_database_directory_set(Ewk_Context *context, const char *directory_path);

/**
 * Gets the favicon database instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Favicon_Database object instance or @c NULL in case of failure.
 */
EAPI Ewk_Favicon_Database *ewk_context_favicon_database_get(const Ewk_Context *context);

/**
 * Gets the storage manager instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Storage_Manager object instance or @c NULL in case of failure.
 */
EAPI Ewk_Storage_Manager *ewk_context_storage_manager_get(const Ewk_Context *context);

/**
 * Register @a scheme in @a context.
 *
 * When an URL request with @a scheme is made in the #Ewk_Context, the callback
 * function provided will be called with a #Ewk_Url_Scheme_Request.
 *
 * It is possible to handle URL scheme requests asynchronously, by calling ewk_url_scheme_ref() on the
 * #Ewk_Url_Scheme_Request and calling ewk_url_scheme_request_finish() later when the data of
 * the request is available.
 *
 * @param context a #Ewk_Context object.
 * @param scheme the network scheme to register
 * @param callback the function to be called when an URL request with @a scheme is made.
 * @param user_data data to pass to callback function
 *
 * @code
 * static void about_url_scheme_request_cb(Ewk_Url_Scheme_Request *request, void *user_data)
 * {
 *     const char *path;
 *     char *contents_data = NULL;
 *     unsigned int contents_length = 0;
 *
 *     path = ewk_url_scheme_request_path_get(request);
 *     if (!strcmp(path, "plugins")) {
 *         // Initialize contents_data with the contents of plugins about page, and set its length to contents_length
 *     } else if (!strcmp(path, "memory")) {
 *         // Initialize contents_data with the contents of memory about page, and set its length to contents_length
 *     } else if (!strcmp(path, "applications")) {
 *         // Initialize contents_data with the contents of application about page, and set its length to contents_length
 *     } else {
 *         Eina_Strbuf *buf = eina_strbuf_new();
 *         eina_strbuf_append_printf(buf, "&lt;html&gt;&lt;body&gt;&lt;p&gt;Invalid about:%s page&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;", path);
 *         contents_data = eina_strbuf_string_steal(buf);
 *         contents_length = strlen(contents);
 *         eina_strbuf_free(buf);
 *     }
 *     ewk_url_scheme_request_finish(request, contents_data, contents_length, "text/html");
 *     free(contents_data);
 * }
 * @endcode
 */
EAPI Eina_Bool ewk_context_url_scheme_register(Ewk_Context *context, const char *scheme, Ewk_Url_Scheme_Request_Cb callback, void *user_data);

/**
 * Sets history callbacks for the given @a context.
 *
 * To stop listening for history events, you may call this function with @c
 * NULL for the callbacks.
 *
 * @param context context object to set history callbacks
 * @param navigate_func The function to call when @c ewk_view did navigation (may be @c NULL).
 * @param client_redirect_func The function to call when @c ewk_view performed a client redirect (may be @c NULL).
 * @param server_redirect_func The function to call when @c ewk_view performed a server redirect (may be @c NULL).
 * @param title_update_func The function to call when history title is updated (may be @c NULL).
 * @param populate_visited_links_func The function is called when client is asked to provide visited links from a
 *        client-managed storage (may be @c NULL).
 * @param data User data (may be @c NULL).
 */
EAPI void ewk_context_history_callbacks_set(Ewk_Context *context,
                                            Ewk_History_Navigation_Cb navigate_func,
                                            Ewk_History_Client_Redirection_Cb client_redirect_func,
                                            Ewk_History_Server_Redirection_Cb server_redirect_func,
                                            Ewk_History_Title_Update_Cb title_update_func,
                                            Ewk_History_Populate_Visited_Links_Cb populate_visited_links_func,
                                            void *data);

/**
 * Registers the given @a visited_url as visited link in @a context visited link cache.
 *
 * This function shall be invoked as a response to @c populateVisitedLinks callback of the history cient.
 *
 * @param context context object to add visited link data
 * @param visited_url visited url
 *
 * @see Ewk_Context_History_Client
 */
EAPI void ewk_context_visited_link_add(Ewk_Context *context, const char *visited_url);

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
 * Sets additional plugin path for @a context.
 *
 * @param context context object to set additional plugin path
 * @param path the path to be used for plugins
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context *context, const char *path);

/**
 * Clears HTTP caches in local storage and all resources cached in memory 
 * such as images, CSS, JavaScript, XSL, and fonts for @a context.
 *
 * @param context context object to clear all resource caches
 */
EAPI void ewk_context_resource_cache_clear(Ewk_Context *context);

/**
 * Posts message to injected bundle.
 *
 * @param context context object to post message to injected bundle
 * @param name message name
 * @param body message body
 */
EAPI void ewk_context_message_post_to_injected_bundle(Ewk_Context *context, const char *name, const char *body);

/**
 * Sets callback for received injected bundle message.
 *
 * Client can pass @c NULL for callback to stop listening for messages.
 *
 * @param context context object
 * @param callback callback for received injected bundle message or @c NULL
 * @param user_data user data
 */
EAPI void ewk_context_message_from_injected_bundle_callback_set(Ewk_Context *context, Ewk_Context_Message_From_Injected_Bundle_Cb callback, void *user_data);

/**
 * Sets the list of preferred languages.
 *
 * Sets the list of preferred langages. This list will be used to build the "Accept-Language"
 * header that will be included in the network requests.
 * Client can pass @c NULL for languages to clear what is set before.
 *
 * @param languages the list of preferred languages (char* as data) or @c NULL
 *
 * @note all contexts will be affected.
 */
EAPI void ewk_context_preferred_languages_set(Eina_List *languages);

/*** TIZEN EXTENSIONS ****/
/**
 * Callback for didStartDownload
 *
 * @param download_url url to download
 * @param user_data user_data will be passsed when download is started
 */
typedef void (*Ewk_Context_Did_Start_Download_Callback)(const char *download_url, void *user_data);

/**
 * @typedef Ewk_Vibration_Client_Vibrate_Cb Ewk_Vibration_Client_Vibrate_Cb
 * @brief Type definition for a function that will be called back when vibrate
 * request receiveed from the vibration controller.
 */
typedef void (*Ewk_Vibration_Client_Vibrate_Cb)(uint64_t vibration_time, void *user_data);

/**
 * @typedef Ewk_Vibration_Client_Vibration_Cancel_Cb Ewk_Vibration_Client_Vibration_Cancel_Cb
 * @brief Type definition for a function that will be called back when cancel
 * vibration request receiveed from the vibration controller.
 */
typedef void (*Ewk_Vibration_Client_Vibration_Cancel_Cb)(void *user_data);

EAPI Eina_Bool ewk_context_application_cache_delete_all(Ewk_Context *ewkContext);

EAPI Eina_Bool ewk_context_application_cache_delete(Ewk_Context *ewkContext, Ewk_Security_Origin *origin);

EAPI Eina_Bool ewk_context_certificate_file_set(Ewk_Context *context, const char *certificateFile);

EAPI void ewk_context_did_start_download_callback_set(Ewk_Context *ewkContext, Ewk_Context_Did_Start_Download_Callback callback, void *userData);

EAPI void ewk_context_form_password_data_clear(Ewk_Context *ewkContext);

EAPI void ewk_context_form_candidate_data_clear(Ewk_Context *ewkContext);

EAPI void ewk_context_memory_sampler_start(Ewk_Context *ewkContext, double timerInterval);

EAPI void ewk_context_memory_sampler_stop(Ewk_Context *ewkContext);

EAPI void ewk_context_network_session_requests_cancel(Ewk_Context *ewkContext);

EAPI Eina_Bool ewk_context_notify_low_memory(Ewk_Context *ewkContext);

EAPI void ewk_context_proxy_uri_set(Ewk_Context *ewkContext, const char *proxy);

EAPI const char *ewk_context_proxy_uri_get(const Ewk_Context *ewkContext);

EAPI void ewk_context_vibration_client_callbacks_set(Ewk_Context *ewkContext, Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancel, void *data);

EAPI Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context *ewkContext);

EAPI Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context* context);

//#if ENABLE(TIZEN_CACHE_CONTROL)
/**
* Toggles the cache enable and disable
*
* @param context context object
* @param enable or disable cache
*
* @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
*/
EAPI Eina_Bool ewk_context_cache_disabled_set(Ewk_Context* ewkContext, Eina_Bool cacheDisabled);

/**
* Queries if the cache is enabled
*
* @param context context object
*
* @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
*/
EAPI Eina_Bool ewk_context_cache_disabled_get(const Ewk_Context* ewkContext);
//#endif // ENABLE(TIZEN_CACHE_CONTROL)

// #if ENABLE(TIZEN_RESET_PATH)
/**
 * Reset storage path such as web storage, web database, application cache and so on
 *
 * @param context context object
 *
 */
EAPI void ewk_context_storage_path_reset(Ewk_Context* ewkContext);
// #endif // ENABLE(TIZEN_RESET_PATH)

// #if ENABLE(TIZEN_APPLICATION_CACHE)

/**
 * Callback for ewk_context_application_cache_quota_get.
 *
 * @param quota application cache quota
 * @param user_data user_data will be passsed when ewk_context_application_cache_quota_get is called
 */
typedef void (*Ewk_Application_Cache_Quota_Get_Callback)(int64_t quota, void* user_data);

/**
 * Callback for ewk_context_application_cache_usage_for_origin_get.
 *
 * @param usage application cache usage for origin
 * @param user_data user_data will be passsed when ewk_context_application_cache_usage_for_origin_get is called
 */
typedef void (*Ewk_Application_Cache_Usage_For_Origin_Get_Callback)(int64_t usage, void* user_data);

/**
 * Callback for ewk_context_application_cache_path_get.
 *
 * @param path application cache directory
 * @param user_data user_data will be passsed when ewk_context_application_cache_path_get is called
 */
typedef void (*Ewk_Application_Cache_Path_Get_Callback)(const char* path, void* user_data);

/**
 * Requests for setting application cache path.
 *
 * @param context context object
 * @param path application cache path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_application_cache_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for getting application cache path.
 *
 * @param context context object
 * @param callback callback to get web application cache directory
 * @param userData will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_application_cache_path_get(Ewk_Context* ewkContext, Ewk_Application_Cache_Path_Get_Callback callback, void* userData);

/**
 * Requests for getting application cache quota.
 *
 * @param context context object
 * @param result_callback callback to get web database quota
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_application_cache_quota_get(Ewk_Context* context, Ewk_Application_Cache_Quota_Get_Callback callback, void* user_data);

/**
 * Requests for setting application cache quota.
 *
 * @param context context object
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_application_cache_quota_set(Ewk_Context* context, int64_t quota);

/**
 * Requests for setting application cache quota for origin.
 *
 * @param context context object
 * @param origin serucity origin
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_application_cache_quota_for_origin_set(Ewk_Context* context, const Ewk_Security_Origin* origin, int64_t quota);

/**
 * Requests for getting application cache usage for origin.
 *
 * @param context context object
 * @param origin security origin
 * @param result_callback callback to get web database usage
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
Eina_Bool ewk_context_application_cache_usage_for_origin_get(Ewk_Context* context, const Ewk_Security_Origin* origin, Ewk_Application_Cache_Usage_For_Origin_Get_Callback callback, void* userData);
// #endif // ENABLE(TIZEN_APPLICATION_CACHE)

// #if Deprecated from here
/**
 * @typedef Ewk_Local_File_System_Origins_Get_Callback Ewk_Local_File_System_Origins_Get_Callback
 * @brief Type definition for use with ewk_context_local_file_system_origins_get()
 */
typedef void (*Ewk_Local_File_System_Origins_Get_Callback)(Eina_List *origins, void *user_data);

/**
 * Requests for setting local file system path.
 *
 * @param context context object
 * @param path local file system path
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_local_file_system_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for deleting all local file systems.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_local_file_system_all_delete(Ewk_Context *context);

/**
 * Requests for deleting local file system for origin.
 *
 * @param context context object
 * @param origin local file system origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_local_file_system_delete(Ewk_Context *context, Ewk_Security_Origin *origin);

 /**
 * Requests for getting local file system origins.
 *
 * @param context context object
 * @param result_callback callback to get local file system origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EAPI Eina_Bool ewk_context_local_file_system_origins_get(const Ewk_Context *context, Ewk_Local_File_System_Origins_Get_Callback callback, void *user_data);
// Deprecated to here

// #if ENABLE(TIZEN_WEB_STORAGE)
/**
 * Callback for ewk_context_web_storage_origins_get.
 *
 * @param origins web storage origins
 * @param user_data user_data will be passsed when ewk_context_web_storage_origins_get is called
 */
typedef void (*Ewk_Web_Storage_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * Callback for ewk_context_web_storage_usage_for_origin_get.
 *
 * @param usage usage of web storage
 * @param user_data user_data will be passsed when ewk_context_web_storage_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Storage_Usage_Get_Callback)(uint64_t usage, void* user_data);

/**
 * Callback for ewk_context_web_storage_path_get.
 *
 * @param path web storage directory
 * @param user_data user_data will be passsed when ewk_context_web_storage_path_get is called
 */
typedef void (*Ewk_Web_Storage_Path_Get_Callback)(const char* path, void* user_data);

/**
 * Deletes all web storage.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* context);

/**
 * Deletes origin that is stored in web storage db.
 *
 * @param context context object
 * @param origin origin of db
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_storage_origin_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * Gets list of origins that is stored in web storage db.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param context context object
 * @param result_callback callback to get web storage origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 *
 * @See ewk_context_origins_free()
 */
EAPI Eina_Bool ewk_context_web_storage_origins_get(Ewk_Context* context, Ewk_Web_Storage_Origins_Get_Callback callback, void* user_data);

/**
 * Requests for setting web storage path.
 *
 * @param context context object
 * @param path web storage path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_storage_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for getting web storage path.
 *
 * @param context context object
 * @param callback callback to get web storage directory
 * @param userData will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_storage_path_get(Ewk_Context* context, Ewk_Web_Storage_Path_Get_Callback callback, void* user_data);

/**
 * Gets usage of web storage for certain origin.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param context context object
 * @param origin security origin
 * @param callback callback to get web storage usage
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_storage_usage_for_origin_get(Ewk_Context* context, Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* user_data);
// #endif // ENABLE(TIZEN_WEB_STORAGE)

// #if ENABLE(TIZEN_SQL_DATABASE)
typedef struct Ewk_Context_Exceeded_Quota Ewk_Context_Exceeded_Quota;

/**
 * Callback for ewk_context_web_database_origins_get.
 *
 * @param origins web database origins
 * @param user_data user_data will be passsed when ewk_context_web_database_origins_get is called
 */
typedef void (*Ewk_Web_Database_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * Callback for ewk_context_web_database_quota_for_origin_get.
 *
 * @param quota web database quota
 * @param user_data user_data will be passsed when ewk_context_web_database_quota_for_origin_get is called
 */
typedef void (*Ewk_Web_Database_Quota_Get_Callback)(uint64_t quota, void* user_data);

/**
 * Callback for ewk_context_web_database_usage_for_origin_get.
 *
 * @param usage web database usage
 * @param user_data user_data will be passsed when ewk_context_web_database_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Database_Usage_Get_Callback)(uint64_t usage, void* user_data);

/**
 * Callback for ewk_context_web_database_path_get.
 *
 * @param path web database directory
 * @param user_data user_data will be passsed when ewk_context_web_database_path_get is called
 */
typedef void (*Ewk_Web_Database_Path_Get_Callback)(const char* path, void* user_data);

/**
 * Requests for getting security origin of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return security origin of exceeded database quota
 */
EAPI Ewk_Security_Origin* ewk_context_web_database_exceeded_quota_security_origin_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting database name of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return database name of exceeded database quota
 */
EAPI const char* ewk_context_web_database_exceeded_quota_database_name_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting display name of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return display name of exceeded database quota
 */
EAPI const char* ewk_context_web_database_exceeded_quota_display_name_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/*
 * Requests for getting current quota of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return current quota of exceeded database quota
 */
EAPI unsigned long long ewk_context_web_database_exceeded_quota_current_quota_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting current origin usage of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return current origin usage of exceeded database quota
 */
EAPI unsigned long long ewk_context_web_database_exceeded_quota_current_origin_usage_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting current database usage of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return current database usage of exceeded database quota
 */
EAPI unsigned long long ewk_context_web_database_exceeded_quota_current_database_usage_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting expected usage of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return expected usage of exceeded database quota
 */
EAPI unsigned long long ewk_context_web_database_exceeded_quota_expected_usage_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting current database usage of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return current database usage of exceeded database quota
 */
EAPI unsigned long long ewk_context_web_database_exceeded_quota_current_database_usage_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for getting expected usage of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 *
 * @return expected usage of exceeded database quota
 */
EAPI unsigned long long ewk_context_web_database_exceeded_quota_expected_usage_get(Ewk_Context_Exceeded_Quota* exceeded_quota);

/**
 * Requests for setting new quota of exceeded database quota data
 *
 * @param exceeded_qouta data of exceeded database quota
 * @param quota new size of database quota
 */
EAPI void ewk_context_web_database_exceeded_quota_new_quota_set(Ewk_Context_Exceeded_Quota* exceeded_quota, unsigned long long quota);

/**
 * Requests for deleting all web databases.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_delete_all(Ewk_Context* context);

/**
 * Requests for deleting web databases for origin.
 *
 * @param context context object
 * @param origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * Requests for getting web database origins.
 *
 * @param context context object
 * @param result_callback callback to get web database origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EAPI Eina_Bool ewk_context_web_database_origins_get(Ewk_Context* context, Ewk_Web_Database_Origins_Get_Callback callback, void* user_data);

/**
 * Requests for setting web database path.
 *
 * @param context context object
 * @param path web database path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for getting web database path.
 *
 * @param context context object
 * @param callback callback to get web database directory
 * @param userData will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_path_get(Ewk_Context* context, Ewk_Web_Database_Path_Get_Callback callback, void* user_data);

/**
 * Requests for getting web database quota for origin.
 *
 * @param context context object
 * @param result_callback callback to get web database quota
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 * @param origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_quota_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Quota_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin);

/**
 * Requests for setting web database default quota.
 *
 * @param context context object
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_default_quota_set(Ewk_Context* context, uint64_t quota);

/**
 * Requests for setting web database quota for origin.
 *
 * @param context context object
 * @param origin database origin
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_quota_for_origin_set(Ewk_Context* context, Ewk_Security_Origin* origin, uint64_t quota);

/**
 * Requests for getting web database usage for origin.
 *
 * @param context context object
 * @param result_callback callback to get web database usage
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 * @param origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_web_database_usage_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Usage_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin);
// #endif // ENABLE(TIZEN_SQL_DATABASE)

// #if ENABLE(TIZEN_CERTIFICATE_HANDLING)
/**
* Request to set certifcate file
*
* @param context context object
* @param certificate_file is path where certificate file is stored
*
* @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
*/
EAPI Eina_Bool ewk_context_certificate_file_set(Ewk_Context *context, const char *certificate_file);

/**
* Request to get certifcate file
*
* @param context context object
*
* @return @c certificate_file is path which is set during ewk_context_certificate_file_set or @c NULL otherwise
*/
EAPI const char* ewk_context_certificate_file_get(const Ewk_Context *context);
// #endif // ENABLE(TIZEN_CERTIFICATE_HANDLING)

// #if ENABLE(TIZEN_EXTENSIBLE_API)
/// Defines extensible api
enum Ewk_Extensible_API {
    EWK_EXTENSIBLE_API_BACKGROUND_MUSIC,
    EWK_EXTENSIBLE_API_AUDIO_RECORDING_CONTROL,
    EWK_EXTENSIBLE_API_CAMERA_CONTROL,
    EWK_EXTENSIBLE_API_CSP,
    EWK_EXTENSIBLE_API_ENCRYPTION_DATABASE,
    EWK_EXTENSIBLE_API_FULL_SCREEN,
    EWK_EXTENSIBLE_API_FULL_SCREEN_FORBID_AUTO_EXIT,
    EWK_EXTENSIBLE_API_MEDIA_STREAM_RECORD,
    EWK_EXTENSIBLE_API_MEDIA_VOLUME_CONTROL,
    EWK_EXTENSIBLE_API_PRERENDERING_FOR_ROTATION,
    EWK_EXTENSIBLE_API_ROTATE_CAMERA_VIEW,
    EWK_EXTENSIBLE_API_ROTATION_LOCK,
    EWK_EXTENSIBLE_API_SOUND_MODE,
    EWK_EXTENSIBLE_API_VISIBILITY_SUSPEND,
    EWK_EXTENSIBLE_API_XWINDOW_FOR_FULL_SCREEN_VIDEO,
    EWK_EXTENSIBLE_API_BACKGROUND_VIBRATION
};
/// Creates a type name for @a _Ewk_Extensible_API.
typedef enum Ewk_Extensible_API Ewk_Extensible_API;

 /**
 * Toggles tizen extensible api enable and disable
 *
 * @param context context object
 * @param extensibleAPI extensible API of every kind
 * @param enable enable or disable tizen extensible api
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_context_tizen_extensible_api_set(Ewk_Context *ewkContext, Ewk_Extensible_API extensibleAPI, Eina_Bool enable);
// #endif // ENABLE(TIZEN_EXTENSIBLE_API)

#ifdef __cplusplus
}
#endif

#endif // ewk_context_h
