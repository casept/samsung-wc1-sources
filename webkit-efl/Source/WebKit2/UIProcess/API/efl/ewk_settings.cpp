/*
 * Copyright (C) 2012 Samsung Electronics
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

#include "config.h"
#include "ewk_settings.h"

#include "EwkView.h"
#include "ewk_settings_private.h"
#include <WebKit2/WebPageGroup.h>
#include <WebKit2/WebPageProxy.h>
#include <WebKit2/WebPreferences.h>

#if ENABLE(TIZEN_ENCODING_VERIFICATION)
#include "TextEncoding.h"
#endif

using namespace WebKit;

const WebKit::WebPreferences* EwkSettings::preferences() const
{
    return m_view->page()->pageGroup()->preferences();
}

WebKit::WebPreferences* EwkSettings::preferences()
{
    return m_view->page()->pageGroup()->preferences();
}

void EwkSettings::setDefaultTextEncodingName(const char* name)
{
    if (m_defaultTextEncodingName == name)
        return;

    preferences()->setDefaultTextEncodingName(String::fromUTF8(name));
    m_defaultTextEncodingName = name;
}

const char* EwkSettings::defaultTextEncodingName() const
{
    return m_defaultTextEncodingName;
}

Eina_Bool ewk_settings_fullscreen_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(FULLSCREEN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setFullScreenEnabled(enable);
    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_fullscreen_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(FULLSCREEN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->preferences()->fullScreenEnabled();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setJavaScriptEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptEnabled();
}

Eina_Bool ewk_settings_dom_paste_allowed_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_PASTEBOARD)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDOMPasteAllowed(enable);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_dom_paste_allowed_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_PASTEBOARD)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->domPasteAllowed();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

Eina_Bool ewk_settings_javascript_can_access_clipboard_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_PASTEBOARD)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setJavaScriptCanAccessClipboard(enable);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_javascript_can_access_clipboard_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_PASTEBOARD)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptCanAccessClipboard();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings* settings, Eina_Bool automatic)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLoadsImagesAutomatically(automatic);

    return true;
}

Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->loadsImagesAutomatically();
}

Eina_Bool ewk_settings_developer_extras_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDeveloperExtrasEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_developer_extras_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->developerExtrasEnabled();
}

Eina_Bool ewk_settings_file_access_from_file_urls_allowed_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setAllowFileAccessFromFileURLs(enable);

    return true;
}

Eina_Bool ewk_settings_file_access_from_file_urls_allowed_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->allowFileAccessFromFileURLs();
}

Eina_Bool ewk_settings_frame_flattening_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setFrameFlatteningEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_frame_flattening_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->frameFlatteningEnabled();
}

Eina_Bool ewk_settings_dns_prefetching_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDNSPrefetchingEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_dns_prefetching_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->dnsPrefetchingEnabled();
}

Eina_Bool ewk_settings_encoding_detector_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setUsesEncodingDetector(enable);

    return true;
}

Eina_Bool ewk_settings_encoding_detector_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->usesEncodingDetector();
}

Eina_Bool ewk_settings_default_text_encoding_name_set(Ewk_Settings* settings, const char* encoding)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->setDefaultTextEncodingName(encoding);
    return EINA_TRUE;
}

const char* ewk_settings_default_text_encoding_name_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);

    return settings->defaultTextEncodingName();
}

Eina_Bool ewk_settings_preferred_minimum_contents_width_set(Ewk_Settings *settings, unsigned width)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLayoutFallbackWidth(width);

    return true;
}

unsigned ewk_settings_preferred_minimum_contents_width_get(const Ewk_Settings *settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->layoutFallbackWidth();
}

Eina_Bool ewk_settings_offline_web_application_cache_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setOfflineWebApplicationCacheEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_offline_web_application_cache_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->offlineWebApplicationCacheEnabled();
}

Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setJavaScriptCanOpenWindowsAutomatically(enable);

    return true;
}

Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptCanOpenWindowsAutomatically();
}

Eina_Bool ewk_settings_link_effect_enabled_set(Ewk_Settings* settings, Eina_Bool linkEffectEnabled)
{
#if ENABLE(TIZEN_UI_TAP_SOUND)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLinkEffectEnabled(linkEffectEnabled);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_link_effect_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_UI_TAP_SOUND)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->linkEffectEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_local_storage_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLocalStorageEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_local_storage_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->localStorageEnabled();
}

Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setPluginsEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->pluginsEnabled();
}

Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings* settings, int size)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDefaultFontSize(size);

    return true;
}

int ewk_settings_default_font_size_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);

    return settings->preferences()->defaultFontSize();
}

Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setPrivateBrowsingEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->privateBrowsingEnabled();
}

Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setTextAutosizingEnabled(enable);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->textAutosizingEnabled();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

/*** TIZEN EXTENSIONS ****/
Eina_Bool ewk_settings_auto_fitting_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_auto_fitting_get(const Ewk_Settings*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_autofill_password_form_enabled_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_autofill_password_form_enabled_get(Ewk_Settings*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_form_candidate_data_enabled_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_form_candidate_data_enabled_get(Ewk_Settings*)
{
    // FIXME: Need to implement
    return false;
}

void ewk_settings_link_magnifier_enabled_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
}

Eina_Bool ewk_settings_link_magnifier_enabled_get(const Ewk_Settings*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_scan_malware_enabled_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_scan_malware_enabled_get(const Ewk_Settings*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_compositing_borders_visible_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_text_zoom_enabled_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_text_zoom_enabled_get(const Ewk_Settings*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_settings_text_autosizing_font_scale_factor_set(Ewk_Settings*, double)
{
    // FIXME: Need to implement
    return false;
}

double ewk_settings_text_autosizing_font_scale_factor_get(const Ewk_Settings*)
{
    // FIXME: Need to implement
    return 1;
}

Eina_Bool ewk_settings_form_profile_data_enabled_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
    return false;
}

void ewk_settings_detect_email_address_automatically_set(Ewk_Settings*, Eina_Bool)
{
    // FIXME: Need to implement
}

Eina_Bool ewk_settings_uses_keypad_without_user_action_set(Ewk_Settings* settings, Eina_Bool use)
{
#if ENABLE(TIZEN_CHOOSE_TO_USE_KEYPAD_OR_NOT_WITHOUT_USER_ACTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setUsesKeypadWithoutUserAction(use);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(use);
    return false;
#endif
}

Eina_Bool ewk_settings_uses_keypad_without_user_action_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_CHOOSE_TO_USE_KEYPAD_OR_NOT_WITHOUT_USER_ACTION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, true);

    return settings->preferences()->usesKeypadWithoutUserAction();
#else
    UNUSED_PARAM(settings);
    return true;
#endif
}

Eina_Bool ewk_settings_is_encoding_valid(const char* encoding)
{
#if ENABLE(TIZEN_ENCODING_VERIFICATION)
    return WebCore::TextEncoding(encoding).isValid();
#else
    UNUSED_PARAM(encoding);
    return false;
#endif
}

Eina_Bool ewk_settings_default_encoding_set(Ewk_Settings* settings, const char* encoding)
{
    return ewk_settings_default_text_encoding_name_set(settings, encoding);
}

const char* ewk_settings_default_encoding_get(const Ewk_Settings* settings)
{
    return ewk_settings_default_text_encoding_name_get(settings);
}

Eina_Bool ewk_settings_style_scoped_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_STYLE_SCOPED)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setStyleScopedEnabled(enable);
    return true;
#endif
}

Eina_Bool ewk_settings_style_scoped_get(Ewk_Settings* settings)
{
#if ENABLE(TIZEN_STYLE_SCOPED)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->styleScopedEnabled();
#endif
}

Eina_Bool ewk_settings_load_remote_images_set(Ewk_Settings* settings, Eina_Bool loadRemoteImages)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLoadRemoteImages(loadRemoteImages);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
}

Eina_Bool ewk_settings_load_remote_images_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->loadRemoteImages();
#else
    return false;
#endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
}

Eina_Bool ewk_settings_text_selection_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_TEXT_SELECTION)
    settings->setTextSelectionEnabled(enable);
    return true;
#else
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_text_selection_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_TEXT_SELECTION)
    return settings->textSelectionEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_clear_text_selection_automatically_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_TEXT_SELECTION)
    settings->setAutoClearTextSelection(enable);
    return true;
#else
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_clear_text_selection_automatically_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_TEXT_SELECTION)
    return settings->autoClearTextSelection();
#else
    return false;
#endif
}

void ewk_settings_text_style_state_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN(settings);
    settings->preferences()->setTextStyleStateEnabled(enabled);
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enabled);
#endif
}

Eina_Bool ewk_settings_text_style_state_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    return settings->preferences()->textStyleStateEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_performance_features_enabled_set(Ewk_Settings*, Eina_Bool /* performance_features_enabled */)
{
    return false;
}

Eina_Bool ewk_settings_force_zoom_set(Ewk_Settings*, Eina_Bool)
{
    return false;
}
Eina_Bool ewk_settings_webkit_text_size_adjust_enabled_set(Ewk_Settings*, Eina_Bool)
{
    return false;
}
void ewk_settings_detect_contents_automatically_set(Ewk_Settings*, Eina_Bool)
{
}

#if ENABLE(TIZEN_TV_PROFILE)
void ewk_settings_cache_builder_enabled_set(Ewk_Settings*, Eina_Bool)
{
}
#endif

Eina_Bool ewk_settings_edge_effect_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_EDGE_EFFECT)
    settings->setEdgeEffectEnabled(enable);
    return true;
#else
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_edge_effect_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_EDGE_EFFECT)
    return settings->edgeEffectEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_spatial_navigation_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    settings->setSpatialNavigationEnabled(enable);
    return true;
#else
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_spatial_navigation_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    return settings->spatialNavigationEnabled();
#else
    return false;
#endif
}
