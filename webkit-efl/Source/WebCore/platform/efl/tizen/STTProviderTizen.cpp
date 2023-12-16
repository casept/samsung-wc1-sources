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
#include "config.h"

#if ENABLE(TIZEN_SCRIPTED_SPEECH)

#include "STTProviderTizen.h"

#include "SpeechRecognitionError.h"

namespace WebCore {

PassRefPtr<SpeechRecognitionError> STTProviderTizen::toCoreError(stt_error_e errorType)
{
    const char* message = 0;
    SpeechRecognitionError::ErrorCode errorCode = SpeechRecognitionError::ErrorCodeOther;
    switch(errorType) {
        case STT_ERROR_OUT_OF_MEMORY:
        case STT_ERROR_IO_ERROR:
        case STT_ERROR_INVALID_PARAMETER:
        case STT_ERROR_OPERATION_FAILED:
        case STT_ERROR_TIMED_OUT:
        case STT_ERROR_INVALID_STATE:
        case STT_ERROR_ENGINE_NOT_FOUND:
        case STT_ERROR_NOT_SUPPORTED_FEATURE:
            return SpeechRecognitionError::create();
        case STT_ERROR_RECORDER_BUSY:
            errorCode = SpeechRecognitionError::ErrorCodeAudioCapture;
            message = "Audio capture failed";
            break;
        case STT_ERROR_OUT_OF_NETWORK:
            errorCode = SpeechRecognitionError::ErrorCodeNetwork;
            message = "Some network communication that was required to complete the recognition failed";
            break;
        case STT_ERROR_INVALID_LANGUAGE:
            errorCode = SpeechRecognitionError::ErrorCodeLanguageNotSupported;
            message = "The language was not supported";
            break;
        case STT_ERROR_NONE:
            break;
    }

    return SpeechRecognitionError::create(errorCode, message);
}

PassOwnPtr<STTProviderTizen> STTProviderTizen::create()
{
    return adoptPtr(new STTProviderTizen);
}

STTProviderTizen::STTProviderTizen()
    : m_sttReady(false)
{
}

STTProviderTizen::~STTProviderTizen()
{
    sttUnprepare();
    sttDestroy();
}

int STTProviderTizen::sttCreate()
{
    int error = stt_create(&m_stt);
    m_sttReady = error == STT_ERROR_NONE;
    return error;
}

int STTProviderTizen::sttPrepare()
{
    return m_sttReady ? stt_prepare(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttUnprepare()
{
    return m_sttReady ? stt_unprepare(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttStart(const char* language, const char* type)
{
    return m_sttReady ? stt_start(m_stt, language, type) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttStop()
{
    return m_sttReady ? stt_stop(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttCancel()
{
    return m_sttReady ? stt_cancel(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttDestroy()
{
    return m_sttReady ? stt_destroy(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttSilenceDetectionSet(stt_option_silence_detection_e option)
{
    return m_sttReady ? stt_set_silence_detection(m_stt, option) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttState(stt_state_e* state)
{
    return m_sttReady ? stt_get_state(m_stt, state) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttDefaultLanguage(char** language)
{
    return m_sttReady ? stt_get_default_language(m_stt, &*language) : STT_ERROR_OPERATION_FAILED;
}

// Callbacks
int STTProviderTizen::sttResultCallbackSet(stt_recognition_result_cb callback, void* data)
{
    return m_sttReady ? stt_set_recognition_result_cb(m_stt, callback, data) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttResultCallbackUnset()
{
    return m_sttReady ? stt_unset_recognition_result_cb(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttErrorCallbackSet(stt_error_cb callback, void* data)
{
    return m_sttReady ? stt_set_error_cb(m_stt, callback, data) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttErrorCallbackUnset()
{
    return m_sttReady ? stt_unset_error_cb(m_stt) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttStateChangedCallbackSet(stt_state_changed_cb callback, void* data)
{
    return m_sttReady ? stt_set_state_changed_cb(m_stt, callback, data) : STT_ERROR_OPERATION_FAILED;
}

int STTProviderTizen::sttStateChangedCallbackUnset()
{
    return m_sttReady ? stt_unset_state_changed_cb(m_stt) : STT_ERROR_OPERATION_FAILED;
}

} // namespace WebCore

#endif // ENABLE(TIZEN_SCRIPTED_SPEECH)

