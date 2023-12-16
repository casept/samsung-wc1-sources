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
 *
 * The following signals (see evas_object_smart_callback_add()) are emitted:
 *
 * - "authentication,request", Ewk_Auth_Request*: reports that user authentication was requested. Call
 *   ewk_auth_request_ref() on the request object to process the authentication asynchronously.
 * - "back,forward,list,changed", void: reports that the view's back / forward list had changed.
 * - "cancel,vibration", void: request to cancel the vibration.
 * - "contents,size,changed", Ewk_CSS_Size*: reports that contents size was changed.
 * - "download,cancelled", Ewk_Download_Job*: reports that a download was effectively cancelled.
 * - "download,failed", Ewk_Download_Job_Error*: reports that a download failed with the given error.
 * - "download,finished", Ewk_Download_Job*: reports that a download completed successfully.
 * - "download,request", Ewk_Download_Job*: reports that a new download has been requested. The client should set the
 *   destination path by calling ewk_download_job_destination_set() or the download will fail.
 * - "file,chooser,request", Ewk_File_Chooser_Request*: reports that a request has been made for the user to choose
 *   a file (or several) on the file system. Call ewk_file_chooser_request_ref() on the request object to process it
 *   asynchronously.
 * - "form,submission,request", Ewk_Form_Submission_Request*: Reports that a form request is about to be submitted.
 *   The Ewk_Form_Submission_Request passed contains information about the text fields of the form. This
 *   is typically used to store login information that can be used later to pre-fill the form.
 *   The form will not be submitted until ewk_form_submission_request_submit() is called.
 *   It is possible to handle the form submission request asynchronously, by simply calling
 *   ewk_form_submission_request_ref() on the request and calling ewk_form_submission_request_submit()
 *   when done to continue with the form submission. If the last reference is removed on a
 *   #Ewk_Form_Submission_Request and the form has not been submitted yet,
 *   ewk_form_submission_request_submit() will be called automatically.
 * - "favicon,changed", void: reports that the view's favicon has changed.
 *   The favicon can be queried using ewk_view_favicon_get().
 * - "load,error", const Ewk_Error*: reports main frame load failed.
 * - "load,finished", void: reports load finished.
 * - "load,progress", double*: load progress has changed (value from 0.0 to 1.0).
 * - "load,provisional,failed", const Ewk_Error*: view provisional load failed.
 * - "load,provisional,redirect", void: view received redirect for provisional load.
 * - "load,provisional,started", void: view started provisional load.
 * - "policy,decision,navigation", Ewk_Navigation_Policy_Decision*: a navigation policy decision should be taken.
 *   To make a policy decision asynchronously, simply increment the reference count of the
 *   #Ewk_Navigation_Policy_Decision object using ewk_navigation_policy_decision_ref().
 * - "policy,decision,new,window", Ewk_Navigation_Policy_Decision*: a new window policy decision should be taken.
 *   To make a policy decision asynchronously, simply increment the reference count of the
 *   #Ewk_Navigation_Policy_Decision object using ewk_navigation_policy_decision_ref().
 * - "text,found", unsigned int*: the requested text was found and it gives the number of matches.
 * - "title,changed", const char*: title of the main frame was changed.
 * - "tooltip,text,set", const char*: tooltip was set.
 * - "tooltip,text,unset", void: tooltip was unset.
 * - "url,changed", const char*: url of the main frame was changed.
 * - "vibrate", uint64_t*: request to vibrate. (value is vibration time)
 * - "webprocess,crashed", Eina_Bool*: expects a @c EINA_TRUE if web process crash is handled; @c EINA_FALSE, otherwise.
 */

#ifndef ewk_view_h
#define ewk_view_h

#include "ewk_back_forward_list.h"
#include "ewk_color_picker.h"
#include "ewk_context.h"
#include "ewk_context_menu.h"
#include "ewk_download_job.h"
#include "ewk_error.h"
#include "ewk_hit_test.h"
#include "ewk_page_group.h"
#include "ewk_popup_menu.h"
#include "ewk_security_origin.h"
#include "ewk_settings.h"
#include "ewk_touch.h"
#include "ewk_url_request.h"
#include "ewk_url_response.h"
#include "ewk_web_application_icon_data.h"
#include "ewk_window_features.h"
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Enum values containing text directionality values.
typedef enum {
    EWK_TEXT_DIRECTION_RIGHT_TO_LEFT,
    EWK_TEXT_DIRECTION_LEFT_TO_RIGHT
} Ewk_Text_Direction;

/// Enum values containing page contents type values.
typedef enum {
    EWK_PAGE_CONTENTS_TYPE_MHTML,
    EWK_PAGE_CONTENTS_TYPE_STRING
} Ewk_Page_Contents_Type;

/// Enum values for ORIENTATION SUPPORT
typedef enum {
    EWK_SCREEN_ORIENTATION_PORTRAIT_PRIMARY = 1,
    EWK_SCREEN_ORIENTATION_LANDSCAPE_PRIMARY = 1 << 1,
    EWK_SCREEN_ORIENTATION_PORTRAIT_SECONDARY = 1 << 2,
    EWK_SCREEN_ORIENTATION_LANDSCAPE_SECONDARY = 1 << 3
} Ewk_Screen_Orientation;

typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
typedef struct Ewk_View_Smart_Class Ewk_View_Smart_Class;

/// Ewk view's class, to be overridden by sub-classes.
struct Ewk_View_Smart_Class {
    Evas_Smart_Class sc; /**< all but 'data' is free to be changed. */
    unsigned long version;

    Eina_Bool (*custom_item_selected)(Ewk_View_Smart_Data *sd, Ewk_Context_Menu_Item *item);
    Eina_Bool (*context_menu_show)(Ewk_View_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Ewk_Context_Menu *menu);
    Eina_Bool (*context_menu_hide)(Ewk_View_Smart_Data *sd);

    Eina_Bool (*popup_menu_show)(Ewk_View_Smart_Data *sd, Eina_Rectangle rect, Ewk_Text_Direction text_direction, double page_scale_factor, Ewk_Popup_Menu *menu);
    Eina_Bool (*popup_menu_hide)(Ewk_View_Smart_Data *sd);

    // TIZEN_SCREEN_ORIENTATION_SUPPORT
    Eina_Bool (*orientation_lock)(Ewk_View_Smart_Data *sd, int orientations);
    void (*orientation_unlock)(Ewk_View_Smart_Data *sd);

// #if ENABLE(TIZEN_TEXT_SELECTION)
    Eina_Bool (*text_selection_down)(Ewk_View_Smart_Data *sd, int x, int y);
    Eina_Bool (*text_selection_move)(Ewk_View_Smart_Data *sd, int x, int y);
    Eina_Bool (*text_selection_up)(Ewk_View_Smart_Data *sd, int x, int y);
// #endif

    // event handling:
    //  - returns true if handled
    //  - if overridden, have to call parent method if desired
    Eina_Bool (*focus_in)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*focus_out)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*fullscreen_enter)(Ewk_View_Smart_Data *sd, Ewk_Security_Origin *origin);
    Eina_Bool (*fullscreen_exit)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*mouse_wheel)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Wheel *ev);
    Eina_Bool (*mouse_down)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Down *ev);
    Eina_Bool (*mouse_up)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Up *ev);
    Eina_Bool (*mouse_move)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Move *ev);
    Eina_Bool (*key_down)(Ewk_View_Smart_Data *sd, const Evas_Event_Key_Down *ev);
    Eina_Bool (*key_up)(Ewk_View_Smart_Data *sd, const Evas_Event_Key_Up *ev);
    Eina_Bool (*window_geometry_set)(Ewk_View_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height);
    Eina_Bool (*window_geometry_get)(Ewk_View_Smart_Data *sd, Evas_Coord *x, Evas_Coord *y, Evas_Coord *width, Evas_Coord *height);

    // javascript popup:
    //   - All strings should be guaranteed to be stringshared.
    void (*run_javascript_alert)(Ewk_View_Smart_Data *sd, const char *message);
    Eina_Bool (*run_javascript_confirm)(Ewk_View_Smart_Data *sd, const char *message);
    const char *(*run_javascript_prompt)(Ewk_View_Smart_Data *sd, const char *message, const char *default_value); /**< return string should be stringshared. */

    // web speech popup:
    //   - Requires to use mike on device.
    Eina_Bool (*run_mike_use_allow)(Ewk_View_Smart_Data *sd, const char *message);

    // color picker:
    //   - Shows and hides color picker.
    Eina_Bool (*input_picker_color_request)(Ewk_View_Smart_Data *sd, Ewk_Color_Picker *color_picker);
    Eina_Bool (*input_picker_color_dismiss)(Ewk_View_Smart_Data *sd);

    // storage:
    //   - Web database.
    unsigned long long (*exceeded_database_quota)(Ewk_View_Smart_Data *sd, const char *databaseName, const char *displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage);

    // window creation and closing:
    //   - Create a new window with specified features and close window.
    Evas_Object *(*window_create)(Ewk_View_Smart_Data *sd, const Ewk_Window_Features *window_features);
    void (*window_close)(Ewk_View_Smart_Data *sd);
};

/**
 * The version you have to put into the version field
 * in the @a Ewk_View_Smart_Class structure.
 */
#define EWK_VIEW_SMART_CLASS_VERSION 8UL

/**
 * Initializer for whole Ewk_View_Smart_Class structure.
 *
 * @param smart_class_init initializer to use for the "base" field
 * (Evas_Smart_Class).
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 */
#define EWK_VIEW_SMART_CLASS_INIT(smart_class_init) {smart_class_init, EWK_VIEW_SMART_CLASS_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Initializer to zero a whole Ewk_View_Smart_Class structure.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NULL EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/**
 * Initializer to zero a whole Ewk_View_Smart_Class structure and set
 * name and version.
 *
 * Similar to EWK_VIEW_SMART_CLASS_INIT_NULL, but will set version field of
 * Evas_Smart_Class (base field) to latest EVAS_SMART_CLASS_VERSION and name
 * to the specific value.
 *
 * It will keep a reference to name field as a "const char *", that is,
 * name must be available while the structure is used (hint: static or global!)
 * and will not be modified.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(name) EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NAME_VERSION(name))

typedef struct EwkView EwkView;
/**
 * @brief Contains an internal View data.
 *
 * It is to be considered private by users, but may be extended or
 * changed by sub-classes (that's why it's in public header file).
 */
struct Ewk_View_Smart_Data {
    Evas_Object_Smart_Clipped_Data base;
    const Ewk_View_Smart_Class* api; /**< reference to casted class instance */
    Evas_Object* self; /**< reference to owner object */
    Evas_Object* image; /**< reference to evas_object_image for drawing web contents */
    Evas_Object* effectImage;
    EwkView* priv; /**< should never be accessed, c++ stuff */
    struct {
        Evas_Coord x, y, w, h; /**< last used viewport */
    } view;
    struct { /**< what changed since last smart_calculate */
        Eina_Bool any:1;
        Eina_Bool size:1;
        Eina_Bool position:1;
    } changed;
};

/// Creates a type name for Ewk_Download_Job_Error.
typedef struct Ewk_Download_Job_Error Ewk_Download_Job_Error;

/**
 * @brief Structure containing details about a download failure.
 */
struct Ewk_Download_Job_Error {
    Ewk_Download_Job *download_job; /**< download that failed */
    Ewk_Error *error; /**< download error */
};

/// Creates a type name for Ewk_CSS_Size.
typedef struct Ewk_CSS_Size Ewk_CSS_Size;

/**
 * @brief Structure representing size.
 */
struct Ewk_CSS_Size {
    Evas_Coord w; /**< width */
    Evas_Coord h; /**< height */
};

/**
 * Enum values used to specify search options.
 * @brief   Provides option to find text
 * @info    Keep this in sync with WKFindOptions.h
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
typedef enum Ewk_Find_Options Ewk_Find_Options;

/**
 * Enum values used to set pagination mode.
 */
typedef enum {
    EWK_PAGINATION_MODE_INVALID = -1, /**< invalid pagination mode that will be returned when error occured. */
    EWK_PAGINATION_MODE_UNPAGINATED, /**< default mode for pagination. not paginated  */
    EWK_PAGINATION_MODE_LEFT_TO_RIGHT, /**< go to the next page with scrolling left to right horizontally. */
    EWK_PAGINATION_MODE_RIGHT_TO_LEFT, /**< go to the next page with scrolling right to left horizontally. */
    EWK_PAGINATION_MODE_TOP_TO_BOTTOM, /**< go to the next page with scrolling top to bottom vertically. */
    EWK_PAGINATION_MODE_BOTTOM_TO_TOP /**< go to the next page with scrolling bottom to top vertically. */
} Ewk_Pagination_Mode;

/**
 * @typedef Ewk_View_Script_Execute_Cb Ewk_View_Script_Execute_Cb
 * @brief Callback type for use with ewk_view_script_execute()
 */
typedef void (*Ewk_View_Script_Execute_Cb)(Evas_Object *o, const char *return_value, void *user_data);

/**
 * Creates a type name for the callback function used to get the page contents.
 *
 * @param type type of the contents
 * @param data string buffer of the contents
 * @param user_data user data will be passed when ewk_view_page_contents_get is called
 */
typedef void (*Ewk_Page_Contents_Cb)(Ewk_Page_Contents_Type type, const char *data, void *user_data);

/**
 * Sets the smart class APIs, enabling view to be inherited.
 *
 * @param api class definition to set, all members with the
 *        exception of @a Evas_Smart_Class->data may be overridden, must
 *        @b not be @c NULL
 *
 * @note @a Evas_Smart_Class->data is used to implement type checking and
 *       is not supposed to be changed/overridden. If you need extra
 *       data for your smart class to work, just extend
 *       Ewk_View_Smart_Class instead.
 *       The Evas_Object which inherits the ewk_view should use
 *       ewk_view_smart_add() to create Evas_Object instead of
 *       evas_object_smart_add() because it performs additional initialization
 *       for the ewk_view.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (probably
 *         version mismatch)
 *
 * @see ewk_view_smart_add()
 */
EAPI Eina_Bool ewk_view_smart_class_set(Ewk_View_Smart_Class *api);

/**
 * Creates a new EFL WebKit view object with Evas_Smart and Ewk_Context.
 *
 * @note The Evas_Object which inherits the ewk_view should create its
 *       Evas_Object using this API instead of evas_object_smart_add()
 *       because the default initialization for ewk_view is done in this API.
 *
 * @param e canvas object where to create the view object
 * @param smart Evas_Smart object. Its type should be EWK_VIEW_TYPE_STR
 * @param context Ewk_Context object which is used for initializing
 * @param pageGroup Ewk_Page_Group object which is used for initializing
 *
 * @return view object on success or @c NULL on failure
 */
EAPI Evas_Object *ewk_view_smart_add(Evas *e, Evas_Smart *smart, Ewk_Context *context, Ewk_Page_Group *pageGroup);

/**
 * Creates a new EFL WebKit view object.
 *
 * @param e canvas object where to create the view object
 *
 * @return view object on success or @c NULL on failure
 */
EAPI Evas_Object *ewk_view_add(Evas *e);

/**
 * Creates a new EFL WebKit view object based on specific Ewk_Context.
 *
 * @param e canvas object where to create the view object
 * @param context Ewk_Context object to declare process model
 *
 * @return view object on success or @c NULL on failure
 */
EAPI Evas_Object *ewk_view_add_with_context(Evas *e, Ewk_Context *context);

/**
 * Gets the Ewk_Context of this view.
 *
 * @param o the view object to get the Ewk_Context
 *
 * @return the Ewk_Context of this view or @c NULL on failure
 */
EAPI Ewk_Context *ewk_view_context_get(const Evas_Object *o);

/**
 * Gets the Ewk_Page_Group of this view.
 *
 * @param o the view object to get the Ewk_Page_Group
 *
 * @return the Ewk_Page_Group of this view or @c NULL on failure
 */
EAPI Ewk_Page_Group *ewk_view_page_group_get(const Evas_Object *o);

/**
 * Asks the object to load the given URL.
 *
 * @param o view object to load @a URL
 * @param url uniform resource identifier to load
 *
 * @return @c EINA_TRUE is returned if @a o is valid, irrespective of load,
 *         or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_url_set(Evas_Object *o, const char *url);

/**
 * Returns the current URL string of view object.
 *
 * It returns an internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o view object to get current URL
 *
 * @return current URL on success or @c NULL on failure
 */
EAPI const char *ewk_view_url_get(const Evas_Object *o);

/**
 * Returns the current favicon of view object.
 *
 * @param o view object to get current icon URL
 *
 * @return current favicon on success or @c NULL if unavailable or on failure.
 * The returned Evas_Object needs to be freed after use.
 */
EAPI Evas_Object *ewk_view_favicon_get(const Evas_Object *o);

/**
 * Asks the main frame to reload the current document.
 *
 * @param o view object to reload current document
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_view_reload_bypass_cache()
 */
EAPI Eina_Bool    ewk_view_reload(Evas_Object *o);

/**
 * Reloads the current page's document without cache.
 *
 * @param o view object to reload current document
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_reload_bypass_cache(Evas_Object *o);

/**
 * Asks the main frame to stop loading.
 *
 * @param o view object to stop loading
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool    ewk_view_stop(Evas_Object *o);

/**
 * Gets the Ewk_Settings of this view.
 *
 * @param o view object to get Ewk_Settings
 *
 * @return the Ewk_Settings of this view or @c NULL on failure
 */
EAPI Ewk_Settings *ewk_view_settings_get(const Evas_Object *o);

/**
 * Asks the main frame to navigate back in the history.
 *
 * @param o view object to navigate back
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_frame_back()
 */
EAPI Eina_Bool    ewk_view_back(Evas_Object *o);

/**
 * Asks the main frame to navigate forward in the history.
 *
 * @param o view object to navigate forward
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_frame_forward()
 */
EAPI Eina_Bool    ewk_view_forward(Evas_Object *o);

/**
 * Queries if it is possible to navigate backwards one item in the history.
 *
 * @param o view object to query if backward navigation is possible
 *
 * @return @c EINA_TRUE if it is possible to navigate backwards in the history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_back_possible(Evas_Object *o);

/**
 * Queries if it is possible to navigate forwards one item in the history.
 *
 * @param o view object to query if forward navigation is possible
 *
 * @return @c EINA_TRUE if it is possible to navigate forwards in the history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_forward_possible(Evas_Object *o);

/**
 * Gets the back-forward list associated with this view.
 *
 * The returned instance is unique for this view and thus multiple calls
 * to this function with the same view as parameter returns the same
 * handle. This handle is alive while view is alive, thus one
 * might want to listen for EVAS_CALLBACK_DEL on given view
 * (@a o) to know when to stop using returned handle.
 *
 * @param o view object to get navigation back-forward list
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
 * Navigates to specified back-forward list item.
 *
 * @param o view object to navigate in the history
 * @param item the back-forward list item
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_navigate_to(Evas_Object *o, const Ewk_Back_Forward_List_Item *item);

/**
 * Gets the current title of the main frame.
 *
 * It returns an internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o view object to get current title
 *
 * @return current title on success or @c NULL on failure
 */
EAPI const char *ewk_view_title_get(const Evas_Object *o);

/**
 * Gets the current load progress of page.
 *
 * The progress estimation from 0.0 to 1.0.
 *
 * @param o view object to get the current progress
 *
 * @return the load progress of page, value from 0.0 to 1.0,
 *         or @c -1.0 on failure
 */
EAPI double ewk_view_load_progress_get(const Evas_Object *o);

/**
 * Loads the specified @a html string as the content of the view.
 *
 * External objects such as stylesheets or images referenced in the HTML
 * document are located relative to @a baseUrl.
 *
 * If an @a unreachableUrl is passed it is used as the url for the loaded
 * content. This is typically used to display error pages for a failed
 * load.
 *
 * @param o view object to load the HTML into
 * @param html HTML data to load
 * @param base_url Base URL used for relative paths to external objects (optional)
 * @param unreachable_url URL that could not be reached (optional)
 *
 * @return @c EINA_TRUE if it the HTML was successfully loaded, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_html_string_load(Evas_Object *o, const char *html, const char *base_url, const char *unreachable_url);

/**
 * Scales the current page, centered at the given point.
 *
 * @param o view object to set the zoom level
 * @param scale_factor a new level to set
 * @param cx x of center coordinate
 * @param cy y of center coordinate
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_scale_set(Evas_Object *o, double scaleFactor, int x, int y);

/**
 * Queries the current scale factor of the page.
 *
 * It returns previous scale factor after ewk_view_scale_set is called immediately
 * until scale factor of page is really changed.
 *
 * @param o view object to get the scale factor
 *
 * @return current scale factor in use on success or @c -1.0 on failure
 */
EAPI double ewk_view_scale_get(const Evas_Object *o);

/**
 * Sets zoom factor of the current page.
 *
 * @param o view object to set the zoom level
 * @param zoom_factor a new level to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_page_zoom_set(Evas_Object *o, double zoom_factor);

/**
 * Queries the current zoom factor of the page.
 *
 * It returns previous zoom factor after ewk_view_page_zoom_factor_set is called immediately
 * until zoom factor of page is really changed.
 *
 * @param o view object to get the zoom factor
 *
 * @return current zoom factor in use on success or @c -1.0 on failure
 */
EAPI double ewk_view_page_zoom_get(const Evas_Object *o);

/**
 * Queries the ratio between the CSS units and device pixels when the content is unscaled.
 *
 * When designing touch-friendly contents, knowing the approximated target size on a device
 * is important for contents providers in order to get the intented layout and element
 * sizes.
 *
 * As most first generation touch devices had a PPI of approximately 160, this became a
 * de-facto value, when used in conjunction with the viewport meta tag.
 *
 * Devices with a higher PPI learning towards 240 or 320, applies a pre-scaling on all
 * content, of either 1.5 or 2.0, not affecting the CSS scale or pinch zooming.
 *
 * This value can be set using this property and it is exposed to CSS media queries using
 * the -webkit-device-pixel-ratio query.
 *
 * For instance, if you want to load an image without having it upscaled on a web view
 * using a device pixel ratio of 2.0 it can be done by loading an image of say 100x100
 * pixels but showing it at half the size.
 *
 * @media (-webkit-min-device-pixel-ratio: 1.5) {
 *     .icon {
 *         width: 50px;
 *         height: 50px;
 *         url: "/images/icon@2x.png"; // This is actually a 100x100 image
 *     }
 * }
 *
 * If the above is used on a device with device pixel ratio of 1.5, it will be scaled
 * down but still provide a better looking image.
 *
 * @param o view object to get device pixel ratio
 *
 * @return the ratio between the CSS units and device pixels,
 *         or @c -1.0 on failure
 */
EAPI float ewk_view_device_pixel_ratio_get(const Evas_Object *o);

/**
 * Sets the ratio between the CSS units and device pixels when the content is unscaled.
 *
 * @param o view object to set device pixel ratio
 *
 * @return @c EINA_TRUE if the device pixel ratio was set, @c EINA_FALSE otherwise
 *
 * @see ewk_view_device_pixel_ratio_get()
 */
EAPI Eina_Bool ewk_view_device_pixel_ratio_set(Evas_Object *o, float ratio);

/**
 * Sets the theme path that will be used by this view.
 *
 * This also sets the theme on the main frame. As frames inherit theme
 * from their parent, this will have all frames with unset theme to
 * use this one.
 *
 * @param o view object to change theme
 * @param path theme path
 */
EAPI void ewk_view_theme_set(Evas_Object *o, const char *path);

/**
 * Gets the theme set on this view.
 *
 * This returns the value set by ewk_view_theme_set().
 *
 * @param o view object to get theme path
 *
 * @return the theme path, may be @c NULL if not set
 */
EAPI const char *ewk_view_theme_get(const Evas_Object *o);

/**
 * Gets the current custom character encoding name.
 *
 * @param o view object to get the current encoding
 *
 * @return @c eina_stringshare containing the current encoding, or
 *         @c NULL if it's not set
 */
EAPI const char *ewk_view_custom_encoding_get(const Evas_Object *o);

/**
 * Sets the custom character encoding and reloads the page.
 *
 * @param o view to set the encoding
 * @param encoding the new encoding to set or @c NULL to restore the default one
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_custom_encoding_set(Evas_Object *o, const char *encoding);

/**
 * Gets the current user agent string.
 *
 * @param o view object to get the current user agent
 *
 * @return @c eina_stringshare containing the current user agent, or
 *         @c default user agent if it's not set
 */
EAPI const char *ewk_view_user_agent_get(const Evas_Object *o);

/**
 * Sets the user agent string.
 *
 * @param o view to set the user agent
 * @param user_agent the user agent string to set or @c NULL to restore the default one
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_user_agent_set(Evas_Object *o, const char *user_agent);

/**
 * Searches and hightlights the given string in the document.
 *
 * @param o view object to find text
 * @param text text to find
 * @param options options to find
 * @param max_match_count maximum match count to find, unlimited if 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_text_find(Evas_Object *o, const char *text, Ewk_Find_Options options, unsigned max_match_count);

/**
 * Clears the highlight of searched text.
 *
 * @param o view object to find text
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object *o);

/**
 * Counts the given string in the document.
 *
 * This does not highlight the matched string and just count the matched string.
 *
 * As the search is carried out through the whole document,
 * only the following EWK_FIND_OPTIONS are valid.
 *  - EWK_FIND_OPTIONS_NONE
 *  - EWK_FIND_OPTIONS_CASE_INSENSITIVE
 *  - EWK_FIND_OPTIONS_AT_WORD_START
 *  - EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START
 *
 * The "text,found" callback will be called with the number of matched string.
 *
 * @param o view object to find text
 * @param text text to find
 * @param options options to find
 * @param max_match_count maximum match count to find, unlimited if 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_text_matches_count(Evas_Object *o, const char *text, Ewk_Find_Options options, unsigned max_match_count);

/**
 * Sets whether the ewk_view supports the mouse events or not.
 *
 * The ewk_view will support the mouse events if EINA_TRUE or not support the
 * mouse events otherwise. The default value is EINA_TRUE.
 *
 * @param o view object to enable/disable the mouse events
 * @param enabled a state to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_mouse_events_enabled_set(Evas_Object *o, Eina_Bool enabled);

/**
 * Queries if the ewk_view supports the mouse events.
 *
 * @param o view object to query if the mouse events are enabled
 *
 * @return @c EINA_TRUE if the mouse events are enabled or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_mouse_events_enabled_get(const Evas_Object *o);

/**
 * Feeds the touch event to the view.
 *
 * @param o view object to feed touch event
 * @param type the type of touch event
 * @param points a list of points (Ewk_Touch_Point) to process
 * @param modifiers an Evas_Modifier handle to the list of modifier keys
 *        registered in the Evas. Users can get the Evas_Modifier from the Evas
 *        using evas_key_modifier_get() and can set each modifier key using
 *        evas_key_modifier_on() and evas_key_modifier_off()
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_feed_touch_event(Evas_Object *o, Ewk_Touch_Event_Type type, const Eina_List *points, const Evas_Modifier *modifiers);

/**
 * Sets whether the ewk_view supports the touch events or not.
 *
 * The ewk_view will support the touch events if @c EINA_TRUE or not support the
 * touch events otherwise. The default value is @c EINA_FALSE.
 *
 * @param o view object to enable/disable the touch events
 * @param enabled a state to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_touch_events_enabled_set(Evas_Object *o, Eina_Bool enabled);

/**
 * Queries if the ewk_view supports the touch events.
 *
 * @param o view object to query if the touch events are enabled
 *
 * @return @c EINA_TRUE if the touch events are enabled or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_touch_events_enabled_get(const Evas_Object *o);

/**
 * Show the inspector to debug a web page.
 *
 * @param o The view to show the inspector.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_developer_extras_enabled_set()
 */
EAPI Eina_Bool ewk_view_inspector_show(Evas_Object *o);

/**
 * Close the inspector
 *
 * @param o The view to close the inspector.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_inspector_close(Evas_Object *o);

/**
 * Set pagination mode to the current web page.
 *
 * @param o view object to set the pagenation mode
 * @param mode The Ewk_Pagination_Mode to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_pagination_mode_set(Evas_Object *o, Ewk_Pagination_Mode mode);

/**
 * Get pagination mode of the current web page. 
 * 
 * The default value is EWK_PAGINATION_MODE_UNPAGINATED.
 * When error occured, EWK_PAGINATION_MODE_INVALID is returned. 
 *
 * @param o view object to get the pagination mode
 *
 * @return The pagination mode of the current web page
 */
EAPI Ewk_Pagination_Mode ewk_view_pagination_mode_get(const Evas_Object *o);

/**
 * Exit fullscreen mode.
 *
 * @param o view object where to exit fullscreen
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_fullscreen_exit(Evas_Object *o);

/**
 * Get contents of the current web page.
 *
 * @param o view object to get the page contents
 * @param type type of the page contents
 * @param callback callback function to be called when the operation is finished
 * @param user_data user data to be passed to the callback function
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_page_contents_get(const Evas_Object *o, Ewk_Page_Contents_Type type, Ewk_Page_Contents_Cb callback, void *user_data);

/**
 * Sets the source mode as EINA_TRUE to display the web source code
 * or EINA_FALSE otherwise. The default value is EINA_FALSE.
 *
 * This method should be called before loading new contents on web view
 * so that the new view mode will be applied to the new contents.
 *
 * @param o view object to set the view source mode
 * @param enabled a state to set view source mode
 *
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_source_mode_set(Evas_Object *o, Eina_Bool enabled);

/**
 * Gets the view source mode of the current web page.
 *
 * @param o view object to get the view source mode
 *
 * @return @c EINA_TRUE if the view mode is set to load source code, or
 *         @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_source_mode_get(const Evas_Object *o);

/**
 * Requests execution of the given script.
 *
 * The result value for the execution can be retrieved from the asynchronous callback.
 *
 * @param o The view to execute script
 * @param script JavaScript to execute
 * @param callback The function to call when the execution is completed, may be @c NULL
 * @param user_data User data, may be @c NULL
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_script_execute(Evas_Object *o, const char *script, Ewk_View_Script_Execute_Cb callback, void *user_data);

/**
 * Sets whether the ewk_view use fixed layout or not.
 *
 * The webview will use fixed layout if EINA_TRUE or not use it otherwise.
 * The default value is EINA_FALSE.
 *
 * @param o view object to set fixed layout
 * @param enabled a state to set
 *
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_layout_fixed_set(Evas_Object *o, Eina_Bool enabled);

/**
 * Queries if the webview is using fixed layout.
 *
 * @param o view object to query the status
 *
 * @return @c EINA_TRUE if the webview is using fixed layout, or
 *         @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_layout_fixed_get(const Evas_Object *o);

/**
 * Sets size of fixed layout to web page.
 *
 * The webview size will be set with given size.
 *
 * @param o view object to set fixed layout
 * @param width an integer value in which to set width of fixed layout
 * @param height an integer value in which to set height of fixed layout
 */
EAPI void ewk_view_layout_fixed_size_set(const Evas_Object *o, Evas_Coord width, Evas_Coord height);

/**
 * Gets the fixed layout size of current web page.
 *
 * @param o view object to query the size
 *
 * @param width pointer to an integer value in which to get the width of fixed layout
 * @param height pointer to an integer value in which to get the height of fixed layout
 */
EAPI void ewk_view_layout_fixed_size_get(const Evas_Object *o, Evas_Coord *width, Evas_Coord *height);

/**
 * Sets the background color and transparency of the view.
 *
 * @param o view object to change the background color
 * @param r red color component
 * @param g green color component
 * @param b blue color component
 * @param a transparency
 */
EAPI void ewk_view_bg_color_set(Evas_Object *o, int r, int g, int b, int a);

/**
 * Gets the background color of the view.
 *
 * @param o view object to get the background color
 * @param r the pointer to store red color component
 * @param g the pointer to store green color component
 * @param b the pointer to store blue color component
 * @param a the pointer to store alpha value
 */
EAPI void ewk_view_bg_color_get(Evas_Object *o, int *r, int *g, int *b, int *a);

/**
 * Get contents size of current web page.
 *
 * If it fails to get the content size, the width and height will be set to 0.
 *
 * @param o view object to get contents size
 * @param width pointer to an integer in which to store the width of contents.
 * @param height pointer to an integer in which to store the height of contents.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_contents_size_get(const Evas_Object *o, Evas_Coord *width, Evas_Coord *height);

//Deprecated and will be removed soon.
/**
* add custom header
*
* @param o view object to add custom header
*
* @param name custom header name to add the custom header
*
* @param value custom header value to add the custom header
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
*/
EAPI Eina_Bool ewk_view_custom_header_add(const Evas_Object* o, const char* name, const char* value);
/**
* remove custom header
*
* @param o view object to remove custom header
*
* @param name custom header name to remove the custom header
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
*/
EAPI Eina_Bool ewk_view_custom_header_remove(const Evas_Object* o, const char* name);
/**
* clears all custom headers
*
* @param o view object to clear custom headers
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
*/
EAPI Eina_Bool ewk_view_custom_header_clear(const Evas_Object* o);
//#endif Deprecated

//#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
/**
 * Notify the foreground/background status of app.
 *
 * @param o view object that is in given status.
 * @param enable EINA_TRUE to notify that page is foreground, EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_foreground_set(Evas_Object *o, Eina_Bool enable);
//#endif

/*** TIZEN EXTENSIONS ****/
/**
 * Creates a type name for the callback function used to get the page contents.
 *
 * @param o view object
 * @param data mhtml data of the page contents
 * @param user_data user data will be passed when ewk_view_mhtml_data_get is called
 */
typedef void (*Ewk_View_MHTML_Data_Get_Callback)(Evas_Object *o, const char *data, void *user_data);

/**
 * Callback for ewk_view_web_app_capable_get
 *
 * @param capable web application capable
 * @param user_data user_data will be passsed when ewk_view_web_app_capable_get is called
 */
typedef void (*Ewk_Web_App_Capable_Get_Callback)(Eina_Bool capable, void *user_data);

/**
 * Callback for ewk_view_web_app_icon_get
 *
 * @param icon_url web application icon
 * @param user_data user_data will be passsed when ewk_view_web_app_icon_get is called
 */
typedef void (*Ewk_Web_App_Icon_URL_Get_Callback)(const char *icon_url, void *user_data);

/**
 * Callback for ewk_view_web_app_icon_urls_get.
 *
 * @param icon_urls list of Ewk_Web_App_Icon_Data for web app
 * @param user_data user_data will be passsed when ewk_view_web_app_icon_urls_get is called
 */
typedef void (*Ewk_Web_App_Icon_URLs_Get_Callback)(Eina_List *icon_urls, void *user_data);

EAPI Eina_Bool ewk_view_custom_header_add(const Evas_Object *ewkView, const char *name, const char *value);

EAPI char *ewk_view_get_cookies_for_url(Evas_Object *ewkView, const char *url);

EAPI void ewk_view_orientation_send(Evas_Object *ewkView, int orientation);

/**
 * Requests loading the given contents by mime type into the view object.
 *
 * @param o view object to load
 * @param contents what to load
 * @param contents_size size of @a contents (in bytes),
 * @param mime_type type of @a contents data, if @c 0 is given "text/html" is assumed
 * @param encoding encoding for @a contents data, if @c 0 is given "UTF-8" is assumed
 * @param base_uri base uri to use for relative resources, may be @c 0,
 *        if provided @b must be an absolute uri
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_contents_set(Evas_Object* o, const char* contents, size_t contents_size, char* mime_type, char* encoding, char* base_uri);

// Use ewk_view_html_string_load instead of this.
EINA_DEPRECATED EAPI Eina_Bool ewk_view_html_contents_set(Evas_Object *ewkView, const char *html, const char *baseUri);

EAPI unsigned int ewk_view_inspector_server_start(Evas_Object *ewkView, unsigned int port);

EAPI Eina_Bool ewk_view_inspector_server_stop(Evas_Object *ewkView);

/**
 * Sets scrollbar visibility for current ewk view.
 *
 * @param o view object to set the scrollbar visibility
 *
 * @param visible visibility state to set
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_main_frame_scrollbar_visible_set(Evas_Object *o, Eina_Bool visible);

/**
 * Queries if the ewk view shows scrollbars.
 *
 * @param o view object to get the scrollbar visibility
 *
 * @return @c EINA_TRUE when scrollbars are visible, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_main_frame_scrollbar_visible_get(Evas_Object *o);

EAPI Eina_Bool ewk_view_mhtml_data_get(Evas_Object *ewkView, Ewk_View_MHTML_Data_Get_Callback callback, void *user_data);

EAPI Eina_Bool ewk_view_notification_closed(Evas_Object *ewkView, Eina_List *ewkNotifications);

EAPI void ewk_view_resume(Evas_Object *ewkView);

EAPI void ewk_view_suspend(Evas_Object *ewkView);

EAPI void ewk_view_scale_range_get(Evas_Object *ewkView, double *minimumScale, double *maximumScale);

EAPI Evas_Object *ewk_view_screenshot_contents_get(const Evas_Object *ewkView, Eina_Rectangle viewArea, float scaleFactor, Evas *canvas);

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

// #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
/**
 * Scrolls webpage of view by dx and dy.
 *
 * @param o view object to scroll
 * @param dx horizontal offset to scroll
 * @param dy vertical offset to scroll
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI void ewk_view_scroll_by(Evas_Object *o, int dx, int dy);

/**
 * Gets the current scroll position of given view.
 *
 * @param o view object to get the current scroll position
 * @param x the pointer to store the horizontal position, may be @c 0
 * @param y the pointer to store the vertical position, may be @c 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_view_scroll_pos_get(const Evas_Object *o, int *x, int *y);

/**
 * Sets an absolute scroll of the given view.
 *
 * Both values are from zero to the contents size minus the viewport
 * size.
 *
 * @param o view object to scroll
 * @param x horizontal position to scroll
 * @param y vertical position to scroll
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_scroll_set(Evas_Object *o, int x, int y);

/**
 * Gets the possible scroll size of the given view.
 *
 * Possible scroll size is contents size minus the viewport size.
 *
 * @param o view object to get scroll size
 * @param w the pointer to store the horizontal size that is possible to scroll,
 *        may be @c 0
 * @param h the pointer to store the vertical size that is possible to scroll,
 *        may be @c 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_view_scroll_size_get(const Evas_Object *o, int *w, int *h);
// #endif // #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)

/**
 * Gets the current text zoom level.
 *
 * @param o view object to get the zoom level
 *
 * @return current zoom level in use on success or @c -1.0 on failure
 */
EAPI double ewk_view_text_zoom_get(const Evas_Object *ewkView);

/**
 * Sets the current text zoom level.
 *
 * @param o view object to set the zoom level
 * @param textZoomFactor a new level to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_text_zoom_set(Evas_Object *ewkView, double textZoomFactor);

EAPI const char *ewk_view_text_selection_text_get(Evas_Object *ewkView);

EAPI Eina_Bool ewk_view_web_application_capable_get(Evas_Object *ewkView, Ewk_Web_App_Capable_Get_Callback callback, void *userData);

EAPI Eina_Bool ewk_view_web_application_icon_url_get(Evas_Object *ewkView, Ewk_Web_App_Icon_URL_Get_Callback callback, void *userData);

EAPI Eina_Bool ewk_view_web_application_icon_urls_get(Evas_Object *ewkView, Ewk_Web_App_Icon_URLs_Get_Callback callback, void *userData);

// #if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
/**
 * Creates a new hit test for the given veiw object and point.
 *
 * The returned object should be freed by ewk_hit_test_free().
 *
 * @param o view object to do hit test on
 * @param x the horizontal position to query
 * @param y the vertical position to query
 * @param hit_test_mode the Ewk_Hit_Test_Mode enum value to query
 *
 * @return a newly allocated hit test on success, @c 0 otherwise
 */
EAPI Ewk_Hit_Test* ewk_view_hit_test_new(Evas_Object* o, int x, int y, int hit_test_mode);
// #endif // #if ENABLE(TIZEN_WEBKIT2_HIT_TEST)

// #if ENABLE(TIZEN_CSP)
/// Enum values containing Content Security Policy header types.
enum _Ewk_CSP_Header_Type {
    EWK_REPORT_ONLY,
    EWK_ENFORCE_POLICY
};
typedef enum _Ewk_CSP_Header_Type Ewk_CSP_Header_Type;

/*
 * Set received Content Security Policy data from web app
 *
 * @param o view object
 * @param policy Content Security Policy data
 * @param type Content Security Policy header type
 *
 */
EAPI void ewk_view_content_security_policy_set(Evas_Object* o, const char* policy, Ewk_CSP_Header_Type type);
// #endif // #if ENABLE(TIZEN_CSP)

// #if ENABLE(TIZEN_MOBILE_WEB_PRINT)
/**
 * Create PDF file of page contents
 *
 * @param o view object to get page contents.
 * @param width the suface width of PDF file.
 * @param height the suface height of PDF file.
 * @param fileName the file name for creating PDF file.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_contents_pdf_get(Evas_Object* o, int width, int height, const char* fileName);
// #endif // ENABLE(TIZEN_MOBILE_WEB_PRINT)

/**
 * When font-family is "Tizen", use system's Settings font as default font-family
 *
 * @param o view object
 *
 */
EAPI void ewk_view_use_settings_font(Evas_Object *o);

// #if // ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
/**
 * Starts offline page save.
 *
 * @param o view object to start offline page save
 * @param path directory path to save offline page
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EAPI Eina_Bool ewk_view_page_save(Evas_Object* o, const char* path);
// #endif // ENABLE(TIZEN_OFFLINE_PAGE_SAVE)

// Deprecated from here
typedef Eina_Bool (*Ewk_View_Exceeded_Local_File_System_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  long long currentQuota, void* user_data);
EAPI void ewk_view_exceeded_local_file_system_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Local_File_System_Quota_Callback callback, void* user_data);
EAPI void ewk_view_exceeded_local_file_system_quota_reply(Evas_Object* o, Eina_Bool allow);
// to here

// #if ENABLE(TIZEN_WEB_STORAGE)
/**
 * Callback for ewk_view_web_storage_quota_get
 *
 * @param quota web storage quota
 * @param user_data user_data will be passed when ewk_view_web_storage_quota_get is called
 */
typedef void (*Ewk_Web_Storage_Quota_Get_Callback)(uint32_t quota, void* user_data);

/**
 * Requests for getting web storage quota.
 *
 * @param o view object to get web storage quota
 * @param result_cb callback to get web database origins
 * @param user_data user_data will be passed when result_cb is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_web_storage_quota_get(const Evas_Object* o, Ewk_Web_Storage_Quota_Get_Callback result_callback, void* user_data);

/**
 * Requests for setting web storage quota.
 *
 * @param o view object to set web storage quota
 * @param quota quota to store web storage db.
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_web_storage_quota_set(Evas_Object* o, uint32_t quota);
// #endif // #if ENABLE(TIZEN_WEB_STORAGE)

// #if ENABLE(TIZEN_TEXT_SELECTION)
EAPI Eina_Bool ewk_view_text_selection_range_clear(Evas_Object* o);
EAPI Eina_Bool ewk_view_text_selection_range_get(Evas_Object* o, Eina_Rectangle* left_rect, Eina_Rectangle* right_rect);
EAPI const char* ewk_view_text_selection_text_get(Evas_Object* o);
EAPI Eina_Bool ewk_view_text_selection_clear(Evas_Object *o);
EINA_DEPRECATED EAPI Eina_Bool ewk_view_text_selection_enable_set(Evas_Object* o, Eina_Bool enable);
EINA_DEPRECATED EAPI Eina_Bool ewk_view_auto_clear_text_selection_mode_set(Evas_Object* o, Eina_Bool enable);
EINA_DEPRECATED EAPI Eina_Bool ewk_view_auto_clear_text_selection_mode_get(Evas_Object* o);
// #endif // #if ENABLE(TIZEN_TEXT_SELECTION)

// #if ENABLE(TIZEN_SQL_DATABASE)
typedef Eina_Bool (*Ewk_View_Exceeded_Database_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin, const char* database_name, unsigned long long expectedQuota, void* user_data);
EAPI void ewk_view_exceeded_database_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Database_Quota_Callback callback, void* user_data);
EAPI void ewk_view_exceeded_database_quota_reply(Evas_Object* o, Eina_Bool allow);
// #endif // ENABLE(TIZEN_SQL_DATABASE)

// #if ENABLE(TIZEN_INDEXED_DATABASE)
typedef Eina_Bool (*Ewk_View_Exceeded_Indexed_Database_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  long long currentQuota, void* user_data);
EAPI void ewk_view_exceeded_indexed_database_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* user_data);
EAPI void ewk_view_exceeded_indexed_database_quota_reply(Evas_Object* o, Eina_Bool allow);
// #endif // ENABLE(TIZEN_INDEXED_DATABASE)

// #if PLATFORM(TIZEN)
typedef Eina_Bool (*Ewk_View_JavaScript_Alert_Callback)(Evas_Object* o, const char* alert_text, void* user_data);
EAPI void ewk_view_javascript_alert_callback_set(Evas_Object* o, Ewk_View_JavaScript_Alert_Callback callback, void* user_data);
EAPI void ewk_view_javascript_alert_reply(Evas_Object* o);

typedef Eina_Bool (*Ewk_View_JavaScript_Confirm_Callback)(Evas_Object* o, const char* message, void* user_data);
EAPI void ewk_view_javascript_confirm_callback_set(Evas_Object* o, Ewk_View_JavaScript_Confirm_Callback callback, void* user_data);
EAPI void ewk_view_javascript_confirm_reply(Evas_Object* o, Eina_Bool result);

typedef Eina_Bool (*Ewk_View_JavaScript_Prompt_Callback)(Evas_Object* o, const char* message, const char* default_value, void* user_data);
EAPI void ewk_view_javascript_prompt_callback_set(Evas_Object* o, Ewk_View_JavaScript_Prompt_Callback callback, void* user_data);
EAPI void ewk_view_javascript_prompt_reply(Evas_Object* o, const char* result);

typedef int Ewk_Frame_Ref;
EAPI Eina_Bool ewk_frame_is_main_frame(Ewk_Frame_Ref frame);

EAPI Evas_Object* ewk_view_add_with_session_data(Evas *e, const char *data, unsigned length);

// #if ENABLE(TIZEN_USER_AGENT)
/**
* Request to set the user agent with application name.
*
* @param o view object to set the user agent with application name
*
* @param application_name string to set the user agent
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
*/
EAPI Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* o, const char* application_name);

/**
* Returns application name string.
*
* @param o view object to get the application name
*
* @return @c application name
*/
EAPI const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* o);
// #endif // #if ENABLE(TIZEN_USER_AGENT)

enum Ewk_Unfocus_Direction {
    EWK_UNFOCUS_DIRECTION_NONE = 0,
    EWK_UNFOCUS_DIRECTION_FORWARD,
    EWK_UNFOCUS_DIRECTION_BACKWARD,
    EWK_UNFOCUS_DIRECTION_UP,
    EWK_UNFOCUS_DIRECTION_DOWN,
    EWK_UNFOCUS_DIRECTION_LEFT,
    EWK_UNFOCUS_DIRECTION_RIGHT,
};
typedef enum Ewk_Unfocus_Direction Ewk_Unfocus_Direction;
typedef Eina_Bool (*Ewk_View_Unfocus_Allow_Callback)(Evas_Object* o, Ewk_Unfocus_Direction direction, void* user_data);

EAPI void ewk_view_unfocus_allow_callback_set(Evas_Object* ewkView, Ewk_View_Unfocus_Allow_Callback callback, void* user_data);

// #if ENABLE(TIZEN_TV_PROFILE)
EAPI void ewk_view_pip_rect_set(Evas_Object* webview, int x, int y, int w, int h);
EAPI void ewk_view_pip_enable_set(Evas_Object* webview, Eina_Bool enable);
EAPI void ewk_view_draw_focus_ring_enable_set(Evas_Object* webview, Eina_Bool enable);
// #endif // #if ENABLE(TIZEN_TV_PROFILE)

/// Defines the page visibility status.
enum _Ewk_Page_Visibility_State {
    EWK_PAGE_VISIBILITY_STATE_VISIBLE,
    EWK_PAGE_VISIBILITY_STATE_HIDDEN,
    EWK_PAGE_VISIBILITY_STATE_PRERENDER,
    EWK_PAGE_VISIBILITY_STATE_UNLOADED
};
/// Creates a type name for @a _Ewk_Page_Visibility_State.
typedef enum _Ewk_Page_Visibility_State Ewk_Page_Visibility_State;

/**
 * Sets the visibility state of the page.
 *
 * Tizen specific API.
 *
 * This function let WebKit knows the visibility status of the page.
 * WebKit will save the current status, and fire a "visibilitychange"
 * event which web application can listen. Web application could slow
 * down or stop itself when it gets a "visibilitychange" event and its
 * visibility state is hidden. If its visibility state is visible, then
 * the web application could use more resources.
 *
 * This feature makes that web application could use the resources efficiently,
 * such as power, CPU, and etc.
 *
 * If more detailed description is needed, please see the specification.
 * (http://www.w3.org/TR/page-visibility)
 *
 * @param o view object to set the visibility state.
 * @param page_visible_state the visible state of the page to set.
 * @param initial_state @c EINA_TRUE if this function is called at page initialization time,
 *                      @c EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure.
 */
EAPI void ewk_view_page_visibility_set(Evas_Object *o, Ewk_Page_Visibility_State state, Eina_Bool initial_state);

/**
 * Sets the state to apply Blur Effect for widgets
 *
 * Tizen specific API.
 *
 * This function let WebKit knows that the widget is Gear 2 widget.
 * WebKit will query  this information to decide to apply Blur effect or not.
 * @param o view object to set the visibility state.
 * @param state @c EINA_TRUE if Gear 2 widget
 *              @c EINA_FALSE otherwise.
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure.
 */
EAPI void ewk_view_mirrored_blur_set(Evas_Object *o,Eina_Bool state);

#ifdef __cplusplus
}
#endif
#endif // ewk_view_h
