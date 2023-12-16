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

#ifndef ewk_geolocation_permission_request_internal_h
#define ewk_geolocation_permission_request_internal_h

#include "ewk_security_origin_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Geolocation_Permission_Request as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Geolocation_Permission_Request;

/**
 * Requests for getting origin of geolocation permission request.
 *
 * @param request Ewk_Geolocation_Permission_Request object to get origin
 *
 * @return security origin of geolocation permission data
 */
EAPI const Ewk_Security_Origin *ewk_geolocation_permission_request_origin_get(const Ewk_Geolocation_Permission_Request *request);

/**
 * Request to allow / deny the geolocation permission request
 *
 * @param request permission request to allow or deny permission
 * @param allow allow or deny permission for geolocation
 */
EAPI void ewk_geolocation_permission_request_set(Ewk_Geolocation_Permission_Request *request, Eina_Bool allow);

/**
 * DEPRECATED
 * Suspend the operation for permission request.
 *
 * This API is deprecated because source code had been refactored, so this API is needed no longer.
 * Instead, the apps which has using this API should use ewk_object_ref() and ewk_object_unref
 * to increase and decrease refCount of instance.
 *
 * @param request Ewk_Geolocation_Permission_Request object to suspend permission request
 */
EINA_DEPRECATED EAPI void ewk_geolocation_permission_request_suspend(Ewk_Geolocation_Permission_Request *request);

#ifdef __cplusplus
}
#endif

#endif // ewk_geolocation_permission_request_internal_h
