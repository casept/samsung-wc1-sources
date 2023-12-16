/*
   Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_view_private.h"
#include "ewk_certificate.h"

#include "WKAPICast.h"
#include "WKAuthenticationChallenge.h"
#include "WKAuthenticationDecisionListener.h"
#include "WKCredential.h"
#include "WKProtectionSpace.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WebPageProxy.h"
#include <wtf/text/CString.h>

using namespace WebKit;

#include "ewk_context.h"
#if USE(SOUP)
#include "ResourceHandle.h"
#include <libsoup/soup.h>
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
struct _Ewk_Certificate_Policy_Decision {
    WKPageRef page;
    const char* url;
    const char* certificatePem;
    int error;
    bool isDecided;
    bool isSuspended;
};
#endif

Ewk_Certificate_Policy_Decision* ewkCertificatePolicyDecisionCreate(WKPageRef page, WKStringRef url, WKStringRef certificate, int error)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Ewk_Certificate_Policy_Decision* certificatePolicyDecision = new Ewk_Certificate_Policy_Decision;

    certificatePolicyDecision->page = page;
    certificatePolicyDecision->url = eina_stringshare_add(toImpl(url)->string().utf8().data());
    certificatePolicyDecision->certificatePem = eina_stringshare_add(toImpl(certificate)->string().utf8().data());
    certificatePolicyDecision->error = error;
    certificatePolicyDecision->isDecided = false;
    certificatePolicyDecision->isSuspended = false;

    return certificatePolicyDecision;
#else
    return 0;
#endif
}

void ewkCertificatePolicyDecisionDelete(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);

    if (certificatePolicyDecision->url)
        eina_stringshare_del(certificatePolicyDecision->url);
    if (certificatePolicyDecision->certificatePem)
        eina_stringshare_del(certificatePolicyDecision->certificatePem);

    delete certificatePolicyDecision;
#endif
}

bool ewkCertificatePolicyDecisionSuspended(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, false);
    return certificatePolicyDecision->isSuspended;
#else
    return false;
#endif
}

bool ewkCertificatePolicyDecisionDecided(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, false);
    return certificatePolicyDecision->isDecided;
#else
    return false;
#endif
}

void ewk_certificate_policy_decision_allowed_set(Ewk_Certificate_Policy_Decision* certificatePolicyDecision, Eina_Bool allowed)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);

    certificatePolicyDecision->isDecided = true;
    toImpl(certificatePolicyDecision->page)->replyPolicyForCertificateError(allowed);
#endif
}

void ewk_certificate_policy_decision_suspend(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);

    certificatePolicyDecision->isSuspended = true;
#endif
}

const char* ewk_certificate_policy_decision_url_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, 0);

    return certificatePolicyDecision->url;
#else
    return 0;
#endif
}

const char* ewk_certificate_policy_decision_certificate_pem_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, 0);

    return certificatePolicyDecision->certificatePem;
#else
    return 0;
#endif
}

int ewk_certificate_policy_decision_error_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, 0);

    return certificatePolicyDecision->error;
#else
    return 0;
#endif
}