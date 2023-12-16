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

#include "config.h"
#include "ewk_view.h"
#include "ewk_view_private.h"

#include "AffineTransform.h"
#include "EwkView.h"
#include "FindClientEfl.h"
#include "FormClientEfl.h"
#include "InputMethodContextEfl.h"
#include "PageLoadClientEfl.h"
#include "PagePolicyClientEfl.h"
#include "PageUIClientEfl.h"
#include "PageViewportController.h"
#include "PageViewportControllerClientEfl.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_context.h"
#include "ewk_context_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_page_group.h"
#include "ewk_page_group_private.h"
#include "ewk_private.h"
#include "ewk_settings_private.h"
#include <Ecore_Evas.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <WebKit2/WKAPICast.h>
#include <WebKit2/WKBackForwardList.h>
#include <WebKit2/WKData.h>
#include <WebKit2/WKEinaSharedString.h>
#include <WebKit2/WKFindOptions.h>
#include <WebKit2/WKInspector.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKSerializedScriptValue.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKView.h>
#include <WebKit2/WKViewEfl.h>
#include <wtf/text/CString.h>

#if ENABLE(INSPECTOR)
#include "WebInspectorProxy.h"
#if ENABLE(TIZEN_REMOTE_INSPECTOR)
#include "WebInspectorServer.h"
#include "WebSocketServer.h"
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#include "ewk_hit_test_private.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
#include "ewk_text_style.h"
#endif

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif

#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
#include <WebKit2/WKPageEfl.h>
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
#include "WKDictionary.h"
#include "ewk_web_application_icon_data_private.h"
#include "ewk_web_application_icon_data.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
#include "ewk_error.h"
#include "ewk_error_private.h"
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "WKArray.h"
#include "WKNotificationManager.h"
#include "WKNumber.h"
#include "ewk_notification_private.h"
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "WKUserMediaPermissionRequest.h"
#include "ewk_user_media_private.h"
#endif

#if ENABLE(TIZEN_SQL_DATABASE) || ENABLE(TIZEN_SCREEN_CAPTURE)
#include "WKPageTizen.h"
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
#include "ewk_console_message.h"
#include "ewk_console_message_private.h"
#endif

#if ENABLE(TIZEN_SCREEN_CAPTURE)
#include <WebKit2/WKImageCairo.h>
#endif

#if PLATFORM(TIZEN)
#include <WebCore/IntPoint.h>

using namespace WebCore;
#endif

using namespace WebKit;

#define EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, ...)                        \
    EwkView* impl = toEwkViewChecked(ewkView);                                 \
    do {                                                                       \
        if (EINA_UNLIKELY(!impl)) {                                            \
            EINA_LOG_CRIT("no private data for object %p", ewkView);           \
            return __VA_ARGS__;                                                \
        }                                                                      \
    } while (0)


static inline EwkView* toEwkViewChecked(const Evas_Object* evasObject)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(evasObject, 0);
    if (EINA_UNLIKELY(!isEwkViewEvasObject(evasObject)))
        return 0;

    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject));
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData, 0);

    return smartData->priv;
}

Eina_Bool ewk_view_smart_class_set(Ewk_View_Smart_Class* api)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(api, false);

    return EwkView::initSmartClassInterface(*api);
}

Evas_Object* EWKViewCreate(WKContextRef context, WKPageGroupRef pageGroup, Evas* canvas, Evas_Smart* smart)
{
    WKRetainPtr<WKViewRef> wkView = adoptWK(WKViewCreate(context, pageGroup));
#if ENABLE(TIZEN_MOBILE_PROFILE) || ENABLE(TIZEN_WEARABLE_PROFILE) || ENABLE(TIZEN_TV_PROFILE)
    // Enable fixed layout as a default for MOBILE and MICRO profile.
    WKPageSetUseFixedLayout(WKViewGetPage(wkView.get()), true);
#endif
    if (EwkView* ewkView = EwkView::create(wkView.get(), canvas, smart))
        return ewkView->evasObject();

    return 0;
}

WKViewRef EWKViewGetWKView(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->wkView();
}

Evas_Object* ewk_view_smart_add(Evas* canvas, Evas_Smart* smart, Ewk_Context* context, Ewk_Page_Group* pageGroup)
{
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);
    EwkPageGroup* ewkPageGroup = ewk_object_cast<EwkPageGroup*>(pageGroup);

    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext->wkContext(), 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkPageGroup, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkPageGroup->wkPageGroup(), 0);

    return EWKViewCreate(ewkContext->wkContext(), ewkPageGroup->wkPageGroup(), canvas, smart);
}

Evas_Object* ewk_view_add(Evas* canvas)
{
    return ewk_view_add_with_context(canvas, ewk_context_default_get());
}

Evas_Object* ewk_view_add_with_context(Evas* canvas, Ewk_Context* context)
{
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext->wkContext(), 0);

    return EWKViewCreate(ewkContext->wkContext(), 0, canvas, 0);
}

Ewk_Context* ewk_view_context_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->ewkContext();
}

Ewk_Page_Group* ewk_view_page_group_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->ewkPageGroup();
}

Eina_Bool ewk_view_url_set(Evas_Object* ewkView, const char* url)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

    WKRetainPtr<WKURLRef> wkUrl = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(impl->wkPage(), wkUrl.get());
    impl->informURLChange();

    return true;
}

const char* ewk_view_url_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->url();
}

Evas_Object* ewk_view_favicon_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->createFavicon();
}

Eina_Bool ewk_view_reload(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageReload(impl->wkPage());
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_reload_bypass_cache(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageReloadFromOrigin(impl->wkPage());
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_stop(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageStopLoading(impl->wkPage());

    return true;
}

Ewk_Settings* ewk_view_settings_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->settings();
}

const char* ewk_view_title_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->title();
}

double ewk_view_load_progress_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);

    return WKPageGetEstimatedProgress(impl->wkPage());
}

Eina_Bool ewk_view_scale_set(Evas_Object* ewkView, double scaleFactor, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

#if ENABLE(TIZEN_SCALE_SET)
    // FIXME: When we use WKPageSetScaleFactor, values of x, y doesn't applied if fixed layout is used.
    if (WKPageUseFixedLayout(WKViewGetPage(impl->wkView()))) {
        impl->scaleTo(scaleFactor, WebCore::FloatPoint(x, y));
        return true;
    }
#endif
    WKPageSetScaleFactor(impl->wkPage(), scaleFactor, WKPointMake(x, y));
    return true;
}

double ewk_view_scale_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1);

    return WKPageGetScaleFactor(impl->wkPage());
}

Eina_Bool ewk_view_page_zoom_set(Evas_Object* ewkView, double zoomFactor)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetPageZoomFactor(impl->wkPage(), zoomFactor);

    return true;
}

double ewk_view_page_zoom_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1);

    return WKPageGetPageZoomFactor(impl->wkPage());
}

Eina_Bool ewk_view_device_pixel_ratio_set(Evas_Object* ewkView, float ratio)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setDeviceScaleFactor(ratio);

    return true;
}

float ewk_view_device_pixel_ratio_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);

    return WKPageGetBackingScaleFactor(impl->wkPage());
}

void ewk_view_theme_set(Evas_Object* ewkView, const char* path)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->setThemePath(path);
}

const char* ewk_view_theme_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->themePath();
}

Eina_Bool ewk_view_back(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageRef page = impl->wkPage();
    if (WKPageCanGoBack(page)) {
        WKPageGoBack(page);
        return true;
    }

    return false;
}

Eina_Bool ewk_view_forward(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageRef page = impl->wkPage();
    if (WKPageCanGoForward(page)) {
        WKPageGoForward(page);
        return true;
    }

    return false;
}

Eina_Bool ewk_view_back_possible(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    return WKPageCanGoBack(impl->wkPage());
}

Eina_Bool ewk_view_forward_possible(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    return WKPageCanGoForward(impl->wkPage());
}

Ewk_Back_Forward_List* ewk_view_back_forward_list_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->backForwardList();
}

void ewk_view_back_forward_list_clear(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKBackForwardListClearBackForwardList(WKPageGetBackForwardList(toAPI(impl->page())));
}

Eina_Bool ewk_view_navigate_to(Evas_Object* ewkView, const Ewk_Back_Forward_List_Item* item)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkBackForwardListItem, item, itemImpl, false);

    WKPageGoToBackForwardListItem(impl->wkPage(), itemImpl->wkItem());

    return true;
}

Eina_Bool ewk_view_html_string_load(Evas_Object* ewkView, const char* html, const char* base_url, const char* unreachable_url)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(html, false);

    WKRetainPtr<WKStringRef> wkHTMLString = adoptWK(WKStringCreateWithUTF8CString(html));
    WKRetainPtr<WKURLRef> wkBaseURL = adoptWK(WKURLCreateWithUTF8CString(base_url));

    if (unreachable_url && *unreachable_url) {
        WKRetainPtr<WKURLRef> wkUnreachableURL = adoptWK(WKURLCreateWithUTF8CString(unreachable_url));
        WKPageLoadAlternateHTMLString(impl->wkPage(), wkHTMLString.get(), wkBaseURL.get(), wkUnreachableURL.get());
    } else
        WKPageLoadHTMLString(impl->wkPage(), wkHTMLString.get(), wkBaseURL.get());

    impl->informURLChange();

    return true;
}

const char* ewk_view_custom_encoding_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->customTextEncodingName();
}

Eina_Bool ewk_view_custom_encoding_set(Evas_Object* ewkView, const char* encoding)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setCustomTextEncodingName(encoding);

    return true;
}

const char* ewk_view_user_agent_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->userAgent();
}

Eina_Bool ewk_view_user_agent_set(Evas_Object* ewkView, const char* userAgent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setUserAgent(userAgent);

    return true;
}

// EwkFindOptions should be matched up orders with WkFindOptions.
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_CASE_INSENSITIVE, kWKFindOptionsCaseInsensitive);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_AT_WORD_STARTS, kWKFindOptionsAtWordStarts);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START, kWKFindOptionsTreatMedialCapitalAsWordStart);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_BACKWARDS, kWKFindOptionsBackwards);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_WRAP_AROUND, kWKFindOptionsWrapAround);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_OVERLAY, kWKFindOptionsShowOverlay);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR, kWKFindOptionsShowFindIndicator);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_HIGHLIGHT, kWKFindOptionsShowHighlight);

Eina_Bool ewk_view_text_find(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    WKRetainPtr<WKStringRef> wkText = adoptWK(WKStringCreateWithUTF8CString(text));
    WKPageFindString(impl->wkPage(), wkText.get(), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageHideFindUI(impl->wkPage());

    return true;
}

Eina_Bool ewk_view_text_matches_count(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    WKRetainPtr<WKStringRef> wkText = adoptWK(WKStringCreateWithUTF8CString(text));
    WKPageCountStringMatches(impl->wkPage(), wkText.get(), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setMouseEventsEnabled(!!enabled);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->mouseEventsEnabled();
}

Eina_Bool ewk_view_feed_touch_event(Evas_Object* ewkView, Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers)
{
#if ENABLE(TOUCH_EVENTS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(points, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->feedTouchEvent(type, points, modifiers);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(type);
    UNUSED_PARAM(points);
    UNUSED_PARAM(modifiers);
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setTouchEventsEnabled(!!enabled);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(enabled);
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_get(const Evas_Object* ewkView)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->touchEventsEnabled();
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_inspector_show(Evas_Object* ewkView)
{
#if ENABLE(INSPECTOR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKInspectorRef wkInspector = WKPageGetInspector(impl->wkPage());
    if (wkInspector)
        WKInspectorShow(wkInspector);

    return true;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_inspector_close(Evas_Object* ewkView)
{
#if ENABLE(INSPECTOR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKInspectorRef wkInspector = WKPageGetInspector(impl->wkPage());
    if (wkInspector)
        WKInspectorClose(wkInspector);

    return true;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

// Ewk_Pagination_Mode should be matched up orders with WKPaginationMode.
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_UNPAGINATED, kWKPaginationModeUnpaginated);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_LEFT_TO_RIGHT, kWKPaginationModeLeftToRight);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_RIGHT_TO_LEFT, kWKPaginationModeRightToLeft);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_TOP_TO_BOTTOM, kWKPaginationModeTopToBottom);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_BOTTOM_TO_TOP, kWKPaginationModeBottomToTop);

Eina_Bool ewk_view_pagination_mode_set(Evas_Object* ewkView, Ewk_Pagination_Mode mode)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetPaginationMode(impl->wkPage(), static_cast<WKPaginationMode>(mode));

    return true;
}

Ewk_Pagination_Mode ewk_view_pagination_mode_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EWK_PAGINATION_MODE_INVALID);
    
    return static_cast<Ewk_Pagination_Mode>(WKPageGetPaginationMode(impl->wkPage()));
}

Eina_Bool ewk_view_fullscreen_exit(Evas_Object* ewkView)
{
#if ENABLE(FULLSCREEN_API)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKViewExitFullScreen(impl->wkView());
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

/// Creates a type name for Ewk_Page_Contents_Context.
typedef struct Ewk_Page_Contents_Context Ewk_Page_Contents_Context;

/*
 * @brief Structure containing page contents context used for ewk_view_page_contents_get() API.
 */
struct Ewk_Page_Contents_Context {
    Ewk_Page_Contents_Type type;
    Ewk_Page_Contents_Cb callback;
    void* userData;
};

/**
 * @internal
 * Callback function used for ewk_view_page_contents_get().
 */
static void ewkViewPageContentsAsMHTMLCallback(WKDataRef wkData, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_Page_Contents_Context* contentsContext = static_cast<Ewk_Page_Contents_Context*>(context);
    contentsContext->callback(contentsContext->type, reinterpret_cast<const char*>(WKDataGetBytes(wkData)), contentsContext->userData);

    delete contentsContext;
}

/**
 * @internal
 * Callback function used for ewk_view_page_contents_get().
 */
static void ewkViewPageContentsAsStringCallback(WKStringRef wkString, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_Page_Contents_Context* contentsContext = static_cast<Ewk_Page_Contents_Context*>(context);
    contentsContext->callback(contentsContext->type, WKEinaSharedString(wkString), contentsContext->userData);

    delete contentsContext;
}

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void ewkViewNotifyPopupReplyWaitingState(Evas_Object* ewkView, bool isWaiting)
{
    if (isWaiting)
        evas_object_smart_callback_call(ewkView, "popup,reply,wait,start", 0);
    else
        evas_object_smart_callback_call(ewkView, "popup,reply,wait,finish", 0);
}
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)

bool ewkViewRunJavaScriptAlert(Evas_Object* ewkView, WKStringRef alertText)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    if (!impl->alertContext || !impl->alertContext->javascriptAlertCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->alertContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(alertText);
    OwnArrayPtr<char> alertTextBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(alertText, alertTextBuffer.get(), length);
    return impl->alertContext->javascriptAlertCallback(impl->alertContext->ewkView, alertTextBuffer.get(), impl->alertContext->userData) == EINA_TRUE;
}

bool ewkViewRunJavaScriptConfirm(Evas_Object* ewkView, WKStringRef message)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    if (!impl->confirmContext || !impl->confirmContext->javascriptConfirmCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->confirmContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(message);
    OwnArrayPtr<char> messageBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(message, messageBuffer.get(), length);
    return impl->confirmContext->javascriptConfirmCallback(impl->confirmContext->ewkView, messageBuffer.get(), impl->confirmContext->userData) == EINA_TRUE;
}

bool ewkViewRunJavaScriptPrompt(Evas_Object* ewkView, WKStringRef message, WKStringRef defaultValue)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    if (!impl->promptContext || !impl->promptContext->javascriptPromptCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->promptContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(message);
    OwnArrayPtr<char> messageBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(message, messageBuffer.get(), length);
    length = WKStringGetMaximumUTF8CStringSize(defaultValue);
    OwnArrayPtr<char> defaultValueBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(defaultValue, defaultValueBuffer.get(), length);
    return impl->promptContext->javascriptPromptCallback(impl->promptContext->ewkView, messageBuffer.get(), defaultValueBuffer.get(), impl->promptContext->userData) == EINA_TRUE;
}
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
bool ewkViewRequestCertificateConfirm(Evas_Object* ewkView, Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    if (impl->certificatePolicyDecision)
        ewkCertificatePolicyDecisionDelete(impl->certificatePolicyDecision);
    impl->certificatePolicyDecision = certificatePolicyDecision;

    evas_object_smart_callback_call(ewkView, "request,certificate,confirm", certificatePolicyDecision);

    return true;
}
#endif

Eina_Bool ewk_view_page_contents_get(const Evas_Object* ewkView, Ewk_Page_Contents_Type type, Ewk_Page_Contents_Cb callback, void* user_data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    Ewk_Page_Contents_Context* context = new Ewk_Page_Contents_Context;
    context->type = type;
    context->callback = callback;
    context->userData = user_data;

    switch (context->type) {
    case EWK_PAGE_CONTENTS_TYPE_MHTML:
        WKPageGetContentsAsMHTMLData(impl->wkPage(), false, context, ewkViewPageContentsAsMHTMLCallback);
        break;
    case EWK_PAGE_CONTENTS_TYPE_STRING:
        WKPageGetContentsAsString(impl->wkPage(), context, ewkViewPageContentsAsStringCallback);
        break;
    default:
        delete context;
        ASSERT_NOT_REACHED();
        return false;
    }

    return true;
}

Eina_Bool ewk_view_source_mode_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKViewSetShowsAsSource(impl->wkView(), enabled);

    return true;
}

Eina_Bool ewk_view_source_mode_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKViewGetShowsAsSource(impl->wkView());
}

struct Ewk_View_Script_Execute_Callback_Context {
    Ewk_View_Script_Execute_Callback_Context(Ewk_View_Script_Execute_Cb callback, Evas_Object* ewkView, void* userData)
        : m_callback(callback)
        , m_view(ewkView)
        , m_userData(userData)
    {
    }

    Ewk_View_Script_Execute_Cb m_callback;
    Evas_Object* m_view;
    void* m_userData;
};

static void runJavaScriptCallback(WKSerializedScriptValueRef scriptValue, WKErrorRef, void* context)
{
    ASSERT(context);

    Ewk_View_Script_Execute_Callback_Context* callbackContext = static_cast<Ewk_View_Script_Execute_Callback_Context*>(context);
    ASSERT(callbackContext->m_view);

    if (!callbackContext->m_callback) {
        delete callbackContext;
        return;
    }

    if (scriptValue) {
        EWK_VIEW_IMPL_GET_OR_RETURN(callbackContext->m_view, impl);
        JSGlobalContextRef jsGlobalContext = impl->ewkContext()->jsGlobalContext();

        JSValueRef value = WKSerializedScriptValueDeserialize(scriptValue, jsGlobalContext, 0);
        JSRetainPtr<JSStringRef> jsStringValue(Adopt, JSValueToStringCopy(jsGlobalContext, value, 0));
        size_t length = JSStringGetMaximumUTF8CStringSize(jsStringValue.get());
        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);
        JSStringGetUTF8CString(jsStringValue.get(), buffer.get(), length);
        callbackContext->m_callback(callbackContext->m_view, buffer.get(), callbackContext->m_userData);
    } else
        callbackContext->m_callback(callbackContext->m_view, 0, callbackContext->m_userData);

    delete callbackContext;
}

Eina_Bool ewk_view_script_execute(Evas_Object* ewkView, const char* script, Ewk_View_Script_Execute_Cb callback, void* userData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(script, false);

    Ewk_View_Script_Execute_Callback_Context* context = new Ewk_View_Script_Execute_Callback_Context(callback, ewkView, userData);
    WKRetainPtr<WKStringRef> scriptString(AdoptWK, WKStringCreateWithUTF8CString(script));
    WKPageRunJavaScriptInMainFrame(impl->wkPage(), scriptString.get(), context, runJavaScriptCallback);
    return true;
}

Eina_Bool ewk_view_layout_fixed_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetUseFixedLayout(WKViewGetPage(impl->wkView()), enabled);

    return true;
}

Eina_Bool ewk_view_layout_fixed_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKPageUseFixedLayout(WKViewGetPage(impl->wkView()));
}

void ewk_view_layout_fixed_size_set(const Evas_Object* ewkView, Evas_Coord width, Evas_Coord height)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKPageSetFixedLayoutSize(WKViewGetPage(impl->wkView()), WKSizeMake(width, height));
}

void ewk_view_layout_fixed_size_get(const Evas_Object* ewkView, Evas_Coord* width, Evas_Coord* height)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKSize size = WKPageFixedLayoutSize(WKViewGetPage(impl->wkView()));

    if (width)
        *width = size.width;
    if (height)
        *height = size.height;
}

void ewk_view_bg_color_set(Evas_Object* ewkView, int red, int green, int blue, int alpha)
{
    if (EINA_UNLIKELY(alpha < 0 || alpha > 255)) {
        EINA_LOG_CRIT("Alpha should be between 0 and 255");
        return;
    }

    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->setBackgroundColor(red, green, blue, alpha);
}

void ewk_view_bg_color_get(const Evas_Object* ewkView, int* red, int* green, int* blue, int* alpha)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKViewGetBackgroundColor(impl->wkView(), red, green, blue, alpha);
}

Eina_Bool ewk_view_contents_size_get(const Evas_Object* ewkView, Evas_Coord* width, Evas_Coord* height)
{
    EwkView* impl = toEwkViewChecked(ewkView);
    if (EINA_UNLIKELY(!impl)) {
        EINA_LOG_CRIT("no private data for object %p", ewkView);
        if (width)
            *width = 0;
        if (height)
            *height = 0;

        return false;
    }

    WKSize contentsSize = WKViewGetContentsSize(impl->wkView());
    if (width)
        *width = contentsSize.width;
    if (height)
        *height = contentsSize.height;

    return true;
}

/*** TIZEN EXTENSIONS ****/
char* ewk_view_get_cookies_for_url(Evas_Object*, const char*)
{
    // FIXME: Need to implement
    return 0;
}

void ewk_view_orientation_send(Evas_Object* ewkView, int orientation)
{
#if ENABLE(TIZEN_ORIENTATION_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKPageSendOrientationChangeEvent(impl->wkPage(), orientation);
#endif
}

Eina_Bool ewk_view_contents_set(Evas_Object* /* ewkView */, const char* /* contents */, size_t /* contentsSize */, char* /* mimeType */, char* /* encoding */, char* /* baseUri */)
{
    return false;
}

Eina_Bool ewk_view_html_contents_set(Evas_Object*, const char*, const char*)
{
    return false;
}

unsigned int ewk_view_inspector_server_start(Evas_Object* ewkView, unsigned int port)
{
#if ENABLE(TIZEN_REMOTE_INSPECTOR)
    // FIXME: We should separate setting of developerExtras.
    ewk_settings_developer_extras_enabled_set(ewk_view_settings_get(ewkView), true);

    if (WebInspectorServer::shared().serverState() == WebSocketServer::Listening)
        return WebInspectorServer::shared().port();

    if (!WebInspectorServer::shared().listen(ASCIILiteral("0.0.0.0"), port)) {
        TIZEN_LOGI("[InspectorServer] Couldn't start listening on: port=%d.", port);
        return 0;
    }

    return WebInspectorServer::shared().port();
#else
    return 0;
#endif
}

Eina_Bool ewk_view_inspector_server_stop(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_REMOTE_INSPECTOR)
    WebInspectorServer::shared().close();

    // FIXME: We should separate setting of developerExtras.
    ewk_settings_developer_extras_enabled_set(ewk_view_settings_get(ewkView), false);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_mhtml_data_get(Evas_Object*, Ewk_View_MHTML_Data_Get_Callback, void*)
{
    // FIXME: Need to implement
    return false;
}

Eina_Bool ewk_view_notification_closed(Evas_Object*, Eina_List*)
{
    // FIXME: Need to implement
    return false;
}

void ewk_view_resume(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    TIZEN_LOGI("");
#if ENABLE(TIZEN_JAVASCRIPT_AND_RESOURCE_SUSPEND_RESUME)
    impl->page()->resumeJavaScriptAndResource();
#endif

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
    impl->page()->resumePlugin();
#endif // ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)

#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
    impl->page()->resumePainting();
#endif
}

Eina_Bool ewk_view_url_request_set(Evas_Object* ewkView, const char* url, Ewk_Http_Method method, Eina_Hash* headers, const char* body)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

    ResourceRequest request(String::fromUTF8(url));

    switch (method) {
    case EWK_HTTP_METHOD_GET:
        request.setHTTPMethod("GET");
        break;
    case EWK_HTTP_METHOD_HEAD:
        request.setHTTPMethod("HEAD");
        break;
    case EWK_HTTP_METHOD_POST:
        request.setHTTPMethod("POST");
        break;
    case EWK_HTTP_METHOD_PUT:
        request.setHTTPMethod("PUT");
        break;
    case EWK_HTTP_METHOD_DELETE:
        request.setHTTPMethod("DELETE");
        break;
    default:
        return false;
    }

    if (headers) {
        Eina_Iterator* it = eina_hash_iterator_tuple_new(headers);
        void* data;
        while (eina_iterator_next(it, &data)) {
            Eina_Hash_Tuple* t = static_cast<Eina_Hash_Tuple*>(data);
            const char* name = static_cast<const char*>(t->key);
            const char* value = static_cast<const char*>(t->data);
            request.addHTTPHeaderField(name, value);
        }
        eina_iterator_free(it);
    }

    if (body)
        request.setHTTPBody(FormData::create(body));

    WKRetainPtr<WKURLRequestRef> urlRequest(AdoptWK,toAPI(WebURLRequest::create(request).leakRef()));
    WKPageLoadURLRequest(toAPI(impl->page()), urlRequest.get());

    return true;
}

void ewk_view_suspend(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    TIZEN_LOGI("");
#if ENABLE(TIZEN_JAVASCRIPT_AND_RESOURCE_SUSPEND_RESUME)
    impl->page()->suspendJavaScriptAndResource();
#endif

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
    impl->page()->suspendPlugin();
#endif // ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)

#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
    impl->page()->suspendPainting();
#endif
}

void ewk_view_scale_range_get(Evas_Object*, double*, double*)
{
    // FIXME: Need to implement
}

Evas_Object* ewk_view_screenshot_contents_get(const Evas_Object* ewkView, Eina_Rectangle viewArea, float scaleFactor, Evas* canvas)
{
#if ENABLE(TIZEN_SCREEN_CAPTURE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKRect rect;
    rect.origin.x = viewArea.x;
    rect.origin.y = viewArea.y;
    rect.size.width = viewArea.w;
    rect.size.height = viewArea.h;

    WKRetainPtr<WKImageRef> snapshot(AdoptWK, WKPageCreateSnapshot(toAPI(impl->page()), rect, scaleFactor));
    if (!snapshot.get())
        return 0;

    RefPtr<cairo_surface_t> screenshotSurface = adoptRef(WKImageCreateCairoSurface(snapshot.get()));

    Evas_Object* screenshotImage = evas_object_image_add(canvas);
    int surfaceWidth = cairo_image_surface_get_width(screenshotSurface.get());
    int surfaceHeight = cairo_image_surface_get_height(screenshotSurface.get());
    evas_object_image_size_set(screenshotImage, surfaceWidth, surfaceHeight);
    evas_object_image_colorspace_set(screenshotImage, EVAS_COLORSPACE_ARGB8888);

    uint8_t* pixels = static_cast<uint8_t*>(evas_object_image_data_get(screenshotImage, true));

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32, surfaceWidth, surfaceHeight, cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, surfaceWidth)));
    RefPtr<cairo_t> cairo = adoptRef(cairo_create(imageSurface.get()));

    cairo_set_source_surface(cairo.get(), screenshotSurface.get(), 0, 0);
    cairo_rectangle(cairo.get(), 0, 0, surfaceWidth, surfaceHeight);
    cairo_fill(cairo.get());

    evas_object_image_smooth_scale_set(screenshotImage, true);
    evas_object_size_hint_min_set(screenshotImage, surfaceWidth, surfaceHeight);
    evas_object_resize(screenshotImage, surfaceWidth, surfaceHeight);
    evas_object_image_fill_set(screenshotImage, 0, 0, surfaceWidth, surfaceHeight);
    evas_object_image_data_set(screenshotImage, pixels);

    return screenshotImage;
#else
    // FIXME: Need to implement
    return 0;
#endif
}

void ewk_view_scroll_by(Evas_Object* ewkView, int deltaX, int deltaY)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    if (WKPageUseFixedLayout(impl->wkPage()))
        impl->scrollBy(IntSize(deltaX, deltaY));
    else
        WKPageScrollBy(impl->wkPage(), WKSizeMake(deltaX, deltaY));

#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(deltaX);
    UNUSED_PARAM(deltaY);
#endif
}

Eina_Bool ewk_view_scroll_pos_get(const Evas_Object* ewkView, int* x, int* y)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    EwkView* impl = toEwkViewChecked(ewkView);
    if (EINA_UNLIKELY(!impl)) {
        EINA_LOG_CRIT("no private data for object %p", ewkView);
        if (x)
            *x = 0;
        if (y)
            *y = 0;

        return false;
    }

    WKPoint position;
    if (WKPageUseFixedLayout(impl->wkPage())) {
        position = WKViewGetContentPosition(impl->wkView());
        float scaleFactor = WKPageGetScaleFactor(impl->wkPage());
        position.x = position.x * scaleFactor;
        position.y = position.y * scaleFactor;
    } else
        position = WKPageGetScrollPosition(impl->wkPage());
    if (x)
        *x = position.x;
    if (y)
        *y = position.y;

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(x);
    UNUSED_PARAM(y);
    return false;
#endif
}

Eina_Bool ewk_view_scroll_set(Evas_Object* ewkView, int x, int y)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    if (WKPageUseFixedLayout(impl->wkPage())) {
        float scaleFactor = WKPageGetScaleFactor(impl->wkPage());
        impl->scrollTo(FloatPoint(x / scaleFactor, y / scaleFactor));
    } else
        WKPageScrollTo(impl->wkPage(), WKPointMake(x, y));

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(x);
    UNUSED_PARAM(y);
    return false;
#endif
}

Eina_Bool ewk_view_scroll_size_get(const Evas_Object* ewkView, int* width, int* height)
{
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    EwkView* impl = toEwkViewChecked(ewkView);
    if (EINA_UNLIKELY(!impl)) {
        EINA_LOG_CRIT("no private data for object %p", ewkView);
        if (width)
            *width = 0;
        if (height)
            *height = 0;

        return false;
    }

    WKSize contentsSize = WKViewGetContentsSize(impl->wkView());
    if (WKPageUseFixedLayout(impl->wkPage())) {
        float scaleFactor = WKPageGetScaleFactor(impl->wkPage());
        contentsSize.width = contentsSize.width * scaleFactor;
        contentsSize.height = contentsSize.height * scaleFactor;
    }

    WKSize viewSize = WKViewGetSize(impl->wkView());
    double deviceScaleFactor = impl->deviceScaleFactor();
    viewSize.width = round(viewSize.width / deviceScaleFactor);
    viewSize.height = round(viewSize.height / deviceScaleFactor);

    if (width)
        *width = contentsSize.width > viewSize.width ? contentsSize.width - viewSize.width : 0;
    if (height)
        *height = contentsSize.height > viewSize.height ? contentsSize.height - viewSize.height : 0;

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(width);
    UNUSED_PARAM(height);
    return false;
#endif
}

double ewk_view_text_zoom_get(const Evas_Object* ewkView)
{
#if ENABLE(TIZEN_TEXT_ZOOM)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 1);

    return WKPageGetTextZoomFactor(toAPI(impl->page()));
#else
    return 1;
#endif
}

Eina_Bool ewk_view_text_zoom_set(Evas_Object* ewkView, double textZoomFactor)
{
#if ENABLE(TIZEN_TEXT_ZOOM)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetTextZoomFactor(toAPI(impl->page()), textZoomFactor);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_foreground_set(Evas_Object*, Eina_Bool)
{
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    // FIXME: Need to implement
    return false;
#else
    return false;
#endif
}

Eina_Bool ewk_view_main_frame_scrollbar_visible_set(Evas_Object* ewkView, Eina_Bool visible)
{
#if ENABLE(TIZEN_SCROLLBAR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setMainFrameScrollbarVisibility(visible);
    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(visible);
    return false;
#endif // ENABLE(TIZEN_SCROLLBAR)
}

Eina_Bool ewk_view_main_frame_scrollbar_visible_get(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_SCROLLBAR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->getMainFrameScrollbarVisibility();
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif // ENABLE(TIZEN_SCROLLBAR)
}

Ewk_Hit_Test* ewk_view_hit_test_new(Evas_Object* ewkView, int x, int y, int hitTestMode)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    IntPoint pointForHitTest = impl->webView()->transformFromScene().mapPoint(IntPoint(x, y));
    WebHitTestResult::Data hitTestResultData = impl->page()->hitTestResultAtPoint(pointForHitTest, hitTestMode);
    Ewk_Hit_Test* hitTest = ewkHitTestCreate(hitTestResultData);

    return hitTest;
#else
    return 0;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
void ewkViewTextStyleState(Evas_Object* ewkView, const IntPoint& startPoint, const IntPoint& endPoint)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    EditorState editorState = impl->page()->editorState();
    Ewk_Text_Style* textStyle = ewkTextStyleCreate(editorState, startPoint, endPoint);
    evas_object_smart_callback_call(ewkView, "text,style,state", static_cast<void*>(textStyle));
    ewkTextStyleDelete(textStyle);
}
#endif

void ewk_view_content_security_policy_set(Evas_Object* ewkView, const char* policy, Ewk_CSP_Header_Type type)
{
#if ENABLE(TIZEN_CSP)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->page()->setContentSecurityPolicy(String::fromUTF8(policy), static_cast<WebCore::ContentSecurityPolicy::HeaderType>(type));
#endif
}

Eina_Bool ewk_view_contents_pdf_get(Evas_Object* ewkView, int width, int height, const char* fileName)
{
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(fileName, false);

    WKPageGetSnapshotPdfFile(toAPI(impl->page()), toAPI(IntSize(width, height)), WKStringCreateWithUTF8CString(fileName));

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(width);
    UNUSED_PARAM(height);
    UNUSED_PARAM(fileName);
    return false;
#endif // ENABLE(TIZEN_MOBILE_WEB_PRINT)
}

Eina_Bool ewk_view_custom_header_add(const Evas_Object*, const char*, const char*)
{
    //FIXME: Remove this
    ERR("TIZEN_CUSTOM_HEADERS is not supprted!");
    return false;
}

Eina_Bool ewk_view_custom_header_remove(const Evas_Object*, const char*)
{
    //FIXME: Remove this
    ERR("TIZEN_CUSTOM_HEADERS is not supprted!");
    return false;
}

Eina_Bool ewk_view_custom_header_clear(const Evas_Object*)
{
    //FIXME: Remove this
    ERR("TIZEN_CUSTOM_HEADERS is not supprted!");
    return false;
}

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
static void didGetWebAppCapable(WKBooleanRef capable, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(capable);
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_View_Callback_Context* webAppContext = static_cast<Ewk_View_Callback_Context*>(context);

    ASSERT(webAppContext->webAppCapableCallback);

    //TIZEN_LOGI("webAppCapableCallback exist. capable(%b)", capable);
    if (capable)
        webAppContext->webAppCapableCallback(toImpl(capable)->value(), webAppContext->userData);
    else
        webAppContext->webAppCapableCallback(0, webAppContext->userData);

    delete webAppContext;
}

static void didGetWebAppIconURL(WKStringRef iconURL, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(iconURL);
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_View_Callback_Context* webAppContext = static_cast<Ewk_View_Callback_Context*>(context);

    EWK_VIEW_IMPL_GET_OR_RETURN(webAppContext->ewkView, impl);

    ASSERT(webAppContext->webAppIconURLCallback);

    if (iconURL) {
        impl->setWebAppIconURL(WKEinaSharedString(iconURL));
        webAppContext->webAppIconURLCallback(impl->getWebAppIconURL(), webAppContext->userData);
    } else
        webAppContext->webAppIconURLCallback(0, webAppContext->userData);

    delete webAppContext;
}

static void didGetWebAppIconURLs(WKDictionaryRef iconURLs, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(iconURLs);
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_View_Callback_Context* webAppContext = static_cast<Ewk_View_Callback_Context*>(context);

    EWK_VIEW_IMPL_GET_OR_RETURN(webAppContext->ewkView, impl);

    ASSERT(webAppContext->webAppIconURLsCallback);

    Eina_List* urls = impl->getWebAppIconURLs();
    if (urls) {
        void* data = 0;
        EINA_LIST_FREE(urls, data)
            ewkWebAppIconDataDelete(static_cast<Ewk_Web_App_Icon_Data*>(data));
    }

    WKRetainPtr<WKArrayRef> wkKeys(AdoptWK, WKDictionaryCopyKeys(iconURLs));
    size_t iconURLCount = WKArrayGetSize(wkKeys.get());
    for (size_t i = 0; i < iconURLCount; i++) {
        WKStringRef urlRef = static_cast<WKStringRef>(WKArrayGetItemAtIndex(wkKeys.get(), i));
        WKStringRef sizeRef = static_cast<WKStringRef>(WKDictionaryGetItemForKey(iconURLs, urlRef));
        urls = eina_list_append(urls, ewkWebAppIconDataCreate(sizeRef, urlRef));
    }

    impl->setWebAppIconURLs(urls);
    //TIZEN_LOGI("webAppIconURLsCallback exist. found %d icon urls", iconURLCount);

    webAppContext->webAppIconURLsCallback(impl->getWebAppIconURLs(), webAppContext->userData);
    delete webAppContext;
}
#endif

Eina_Bool ewk_view_web_application_capable_get(Evas_Object* ewkView, Ewk_Web_App_Capable_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    //TIZEN_LOGI("callback(%d), userData(%d)", callback, userData);
    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webAppCapableCallback = callback;
    context->ewkView = ewkView;
    context->userData = userData;

    WKPageGetWebAppCapable(impl->wkPage(), context, didGetWebAppCapable);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_web_application_icon_url_get(Evas_Object* ewkView, Ewk_Web_App_Icon_URL_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webAppIconURLCallback = callback;
    context->ewkView = ewkView;
    context->userData = userData;

    WKPageGetWebAppIconURL(impl->wkPage(), context, didGetWebAppIconURL);

    return true;
#else
    return 0;
#endif
}

Eina_Bool ewk_view_web_application_icon_urls_get(Evas_Object* ewkView, Ewk_Web_App_Icon_URLs_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    //TIZEN_LOGI("callback(%d), userData(%d)", callback, userData);
    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webAppIconURLsCallback = callback;
    context->ewkView = ewkView;
    context->userData = userData;

    WKPageGetWebAppIconURLs(impl->wkPage(), context, didGetWebAppIconURLs);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
    return 0;
#endif
}

void ewk_view_use_settings_font(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_USE_SETTINGS_FONT)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->page()->useSettingsFont();
#endif // ENABLE(TIZEN_USE_SETTINGS_FONT)
}

#if ENABLE(TIZEN_INPUT_DATE_TIME)
void ewk_view_date_time_picker_set(Evas_Object* ewkView, const char* value)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->page()->didEndDateTimeChooser(String::fromUTF8(value));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
void ewkViewPolicyNavigationDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
    if (impl->policyDecision)
        ewkPolicyDecisionDelete(impl->policyDecision);
    impl->policyDecision = policyDecision;

    evas_object_smart_callback_call(ewkView, "policy,navigation,decide", impl->policyDecision);
}

void ewkViewPolicyNewWindowDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
    if (impl->policyDecision)
        ewkPolicyDecisionDelete(impl->policyDecision);
    impl->policyDecision = policyDecision;

    evas_object_smart_callback_call(ewkView, "policy,newwindow,decide", impl->policyDecision);
}

void ewkViewPolicyResponseDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);
    if (impl->policyDecision)
        ewkPolicyDecisionDelete(impl->policyDecision);
    impl->policyDecision = policyDecision;

    evas_object_smart_callback_call(ewkView, "policy,response,decide", impl->policyDecision);
}
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)

Eina_Bool ewk_view_page_save(Evas_Object* ewkView, const char* path)
{
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    String title = ewk_view_title_get(ewkView);
    String url = ewk_view_url_get(ewkView);
    String directoryPath(path);
    impl->startOfflinePageSave(directoryPath, url, title);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(path);
    return false;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
void ewkViewLoadError(Evas_Object* ewkView, WKErrorRef error)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    ewk_error_load_error_page(ewkError.get(), toAPI(impl->page()));
    evas_object_smart_callback_call(ewkView, "load,error", ewkError.get());
}
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)

Eina_Bool ewk_frame_is_main_frame(Ewk_Frame_Ref)
{
    // FIXME: Need to implement
    return false;
}

#if ENABLE(TIZEN_NOTIFICATIONS)
void ewkViewCancelNotification(Evas_Object* ewkView, uint64_t notificationID)
{
    // TIZEN_LOGI("notification,cancel");
    evas_object_smart_callback_call(ewkView, "notification,cancel", &notificationID);
}

void ewkViewRequestNotificationPermission(Evas_Object* ewkView, Ewk_Notification_Permission_Request* notificationPermissionRequest)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    // TIZEN_LOGI("notification,permission,request");
    impl->notificationPermissionRequests = eina_list_append(impl->notificationPermissionRequests, notificationPermissionRequest);
    evas_object_smart_callback_call(ewkView, "notification,permission,request", notificationPermissionRequest);
}

void ewkViewShowNotification(Evas_Object* ewkView, Ewk_Notification* notification)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    // TIZEN_LOGI("notification,show");
    Eina_List* listIterator=0;
    void* data=0;
    const char* replaceID = ewkNotificationGetReplaceID(notification);
    if(strlen(replaceID)) {
        EINA_LIST_FOREACH(impl->notifications, listIterator, data) {
            Ewk_Notification* notificationForReplace = static_cast<Ewk_Notification*>(data);
            if(!strcmp(ewkNotificationGetReplaceID(notificationForReplace), replaceID))
                ewkViewCancelNotification(ewkView, ewk_notification_id_get(notificationForReplace));
        }
    }

    impl->notifications = eina_list_append(impl->notifications, notification);
    evas_object_smart_callback_call(ewkView, "notification,show", notification);
}

void ewkViewDeleteNotificationPermissionRequest(Evas_Object* ewkView, Ewk_Notification_Permission_Request* ewkNotificationPermissionRequest)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->notificationPermissionRequests = eina_list_remove(impl->notificationPermissionRequests, ewkNotificationPermissionRequest);
}
#endif // ENABLE(TIZEN_NOTIFICATIONS)

void ewk_view_exceeded_local_file_system_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* userData)
{
    //FIXME: Remove this
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
}

void ewk_view_exceeded_local_file_system_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
    //FIXME: Remove this
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
}

Eina_Bool ewk_view_text_selection_enable_set(Evas_Object* ewkView, Eina_Bool enable)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    return ewk_settings_text_selection_enabled_set(ewk_view_settings_get(ewkView), enable);
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_view_text_selection_range_get(Evas_Object* ewkView, Eina_Rectangle* leftRect, Eina_Rectangle* rightRect)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    IntRect leftSelectionRect;
    IntRect rightSelectionRect;
    if (!impl->page()->getSelectionHandlers(leftSelectionRect, rightSelectionRect)) {
        leftRect->x = 0;
        leftRect->y = 0;
        leftRect->w = 0;
        leftRect->h = 0;

        rightRect->x = 0;
        rightRect->y = 0;
        rightRect->w = 0;
        rightRect->h = 0;
        return false;
    }

    AffineTransform contentToScreen = impl->webView()->transformToScene();
    leftSelectionRect = contentToScreen.mapRect(leftSelectionRect);
    rightSelectionRect = contentToScreen.mapRect(rightSelectionRect);

    leftRect->x = leftSelectionRect.x();
    leftRect->y = leftSelectionRect.y();
    leftRect->w = leftSelectionRect.width();
    leftRect->h = leftSelectionRect.height();

    rightRect->x = rightSelectionRect.x();
    rightRect->y = rightSelectionRect.y();
    rightRect->w = rightSelectionRect.width();
    rightRect->h = rightSelectionRect.height();

    return true;;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(leftRect);
    UNUSED_PARAM(rightRect);
    return false;
#endif
}

const char* ewk_view_text_selection_text_get(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->page()->getSelectionText().utf8().data();
#else
    UNUSED_PARAM(ewkView);
    return 0;
#endif
}

Eina_Bool ewk_view_auto_clear_text_selection_mode_set(Evas_Object* ewkView, Eina_Bool enable)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    return ewk_settings_clear_text_selection_automatically_set(ewk_view_settings_get(ewkView), enable);
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_view_auto_clear_text_selection_mode_get(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    return ewk_settings_clear_text_selection_automatically_get(ewk_view_settings_get(ewkView));
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_text_selection_clear(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    bool isTextSelectionMode = impl->isTextSelectionMode();
    impl->setIsTextSelectionMode(false);
    return isTextSelectionMode;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_text_selection_range_clear(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_TEXT_SELECTION)
    return ewk_view_text_selection_clear(ewkView);
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

#if ENABLE(TIZEN_WEB_STORAGE)
static void didGetWebStorageQuota(WKUInt32Ref quota, WKErrorRef, void* context)
{
    Ewk_View_Callback_Context* storageContext = static_cast<Ewk_View_Callback_Context*>(context);

    if (quota)
        storageContext->webStorageQuotaCallback(toImpl(quota)->value(), storageContext->userData);
    else
        storageContext->webStorageQuotaCallback(0, storageContext->userData);

    delete storageContext;
}
#endif // ENABLE(TIZEN_WEB_STORAGE)

Eina_Bool ewk_view_web_storage_quota_get(const Evas_Object* ewkView, Ewk_Web_Storage_Quota_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    // TIZEN_LOGI("callback (%p)", callback);
    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webStorageQuotaCallback = callback;
    context->userData = userData;

    WKPageRef pageRef = toAPI(impl->page());
    WKPageGetWebStorageQuota(pageRef, context, didGetWebStorageQuota);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

Eina_Bool ewk_view_web_storage_quota_set(Evas_Object* ewkView, uint32_t quota)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    // TIZEN_LOGI("quota (%d)", quota);
    WKPageRef pageRef = toAPI(impl->page());
    WKPageSetWebStorageQuota(pageRef, quota);

    return true;
#else
    return false;
#endif // ENABLE(TIZEN_WEB_STORAGE)
}

#if ENABLE(TIZEN_MEDIA_STREAM)
void ewkViewRequestUserMediaPermission(Evas_Object* ewkView, Ewk_User_Media_Permission_Request* userMediaPermission)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->userMediaPermissionRequests = eina_list_append(impl->userMediaPermissionRequests, userMediaPermission);
    evas_object_smart_callback_call(ewkView, "usermedia,permission,request", userMediaPermission);
}

void ewkViewDeleteUserMediaPermissionRequest(Evas_Object* ewkView, Ewk_User_Media_Permission_Request* permission)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->userMediaPermissionRequests = eina_list_remove(impl->userMediaPermissionRequests, permission);
}
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_SQL_DATABASE)
bool ewkViewExceededDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, WKStringRef databaseName, unsigned long long expectedUsage)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    if (!impl->exceededDatabaseQuotaContext || !impl->exceededDatabaseQuotaContext->exceededDatabaseQuotaCallback)
        return false;

    // TIZEN_LOGI("No error in prameter. Request to display user confirm popup. expectedUsage(%llu)", expectedUsage);
    impl->isWaitingForExceededQuotaPopupReply = true;
    impl->exceededQuotaOrigin = EwkSecurityOrigin::create(origin);

    int length = WKStringGetMaximumUTF8CStringSize(databaseName);
    OwnArrayPtr<char> databaseNameBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(databaseName, databaseNameBuffer.get(), length);

    return impl->exceededDatabaseQuotaContext->exceededDatabaseQuotaCallback(ewkView, impl->exceededQuotaOrigin.get(), databaseNameBuffer.get(), expectedUsage, impl->exceededDatabaseQuotaContext->userData) == EINA_TRUE;
}
#endif // ENABLE(TIZEN_SQL_DATABASE)

void ewk_view_exceeded_database_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Database_Quota_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    if (!impl->exceededDatabaseQuotaContext)
        impl->exceededDatabaseQuotaContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->exceededDatabaseQuotaContext->exceededDatabaseQuotaCallback = callback;
    impl->exceededDatabaseQuotaContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

void ewk_view_exceeded_database_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    // TIZEN_LOGI("allow %d", allow);
    WKPageReplyExceededDatabaseQuota(toAPI(impl->page()), allow == EINA_TRUE);
    impl->exceededQuotaOrigin = 0;
    impl->isWaitingForExceededQuotaPopupReply = false;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
#endif // ENABLE(TIZEN_SQL_DATABASE)
}

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
void ewkViewRequestWebSpeechPermission(Evas_Object* ewkView, Ewk_WebSpeech_Permission_Request* webSpeechPermission)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    evas_object_smart_callback_call(ewkView, "webspeech,permission,request", webSpeechPermission);
}
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
bool ewkViewExceededIndexedDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, long long currentUsage)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    if (!impl->exceededIndexedDatabaseQuotaContext || !impl->exceededIndexedDatabaseQuotaContext->exceededIndexedDatabaseQuotaCallback)
        return false;

    impl->isWaitingForExceededQuotaPopupReply = true;
    impl->exceededQuotaOrigin = EwkSecurityOrigin::create(origin);

    // TIZEN_LOGI("currentUsage(%lld)", currentUsage);

    return impl->exceededIndexedDatabaseQuotaContext->exceededIndexedDatabaseQuotaCallback(ewkView, impl->exceededQuotaOrigin.get(), currentUsage, impl->exceededIndexedDatabaseQuotaContext->userData) == EINA_TRUE;
}
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

void ewk_view_exceeded_indexed_database_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* userData)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    if (!impl->exceededIndexedDatabaseQuotaContext)
        impl->exceededIndexedDatabaseQuotaContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->exceededIndexedDatabaseQuotaContext->exceededIndexedDatabaseQuotaCallback = callback;
    impl->exceededIndexedDatabaseQuotaContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif // ENABLE(TIZEN_INDEXED_DATABASE)
}

void ewk_view_exceeded_indexed_database_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    // TIZEN_LOGI("allow %d", allow);
    WKPageReplyExceededIndexedDatabaseQuota(toAPI(impl->page()), allow == EINA_TRUE);
    impl->exceededQuotaOrigin = 0;
    impl->isWaitingForExceededQuotaPopupReply = false;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
#endif // ENABLE(TIZEN_INDEXED_DATABASE)
}

#if PLATFORM(TIZEN)
static Eina_Bool _ewk_view_default_javascript_alert(Evas_Object* ewkView, const char* alertText, void*)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->javascriptPopup->alert(alertText);
}

static Eina_Bool _ewk_view_default_javascript_confirm(Evas_Object* ewkView, const char* message, void*)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->javascriptPopup->confirm(message);
}

static Eina_Bool _ewk_view_default_javascript_prompt(Evas_Object* ewkView, const char* message, const char* defaultValue, void*)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->javascriptPopup->prompt(message, defaultValue);
}

void ewkViewInitializeJavaScriptPopup(Evas_Object* ewkView)
{
    ewk_view_javascript_alert_callback_set(ewkView, _ewk_view_default_javascript_alert, 0);
    ewk_view_javascript_confirm_callback_set(ewkView, _ewk_view_default_javascript_confirm, 0);
    ewk_view_javascript_prompt_callback_set(ewkView, _ewk_view_default_javascript_prompt, 0);
}
#endif // PLATFORM(TIZEN)

void ewk_view_javascript_alert_callback_set(Evas_Object* ewkView, Ewk_View_JavaScript_Alert_Callback callback, void* userData)
{
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    if (!impl->alertContext)
        impl->alertContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->alertContext->javascriptAlertCallback = callback;
    impl->alertContext->ewkView = ewkView;
    impl->alertContext->userData = userData;
#endif // PLATFORM(TIZEN)
}

void ewk_view_javascript_alert_reply(Evas_Object* ewkView)
{
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKPageReplyJavaScriptAlert(toAPI(impl->page()));
    impl->isWaitingForJavaScriptPopupReply = false;
#endif // PLATFORM(TIZEN)
}

void ewk_view_javascript_confirm_callback_set(Evas_Object* ewkView, Ewk_View_JavaScript_Confirm_Callback callback, void* userData)
{
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    if (!impl->confirmContext)
        impl->confirmContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->confirmContext->javascriptConfirmCallback = callback;
    impl->confirmContext->ewkView = ewkView;
    impl->confirmContext->userData = userData;
#endif // PLATFORM(TIZEN)
}

void ewk_view_javascript_confirm_reply(Evas_Object* ewkView, Eina_Bool result)
{
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKPageReplyJavaScriptConfirm(toAPI(impl->page()), result == EINA_TRUE);
    impl->isWaitingForJavaScriptPopupReply = false;
#endif // PLATFORM(TIZEN)
}

void ewk_view_javascript_prompt_callback_set(Evas_Object* ewkView, Ewk_View_JavaScript_Prompt_Callback callback, void* userData)
{
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    if (!impl->promptContext)
        impl->promptContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->promptContext->javascriptPromptCallback = callback;
    impl->promptContext->ewkView = ewkView;
    impl->promptContext->userData = userData;
#endif // PLATFORM(TIZEN)
}

void ewk_view_javascript_prompt_reply(Evas_Object* ewkView, const char* result)
{
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKRetainPtr<WKStringRef> resultString(AdoptWK, WKStringCreateWithUTF8CString(result));
    WKPageReplyJavaScriptPrompt(toAPI(impl->page()), result ? resultString.get() : 0);
    impl->isWaitingForJavaScriptPopupReply = false;
#endif // PLATFORM(TIZEN)
}

Evas_Object* ewk_view_add_with_session_data(Evas* canvas, const char*, unsigned)
{
    // FIXME: need to implement
    return ewk_view_add(canvas);
}

#if ENABLE(TIZEN_POPUP_INTERNAL)
Eina_Bool ewk_view_popup_menu_close(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    impl->closePopupMenu();
    ewk_view_touch_events_enabled_set(ewkView, true);

    return true;
}
#endif

Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* ewkView, const char* applicationName)
{
#if ENABLE(TIZEN_USER_AGENT)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setApplicationName(applicationName);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(applicationName);
    return false;
#endif // #if ENABLE(TIZEN_USER_AGENT)
}

const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* ewkView)
{
#if ENABLE(TIZEN_USER_AGENT)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->applicationName();
#else
    UNUSED_PARAM(ewkView);
    return 0;
#endif // #if ENABLE(TIZEN_USER_AGENT)
}

void ewk_view_unfocus_allow_callback_set(Evas_Object*, Ewk_View_Unfocus_Allow_Callback, void*)
{
    // FIXME: Is it really needed?
}

#if ENABLE(TIZEN_CHECK_MODAL_POPUP)
void ewk_view_modal_popup_open_set(Evas_Object* ewkView, bool open)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->setOpenModalPopup(open);
}

bool ewk_view_modal_popup_open_get(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->isOpenModalPopup();
}
#endif

#if ENABLE(TIZEN_TV_PROFILE)
void ewk_view_pip_rect_set(Evas_Object*, int, int, int, int)
{
}

void ewk_view_pip_enable_set(Evas_Object*, Eina_Bool)
{
}

void ewk_view_draw_focus_ring_enable_set(Evas_Object*, Eina_Bool)
{
}
#endif

#if ENABLE(TIZEN_PAGE_VISIBILITY_API)
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGE_VISIBILITY_STATE_VISIBLE, kWKPageVisibilityStateVisible);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGE_VISIBILITY_STATE_HIDDEN, kWKPageVisibilityStateHidden);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGE_VISIBILITY_STATE_PRERENDER, kWKPageVisibilityStatePrerender);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGE_VISIBILITY_STATE_UNLOADED, kWKPageVisibilityStateUnloaded);
#endif

void ewk_view_page_visibility_set(Evas_Object* ewkView, Ewk_Page_Visibility_State pageVisibilityState, Eina_Bool initialState)
{
#if ENABLE(TIZEN_PAGE_VISIBILITY_API)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

#if ENABLE(TIZEN_PAGE_SUSPEND_RESUME)
    if (pageVisibilityState == kWKPageVisibilityStateVisible)
        impl->page()->resumePainting();

    if (pageVisibilityState == kWKPageVisibilityStateHidden)
        impl->page()->suspendPainting();
#endif
    WKPageSetVisibilityState(impl->wkPage(), static_cast<WKPageVisibilityState>(pageVisibilityState), initialState);
#endif
}

void ewk_view_mirrored_blur_set(Evas_Object *o,Eina_Bool state)
{
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    EwkView* impl = toEwkViewChecked(o);
    if (impl)
        impl->setAppNeedEffect(o,state);
    TIZEN_LOGI(" Set Mirrored Blur Effect  Called state = %d ",state);
#endif
}
