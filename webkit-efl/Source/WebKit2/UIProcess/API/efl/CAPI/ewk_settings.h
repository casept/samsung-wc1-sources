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

/**
 * @addtogroup WEBVIEW
 * @{
 */

typedef struct Ewk_Settings Ewk_Settings;
#ifndef ewk_settings_type
#define ewk_settings_type
/**
 * @brief Creates a type name for Ewk_Settings
 */
typedef struct Ewk_Settings Ewk_Settings;
#endif

/**
 * @brief Requests setting of auto fit.
 *
 * @param[in] settings settings object to fit to width
 * @param[in] enable to fit to width
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_auto_fitting_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * @brief Returns the auto fit status.
 *
 * @param[in] settings settings object to fit to width
 *
 * @return @c EINA_TRUE if enable auto fit or @c EINA_FALSE.
 */
EAPI Eina_Bool ewk_settings_auto_fitting_get(const Ewk_Settings *settings);

/**
 * @brief Enables/disables the javascript executing.
 *
 * @param[in] settings settings object to set javascript executing
 * @param[in] enable @c EINA_TRUE to enable javascript executing\n
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * @brief Returns the javascript can be executable or not.
 *
 * @param[in] settings settings object to query if the javascript can be executed
 *
 * @return @c EINA_TRUE if the javascript can be executed\n
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings *settings);

/**
 * @brief Enables/disables auto loading of the images.
 *
 * @param[in] settings settings object to set auto loading of the images
 * @param[in] automatic @c EINA_TRUE to enable auto loading of the images,\n
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings *settings, Eina_Bool automatic);

/**
 * @brief Returns the images can be loaded automatically or not.
 *
 * @param[in] settings settings object to get auto loading of the images
 *
 * @return @c EINA_TRUE if the images are loaded automatically,\n
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings *settings);

/**
 * @brief Sets the default text encoding name.
 *
 * @param[in] settings settings object to set default text encoding name
 * @param[in] encoding default text encoding name
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_default_text_encoding_name_set(Ewk_Settings *settings, const char *encoding);

/**
 * @brief Gets the default text encoding name.
 *
 * @details The returned string is guaranteed to be stringshared.
 *
 * @param[in] settings settings object to query default text encoding name
 *
 * @return default text encoding name
 */
EAPI const char *ewk_settings_default_text_encoding_name_get(const Ewk_Settings *settings);

/**
 * @brief Sets the default font size.
 *
 * @details By default, the default font size is @c 16.
 *
 * @param[in] settings settings object to set the default font size
 * @param[in] size a new default font size to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings *settings, int size);

/**
 * @brief Returns the default font size.
 *
 * @param[in] settings settings object to get the default font size
 *
 * @return @c the default font size or @c 0 on failure
 */
EAPI int ewk_settings_default_font_size_get(const Ewk_Settings *settings);

/**
 * @brief Requests enables/disables private browsing.
 *
 * @param[in] settings settings object to set private browsing
 * @param[in] enable @c EINA_TRUE to enable private browsing\n
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * @brief Returns enables/disables private browsing.
 *
 * @param[in] settings settings object to query if private browsing was enabled
 *
 * @return @c EINA_TRUE if private browsing was enabled\n
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings *settings);

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
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_settings_h
