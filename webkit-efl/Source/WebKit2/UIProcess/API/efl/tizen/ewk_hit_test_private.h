/*
   Copyright (C) 2011 Samsung Electronics

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

#ifndef ewk_hit_test_private_h
#define ewk_hit_test_private_h

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)

#include "WKAPICast.h"
#include "WebHitTestResult.h"
#include "ewk_hit_test.h"

Ewk_Hit_Test* ewkHitTestCreate(WebKit::WebHitTestResult::Data& hitTestResultData);

#endif // #if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#endif // ewk_hit_test_private_h
