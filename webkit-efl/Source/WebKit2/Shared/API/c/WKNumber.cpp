/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKNumber.h"

#include "WKAPICast.h"
#include "WebNumber.h"

using namespace WebKit;

WKTypeID WKBooleanGetTypeID()
{
    return toAPI(WebBoolean::APIType);
}

WKBooleanRef WKBooleanCreate(bool value)
{
    RefPtr<WebBoolean> booleanObject = WebBoolean::create(value);
    return toAPI(booleanObject.release().leakRef());
}

bool WKBooleanGetValue(WKBooleanRef booleanRef)
{
    return toImpl(booleanRef)->value();
}

WKTypeID WKDoubleGetTypeID()
{
    return toAPI(WebDouble::APIType);
}

WKDoubleRef WKDoubleCreate(double value)
{
    RefPtr<WebDouble> doubleObject = WebDouble::create(value);
    return toAPI(doubleObject.release().leakRef());
}

double WKDoubleGetValue(WKDoubleRef doubleRef)
{
    return toImpl(doubleRef)->value();
}

WKTypeID WKUInt64GetTypeID()
{
    return toAPI(WebUInt64::APIType);
}

WKUInt64Ref WKUInt64Create(uint64_t value)
{
    RefPtr<WebUInt64> uint64Object = WebUInt64::create(value);
    return toAPI(uint64Object.release().leakRef());
}

uint64_t WKUInt64GetValue(WKUInt64Ref uint64Ref)
{
    return toImpl(uint64Ref)->value();
}

#if PLATFORM(TIZEN)
WKTypeID WKUInt32GetTypeID()
{
    return toAPI(WebUInt32::APIType);
}

WKUInt32Ref WKUInt32Create(uint32_t value)
{
    RefPtr<WebUInt32> uint32Object = WebUInt32::create(value);
    return toAPI(uint32Object.release().leakRef());
}

uint32_t WKUInt32GetValue(WKUInt32Ref uint32Ref)
{
    return toImpl(uint32Ref)->value();
}

WKTypeID WKInt64GetTypeID()
{
    return toAPI(WebInt64::APIType);
}

WKInt64Ref WKInt64Create(int64_t value)
{
    RefPtr<WebInt64> int64Object = WebInt64::create(value);
    return toAPI(int64Object.release().leakRef());
}

int64_t WKInt64GetValue(WKInt64Ref int64Ref)
{
    return toImpl(int64Ref)->value();
}
#endif // PLATFORM(TIZEN)
