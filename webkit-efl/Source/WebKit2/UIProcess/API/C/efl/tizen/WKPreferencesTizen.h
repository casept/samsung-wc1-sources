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

#ifndef WKPreferencesTizen_h
#define WKPreferencesTizen_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

// Defaults to 1.0.
WK_EXPORT void WKPreferencesSetDefaultViewLevel(WKPreferencesRef preferencesRef, double);
WK_EXPORT double WKPreferencesGetDefaultViewLevel(WKPreferencesRef preferencesRef);

// #if ENABLE(TIZEN_STYLE_SCOPED)
// Defaults to true
void WKPreferencesSetStyleScopedEnabled(WKPreferencesRef preferencesRef, bool enable);
bool WKPreferencesGetStyleScopedEnabled(WKPreferencesRef preferencesRef);
// #endif // ENABLE(TIZEN_STYLE_SCOPED)

// #if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
WK_EXPORT void WKPreferencesSetLoadRemoteImages(WKPreferencesRef preferencesRef, bool);
WK_EXPORT bool WKPreferencesGetLoadRemoteImages(WKPreferencesRef preferencesRef);
// #endif // ENABLE(TIZEN_LOAD_REMOTE_IMAGES)

// #if ENABLE(TIZEN_PREFERENCE)
WK_EXPORT void WKPreferencesSetDeviceSupportsMouse(WKPreferencesRef, bool);
// #endif // #if ENABLE(TIZEN_PREFERENCE)

#ifdef __cplusplus
}
#endif

#endif /* WKPreferencesTizen_h */
