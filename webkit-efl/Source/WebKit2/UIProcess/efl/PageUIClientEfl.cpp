/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "PageUIClientEfl.h"

#include "EwkView.h"
#include "WKAPICast.h"
#include "WKEvent.h"
#include "WKPageEfl.h"
#include "WKString.h"
#include "ewk_file_chooser_request_private.h"
#include "ewk_notification.h"
#include "ewk_view_private.h"
#include "ewk_window_features_private.h"
#include <Ecore_Evas.h>
#include <WebCore/Color.h>

#if ENABLE(TIZEN_GEOLOCATION)
#include "WKGeolocationPermissionRequest.h"
#include "ewk_geolocation_permission_request_private.h"
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "WKNotificationPermissionRequest.h"
#endif

#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
#include "WKPageTizen.h"
#endif

using namespace EwkViewCallbacks;

namespace WebKit {

static inline PageUIClientEfl* toPageUIClientEfl(const void* clientInfo)
{
    return static_cast<PageUIClientEfl*>(const_cast<void*>(clientInfo));
}

PageUIClientEfl::PageUIClientEfl(EwkView* view)
    : m_view(view)
{
    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(WKPageUIClient));
    uiClient.version = kWKPageUIClientCurrentVersion;
    uiClient.clientInfo = this;
    uiClient.close = close;
    uiClient.takeFocus = takeFocus;
    uiClient.focus = focus;
    uiClient.unfocus = unfocus;
    uiClient.runJavaScriptAlert = runJavaScriptAlert;
    uiClient.runJavaScriptConfirm = runJavaScriptConfirm;
    uiClient.runJavaScriptPrompt = runJavaScriptPrompt;
    uiClient.toolbarsAreVisible = toolbarsAreVisible;
    uiClient.setToolbarsAreVisible = setToolbarsAreVisible;
    uiClient.menuBarIsVisible = menuBarIsVisible;
    uiClient.setMenuBarIsVisible = setMenuBarIsVisible;
    uiClient.statusBarIsVisible = statusBarIsVisible;
    uiClient.setStatusBarIsVisible = setStatusBarIsVisible;
    uiClient.isResizable = isResizable;
    uiClient.setIsResizable = setIsResizable;
    uiClient.getWindowFrame = getWindowFrame;
    uiClient.setWindowFrame = setWindowFrame;
    uiClient.runBeforeUnloadConfirmPanel = runBeforeUnloadConfirmPanel;
#if ENABLE(SQL_DATABASE)
    uiClient.exceededDatabaseQuota = exceededDatabaseQuota;
#endif
    uiClient.runOpenPanel = runOpenPanel;
    uiClient.createNewPage = createNewPage;
#if ENABLE(INPUT_TYPE_COLOR)
    uiClient.showColorPicker = showColorPicker;
    uiClient.hideColorPicker = hideColorPicker;
#endif

#if ENABLE(TIZEN_INPUT_DATE_TIME)
    uiClient.showDateTimePicker = showDateTimePicker;
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    uiClient.decidePolicyForGeolocationPermissionRequest = decidePolicyForGeolocationPermissionRequest;
#endif // ENABLE(TIZEN_GEOLOCATION)
#if ENABLE(TIZEN_NOTIFICATIONS)
    uiClient.decidePolicyForNotificationPermissionRequest = decidePolicyForNotificationPermissionRequest;
#endif // ENABLE(TIZEN_NOTIFICATIONS)
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    uiClient.notifyPopupReplyWaitingState = notifyPopupReplyWaitingState;
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
    uiClient.pageDidScroll = pageDidScroll;
#endif

    WKPageSetPageUIClient(pageRef, &uiClient);

    // Popup Menu UI client.
    WKPageUIPopupMenuClient uiPopupMenuClient;
    memset(&uiPopupMenuClient, 0, sizeof(WKPageUIPopupMenuClient));
    uiPopupMenuClient.version = kWKPageUIPopupMenuClientCurrentVersion;
    uiPopupMenuClient.clientInfo = this;
    uiPopupMenuClient.showPopupMenu = showPopupMenu;
    uiPopupMenuClient.hidePopupMenu = hidePopupMenu;
    WKPageSetUIPopupMenuClient(pageRef, &uiPopupMenuClient);
}


void PageUIClientEfl::close(WKPageRef, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_view->close();
}

void PageUIClientEfl::takeFocus(WKPageRef, WKFocusDirection, const void* clientInfo)
{
    // FIXME: this is only a partial implementation.
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_view->evasObject(), false);
}

void PageUIClientEfl::focus(WKPageRef, const void* clientInfo)
{
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_view->evasObject(), true);
}

void PageUIClientEfl::unfocus(WKPageRef, const void* clientInfo)
{
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_view->evasObject(), false);
}

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void PageUIClientEfl::notifyPopupReplyWaitingState(WKPageRef, bool isWaiting, const void* clientInfo)
{
    ewkViewNotifyPopupReplyWaitingState(toPageUIClientEfl(clientInfo)->m_view->evasObject(), isWaiting);
}
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)

bool PageUIClientEfl::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo)
{
    return ewkViewRunJavaScriptAlert(toPageUIClientEfl(clientInfo)->m_view->evasObject(), alertText);
}

bool PageUIClientEfl::runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return ewkViewRunJavaScriptConfirm(toPageUIClientEfl(clientInfo)->m_view->evasObject(), message);
}

bool PageUIClientEfl::runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    return ewkViewRunJavaScriptPrompt(toPageUIClientEfl(clientInfo)->m_view->evasObject(), message, defaultValue);
}
#else // PLATFORM(TIZEN)
void PageUIClientEfl::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_view->requestJSAlertPopup(WKEinaSharedString(alertText));
}

bool PageUIClientEfl::runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_view->requestJSConfirmPopup(WKEinaSharedString(message));
}

WKStringRef PageUIClientEfl::runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    WKEinaSharedString value = toPageUIClientEfl(clientInfo)->m_view->requestJSPromptPopup(WKEinaSharedString(message), WKEinaSharedString(defaultValue));
    return value ? WKStringCreateWithUTF8CString(value) : 0;
}
#endif // PLATFORM(TIZEN)

bool PageUIClientEfl::toolbarsAreVisible(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    return features->toolbarVisible();
}

void PageUIClientEfl::setToolbarsAreVisible(WKPageRef, bool toolbarVisible, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    features->setToolbarVisible(toolbarVisible);
}

bool PageUIClientEfl::menuBarIsVisible(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    return features->menuBarVisible();
}

void PageUIClientEfl::setMenuBarIsVisible(WKPageRef, bool menuBarVisible, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    features->setMenuBarVisible(menuBarVisible);
}

bool PageUIClientEfl::statusBarIsVisible(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    return features->statusBarVisible();
}

void PageUIClientEfl::setStatusBarIsVisible(WKPageRef, bool statusBarVisible, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    features->setStatusBarVisible(statusBarVisible);
}

bool PageUIClientEfl::isResizable(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    return features->resizable();
}

void PageUIClientEfl::setIsResizable(WKPageRef, bool resizable, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_view->windowFeatures();
    ASSERT(features);
    features->setResizable(resizable);
}

WKRect PageUIClientEfl::getWindowFrame(WKPageRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_view->windowGeometry();
}

void PageUIClientEfl::setWindowFrame(WKPageRef, WKRect frame, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_view->setWindowGeometry(frame);
}

bool PageUIClientEfl::runBeforeUnloadConfirmPanel(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_view->requestJSConfirmPopup(WKEinaSharedString(message));
}

#if ENABLE(SQL_DATABASE)
#if ENABLE(TIZEN_SQL_DATABASE)
bool PageUIClientEfl::exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef origin, WKStringRef displayName, unsigned long long expectedUsage, const void *clientInfo)
{
    return ewkViewExceededDatabaseQuota(toPageUIClientEfl(clientInfo)->m_view->evasObject(), origin, displayName, expectedUsage);
}
#else // ENABLE(TIZEN_SQL_DATABASE)
unsigned long long PageUIClientEfl::exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void* clientInfo)
{
    EwkView* view = toPageUIClientEfl(clientInfo)->m_view;
    return view->informDatabaseQuotaReached(toImpl(databaseName)->string(), toImpl(displayName)->string(), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
}
#endif // ENABLE(TIZEN_SQL_DATABASE)
#endif

void PageUIClientEfl::runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    EwkView* view = toPageUIClientEfl(clientInfo)->m_view;
    RefPtr<EwkFileChooserRequest> fileChooserRequest = EwkFileChooserRequest::create(parameters, listener);
    view->smartCallback<FileChooserRequest>().call(fileChooserRequest.get());
}

WKPageRef PageUIClientEfl::createNewPage(WKPageRef, WKURLRequestRef wkRequest, WKDictionaryRef wkWindowFeatures, WKEventModifiers, WKEventMouseButton, const void* clientInfo)
{
    RefPtr<EwkUrlRequest> request = EwkUrlRequest::create(wkRequest);
    return toPageUIClientEfl(clientInfo)->m_view->createNewPage(request, wkWindowFeatures);
}

#if ENABLE(INPUT_TYPE_COLOR)
void PageUIClientEfl::showColorPicker(WKPageRef, WKStringRef initialColor, WKColorPickerResultListenerRef listener, const void* clientInfo)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    WebCore::Color color = WebCore::Color(WebKit::toWTFString(initialColor));
    pageUIClient->m_view->requestColorPicker(listener, color);
}

void PageUIClientEfl::hideColorPicker(WKPageRef, const void* clientInfo)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    pageUIClient->m_view->dismissColorPicker();
}
#endif

#if ENABLE(TIZEN_INPUT_DATE_TIME)
void PageUIClientEfl::showDateTimePicker(WKPageRef, const void* clientInfo, bool isTimeType, WKStringRef value)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    pageUIClient->m_view->showDateTimePicker(isTimeType, WKEinaSharedString(value));
}
#endif

void PageUIClientEfl::showPopupMenu(WKPageRef, WKPopupMenuListenerRef menuListenerRef, WKRect rect, WKPopupItemTextDirection textDirection, double pageScaleFactor, WKArrayRef itemsRef, int32_t selectedIndex, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_view->requestPopupMenu(menuListenerRef, rect, textDirection, pageScaleFactor, itemsRef, selectedIndex);
}

void PageUIClientEfl::hidePopupMenu(WKPageRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_view->closePopupMenu();
}

#if ENABLE(TIZEN_GEOLOCATION)
void PageUIClientEfl::decidePolicyForGeolocationPermissionRequest(WKPageRef, WKFrameRef, WKSecurityOriginRef wkOrigin, WKGeolocationPermissionRequestRef wkPermissionRequest, const void* clientInfo)
{
    EwkView* view = toPageUIClientEfl(clientInfo)->m_view;
    RefPtr<EwkGeolocationPermissionRequest> permissionRequest = EwkGeolocationPermissionRequest::create(wkPermissionRequest, wkOrigin);
    view->smartCallback<EwkViewCallbacks::GeolocationPermissionRequest>().call(permissionRequest.get());

    if (permissionRequest->isSuspended())
        view->addPendingCallbackArgument(permissionRequest);

    if (permissionRequest->hasOneRef() && !permissionRequest->permissionSelected())
        permissionRequest->setAllowed(false);
}
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
void PageUIClientEfl::decidePolicyForNotificationPermissionRequest(WKPageRef, WKSecurityOriginRef origin, WKNotificationPermissionRequestRef permissionRequest, const void* clientInfo)
{
    Evas_Object* ewkView = toPageUIClientEfl(clientInfo)->m_view->evasObject();
    Ewk_Notification_Permission_Request* ewkNotificationPermissionRequest = ewkNotificationCreatePermissionRequest(ewkView, permissionRequest, origin);
    ewkViewRequestNotificationPermission(ewkView, ewkNotificationPermissionRequest);

    if(!ewkNotificationIsPermissionRequestSuspended(ewkNotificationPermissionRequest))
        if(!ewkNotificationIsPermissionRequestDecided(ewkNotificationPermissionRequest))
            WKNotificationPermissionRequestDeny(ewkNotificationGetWKNotificationPermissionRequest(ewkNotificationPermissionRequest));
}
#endif // ENABLE(TIZEN_NOTIFICATIONS)

#if ENABLE(TIZEN_SUPPORT_SCROLLING_APIS)
#if ENABLE(TIZEN_SCROLLBAR)
void PageUIClientEfl::pageDidScroll(WKPageRef wkPageRef, const void* clientInfo)
{
    WKPageUpdateScrollPosition(wkPageRef);
    toPageUIClientEfl(clientInfo)->m_view->updateScrollbar();
}
#else
void PageUIClientEfl::pageDidScroll(WKPageRef wkPageRef, const void*)
{
    WKPageUpdateScrollPosition(wkPageRef);
}
#endif // ENABLE(TIZEN_SCROLLBAR)
#endif
} // namespace WebKit
