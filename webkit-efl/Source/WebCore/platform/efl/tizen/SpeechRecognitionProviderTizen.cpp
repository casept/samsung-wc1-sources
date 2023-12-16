/*
 *  Copyright (C) 2013 Samsung Electronics.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(TIZEN_SCRIPTED_SPEECH)

#include "SpeechRecognitionProviderTizen.h"

#include <STTProviderTizen.h>
#include <SpeechGrammarList.h>
#include <SpeechRecognition.h>
#include <SpeechRecognitionAlternative.h>
#include <SpeechRecognitionError.h>
#include <SpeechRecognitionResult.h>
#include <wtf/text/CString.h>

using namespace WebCore;

SpeechRecognitionProviderTizen::SpeechRecognitionProviderTizen()
    : m_speechRecognition(0)
    , m_grammarList(0)
    , m_providerInitialized(false)
{
}

SpeechRecognitionProviderTizen::~SpeechRecognitionProviderTizen()
{
}

PassOwnPtr<SpeechRecognitionProviderTizen> SpeechRecognitionProviderTizen::create()
{
    return adoptPtr(new SpeechRecognitionProviderTizen);
}

bool SpeechRecognitionProviderTizen::initProvider()
{
    // If any error occurs while stt is running, we initialize stt again on start, and re-connects to server.
    if (m_sttProvider->sttReady())
        if (isError(m_sttProvider->sttDestroy()))
            return false;

    // Create handle
    if (isError(m_sttProvider->sttCreate()))
        return false;

    if (isError(m_sttProvider->sttResultCallbackSet(sttResultCallback, this)))
        return false;

    if (isError(m_sttProvider->sttStateChangedCallbackSet(sttStateChangedCallback, this)))
        return false;

    if (isError(m_sttProvider->sttErrorCallbackSet(sttErrorCallback, m_speechRecognition)))
        return false;

    // Connects daemon
    return !isError(m_sttProvider->sttPrepare());
}

void SpeechRecognitionProviderTizen::start(SpeechRecognition* speechRecognition, const SpeechGrammarList* speechGrammarList, const String& lang, bool continuous, bool interimResults, unsigned long maxAlternatives)
{
    if (!speechRecognition || !speechGrammarList)
        return;

    // Parameters that come from web developer, stored for later use.
    m_speechRecognition = speechRecognition;
    m_grammarList = speechGrammarList;
    m_language = lang;
    // When false, return no more than one final result in r
    m_continuous = continuous;
    m_interimResults = interimResults;
    m_maxAlternatives = maxAlternatives;

    m_speechRecognition->didStart();

    if (!m_sttProvider)
        m_sttProvider = STTProviderTizen::create();

    if (!m_providerInitialized) {
        // Informs that stt has been initialized.
        // It does not have to be initialized again on the same session, unless errors related to stt appears.
        // In case of any errors, it will be started again.
        m_providerInitialized = true;
        // Initialize stt, in case of any errors, stt_start will not be run.
        if (!initProvider())
            return;
    }

    startRecognition();
}

void SpeechRecognitionProviderTizen::startRecognition()
{
    if (!startProvider())
        return;

    if (m_speechRecognition)
        m_speechRecognition->didStartAudio();
}

bool SpeechRecognitionProviderTizen::startProvider()
{
    stt_state_e state;
    if (isError(m_sttProvider->sttState(&state)))
        return false;

    if (state != STT_STATE_READY)
        return false;

    // But default continuous is false, which means we return no more than one final result on response to starting recognition
    stt_option_silence_detection_e silenceOption = STT_OPTION_SILENCE_DETECTION_AUTO;
    // When m_continuous is true, could return zero or more final results.
    if (m_continuous)
        silenceOption = STT_OPTION_SILENCE_DETECTION_FALSE;

    if (isError(m_sttProvider->sttSilenceDetectionSet(silenceOption)))
        return false;

    // In case of missing language, we set default system language.
    if (m_language.isEmpty()) {
        char* language = 0;
        if (isError(m_sttProvider->sttDefaultLanguage(&language)))
            return false;
        m_language = language;
        free(language);
    }

    // Start capturing audio
    return !isError(m_sttProvider->sttStart(m_language.utf8().data(), STT_RECOGNITION_TYPE_FREE));
}

void SpeechRecognitionProviderTizen::stop(SpeechRecognition* speechRecognition)
{
    if (speechRecognition)
        speechRecognition->didEnd();

    stt_state_e state;
    if (isError(m_sttProvider->sttState(&state)))
        return;

    // Stop capturing audio
    if (state == STT_STATE_RECORDING)
        isError(m_sttProvider->sttStop());
}

void SpeechRecognitionProviderTizen::abort(SpeechRecognition* speechRecognition)
{
    if (speechRecognition) {
        speechRecognition->didEnd();
        speechRecognition->didReceiveError(SpeechRecognitionError::create(SpeechRecognitionError::ErrorCodeAborted, "Speech input was aborted"));
    }

    stt_state_e state;
    if (isError(m_sttProvider->sttState(&state)))
        return;

    // Cancel capturing audio
    if (state == STT_STATE_RECORDING || state == STT_STATE_PROCESSING)
        isError(m_sttProvider->sttCancel());
}

// Callbacks
void SpeechRecognitionProviderTizen::sttResultCallback(stt_h /* stt */, stt_result_event_e, const char** data, int dataCount, const char* msg, void* userData)
{
    SpeechRecognitionProviderTizen* provider = static_cast<SpeechRecognitionProviderTizen*>(userData);
    if (!provider && !provider->m_speechRecognition)
        return;

    Vector<RefPtr<SpeechRecognitionResult> > newFinalResults;
    Vector<RefPtr<SpeechRecognitionResult> > currentInterimResults;
    Vector<RefPtr<SpeechRecognitionAlternative> > alternativeResults;

    // Collecting recognition results
    for(int i = 0; i < dataCount && i < provider->m_maxAlternatives; i++)
        if (data[i])
            alternativeResults.append(SpeechRecognitionAlternative::create(String(data[i]), 1));

    newFinalResults.append(SpeechRecognitionResult::create(alternativeResults, 1));
    provider->m_speechRecognition->didReceiveResults(newFinalResults, currentInterimResults);

    if (!dataCount)
        provider->m_speechRecognition->didReceiveError(SpeechRecognitionError::create(SpeechRecognitionError::ErrorCodeNoSpeech, "No speech detected"));

    // Server may send a message, when speech were not recognized (speaking to quite/to fast etc.)
    // When such a message occur, we can send nomatch event.
    if (msg && provider->noMatchResults(msg))
        provider->m_speechRecognition->didReceiveNoMatch(SpeechRecognitionResult::create(alternativeResults, 1));

    provider->m_speechRecognition->didEndAudio();
    provider->m_speechRecognition->didEnd();
}

bool SpeechRecognitionProviderTizen::noMatchResults(const String& message)
{
    return message == STT_RESULT_MESSAGE_ERROR_TOO_SOON ||
           message == STT_RESULT_MESSAGE_ERROR_TOO_SHORT ||
           message == STT_RESULT_MESSAGE_ERROR_TOO_LONG ||
           message == STT_RESULT_MESSAGE_ERROR_TOO_QUIET ||
           message == STT_RESULT_MESSAGE_ERROR_TOO_LOUD ||
           message == STT_RESULT_MESSAGE_ERROR_TOO_FAST;
}

bool SpeechRecognitionProviderTizen::isError(int err)
{
    if (err == STT_ERROR_NONE)
        return false;

    // In case of any errors related to stt, stt's errors try to be matched to respective WebCore's speech error.
    if (m_speechRecognition)
        m_speechRecognition->didReceiveError(STTProviderTizen::toCoreError(static_cast<stt_error_e>(err)));
    // To make sure stt works properly, on the next start stt will be set up again.
    m_providerInitialized = false;
    return true;
}

void SpeechRecognitionProviderTizen::sttErrorCallback(stt_h /* stt */, stt_error_e reason, void* userData)
{
    SpeechRecognition* speech = static_cast<SpeechRecognition*>(userData);
    if (!speech)
        return;

    speech->didReceiveError(STTProviderTizen::toCoreError(static_cast<stt_error_e>(reason)));
}

void SpeechRecognitionProviderTizen::sttStateChangedCallback(stt_h /* stt */, stt_state_e previous, stt_state_e current, void* userData)
{
    SpeechRecognitionProviderTizen* provider = static_cast<SpeechRecognitionProviderTizen*>(userData);
    if (!provider)
        return;

    if (previous == STT_STATE_CREATED && current == STT_STATE_READY)
        provider->startRecognition();
}

#endif // ENABLE(TIZEN_SCRIPTED_SPEECH)
