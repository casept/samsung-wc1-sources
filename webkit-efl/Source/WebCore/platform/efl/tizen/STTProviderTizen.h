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
#ifndef STTProviderTizen_h
#define STTProviderTizen_h

#if ENABLE(TIZEN_SCRIPTED_SPEECH)

#include <stt.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class SpeechRecognition;
class SpeechRecognitionError;

class STTProviderTizen {
    WTF_MAKE_NONCOPYABLE(STTProviderTizen);
public:
    static PassOwnPtr<STTProviderTizen> create();
    static PassRefPtr<SpeechRecognitionError> toCoreError(stt_error_e);

    ~STTProviderTizen();

    int sttCreate();
    int sttPrepare();
    int sttUnprepare();
    int sttStart(const char*, const char*);
    int sttStop();
    int sttCancel();
    int sttDestroy();

    int sttResultCallbackSet(stt_recognition_result_cb, void*);
    int sttResultCallbackUnset();

    int sttStateChangedCallbackSet(stt_state_changed_cb, void*);
    int sttStateChangedCallbackUnset();

    int sttErrorCallbackSet(stt_error_cb, void*);
    int sttErrorCallbackUnset();

    int sttDefaultLanguage(char**);

    int sttSilenceDetectionSet(stt_option_silence_detection_e);
    int sttState(stt_state_e*);

    bool sttReady() const { return m_sttReady; }

private:
    STTProviderTizen();

    stt_h m_stt;

    bool m_sttReady;
};

}

#endif // ENABLE(TIZEN_SCRIPTED_SPEECH)

#endif // STTProviderTizen_h
