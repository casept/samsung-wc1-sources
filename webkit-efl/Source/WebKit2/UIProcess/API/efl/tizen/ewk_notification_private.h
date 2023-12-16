/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ewk_notification_private_h
#define ewk_notification_private_h

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ewk_notification.h"
#include "ewk_security_origin.h"
#include <WebKit2/WKBase.h>

typedef struct _Ewk_Notification_Permission_Request Ewk_Notification_Permission_Request;

Ewk_Notification* ewkNotificationCreateNotification(WKNotificationRef);
Ewk_Notification_Permission_Request* ewkNotificationCreatePermissionRequest(Evas_Object*, WKNotificationPermissionRequestRef, WKSecurityOriginRef);
void ewkNotificationDeleteNotificationList(Eina_List* notifications);
void ewkNotificationDeletePermissionRequestList(Eina_List* requestList);
const char* ewkNotificationGetReplaceID(const Ewk_Notification*);
WKNotificationPermissionRequestRef ewkNotificationGetWKNotificationPermissionRequest(const Ewk_Notification_Permission_Request*);
bool ewkNotificationIsPermissionRequestDecided(const Ewk_Notification_Permission_Request*);
bool ewkNotificationIsPermissionRequestSuspended(const Ewk_Notification_Permission_Request*);

#endif // ENABLE(TIZEN_NOTIFICATIONS)

#endif // ewk_notification_private_h
