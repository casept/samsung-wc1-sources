/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioProcessingEvent_h
#define AudioProcessingEvent_h

#include "AudioBuffer.h"
#include "Event.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

#if ENABLE(TIZEN_WEB_AUDIO)
#include "ScriptProcessorNode.h"
#endif // ENABLE(TIZEN_WEB_AUDIO)

namespace WebCore {

class AudioBuffer;
    
class AudioProcessingEvent : public Event {
public:
    static PassRefPtr<AudioProcessingEvent> create();
#if ENABLE(TIZEN_WEB_AUDIO)
    static PassRefPtr<AudioProcessingEvent> create(ScriptProcessorNode* node, PassRefPtr<AudioBuffer> inputBuffer, PassRefPtr<AudioBuffer> outputBuffer, double time);
#else
    static PassRefPtr<AudioProcessingEvent> create(PassRefPtr<AudioBuffer> inputBuffer, PassRefPtr<AudioBuffer> outputBuffer);
#endif // ENABLE(TIZEN_WEB_AUDIO)
    
    virtual ~AudioProcessingEvent();

    AudioBuffer* inputBuffer() { return m_inputBuffer.get(); }
    AudioBuffer* outputBuffer() { return m_outputBuffer.get(); }

    virtual const AtomicString& interfaceName() const;

#if ENABLE(TIZEN_WEB_AUDIO)
    ScriptProcessorNode* node() { return m_node;}
    void setNode(ScriptProcessorNode* node) { m_node = node; }
    double playbackTime() { return m_playbackTime; }
#endif // ENABLE(TIZEN_WEB_AUDIO)

private:
    AudioProcessingEvent();
#if ENABLE(TIZEN_WEB_AUDIO)
    AudioProcessingEvent(ScriptProcessorNode* node, PassRefPtr<AudioBuffer> inputBuffer, PassRefPtr<AudioBuffer> outputBuffer, double time);
#else
    AudioProcessingEvent(PassRefPtr<AudioBuffer> inputBuffer, PassRefPtr<AudioBuffer> outputBuffer);
#endif // ENABLE(TIZEN_WEB_AUDIO)

    RefPtr<AudioBuffer> m_inputBuffer;
    RefPtr<AudioBuffer> m_outputBuffer;
#if ENABLE(TIZEN_WEB_AUDIO)
    ScriptProcessorNode* m_node;
    double m_playbackTime;
#endif // ENABLE(TIZEN_WEB_AUDIO)
};

} // namespace WebCore

#endif // AudioProcessingEvent_h
