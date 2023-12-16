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
#include "ewk_geolocation_permission_request.h"

#if ENABLE(TIZEN_GEOLOCATION)
#include "ewk_geolocation_permission_request_private.h"
#include "ewk_security_origin_private.h"
#include <WebKit2/WKGeolocationPermissionRequest.h>
#include <WebKit2/WKSecurityOrigin.h>

EwkGeolocationPermissionRequest::EwkGeolocationPermissionRequest(WKGeolocationPermissionRequestRef wkPermissionRequest, WKSecurityOriginRef wkOrigin)
    : m_wkPermissionRequest(wkPermissionRequest)
    , m_securityOrigin(EwkSecurityOrigin::create(wkOrigin))
    , m_suspended(false)
{
}

EwkSecurityOrigin* EwkGeolocationPermissionRequest::securityOrigin() const
{
    return m_securityOrigin.get();
}

void EwkGeolocationPermissionRequest::setAllowed(bool allow)
{
    if (!m_wkPermissionRequest)
        return;

    if (allow)
        WKGeolocationPermissionRequestAllow(m_wkPermissionRequest.get());
    else
        WKGeolocationPermissionRequestDeny(m_wkPermissionRequest.get());

    m_wkPermissionRequest.clear();
}
#endif // ENABLE(TIZEN_GEOLOCATION)

const Ewk_Security_Origin* ewk_geolocation_permission_request_origin_get(const Ewk_Geolocation_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkGeolocationPermissionRequest, permissionRequest, impl, nullptr);

    return impl->securityOrigin();
#else
    UNUSED_PARAM(permissionRequest);
    return 0;
#endif
}

void ewk_geolocation_permission_request_set(Ewk_Geolocation_Permission_Request* permissionRequest, Eina_Bool allow)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkGeolocationPermissionRequest, permissionRequest, impl);

    impl->setAllowed(allow);
#else
    UNUSED_PARAM(permissionRequest);
    UNUSED_PARAM(allow);
#endif
}

void ewk_geolocation_permission_request_suspend(Ewk_Geolocation_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkGeolocationPermissionRequest, permissionRequest, impl);

    impl->suspend();
#else
    UNUSED_PARAM(permissionRequest);
#endif
}
