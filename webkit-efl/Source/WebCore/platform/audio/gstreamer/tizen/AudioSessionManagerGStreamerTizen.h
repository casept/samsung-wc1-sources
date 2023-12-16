/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioSessionManagerGStreamerTizen_h
#define AudioSessionManagerGStreamerTizen_h

#if ENABLE(TIZEN_GSTREAMER_AUDIO)

#include "Logging.h"
#include "MediaPlayerPrivate.h"
#include <audio-session-manager.h>
#if ENABLE(TIZEN_TV_ASM_READY)
#include <gst/gstelement.h>
#endif
#include <mm_session.h>
#include <mm_session_private.h>
#include <wtf/Forward.h>

namespace WebCore {

class AudioSessionManagerCallbackClient {
public:
    virtual void notifyResumeByASMCallback() = 0;
};

class AudioSessionManagerGStreamerTizen : public RefCounted<AudioSessionManagerGStreamerTizen> {
public:
    static PassRefPtr<AudioSessionManagerGStreamerTizen> createAudioSessionManager()
    {
        return adoptRef(new AudioSessionManagerGStreamerTizen());
    }

    static void setVolumeSessionToMediaType();
    static void clearVolumeSessionFromMediaType();

    ~AudioSessionManagerGStreamerTizen();

    bool registerAudioSessionManager(ASM_sound_cb_t, void*);
    bool unregisterAudioSessionManager();

    int getSoundState(ASM_sound_states_t*);
    bool setSoundState(ASM_sound_states_t);

#if ENABLE(TIZEN_TV_ASM_READY)
    bool allocateResources(GstElement*);
    bool deallocateResources();
#endif

    static void addCallbackClient(AudioSessionManagerCallbackClient*);
    static void removeCallbackClient(AudioSessionManagerCallbackClient*);
    static void notifyResume();

private:
    AudioSessionManagerGStreamerTizen();

    ASM_sound_events_t m_eventType;
    int m_handle;
    ASM_sound_cb_t m_notifyCallback;
    int m_processIdentifier;
    MMSessionType m_sessionType;
    ASM_sound_states_t m_stateType;
#if ENABLE(TIZEN_TV_ASM_READY)
    bool m_allocated;
    ASM_request_resources_t m_resources;
#endif
    static Vector<AudioSessionManagerCallbackClient*> m_callbackList;
};
}
#endif
#endif
