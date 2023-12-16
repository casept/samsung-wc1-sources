/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
#ifndef PlatformSynthesisProviderTizen_h
#define PlatformSynthesisProviderTizen_h

#if ENABLE(SPEECH_SYNTHESIS)

#include <PlatformSpeechSynthesisVoice.h>

#include <wtf/RefPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

#include <wtf/text/WTFString.h>

#include <tts.h>

namespace WebCore {

class PlatformSpeechSynthesisUtterance;
class PlatformSpeechSynthesisVoice;
class PlatformSpeechSynthesizer;

class PlatformSynthesisProviderTizen {
public:
    static PassOwnPtr<PlatformSynthesisProviderTizen> create(PlatformSpeechSynthesizer*);
    ~PlatformSynthesisProviderTizen();

    void initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice> >&);
    void speak(PassRefPtr<PlatformSpeechSynthesisUtterance>);
    void pause();
    void resume();
    void cancel();

private:
    explicit PlatformSynthesisProviderTizen(PlatformSpeechSynthesizer*);
    void finishInitialization();
    void addVoice(String language, int name);
    void speakStoredUtterance();

    static void ttsStateChangedCallback(tts_h tts, tts_state_e previous, tts_state_e current, void* userData);
    static void ttsUtteranceStartedCallback(tts_h tts, int utteranceId, void* userData);
    static void ttsUtteranceCompletedCallback(tts_h tts, int utteranceId, void* userData);
    static void ttsErrorCallback(tts_h tts, int utteranceId, tts_error_e reason, void* userData);
    static bool ttsSupportedVoiceCallback(tts_h tts, const char* language, int voice_type, void* user_data);

    PlatformSpeechSynthesizer* m_platformSynthesizerObject;
    RefPtr<PlatformSpeechSynthesisUtterance> m_utterance;
    bool m_initialized;

    tts_h m_tts;
    String m_defaultLanguage;
    int m_defaultVoice;
    Vector<RefPtr<PlatformSpeechSynthesisVoice> > m_voiceList;
    bool m_utteranceWaiting;
};

}
#endif // ENABLE(SPEECH_SYNTHESIS)

#endif // PlatformSynthesisProviderTizen_h

