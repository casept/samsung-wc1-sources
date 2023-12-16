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

#ifndef ewk_user_media_private_h
#define ewk_user_media_private_h

#if ENABLE(TIZEN_MEDIA_STREAM)
#include <Evas.h>
#include <WebKit2/WKBase.h>

typedef struct _Ewk_User_Media_Permission_Request Ewk_User_Media_Permission_Request;

Ewk_User_Media_Permission_Request* ewkUserMediaPermissionRequestCreate(Evas_Object* ewkView, WKUserMediaPermissionRequestRef, WKSecurityOriginRef);
void ewkUserMediaDeletePermissionRequestList(Eina_List* requests);
bool ewkUserMediaPermissionRequestDecided(Ewk_User_Media_Permission_Request*);
bool ewkUserMediaPermissionRequestSuspended(Ewk_User_Media_Permission_Request*);

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // ewk_user_media_private_h
