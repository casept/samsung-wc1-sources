/*
 *  Copyright (C) 2012 Samsung Electronics
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
#include "WebSpeechRecognitionClient.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "WebPage.h"
#include "WebProcess.h"
#include "WebPageProxyMessages.h"
#include <WebCore/AXObjectCache.h>
#include <WebCore/Frame.h>

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
#include "WebSpeechPermissionRequestManager.h"
#endif

using namespace WebCore;

namespace WebKit {

void WebSpeechRecognitionClient::start(SpeechRecognition* speechRecognition, const SpeechGrammarList* speechGrammarList, const String& lang, bool continuous, bool interimResults, unsigned long maxAlternatives)
{
    if (!m_provider)
        m_provider = SpeechRecognitionProviderTizen::create();

    m_provider->start(speechRecognition, speechGrammarList, lang, continuous, interimResults, maxAlternatives);
}

void WebSpeechRecognitionClient::stop(SpeechRecognition* speechRecognition)
{
    if (m_provider)
        m_provider->stop(speechRecognition);
}

void WebSpeechRecognitionClient::abort(SpeechRecognition* speechRecognition)
{
    if (m_provider)
        m_provider->abort(speechRecognition);
}

#if ENABLE(TIZEN_SCRIPTED_SPEECH)
static uint64_t generateRequestID()
{
    static uint64_t uniqueRequestID = 1;
    return uniqueRequestID++;
}

void WebSpeechRecognitionClient::requestPermissionToUseMicrophone(SpeechRecognition* recognition)
{
    m_speechRecognition = recognition;

    m_page->webSpeechPermissionRequestManager()->requestPermission(this);
}

void WebSpeechRecognitionClient::decidePermission(bool allowed)
{
    Permission permission = allowed ? PermissionAllowed : PermissionDenied;

    switch(permission) {
    case PermissionAllowed:
        if (!m_speechRecognition)
            return;

        m_speechRecognition->startReal();
        return;
    case PermissionDenied:
        // FIXME: What should we do when user denied to use microphone ?
        return;
    }
}

void WebSpeechRecognitionClient::stopWebSpeechOnBackgroundMode()
{
    if (m_speechRecognition)
        m_speechRecognition->stopFunction();
}
#endif
} // namespace WebKit

#endif // ENABLE(SCRIPTED_SPEECH)
