/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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

#include "config.h"
#include "ewk_webspeech_permission_request.h"

#include "WKAPICast.h"
#include "WKWebSpeechPermissionRequest.h"
#include "WKRetainPtr.h"
#include "ewk_security_origin_private.h"
#include "ewk_view_private.h"
#include "ewk_webspeech_permission_request_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

struct _Ewk_WebSpeech_Permission_Request {
    WKRetainPtr<WKWebSpeechPermissionRequestRef> requestRef;
    Evas_Object* ewkView;
    bool isDecided;
    bool isSuspended;
    RefPtr<EwkSecurityOrigin> securityOrigin;

    _Ewk_WebSpeech_Permission_Request(Evas_Object* ewkView, WKRetainPtr<WKWebSpeechPermissionRequestRef> webSpeechPermissionRequestRef, WKSecurityOriginRef wkOrigin)
        : requestRef(webSpeechPermissionRequestRef)
        , ewkView(ewkView)
        , isDecided(false)
        , isSuspended(false)
        , securityOrigin(EwkSecurityOrigin::create(wkOrigin))
    { }
    _Ewk_WebSpeech_Permission_Request(Evas_Object* ewkView, WKRetainPtr<WKWebSpeechPermissionRequestRef> webSpeechPermissionRequestRef,
            const char* host, const char* protocol, uint32_t port)
        : requestRef(webSpeechPermissionRequestRef)
        , ewkView(ewkView)
        , isDecided(false)
        , isSuspended(false)
        , securityOrigin(EwkSecurityOrigin::create(host, protocol, port))
    { }
};

Ewk_WebSpeech_Permission_Request* ewkWebSpeechPermissionRequestCreate(Evas_Object* ewkView, WKWebSpeechPermissionRequestRef webSpeechPermissionRequestRef,
    WKStringRef host, WKStringRef protocol, int port)
{
    return new Ewk_WebSpeech_Permission_Request(ewkView, webSpeechPermissionRequestRef,
        toImpl(host)->string().utf8().data(), toImpl(protocol)->string().utf8().data(), port);
}

bool ewkWebSpeechPermissionRequestDecided(Ewk_WebSpeech_Permission_Request* permissionRequest)
{
    return permissionRequest->isDecided;
}

bool ewkWebSpeechPermissionRequestSuspended(Ewk_WebSpeech_Permission_Request* permissionRequest)
{
    return permissionRequest->isSuspended;
}

const Ewk_Security_Origin* ewk_webspeech_permission_request_origin_get(const Ewk_WebSpeech_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_SCRIPTED_SPEECH)
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, 0);

    return permissionRequest->securityOrigin.get();
#else
    UNUSED_PARAM(permissionRequest);
    return 0;
#endif
}

Eina_Bool ewk_webspeech_permission_request_suspend(Ewk_WebSpeech_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_SCRIPTED_SPEECH)
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest->requestRef, false);

    permissionRequest->isSuspended = true;

    return true;
#else
    UNUSED_PARAM(permissionRequest);
    return false;
#endif
}

void ewk_webspeech_permission_request_set(Ewk_WebSpeech_Permission_Request* permissionRequest, Eina_Bool allowed)
{
#if ENABLE(TIZEN_SCRIPTED_SPEECH)
    EINA_SAFETY_ON_NULL_RETURN(permissionRequest);

    permissionRequest->isDecided = true;

    if (allowed)
        WKWebSpeechPermissionRequestAllow(permissionRequest->requestRef.get());
    else
        WKWebSpeechPermissionRequestDeny(permissionRequest->requestRef.get());

    delete permissionRequest;
#else
    UNUSED_PARAM(permissionRequest);
    UNUSED_PARAM(allowed);
#endif
}
