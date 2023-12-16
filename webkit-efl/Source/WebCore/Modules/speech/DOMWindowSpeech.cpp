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
#include "DOMWindowSpeech.h"

#if ENABLE(TIZEN_SCRIPTED_SPEECH)

#include "DOMWindow.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

DOMWindowSpeech::DOMWindowSpeech(DOMWindow* window)
    : DOMWindowProperty(window->frame())
{
}

DOMWindowSpeech::~DOMWindowSpeech()
{
}

DOMWindowSpeech* DOMWindowSpeech::from(DOMWindow* window)
{
    DOMWindowSpeech* supplement = static_cast<DOMWindowSpeech*>(Supplement<DOMWindow>::from(window, supplementName()));
    if (!supplement) {
        supplement = new DOMWindowSpeech(window);
        provideTo(window, supplementName(), adoptPtr(supplement));
    }
    return supplement;
}

const char* DOMWindowSpeech::supplementName()
{
	return "DOMWindowSpeech";
}

SpeechRecognition* DOMWindowSpeech::speechRecognition(ScriptExecutionContext* context, DOMWindow* window)
{
    return DOMWindowSpeech::from(window)->speechRecognition(context);
}

SpeechRecognition* DOMWindowSpeech::speechRecognition(ScriptExecutionContext* context)
{
    if (!m_speechRecognition && frame())
        m_speechRecognition = SpeechRecognition::create(context);
    return m_speechRecognition.get();
}

} // namespace WebCore

#endif // ENABLE(TIZEN_SCRIPTED_SPEECH)
