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
#include "SSLKeyGenerator.h"

#if ENABLE(TIZEN_KEYGEN)

#include "SSLPrivateKeyStoreTizen.h"
#include <openssl/asn1.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <wtf/text/CString.h>

namespace WebCore {

void getSupportedKeySizes(Vector<String>& supportedKeySizes)
{
    ASSERT(supportedKeySizes.isEmpty());
    supportedKeySizes.append(ASCIILiteral("2048 (High Grade)"));
    supportedKeySizes.append(ASCIILiteral("1024 (Medium Grade)"));
    supportedKeySizes.append(ASCIILiteral("512 (Low Grade)"));
}

String SSLErrorHandler(RSA* rsa, EVP_PKEY* pkey, NETSCAPE_SPKI* spki)
{
    NETSCAPE_SPKI_free(spki);
    EVP_PKEY_free(pkey);
    RSA_free(rsa);
    return String();
}

String signedPublicKeyAndChallengeString(unsigned keySizeIndex, const String& challengeString, const KURL& url)
{
    RSA* rsa = 0;
    NETSCAPE_SPKI* spki = 0;
    EVP_PKEY* pkey = 0;
    int keySize;

    switch (keySizeIndex) {
    case 0:
        keySize = 2048;
        break;
    case 1:
        keySize = 1024;
        break;
    case 2:
        keySize = 512;
        break;
    default:
        ASSERT_NOT_REACHED();
        return String();
    }

    pkey = EVP_PKEY_new();
    spki = NETSCAPE_SPKI_new();

    rsa = RSA_generate_key(keySize, 3, 0, 0);

    if (!(RSA_check_key(rsa) == 1))
        return SSLErrorHandler(rsa, pkey, spki);

    if (!EVP_PKEY_assign_RSA(pkey, rsa))
        return SSLErrorHandler(rsa, pkey, spki);

    SSLPrivateKeyStoreTizen::getInstance()->storePrivateKey(url, pkey);

    if (!NETSCAPE_SPKI_set_pubkey(spki, pkey))
        return SSLErrorHandler(0, pkey, spki);

    if (!ASN1_STRING_set((ASN1_STRING *)spki->spkac->challenge, challengeString.utf8().data(),  challengeString.length()))
        return SSLErrorHandler(0, pkey, spki);

    if (!NETSCAPE_SPKI_sign(spki, pkey, EVP_md5()))
        return SSLErrorHandler(0, pkey, spki);

    char* spkistr = NETSCAPE_SPKI_b64_encode(spki);
    String result(spkistr);
    OPENSSL_free(spkistr);

    return result;
}

}
#endif
