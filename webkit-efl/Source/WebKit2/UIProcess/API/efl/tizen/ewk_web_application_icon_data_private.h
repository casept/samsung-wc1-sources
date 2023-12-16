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

#ifndef ewk_web_application_icon_data_private_h
#define ewk_web_application_icon_data_private_h

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
#include <WebKit2/WKBase.h>

/// Creates a type name for _Ewk_Web_App_Icon_Data.
typedef struct _Ewk_Web_App_Icon_Data Ewk_Web_App_Icon_Data;

Ewk_Web_App_Icon_Data* ewkWebAppIconDataCreate(WKStringRef iconSize, WKStringRef iconURL);
void ewkWebAppIconDataDelete(Ewk_Web_App_Icon_Data* iconData);

#endif // ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)

#endif // ewk_web_application_icon_data_private_h
