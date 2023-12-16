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

#ifndef SSLPrivateKeyStoreTizen_h
#define SSLPrivateKeyStoreTizen_h

#if ENABLE(TIZEN_KEYGEN)

#include "KURL.h"
#include <openssl/evp.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

namespace WebCore {
class SSLPrivateKeyStoreTizen : public RefCounted<SSLPrivateKeyStoreTizen> {
private:
    SSLPrivateKeyStoreTizen() { };

public:
    ~SSLPrivateKeyStoreTizen();

    static SSLPrivateKeyStoreTizen* getInstance();

    void storePrivateKey(const KURL&, EVP_PKEY*);
    EVP_PKEY* fetchPrivateKey(const KURL&);

private:
    typedef WTF::HashMap<String, EVP_PKEY*> PrivateKeyMap;
    PrivateKeyMap m_privateKeyMap;
};
} // namespace WebCore

#endif // ENABLE(TIZEN_KEYGEN)

#endif // SSLPrivateKeyStore_h
