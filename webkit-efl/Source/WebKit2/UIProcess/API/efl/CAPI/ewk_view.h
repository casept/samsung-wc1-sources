/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/**
 * @file    ewk_view.h
 * @brief   WebKit main smart object.
 *
 * This object provides view related APIs of WebKit2 to EFL object.
 */

#ifndef ewk_view_h
#define ewk_view_h

#include "ewk_back_forward_list.h"
#include "ewk_context.h"
#include "ewk_settings.h"
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Creates a new EFL WebKit view object.
 *
 * @param[in] e canvas object where to create the view object
 *
 * @return view object on success or @c NULL on failure
 */
EAPI Evas_Object *ewk_view_add(Evas *e);

/**
 * @brief Gets the Ewk_Context of this view.
 *
 * @param[in] o the view object to get the Ewk_Context
 *
 * @return the Ewk_Context of this view or @c NULL on failure
 */
EAPI Ewk_Context *ewk_view_context_get(const Evas_Object *o);

/**
 * @brief Asks the object to load the given URL.
 *
 * @param[in] o view object to load @a URL
 * @param[in] url uniform resource identifier to load
 *
 * @return @c EINA_TRUE is returned if @a o is valid, irrespective of load,\n
 *         or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_url_set(Evas_Object *o, const char *url);

/**
 * @brief Returns the current URL string of view object.
 *
 * @details It returns an internal string and should not\n
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param[in] o view object to get current URL
 *
 * @return current URL on success or @c NULL on failure
 */
EAPI const char *ewk_view_url_get(const Evas_Object *o);

/**
 * @brief Asks the main frame to reload the current document.
 *
 * @param[in] o view object to reload current document
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_reload(Evas_Object *o);

/**
 * @brief Asks the main frame to stop loading.
 *
 * @param[in] o view object to stop loading
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_view_stop(Evas_Object* o);

/**
 * @brief Gets the Ewk_Settings of this view.
 *
 * @param[in] o view object to get Ewk_Settings
 *
 * @return the Ewk_Settings of this view or @c NULL on failure
 */
EAPI Ewk_Settings *ewk_view_settings_get(const Evas_Object *o);

/**
 * @brief Asks the main frame to navigate back in the history.
 *
 * @param[in] o view object to navigate back
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_back(Evas_Object *o);

/**
 * @brief Asks the main frame to navigate forward in the history.
 *
 * @param[in] o view object to navigate forward
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_forward(Evas_Object *o);

/**
 * @brief Queries if it is possible to navigate backwards one item in the history.
 *
 * @param[in] o view object to query if backward navigation is possible
 *
 * @return @c EINA_TRUE if it is possible to navigate backwards in the history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_back_possible(Evas_Object *o);

/**
 * @brief Queries if it is possible to navigate forwards one item in the history.
 *
 * @param[in] o view object to query if forward navigation is possible
 *
 * @return @c EINA_TRUE if it is possible to navigate forwards in the history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_forward_possible(Evas_Object *o);

/**
 * @brief Gets the back-forward list associated with this view.
 *
 * @details The returned instance is unique for this view and thus multiple calls\n
 * to this function with the same view as parameter returns the same\n
 * handle. This handle is alive while view is alive, thus one\n
 * might want to listen for EVAS_CALLBACK_DEL on given view\n
 * (@a o) to know when to stop using returned handle.
 *
 * @param[in] o view object to get navigation back-forward list
 *
 * @return the back-forward list instance handle associated with this view
 */
EAPI Ewk_Back_Forward_List *ewk_view_back_forward_list_get(const Evas_Object *o);

/**
 * @brief Clear back forward list of a page.
 *
 * @param[in] o view object to clear back forward list
 */
EAPI void ewk_view_back_forward_list_clear(const Evas_Object *o);

/**
 * @brief Gets the current title of the main frame.
 *
 * @details It returns an internal string and should not\n
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param[in] o view object to get current title
 *
 * @return current title on success or @c NULL on failure
 */
EAPI const char *ewk_view_title_get(const Evas_Object *o);

/**
 * @brief Gets the current load progress of page.
 *
 * @details The progress estimation from 0.0 to 1.0.
 *
 * @param[in] o view object to get the current progress
 *
 * @return the load progress of page, value from 0.0 to 1.0,\n
 *         or @c -1.0 on failure
 */
EAPI double ewk_view_load_progress_get(const Evas_Object *o);

/**
* @brief Request to set the user agent string.
*
* @param[in] o view object to set the user agent string
* @param[in] user_agent the user agent string to set or @c NULL to restore the default one
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 * @param user_agent the user agent string to set or @c NULL to restore the default one
*/
EAPI Eina_Bool ewk_view_user_agent_set(Evas_Object* o, const char* user_agent);

/**
* @brief Returns user agent string.
*
* @param[in] o view object to get the user agent string
*
* @return @c user agent string
*/
EAPI const char* ewk_view_user_agent_get(const Evas_Object* o);

/**
 * @brief Gets last known contents size.
 *
 * @param[in] o view object to get contents size
 * @param[in] width pointer to store contents size width, may be @c 0
 * @param[in] height pointer to store contents size height, may be @c 0
 *
 * @return[in] @c EINA_TRUE on success or @c EINA_FALSE on failure and\n
 *         @a width and @a height will be zeroed
 */
EAPI Eina_Bool ewk_view_contents_size_get(const Evas_Object* o, Evas_Coord* width, Evas_Coord* height);

/**
 * @brief Callback for ewk_view_script_execute
 *
 * @param[in] o the view object
 * @param[in] result_value value returned by script
 * @param[in] user_data user data
 */
typedef void (*Ewk_View_Script_Execute_Cb)(Evas_Object* o, const char* result_value, void* user_data);

/**
 * @brief Requests execution of the given script.
 *
 * @remarks This allows to use NULL for the callback parameter.\n
 *       So, if the result data from the script is not required, NULL might be used for the callback parameter.
 *
 * @param[in] o view object to execute script
 * @param[in] script Java Script to execute
 * @param[in] callback result callback
 * @param[in] user_data user data
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_script_execute(Evas_Object* o, const char* script, Ewk_View_Script_Execute_Cb callback, void* user_data);

/**
 * @brief Scales the current page, centered at the given point.
 *
 * @param[in] o view object to set the zoom level
 * @param[in] scale_factor a new level to set
 * @param[in] cx x of center coordinate
 * @param[in] cy y of center coordinate
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_scale_set(Evas_Object *o, double scale_factor, int cx, int cy);

/**
 * @brief Queries the current scale factor of the page.
 *
 * @details It returns previous scale factor after ewk_view_scale_set is called immediately\n
 * until scale factor of page is really changed.
 *
 * @param[in] o view object to get the scale factor
 *
 * @return current scale factor in use on success or @c -1.0 on failure
 */
EAPI double ewk_view_scale_get(const Evas_Object *o);

/**
 * @brief Exit fullscreen when the back key is pressed.
 *
 * @param[in] o view object to exit fullscreen mode
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_fullscreen_exit(Evas_Object* o);

/**
 * @brief Suspend operation associated with view object.
 *
 * @param[in] o view object to suspend
 */
EAPI void ewk_view_suspend(Evas_Object* o);

/**
 * @brief Resume operation associated with view object after calling ewk_view_suspend().
 *
 * @param[in] o view object to resume
 */
EAPI void ewk_view_resume(Evas_Object* o);

/**
 * \enum Ewk_Http_Method
 * @brief Provides http method option
 */
enum Ewk_Http_Method {
    EWK_HTTP_METHOD_GET,
    EWK_HTTP_METHOD_HEAD,
    EWK_HTTP_METHOD_POST,
    EWK_HTTP_METHOD_PUT,
    EWK_HTTP_METHOD_DELETE,
};
/**
 * @brief Creates a type name for the Ewk_Http_Method.
 */
typedef enum Ewk_Http_Method Ewk_Http_Method;

/**
 * @brief Requests loading of the given request data.
 *
 * @param[in] o view object to load
 * @param[in] url uniform resource identifier to load
 * @param[in] method http method
 * @param[in] headers http headers
 * @param[in] body http body data
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_url_request_set(Evas_Object* o, const char* url, Ewk_Http_Method method, Eina_Hash* headers, const char* body);

/**
 * @brief Requests loading the given contents by mime type into the view object.
 *
 * @param[in] o view object to load
 * @param[in] contents what to load
 * @param[in] contents_size size of @a contents (in bytes),
 * @param[in] mime_type type of @a contents data, if @c 0 is given "text/html" is assumed
 * @param[in] encoding encoding for @a contents data, if @c 0 is given "UTF-8" is assumed
 * @param[in] base_uri base uri to use for relative resources, may be @c 0,\n
 *        if provided @b must be an absolute uri
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_contents_set(Evas_Object* o, const char* contents, size_t contents_size, char* mime_type, char* encoding, char* base_uri);

/**
 * @brief Scrolls webpage of view by dx and dy.
 *
 * @param[in] o view object to scroll
 * @param[in] dx horizontal offset to scroll
 * @param[in] dy vertical offset to scroll
 */
EAPI void ewk_view_scroll_by(Evas_Object* o, int dx, int dy);

/**
 * @brief Gets the current scroll position of given view.
 *
 * @param[in] o view object to get the current scroll position
 * @param[in] x the pointer to store the horizontal position, may be @c 0
 * @param[in] y the pointer to store the vertical position, may be @c 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise and\n
 *         values are zeroed.
 */
EAPI Eina_Bool ewk_view_scroll_pos_get(Evas_Object* o, int* x, int* y);

/**
 * @brief Sets an absolute scroll of the given view.
 *
 * @details Both values are from zero to the contents size minus the viewport size.
 *
 * @param[in] o view object to scroll
 * @param[in] x horizontal position to scroll
 * @param[in] y vertical position to scroll
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_scroll_set(Evas_Object* o, int x, int y);

#ifndef ewk_find_options_type
#define ewk_find_options_type
/**
 * Enum values used to specify search options.
 * @brief   Provides option to find text
 */
enum Ewk_Find_Options {
    EWK_FIND_OPTIONS_NONE, /**< no search flags, this means a case sensitive, no wrap, forward only search. */
    EWK_FIND_OPTIONS_CASE_INSENSITIVE = 1 << 0, /**< case insensitive search. */
    EWK_FIND_OPTIONS_AT_WORD_STARTS = 1 << 1, /**< search text only at the beginning of the words. */
    EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START = 1 << 2, /**< treat capital letters in the middle of words as word start. */
    EWK_FIND_OPTIONS_BACKWARDS = 1 << 3, /**< search backwards. */
    EWK_FIND_OPTIONS_WRAP_AROUND = 1 << 4, /**< if not present search will stop at the end of the document. */
    EWK_FIND_OPTIONS_SHOW_OVERLAY = 1 << 5, /**< show overlay */
    EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR = 1 << 6, /**< show indicator */
    EWK_FIND_OPTIONS_SHOW_HIGHLIGHT = 1 << 7 /**< show highlight */
};
/**
 * @brief Creates a type name for the Ewk_Find_Options.
 */
typedef enum Ewk_Find_Options Ewk_Find_Options;
#endif

/**
 * @brief Searches and hightlights the given string in the document.
 *
 * @param[in] o view object to find text
 * @param[in] text text to find
 * @param[in] options options to find
 * @param[in] max_match_count maximum match count to find, unlimited if 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_text_find(Evas_Object *o, const char *text, Ewk_Find_Options options, unsigned max_match_count);

/**
 * @brief Loads the specified @a html string as the content of the view.
 *
 * @details External objects such as stylesheets or images referenced in the HTML
 * document are located relative to @a baseUrl.
 *
 * If an @a unreachableUrl is passed it is used as the url for the loaded
 * content. This is typically used to display error pages for a failed
 * load.
 *
 * @since_tizen 2.3.1
 *
 * @param[in] o view object to load the HTML into
 * @param[in] html HTML data to load
 * @param[in] base_url Base URL used for relative paths to external objects (optional)
 * @param[in] unreachable_url URL that could not be reached (optional)
 *
 * @return @c EINA_TRUE if it the HTML was successfully loaded, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_html_string_load(Evas_Object *o, const char *html, const char *base_url, const char *unreachable_url);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_view_h
