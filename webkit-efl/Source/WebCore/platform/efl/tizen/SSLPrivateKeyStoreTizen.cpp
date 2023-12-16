/*
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SSLPrivateKeyStoreTizen.h"

#if ENABLE(TIZEN_KEYGEN)
#include <utility>

namespace WebCore {

static SSLPrivateKeyStoreTizen* s_SSLPrivateKeyStoreTizen = 0;
static Mutex s_SSLPrivateKeyStoreMutex;

SSLPrivateKeyStoreTizen::~SSLPrivateKeyStoreTizen()
{
    PrivateKeyMap::const_iterator end = m_privateKeyMap.end();
    for (PrivateKeyMap::const_iterator i = m_privateKeyMap.begin(); i != end; ++i)
        EVP_PKEY_free((EVP_PKEY*)(i->value));

    m_privateKeyMap.clear();
}

SSLPrivateKeyStoreTizen* SSLPrivateKeyStoreTizen::getInstance()
{
    if (!s_SSLPrivateKeyStoreTizen) {
        MutexLocker locker(s_SSLPrivateKeyStoreMutex);

        if (!s_SSLPrivateKeyStoreTizen)
            s_SSLPrivateKeyStoreTizen = new SSLPrivateKeyStoreTizen();
    }

    return s_SSLPrivateKeyStoreTizen;
}

void SSLPrivateKeyStoreTizen::storePrivateKey(const KURL& url, EVP_PKEY* pkey)
{
    EVP_PKEY* oldPkey = static_cast<EVP_PKEY*>(m_privateKeyMap.take(url.host()));
    if (oldPkey)
        EVP_PKEY_free(oldPkey);

    m_privateKeyMap.set(url.host(), pkey);
}

EVP_PKEY* SSLPrivateKeyStoreTizen::fetchPrivateKey(const KURL& url)
{
    return (EVP_PKEY*)m_privateKeyMap.get(url.host());
}

}

#endif // ENABLE(TIZEN_KEYGEN)
