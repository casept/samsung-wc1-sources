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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef WKURLResponseTizen_h
#define WKURLResponseTizen_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKStringRef WKURLResponseEflCopyContentType(WKURLResponseRef);

#ifdef __cplusplus
}
#endif

#endif /* WKURLResponseTizen_h */
