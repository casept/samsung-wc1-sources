/*
 * Copyright (C) 2012 Samsung Electronics
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
 * @file    ewk_settings.h
 * @brief   Describes the settings API.
 *
 * @note The ewk_settings is for setting the preference of specific ewk_view.
 * We can get the ewk_settings from ewk_view using ewk_view_settings_get() API.
 */

#ifndef ewk_settings_h
#define ewk_settings_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for Ewk_Settings */
typedef struct EwkSettings Ewk_Settings;


/**
 * Enables/disables the Javascript Fullscreen API. The Javascript API allows
 * to request full screen mode, for more information see:
 * http://dvcs.w3.org/hg/fullscreen/raw-file/tip/Overview.html
 *
 * Default value for Javascript Fullscreen API setting is @c EINA_TRUE .
 *
 * @param settings settings object to enable Javascript Fullscreen API
 * @param enable @c EINA_TRUE to enable Javascript Fullscreen API or
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_fullscreen_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the Javascript Fullscreen API is enabled or not.
 *
 * @param settings settings object to query whether Javascript Fullscreen API is enabled
 *
 * @return @c EINA_TRUE if the Javascript Fullscreen API is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_fullscreen_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables the javascript executing.
 *
 * By default, JavaScript execution is enabled.
 *
 * @param settings settings object to set javascript executing
 * @param enable @c EINA_TRUE to enable javascript executing
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether JavaScript execution is enabled.
 *
 * @param settings settings object to query if the javascript can be executed
 *
 * @return @c EINA_TRUE if the javascript can be executed
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables the javascript access to clipboard.
 *
 * By default, JavaScript access to clipboard is disabled.
 *
 * @param settings settings object to set javascript access to clipboard
 * @param enable @c EINA_TRUE to enable javascript access to clipboard
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_javascript_can_access_clipboard_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether javascript access to clipboard is enabled.
 *
 * @param settings settings object to query if the javascript can access to clipboard
 *
 * @return @c EINA_TRUE if the javascript can access to clipboard
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_javascript_can_access_clipboard_get(const Ewk_Settings *settings);

/**
 * Enables/disables the DOM Paste is allowed.
 *
 * By default, DOM Paste is disabled.
 *
 * @param settings settings object to set DOM Paste allowance
 * @param enable @c EINA_TRUE to enable DOM Paste
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_dom_paste_allowed_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether DOM Paste is allowed.
 *
 * @param settings settings object to query if the DOM Paste is enabled.
 *
 * @return @c EINA_TRUE if the DOM Paste is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_dom_paste_allowed_get(const Ewk_Settings *settings);

/**
 * Enables/disables auto loading of the images.
 *
 * By default, auto loading of the images is enabled.
 *
 * @param settings settings object to set auto loading of the images
 * @param automatic @c EINA_TRUE to enable auto loading of the images
 *                  @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings *settings, Eina_Bool automatic);

/**
 * Returns whether the images can be loaded automatically or not.
 *
 * @param settings settings object to get auto loading of the images
 *
 * @return @c EINA_TRUE if the images are loaded automatically
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings *settings);

/**
 * Enables/disables developer extensions.
 *
 * By default, the developer extensions are disabled.
 *
 * @param settings settings object to set developer extensions
 * @param enable @c EINA_TRUE to enable developer extensions
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_developer_extras_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Queries if developer extensions are enabled.
 *
 * By default, the developer extensions are disabled.
 *
 * @param settings settings object to set developer extensions
 *
 * @return @c EINA_TRUE if developer extensions are enabled
           @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_developer_extras_enabled_get(const Ewk_Settings *settings);

/**
 * Allow / Disallow file access from file:// URLs.
 *
 * By default, file access from file:// URLs is not allowed.
 *
 * @param settings settings object to set file access permission
 * @param enable @c EINA_TRUE to enable file access permission
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_file_access_from_file_urls_allowed_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Queries if  file access from file:// URLs is allowed.
 *
 * By default, file access from file:// URLs is not allowed.
 *
 * @param settings settings object to query file access permission
 *
 * @return @c EINA_TRUE if file access from file:// URLs is allowed
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_file_access_from_file_urls_allowed_get(const Ewk_Settings *settings);

/**
 * Enables/disables frame flattening.
 *
 * By default, the frame flattening is disabled.
 *
 * @param settings settings object to set the frame flattening
 * @param enable @c EINA_TRUE to enable the frame flattening
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_enable_frame_flattening_get()
 */
EAPI Eina_Bool ewk_settings_frame_flattening_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the frame flattening is enabled.
 *
 * The frame flattening is a feature which expands sub frames until all the frames become
 * one scrollable page.
 *
 * @param settings settings object to get the frame flattening.
 *
 * @return @c EINA_TRUE if the frame flattening is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_frame_flattening_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables DNS prefetching.
 *
 * By default, DNS prefetching is disabled.
 *
 * @param settings settings object to set DNS prefetching
 * @param enable @c EINA_TRUE to enable DNS prefetching or
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_DNS_prefetching_enabled_get()
 */
EAPI Eina_Bool ewk_settings_dns_prefetching_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Requests to enable/disable link effect
 *
 * @param settings settings object to enable/disable link effect
 *
 * @param linkEffectEnabled @c EINA_TRUE to enable the link effect
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_link_effect_enabled_set(Ewk_Settings *settings, Eina_Bool linkEffectEnabled);

/**
 * Returns enable/disable link effect
 *
 * @param settings settings object to get whether link effect is enabled or disabled
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EAPI Eina_Bool ewk_settings_link_effect_enabled_get(const Ewk_Settings *settings);

/**
 * Returns whether DNS prefetching is enabled or not.
 *
 * DNS prefetching is an attempt to resolve domain names before a user tries to follow a link.
 *
 * @param settings settings object to get DNS prefetching
 *
 * @return @c EINA_TRUE if DNS prefetching is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_dns_prefetching_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables the encoding detector.
 *
 * By default, the encoding detector is disabled.
 *
 * @param settings settings object to set the encoding detector
 * @param enable @c EINA_TRUE to enable the encoding detector,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_encoding_detector_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
* Returns whether the encoding detector is enabled or not.
 *
 * @param settings settings object to query whether encoding detector is enabled
 *
 * @return @c EINA_TRUE if the encoding detector is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_encoding_detector_enabled_get(const Ewk_Settings *settings);

/**
 * Sets the default text encoding name.
 *
 * @param settings settings object to set default text encoding name
 * @param encoding default text encoding name
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_default_text_encoding_name_set(Ewk_Settings *settings, const char *encoding);

/**
 * Gets the default text encoding name.
 *
 * The returned string is guaranteed to be stringshared.
 *
 * @param settings settings object to query default text encoding name
 *
 * @return default text encoding name
 */
EAPI const char *ewk_settings_default_text_encoding_name_get(const Ewk_Settings *settings);

/**
 * Sets preferred minimum contents width which is used as default minimum contents width
 * for non viewport meta element sites.
 *
 * By default, preferred minimum contents width is equal to @c 980.
 *
 * @param settings settings object to set the encoding detector
 * @param enable @c EINA_TRUE to enable the encoding detector,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_preferred_minimum_contents_width_set(Ewk_Settings *settings, unsigned width);

/**
 * Returns preferred minimum contents width or @c 0 on failure.
 *
 * @param settings settings object to query preferred minimum contents width
 *
 * @return preferred minimum contents width
 *         @c 0 on failure
 */
EAPI unsigned ewk_settings_preferred_minimum_contents_width_get(const Ewk_Settings *settings);

/**
 * Enables/disables the offline application cache.
 *
 * By default, the offline application cache is enabled.
 *
 * @param settings settings object to set the offline application cache state
 * @param enable @c EINA_TRUE to enable the offline application cache,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_offline_web_application_cache_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the offline application cache is enabled or not.
 *
 * @param settings settings object to query whether offline application cache is enabled
 *
 * @return @c EINA_TRUE if the offline application cache is enabled
 *         @c EINA_FALSE if disabled or on failure
 */
EAPI Eina_Bool ewk_settings_offline_web_application_cache_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables if the scripts can open new windows.
 *
 * By default, the scripts can open new windows.
 *
 * @param settings settings object to set if the scripts can open new windows
 * @param enable @c EINA_TRUE if the scripts can open new windows
 *        @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (scripts are disabled)
 */
EAPI Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the scripts can open new windows.
 *
 * @param settings settings object to query whether the scripts can open new windows
 *
 * @return @c EINA_TRUE if the scripts can open new windows
 *         @c EINA_FALSE if not or on failure (scripts are disabled)
 */
EAPI Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings *settings);

/**
 * Enables/disables the HTML5 local storage functionality.
 *
 * Local storage provides simple synchronous storage access.
 * HTML5 local storage specification is available at
 * http://dev.w3.org/html5/webstorage/.
 *
 * By default, the HTML5 local storage is enabled.
 *
 * @param settings settings object to set the HTML5 local storage state
 * @param enable @c EINA_TRUE to enable HTML5 local storage,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_local_storage_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the HTML5 local storage functionality is enabled or not.
 *
 * Local storage provides simple synchronous storage access.
 * HTML5 local storage specification is available at
 * http://dev.w3.org/html5/webstorage/.
 *
 * By default, the HTML5 local storage is enabled.
 *
 * @param settings settings object to query whether HTML5 local storage is enabled
 *
 * @return @c EINA_TRUE if the HTML5 local storage is enabled
 *         @c EINA_FALSE if disabled or on failure
 */
EAPI Eina_Bool ewk_settings_local_storage_enabled_get(const Ewk_Settings *settings);

/**
 * Toggles plug-ins support.
 *
 * By default, plug-ins support is enabled.
 *
 * @param settings settings object to set plug-ins support
 * @param enable @c EINA_TRUE to enable plug-ins support
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether plug-ins support is enabled or not.
 *
 * @param settings settings object to query whether plug-ins support is enabled
 *
 * @return @c EINA_TRUE if plug-ins support is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings *settings);

/**
 * Sets the default font size.
 *
 * By default, the default font size is @c 16.
 *
 * @param settings settings object to set the default font size
 * @param size a new default font size to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings *settings, int size);

/**
 * Returns the default font size.
 *
 * @param settings settings object to get the default font size
 *
 * @return @c the default font size or @c 0 on failure
 */
EAPI int ewk_settings_default_font_size_get(const Ewk_Settings *settings);

/**
 * Enables/disables private browsing.
 *
 * By default, private browsing is disabled.
 *
 * @param settings settings object to set private browsing
 * @param enable @c EINA_TRUE to enable private browsing
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether private browsing is enabled or not.
 *
 * Private Browsing allows a user to browse the Internet without saving any information
 * about which sites and pages a user has visited.
 *
 * @param settings settings object to query whether private browsing is enabled
 *
 * @return @c EINA_TRUE if private browsing is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings *settings);

/**
 * Enables/disables text autosizing.
 *
 * By default, the text autosizing is disabled.
 *
 * @param settings settings object to set the text autosizing
 * @param enable @c EINA_TRUE to enable the text autosizing
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_text_autosizing_enabled_get()
 */
EAPI Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns whether the text autosizing is enabled.
 *
 * The text autosizing is a feature which adjusts the font size of text in wide
 * columns, and makes text more legible.
 *
 * @param settings settings object to query whether text autosizing is enabled
 *
 * @return @c EINA_TRUE if the text autosizing is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings *settings);

/*** TIZEN EXTENSIONS ****/
EAPI Eina_Bool ewk_settings_auto_fitting_set(Ewk_Settings *settings, Eina_Bool enable);

EAPI Eina_Bool ewk_settings_auto_fitting_get(const Ewk_Settings *settings);

EAPI Eina_Bool ewk_settings_autofill_password_form_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

EAPI Eina_Bool ewk_settings_autofill_password_form_enabled_get(Ewk_Settings *settings);

EAPI Eina_Bool ewk_settings_form_candidate_data_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

EAPI Eina_Bool ewk_settings_form_candidate_data_enabled_get(Ewk_Settings *settings);

EAPI void ewk_settings_link_magnifier_enabled_set(Ewk_Settings *settings, Eina_Bool enabled);

EAPI Eina_Bool ewk_settings_link_magnifier_enabled_get(const Ewk_Settings *settings);

EAPI Eina_Bool ewk_settings_scan_malware_enabled_set(Ewk_Settings *settings, Eina_Bool scanMalwareEnabled);

EAPI Eina_Bool ewk_settings_scan_malware_enabled_get(const Ewk_Settings *settings);

EAPI Eina_Bool ewk_settings_compositing_borders_visible_set(Ewk_Settings *settings, Eina_Bool enable);

EAPI Eina_Bool ewk_settings_text_zoom_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

EAPI Eina_Bool ewk_settings_text_zoom_enabled_get(const Ewk_Settings *settings);

EAPI Eina_Bool ewk_settings_text_autosizing_font_scale_factor_set(Ewk_Settings *settings, double factor);

EAPI double ewk_settings_text_autosizing_font_scale_factor_get(const Ewk_Settings *settings);

EAPI Eina_Bool ewk_settings_form_profile_data_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

EAPI void ewk_settings_detect_email_address_automatically_set(Ewk_Settings* settings, Eina_Bool enable);

//#if ENABLE(TIZEN_CHOOSE_TO_USE_KEYPAD_OR_NOT_WITHOUT_USER_ACTION)
/**
 * Requests to set using keypad without user action (default value : true)
 *
 * @param settings settings object using keypad without user action
 * @param enable @c EINA_TRUE to use without user action @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_uses_keypad_without_user_action_set(Ewk_Settings *settings, Eina_Bool use);

/**
 * Returns using keypad without user action
 *
 * @param settings settings object using keypad without user action
 * @param settings settings object to query using keypad without user action
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EAPI Eina_Bool ewk_settings_uses_keypad_without_user_action_get(const Ewk_Settings *settings);
//#endif

// #if ENABLE(TIZEN_ENCODING_VERIFICATION)
/**
 * Checks whether WebKit supports the @a encoding.
 *
 * @param encoding the encoding string to check whether WebKit supports it
 *
 * @return @c EINA_TRUE if WebKit supports @a encoding or @c EINA_FALSE if not or
 *      on failure
 */
EAPI Eina_Bool ewk_settings_is_encoding_valid(const char *encoding);
// #endif ENABLE(TIZEN_ENCODING_VERIFICATION)

// #if PLATFORM(TIZEN)
/**
 * Requests to set default text encoding name.
 *
 * @param settings settings object to set default text encoding name
 * @param encoding default text encoding name
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EINA_DEPRECATED EAPI Eina_Bool ewk_settings_default_encoding_set(Ewk_Settings *settings, const char *encoding);

/**
 * Returns default text encoding name.
 *
 * @param settings settings object to query default text encoding nae
 *
 * @return default text encoding name
 */
EINA_DEPRECATED EAPI const char *ewk_settings_default_encoding_get(const Ewk_Settings *settings);
// #endif PLATFORM(TIZEN)

// #if ENABLE(TIZEN_STYLE_SCOPED)
/**
 * Requests to set style scoped option (default value : true)
 *
 * @param settings settings object to enable style coped
 * @param enable @c EINA_TRUE to enable style scoped @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_style_scoped_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * Returns enable/disable style scoped
 *
 * @param settings settings object to style scoped
 *
 * @return @c EINA_TRUE if enable style scoped of @c EINA_FALSE.
 */
EAPI Eina_Bool ewk_settings_style_scoped_enabled_get(const Ewk_Settings *settings);
// #endif // ENABLE(TIZEN_STYLE_SCOPED)

// #if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
/**
 * Requests to set the load remote images enable/disable
 *
 * @param settings settings object to set load remote images
 *
 * @param loadRemoteImages @c EINA_TRUE to enable the load remote images
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_load_remote_images_set(Ewk_Settings *settings, Eina_Bool loadRemoteImages);
/**
 * Returns enable/disable the load remote images
 *
 * @param settings settings object to get editable link behavior
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EAPI Eina_Bool ewk_settings_load_remote_images_get(const Ewk_Settings *settings);
// #endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)

//#if ENABLE(TIZEN_TEXT_SELECTION)
/**
 * Requests enable/disable text selection by default WebKit.
 *
 * @param settings setting object to set text selection by default WebKit
 * @param enable @c EINA_TRUE to enable text selection by default WebKit
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_text_selection_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns if text selection by default WebKit is enabled or disabled.
 *
 * @param settings setting object to get text selection by default WebKit
 *
 * @return @c EINA_TRUE if text selection by default WebKit is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_text_selection_enabled_get(const Ewk_Settings* settings);

/**
 * Requests enables/disables to clear text selection when webview lose focus
 *
 * @param settings setting object to set to clear text selection when webview lose focus
 * @param enable @c EINA_TRUE to clear text selection when webview lose focus
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_clear_text_selection_automatically_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns whether text selection is cleared when webview lose focus or not.
 *
 * @param settings setting object to get whether text selection is cleared when webview lose focus or not
 *
 * @return @c EINA_TRUE if text selection is cleared when webview lose focus
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_clear_text_selection_automatically_get(const Ewk_Settings* settings);
//#endif // ENABLE(TIZEN_TEXT_SELECTION)

// #if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
/**
 * Sets text style for selection mode enabled.
 *
 * @param settings settings object
 * @param enabled text style for selection mode
 */

EAPI void ewk_settings_text_style_state_enabled_set(Ewk_Settings *settings, Eina_Bool enabled);
/**
 * Gets text style for selection mode enabled.
 *
 * @param settings settings object
 *
 * @return @c EINA_TRUE if text style for selection mode enabled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_settings_text_style_state_enabled_get(const Ewk_Settings *settings);
// #endif // ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)

EAPI Eina_Bool ewk_settings_performance_features_enabled_set(Ewk_Settings *settings, Eina_Bool performance_features_enabled);

EAPI Eina_Bool ewk_settings_force_zoom_set(Ewk_Settings *settings, Eina_Bool enable);
EAPI Eina_Bool ewk_settings_webkit_text_size_adjust_enabled_set(Ewk_Settings* settings, Eina_Bool enabled);
EAPI void ewk_settings_detect_contents_automatically_set(Ewk_Settings* settings, Eina_Bool enable);

// #if ENABLE(TIZEN_TV_PROFILE)
EAPI void ewk_settings_cache_builder_enabled_set(Ewk_Settings* settings, Eina_Bool enable);
// #endif // ENABLE(TIZEN_TV_PROFILE)

//#if ENABLE(TIZEN_EDGE_EFFECT)
/**
 * Requests to enable/disable edge effect
 *
 * @param settings settings object to enable/disable edge effect
 *
 * @param enable @c EINA_TRUE to enable the edge effect
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_edge_effect_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns enable/disable edge effect
 *
 * @param settings settings object to get whether edge effect is enabled or disabled
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EAPI Eina_Bool ewk_settings_edge_effect_enabled_get(const Ewk_Settings* settings);
//#endif

//#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
/**
 * Requests to enable/disable spatial navigation
 *
 * @param settings settings object to enable/disable spatial navigation
 *
 * @param enable @c EINA_TRUE to enable the spatial navigation
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_spatial_navigation_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns enable/disable spatial navigation
 *
 * @param settings settings object to get whether spatial navigation is enabled or disabled
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EAPI Eina_Bool ewk_settings_spatial_navigation_enabled_get(const Ewk_Settings* settings);
//#endif

#ifdef __cplusplus
}
#endif
#endif // ewk_settings_h
