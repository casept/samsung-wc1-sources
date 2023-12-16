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
#ifndef DOMWindowSpeech_h
#define DOMWindowSpeech_h

#if ENABLE(TIZEN_SCRIPTED_SPEECH)

#include "DOMWindowProperty.h"
#include "SpeechRecognition.h"
#include "Supplementable.h"

namespace WebCore {

class DOMWindow;
class ScriptExecutionContext;

class DOMWindowSpeech : public Supplement<DOMWindow>, public DOMWindowProperty {
public:
    virtual ~DOMWindowSpeech();

    static SpeechRecognition* speechRecognition(ScriptExecutionContext*, DOMWindow*);
    static DOMWindowSpeech* from(DOMWindow*);
    static const char* supplementName();

private:
    explicit DOMWindowSpeech(DOMWindow*);

    SpeechRecognition* speechRecognition(ScriptExecutionContext*);

    RefPtr<SpeechRecognition> m_speechRecognition;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_SCRIPTED_SPEECH)

#endif // DOMWindowSpeech_h
