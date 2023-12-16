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

#ifndef ewk_geolocation_permission_request_private_h
#define ewk_geolocation_permission_request_private_h

#if ENABLE(TIZEN_GEOLOCATION)
#include "ewk_object_private.h"
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/RefPtr.h>

class EwkSecurityOrigin;

class EwkGeolocationPermissionRequest : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkGeolocationPermissionRequest)

    static PassRefPtr<EwkGeolocationPermissionRequest> create(WKGeolocationPermissionRequestRef wkPermissionRequest, WKSecurityOriginRef wkOrigin)
    {
        return adoptRef(new EwkGeolocationPermissionRequest(wkPermissionRequest, wkOrigin));
    }

    EwkSecurityOrigin* securityOrigin() const;

    bool isSuspended() const { return m_suspended; }
    void suspend() { m_suspended = true; }

    void setAllowed(bool);
    bool permissionSelected() const { return !m_wkPermissionRequest; }

private:
    EwkGeolocationPermissionRequest(WKGeolocationPermissionRequestRef, WKSecurityOriginRef);

    WKRetainPtr<WKGeolocationPermissionRequestRef> m_wkPermissionRequest;
    RefPtr<EwkSecurityOrigin> m_securityOrigin;
    bool m_suspended;
};

#endif // ENABLE(TIZEN_GEOLOCATION)

#endif // ewk_geolocation_permission_request_private_h
