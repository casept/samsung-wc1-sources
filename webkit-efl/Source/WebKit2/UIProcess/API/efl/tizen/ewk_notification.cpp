/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ewk_notification.h"

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ImmutableDictionary.h"
#include "WKAPICast.h"
#include "WKArray.h"
#include "WKContext.h"
#include "WKContextPrivate.h"
#include "WKNotification.h"
#include "WKNotificationManager.h"
#include "WKNotificationPermissionRequest.h"
#include "WKNumber.h"
#include "WKRetainPtr.h"
#include "WKSecurityOrigin.h"
#include "WKString.h"
#include "ewk_context_private.h"
#include "ewk_view.h"
#include "ewk_view_private.h"
#include <Evas.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

struct _Ewk_Notification {
    const char* body;
    const char* iconURL;
    const char* replaceID;
    const char* title;
    uint64_t notificationID;
    RefPtr<EwkSecurityOrigin> securityOrigin;
};

struct _Ewk_Notification_Permission_Request {
    WKRetainPtr<WKNotificationPermissionRequestRef> requestRef;
    Evas_Object* ewkView;
    RefPtr<EwkSecurityOrigin> origin;
    bool isDecided;
    bool isSuspended;

    _Ewk_Notification_Permission_Request(Evas_Object* ewkView, WKRetainPtr<WKNotificationPermissionRequestRef> notificationPermissionRequestRef, WKSecurityOriginRef originRef)
        : requestRef(notificationPermissionRequestRef)
        , ewkView(ewkView)
        , isDecided(false)
        , isSuspended(false)
    {
        origin = EwkSecurityOrigin::create(originRef);
    }

    ~_Ewk_Notification_Permission_Request()
    {
    }
};

Ewk_Notification* ewkNotificationCreateNotification(WKNotificationRef notificationRef)
{
    Ewk_Notification* ewkNotification = new Ewk_Notification();

    WKRetainPtr<WKStringRef> bodyRef(AdoptWK, WKNotificationCopyBody(notificationRef));
    WKRetainPtr<WKStringRef> iconURLRef(AdoptWK, WKNotificationCopyIconURL(notificationRef));
    WKRetainPtr<WKStringRef> replaceIDRef(AdoptWK, WKNotificationCopyTag(notificationRef));
    WKRetainPtr<WKStringRef> titleRef(AdoptWK, WKNotificationCopyTitle(notificationRef));

    eina_stringshare_replace(&ewkNotification->body, toImpl(bodyRef.get())->string().utf8().data());
    eina_stringshare_replace(&ewkNotification->iconURL, toImpl(iconURLRef.get())->string().utf8().data());
    eina_stringshare_replace(&ewkNotification->replaceID, toImpl(replaceIDRef.get())->string().utf8().data());
    eina_stringshare_replace(&ewkNotification->title, toImpl(titleRef.get())->string().utf8().data());
    ewkNotification->notificationID = WKNotificationGetID(notificationRef);
    ewkNotification->securityOrigin = EwkSecurityOrigin::create(WKNotificationGetSecurityOrigin(notificationRef));

    return ewkNotification;
}

Ewk_Notification_Permission_Request* ewkNotificationCreatePermissionRequest(Evas_Object* ewkView, WKNotificationPermissionRequestRef notificationPermissionRequestRef, WKSecurityOriginRef originRef)
{
    return new Ewk_Notification_Permission_Request(ewkView, notificationPermissionRequestRef, originRef);
}

void deleteEwkNotification(Ewk_Notification* ewkNotification)
{
    eina_stringshare_del(ewkNotification->body);
    eina_stringshare_del(ewkNotification->iconURL);
    eina_stringshare_del(ewkNotification->replaceID);
    eina_stringshare_del(ewkNotification->title);
    delete ewkNotification;
}

void ewkNotificationDeleteNotificationList(Eina_List* notifications)
{
    Eina_List* listIterator = 0;
    void* data = 0;
    EINA_LIST_FOREACH(notifications, listIterator, data) {
        Ewk_Notification* notification = static_cast<Ewk_Notification*>(data);
        deleteEwkNotification(notification);
    }
    eina_list_free(notifications);
}

void ewkNotificationDeletePermissionRequestList(Eina_List* requestList)
{
    void* data = 0;
    EINA_LIST_FREE(requestList, data)
        delete (static_cast<Ewk_Notification_Permission_Request*>(data));
}

const char* ewkNotificationGetReplaceID(const Ewk_Notification* ewkNotification)
{
    return ewkNotification->replaceID;
}

WKNotificationPermissionRequestRef ewkNotificationGetWKNotificationPermissionRequest(const Ewk_Notification_Permission_Request* permissionRequest)
{
    return permissionRequest->requestRef.get();
}

bool ewkNotificationIsPermissionRequestDecided(const Ewk_Notification_Permission_Request* permissionRequest)
{
    return permissionRequest->isDecided;
}

bool ewkNotificationIsPermissionRequestSuspended(const Ewk_Notification_Permission_Request* permissionRequest)
{
    return permissionRequest->isSuspended;
}
#endif // ENABLE(TIZEN_NOTIFICATIONS)

const char* ewk_notification_body_get(const Ewk_Notification* ewkNotification)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkNotification, 0);
    // TIZEN_LOGI("body (%s)", ewkNotification->body);

    return ewkNotification->body;
#else
    UNUSED_PARAM(ewkNotification);
    return 0;
#endif
}

void ewk_notification_clicked(Ewk_Context* context, uint64_t notificationID)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);

    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(ewkContext->wkContext());
    // TIZEN_LOGI("id (%llu)", notificationID);

    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(ewkContext->wkContext());
    WKNotificationManagerProviderDidClickNotification(notificationManagerRef, notificationID);
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(notificationID);
#endif
}

const char* ewk_notification_icon_url_get(const Ewk_Notification* ewkNotification)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkNotification, 0);

    return ewkNotification->iconURL;
#else
    UNUSED_PARAM(ewkNotification);
    return 0;
#endif
}

uint64_t ewk_notification_id_get(const Ewk_Notification* ewkNotification)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkNotification, 0);
    // TIZEN_LOGI("id (%llu)", ewkNotification->notificationID);

    return ewkNotification->notificationID;
#else
    UNUSED_PARAM(ewkNotification);
    return 0;
#endif
}

void ewk_notification_cached_permissions_set(Ewk_Context* context, Eina_List* ewkNotificationPermissions)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);

    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(ewkContext->wkContext());
    EINA_SAFETY_ON_NULL_RETURN(ewkNotificationPermissions);

    unsigned permissionsCount = eina_list_count(ewkNotificationPermissions);
    if(!permissionsCount)
        return;

    ImmutableDictionary::MapType cachedPermissions;
    Eina_List* listIterator = 0;
    void* data = 0;
    EINA_LIST_FOREACH(ewkNotificationPermissions, listIterator, data) {
        Ewk_Notification_Permission* notificationPermission = static_cast<Ewk_Notification_Permission*>(data);
        cachedPermissions.set(String(notificationPermission->origin), WebBoolean::create(notificationPermission->allowed));
        // TIZEN_LOGI("list(%s)", String(notificationPermission->origin).ascii().data());
    }

    RefPtr<ImmutableDictionary> permissionDictionary = ImmutableDictionary::adopt(cachedPermissions);
    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(ewkContext->wkContext());
    WKNotificationManagerSetCachedPermissions(notificationManagerRef, toAPI(permissionDictionary.get()));
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(ewkNotificationPermissions);
#endif
}

const Ewk_Security_Origin* ewk_notification_permission_request_origin_get(const Ewk_Notification_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, 0);

    return permissionRequest->origin.get();
#else
    UNUSED_PARAM(permissionRequest);
    return 0;
#endif
}

void ewk_notification_permission_request_set(Ewk_Notification_Permission_Request* permissionRequest, Eina_Bool allowed)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN(permissionRequest);
    // TIZEN_LOGI("allowed(%d)", allowed);

    permissionRequest->isDecided = true;

    EwkSecurityOrigin* origin = permissionRequest->origin.get();
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> securityOriginRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(ewk_view_context_get(permissionRequest->ewkView));
    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(ewkContext->wkContext());

    WKNotificationManagerProviderDidUpdateNotificationPolicy(notificationManagerRef, securityOriginRef.get(), static_cast<bool>(allowed));

    if(allowed)
        WKNotificationPermissionRequestAllow(permissionRequest->requestRef.get());
    else
       WKNotificationPermissionRequestDeny(permissionRequest->requestRef.get());

    ewkViewDeleteNotificationPermissionRequest(permissionRequest->ewkView, permissionRequest);
    delete permissionRequest;
#else
    UNUSED_PARAM(permissionRequest);
    UNUSED_PARAM(allowed);
#endif
}

void ewk_notification_permission_request_suspend(Ewk_Notification_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN(permissionRequest);
    // TIZEN_LOGI("ewkNotificationPermissionRequest (%p)", permissionRequest);

    permissionRequest->isSuspended = true;
#else
    UNUSED_PARAM(permissionRequest);
#endif
}

void ewk_notification_policies_removed(Ewk_Context* context, Eina_List* origins)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);

    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(ewkContext->wkContext());
    EINA_SAFETY_ON_NULL_RETURN(origins);
    // TIZEN_LOGI("origin count (%d)", eina_list_count(origins));

    unsigned count = eina_list_count(origins);
    if (!count)
        return;

    Vector<WKTypeRef> originsList;
    originsList.reserveInitialCapacity(count);

    Eina_List* listIterator = 0;
    void* data = 0;
    EINA_LIST_FOREACH(origins, listIterator, data) {
        EwkSecurityOrigin* origin = static_cast<EwkSecurityOrigin*>(data);
        WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
        WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
        WKRetainPtr<WKSecurityOriginRef> securityOriginRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
        originsList.append(securityOriginRef.leakRef());
        // TIZEN_LOGI("host (%s)", ewk_security_origin_host_get(origin));
    }

    WKRetainPtr<WKArrayRef> notificationOriginsArrayRef(AdoptWK, WKArrayCreate(originsList.data(), originsList.size()));
    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(ewkContext->wkContext());
    WKNotificationManagerProviderDidRemoveNotificationPolicies(notificationManagerRef, notificationOriginsArrayRef.get());

    for (size_t i = 0; i < originsList.size(); ++i)
        WKRelease(originsList[i]);
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(origins);
#endif
}


const Ewk_Security_Origin* ewk_notification_security_origin_get(const Ewk_Notification* ewkNotification)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkNotification, 0);

    return ewkNotification->securityOrigin.get();
#else
    UNUSED_PARAM(ewkNotification);
    return 0;
#endif
}

void ewk_notification_showed(Ewk_Context* context, uint64_t notificationID)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);

    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(ewkContext->wkContext());
    // TIZEN_LOGI("id (%llu)", notificationID);

    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(ewkContext->wkContext());
    WKNotificationManagerProviderDidShowNotification(notificationManagerRef, notificationID);
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(notificationID);
#endif
}

const char* ewk_notification_title_get(const Ewk_Notification* ewkNotification)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkNotification, 0);
    // TIZEN_LOGI("title (%s)", ewkNotification->title);

    return ewkNotification->title;
#else
    UNUSED_PARAM(ewkNotification);
    return 0;
#endif
}
