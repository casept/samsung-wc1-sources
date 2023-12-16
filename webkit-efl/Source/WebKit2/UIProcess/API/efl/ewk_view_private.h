/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef ewk_view_private_h
#define ewk_view_private_h

#include <Evas.h>
#include <WebKit2/WKBase.h>

#ifdef BUILDING_WEBKIT
#if PLATFORM(TIZEN)
#include "EditorState.h"
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
#include "ewk_text_style.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
#include "ewk_policy_decision.h"
#include <WebKit2/WKPageLoadTypes.h>
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ewk_notification.h"
#include "ewk_notification_private.h"
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "ewk_user_media.h"
#endif

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
#include "ewk_webspeech_permission_request.h"
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#include "ewk_certificate.h"
#endif

#if PLATFORM(TIZEN)
namespace WebCore {
class IntPoint;
}
#endif

#ifdef BUILDING_WEBKIT
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
void ewkViewTextStyleState(Evas_Object* ewkView, const WebCore::IntPoint& startPoint, const WebCore::IntPoint& endPoint);
Ewk_Text_Style* ewkTextStyleCreate(const WebKit::EditorState editorState, const WebCore::IntPoint& startPoint, const WebCore::IntPoint& endPoint);
void ewkTextStyleDelete(Ewk_Text_Style* textStyle);
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)
Ewk_Policy_Decision* ewkPolicyDecisionCreate(WKFramePolicyListenerRef listener, WKURLRequestRef request, WKFrameRef frame = 0, WKURLResponseRef response = 0, WKFrameNavigationType navigationType = kWKFrameNavigationTypeOther);
void ewkPolicyDecisionDelete(Ewk_Policy_Decision* policyDecision);
bool ewkPolicyDecisionDecided(Ewk_Policy_Decision* policyDecision);
bool ewkPolicyDecisionSuspended(Ewk_Policy_Decision* policyDecision);
void ewkViewPolicyNavigationDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision);
void ewkViewPolicyNewWindowDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision);
void ewkViewPolicyResponseDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision);
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY)

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
void ewkViewLoadError(Evas_Object* ewkView, WKErrorRef error);
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)

#if ENABLE(TIZEN_NOTIFICATIONS)
void ewkViewCancelNotification(Evas_Object* ewkView, uint64_t notificationID);
void ewkViewRequestNotificationPermission(Evas_Object* ewkView, Ewk_Notification_Permission_Request*);
void ewkViewShowNotification(Evas_Object* ewkView, Ewk_Notification*);
void ewkViewDeleteNotificationPermissionRequest(Evas_Object* ewkView, Ewk_Notification_Permission_Request*);
#endif // ENABLE(TIZEN_NOTIFICATIONS)

#if ENABLE(TIZEN_MEDIA_STREAM)
void ewkViewRequestUserMediaPermission(Evas_Object* ewkView, Ewk_User_Media_Permission_Request*);
void ewkViewDeleteUserMediaPermissionRequest(Evas_Object* ewkView, Ewk_User_Media_Permission_Request*);
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_SQL_DATABASE)
bool ewkViewExceededDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, WKStringRef displayName, unsigned long long expectedUsage);
Ewk_Context_Exceeded_Quota* ewkContextCreateExceededQuota(WKSecurityOriginRef origin, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage);
void ewkContextDeleteExceededQuota(Ewk_Context_Exceeded_Quota* exceededQuota);
unsigned long long ewkContextGetNewQuotaForExceededQuota(Ewk_Context* context, Ewk_Context_Exceeded_Quota* exceededQuota);
#endif // ENABLE(TIZEN_SQL_DATABASE)

#if ENABLE(TIZEN_INDEXED_DATABASE)
bool ewkViewExceededIndexedDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, long long currentUsage);
#endif // ENABLE(TIZEN_INDEXED_DATABASE)

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void ewkViewNotifyPopupReplyWaitingState(Evas_Object* ewkView, bool isWaiting);
#endif // ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void ewkViewInitializeJavaScriptPopup(Evas_Object* ewkView);
bool ewkViewRunJavaScriptAlert(Evas_Object* ewkView, WKStringRef alertText);
bool ewkViewRunJavaScriptConfirm(Evas_Object* ewkView, WKStringRef message);
bool ewkViewRunJavaScriptPrompt(Evas_Object* ewkView, WKStringRef message, WKStringRef defaultValue);
#endif // PLATFORM(TIZEN)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
bool ewkViewRequestCertificateConfirm(Evas_Object* ewkView, Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
Ewk_Certificate_Policy_Decision* ewkCertificatePolicyDecisionCreate(WKPageRef page, WKStringRef url, WKStringRef certificate, int error);
void ewkCertificatePolicyDecisionDelete(Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
bool ewkCertificatePolicyDecisionSuspended(Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
bool ewkCertificatePolicyDecisionDecided(Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
#endif

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
void ewkViewRequestWebSpeechPermission(Evas_Object* ewkView, Ewk_WebSpeech_Permission_Request*);
#endif

#if ENABLE(TIZEN_CHECK_MODAL_POPUP)
void ewk_view_modal_popup_open_set(Evas_Object* ewkView, bool open);
bool ewk_view_modal_popup_open_get(Evas_Object* ewkView);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if ENABLE(TIZEN_POPUP_INTERNAL)
Eina_Bool ewk_view_popup_menu_close(Evas_Object* ewkView);
#endif

#if ENABLE(TIZEN_INPUT_DATE_TIME)
void ewk_view_date_time_picker_set(Evas_Object* ewkView, const char* value);
#endif

EAPI Evas_Object* EWKViewCreate(WKContextRef, WKPageGroupRef, Evas*, Evas_Smart*);
EAPI WKViewRef EWKViewGetWKView(Evas_Object*);

#ifdef __cplusplus
}
#endif
#endif // ewk_view_private_h
