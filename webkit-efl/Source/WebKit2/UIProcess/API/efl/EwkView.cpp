/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.
   Copyright (C) 2014 Nokia Corporation and/or its subsidiary(-ies)

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
#include "EwkView.h"

#include "ContextMenuClientEfl.h"
#include "EasingCurves.h"
#include "EflScreenUtilities.h"
#include "FindClientEfl.h"
#include "FormClientEfl.h"
#include "InputMethodContextEfl.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "NotImplemented.h"
#include "PageLoadClientEfl.h"
#include "PagePolicyClientEfl.h"
#include "PageUIClientEfl.h"
#include "PageViewportController.h"
#include "PageViewportControllerClientEfl.h"
#include "SnapshotImageGL.h"
#include "ViewClientEfl.h"
#include "WKArray.h"
#include "WKDictionary.h"
#include "WKEventEfl.h"
#include "WKGeometry.h"
#include "WKNumber.h"
#include "WKPageGroup.h"
#include "WKPopupItem.h"
#include "WKString.h"
#include "WKView.h"
#include "WKViewEfl.h"
#include "WebContext.h"
#include "WebImage.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_color_picker_private.h"
#include "ewk_context_menu_item_private.h"
#include "ewk_context_menu_private.h"
#include "ewk_context_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_page_group_private.h"
#include "ewk_popup_menu_item_private.h"
#include "ewk_popup_menu_private.h"
#include "ewk_private.h"
#include "ewk_security_origin_private.h"
#include "ewk_settings_private.h"
#include "ewk_view.h"
#include "ewk_window_features_private.h"
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Edje.h>
#if USE(ACCELERATED_COMPOSITING)
#if PLATFORM(TIZEN)
#include "OpenGLShims.h"
#else
#include <Evas_GL.h>
#endif
#endif
#include <WebCore/CairoUtilitiesEfl.h>
#include <WebCore/Cursor.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/PlatformContextCairo.h>
#include <WebKit2/WKImageCairo.h>
#include <wtf/MathExtras.h>

#if ENABLE(VIBRATION)
#include "VibrationClientEfl.h"
#endif

#if ENABLE(TIZEN_POPUP_INTERNAL)
#include "ewk_popup_picker.h"
#endif

#if ENABLE(FULLSCREEN_API)
#include "WebFullScreenManagerProxy.h"
#endif

#if PLATFORM(TIZEN)
#include "ewk_view_tizen_client.h"
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
#include "ewk_web_application_icon_data_private.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
#include "ewk_view_private.h"
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ewk_notification_private.h" 
#include "ewk_view_notification_provider.h"
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "ewk_user_media_private.h"
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
#include "TextSelection.h"
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "Drag.h"
#include <WebCore/DragData.h>
#endif

#if ENABLE(TIZEN_HANDLE_MOUSE_WHEEL_IN_THE_UI_SIDE)
#include <WebCore/Scrollbar.h>
#endif

#if ENABLE(TIZEN_PREFERENCE)
#include "WKPreferencesTizen.h"
#endif

#if ENABLE(TIZEN_SCROLLBAR)
#include "MainFrameScrollbarTizen.h"
#include "WKPageTizen.h"
#endif // ENABLE(TIZEN_SCROLLBAR)

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
#include <WebCore/WindowsKeyboardCodes.h>
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
#include "ewk_console_message.h"
#include "ewk_console_message_private.h"
#endif

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
#include <Evas_Engine_GL_X11.h>
#include <widget_errno.h>
#include <widget_buffer.h>
#endif

using namespace EwkViewCallbacks;
using namespace WebCore;
using namespace WebKit;

static const int defaultCursorSize = 16;

// Auxiliary functions.

const char EwkView::smartClassName[] = "EWK2_View";

static inline void smartDataChanged(Ewk_View_Smart_Data* smartData)
{
    ASSERT(smartData);

    if (smartData->changed.any)
        return;

    smartData->changed.any = true;
    evas_object_smart_changed(smartData->self);
}

static Evas_Smart* defaultSmartClassInstance()
{
    static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(EwkView::smartClassName);
    static Evas_Smart* smart = 0;

    if (!smart) {
        EwkView::initSmartClassInterface(api);
        smart = evas_smart_class_new(&api.sc);
    }

    return smart;
}

static inline Ewk_View_Smart_Data* toSmartData(Evas_Object* evasObject)
{
    ASSERT(evasObject);
    ASSERT(isEwkViewEvasObject(evasObject));

    return static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject));
}

static inline EwkView* toEwkView(const Ewk_View_Smart_Data* smartData)
{
    ASSERT(smartData);
    ASSERT(smartData->priv);

    return smartData->priv;
}

EwkView* toEwkView(const Evas_Object* evasObject)
{
    ASSERT(evasObject);
    ASSERT(isEwkViewEvasObject(evasObject));

    return toEwkView(static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject)));
}

static inline void showEvasObjectsIfNeeded(const Ewk_View_Smart_Data* smartData)
{
    ASSERT(smartData);

    if (evas_object_clipees_get(smartData->base.clipper))
        evas_object_show(smartData->base.clipper);
    evas_object_show(smartData->image);
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    EwkView* view = toEwkView(smartData);
    if (view && view->getAppNeedEffect())
        evas_object_show(smartData->effectImage);
#endif
}

// EwkViewEventHandler implementation.

template <Evas_Callback_Type EventType>
class EwkViewEventHandler {
public:
    static void subscribe(Evas_Object* evasObject)
    {
        evas_object_event_callback_add(evasObject, EventType, handleEvent, toSmartData(evasObject));
    }

    static void unsubscribe(Evas_Object* evasObject)
    {
        evas_object_event_callback_del(evasObject, EventType, handleEvent);
    }

    static void handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo);
};

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_down)
        smartData->api->mouse_down(smartData, static_cast<Evas_Event_Mouse_Down*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_UP>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_up)
        smartData->api->mouse_up(smartData, static_cast<Evas_Event_Mouse_Up*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_move)
        smartData->api->mouse_move(smartData, static_cast<Evas_Event_Mouse_Move*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_FOCUS_IN>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->focus_in)
        smartData->api->focus_in(smartData);
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_FOCUS_OUT>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->focus_out)
        smartData->api->focus_out(smartData);
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_wheel)
        smartData->api->mouse_wheel(smartData, static_cast<Evas_Event_Mouse_Wheel*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_IN>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
#if !ENABLE(TIZEN_WEARABLE_PROFILE)
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EwkView* self = toEwkView(smartData);

    self->updateCursor(true);
#else
    UNUSED_PARAM(data);
#endif
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_KEY_DOWN>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->key_down)
        smartData->api->key_down(smartData, static_cast<Evas_Event_Key_Down*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_KEY_UP>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->key_up)
        smartData->api->key_up(smartData, static_cast<Evas_Event_Key_Up*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_SHOW>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    WKViewSetIsVisible(toEwkView(smartData)->wkView(), true);
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_HIDE>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    WKViewSetIsVisible(toEwkView(smartData)->wkView(), false);
#if ENABLE(TIZEN_SUSPEND_TOUCH_CANCEL)
    if (evas_touch_point_list_count(smartData->base.evas))
        toEwkView(smartData)->feedTouchCancelEvent();
#endif
}

typedef HashMap<WKPageRef, Evas_Object*> WKPageToEvasObjectMap;

static inline WKPageToEvasObjectMap& wkPageToEvasObjectMap()
{
    DEFINE_STATIC_LOCAL(WKPageToEvasObjectMap, map, ());
    return map;
}

// Transition implementation.

class Transition {
public:
#if PLATFORM(TIZEN)
    static PassOwnPtr<Transition> create(EwkView* view)
    {
        return adoptPtr(new Transition(view));
    }
#endif
    explicit Transition(EwkView*);
    ~Transition();

    void run(const FloatRect& baseRect, const FloatRect& targetRect);

private:
    static Eina_Bool animatorCallback(void*);

    static const unsigned s_duration = 20;

    EwkView* m_view;
    Ecore_Animator* m_animator;
    FloatRect m_targetRect;
    FloatRect m_baseRect;
    unsigned m_scaleIndex;
};

Transition::Transition(EwkView* view)
    : m_view(view)
    , m_animator(0)
    , m_scaleIndex(0)
{
}

Transition::~Transition()
{
    if (m_animator)
        ecore_animator_del(m_animator);
}

void Transition::run(const FloatRect& baseRect, const FloatRect& targetRect)
{
    if (m_animator)
        ecore_animator_del(m_animator);
    m_animator = ecore_animator_add(animatorCallback, this);
    m_baseRect = baseRect;
    m_targetRect = targetRect;
    m_scaleIndex = 1;
}

Eina_Bool Transition::animatorCallback(void* data)
{
    Transition* transition = static_cast<Transition*>(data);
    FloatRect baseRect = transition->m_baseRect;
    FloatRect targetRect = transition->m_targetRect;
    float multiplier = easeInOutQuad(transition->m_scaleIndex++, 0, 1, transition->s_duration);

    FloatRect rect(baseRect.x() + (targetRect.x() - baseRect.x()) * multiplier,
        baseRect.y() + (targetRect.y() - baseRect.y()) * multiplier,
        baseRect.width() + (targetRect.width() - baseRect.width()) * multiplier,
        baseRect.height() + (targetRect.height() - baseRect.height()) * multiplier);
    WKSize size = WKViewGetSize(transition->m_view->wkView());
    transition->m_view->scaleTo(size.width / transition->m_view->deviceScaleFactor() / rect.width(), rect.location());

    if (transition->m_scaleIndex > transition->s_duration) {
        transition->m_animator = 0;
        return ECORE_CALLBACK_CANCEL;
    }

    return ECORE_CALLBACK_RENEW;
}

// EwkView implementation.

EwkView::EwkView(WKViewRef view, Evas_Object* evasObject)
    : m_webView(view)
#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
    , policyDecision(0)
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
#if ENABLE(TIZEN_NOTIFICATIONS)
    , notifications(0)
    , notificationPermissionRequests(0)
#endif // ENABLE(TIZEN_NOTIFICATIONS)
#if ENABLE(TIZEN_MEDIA_STREAM)
    , userMediaPermissionRequests(0)
#endif // ENABLE(TIZEN_MEDIA_STREAM)
#if ENABLE(TIZEN_POPUP_INTERNAL)
    , popupPicker(0)
#endif
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    , certificatePolicyDecision(0)
#endif
    , m_evasObject(evasObject)
    , m_context(EwkContext::findOrCreateWrapper(WKPageGetContext(wkPage())))
    , m_pageGroup(EwkPageGroup::findOrCreateWrapper(WKPageGetPageGroup(wkPage())))
    , m_pageLoadClient(PageLoadClientEfl::create(this))
    , m_pagePolicyClient(PagePolicyClientEfl::create(this))
    , m_pageUIClient(PageUIClientEfl::create(this))
    , m_contextMenuClient(ContextMenuClientEfl::create(this))
    , m_findClient(FindClientEfl::create(this))
    , m_formClient(FormClientEfl::create(this))
    , m_viewClient(ViewClientEfl::create(this))
#if ENABLE(VIBRATION)
    , m_vibrationClient(VibrationClientEfl::create(this))
#endif
    , m_backForwardList(EwkBackForwardList::create(WKPageGetBackForwardList(wkPage())))
    , m_settings(EwkSettings::create(this))
    , m_useCustomCursor(false)
    , m_userAgent(WKEinaSharedString(AdoptWK, WKPageCopyUserAgent(wkPage())))
    , m_mouseEventsEnabled(false)
#if ENABLE(TOUCH_EVENTS)
    , m_touchEventsEnabled(false)
    , m_gestureRecognizer(GestureRecognizer::create(this))
#endif
    , m_displayTimer(this, &EwkView::displayTimerFired)
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    , m_effectImageTimer(this, &EwkView::effectImageTimerFired)
#endif
#if ENABLE(TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT)
    , m_inputMethodContext(InputMethodContextEfl::create(this))
#else
    , m_inputMethodContext(InputMethodContextEfl::create(this, smartData()->base.evas))
#endif
#if USE(ACCELERATED_COMPOSITING)
    , m_pageViewportControllerClient(PageViewportControllerClientEfl::create(this))
    , m_pageViewportController(adoptPtr(new PageViewportController(page(), m_pageViewportControllerClient.get())))
#endif
    , m_isAccelerated(true)
#if PLATFORM(TIZEN)
    , m_transition(Transition::create(this))
#else
    , m_transition(std::make_unique<Transition>(this))
#endif
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    , m_webAppIconURLs(0)
#endif
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    , m_offlinePageSave(OfflinePageSave::create(this))
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
    , m_drag(Drag::create(this))
#endif // ENABLE(TIZEN_DRAG_SUPPORT)
#if ENABLE(TIZEN_USER_AGENT)
    , m_applicationName(WKEinaSharedString(AdoptWK, WKPageCopyApplicationNameForUserAgent(wkPage())))
#endif // #if ENABLE(TIZEN_USER_AGENT)
#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
    , m_isVerticalEdge(false)
    , m_isHorizontalEdge(false)
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    , m_hasVerticalScrollableArea(false)
    , m_hasHorizontalScrollableArea(false)
#endif
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    , m_scheduleUpdateDisplayAnimator(0)
    , m_hasDirty(false)
#endif
#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
    , m_didFirstFrameReady(false)
    , m_didSetVisibleContentsRect(false)
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    , m_focusRing(FocusRing::create(this))
#endif
#if ENABLE(TIZEN_POINTER_LOCK)
    , m_mouseLocked(false)
    , m_lockingMouse(false)
    , m_unlockingMouse(false)
    , m_mouseClickCounter(0)
#endif
#if ENABLE(TIZEN_VD_WHEEL_FLICK)
    , m_wheelDirection(0)
    , m_wheelOffset(0)
    , m_flickAnimator(0)
#endif
#if ENABLE(TIZEN_SCROLLBAR)
    , m_mainFrameScrollbarVisibility(true)
#endif // ENABLE(TIZEN_SCROLLBAR)
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    , m_providedPixmap(0)
    , m_widgetResourceLock(0)
    , m_didStartWidgetDisplay(false)
#endif
#if ENABLE(TIZEN_CHECK_MODAL_POPUP)
    , m_isOpenModalPopup(0)
#endif
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    , m_needMirrorBlurEffect(0)
#endif
{
    ASSERT(m_evasObject);
    ASSERT(m_context);

#if ENABLE(TIZEN_TEXT_SELECTION)
    m_textSelection = TextSelection::create(this);
#endif

    // FIXME: Remove when possible.
    static_cast<WebViewEfl*>(webView())->setEwkView(this);

#if PLATFORM(TIZEN)
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(smartData()->base.evas);
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    bool useOpenGLBackend = false;
    const char* engine = ecore_evas_engine_name_get(ee);
    if (engine && !strcmp(engine, "opengl_x11"))
        useOpenGLBackend = true;

    TIZEN_LOGI("useOpenGLBackend : %d , engine : %s\n", useOpenGLBackend, engine);
#endif

#if USE(ACCELERATED_COMPOSITING)
#if ENABLE(TIZEN_DIRECT_RENDERING)
    bool useDirectRendering = true;

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    useDirectRendering = useOpenGLBackend;
#endif

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    m_providedPixmap = ecore_evas_gl_x11_pixmap_get(ee);
    TIZEN_LOGI("useDirectRendering : %d, usePixmapSurface : %d, pixmap id : %x\n", useDirectRendering, useProvidedPixmap(), m_providedPixmap);
    if (m_providedPixmap) {
        // TIZEN_LOGI("[IDLECLOCK]init this:%p, ee:%p", this, ee);
        ecore_evas_data_set(ee, "dirty", "false");
        evas_event_callback_add(ecore_evas_get(ee), EVAS_CALLBACK_RENDER_PRE, acquireWidgetResourceLock, this);
        evas_event_callback_add(ecore_evas_get(ee), EVAS_CALLBACK_RENDER_POST, releaseWidgetResourceLock, this);
    }
#endif

    if (useDirectRendering)
        m_evasGLSurfaceOption = (Evas_GL_Options_Bits)( EVAS_GL_OPTIONS_DIRECT | EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION | 0x1000 | 0x2000);
    else
        m_evasGLSurfaceOption = EVAS_GL_OPTIONS_NONE;
#endif // ENABLE(TIZEN_DIRECT_RENDERING)

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    if (useOpenGLBackend)
#endif
    {
        m_evasGL = adoptPtr(evas_gl_new(evas_object_evas_get(m_evasObject)));
        if (m_evasGL)
            m_evasGLContext = EvasGLContext::create(m_evasGL.get());
    }

    if (!m_evasGLContext) {
        WARN("Failed to create Evas_GL, falling back to software mode.");
        m_isAccelerated = false;
    }

    m_pendingSurfaceResize = m_isAccelerated & useDirectRendering;
#endif // USE(ACCELERATED_COMPOSITING)

    WKViewInitialize(wkView());
#if ENABLE(TIZEN_FORCE_CREATION_TEXTUREMAPPER)
    WKViewSetAccelerationMode(wkView(), m_isAccelerated); // early creation of TextureMapper
#endif
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    WKViewSetIdleClockMode(wkView(), useProvidedPixmap());
#endif
    WKPageGroupRef wkPageGroup = WKPageGetPageGroup(wkPage());
    WKPreferencesRef wkPreferences = WKPageGroupGetPreferences(wkPageGroup);
#if USE(ACCELERATED_COMPOSITING)
    WKPreferencesSetWebGLEnabled(wkPreferences, true);
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    WKPreferencesSetAcceleratedCompositingForOverflowScrollEnabled(wkPreferences, true);
#endif
#if PLATFORM(TIZEN)
    WKPreferencesSetAccelerated2DCanvasEnabled(wkPreferences, m_isAccelerated);
#else
    WKPreferencesSetAccelerated2DCanvasEnabled(wkPreferences, true);
#endif
#endif
    WKPreferencesSetFullScreenEnabled(wkPreferences, true);
    WKPreferencesSetWebAudioEnabled(wkPreferences, true);
    WKPreferencesSetOfflineWebApplicationCacheEnabled(wkPreferences, true);
#if ENABLE(SPELLCHECK)
    WKPreferencesSetAsynchronousSpellCheckingEnabled(wkPreferences, true);
#endif
    WKPreferencesSetInteractiveFormValidationEnabled(wkPreferences, true);

#if PLATFORM(TIZEN)
#if ENABLE(TOUCH_EVENTS)
    // Enable touch events by default
    setMouseEventsEnabled(false);
    setTouchEventsEnabled(true);
#endif

    // FIXME: This line MUST be removed.
    // http://slp-info.sec.samsung.net/bugs/browse/WEB-5445
    WKPreferencesSetUniversalAccessFromFileURLsAllowed(wkPreferences, true);
#endif

#if !PLATFORM(TIZEN) || ENABLE(TIZEN_TV_PROFILE)
    // Enable mouse events by default
    setMouseEventsEnabled(true);
#endif

#if ENABLE(TIZEN_FALLBACK_THEME)
    // FIXME: It should be moved out
    setThemePath("/usr/share/edje/legacyWebkit.edj");
#endif

    // Listen for favicon changes.
    EwkFaviconDatabase* iconDatabase = m_context->faviconDatabase();
    ASSERT(iconDatabase);

    iconDatabase->watchChanges(IconChangeCallbackData(EwkView::handleFaviconChanged, this));

    WKPageToEvasObjectMap::AddResult result = wkPageToEvasObjectMap().add(wkPage(), m_evasObject);
    ASSERT_UNUSED(result, result.isNewEntry);

#if PLATFORM(TIZEN)
    javascriptPopup = adoptPtr<JavaScriptPopup>(new JavaScriptPopup(m_evasObject));

    ewkViewTizenClientAttachClient(this);
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_INPUT_DATE_TIME)
    m_inputPicker = adoptPtr(new InputPicker(m_evasObject));
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
    ewkViewNotificationProviderAttachProvider(m_evasObject, ewkContext()->wkContext());
#endif // ENABLE(TIZEN_NOTIFICATIONS)

#if ENABLE(TIZEN_WEARABLE_PROFILE)
    m_settings->preferences()->setLayoutFallbackWidth(320);
#endif

#if ENABLE(TIZEN_MSE)
    WKPreferencesSetMediaSourceEnabled(wkPreferences, true);
#endif
}

EwkView::~EwkView()
{
    // Unregister icon change callback.
    EwkFaviconDatabase* iconDatabase = m_context->faviconDatabase();
    ASSERT(iconDatabase);

    iconDatabase->unwatchChanges(EwkView::handleFaviconChanged);

    ASSERT(wkPageToEvasObjectMap().get(wkPage()) == m_evasObject);
    wkPageToEvasObjectMap().remove(wkPage());

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    if (m_needMirrorBlurEffect)
        WKViewCleanBlurEffectResources(wkView());
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    if (m_webAppIconURLs) {
        void* data = 0;
        EINA_LIST_FREE(m_webAppIconURLs, data)
            ewkWebAppIconDataDelete(static_cast<Ewk_Web_App_Icon_Data*>(data));
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
    if (policyDecision)
        ewkPolicyDecisionDelete(policyDecision);
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)

#if ENABLE(TIZEN_NOTIFICATIONS)
    if (notifications)
        ewkNotificationDeleteNotificationList(notifications);
    if (notificationPermissionRequests)
        ewkNotificationDeletePermissionRequestList(notificationPermissionRequests);
#endif // ENABLE(TIZEN_NOTIFICATIONS)

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (userMediaPermissionRequests)
        ewkUserMediaDeletePermissionRequestList(userMediaPermissionRequests);
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    if (certificatePolicyDecision)
        ewkCertificatePolicyDecisionDelete(certificatePolicyDecision);
#endif
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    if (m_scheduleUpdateDisplayAnimator) {
        ecore_animator_del(m_scheduleUpdateDisplayAnimator);
        m_scheduleUpdateDisplayAnimator = 0;
    }
#endif
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    if (m_providedPixmap) {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(smartData()->base.evas);
        // TIZEN_LOGI("[IDLECLOCK]destory this:%p, ee:%p", this, ee);
        if (m_widgetResourceLock)
            releaseWidgetResourceLock(this, 0, 0);
        evas_event_callback_del(ecore_evas_get(ee), EVAS_CALLBACK_RENDER_PRE, acquireWidgetResourceLock);
        evas_event_callback_del(ecore_evas_get(ee), EVAS_CALLBACK_RENDER_POST, releaseWidgetResourceLock);
    }
#endif
}

EwkView* EwkView::create(WKViewRef webView, Evas* canvas, Evas_Smart* smart)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    Evas_Object* evasObject = evas_object_smart_add(canvas, smart ? smart : defaultSmartClassInstance());
    EINA_SAFETY_ON_NULL_RETURN_VAL(evasObject, 0);

    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    if (!smartData) {
        evas_object_del(evasObject);
        return 0;
    }

    ASSERT(!smartData->priv);

    smartData->priv = new EwkView(webView, evasObject);
#if PLATFORM(TIZEN)
    ewkViewInitializeJavaScriptPopup(smartData->priv->evasObject());
#endif

    return smartData->priv;
}

bool EwkView::initSmartClassInterface(Ewk_View_Smart_Class& api)
{
    if (api.version != EWK_VIEW_SMART_CLASS_VERSION) {
        EINA_LOG_CRIT("Ewk_View_Smart_Class %p is version %lu while %lu was expected.",
            &api, api.version, EWK_VIEW_SMART_CLASS_VERSION);
        return false;
    }

    if (!parentSmartClass.add)
        evas_object_smart_clipped_smart_set(&parentSmartClass);

    evas_object_smart_clipped_smart_set(&api.sc);

    // Set Evas_Smart_Class callbacks.
    api.sc.add = handleEvasObjectAdd;
    api.sc.del = handleEvasObjectDelete;
    api.sc.move = handleEvasObjectMove;
    api.sc.resize = handleEvasObjectResize;
    api.sc.show = handleEvasObjectShow;
    api.sc.hide = handleEvasObjectHide;
    api.sc.color_set = handleEvasObjectColorSet;
    api.sc.calculate = handleEvasObjectCalculate;
    api.sc.data = EwkView::smartClassName; // It is used for type checking.

    // Set Ewk_View_Smart_Class callbacks.
    api.focus_in = handleEwkViewFocusIn;
    api.focus_out = handleEwkViewFocusOut;

    api.mouse_wheel = handleEwkViewMouseWheel;
    api.mouse_down = handleEwkViewMouseDown;
    api.mouse_up = handleEwkViewMouseUp;
    api.mouse_move = handleEwkViewMouseMove;
    api.key_down = handleEwkViewKeyDown;
    api.key_up = handleEwkViewKeyUp;

#if ENABLE(TIZEN_FULLSCREEN_API)
    api.fullscreen_enter = handleEwkViewFullscreenEnter;
    api.fullscreen_exit = handleEwkViewFullscreenExit;
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
    api.text_selection_down = handleEwkViewTextSelectionDown;
    api.text_selection_move = handleEwkViewTextSelectionMove;
    api.text_selection_up = handleEwkViewTextSelectionUp;
#endif

#if ENABLE(TIZEN_POPUP_INTERNAL)
    api.popup_menu_show = handleEwkViewPopupMenuShow;
    api.popup_menu_hide = handleEwkViewPopupMenuHide;
#endif

    return true;
}

Evas_Object* EwkView::toEvasObject(WKPageRef page)
{
    ASSERT(page);
    return wkPageToEvasObjectMap().get(page);
}

WKPageRef EwkView::wkPage() const
{
    return WKViewGetPage(wkView());
}

#if !ENABLE(TIZEN_WEARABLE_PROFILE)
void EwkView::updateCursor(bool hadCustomCursor)
{
    Ewk_View_Smart_Data* sd = smartData();

    if (m_useCustomCursor) {
        Image* cursorImage = static_cast<Image*>(m_cursorIdentifier.image);
        if (!cursorImage)
            return;

        RefPtr<Evas_Object> cursorObject = adoptRef(cursorImage->getEvasObject(sd->base.evas));
        if (!cursorObject)
            return;

        IntSize cursorSize = cursorImage->size();
        // Resize cursor.
        evas_object_resize(cursorObject.get(), cursorSize.width(), cursorSize.height());

        // Get cursor hot spot.
        IntPoint hotSpot;
        cursorImage->getHotSpot(hotSpot);

        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        // ecore_evas takes care of freeing the cursor object.
        ecore_evas_object_cursor_set(ecoreEvas, cursorObject.release().leakRef(), EVAS_LAYER_MAX, hotSpot.x(), hotSpot.y());

        return;
    }

    const char* group = static_cast<const char*>(m_cursorIdentifier.group);
    if (!group)
        return;

    RefPtr<Evas_Object> cursorObject = adoptRef(edje_object_add(sd->base.evas));

    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
    if (!m_theme || !edje_object_file_set(cursorObject.get(), m_theme, group)) {
        if (hadCustomCursor)
            ecore_evas_object_cursor_set(ecoreEvas, 0, 0, 0, 0);
#ifdef HAVE_ECORE_X
        if (WebCore::isUsingEcoreX(sd->base.evas))
            WebCore::applyFallbackCursor(ecoreEvas, group);
#endif
        return;
    }

    // Set cursor size.
    Evas_Coord width, height;
    edje_object_size_min_get(cursorObject.get(), &width, &height);
    if (width <= 0 || height <= 0)
        edje_object_size_min_calc(cursorObject.get(), &width, &height);
    if (width <= 0 || height <= 0) {
        width = defaultCursorSize;
        height = defaultCursorSize;
    }
    evas_object_resize(cursorObject.get(), width, height);

    // Get cursor hot spot.
    const char* data;
    int hotspotX = 0;
    data = edje_object_data_get(cursorObject.get(), "hot.x");
    if (data)
        hotspotX = atoi(data);

    int hotspotY = 0;
    data = edje_object_data_get(cursorObject.get(), "hot.y");
    if (data)
        hotspotY = atoi(data);

    // ecore_evas takes care of freeing the cursor object.
    ecore_evas_object_cursor_set(ecoreEvas, cursorObject.release().leakRef(), EVAS_LAYER_MAX, hotspotX, hotspotY);
}

void EwkView::setCursor(const Cursor& cursor)
{
    bool hadCustomCursor = false;

    if(m_useCustomCursor)
       hadCustomCursor = true;

    if (cursor.image()) {
        // Custom cursor.
        if (cursor.image() == m_cursorIdentifier.image)
            return;

        m_cursorIdentifier.image = cursor.image();
        m_useCustomCursor = true;
    } else {
        // Standard cursor.
        const char* group = cursor.platformCursor();
        if (!group || group == m_cursorIdentifier.group)
            return;

        m_cursorIdentifier.group = group;
        m_useCustomCursor = false;
    }

    updateCursor(hadCustomCursor);
}

#endif

void EwkView::setDeviceScaleFactor(float scale)
{
    const WKSize& deviceSize = WKViewGetSize(wkView());
    WKPageSetCustomBackingScaleFactor(wkPage(), scale);

    // Update internal viewport size after device-scale change.
    WKViewSetSize(wkView(), deviceSize);
}

float EwkView::deviceScaleFactor() const
{
    return WKPageGetBackingScaleFactor(wkPage());
}

AffineTransform EwkView::transformToScreen() const
{
    AffineTransform transform;

    int windowGlobalX = 0;
    int windowGlobalY = 0;

    Ewk_View_Smart_Data* sd = smartData();

#if !PLATFORM(TIZEN)
#ifdef HAVE_ECORE_X
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);

    Ecore_X_Window window;
    window = ecore_evas_gl_x11_window_get(ecoreEvas);
    // Fallback to software mode if necessary.
    if (!window)
        window = ecore_evas_software_x11_window_get(ecoreEvas); // Returns 0 if none.

    int x, y; // x, y are relative to parent (in a reparenting window manager).
    while (window) {
        ecore_x_window_geometry_get(window, &x, &y, 0, 0);
        windowGlobalX += x;
        windowGlobalY += y;
        window = ecore_x_window_parent_get(window);
    }
#endif
#endif

    transform.translate(-sd->view.x, -sd->view.y);
    transform.translate(windowGlobalX, windowGlobalY);

    return transform;
}

inline Ewk_View_Smart_Data* EwkView::smartData() const
{
    return toSmartData(m_evasObject);
}

inline IntSize EwkView::size() const
{
    // WebPage expects a size in UI units, and not raw device units.
    FloatSize uiSize = deviceSize();
    uiSize.scale(1 / deviceScaleFactor());
    return roundedIntSize(uiSize);
}

inline IntSize EwkView::deviceSize() const
{
    return toIntSize(WKViewGetSize(wkView()));
}

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
void EwkView::effectImageTimerFired(Timer<EwkView>*)
{
    evas_object_image_pixels_dirty_set(smartData()->image, true);
}
#endif

void EwkView::displayTimerFired(Timer<EwkView>*)
{
    Ewk_View_Smart_Data* sd = smartData();

#if USE(ACCELERATED_COMPOSITING)
    if (m_pendingSurfaceResize) {
        // Create a GL surface here so that Evas has no chance of painting to an empty GL surface.
        if (!createGLSurface())
            return;
        // Make Evas objects visible here in order not to paint empty Evas objects with black color.
        showEvasObjectsIfNeeded(sd);

        m_pendingSurfaceResize = false;
    }
#endif

    if (!m_isAccelerated) {
        RefPtr<cairo_surface_t> surface = createSurfaceForImage(sd->image);
        if (!surface)
            return;

        WKViewPaintToCairoSurface(wkView(), surface.get());
#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
        callFrameRenderedCallBack();
#endif
        evas_object_image_data_update_add(sd->image, 0, 0, sd->view.w, sd->view.h);
        return;
    }

#if USE(ACCELERATED_COMPOSITING)
    if (!evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context()))
        return;

    WKViewPaintToCurrentGLContext(wkView());
#endif

#if !OS(TIZEN)
    // sd->image is tied to a native surface, which is in the parent's coordinates.
    evas_object_image_data_update_add(sd->image, sd->view.x, sd->view.y, sd->view.w, sd->view.h);
#endif

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
    callFrameRenderedCallBack();
#endif
}

#if ENABLE(TIZEN_DIRECT_RENDERING)
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
void EwkView::setAppNeedEffect(Evas_Object *o, Eina_Bool state)
{
    if (m_needMirrorBlurEffect == state)
        return;

    Ewk_View_Smart_Data* smartData = toSmartData(o);
    smartData->effectImage = evas_object_image_add(smartData->base.evas);
    evas_object_image_alpha_set(smartData->effectImage, false);
    evas_object_image_filled_set(smartData->effectImage, true);
    evas_object_resize(smartData->effectImage, 360, 360);
    evas_object_image_size_set(smartData->effectImage, 360, 360);
    evas_object_image_fill_set(smartData->effectImage, 0, 0, 360, 360);
    evas_object_pass_events_set(smartData->effectImage ,1);
    evas_object_show(smartData->effectImage);

    m_needMirrorBlurEffect = state;
}
bool EwkView::getAppNeedEffect()
{
    return m_needMirrorBlurEffect;
}
void EwkView::compositeWithEffect()
{
    if (!evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context()))
        return;

    WKViewPaintToBackupTexture(wkView());
}
#endif
void EwkView::composite()
{
    if (!evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context()))
        return;

    int angle = evas_gl_rotation_get(m_evasGL.get());

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    if (useProvidedPixmap()) {
        if (!m_widgetResourceLock)
            return;
        WKViewPaintToCurrentGLContextWithOptions(wkView(), false, angle);
        // glFlush();
        if (m_didStartWidgetDisplay) {
            Ecore_Evas* ee = ecore_evas_ecore_evas_get(smartData()->base.evas);
            ecore_evas_data_set(ee, "dirty", "true");
        }
    } else
        WKViewPaintToCurrentGLContextWithOptions(wkView(), false, angle);
#else
    // 1. DIRECT_RENDERING must be off if a provided pixmap is existing.
    // 2. The flipY parameter should only be true when existing a provided pixmap.
    // This is only called when DIRECT_RENDERING is on.
    // So we always pass 'false' to WKViewPaintToCurrentGLContextWithOptions for the flipY parameter.
    WKViewPaintToCurrentGLContextWithOptions(wkView(), false, angle);
#endif

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
    callFrameRenderedCallBack();
#endif
}
#endif

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
bool EwkView::useProvidedPixmap()
{
    return !!m_providedPixmap;
}

void EwkView::startWidgetDisplay()
{
    // TIZEN_LOGI("[IDLECLOCK]START");
    m_didStartWidgetDisplay = true;
}
#endif

#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
Eina_Bool EwkView::scheduleUpdateDisplayAnimatorCallback(void* data)
{
    EwkView* ewkView = static_cast<EwkView*>(data);

    if (!ewkView->m_hasDirty)
        return ECORE_CALLBACK_RENEW;

#if ENABLE(TIZEN_DIRECT_RENDERING)
    if (ewkView->m_isAccelerated) {
        evas_object_image_pixels_dirty_set(ewkView->smartData()->image, true);
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
        if (ewkView->smartData()->effectImage)
            evas_object_image_pixels_dirty_set(ewkView->smartData()->effectImage, true);
#endif
    }
    else
#endif
    if (!ewkView->m_displayTimer.isActive())
        ewkView->m_displayTimer.startOneShot(0);
    ewkView->m_hasDirty = false;

    return ECORE_CALLBACK_RENEW;
}

void EwkView::setScheduleUpdateDisplayWithAnimator(bool enable)
{
    if (enable) {
        if (!m_scheduleUpdateDisplayAnimator)
            m_scheduleUpdateDisplayAnimator = ecore_animator_add(scheduleUpdateDisplayAnimatorCallback, this);
    } else {
        if (m_scheduleUpdateDisplayAnimator) {
            ecore_animator_del(m_scheduleUpdateDisplayAnimator);
            m_scheduleUpdateDisplayAnimator = 0;
            if (m_hasDirty) {
                scheduleUpdateDisplay();
                m_hasDirty = false;
            }
        }
    }
}
#endif

void EwkView::scheduleUpdateDisplay()
{
    if (deviceSize().isEmpty())
        return;

#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    if (m_scheduleUpdateDisplayAnimator) {
        m_hasDirty = true;
        return;
    }
#endif // #if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
#if ENABLE(TIZEN_DIRECT_RENDERING)
    if (m_isAccelerated) {
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
        if (!m_providedPixmap || m_didStartWidgetDisplay)
#endif
            evas_object_image_pixels_dirty_set(smartData()->image, true);
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
        if (smartData()->effectImage) {
            evas_object_image_pixels_dirty_set(smartData()->effectImage, true);
            if (m_effectImageTimer.isActive())
                m_effectImageTimer.stop();

            m_effectImageTimer.startOneShot(0.01);
          }
#endif
    }
    else
#endif
    if (!m_displayTimer.isActive())
        m_displayTimer.startOneShot(0);
}

#if ENABLE(TOUCH_EVENTS)
void EwkView::didChangeViewportAttributes()
{
    m_gestureRecognizer->setUseDoubleTap((m_pageViewportController->maximumScale() - m_pageViewportController->minimumScale()) > std::numeric_limits<float>::epsilon() && m_pageViewportController->allowsUserScaling());
}
#endif

#if ENABLE(TIZEN_HANDLE_MOUSE_WHEEL_IN_THE_UI_SIDE)
void EwkView::handleMouseWheelInTheUISide(const Evas_Event_Mouse_Wheel* event)
{
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    IntPoint point(event->canvas.x, event->canvas.y);
    findScrollableArea(point);
#endif

    int deltaX = 0;
    int deltaY = 0;
    if (event->direction == 0) // Vertical scroll direction.
        deltaY = event->z * Scrollbar::pixelsPerLineStep();
    else if (event->direction == 1) // Horizontal scroll direction.
        deltaX = event->z * Scrollbar::pixelsPerLineStep();

    scrollBy(IntSize(deltaX, deltaY));
#if ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
    scrollFinished();
#endif
}
#endif

#if ENABLE(FULLSCREEN_API)
/**
 * @internal
 * Calls fullscreen_enter callback or falls back to default behavior and enables fullscreen mode.
 */
void EwkView::enterFullScreen()
{
    Ewk_View_Smart_Data* sd = smartData();

    RefPtr<EwkSecurityOrigin> origin = EwkSecurityOrigin::create(m_url);

    if (!sd->api->fullscreen_enter || !sd->api->fullscreen_enter(sd, origin.get())) {
        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_fullscreen_set(ecoreEvas, true);
    }
}

/**
 * @internal
 * Calls fullscreen_exit callback or falls back to default behavior and disables fullscreen mode.
 */
void EwkView::exitFullScreen()
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->fullscreen_exit || !sd->api->fullscreen_exit(sd)) {
        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_fullscreen_set(ecoreEvas, false);
    }
}
#endif

WKRect EwkView::windowGeometry() const
{
    Evas_Coord x, y, width, height;
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->window_geometry_get || !sd->api->window_geometry_get(sd, &x, &y, &width, &height)) {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_request_geometry_get(ee, &x, &y, &width, &height);
    }

    return WKRectMake(x, y, width, height);
}

void EwkView::setWindowGeometry(const WKRect& rect)
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->window_geometry_set || !sd->api->window_geometry_set(sd, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)) {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_move_resize(ee, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }
}

const char* EwkView::title() const
{
    m_title = WKEinaSharedString(AdoptWK, WKPageCopyTitle(wkPage()));

    return m_title;
}

/**
 * @internal
 * This function may return @c NULL.
 */
InputMethodContextEfl* EwkView::inputMethodContext()
{
    return m_inputMethodContext.get();
}

const char* EwkView::themePath() const
{
    return m_theme;
}

void EwkView::setThemePath(const char* theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        WKRetainPtr<WKStringRef> wkTheme = adoptWK(WKStringCreateWithUTF8CString(theme));
        WKViewSetThemePath(wkView(), wkTheme.get());
    }
}

void EwkView::setCustomTextEncodingName(const char* customEncoding)
{
    if (m_customEncoding == customEncoding)
        return;

    m_customEncoding = customEncoding;
    WKRetainPtr<WKStringRef> wkCustomEncoding = adoptWK(WKStringCreateWithUTF8CString(customEncoding));
    WKPageSetCustomTextEncodingName(wkPage(), wkCustomEncoding.get());
}

void EwkView::setUserAgent(const char* userAgent)
{
    if (m_userAgent == userAgent)
        return;

    WKRetainPtr<WKStringRef> wkUserAgent = adoptWK(WKStringCreateWithUTF8CString(userAgent));
    WKPageSetCustomUserAgent(wkPage(), wkUserAgent.get());

    // When 'userAgent' is 0, user agent is set as a standard user agent by WKPageSetCustomUserAgent()
    // so m_userAgent needs to be updated using WKPageCopyUserAgent().
    m_userAgent = WKEinaSharedString(AdoptWK, WKPageCopyUserAgent(wkPage()));
}

void EwkView::setMouseEventsEnabled(bool enabled)
{
#if ENABLE(TIZEN_PREFERENCE)
    WKPreferencesSetDeviceSupportsMouse(WKPageGroupGetPreferences(WKPageGetPageGroup(wkPage())), enabled);
#endif
    if (m_mouseEventsEnabled == enabled)
        return;

    m_mouseEventsEnabled = enabled;
    if (enabled) {
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::subscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_UP>::subscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::subscribe(m_evasObject);
    } else {
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::unsubscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_UP>::unsubscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::unsubscribe(m_evasObject);
    }
}

#if ENABLE(TOUCH_EVENTS)
static WKTouchPointState toWKTouchPointState(Evas_Touch_Point_State state)
{
    switch (state) {
    case EVAS_TOUCH_POINT_UP:
        return kWKTouchPointStateTouchReleased;
    case EVAS_TOUCH_POINT_MOVE:
        return kWKTouchPointStateTouchMoved;
    case EVAS_TOUCH_POINT_DOWN:
        return kWKTouchPointStateTouchPressed;
    case EVAS_TOUCH_POINT_STILL:
        return kWKTouchPointStateTouchStationary;
    case EVAS_TOUCH_POINT_CANCEL:
    default:
        return kWKTouchPointStateTouchCancelled;
    }
}

static WKEventModifiers toWKEventModifiers(const Evas_Modifier* modifiers)
{
    WKEventModifiers wkModifiers = 0;
    if (evas_key_modifier_is_set(modifiers, "Shift"))
        wkModifiers |= kWKEventModifiersShiftKey;
    if (evas_key_modifier_is_set(modifiers, "Control"))
        wkModifiers |= kWKEventModifiersControlKey;
    if (evas_key_modifier_is_set(modifiers, "Alt"))
        wkModifiers |= kWKEventModifiersAltKey;
    if (evas_key_modifier_is_set(modifiers, "Meta"))
        wkModifiers |= kWKEventModifiersMetaKey;

    return wkModifiers;
}

void EwkView::feedTouchEvent(Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers)
{
    unsigned length = eina_list_count(points);
#if ENABLE(TIZEN_TEXT_SELECTION) && ENABLE(TIZEN_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP)
    if (isTextSelectionMode() && length == 1) {
        Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(eina_list_data_get(points));
        IntPoint handlePoint(point->x, point->y);

        if (type == EWK_TOUCH_START) {
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
            impl->page()->stopSpatialNavigation();
#endif
            textSelectonHandleDown(handlePoint);
        } else if (type == EWK_TOUCH_MOVE)
            textSelectonHandleMove(handlePoint);
        else
            textSelectonHandleUp();

        if (isTextSelectionHandleDowned())
            return;
    }
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
    if (isTextSelectionDowned() && type != EWK_TOUCH_CANCEL)
        return;
#endif

#if PLATFORM(TIZEN)
    if (type == EWK_TOUCH_START && length == 1)
        pageViewportController()->setHadUserInteraction(true);
#endif

    OwnArrayPtr<WKTypeRef> touchPoints = adoptArrayPtr(new WKTypeRef[length]);
    for (unsigned i = 0; i < length; ++i) {
        Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(eina_list_nth(points, i));
        ASSERT(point);
        IntPoint position(point->x, point->y);
        touchPoints[i] = WKTouchPointCreate(point->id, toAPI(IntPoint(position)), toAPI(transformToScreen().mapPoint(position)), toWKTouchPointState(point->state), WKSizeMake(0, 0), 0, 1);
    }
    WKRetainPtr<WKArrayRef> wkTouchPoints(AdoptWK, WKArrayCreateAdoptingValues(touchPoints.get(), length));

    WKViewSendTouchEvent(wkView(), adoptWK(WKTouchEventCreate(static_cast<WKEventType>(type), wkTouchPoints.get(), toWKEventModifiers(modifiers), ecore_time_get())).get());
}

void EwkView::setTouchEventsEnabled(bool enabled)
{
    if (m_touchEventsEnabled == enabled)
        return;

    m_touchEventsEnabled = enabled;

    if (enabled) {
        // FIXME: We have to connect touch callbacks with mouse and multi events
        // because the Evas creates mouse events for first touch and multi events
        // for second and third touches. Below codes should be fixed when the Evas
        // supports the touch events.
        // See https://bugs.webkit.org/show_bug.cgi?id=97785 for details.
        Ewk_View_Smart_Data* sd = smartData();
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MOUSE_DOWN, handleMouseDownForTouch, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MOUSE_UP, handleMouseUpForTouch, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MOUSE_MOVE, handleMouseMoveForTouch, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MULTI_DOWN, handleMultiDownForTouch, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MULTI_UP, handleMultiUpForTouch, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MULTI_MOVE, handleMultiMoveForTouch, sd);
    } else {
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MOUSE_DOWN, handleMouseDownForTouch);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MOUSE_UP, handleMouseUpForTouch);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MOUSE_MOVE, handleMouseMoveForTouch);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MULTI_DOWN, handleMultiDownForTouch);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MULTI_UP, handleMultiUpForTouch);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MULTI_MOVE, handleMultiMoveForTouch);
    }
}

void EwkView::doneWithTouchEvent(WKTouchEventRef event, bool wasEventHandled)
{
    if (wasEventHandled) {
        m_gestureRecognizer->reset();
        return;
    }

    m_gestureRecognizer->processTouchEvent(event);
}
#endif

#if ENABLE(TIZEN_DIRECT_RENDERING)
static void evasImageObjectPixelsGetCallback(void* data, Evas_Object* /* obj */)
{
    static_cast<EwkView*>(data)->composite();
}
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
static void evasImageEffectObjectPixelsGetCallback(void* data, Evas_Object* )
{
    static_cast<EwkView*>(data)->compositeWithEffect();
}
#endif
#endif

#if USE(ACCELERATED_COMPOSITING)
bool EwkView::createGLSurface()
{
    if (!m_isAccelerated)
        return true;

    static Evas_GL_Config evasGLConfig = {
        EVAS_GL_RGBA_8888,
        EVAS_GL_DEPTH_BIT_8,
        EVAS_GL_STENCIL_NONE,
#if ENABLE(TIZEN_DIRECT_RENDERING)
        m_evasGLSurfaceOption,
#else
        EVAS_GL_OPTIONS_NONE,
#endif
        EVAS_GL_MULTISAMPLE_NONE
    };

#if PLATFORM(TIZEN)
    Ewk_View_Smart_Data* sd = smartData();
    IntSize viewSize(sd->view.w, sd->view.h);

    m_evasGLSurface = EvasGLSurface::create(m_evasGL.get(), &evasGLConfig, viewSize);
#else
    // Recreate to current size: Replaces if non-null, and frees existing surface after (OwnPtr).
    m_evasGLSurface = EvasGLSurface::create(m_evasGL.get(), &evasGLConfig, deviceSize());
#endif
    if (!m_evasGLSurface)
        return false;

    Evas_Native_Surface nativeSurface;
    evas_gl_native_surface_get(m_evasGL.get(), m_evasGLSurface->surface(), &nativeSurface);
    evas_object_image_native_surface_set(smartData()->image, &nativeSurface);

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    if (smartData()->effectImage)
        evas_object_image_native_surface_set(smartData()->effectImage, &nativeSurface);
#endif

#if ENABLE(TIZEN_DIRECT_RENDERING)
    evas_object_image_pixels_get_callback_set(smartData()->image, evasImageObjectPixelsGetCallback, this);
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    if (smartData()->effectImage)
        evas_object_image_pixels_get_callback_set(smartData()->effectImage, evasImageEffectObjectPixelsGetCallback, this);
#endif
#endif

    evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context());
    Evas_GL_API* gl = evas_gl_api_get(m_evasGL.get());

#if PLATFORM(TIZEN)
    // FIXME : rebase
    WebCore::EvasGlApiInterface::shared().initialize(gl);
#if ENABLE(TIZEN_DIRECT_RENDERING)
    // If direct rendering is enabled, gl functions should be called in evasImageObjectPixelsGetCallback function.
    scheduleUpdateDisplay();
#else
    glViewport(0, 0, viewSize.width(), viewSize.height());
    glClearColor(1.0, 1.0, 1.0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
#endif
#else
    WKPoint boundsEnd = WKViewUserViewportToScene(wkView(), WKPointMake(deviceSize().width(), deviceSize().height()));
    gl->glViewport(0, 0, boundsEnd.x, boundsEnd.y);
    gl->glClearColor(1.0, 1.0, 1.0, 0);
    gl->glClear(GL_COLOR_BUFFER_BIT);
#endif

    return true;
}

void EwkView::setNeedsSurfaceResize()
{
#if ENABLE(TIZEN_DIRECT_RENDERING)
    createGLSurface();
#else
    m_pendingSurfaceResize = true;
#endif
}
#endif

#if ENABLE(INPUT_TYPE_COLOR)
/**
 * @internal
 * Requests to show external color picker.
 */
void EwkView::requestColorPicker(WKColorPickerResultListenerRef listener, const WebCore::Color& color)
{
    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api->input_picker_color_request);

    if (!sd->api->input_picker_color_request)
        return;

    if (m_colorPicker)
        dismissColorPicker();

    m_colorPicker = EwkColorPicker::create(listener, color);

    sd->api->input_picker_color_request(sd, m_colorPicker.get());
}

/**
 * @internal
 * Requests to hide external color picker.
 */
void EwkView::dismissColorPicker()
{
    if (!m_colorPicker)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api->input_picker_color_dismiss);

    if (sd->api->input_picker_color_dismiss)
        sd->api->input_picker_color_dismiss(sd);

    m_colorPicker.clear();
}
#endif

COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_RIGHT_TO_LEFT, RTL);
COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_LEFT_TO_RIGHT, LTR);

void EwkView::customContextMenuItemSelected(WKContextMenuItemRef contextMenuItem)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->custom_item_selected)
        return;

    OwnPtr<EwkContextMenuItem> item = EwkContextMenuItem::create(contextMenuItem, 0);

    sd->api->custom_item_selected(sd, item.get());
}

#if ENABLE(TIZEN_CALL_SIGNAL_FOR_WRT)
void EwkView::getContextMenuFromProposedMenu()
{
    evas_object_smart_callback_call(m_evasObject, "contextmenu,customize", 0);
}
#endif

void EwkView::showContextMenu(WKPoint position, WKArrayRef items)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->context_menu_show)
        return;

    if (m_contextMenu)
        hideContextMenu();

    m_contextMenu = EwkContextMenu::create(this, items);

    position = WKViewContentsToUserViewport(wkView(), position);

    sd->api->context_menu_show(sd, position.x, position.y, m_contextMenu.get());
}

void EwkView::hideContextMenu()
{
    if (!m_contextMenu)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (sd->api->context_menu_hide)
        sd->api->context_menu_hide(sd);

    m_contextMenu.clear();
}

COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_RIGHT_TO_LEFT, kWKPopupItemTextDirectionRTL);
COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_LEFT_TO_RIGHT, kWKPopupItemTextDirectionLTR);

void EwkView::requestPopupMenu(WKPopupMenuListenerRef popupMenuListener, const WKRect& rect, WKPopupItemTextDirection textDirection, double pageScaleFactor, WKArrayRef items, int32_t selectedIndex)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    ASSERT(popupMenuListener);

    if (!sd->api->popup_menu_show)
        return;

    if (m_popupMenu)
        closePopupMenu();

    m_popupMenu = EwkPopupMenu::create(this, popupMenuListener, items, selectedIndex);

    WKPoint popupMenuPosition = WKViewContentsToUserViewport(wkView(), rect.origin);

    Eina_Rectangle einaRect;
    EINA_RECTANGLE_SET(&einaRect, popupMenuPosition.x, popupMenuPosition.y, rect.size.width, rect.size.height);

    sd->api->popup_menu_show(sd, einaRect, static_cast<Ewk_Text_Direction>(textDirection), pageScaleFactor, m_popupMenu.get());
}

void EwkView::closePopupMenu()
{
    if (!m_popupMenu)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (sd->api->popup_menu_hide)
        sd->api->popup_menu_hide(sd);

    m_popupMenu.clear();
}

/**
 * @internal
 * Calls a smart member function for javascript alert().
 */
void EwkView::requestJSAlertPopup(const WKEinaSharedString& message)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_alert)
        return;

    sd->api->run_javascript_alert(sd, message);
}

/**
 * @internal
 * Calls a smart member function for javascript confirm() and returns a value from the function. Returns false by default.
 */
bool EwkView::requestJSConfirmPopup(const WKEinaSharedString& message)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_confirm)
        return false;

    return sd->api->run_javascript_confirm(sd, message);
}

/**
 * @internal
 * Calls a smart member function for javascript prompt() and returns a value from the function. Returns null string by default.
 */
WKEinaSharedString EwkView::requestJSPromptPopup(const WKEinaSharedString& message, const WKEinaSharedString& defaultValue)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_prompt)
        return WKEinaSharedString();

    return WKEinaSharedString::adopt(sd->api->run_javascript_prompt(sd, message, defaultValue));
}

#if ENABLE(SQL_DATABASE)
/**
 * @internal
 * Calls exceeded_database_quota callback or falls back to default behavior returns default database quota.
 */
unsigned long long EwkView::informDatabaseQuotaReached(const String& databaseName, const String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    static const unsigned long long defaultQuota = 5 * 1024 * 1204; // 5 MB
    if (sd->api->exceeded_database_quota)
        return sd->api->exceeded_database_quota(sd, databaseName.utf8().data(), displayName.utf8().data(), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);

    return defaultQuota;
}
#endif

WebView* EwkView::webView()
{
    return toImpl(m_webView.get());
}

/**
 * @internal
 * The url of view was changed by the frame loader.
 *
 * Emits signal: "url,changed" with pointer to new url string.
 */
void EwkView::informURLChange()
{
    WKRetainPtr<WKURLRef> wkActiveURL = adoptWK(WKPageCopyActiveURL(wkPage()));
    WKRetainPtr<WKStringRef> wkURLString = wkActiveURL ? adoptWK(WKURLCopyString(wkActiveURL.get())) : adoptWK(WKStringCreateWithUTF8CString(""));

    if (WKStringIsEqualToUTF8CString(wkURLString.get(), m_url))
        return;

    m_url = WKEinaSharedString(wkURLString.get());
    smartCallback<URLChanged>().call(m_url);

    // Update the view's favicon.
    smartCallback<FaviconChanged>().call();
}

Evas_Object* EwkView::createFavicon() const
{
    EwkFaviconDatabase* iconDatabase = m_context->faviconDatabase();
    ASSERT(iconDatabase);

    return ewk_favicon_database_icon_get(iconDatabase, m_url, smartData()->base.evas);
}

EwkWindowFeatures* EwkView::windowFeatures()
{
    if (!m_windowFeatures)
        m_windowFeatures = EwkWindowFeatures::create(0, this);

    return m_windowFeatures.get();
}

WKPageRef EwkView::createNewPage(PassRefPtr<EwkUrlRequest>, WKDictionaryRef windowFeatures)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->window_create)
        return 0;

    RefPtr<EwkWindowFeatures> ewkWindowFeatures = EwkWindowFeatures::create(windowFeatures, this);

    Evas_Object* newEwkView = sd->api->window_create(sd, ewkWindowFeatures.get());
    if (!newEwkView)
        return 0;

    EwkView* newViewImpl = toEwkView(newEwkView);
    ASSERT(newViewImpl);

    newViewImpl->m_windowFeatures = ewkWindowFeatures;

    return static_cast<WKPageRef>(WKRetain(newViewImpl->page()));
}

void EwkView::close()
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->window_close)
        return;

    sd->api->window_close(sd);
}

void EwkView::handleEvasObjectAdd(Evas_Object* evasObject)
{
    const Evas_Smart* smart = evas_object_smart_smart_get(evasObject);
    const Evas_Smart_Class* smartClass = evas_smart_class_get(smart);
    const Ewk_View_Smart_Class* api = reinterpret_cast<const Ewk_View_Smart_Class*>(smartClass);
    ASSERT(api);

    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);

    if (!smartData) {
        // Allocating with 'calloc' as the API contract is that it should be deleted with 'free()'.
        smartData = static_cast<Ewk_View_Smart_Data*>(calloc(1, sizeof(Ewk_View_Smart_Data)));
        evas_object_smart_data_set(evasObject, smartData);
    }

    smartData->self = evasObject;
    smartData->api = api;

    parentSmartClass.add(evasObject);

    smartData->priv = 0; // Will be initialized further.

    // Create evas_object_image to draw web contents.
    smartData->image = evas_object_image_add(smartData->base.evas);
    evas_object_image_alpha_set(smartData->image, false);
    evas_object_image_filled_set(smartData->image, true);
    evas_object_smart_member_add(smartData->image, evasObject);
    evas_object_show(smartData->image);

    smartData->effectImage = 0;

    EwkViewEventHandler<EVAS_CALLBACK_FOCUS_IN>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_FOCUS_OUT>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_MOUSE_IN>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_MOUSE_IN>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_KEY_DOWN>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_KEY_UP>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_SHOW>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_HIDE>::subscribe(evasObject);
}

void EwkView::handleEvasObjectDelete(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    if (smartData) {
        ASSERT(smartData->priv); // smartData->priv is EwkView instance.
        delete smartData->priv;
    }

    parentSmartClass.del(evasObject);
}

void EwkView::handleEvasObjectResize(Evas_Object* evasObject, Evas_Coord width, Evas_Coord height)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    TIZEN_LOGI("%d, %d", width, height);

    evas_object_resize(smartData->image, width, height);
    evas_object_image_size_set(smartData->image, width, height);
    evas_object_image_fill_set(smartData->image, 0, 0, width, height);

    smartData->changed.size = true;
    smartDataChanged(smartData);
}

void EwkView::handleEvasObjectMove(Evas_Object* evasObject, Evas_Coord /*x*/, Evas_Coord /*y*/)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    smartData->changed.position = true;
    smartDataChanged(smartData);
}

void EwkView::handleEvasObjectCalculate(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    EwkView* self = toEwkView(smartData);

    smartData->changed.any = false;

    Evas_Coord x, y, width, height;
    evas_object_geometry_get(evasObject, &x, &y, &width, &height);

    if (smartData->changed.position) {
        smartData->changed.position = false;
        smartData->view.x = x;
        smartData->view.y = y;
        evas_object_move(smartData->image, x, y);
        WKViewSetUserViewportTranslation(self->wkView(), x, y);
#if ENABLE(TIZEN_TEXT_SELECTION)
        self->updateTextSelectionHandlesAndContextMenu(true);
#endif
    }

    if (smartData->changed.size) {
        smartData->changed.size = false;
        smartData->view.w = width;
        smartData->view.h = height;

        WKViewSetSize(self->wkView(), WKSizeMake(width, height));
#if USE(ACCELERATED_COMPOSITING)
        if (WKPageUseFixedLayout(self->wkPage()))
            self->pageViewportController()->didChangeViewportSize(self->size());

        self->setNeedsSurfaceResize();
#endif
    }
#if ENABLE(TIZEN_SCROLLBAR)
    self->frameRectChanged();
#endif // ENABLE(TIZEN_SCROLLBAR)
}

void EwkView::handleEvasObjectShow(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    // If we use direct rendering, we don't use displayTimerFired,
    // and m_pendingSurfaceResize is never set to false.
#if !ENABLE(TIZEN_DIRECT_RENDERING)
#if USE(ACCELERATED_COMPOSITING)
    if (!toEwkView(smartData)->m_pendingSurfaceResize)
#endif
#endif // #if !ENABLE(TIZEN_DIRECT_RENDERING)
        showEvasObjectsIfNeeded(smartData);
}

void EwkView::handleEvasObjectHide(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    evas_object_hide(smartData->base.clipper);
    evas_object_hide(smartData->image);
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    EwkView* view = toEwkView(smartData);
    if (smartData->effectImage)
        evas_object_hide(smartData->effectImage);
#endif
#if ENABLE(TIZEN_TEXT_SELECTION)
    if (toEwkView(smartData)->isTextSelectionMode())
        toEwkView(smartData)->setIsTextSelectionMode(false);
#endif
}

void EwkView::handleEvasObjectColorSet(Evas_Object* evasObject, int red, int green, int blue, int alpha)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    int backgroundAlpha;
    WKViewGetBackgroundColor(toEwkView(smartData)->wkView(), nullptr, nullptr, nullptr, &backgroundAlpha);
    evas_object_image_alpha_set(smartData->image, alpha < 255 || backgroundAlpha < 255);
    parentSmartClass.color_set(evasObject, red, green, blue, alpha);
}

Eina_Bool EwkView::handleEwkViewFocusIn(Ewk_View_Smart_Data* smartData)
{
    WKViewSetIsFocused(toEwkView(smartData)->wkView(), true);
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    if (ewk_view_settings_get(smartData->self)->spatialNavigationEnabled())
        toEwkView(smartData)->page()->resumeSpatialNavigation();
#endif

#if ENABLE(TIZEN_WEBKIT2_IMF_CONTEXT_INPUT_PANEL)
    if (toEwkView(smartData)->page()->isContentEditable())
        toEwkView(smartData)->inputMethodContext()->showInputPanel();
#endif

    return true;
}

Eina_Bool EwkView::handleEwkViewFocusOut(Ewk_View_Smart_Data* smartData)
{
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    toEwkView(smartData)->page()->suspendSpatialNavigation();
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
    EwkView* self = toEwkView(smartData);
    if (self->textSelection()->isTextSelectionMode() && ewk_settings_clear_text_selection_automatically_get(self->settings()))
        self->textSelection()->setIsTextSelectionMode(false);
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    WebViewEfl* webView = static_cast<WebViewEfl*>(toEwkView(smartData)->webView());
    if (webView->isDragMode())
        webView->setDragMode(false);
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if ENABLE(TIZEN_WEBKIT2_IMF_CONTEXT_INPUT_PANEL)
    toEwkView(smartData)->inputMethodContext()->hideInputPanel();
#endif

    WKViewSetIsFocused(toEwkView(smartData)->wkView(), false);
    return true;
}

Eina_Bool EwkView::handleEwkViewMouseWheel(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Wheel* wheelEvent)
{
    EwkView* self = toEwkView(smartData);

#if ENABLE(TIZEN_HANDLE_MOUSE_WHEEL_IN_THE_UI_SIDE)
    if (WKPageUseFixedLayout(self->wkPage()))
        self->handleMouseWheelInTheUISide(wheelEvent);
    else
#endif
    self->page()->handleWheelEvent(NativeWebWheelEvent(wheelEvent, self->webView()->transformFromScene(), self->transformToScreen()));

#if ENABLE(TIZEN_VD_WHEEL_FLICK)
    if (WKPageUseFixedLayout(self->wkPage())) {
        static const int defaultScrollOffset = Scrollbar::pixelsPerLineStep();

        // FIXME: We should process flick whenever mouse wheel event occured.
        // Because we can't find out when user stop the scrolling by mouse wheel.
        if (self->m_flickAnimator) {
            ecore_animator_del(self->m_flickAnimator);
            self->m_flickAnimator = 0;
        }

        if (wheelEvent->direction == self->m_wheelDirection && wheelEvent->z == self->m_wheelOffset) {
            int offsetWidth = 0;
            int offsetHeight = 0;

            if (wheelEvent->direction == 0) // Vertical scroll direction.
                offsetHeight = defaultScrollOffset * wheelEvent->z;
            else if (wheelEvent->direction == 1) // Horizontal scroll direction.
                offsetWidth = defaultScrollOffset * wheelEvent->z;
            self->handleRemoteControlWheelFlick(IntSize(offsetWidth, offsetHeight));
        }
        self->m_wheelOffset = wheelEvent->z;
        self->m_wheelDirection = wheelEvent->direction;
    }
#endif
    return true;
}

#if ENABLE(TIZEN_VD_WHEEL_FLICK)
void EwkView::handleRemoteControlWheelFlick(const IntSize& offset)
{
    static const float defaultFlickDuration = 0.5;

    m_flickOffset = offset;
    m_flickIndex = m_flickDuration = defaultFlickDuration / ecore_animator_frametime_get();
    m_flickAnimator = ecore_animator_add(remoteControlWheelFlickAnimatorCallback, this);
}

Eina_Bool EwkView::remoteControlWheelFlickAnimatorCallback(void* data)
{
    EwkView* view = static_cast<EwkView*>(data);
    float multiplier = easeInOutQuad(view->m_flickIndex, 0, 1.0, view->m_flickDuration);
    float offsetWidth = view->m_flickOffset.width() * multiplier;
    float offsetHeight = view->m_flickOffset.height() * multiplier;
    offsetWidth = (offsetWidth > 0) ? ceilf(offsetWidth) : floorf(offsetWidth);
    offsetHeight = (offsetHeight > 0) ? ceilf(offsetHeight) : floorf(offsetHeight);
    IntSize offset(offsetWidth, offsetHeight);
    view->m_flickIndex--;

    if (offset.isZero() || !view->scrollBy(offset) || !view->m_flickIndex) {
        view->m_flickAnimator = 0;
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
        view->scrollFinished();
#endif
        return ECORE_CALLBACK_CANCEL;
    }

    return ECORE_CALLBACK_RENEW;
}
#endif // ENABLE(TIZEN_VD_WHEEL_FLICK)

Eina_Bool EwkView::handleEwkViewMouseDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Down* downEvent)
{
    EwkView* self = toEwkView(smartData);
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    self->page()->stopSpatialNavigation();
#endif
    self->page()->handleMouseEvent(NativeWebMouseEvent(downEvent, self->webView()->transformFromScene(), self->transformToScreen()));
    return true;
}

Eina_Bool EwkView::handleEwkViewMouseUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Up* upEvent)
{
    EwkView* self = toEwkView(smartData);
    self->page()->handleMouseEvent(NativeWebMouseEvent(upEvent, self->webView()->transformFromScene(), self->transformToScreen()));

    if (InputMethodContextEfl* inputMethodContext = self->inputMethodContext())
        inputMethodContext->handleMouseUpEvent(upEvent);

    return true;
}

Eina_Bool EwkView::handleEwkViewMouseMove(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Move* moveEvent)
{
    EwkView* self = toEwkView(smartData);

#if ENABLE(TIZEN_POINTER_LOCK)
    if (self->isMouseLocked()) {
        self->page()->handleMouseEvent(NativeWebMouseEvent(moveEvent, self->webView()->transformFromScene(), self->transformToScreen()));
        return true;
    }

    if (self->isMouseUnlocked()) {
        self->m_unlockingMouse = false;
        return true;
    }
#endif

    self->page()->handleMouseEvent(NativeWebMouseEvent(moveEvent, self->webView()->transformFromScene(), self->transformToScreen()));
    return true;
}

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
template<typename KeyEventType>
static bool isHWKeyboardEvent(KeyEventType event)
{
    return (event->timestamp > 1);
}

static bool isSpatialNavigationActivationKeyCode(int32_t keycode)
{
    return (keycode == VK_UP || keycode == VK_DOWN || keycode == VK_LEFT || keycode == VK_RIGHT
           || keycode == VK_TAB || keycode == VK_RETURN);
}
#endif

Eina_Bool EwkView::handleEwkViewKeyDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Down* downEvent)
{
    bool isFiltered = false;
    EwkView* self = toEwkView(smartData);

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    NativeWebKeyboardEvent nativeEvent(downEvent, false);
    if (!nativeEvent.windowsVirtualKeyCode())
        return false;

    WebPageProxy* page = self->page();

    if (isHWKeyboardEvent(downEvent)
        && ewk_view_settings_get(smartData->self)->spatialNavigationEnabled()
        && !page->spatialNavigationActivated()) {
        bool isActivationEvent = isSpatialNavigationActivationKeyCode(nativeEvent.windowsVirtualKeyCode());

        if (page->editorState().isContentEditable || isActivationEvent)
            page->activateSpatialNavigation();
        else
            return false;

        if (isActivationEvent)
            return false;
    }

#endif
    if (InputMethodContextEfl* inputMethodContext = self->inputMethodContext())
        inputMethodContext->handleKeyDownEvent(downEvent, &isFiltered);

#if ENABLE(TIZEN_POINTER_LOCK)
    if (self->isMouseLocked()) {
        if (strncmp("Escape", downEvent->keyname, 6)) {
            self->unlockMouse();
            self->page()->didLosePointerLock();
            return true;
        }
    }
#endif

    self->page()->handleKeyboardEvent(NativeWebKeyboardEvent(downEvent, isFiltered));
    return true;
}

Eina_Bool EwkView::handleEwkViewKeyUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Up* upEvent)
{
    EwkView* self = toEwkView(smartData);
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    WebPageProxy* page = self->page();
    if (isHWKeyboardEvent(upEvent) && !page->spatialNavigationActivated())
        return false;
#endif

    if (InputMethodContextEfl* inputMethodContext = self->inputMethodContext())
        inputMethodContext->handleKeyUpEvent(upEvent);

    toEwkView(smartData)->page()->handleKeyboardEvent(NativeWebKeyboardEvent(upEvent));
    return true;
}

#if ENABLE(TIZEN_TEXT_SELECTION)
Eina_Bool EwkView::handleEwkViewTextSelectionDown(Ewk_View_Smart_Data *smartData, int x, int y)
{
    return toEwkView(smartData)->textSelection()->textSelectionDown(IntPoint(x, y));
}

Eina_Bool EwkView::handleEwkViewTextSelectionMove(Ewk_View_Smart_Data *smartData, int x, int y)
{
    toEwkView(smartData)->textSelection()->textSelectionMove(IntPoint(x, y));

    return true;
}

Eina_Bool EwkView::handleEwkViewTextSelectionUp(Ewk_View_Smart_Data *smartData, int x, int y)
{
    toEwkView(smartData)->textSelection()->textSelectionUp(IntPoint(x, y), true);

    return true;
}
#endif

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
void EwkView::acquireWidgetResourceLock(void* data, Evas*, void*)
{
    if (data) {
        EwkView* ewkView = static_cast<EwkView*>(data);
        if (!ewkView->m_widgetResourceLock) {
            Evas_Engine_Info_GL_X11 *einfo = (Evas_Engine_Info_GL_X11 *)evas_engine_info_get(ewkView->smartData()->base.evas);
            Ecore_X_Pixmap pixmap = (Ecore_X_Pixmap)(einfo->info.offscreen ? einfo->info.drawable_back : einfo->info.drawable);
            ewkView->m_widgetResourceLock = widget_service_create_resource_lock(pixmap, WIDGET_LOCK_WRITE);
            if (ewkView->m_widgetResourceLock) {
                widget_service_acquire_resource_lock(ewkView->m_widgetResourceLock);
                // TIZEN_LOGI(" [IDLECLOCK]+++LOCKED   lock:%p, pixmap:%p", ewkView->m_widgetResourceLock, pixmap);
            }
        }
    }
}

void EwkView::releaseWidgetResourceLock(void* data, Evas*, void*)
{
    if (data) {
        EwkView* ewkView = static_cast<EwkView*>(data);
        if (ewkView->m_widgetResourceLock) {
            widget_service_release_resource_lock(ewkView->m_widgetResourceLock);
            // TIZEN_LOGI("[IDLECLOCK]---UNLOCKED lock:%p", ewkView->m_widgetResourceLock);
            widget_service_destroy_resource_lock(ewkView->m_widgetResourceLock, 0);
            ewkView->m_widgetResourceLock = 0;
        }
    }
}
#endif

#if ENABLE(TIZEN_POPUP_INTERNAL)
Eina_Bool EwkView::handleEwkViewPopupMenuShow(Ewk_View_Smart_Data* smartData, Eina_Rectangle rect, Ewk_Text_Direction /* text_direction */, double /* page_scale_factor */,  Ewk_Popup_Menu *ewk_menu)
{
    EwkView* self = toEwkView(smartData);

    if (self->popupPicker)
        ewk_popup_picker_del(self->popupPicker);

    self->popupPicker = ewk_popup_picker_new(smartData->self, ewk_popup_menu_items_get(ewk_menu), ewk_popup_menu_selected_index_get(ewk_menu), ewk_menu, &rect);
    return true;
}

Eina_Bool EwkView::handleEwkViewPopupMenuHide(Ewk_View_Smart_Data* smartData)
{
    EwkView* self = toEwkView(smartData);

    if (!self->popupPicker)
        return false;

    ewk_popup_picker_del(self->popupPicker);
    self->popupPicker = 0;

    return true;
}
#endif

#if ENABLE(TIZEN_INPUT_DATE_TIME)
void EwkView::showDateTimePicker(bool isTimeType, const char* value)
{
    m_inputPicker->show(isTimeType, value);
}
#endif

#if ENABLE(TOUCH_EVENTS)
void EwkView::feedTouchEvents(Ewk_Touch_Event_Type type, double timestamp)
{
    Ewk_View_Smart_Data* sd = smartData();

    unsigned length = evas_touch_point_list_count(sd->base.evas);
    if (!length)
        return;

#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
    if (type == EWK_TOUCH_START && length == 1) {
        m_isVerticalEdge = false;
        m_isHorizontalEdge = false;
    }
#endif

#if PLATFORM(TIZEN)
    if (type == EWK_TOUCH_START && length == 1)
        pageViewportController()->setHadUserInteraction(true);
#endif

    OwnArrayPtr<WKTypeRef> touchPoints = adoptArrayPtr(new WKTypeRef[length]);
    for (unsigned i = 0; i < length; ++i) {
        int x, y;
        evas_touch_point_list_nth_xy_get(sd->base.evas, i, &x, &y);
        IntPoint position(x, y);
        Evas_Touch_Point_State state = evas_touch_point_list_nth_state_get(sd->base.evas, i);
#if ENABLE(TIZEN_SUSPEND_TOUCH_CANCEL) || ENABLE(TIZEN_TOUCH_CANCEL)
        if (type == EWK_TOUCH_CANCEL)
            state = EVAS_TOUCH_POINT_CANCEL;
#endif
        int id = evas_touch_point_list_nth_id_get(sd->base.evas, i);
        touchPoints[i] = WKTouchPointCreate(id, toAPI(IntPoint(position)), toAPI(transformToScreen().mapPoint(position)), toWKTouchPointState(state), WKSizeMake(0, 0), 0, 1);
    }
    WKRetainPtr<WKArrayRef> wkTouchPoints(AdoptWK, WKArrayCreateAdoptingValues(touchPoints.get(), length));

    WKViewSendTouchEvent(wkView(), adoptWK(WKTouchEventCreate(static_cast<WKEventType>(type), wkTouchPoints.get(), toWKEventModifiers(evas_key_modifier_get(sd->base.evas)), timestamp)).get());
}

void EwkView::handleMouseDownForTouch(void*, Evas*, Evas_Object* ewkView, void* eventInfo)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_START, static_cast<Evas_Event_Mouse_Down*>(eventInfo)->timestamp / 1000.0);
}

void EwkView::handleMouseUpForTouch(void*, Evas*, Evas_Object* ewkView, void* eventInfo)
{
#if ENABLE(TIZEN_TOUCH_CANCEL)
    if (static_cast<Evas_Event_Mouse_Up*>(eventInfo)->event_flags == EVAS_EVENT_FLAG_ON_HOLD) {
        toEwkView(ewkView)->m_gestureRecognizer->reset();
        toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_CANCEL, static_cast<Evas_Event_Mouse_Up*>(eventInfo)->timestamp / 1000.0);
        return;
    }
#endif
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_END, static_cast<Evas_Event_Mouse_Up*>(eventInfo)->timestamp / 1000.0);
}

void EwkView::handleMouseMoveForTouch(void*, Evas*, Evas_Object* ewkView, void* eventInfo)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_MOVE, static_cast<Evas_Event_Mouse_Move*>(eventInfo)->timestamp / 1000.0);
}

void EwkView::handleMultiDownForTouch(void*, Evas*, Evas_Object* ewkView, void* eventInfo)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_START, static_cast<Evas_Event_Multi_Down*>(eventInfo)->timestamp / 1000.0);
}

void EwkView::handleMultiUpForTouch(void*, Evas*, Evas_Object* ewkView, void* eventInfo)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_END, static_cast<Evas_Event_Multi_Up*>(eventInfo)->timestamp / 1000.0);
}

void EwkView::handleMultiMoveForTouch(void*, Evas*, Evas_Object* ewkView, void* eventInfo)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_MOVE, static_cast<Evas_Event_Multi_Move*>(eventInfo)->timestamp / 1000.0);
}
#endif

void EwkView::handleFaviconChanged(const char* pageURL, void* eventInfo)
{
    EwkView* view = static_cast<EwkView*>(eventInfo);

    if (!view->url() || strcasecmp(view->url(), pageURL))
        return;

    view->smartCallback<FaviconChanged>().call();
}

PassRefPtr<cairo_surface_t> EwkView::takeSnapshot()
{
    // Suspend all animations before taking the snapshot.
    WKViewSuspendActiveDOMObjectsAndAnimations(wkView());

    // Wait for the pending repaint events to be processed.
    while (m_displayTimer.isActive())
        ecore_main_loop_iterate();

    Ewk_View_Smart_Data* sd = smartData();
#if USE(ACCELERATED_COMPOSITING)
    if (m_isAccelerated) {
        RefPtr<cairo_surface_t> snapshot = getImageSurfaceFromFrameBuffer(0, 0, sd->view.w, sd->view.h);
        // Resume all animations.
        WKViewResumeActiveDOMObjectsAndAnimations(wkView());

        return snapshot.release();
    }
#endif
    RefPtr<cairo_surface_t> snapshot = createSurfaceForImage(sd->image);
    // Resume all animations.
    WKViewResumeActiveDOMObjectsAndAnimations(wkView());

    return snapshot.release();
}

void EwkView::didFindZoomableArea(const WKPoint& point, const WKRect& area)
{
    if (!area.size.width || !area.size.height)
        return;

    FloatRect targetRect = toIntRect(area);
    targetRect.inflateX(s_marginForZoomableArea);
    scaleToAreaWithAnimation(toIntPoint(point), targetRect);
}

void EwkView::scaleToAreaWithAnimation(const IntPoint& target, const FloatRect& targetRect)
{
    if (!m_pageViewportController->allowsUserScaling())
        return;

    IntSize viewSize = size();
    WKPoint contentsPosition = WKViewGetContentPosition(wkView());
    float contentScale = WKViewGetContentScaleFactor(wkView());
    FloatRect currentContentsRect(contentsPosition.x, contentsPosition.y, viewSize.width() / contentScale, viewSize.height() / contentScale);

    float targetScale;
    if (fuzzyCompare(m_pageViewportController->minimumScale(), m_pageViewportController->currentScale(), 0.0001))
        targetScale = m_pageViewportController->innerBoundedViewportScale(viewSize.width() / targetRect.width());
    else
        targetScale = m_pageViewportController->minimumScale();
    FloatRect newContentsRect(targetRect.center(), FloatSize(viewSize.width() / targetScale, viewSize.height() / targetScale));
    if (targetRect.height() > newContentsRect.height())
        newContentsRect.setY(target.y());
    newContentsRect.move(-newContentsRect.width() / 2, -newContentsRect.height() / 2);
    newContentsRect.setLocation(m_pageViewportController->boundContentsPositionAtScale(newContentsRect.location(), targetScale));

    m_transition->run(currentContentsRect, newContentsRect);
}

#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
void EwkView::sendScrollSmartEvents(const IntSize& movedOffset, const IntSize& scrolledOffset)
{
    WKPoint position = WKViewGetContentPosition(wkView());
    WKSize contentsSize = WKViewGetContentsSize(wkView());
    float scale = WKViewGetContentScaleFactor(wkView());
    Ewk_View_Smart_Data* sd = smartData();

    // Send scroll events if the page is scrolled and send edge events
    // if the page reaches to the edge.
    if (scrolledOffset.width()) {
        int velocity = abs(movedOffset.width() / ecore_animator_frametime_get());
        if (movedOffset.width() > 0)
            evas_object_smart_callback_call(m_evasObject, "scroll,right", &velocity);
        else
            evas_object_smart_callback_call(m_evasObject, "scroll,left", &velocity);
        m_isHorizontalEdge = false;
    } else if (!m_isHorizontalEdge) {
        if (position.x == 0 && movedOffset.width() < 0) {
            TIZEN_LOGI("[Touch] edge,left scrolledOffset[%d]", scrolledOffset.width());
            evas_object_smart_callback_call(m_evasObject, "edge,left", 0);
            m_isHorizontalEdge = true;
        } else if (position.x >= contentsSize.width - (sd->view.w / scale / deviceScaleFactor()) && movedOffset.width() > 0) {
            TIZEN_LOGI("[Touch] edge,right scrolledOffset[%d]", scrolledOffset.width());
            evas_object_smart_callback_call(m_evasObject, "edge,right", 0);
            m_isHorizontalEdge = true;
        }
    }

    if (scrolledOffset.height()) {
        int velocity = abs(movedOffset.height() / ecore_animator_frametime_get());
        if (movedOffset.height() > 0)
            evas_object_smart_callback_call(m_evasObject, "scroll,down", &velocity);
        else
            evas_object_smart_callback_call(m_evasObject, "scroll,up", &velocity);
        m_isVerticalEdge = false;
    } else if (!m_isVerticalEdge) {
        if (position.y == 0 && movedOffset.height() < 0) {
            TIZEN_LOGI("[Touch] edge,top scrolledOffset[%d]", scrolledOffset.height());
            evas_object_smart_callback_call(m_evasObject, "edge,top", 0);
            m_isVerticalEdge = true;
        } else if (position.y >= contentsSize.height - (sd->view.h / scale / deviceScaleFactor()) && movedOffset.height() > 0) {
            TIZEN_LOGI("[Touch] edge,bottom scrolledOffset[%d]", scrolledOffset.height());
            evas_object_smart_callback_call(m_evasObject, "edge,bottom", 0);
            m_isVerticalEdge = true;
        }
    }
}
#endif

bool EwkView::scrollBy(const IntSize& offset)
{
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    if (hasScrollableArea()) {
        WKSize scrolledSize = WKViewScrollScrollableArea(wkView(), toAPI(offset));
        if (scrolledSize.width || scrolledSize.height) {
#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
            m_isHorizontalEdge = false;
            m_isVerticalEdge = false;
#endif
            return true;
        }
    }
#endif
    WKPoint oldPosition = WKViewGetContentPosition(wkView());
    float contentScale = WKViewGetContentScaleFactor(wkView());

    float effectiveScale = contentScale * deviceScaleFactor();
    FloatPoint newPosition(oldPosition.x + offset.width() / effectiveScale, oldPosition.y + offset.height() / effectiveScale);

    // Update new position to the PageViewportController.
    newPosition = m_pageViewportController->boundContentsPositionAtScale(newPosition, contentScale);
    m_pageViewportController->didChangeContentsVisibility(newPosition, contentScale, FloatPoint(offset.width(),offset.height()));

    // Update new position to the WKView.
    WKPoint position = WKPointMake(newPosition.x(), newPosition.y());
    WKViewSetContentPosition(wkView(), position);
#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
    sendScrollSmartEvents(offset, IntSize(ceil(position.x - oldPosition.x), ceil(position.y - oldPosition.y)));
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    double scrolledSizeX = position.x - oldPosition.x < 0 ? floor(position.x - oldPosition.x) : ceil(position.x - oldPosition.x);
    double scrolledSizeY = position.y - oldPosition.y < 0 ? floor(position.y - oldPosition.y) : ceil(position.y - oldPosition.y);
    WKViewUpdateEdgeEffect(wkView(), WKSizeMake(offset.width(), offset.height()), WKSizeMake(scrolledSizeX, scrolledSizeY));
#endif

    // If the page position has not changed, notify the caller using the return value.
#if ENABLE(TIZEN_SCROLLBAR)
    bool positionChanged = !(oldPosition == position);

    if (WKPageUseFixedLayout(wkPage()) && positionChanged)
        updateScrollbar();

    return positionChanged;
#else
    return !(oldPosition == position);
#endif // ENABLE(TIZEN_SCROLLBAR)
}

void EwkView::scaleTo(float newScale, const FloatPoint& newPosition)
{
    WKViewSetContentPosition(wkView(), WKPointMake(newPosition.x(), newPosition.y()));
    WKViewSetContentScaleFactor(wkView(), newScale);
    m_pageViewportController->applyPositionAfterRenderingContents(newPosition);
    m_pageViewportController->applyScaleAfterRenderingContents(newScale);
}

#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
void EwkView::scrollTo(const FloatPoint& position)
{
    // Update new position to the PageViewportController.
    float contentScale = WKViewGetContentScaleFactor(wkView());
    FloatPoint newPosition = m_pageViewportController->boundContentsPositionAtScale(position, contentScale);
    m_pageViewportController->didChangeContentsVisibility(newPosition, contentScale);

    // Update new position to the WKView.
    WKPoint wkPosition = WKPointMake(newPosition.x(), newPosition.y());
    WKViewSetContentPosition(wkView(), wkPosition);
}
#endif // #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
void EwkView::findScrollableArea(const IntPoint& point)
{
    WKViewFindScrollableArea(wkView(), toAPI(point), &m_hasVerticalScrollableArea, &m_hasHorizontalScrollableArea);
    TIZEN_LOGI("[Touch] scrollable[%d, %d]", m_hasVerticalScrollableArea, m_hasHorizontalScrollableArea);
}
#endif // ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)

#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
void EwkView::scrollFinished()
{
    WKViewScrollFinished(wkView());
}
#endif // #if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)

#if ENABLE(TIZEN_SUSPEND_TOUCH_CANCEL)
void EwkView::feedTouchCancelEvent()
{
    feedTouchEvents(EWK_TOUCH_CANCEL, ecore_loop_time_get());
}
#endif

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
bool EwkView::lockOrientation(int willLockOrientation)
{
    Ewk_View_Smart_Data* sd = smartData();
    if (sd->api->orientation_lock)
        return sd->api->orientation_lock(sd, willLockOrientation);

    return false;
}

void EwkView::unlockOrientation()
{
    Ewk_View_Smart_Data* sd = smartData();
    if (sd->api->orientation_unlock)
        sd->api->orientation_unlock(sd);
}
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
bool EwkView::getStandaloneStatus()
{
    //TIZEN_LOGI("webapp,metatag,standalone");
    bool standalone = true;
    smartCallback<WebAppStandalone>().call(&standalone);
    return standalone;
}
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void EwkView::saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName)
{
    m_offlinePageSave->saveSerializedHTMLDataForMainPage(serializedData, fileName);
}

void EwkView::saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData)
{
    m_offlinePageSave->saveSubresourcesData(subresourceData);
}

void EwkView::startOfflinePageSave(String& path, String& url, String& title)
{
    m_offlinePageSave->startOfflinePageSave(path, url, title);
}
#endif

#if ENABLE(TIZEN_FULLSCREEN_API)
Eina_Bool EwkView::handleEwkViewFullscreenEnter(Ewk_View_Smart_Data* smartData, Ewk_Security_Origin* /* origin */)
{
    EwkView* self = toEwkView(smartData);

    ewk_view_main_frame_scrollbar_visible_set(self->evasObject(), true);
    self->smartCallback<FullscreenEnter>().call();

    return true;
}

Eina_Bool EwkView::handleEwkViewFullscreenExit(Ewk_View_Smart_Data* smartData)
{
    EwkView* self = toEwkView(smartData);

    ewk_view_main_frame_scrollbar_visible_set(self->evasObject(), true);
    self->smartCallback<FullscreenExit>().call();

    return true;
}
#endif // ENABLE(TIZEN_FULLSCREEN_API)

#if ENABLE(TIZEN_TEXT_SELECTION)
bool EwkView::isTextSelectionDowned()
{
    return textSelection()->isTextSelectionDowned();
}

bool EwkView::isTextSelectionMode()
{
    return textSelection()->isTextSelectionMode();
}

void EwkView::setIsTextSelectionMode(bool isTextSelectionMode)
{
    textSelection()->setIsTextSelectionMode(isTextSelectionMode);
}

void EwkView::updateTextSelectionHandlesAndContextMenu(bool isShow, bool isScrolling)
{
    if (textSelection()->isTextSelectionMode() && evas_object_focus_get(evasObject()))
        textSelection()->updateHandlesAndContextMenu(isShow, isScrolling);
}

bool EwkView::textSelectionDown(const WebCore::IntPoint& point)
{
    if (!evas_object_focus_get(evasObject())) {
        // FixMe: This code should be uncommented if TIZEN_ISF_PORT is enabled.
        //if (InputMethodContextEfl* inputMethodContext = inputMethodContext())
        //    inputMethodContext->hideIMFContext();

        evas_object_focus_set(evasObject(), true);
    }

    return textSelection()->textSelectionDown(point);
}

void EwkView::textSelectionMove(const WebCore::IntPoint& point)
{
    textSelection()->textSelectionMove(point);
}

void EwkView::textSelectionUp(const WebCore::IntPoint& point, bool isStartedTextSelectionFromOutside)
{
    textSelection()->textSelectionUp(point, isStartedTextSelectionFromOutside);
}

bool EwkView::isTextSelectionHandleDowned()
{
    return textSelection()->isTextSelectionHandleDowned();
}

void EwkView::requestToShowTextSelectionHandlesAndContextMenu()
{
    textSelection()->requestToShow();
}

void EwkView::initTextSelectionHandlesMouseDownedStatus()
{
    textSelection()->initHandlesMouseDownedStatus();
}

void EwkView::changeContextMenuPosition(IntPoint& point)
{
    textSelection()->changeContextMenuPosition(point);
}
#endif // ENABLE(TIZEN_TEXT_SELECTION)

#if ENABLE(TIZEN_DRAG_SUPPORT)
void EwkView::setDragPoint(const WebCore::IntPoint& point)
{
    m_drag->setDragPoint(point);
}
bool EwkView::isDragMode()
{
    return m_drag->isDragMode();
}

void EwkView::setDragMode(bool isDragMode)
{
    m_drag->setDragMode(isDragMode);
}

void EwkView::startDrag(const DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    DragData* dragInfo = new DragData(dragData.platformData(), m_drag->getDragPoint(),
        m_drag->getDragPoint(), dragData.draggingSourceOperationMask(), dragData.flags());

    String dragStorageName("Drag");
    page()->dragEntered(dragInfo, dragStorageName);
    setDragMode(true);
    m_drag->setDragData(dragInfo, dragImage);
    m_drag->show();
}
#endif // #if ENABLE(TIZEN_DRAG_SUPPORT)

#if ENABLE(TIZEN_DRAG_SUPPORT) || ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
AffineTransform EwkView::transformFromScene() const
{
    AffineTransform transform;

    Ewk_View_Smart_Data* sd = smartData();
    transform.translate(-sd->view.x, -sd->view.y);

    return transform;
}

AffineTransform EwkView::transformToScene() const
{
    return transformFromScene().inverse();
}
#endif // #if ENABLE(TIZEN_DRAG_SUPPORT) || ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

#if ENABLE(TIZEN_USER_AGENT)
void EwkView::setApplicationName(const char* applicationName)
{
    if (m_applicationName == applicationName)
        return;

    WKRetainPtr<WKStringRef> applicationNameString(AdoptWK, WKStringCreateWithUTF8CString(applicationName));
    WKPageSetApplicationNameForUserAgent(wkPage(), applicationNameString.get());
    m_applicationName = WKEinaSharedString(AdoptWK, WKPageCopyApplicationNameForUserAgent(wkPage()));
}
#endif // #if ENABLE(TIZEN_USER_AGENT)

#if PLATFORM(TIZEN)
bool EwkView::makeContextCurrent()
{
    if (m_evasGLSurface && m_evasGLContext)
        return evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context());

    return false;
}
#endif

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
void EwkView::callFrameRenderedCallBack()
{
    if (m_didFirstFrameReady && m_didSetVisibleContentsRect) {
        TIZEN_LOGI("frame,rendered");
        evas_object_smart_callback_call(m_evasObject, "frame,rendered", 0);
        m_didFirstFrameReady = false;
        m_didSetVisibleContentsRect = false;
    }
}
#endif

#if ENABLE(TIZEN_POINTER_LOCK)
bool EwkView::lockMouse()
{
    if (m_mouseLocked)
        return true;

    Evas* evas = evas_object_evas_get(evasObject());
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(evas);

    Ecore_X_Window window;
#if USE(ACCELERATED_COMPOSITING)
    window = ecore_evas_gl_x11_window_get(ecoreEvas);
    // Fallback to software mode if necessary.
    if (!window)
#endif
    window = ecore_evas_software_x11_window_get(ecoreEvas);

    ecore_x_pointer_grab(window);

    m_mouseLocked = true;
    m_lockingMouse = true;

    return true;
}

void EwkView::unlockMouse()
{
    if (!m_mouseLocked)
        return;

    ecore_x_pointer_ungrab();

    m_mouseLocked = false;
    m_unlockingMouse = true;
}
#endif

#if ENABLE(TIZEN_SCROLLBAR)
void EwkView::updateScrollbar()
{
    bool useFixedLayout = WKPageUseFixedLayout(wkPage());

    IntPoint scaledScrollPosition;
    IntSize scaledContentsSize = webView()->contentsSize();

    if (useFixedLayout) {
        scaledScrollPosition = toIntPoint(WKViewGetContentPosition(wkView()));

        float scaleFactor = WKViewGetContentScaleFactor(wkView());
        scaledContentsSize.scale(scaleFactor);
        scaledScrollPosition.scale(scaleFactor, scaleFactor);
    } else
        scaledScrollPosition = toIntPoint(WKPageGetScrollPosition(wkPage()));

    if (size().width() < scaledContentsSize.width()) {
        if (!m_horizontalScrollbar)
            m_horizontalScrollbar = MainFrameScrollbarTizen::createNativeScrollbar(evasObject(), HorizontalScrollbar);

        m_horizontalScrollbar->setEnabled(WKPageHasHorizontalScrollbar(wkPage()) && m_mainFrameScrollbarVisibility);
        m_horizontalScrollbar->setFrameRect(IntRect(0, deviceSize().height(), deviceSize().width(), 0));
        m_horizontalScrollbar->setProportion(size().width(), scaledContentsSize.width());
        m_horizontalScrollbar->setPosition(scaledScrollPosition.x());
    } else if (m_horizontalScrollbar)
        m_horizontalScrollbar->setEnabled(false);

    if (size().height() < scaledContentsSize.height()) {
        if (!m_verticalScrollbar)
            m_verticalScrollbar = MainFrameScrollbarTizen::createNativeScrollbar(evasObject(), VerticalScrollbar);

        m_verticalScrollbar->setEnabled(WKPageHasVerticalScrollbar(wkPage()) && m_mainFrameScrollbarVisibility);
        m_verticalScrollbar->setFrameRect(IntRect(deviceSize().width(), 0, 0, deviceSize().height()));
        m_verticalScrollbar->setProportion(size().height(), scaledContentsSize.height());
        m_verticalScrollbar->setPosition(scaledScrollPosition.y());
    } else if (m_verticalScrollbar)
        m_verticalScrollbar->setEnabled(false);
}

void EwkView::frameRectChanged()
{
    if (m_horizontalScrollbar)
        m_horizontalScrollbar->setFrameRect(IntRect(0, deviceSize().height(), deviceSize().width(), 0));
    if (m_verticalScrollbar)
        m_verticalScrollbar->setFrameRect(IntRect(deviceSize().width(), 0, 0, deviceSize().height()));
}

void EwkView::setMainFrameScrollbarVisibility(bool visible)
{
    bool useFixedLayout = WKPageUseFixedLayout(wkPage());

    if (m_mainFrameScrollbarVisibility != visible) {
        m_mainFrameScrollbarVisibility = visible;
        if (m_horizontalScrollbar)
            m_horizontalScrollbar->setEnabled((WKPageHasHorizontalScrollbar(wkPage()) != useFixedLayout) && visible);
        if (m_verticalScrollbar)
            m_verticalScrollbar->setEnabled((WKPageHasVerticalScrollbar(wkPage()) != useFixedLayout) && visible);
    }
}

bool EwkView::getMainFrameScrollbarVisibility()
{
    return m_mainFrameScrollbarVisibility;
}
#endif // ENABLE(TIZEN_SCROLLBAR)

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
void EwkView::addMessageToConsole(unsigned level, const String& message, unsigned lineNumber, const String& source)
{
    Ewk_Console_Message ewkConsoleMessage(level, message, lineNumber, source);
    smartCallback<ConsoleMessage>().call(&ewkConsoleMessage);
}
#endif

#if ENABLE(TIZEN_GEOLOCATION)
void EwkView::addPendingCallbackArgument(PassRefPtr<EwkObject> callback)
{
    m_pendingCallbackArguments.add(callback);
}
#endif // ENABLE(TIZEN_GEOLOCATION)

void EwkView::setBackgroundColor(int red, int green, int blue, int alpha)
{
    if (red == 255 && green == 255 && blue == 255 && alpha == 255)
        WKViewSetDrawsBackground(wkView(), true);
    else
        WKViewSetDrawsBackground(wkView(), false);

    int objectAlpha;
    Evas_Object* image = smartData()->image;
    evas_object_color_get(image, nullptr, nullptr, nullptr, &objectAlpha);
    evas_object_image_alpha_set(image, alpha < 255 || objectAlpha < 255);

    WKViewSetBackgroundColor(wkView(), red, green, blue, alpha);
}

#if ENABLE(TIZEN_SCROLL_SNAP)
IntSize EwkView::findScrollSnapOffset(const IntSize& offset)
{
    return toIntSize(WKViewFindScrollSnapOffset(wkView(), toAPI(offset)));
}
#endif

Evas_Smart_Class EwkView::parentSmartClass = EVAS_SMART_CLASS_INIT_NULL;

