/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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
#include "CDMPrivateClearKeyTizen.h"

#if ENABLE(ENCRYPTED_MEDIA_V2) && ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)

#include "CDM.h"
#include "ExceptionCode.h"
#include "GStreamerUtilities.h"
#include "MediaPlayer.h"
#include "UUID.h"
#include <gst/gst.h>

namespace WebCore {

class CDMSessionClearKey : public CDMSession {
public:
    CDMSessionClearKey(CDMPrivateClearKey* parent);
    virtual ~CDMSessionClearKey() { }

    virtual const String& sessionId() const OVERRIDE { return m_sessionId; }
    virtual PassRefPtr<Uint8Array> generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode) OVERRIDE;
    virtual void releaseKeys() OVERRIDE;
    virtual bool update(Uint8Array*, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode) OVERRIDE;
    virtual void setSourceBuffer(SourceBuffer* buffer) OVERRIDE { m_sourceBuffer = buffer; }

protected:
    CDMPrivateClearKey* m_parent;
    String m_sessionId;
    static unsigned s_sessionCounter;
    RefPtr<Uint8Array> m_initData;
    String m_mimeType;
    SourceBuffer* m_sourceBuffer;
//    RetainPtr<AVAssetResourceLoadingRequest> m_request;
};

unsigned CDMSessionClearKey::s_sessionCounter = 0;

bool CDMPrivateClearKey::supportsKeySystem(const String& keySystem)
{
    return equalIgnoringCase(keySystem, "webkit-org.w3.clearkey");
}

bool CDMPrivateClearKey::supportsKeySystemAndMimeType(const String& keySystem, const String& mimeType)
{
    if (!supportsKeySystem(keySystem))
        return false;
    return mimeType.startsWith("video/mp4", false) || mimeType.startsWith("audio/mp4", false) || mimeType.startsWith("video/webm", false) || mimeType.startsWith("audio/webm", false);
}

bool CDMPrivateClearKey::supportsMIMEType(const String& mimeType)
{
    return mimeType.startsWith("video/mp4", false) || mimeType.startsWith("audio/mp4", false) || mimeType.startsWith("video/webm", false) || mimeType.startsWith("audio/webm", false);
}

PassOwnPtr<CDMSession> CDMPrivateClearKey::createSession()
{
    return adoptPtr(new CDMSessionClearKey(this));
}

CDMSessionClearKey::CDMSessionClearKey(CDMPrivateClearKey* parent)
    : m_parent(parent)
    , m_sessionId(String::number(++s_sessionCounter))
    , m_initData(0)
    , m_mimeType(" ")
{
}

PassRefPtr<Uint8Array> CDMSessionClearKey::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& /* destinationURL */, unsigned short& errorCode, unsigned long& systemCode)
{
    UNUSED_PARAM(systemCode);
    LOG_MEDIA_MESSAGE("[EME] generateKeyRequest is called for clearKey");

    if (!initData || !initData->byteLength()) {
        errorCode = INVALID_STATE_ERR;
        return RefPtr<Uint8Array> (0);
    }
    LOG_MEDIA_MESSAGE("[EME] generateKeyRequest is called for clearKey OK: len: %d, initData: %s", initData->length(), reinterpret_cast<char*>(initData->data()));

    String keySystem("webkit-org.w3.clearkey");

    m_initData = Uint8Array::create(initData->data(), initData->length());
    m_mimeType = mimeType;

    if (m_sourceBuffer)
        return m_sourceBuffer->generateKeyRequest(keySystem, initData);
    else
        return m_parent->cdm()->mediaPlayer()->generateKeyRequest(mimeType, keySystem, initData);
}

void CDMSessionClearKey::releaseKeys()
{
}

bool CDMSessionClearKey::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode)
{
    LOG_MEDIA_MESSAGE("[EME] update is called for clearKey");
    UNUSED_PARAM(errorCode);
    UNUSED_PARAM(systemCode);
    nextMessage = 0;

    if (!key)
        return false;

    if (!m_initData.get())
        return false;

    String keySystem("webkit-org.w3.clearkey");
    if (m_sourceBuffer)
        return m_sourceBuffer->updateKey(keySystem, key, m_initData.get());
    else
        return m_parent->cdm()->mediaPlayer()->updateKey(m_mimeType, keySystem, key, m_initData.get());
}

}

#endif // ENABLE(ENCRYPTED_MEDIA_V2) && ENABLE(TIZEN_ENCRYPTED_MEDIA_V2)
