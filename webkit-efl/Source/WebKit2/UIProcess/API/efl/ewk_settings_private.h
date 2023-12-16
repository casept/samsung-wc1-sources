/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef ewk_settings_private_h
#define ewk_settings_private_h

#include <wtf/PassOwnPtr.h>

#if PLATFORM(TIZEN)
#include "WKEinaSharedString.h"
#endif

namespace WebKit {
class WebPreferences;
}
class EwkView;
/**
 * \struct  Ewk_Settings
 * @brief   Contains the settings data.
 */
class EwkSettings {
public:
    static PassOwnPtr<EwkSettings> create(EwkView* viewImpl)
    {
        return adoptPtr(new EwkSettings(viewImpl));
    }

    const WebKit::WebPreferences* preferences() const;
    WebKit::WebPreferences* preferences();

#if ENABLE(TIZEN_TEXT_SELECTION)
    bool textSelectionEnabled() const { return m_textSelectionEnabled; }
    void setTextSelectionEnabled(bool enable) { m_textSelectionEnabled = enable; }
    bool autoClearTextSelection() const { return m_autoClearTextSelection; }
    void setAutoClearTextSelection(bool enable) { m_autoClearTextSelection = enable; }
#endif

    const char* defaultTextEncodingName() const;
    void setDefaultTextEncodingName(const char*);

#if ENABLE(TIZEN_EDGE_EFFECT)
    bool edgeEffectEnabled() const { return m_edgeEffectEnabled; }
    void setEdgeEffectEnabled(bool enable) { m_edgeEffectEnabled = enable; }
#endif

#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    bool spatialNavigationEnabled() const { return m_spatialNavigationEnabled; }
    void setSpatialNavigationEnabled(bool enable) { m_spatialNavigationEnabled = enable; }
#endif

private:
    explicit EwkSettings(EwkView* viewImpl)
        : m_view(viewImpl)
#if ENABLE(TIZEN_TEXT_SELECTION)
        , m_textSelectionEnabled(true)
        , m_autoClearTextSelection(true)
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
        , m_edgeEffectEnabled(true)
#endif
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
        , m_spatialNavigationEnabled(true)
#endif
    {
        ASSERT(m_view);
        m_defaultTextEncodingName = preferences()->defaultTextEncodingName().utf8().data();
    }

    EwkView* m_view;
    WKEinaSharedString m_defaultTextEncodingName;

#if ENABLE(TIZEN_TEXT_SELECTION)
    bool m_textSelectionEnabled;
    bool m_autoClearTextSelection;
#endif
#if ENABLE(TIZEN_EDGE_EFFECT)
    bool m_edgeEffectEnabled;
#endif
#if ENABLE(TIZEN_SPATIAL_NAVIGATION)
    bool m_spatialNavigationEnabled;
#endif
};

#endif // ewk_settings_private_h
