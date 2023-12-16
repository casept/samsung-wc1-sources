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

#ifndef EwkView_h
#define EwkView_h

#include "EvasGLContext.h"
#include "EvasGLSurface.h"
#include "EwkViewCallbacks.h"
#include "ImmutableDictionary.h"
#include "RefPtrEfl.h"
#include "WKEinaSharedString.h"
#include "WKRetainPtr.h"
#include "WebViewEfl.h"
#include "ewk_url_request_private.h"
#include <Ecore_X.h>
#include <Evas.h>
#include <WebCore/FloatPoint.h>
#include <WebCore/IntRect.h>
#include <WebCore/RefPtrCairo.h>
#include <WebCore/TextDirection.h>
#include <WebCore/Timer.h>
#include <WebKit2/WKBase.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TOUCH_EVENTS)
#include "GestureRecognizer.h"
#include "ewk_touch.h"
#endif

#include "WebContext.h"
#include "WebPageGroup.h"
#include "WebPreferences.h"

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
#include "ewk_policy_decision.h"
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "OfflinePageSave.h"
#include "WebSubresourceTizen.h"
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
#include "TextSelection.h"
#endif

#if ENABLE(TIZEN_POPUP_INTERNAL)
#include "ewk_popup_picker.h"
#endif

#if PLATFORM(TIZEN)
#include "ewk_security_origin_private.h"
#include "JavaScriptPopup.h"
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "Drag.h"
#include "ShareableBitmap.h"
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if ENABLE(TIZEN_INPUT_DATE_TIME)
#include "InputPicker.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include "FocusRing.h"
#endif

typedef struct _cairo_surface cairo_surface_t;

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
typedef struct _Ewk_Certificate_Policy_Decision Ewk_Certificate_Policy_Decision;
#endif

#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
typedef struct widget_resource_lock_info *widget_resource_lock_t;
#endif

namespace WebKit {
class ContextMenuClientEfl;
class FindClientEfl;
class FormClientEfl;
class InputMethodContextEfl;
class PageLoadClientEfl;
class PagePolicyClientEfl;
class PageUIClientEfl;
class ViewClientEfl;
#if USE(ACCELERATED_COMPOSITING)
class PageViewportController;
class PageViewportControllerClientEfl;
#endif
class WebPageGroup;
class WebPageProxy;

#if ENABLE(VIBRATION)
class VibrationClientEfl;
#endif
}

namespace WebCore {
class AffineTransform;
class Color;
class CoordinatedGraphicsScene;
class Cursor;
class Image;
class IntSize;
class TransformationMatrix;
}

class EwkContext;
class EwkBackForwardList;
class EwkColorPicker;
class EwkContextMenu;
class EwkPageGroup;
class EwkPopupMenu;
class EwkSettings;
class EwkWindowFeatures;

#if USE(ACCELERATED_COMPOSITING)
typedef struct _Evas_GL_Context Evas_GL_Context;
typedef struct _Evas_GL_Surface Evas_GL_Surface;
#endif

typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
typedef struct Ewk_View_Smart_Class Ewk_View_Smart_Class;

#if ENABLE(TIZEN_SUPPORT)
namespace WebCore {
class AffineTransform;
}

// FIXME: we have to include ewk_view.h instead of typedef,
// because there are "circular include" in the local code unlike open source,
// so we can not do typedef again here.
//typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
#include "ewk_view.h"

struct Ewk_View_Callback_Context {
    union {
        Ewk_Web_App_Capable_Get_Callback webAppCapableCallback;
        Ewk_Web_App_Icon_URL_Get_Callback webAppIconURLCallback;
        Ewk_Web_App_Icon_URLs_Get_Callback webAppIconURLsCallback;
#if ENABLE(TIZEN_WEB_STORAGE)
        Ewk_Web_Storage_Quota_Get_Callback webStorageQuotaCallback;
#endif // ENABLE(TIZEN_WEB_STORAGE)
#if ENABLE(TIZEN_SQL_DATABASE)
        Ewk_View_Exceeded_Database_Quota_Callback exceededDatabaseQuotaCallback;
#endif // ENABLE(TIZEN_SQL_DATABASE)
#if ENABLE(TIZEN_INDEXED_DATABASE)
        Ewk_View_Exceeded_Indexed_Database_Quota_Callback exceededIndexedDatabaseQuotaCallback;
#endif // ENABLE(TIZEN_INDEXED_DATABASE)
#if PLATFORM(TIZEN)
        Ewk_View_JavaScript_Alert_Callback javascriptAlertCallback;
        Ewk_View_JavaScript_Confirm_Callback javascriptConfirmCallback;
        Ewk_View_JavaScript_Prompt_Callback javascriptPromptCallback;
#endif // PLATFORM(TIZEN)
   };

    Evas_Object* ewkView;
    void* userData;
};
typedef struct Ewk_View_Callback_Context Ewk_View_Callback_Context;
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
namespace WebKit {
class OfflinePageSave;
class WebSubresourceTizen;
}
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
namespace WebKit {
class TextSelection;
}
#endif

#if ENABLE(TIZEN_SCROLLBAR)
namespace WebKit {
class MainFrameScrollbarTizen;
}
#endif

class Transition;

class EwkView {
public:
    static EwkView* create(WKViewRef, Evas* canvas, Evas_Smart* smart = 0);

    static bool initSmartClassInterface(Ewk_View_Smart_Class&);

    static Evas_Object* toEvasObject(WKPageRef);

    Evas_Object* evasObject() { return m_evasObject; }

    WKViewRef wkView() const { return m_webView.get(); }
    WKPageRef wkPage() const;

    WebKit::WebPageProxy* page() { return webView()->page(); }
    EwkContext* ewkContext() { return m_context.get(); }
    EwkPageGroup* ewkPageGroup() { return m_pageGroup.get(); }
    EwkSettings* settings() { return m_settings.get(); }
    EwkBackForwardList* backForwardList() { return m_backForwardList.get(); }
    EwkWindowFeatures* windowFeatures();

#if USE(ACCELERATED_COMPOSITING)
    WebKit::PageViewportController* pageViewportController() { return m_pageViewportController.get(); }
#endif

    void setDeviceScaleFactor(float scale);
    float deviceScaleFactor() const;

    WebCore::AffineTransform transformToScreen() const;

    const char* url() const { return m_url; }
    Evas_Object* createFavicon() const;
    const char* title() const;
    WebKit::InputMethodContextEfl* inputMethodContext();

    const char* themePath() const;
    void setThemePath(const char* theme);
    const char* customTextEncodingName() const { return m_customEncoding; }
    void setCustomTextEncodingName(const char* customEncoding);
    const char* userAgent() const { return m_userAgent; }
    void setUserAgent(const char* userAgent);

    bool mouseEventsEnabled() const { return m_mouseEventsEnabled; }
    void setMouseEventsEnabled(bool enabled);
#if ENABLE(TOUCH_EVENTS)
    void feedTouchEvent(Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers);
    bool touchEventsEnabled() const { return m_touchEventsEnabled; }
    void setTouchEventsEnabled(bool enabled);
    void doneWithTouchEvent(WKTouchEventRef, bool);
#endif

#if !ENABLE(TIZEN_WEARABLE_PROFILE)
    void updateCursor(bool hadCustomCursor);
    void setCursor(const WebCore::Cursor& cursor);
#endif

#if ENABLE(TIZEN_INPUT_DATE_TIME)
    void showDateTimePicker(bool isTimeType, const char* value);
#endif

#if ENABLE(TIZEN_DIRECT_RENDERING)
    void composite();
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    bool getAppNeedEffect();
    void compositeWithEffect();
    void setAppNeedEffect(Evas_Object *, Eina_Bool);
#endif
#endif
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    bool useProvidedPixmap();
    void startWidgetDisplay();
#endif

    void scheduleUpdateDisplay();

#if ENABLE(FULLSCREEN_API)
    void enterFullScreen();
    void exitFullScreen();
#endif

    WKRect windowGeometry() const;
    void setWindowGeometry(const WKRect&);
#if USE(ACCELERATED_COMPOSITING)
    bool createGLSurface();
    void setNeedsSurfaceResize();
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    void requestColorPicker(WKColorPickerResultListenerRef listener, const WebCore::Color&);
    void dismissColorPicker();
#endif

    WKPageRef createNewPage(PassRefPtr<EwkUrlRequest>, WKDictionaryRef windowFeatures);
    void close();

    void requestPopupMenu(WKPopupMenuListenerRef, const WKRect&, WKPopupItemTextDirection, double pageScaleFactor, WKArrayRef items, int32_t selectedIndex);
    void closePopupMenu();

#if ENABLE(TIZEN_CALL_SIGNAL_FOR_WRT)
    void getContextMenuFromProposedMenu();
#endif
    void customContextMenuItemSelected(WKContextMenuItemRef contextMenuItem);
    void showContextMenu(WKPoint position, WKArrayRef items);
    void hideContextMenu();

    void updateTextInputState();

    void requestJSAlertPopup(const WKEinaSharedString& message);
    bool requestJSConfirmPopup(const WKEinaSharedString& message);
    WKEinaSharedString requestJSPromptPopup(const WKEinaSharedString& message, const WKEinaSharedString& defaultValue);

    template<EwkViewCallbacks::CallbackType callbackType>
    EwkViewCallbacks::CallBack<callbackType> smartCallback() const
    {
        return EwkViewCallbacks::CallBack<callbackType>(m_evasObject);
    }

    unsigned long long informDatabaseQuotaReached(const String& databaseName, const String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage);

    // FIXME: Remove when possible.
    WebKit::WebView* webView();

    // FIXME: needs refactoring (split callback invoke)
    void informURLChange();

    PassRefPtr<cairo_surface_t> takeSnapshot();
#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
    void sendScrollSmartEvents(const WebCore::IntSize&, const WebCore::IntSize&);
#endif
    bool scrollBy(const WebCore::IntSize&);
    void scaleTo(float, const WebCore::FloatPoint&);
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    void scrollTo(const WebCore::FloatPoint&);
#endif // #if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    void findScrollableArea(const WebCore::IntPoint&);
    bool hasScrollableArea() { return m_hasVerticalScrollableArea || m_hasHorizontalScrollableArea; }
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA) || ENABLE(TIZEN_EDGE_EFFECT)
    void scrollFinished();
#endif
#if ENABLE(TIZEN_SUSPEND_TOUCH_CANCEL)
    void feedTouchCancelEvent();
#endif

    void setBackgroundColor(int red, int green, int blue, int alpha);

    void didFindZoomableArea(const WKPoint&, const WKRect&);

#if ENABLE(TOUCH_EVENTS)
    void didChangeViewportAttributes();
#endif

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
    bool lockOrientation(int willLockOrientation);
    void unlockOrientation();
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    bool getStandaloneStatus();

    WKEinaSharedString getWebAppIconURL() { return m_webAppIconURL; }
    void setWebAppIconURL(WKEinaSharedString url) { m_webAppIconURL = url; }
    Eina_List* getWebAppIconURLs() { return m_webAppIconURLs; }
    void setWebAppIconURLs(Eina_List* list) { m_webAppIconURLs = list; }
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    FocusRing* focusRing() { return m_focusRing.get(); }
#endif

#if ENABLE(TIZEN_POINTER_LOCK)
    bool lockMouse();
    void unlockMouse();
    bool isMouseLocked() const { return m_mouseLocked; }
    bool isMouseUnlocked() const { return m_unlockingMouse; }
#endif

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    bool shouldHandleKeyDownEvent(const Evas_Event_Key_Down*);
    bool shouldHandleKeyUpEvent(const Evas_Event_Key_Up*);
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    void addPendingCallbackArgument(PassRefPtr<EwkObject>);
#endif

    static const char smartClassName[];

private:
    WKRetainPtr<WKViewRef> m_webView;

public:
#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
    Ewk_Policy_Decision* policyDecision;
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    void saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName);
    void saveSubresourcesData(Vector<WebKit::WebSubresourceTizen>& subresourceData);
    void startOfflinePageSave(String& path, String& url, String& title);
#endif

#if ENABLE(TIZEN_TEXT_SELECTION)
    bool isTextSelectionDowned();
    bool isTextSelectionMode();
    void setIsTextSelectionMode(bool isTextSelectionMode);
    void updateTextSelectionHandlesAndContextMenu(bool isShow, bool isScrolling = false);
    bool textSelectionDown(const WebCore::IntPoint& point);
    void textSelectionMove(const WebCore::IntPoint& point);
    void textSelectionUp(const WebCore::IntPoint& point, bool isStartedTextSelectionFromOutside = false);
    bool isTextSelectionHandleDowned();
    void requestToShowTextSelectionHandlesAndContextMenu();
    void initTextSelectionHandlesMouseDownedStatus();
    void changeContextMenuPosition(WebCore::IntPoint& point);
    WebKit::TextSelection* textSelection() { return m_textSelection.get(); }
#endif

#if PLATFORM(TIZEN)
    WebCore::IntRect visibleContentRect() { return m_visibleContentRect; }
#endif

#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
    void didFirstFrameReady() { m_didFirstFrameReady = true; }
    void didSetVisibleContentsRect() { m_didSetVisibleContentsRect = true; }
    void callFrameRenderedCallBack();
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    void setDragPoint(const WebCore::IntPoint& point);
    bool isDragMode();
    void setDragMode(bool isDragMode);
    void startDrag(const WebCore::DragData&, PassRefPtr<WebKit::ShareableBitmap> dragImage);
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT) || ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    WebCore::AffineTransform transformFromScene() const;
    WebCore::AffineTransform transformToScene() const;
#endif

#if ENABLE(TIZEN_USER_AGENT)
    void setApplicationName(const char*);
    const char* applicationName() const { return m_applicationName; }
#endif // #if ENABLE(TIZEN_USER_AGENT)

#if PLATFORM(TIZEN)
    RefPtr<EwkSecurityOrigin> exceededQuotaOrigin;
    bool isWaitingForExceededQuotaPopupReply;
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_NOTIFICATIONS)
    Eina_List* notifications;
    Eina_List* notificationPermissionRequests;
#endif // ENABLE(TIZEN_NOTIFICATIONS)

#if ENABLE(TIZEN_MEDIA_STREAM)
    Eina_List* userMediaPermissionRequests;
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_TEXT_SELECTION)
    OwnPtr<WebKit::TextSelection> m_textSelection;
#endif

#if ENABLE(TIZEN_POPUP_INTERNAL)
    Ewk_Popup_Picker* popupPicker;
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
    OwnPtr<Ewk_View_Callback_Context> exceededDatabaseQuotaContext;
#endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_INDEXED_DATABASE)
    OwnPtr<Ewk_View_Callback_Context> exceededIndexedDatabaseQuotaContext;
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if PLATFORM(TIZEN)
    bool isWaitingForJavaScriptPopupReply;
    OwnPtr<WebKit::JavaScriptPopup> javascriptPopup;

#if ENABLE(TIZEN_INPUT_DATE_TIME)
    OwnPtr<WebKit::InputPicker> m_inputPicker;
#endif
    OwnPtr<Ewk_View_Callback_Context> alertContext;
    OwnPtr<Ewk_View_Callback_Context> confirmContext;
    OwnPtr<Ewk_View_Callback_Context> promptContext;
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Ewk_Certificate_Policy_Decision* certificatePolicyDecision;
#endif

#if PLATFORM(TIZEN)
    bool makeContextCurrent();
#endif
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    bool useGLBackend() { return m_isAccelerated; }
#endif

#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    void setScheduleUpdateDisplayWithAnimator(bool);
    static Eina_Bool scheduleUpdateDisplayAnimatorCallback(void*);
#endif

#if ENABLE(TIZEN_VD_WHEEL_FLICK)
    static Eina_Bool remoteControlWheelFlickAnimatorCallback(void*);
    void handleRemoteControlWheelFlick(const WebCore::IntSize&);
#endif

#if ENABLE(TIZEN_SCROLLBAR)
    void setMainFrameScrollbarVisibility(bool);
    bool getMainFrameScrollbarVisibility();

    void updateScrollbar();
    void frameRectChanged();
#endif // ENABLE(TIZEN_SCROLLBAR)

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
    void addMessageToConsole(unsigned level, const String& message, unsigned lineNumber, const String& source);
#endif

#if ENABLE(TIZEN_CHECK_MODAL_POPUP)
    void setOpenModalPopup(bool isOpened) { m_isOpenModalPopup = isOpened; }
    bool isOpenModalPopup() { return m_isOpenModalPopup; }
#endif

#if ENABLE(TIZEN_SCROLL_SNAP)
    WebCore::IntSize findScrollSnapOffset(const WebCore::IntSize&);
#endif

private:
    EwkView(WKViewRef, Evas_Object*);
    ~EwkView();

    void setDeviceSize(const WebCore::IntSize&);
    Ewk_View_Smart_Data* smartData() const;

    WebCore::IntSize size() const;
    WebCore::IntSize deviceSize() const;

    void displayTimerFired(WebCore::Timer<EwkView>*);

#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    void effectImageTimerFired(WebCore::Timer<EwkView>*);
#endif

    void scaleToAreaWithAnimation(const WebCore::IntPoint&, const WebCore::FloatRect&);

#if ENABLE(TIZEN_HANDLE_MOUSE_WHEEL_IN_THE_UI_SIDE)
    void handleMouseWheelInTheUISide(const Evas_Event_Mouse_Wheel*);
#endif

    // Evas_Smart_Class callback interface:
    static void handleEvasObjectAdd(Evas_Object*);
    static void handleEvasObjectDelete(Evas_Object*);
    static void handleEvasObjectMove(Evas_Object*, Evas_Coord x, Evas_Coord y);
    static void handleEvasObjectResize(Evas_Object*, Evas_Coord width, Evas_Coord height);
    static void handleEvasObjectShow(Evas_Object*);
    static void handleEvasObjectHide(Evas_Object*);
    static void handleEvasObjectColorSet(Evas_Object*, int red, int green, int blue, int alpha);
    static void handleEvasObjectCalculate(Evas_Object*);

    // Ewk_View_Smart_Class callback interface:
    static Eina_Bool handleEwkViewFocusIn(Ewk_View_Smart_Data* smartData);
    static Eina_Bool handleEwkViewFocusOut(Ewk_View_Smart_Data* smartData);
    static Eina_Bool handleEwkViewMouseWheel(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Wheel* wheelEvent);
    static Eina_Bool handleEwkViewMouseDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Down* downEvent);
    static Eina_Bool handleEwkViewMouseUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Up* upEvent);
    static Eina_Bool handleEwkViewMouseMove(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Move* moveEvent);
    static Eina_Bool handleEwkViewKeyDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Down* downEvent);
    static Eina_Bool handleEwkViewKeyUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Up* upEvent);

#if ENABLE(TOUCH_EVENTS)
    void feedTouchEvents(Ewk_Touch_Event_Type type, double timestamp);
    static void handleMouseDownForTouch(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleMouseUpForTouch(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleMouseMoveForTouch(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleMultiDownForTouch(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleMultiUpForTouch(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleMultiMoveForTouch(void* data, Evas*, Evas_Object*, void* eventInfo);
#endif
    static void handleFaviconChanged(const char* pageURL, void* eventInfo);

#if ENABLE(TIZEN_POPUP_INTERNAL)
    static Eina_Bool handleEwkViewPopupMenuShow(Ewk_View_Smart_Data* smartData, Eina_Rectangle rect, Ewk_Text_Direction text_direction, double page_scale_factor,  Ewk_Popup_Menu *ewk_menu);
    static Eina_Bool handleEwkViewPopupMenuHide(Ewk_View_Smart_Data* smartData);
#endif

#if ENABLE(TIZEN_FULLSCREEN_API)
    static Eina_Bool handleEwkViewFullscreenEnter(Ewk_View_Smart_Data* smartData, Ewk_Security_Origin* origin);
    static Eina_Bool handleEwkViewFullscreenExit(Ewk_View_Smart_Data* smartData);
#endif // ENABLE(TIZEN_FULLSCREEN_API)

#if ENABLE(TIZEN_TEXT_SELECTION)
    static Eina_Bool handleEwkViewTextSelectionDown(Ewk_View_Smart_Data*, int, int);
    static Eina_Bool handleEwkViewTextSelectionMove(Ewk_View_Smart_Data*, int, int);
    static Eina_Bool handleEwkViewTextSelectionUp(Ewk_View_Smart_Data*, int, int);
#endif
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    static void acquireWidgetResourceLock(void* data, Evas*, void*);
    static void releaseWidgetResourceLock(void* data, Evas*, void*);
#endif

private:
    // Note, initialization order matters.
    Evas_Object* m_evasObject;
    RefPtr<EwkContext> m_context;
    RefPtr<EwkPageGroup> m_pageGroup;
#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<Evas_GL> m_evasGL;
    OwnPtr<WebKit::EvasGLContext> m_evasGLContext;
    OwnPtr<WebKit::EvasGLSurface> m_evasGLSurface;
    bool m_pendingSurfaceResize;
#endif
#if ENABLE(TIZEN_DIRECT_RENDERING)
    Evas_GL_Options_Bits m_evasGLSurfaceOption;
#endif
    WebCore::TransformationMatrix m_userViewportTransform;
    OwnPtr<WebKit::PageLoadClientEfl> m_pageLoadClient;
    OwnPtr<WebKit::PagePolicyClientEfl> m_pagePolicyClient;
    OwnPtr<WebKit::PageUIClientEfl> m_pageUIClient;
    OwnPtr<WebKit::ContextMenuClientEfl> m_contextMenuClient;
    OwnPtr<WebKit::FindClientEfl> m_findClient;
    OwnPtr<WebKit::FormClientEfl> m_formClient;
    OwnPtr<WebKit::ViewClientEfl> m_viewClient;
#if ENABLE(VIBRATION)
    OwnPtr<WebKit::VibrationClientEfl> m_vibrationClient;
#endif
    OwnPtr<EwkBackForwardList> m_backForwardList;
    OwnPtr<EwkSettings> m_settings;
    RefPtr<EwkWindowFeatures> m_windowFeatures;

    union CursorIdentifier {
        CursorIdentifier()
            : image(0)
        {
        }
        WebCore::Image* image;
        const char* group;
    } m_cursorIdentifier;
    bool m_useCustomCursor;

    WKEinaSharedString m_url;
    mutable WKEinaSharedString m_title;
    WKEinaSharedString m_theme;
    WKEinaSharedString m_customEncoding;
    WKEinaSharedString m_userAgent;
    bool m_mouseEventsEnabled;
#if ENABLE(TOUCH_EVENTS)
    bool m_touchEventsEnabled;
    OwnPtr<WebKit::GestureRecognizer> m_gestureRecognizer;
#endif
    WebCore::Timer<EwkView> m_displayTimer;
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    WebCore::Timer<EwkView> m_effectImageTimer;
#endif
    RefPtr<EwkContextMenu> m_contextMenu;
    OwnPtr<EwkPopupMenu> m_popupMenu;
    OwnPtr<WebKit::InputMethodContextEfl> m_inputMethodContext;
#if ENABLE(INPUT_TYPE_COLOR)
    OwnPtr<EwkColorPicker> m_colorPicker;
#endif
#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<WebKit::PageViewportControllerClientEfl> m_pageViewportControllerClient;
    OwnPtr<WebKit::PageViewportController> m_pageViewportController;
#endif
    bool m_isAccelerated;
    static const unsigned s_marginForZoomableArea = 5;
#if PLATFORM(TIZEN)
    OwnPtr<Transition> m_transition;
#else
    std::unique_ptr<Transition> m_transition;
#endif
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    WKEinaSharedString m_webAppIconURL;
    Eina_List* m_webAppIconURLs;
#endif
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    OwnPtr<WebKit::OfflinePageSave> m_offlinePageSave;
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
    OwnPtr<WebKit::Drag> m_drag;
#endif // ENABLE(TIZEN_DRAG_SUPPORT)

#if ENABLE(TIZEN_USER_AGENT)
    WKEinaSharedString m_applicationName;
#endif // #if ENABLE(TIZEN_USER_AGENT)

#if PLATFORM(TIZEN)
    // FIXME: It will not work
    WebCore::IntRect m_visibleContentRect;
#endif

#if ENABLE(TIZEN_SUPPORT_GESTURE_SMART_EVENTS)
    bool m_isVerticalEdge;
    bool m_isHorizontalEdge;
#endif
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
    bool m_hasVerticalScrollableArea;
    bool m_hasHorizontalScrollableArea;
#endif
#if ENABLE(TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR)
    Ecore_Animator* m_scheduleUpdateDisplayAnimator;
    bool m_hasDirty;
#endif
#if ENABLE(TIZEN_VD_WHEEL_FLICK)
    int m_wheelDirection;
    int m_wheelOffset;
    WebCore::IntSize m_flickOffset;
    Ecore_Animator* m_flickAnimator;
    unsigned m_flickDuration;
    unsigned m_flickIndex;
#endif
#if ENABLE(TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE)
    bool m_didFirstFrameReady;
    bool m_didSetVisibleContentsRect;
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    OwnPtr<FocusRing> m_focusRing;
#endif
#if ENABLE(TIZEN_POINTER_LOCK)
    bool m_mouseLocked;
    bool m_lockingMouse;
    bool m_unlockingMouse;
    int m_mouseClickCounter;
#endif

#if ENABLE(TIZEN_SCROLLBAR)
    RefPtr<WebKit::MainFrameScrollbarTizen> m_horizontalScrollbar;
    RefPtr<WebKit::MainFrameScrollbarTizen> m_verticalScrollbar;

    bool m_mainFrameScrollbarVisibility;
#endif // ENABLE(TIZEN_SCROLLBAR)
#if ENABLE(TIZEN_SUPPORT_IDLECLOCK_AC)
    Ecore_X_Pixmap m_providedPixmap;
    widget_resource_lock_t m_widgetResourceLock;
    bool m_didStartWidgetDisplay;
#endif
#if ENABLE(TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT)
    bool m_needMirrorBlurEffect;
#endif
#if ENABLE(TIZEN_GEOLOCATION)
    HashSet<RefPtr<EwkObject> > m_pendingCallbackArguments;
#endif

#if ENABLE(TIZEN_CHECK_MODAL_POPUP)
    bool m_isOpenModalPopup;
#endif

    static Evas_Smart_Class parentSmartClass;
};

// FIXME: It should be moved into EwkView.cpp
EwkView* toEwkView(const Evas_Object* evasObject);

inline bool isEwkViewEvasObject(const Evas_Object* evasObject)
{
    ASSERT(evasObject);

    const Evas_Smart* evasSmart = evas_object_smart_smart_get(evasObject);
    if (EINA_UNLIKELY(!evasSmart)) {
        const char* evasObjectType = evas_object_type_get(evasObject);
        EINA_LOG_CRIT("%p (%s) is not a smart object!", evasObject, evasObjectType ? evasObjectType : "(null)");
        return false;
    }

    const Evas_Smart_Class* smartClass = evas_smart_class_get(evasSmart);
    if (EINA_UNLIKELY(!smartClass)) {
        const char* evasObjectType = evas_object_type_get(evasObject);
        EINA_LOG_CRIT("%p (%s) is not a smart class object!", evasObject, evasObjectType ? evasObjectType : "(null)");
        return false;
    }

    if (EINA_UNLIKELY(smartClass->data != EwkView::smartClassName)) {
        const char* evasObjectType = evas_object_type_get(evasObject);
        EINA_LOG_CRIT("%p (%s) is not of an ewk_view (need %p, got %p)!", evasObject, evasObjectType ? evasObjectType : "(null)",
            EwkView::smartClassName, smartClass->data);
        return false;
    }

    return true;
}

#endif // EwkView_h
