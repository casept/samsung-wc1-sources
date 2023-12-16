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

#ifndef ewk_webspeech_permission_request_h
#define ewk_webspeech_permission_request_h

#include "ewk_security_origin.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_WebSpeech_Permission_Request as Ewk_Object.
 *
 * Tizen specific APIs for webspeech.
 */
typedef struct _Ewk_WebSpeech_Permission_Request Ewk_WebSpeech_Permission_Request;

/**
 * Requests for getting origin of webspeech permission request.
 *
 * @param request Ewk_WebSpeech_Permission_Request object to get origin
 *
 * @return security origin of webspeech permission data
 */
EAPI const Ewk_Security_Origin *ewk_webspeech_permission_request_origin_get(const Ewk_WebSpeech_Permission_Request *request);

/**
 * Request to allow / deny the webspeech permission request
 *
 * Tizen specific APIs for webspeech.
 *
 * @param request permission request to allow or deny permission
 * @param allow allow or deny permission for webspeech
 */
EAPI void ewk_webspeech_permission_request_set(Ewk_WebSpeech_Permission_Request *request, Eina_Bool allow);

/**
 * Suspend the operation for webspeech permission
 *
 * @param request webspeech permission request object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_webspeech_permission_request_suspend(Ewk_WebSpeech_Permission_Request *request);

#ifdef __cplusplus
}
#endif

#endif // ewk_webspeech_permission_request_h
