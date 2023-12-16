/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "ewk_error.h"

#include "ErrorsEfl.h"
#include "WKError.h"
#include "WKString.h"
#include "WKURL.h"
#include "ewk_error_private.h"

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
#include <libintl.h>
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
#include "WebError.h"
#include "WKPage.h"
#include "WKSharedAPICast.h"
#include <WebCore/FileSystem.h>
#include <wtf/text/CString.h>
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-request.h>
#include <libsoup/soup.h>

using namespace WebKit;
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)

using namespace WebCore;

EwkError::EwkError(WKErrorRef errorRef)
    : m_wkError(errorRef)
    , m_url(AdoptWK, WKErrorCopyFailingURL(errorRef))
    , m_description(AdoptWK, WKErrorCopyLocalizedDescription(errorRef))
{ }

const char* EwkError::url() const
{
    return m_url;
}

const char* EwkError::description() const
{
    return m_description;
}

WKRetainPtr<WKStringRef> EwkError::domain() const
{
    return adoptWK(WKErrorCopyDomain(m_wkError.get()));
}

int EwkError::errorCode() const
{
    return WKErrorGetErrorCode(m_wkError.get());
}

bool EwkError::isCancellation() const
{
    return WKStringIsEqualToUTF8CString(domain().get(), errorDomainNetwork) && errorCode() == NetworkErrorCancelled;
}

Ewk_Error_Type ewk_error_type_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, EWK_ERROR_TYPE_NONE);

    WKRetainPtr<WKStringRef> wkErrorDomain = error->domain();

    if (WKStringIsEqualToUTF8CString(wkErrorDomain.get(), errorDomainNetwork))
        return EWK_ERROR_TYPE_NETWORK;
    if (WKStringIsEqualToUTF8CString(wkErrorDomain.get(), errorDomainPolicy))
        return EWK_ERROR_TYPE_POLICY;
    if (WKStringIsEqualToUTF8CString(wkErrorDomain.get(), errorDomainPlugin))
        return EWK_ERROR_TYPE_PLUGIN;
    if (WKStringIsEqualToUTF8CString(wkErrorDomain.get(), errorDomainDownload))
        return EWK_ERROR_TYPE_DOWNLOAD;
    if (WKStringIsEqualToUTF8CString(wkErrorDomain.get(), errorDomainPrint))
        return EWK_ERROR_TYPE_PRINT;

    return EWK_ERROR_TYPE_INTERNAL;
}

const char* ewk_error_url_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->url();
}

int ewk_error_code_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->errorCode();
}

const char* ewk_error_description_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->description();
}

Eina_Bool ewk_error_cancellation_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, false);

    return error->isCancellation();
}

#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
#pragma GCC diagnostic ignored "-Wformat-extra-args"
void ewk_error_load_error_page(Ewk_Error* error, WKPageRef page)
{
    // if error is null, it is not a error.
    EINA_SAFETY_ON_NULL_RETURN(error);

    int errorCode = error->errorCode();
    // error_code == 0  is ok, error_code == -999 is request cancelled
    CString errorDomain = toWTFString(error->domain().get()).utf8();
    if (!strcmp(errorDomain.data(), g_quark_to_string(SOUP_HTTP_ERROR))
        && (!errorCode || errorCode == EWK_ERROR_NETWORK_STATUS_CANCELLED))
            return;

    // webDEV error code but not a error
    if (!strcmp(errorDomain.data(), WebError::webKitErrorDomain().utf8().data())) {
        if (errorCode == EWK_ERROR_CODE_CANNOTSHOWMIMETYPE || errorCode == EWK_ERROR_CODE_FRAMELOADINTERRUPTEDBYPOLICYCHANGE || errorCode == EWK_ERROR_CODE_PLUGINWILLHANDLELOAD)
            return;
    }

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    bindtextdomain("WebKit", WEBKIT_TEXT_DIR);
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)

    // make Error Page Source
    String errorPageFile = WEBKIT_HTML_DIR"/errorPage.html";
    long long fileSize = 0;
    if (!getFileSize(errorPageFile, fileSize) || fileSize <= 0)
        return;

    PlatformFileHandle errorHtmlFile = openFile(errorPageFile, OpenForRead);

    OwnArrayPtr<char> arrayHtmlSource = adoptArrayPtr(new char[fileSize]);
    if (readFromFile(errorHtmlFile, arrayHtmlSource.get(), fileSize) != fileSize) {
        closeFile(errorHtmlFile);
        return;
    }
    closeFile(errorHtmlFile);

    String htmlSource(arrayHtmlSource.get(), fileSize);

    gboolean isWebKitCannotShowUrlError = !strcmp(errorDomain.data(), WebError::webKitErrorDomain().utf8().data()) && (errorCode == EWK_ERROR_CODE_CANNOTSHOWURL);
    if (isWebKitCannotShowUrlError) {
        int startIndex = htmlSource.find("<!--errorMessage-->");
        int endIndex = htmlSource.reverseFind("<!--errorSuggestions-->");
        if ((startIndex != -1) && (endIndex-startIndex > 0))
            htmlSource = htmlSource.replace(startIndex, endIndex-startIndex, String());
    }

    if (!strcmp(errorDomain.data(), g_quark_to_string(SOUP_HTTP_ERROR))
        && errorCode >= SOUP_STATUS_CANT_RESOLVE
        && errorCode <= SOUP_STATUS_SSL_FAILED) {
#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
        htmlSource = htmlSource.replace("%errorPageTitle%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_WEB_PAGE_NOT_AVAILABLE")));
        htmlSource = htmlSource.replace("%errorMessageTitle%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_WEB_PAGE_NOT_AVAILABLE")));
    } else if (isWebKitCannotShowUrlError) {
        htmlSource = htmlSource.replace("%errorPageTitle%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_WEB_PAGE_NOT_AVAILABLE")));
        htmlSource = htmlSource.replace("%errorMessageTitle%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_WEB_PAGE_NOT_AVAILABLE")));
    } else {
        htmlSource = htmlSource.replace("%errorPageTitle%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_WEB_PAGE_TEMPORARILY_NOT_AVAILABLE")));
        htmlSource = htmlSource.replace("%errorMessageTitle%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_WEB_PAGE_TEMPORARILY_NOT_AVAILABLE")));
    }
#else
        htmlSource = htmlSource.replace("%errorPageTitle%", "Error page");
        htmlSource = htmlSource.replace("%errorMessageTitle%", "Error page");
    } else if (isWebKitCannotShowUrlError) {
        htmlSource = htmlSource.replace("%errorPageTitle%", "Web page not available");
        htmlSource = htmlSource.replace("%errorMessageTitle%", "Web page not available");
    } else {
        htmlSource = htmlSource.replace("%errorPageTitle%", "Web page temporarily not available");
        htmlSource = htmlSource.replace("%errorMessageTitle%", "Web page temporarily not available");
    }
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)

    String urlLink = String::format("<a style=word-break:break-all href=%s>%s</a>", error->url(), error->url());
    char *errorMessage = NULL;
    errorMessage = (char *) malloc (urlLink.length() + 128 + 1);

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
    snprintf(errorMessage, urlLink.length() + 128 + 1, dgettext("WebKit", "IDS_WEBVIEW_BODY_WHILE_RETRIEVING_WEB_PAGE_PS_FOLLOWING_ERROR_OCCURRED"), urlLink.utf8().data());

    htmlSource = htmlSource.replace("%errorMessage%", String::fromUTF8(errorMessage));
    htmlSource = htmlSource.replace("%errorCause%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_UNABLE_TO_RETRIEVE_WEB_PAGE_HWEB_PAGE_MIGHT_BE_TEMPORARILY_DOWN_OR_HAVE_MOVED_TO_NEW_URL")));
    htmlSource = htmlSource.replace("%errorSuggestionsHeader%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_THE_MOST_LIKELY_CAUSE_IS_GIVEN_BELOW")));
    htmlSource = htmlSource.replace("%errorSuggestions1%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_NETWORK_CONNECTION_NOT_ESTABLISHED_NORMALLY")));
    htmlSource = htmlSource.replace("%errorSuggestions2%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_CHECK_WEB_PAGE_URL")));
    htmlSource = htmlSource.replace("%errorSuggestions3%", String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_RELOAD_WEB_PAGE_LATER")));
#else
    snprintf(errorMessage,  urlLink.length() + 128 + 1, "While retrieving Web page %s, following error occurred.", urlLink.utf8().data());

    htmlSource = htmlSource.replace("%errorMessage%", String::fromUTF8(errorMessage));
    htmlSource = htmlSource.replace("%errorCause%", "Unable to retrieve Web page. (Web page might be temporarily down or have moved to new URL)");
    htmlSource = htmlSource.replace("%errorSuggestionsHeader%", "The most likely cause is given below");
    htmlSource = htmlSource.replace("%errorSuggestions1%", "Network connection not established normally");
    htmlSource = htmlSource.replace("%errorSuggestions2%", "Check Web page URL");
    htmlSource = htmlSource.replace("%errorSuggestions3%", "Reload Web page later");
#endif // ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)

    WKRetainPtr<WKStringRef> htmlString(AdoptWK, WKStringCreateWithUTF8CString(htmlSource.utf8().data()));
    WKRetainPtr<WKURLRef> baseURL(AdoptWK, WKURLCreateWithUTF8CString(error->url()));
    WKRetainPtr<WKURLRef> unreachableURL(AdoptWK, WKURLCreateWithUTF8CString(error->url()));
    WKPageLoadAlternateHTMLString(page, htmlString.get(), baseURL.get(), unreachableURL.get());
    if (errorMessage)
        free (errorMessage);
}
#pragma GCC diagnostic warning "-Wformat-extra-args"
#endif // ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
