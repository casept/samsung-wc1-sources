/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
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
#include "WebErrors.h"

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
#include "WebError.h"
#include "WKError.h"
#include <WebCore/ErrorsEfl.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/text/CString.h>

#include <libsoup/soup.h>
#include <gio/gio.h>

using namespace WebCore;

namespace WebKit {

ResourceError cancelledError(const ResourceRequest& request)
{
    return ResourceError(g_quark_to_string(SOUP_HTTP_ERROR), SOUP_STATUS_CANCELLED,
            request.url().string(), "Request cancelled");
}

ResourceError blockedError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotUseRestrictedPort,
            request.url().string(), "Request blocked");
}

ResourceError cannotShowURLError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotShowURL,
            request.url().string(), "Cannot show URL");
}

ResourceError interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeFrameLoadInterruptedByPolicyChange,
            request.url().string(), "Frame load interrupted by policy change");
}

ResourceError cannotShowMIMETypeError(const ResourceResponse& response)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodeCannotShowMIMEType,
            response.url().string(), "Cannot show mimetype");
}

ResourceError fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError(g_quark_to_string(G_IO_ERROR), G_IO_ERROR_NOT_FOUND,
            response.url().string(), "File does not exist");
}

ResourceError pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(WebError::webKitErrorDomain(), kWKErrorCodePlugInWillHandleLoad,
            response.url().string(), "Plugin will handle load");
}

#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)

} // namespace WebKit
