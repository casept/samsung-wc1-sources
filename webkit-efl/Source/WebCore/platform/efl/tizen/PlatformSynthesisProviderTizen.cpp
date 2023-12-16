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
#include <config.h>

#if ENABLE(SPEECH_SYNTHESIS)

#include "PlatformSynthesisProviderTizen.h"
#include <PlatformSpeechSynthesizer.h>
#include <PlatformSpeechSynthesisUtterance.h>

#include <wtf/text/CString.h>

#define URI_PREFIX "localhost:"

namespace WebCore {

String voiceTypeToName(int type)
{
    switch (type) {
    case TTS_VOICE_TYPE_AUTO:
        return String("Auto");
    case TTS_VOICE_TYPE_MALE:
        return String("Male");
    case TTS_VOICE_TYPE_FEMALE:
        return String("Female");
    case TTS_VOICE_TYPE_CHILD:
        return String("Child");
    }
    return "Auto";
}

int voiceNameToType(String name)
{
    if (name == "Auto")
        return TTS_VOICE_TYPE_AUTO;
    else if (name == "Male")
        return TTS_VOICE_TYPE_MALE;
    else if (name == "Female")
        return TTS_VOICE_TYPE_FEMALE;
    else if (name == "Child")
        return TTS_VOICE_TYPE_CHILD;
    else return TTS_VOICE_TYPE_AUTO;
}

void PlatformSynthesisProviderTizen::ttsStateChangedCallback(tts_h /* tts */, tts_state_e previous, tts_state_e current, void* userData)
{
    ASSERT(userData);
    PlatformSynthesisProviderTizen* _this = static_cast<PlatformSynthesisProviderTizen*>(userData);
    ASSERT(tts == _this->m_tts);
    if (TTS_STATE_CREATED == previous && TTS_STATE_READY == current) {
        _this->finishInitialization();
    } else if (TTS_STATE_PAUSED == previous && TTS_STATE_PLAYING == current && _this->m_utterance) {
        _this->m_platformSynthesizerObject->client()->didResumeSpeaking(_this->m_utterance);
    } else if (TTS_STATE_PLAYING == previous && TTS_STATE_PAUSED == current && _this->m_utterance) {
        _this->m_platformSynthesizerObject->client()->didPauseSpeaking(_this->m_utterance);
    }
}

void PlatformSynthesisProviderTizen::ttsUtteranceStartedCallback(tts_h /* tts */, int /* utteranceId */, void* userData)
{
    ASSERT(userData);
    PlatformSynthesisProviderTizen* _this = static_cast<PlatformSynthesisProviderTizen*>(userData);
    ASSERT(tts == _this->m_tts);
    if (_this->m_utterance)
        _this->m_platformSynthesizerObject->client()->didStartSpeaking(_this->m_utterance);
}
void PlatformSynthesisProviderTizen::ttsUtteranceCompletedCallback(tts_h /* tts */, int /* utteranceId */, void *userData)
{
    ASSERT(userData);
    PlatformSynthesisProviderTizen* _this = static_cast<PlatformSynthesisProviderTizen*>(userData);
    ASSERT(tts == _this->m_tts);
    if (_this->m_utterance)
        _this->m_platformSynthesizerObject->client()->didFinishSpeaking(_this->m_utterance);
}
void PlatformSynthesisProviderTizen::ttsErrorCallback(tts_h /* tts */, int /* utteranceId */, tts_error_e /* reason */, void* userData)
{
    ASSERT(userData);
    PlatformSynthesisProviderTizen* _this = static_cast<PlatformSynthesisProviderTizen*>(userData);
    ASSERT(tts == _this->m_tts);
    if (_this->m_utterance)
        _this->m_platformSynthesizerObject->client()->speakingErrorOccurred(_this->m_utterance);
}

bool PlatformSynthesisProviderTizen::ttsSupportedVoiceCallback(tts_h /* tts */, const char* language, int voiceType, void* userData)
{
    ASSERT(userData);
    PlatformSynthesisProviderTizen* _this = static_cast<PlatformSynthesisProviderTizen*>(userData);
    ASSERT(tts == _this->m_tts);
    _this->addVoice(String(language), voiceType);
    return true;
}

PassOwnPtr<PlatformSynthesisProviderTizen> PlatformSynthesisProviderTizen::create(PlatformSpeechSynthesizer* platformSynthesizerObject)
{
    return adoptPtr(new PlatformSynthesisProviderTizen(platformSynthesizerObject));
}

void PlatformSynthesisProviderTizen::finishInitialization()
{
    m_initialized = true;
    if (m_utteranceWaiting)
        speakStoredUtterance();
    m_utteranceWaiting = false;
}

PlatformSynthesisProviderTizen::PlatformSynthesisProviderTizen(PlatformSpeechSynthesizer* platformSynthesizerObject)
    : m_platformSynthesizerObject(platformSynthesizerObject),
      m_initialized(false),
      m_tts(NULL),
      m_utteranceWaiting(false)
{
    int ret = tts_create(&m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to create TTS (%d)", ret);
#endif
        m_tts = NULL;
        return;
    }
    ret = tts_set_mode(m_tts, TTS_MODE_DEFAULT);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to set TTS mode (%d)", ret);
#endif
    }
    ret = tts_prepare(m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to prepare TTS (%d)", ret);
#endif
        m_tts = NULL;
        return;
    }
    char* currentLanguage = NULL;
    ret = tts_get_default_voice(m_tts, &currentLanguage, &m_defaultVoice);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to get default voice (%d)", ret);
#endif
        currentLanguage = NULL;
        m_defaultVoice = TTS_VOICE_TYPE_AUTO;
    }
    m_defaultLanguage = String(currentLanguage);
    if (!currentLanguage)
        free(currentLanguage);

    // Set callbacks
    ret = tts_set_state_changed_cb(m_tts, ttsStateChangedCallback, this);
    if (TTS_ERROR_NONE != ret) {
        TIZEN_LOGE("TTS: Unable to set state callback (%d)", ret);
    }
    ret = tts_set_utterance_started_cb(m_tts, ttsUtteranceStartedCallback, this);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to set utterance started callback (%d)", ret);
#endif
    }
    ret = tts_set_utterance_completed_cb(m_tts, ttsUtteranceCompletedCallback, this);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to set utterance completed callback (%d)", ret);
#endif
    }
    ret = tts_set_error_cb(m_tts, ttsErrorCallback, this);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to set error callback (%d)", ret);
#endif
    }

    ret = tts_foreach_supported_voices(m_tts, ttsSupportedVoiceCallback, this);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to get supported voices (%d)", ret);
#endif
    }
}

PlatformSynthesisProviderTizen::~PlatformSynthesisProviderTizen()
{
    int ret = tts_unset_state_changed_cb(m_tts) &&
            tts_unset_utterance_completed_cb(m_tts) &&
            tts_unset_error_cb(m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to unset callbacks (%d)", ret);
#endif
    }

    ret = tts_unprepare(m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to unprepare (%d)", ret);
#endif
    }
    tts_destroy(m_tts);
    m_tts = NULL;
}

void PlatformSynthesisProviderTizen::initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice> >& voiceList)
{
    voiceList = m_voiceList;
}

void PlatformSynthesisProviderTizen::speak(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
    ASSERT(!m_utterance);
    m_utterance = utterance;
    if(!m_initialized) {
        TIZEN_LOGE("TTS: Client is not prepared yet.");
        m_utteranceWaiting = true;
        return;
    }
    speakStoredUtterance();
}
void PlatformSynthesisProviderTizen::speakStoredUtterance()
{
    if (!m_utterance || m_utterance->text().isEmpty()) {
        return;
    }

    // FIXME: When web application doesn't set voice, we need to set fallback voice to pass data to tts.
    if (!m_utterance->voice()) {
        RefPtr<PlatformSpeechSynthesisVoice> fallbackVoice = PlatformSpeechSynthesisVoice::create();
        if (!m_utterance->lang().isEmpty())
            fallbackVoice->setLang(m_utterance->lang());
        m_utterance->setVoice(fallbackVoice.get());
    }

    String currentLanguage = m_defaultLanguage;

    // Check if the language passed from script is supported by the platform or not
    if (m_utterance->voice() && !m_utterance->voice()->lang().isEmpty()) {
        bool isSupportedLanguage = false;
        currentLanguage = m_utterance->voice()->lang();
        // Spec mandates usage of hyphen(-) but platform tts module supports only underscore(_)
        currentLanguage.replace("-", "_");
        for (unsigned i = 0; i < m_voiceList.size(); ++i) {
            if (m_voiceList[i]->lang() == currentLanguage) {
                isSupportedLanguage = true;
                break;
            }
        }
        if (!isSupportedLanguage) {
            TIZEN_LOGE("TTS: Unsupported language (%s)", m_utterance->voice()->lang().utf8().data());
            m_platformSynthesizerObject->client()->speakingErrorOccurred(m_utterance);
            return;
        }
    }
    int voiceType = m_defaultVoice;
    if (m_utterance->voice() && !m_utterance->voice()->name().isEmpty())
        voiceType = voiceNameToType(m_utterance->voice()->name());

    int utteranceId;
    int ret = tts_add_text(m_tts, m_utterance->text().utf8().data(), currentLanguage.utf8().data(), voiceType, TTS_SPEED_AUTO, &utteranceId);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to add text (%d)", ret);
#endif
        m_platformSynthesizerObject->client()->speakingErrorOccurred(m_utterance);
        return;
    }
    tts_state_e currentState;
    tts_get_state(m_tts, &currentState);
    if (TTS_STATE_PLAYING != currentState)
        ret = tts_play(m_tts);
    if(TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Play Error occured (%d)", ret);
#endif
        m_platformSynthesizerObject->client()->speakingErrorOccurred(m_utterance);
        return;
    }
}

void PlatformSynthesisProviderTizen::pause()
{
    if (!m_utterance)
        return;

    int ret = tts_pause(m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to pause (%d)", ret);
#endif
        m_platformSynthesizerObject->client()->speakingErrorOccurred(m_utterance);
    }
}

void PlatformSynthesisProviderTizen::resume()
{
    if (!m_utterance)
        return;

    int ret = tts_play(m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to resume (%d)", ret);
#endif
        m_platformSynthesizerObject->client()->speakingErrorOccurred(m_utterance);
    }
}

void PlatformSynthesisProviderTizen::cancel()
{
    if (!m_utterance)
        return;

    int ret = tts_stop(m_tts);
    if (TTS_ERROR_NONE != ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("TTS: Fail to cancel (%d)", ret);
#endif
        m_platformSynthesizerObject->client()->speakingErrorOccurred(m_utterance);
    }
    m_platformSynthesizerObject->client()->didFinishSpeaking(m_utterance);
    m_utterance = 0;
}

void PlatformSynthesisProviderTizen::addVoice(String language, int type)
{
    RefPtr<PlatformSpeechSynthesisVoice> voice = PlatformSpeechSynthesisVoice::create();
    String uri(URI_PREFIX);
    uri.append(voiceTypeToName(type));
    uri.append('/');
    uri.append(language);
    voice->setVoiceURI(uri);
    voice->setName(voiceTypeToName(type));
    voice->setLang(language);
    voice->setIsDefault( language == m_defaultLanguage);
    m_voiceList.append(voice);
}

}

#endif //ENABLE(SPEECH_SYNTHESIS)

