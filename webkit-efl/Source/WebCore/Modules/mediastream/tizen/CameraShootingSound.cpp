/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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
#include "CameraShootingSound.h"

#if ENABLE(TIZEN_CAMERA_SHOOTING_SOUND)
#include <mm_sound.h>
#include <vconf.h>

namespace WebCore {

static const char imageCaptureSoundPath[] = "/usr/share/sounds/mm-camcorder/Shutter.ogg";
static const char recordStartSoundPath[] = "/usr/share/sounds/mm-camcorder/Cam_Start.ogg";
static const char recordStopSoundPath[] = "/usr/share/sounds/mm-camcorder/Cam_Stop.ogg";

CameraShootingSound::CameraShootingSound()
{
    vconf_get_int(VCONFKEY_CAMERA_SHUTTER_SOUND_POLICY, &m_shutterSoundPolicy);
    vconf_get_bool(VCONFKEY_SETAPPL_SOUND_STATUS_BOOL, &m_soundState);
}

void CameraShootingSound::playCameraShootingSound(type soundType)
{
    int gainType;
    const char* filePath;
    switch (soundType) {
    case ImageCapture:
        filePath = imageCaptureSoundPath;
        gainType = VOLUME_GAIN_SHUTTER1;
        break;
    case RecordStart:
        filePath = recordStartSoundPath;
        gainType = VOLUME_GAIN_CAMCORDING;
        break;
    case RecordStop:
        filePath = recordStopSoundPath;
        gainType = VOLUME_GAIN_CAMCORDING;
        break;
    default:
        TIZEN_LOGI("Invalid type of camera shooting sound.[%x]", soundType);
        return;
    }

    // FIXME: Added handle because mmplayer make a crash when we didn't pass handle although it is not used.
    int handle = -1;
    int result = MM_ERROR_NONE;
    if (m_shutterSoundPolicy == VCONFKEY_CAMERA_SHUTTER_SOUND_POLICY_ON)
#if ENABLE(TIZEN_TV_PROFILE)
        result = mm_sound_play_loud_solo_sound(filePath, VOLUME_TYPE_FIXED | gainType, 0, 0, 1, &handle);
#else
        result = mm_sound_play_loud_solo_sound(filePath, VOLUME_TYPE_FIXED | gainType, 0, 0, &handle);
#endif
    else {
        mm_sound_device_in deviceIn;
        mm_sound_device_out deviceOut;
        mm_sound_get_active_device(&deviceIn, &deviceOut);

        if (m_soundState || deviceOut != MM_SOUND_DEVICE_OUT_SPEAKER)
#if ENABLE(TIZEN_TV_PROFILE)
            result = mm_sound_play_solo_sound(filePath, VOLUME_TYPE_SYSTEM | gainType, 0, 0, 1, &handle);
#else
            result = mm_sound_play_solo_sound(filePath, VOLUME_TYPE_SYSTEM | gainType, 0, 0, &handle);
#endif
    }

    if (result != MM_ERROR_NONE)
        TIZEN_LOGI("FAILED to play camera shooting sound.[%x]", result);
}

} // namespace WebCore
#endif