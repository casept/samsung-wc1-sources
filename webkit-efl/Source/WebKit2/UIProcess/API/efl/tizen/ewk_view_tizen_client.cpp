/*
   Copyright (C) 2012 Samsung Electronics

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
#include "ewk_view_tizen_client.h"

#include "WKFrame.h"
#include "WKPageEfl.h"
#include "WKSharedAPICast.h"
#include "WKString.h"
#include "ewk_view.h"
#include "ewk_view_private.h"

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "ewk_user_media_private.h"
#endif

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
#include "WKWebSpeechPermissionRequest.h"
#include "ewk_webspeech_permission_request_private.h"
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#include "ewk_certificate.h"
#endif

static void decidePolicyForUserMediaPermissionRequest(WKPageRef, WKUserMediaPermissionRequestRef permissionRequest, WKSecurityOriginRef origin, const void* clientInfo)
{
#if ENABLE(TIZEN_MEDIA_STREAM)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    Ewk_User_Media_Permission_Request* userMediaPermissionRequest = ewkUserMediaPermissionRequestCreate(ewkView, permissionRequest, origin);
    ewkViewRequestUserMediaPermission(ewkView, userMediaPermissionRequest);
#else
    UNUSED_PARAM(permissionRequest);
    UNUSED_PARAM(clientInfo);
#endif
}

static bool exceededIndexedDatabaseQuota(WKPageRef, WKSecurityOriginRef origin, long long currentUsage, WKFrameRef, const void* clientInfo)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    return ewkViewExceededIndexedDatabaseQuota(static_cast<Evas_Object*>(const_cast<void*>(clientInfo)), origin, currentUsage);
#else
    UNUSED_PARAM(origin);
    UNUSED_PARAM(currentUsage);
    UNUSED_PARAM(clientInfo);
    return false;
#endif
}

static bool decidePolicyForCertificateError(WKPageRef page, WKStringRef url, WKStringRef certificate, int error, const void* clientInfo)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));

    Ewk_Certificate_Policy_Decision* certificatePolicyDecision = ewkCertificatePolicyDecisionCreate(page, url, certificate, error);
    ewkViewRequestCertificateConfirm(ewkView, certificatePolicyDecision);

    if (!ewkCertificatePolicyDecisionDecided(certificatePolicyDecision) && !ewkCertificatePolicyDecisionSuspended(certificatePolicyDecision))
        ewk_certificate_policy_decision_allowed_set(certificatePolicyDecision, true);

    return true;
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(url);
    UNUSED_PARAM(certificate);
    UNUSED_PARAM(error);
    UNUSED_PARAM(clientInfo);
    return false;
#endif
}

static void decidePolicyForWebSpeechPermissionRequest(WKPageRef, WKWebSpeechPermissionRequestRef wkPermissionRequest,
    WKStringRef host, WKStringRef protocol, int port, const void* clientInfo)
{
#if ENABLE(TIZEN_SCRIPTED_SPEECH)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    Ewk_WebSpeech_Permission_Request* webSpeechPermissionRequest = ewkWebSpeechPermissionRequestCreate(ewkView, wkPermissionRequest, host, protocol, port);
    ewkViewRequestWebSpeechPermission(ewkView, webSpeechPermissionRequest);
#else
    UNUSED_PARAM(wkPermissionRequest);
    UNUSED_PARAM(clientInfo);
#endif
}

void ewkViewTizenClientAttachClient(EwkView* ewkView)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    WKPageTizenClient tizenClient = {
        kWKPageTizenClientCurrentVersion,
        ewkView->evasObject(), // clientInfo
//#if ENABLE(TIZEN_MEDIA_STREAM)
        decidePolicyForUserMediaPermissionRequest,
//#endif
//#if ENABLE(TIZEN_INDEXED_DATABASE)
        exceededIndexedDatabaseQuota,
//#endif
//#if EANBLE(TIZEN_SCRIPTED_SPEECH)
        decidePolicyForWebSpeechPermissionRequest,
//#endif
        decidePolicyForCertificateError
    };

    WKPageSetPageTizenClient(toAPI(ewkView->page()), &tizenClient);
}
