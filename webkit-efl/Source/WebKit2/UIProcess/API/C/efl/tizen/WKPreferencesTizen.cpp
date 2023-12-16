/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "WKPreferencesTizen.h"

#include "WKAPICast.h"
#include "WebPreferences.h"

using namespace WebKit;

void WKPreferencesSetDefaultViewLevel(WKPreferencesRef preferencesRef, double level)
{
    toImpl(preferencesRef)->setDefaultViewLevel(level);
}

double WKPreferencesGetDefaultViewLevel(WKPreferencesRef preferencesRef)
{
    return toImpl(preferencesRef)->defaultViewLevel();
}

void WKPreferencesSetStyleScopedEnabled(WKPreferencesRef preferencesRef, bool enable)
{
#if ENABLE(TIZEN_STYLE_SCOPED)
    toImpl(preferencesRef)->setStyleScopedEnabled(enable);
#endif // ENABLE(TIZEN_STYLE_SCOPED)
}

bool WKPreferencesGetStyleScopedEnabled(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_STYLE_SCOPED)
    return toImpl(preferencesRef)->styleScopedEnabled();
#endif // ENABLE(TIZEN_STYLE_SCOPED)
}

void WKPreferencesSetLoadRemoteImages(WKPreferencesRef preferencesRef, bool loadRemoteImages)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    toImpl(preferencesRef)->setLoadRemoteImages(loadRemoteImages);
#endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
}

bool WKPreferencesGetLoadRemoteImages(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    return toImpl(preferencesRef)->loadRemoteImages();
#else
    return false;
#endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
}

void WKPreferencesSetDeviceSupportsMouse(WKPreferencesRef preferencesRef, bool deviceSupportsMouse)
{
#if ENABLE(TIZEN_PREFERENCE)
    toImpl(preferencesRef)->setDeviceSupportsMouse(deviceSupportsMouse);
#endif // #if ENABLE(TIZEN_PREFERENCE)
}
