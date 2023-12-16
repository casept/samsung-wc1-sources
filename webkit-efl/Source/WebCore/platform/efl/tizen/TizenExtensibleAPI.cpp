/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"

#if ENABLE(TIZEN_EXTENSIBLE_API)

#include "TizenExtensibleAPI.h"

namespace WebCore {

static TizenExtensibleAPI* tizenExtensibleAPI = 0;

void TizenExtensibleAPI::initializeTizenExtensibleAPI()
{
    ASSERT(!tizenExtensibleAPI);

    tizenExtensibleAPI = new TizenExtensibleAPI();
}

TizenExtensibleAPI& TizenExtensibleAPI::extensibleAPI()
{
    return *tizenExtensibleAPI;
}

TizenExtensibleAPI::TizenExtensibleAPI()
    : m_backgroundMusic(true)
    , m_backgroundVibration(false)
    , m_audioRecordingControl(false)
    , m_cameraControl(false)
    , m_csp(false)
    , m_encryptDatabase(false)
    , m_fullScreen(false)
    , m_mediaStreamRecord(false)
    , m_mediaVolumeControl(false)
    , m_prerenderingForRotation(false)
    , m_rotateCameraView(true)
    , m_soundMode(false)
{
}

void TizenExtensibleAPI::setTizenExtensibleAPI(ExtensibleAPI extensibleAPI, bool enable)
{
    switch (extensibleAPI) {
        case BackgroundMusic:
            m_backgroundMusic = enable;
            break;
        case BackgroundVibration:
            m_backgroundVibration = enable;
            break;
        case TizenAudioRecordingControl:
            m_audioRecordingControl = enable;
            break;
        case TizenCameraControl:
            m_cameraControl = enable;
            break;
        case CSP:
            m_csp = enable;
            break;
        case EncryptionDatabase:
            m_encryptDatabase = enable;
            break;
        case FullScreen:
            m_fullScreen = enable;
            break;
        case MediaStreamRecord:
            m_mediaStreamRecord = enable;
            break;
        case MediaVolumeControl:
            m_mediaVolumeControl = enable;
            break;
        case PrerenderingForRotation:
            m_prerenderingForRotation = enable;
            break;
        case RotateCameraView:
            m_rotateCameraView = enable;
            break;
        case SoundMode:
            m_soundMode = enable;
            break;
        default:
            ASSERT_NOT_REACHED();
            return;
    }
}
} // namespace WebCore

#endif // ENABLE(TIZEN_EXTENSIBLE_API)
