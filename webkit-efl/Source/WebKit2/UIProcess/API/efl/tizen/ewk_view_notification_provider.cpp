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
#include "ewk_view_notification_provider.h"

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "WKAPICast.h"
#include "WKContext.h"
#include "WKNotification.h"
#include "WKNotificationManager.h"
#include "WKNotificationProvider.h"
#include "WKPage.h"
#include "ewk_notification.h"
#include "ewk_view_private.h"

using namespace WebKit;

static void cancelNotification(WKNotificationRef notification, const void* clientInfo)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    uint64_t notificationID = WKNotificationGetID(notification);
    ewkViewCancelNotification(ewkView, notificationID);
}

static void showNotification(WKPageRef, WKNotificationRef notification, const void* clientInfo)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    Ewk_Notification* ewkNotification = ewkNotificationCreateNotification(notification);
    ewkViewShowNotification(ewkView, ewkNotification);
}

void ewkViewNotificationProviderAttachProvider(Evas_Object* ewkView, WKContextRef contextRef)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    WKNotificationProvider notificationProvider = {
        kWKNotificationProviderCurrentVersion, //version
        ewkView, //clientInfo
        showNotification, //show
        cancelNotification, //cancel
        0, //didDestroyNotification
        0, //addNotificationManger
        0, //removeNotificationManager
        0, //notificationPermissions
        0 //clearNotifications
    };

    WKNotificationManagerRef notificationManager = WKContextGetNotificationManager(contextRef);
    WKNotificationManagerSetProvider(notificationManager, &notificationProvider);
}
#endif // ENABLE(TIZEN_NOTIFICATIONS)
