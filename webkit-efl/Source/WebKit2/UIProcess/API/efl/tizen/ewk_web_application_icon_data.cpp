/*
 * Copyright (C) 2013 Samsung Electronics
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
#include "ewk_web_application_icon_data.h"

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
#include "WKSharedAPICast.h"
#include "WKString.h"
#include <Eina.h>
#include <wtf/text/CString.h>

using namespace WebKit;

struct _Ewk_Web_App_Icon_Data {
    CString size;
    CString url;

    _Ewk_Web_App_Icon_Data(WKStringRef sizeRef, WKStringRef urlRef)
    {
        size = toImpl(sizeRef)->string().utf8();
        url = toImpl(urlRef)->string().utf8();
    }
};

Ewk_Web_App_Icon_Data* ewkWebAppIconDataCreate(WKStringRef sizeRef, WKStringRef urlRef)
{
    return new Ewk_Web_App_Icon_Data(sizeRef, urlRef);
}

void ewkWebAppIconDataDelete(Ewk_Web_App_Icon_Data* iconData)
{
    delete iconData;
}
#endif

EAPI const char* ewk_web_application_icon_data_size_get(Ewk_Web_App_Icon_Data* data)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);

    //TIZEN_LOGI("size: %s", data->size.data());
    return data->size.data();
#else
    UNUSED_PARAM(data);
#endif
}

EAPI const char* ewk_web_application_icon_data_url_get(Ewk_Web_App_Icon_Data* data)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);

    //TIZEN_LOGI("url: %s", data->url.data());
    return data->url.data();
#else
    UNUSED_PARAM(data);
#endif
}
