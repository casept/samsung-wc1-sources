/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics
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

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include "UnitTestUtils/EWK2UnitTestServer.h"
#include "ewk_context_private.h"
#include "ewk_view_private.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static const char htmlReply[] = "<html><head><title>Foo</title></head><body>Bar</body></html>";

TEST_F(EWK2UnitTestBase, ewk_context_default_get)
{
    Ewk_Context* defaultContext = ewk_context_default_get();
    ASSERT_TRUE(defaultContext);
    ASSERT_EQ(defaultContext, ewk_context_default_get());
}

TEST_F(EWK2UnitTestBase, ewk_context_application_cache_manager_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    Ewk_Application_Cache_Manager* applicationCacheManager = ewk_context_application_cache_manager_get(context);
    ASSERT_TRUE(applicationCacheManager);
    ASSERT_EQ(applicationCacheManager, ewk_context_application_cache_manager_get(context));
}

TEST_F(EWK2UnitTestBase, ewk_context_cookie_manager_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    Ewk_Cookie_Manager* cookieManager = ewk_context_cookie_manager_get(context);
    ASSERT_TRUE(cookieManager);
    ASSERT_EQ(cookieManager, ewk_context_cookie_manager_get(context));
}

TEST_F(EWK2UnitTestBase, ewk_context_database_manager_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    Ewk_Database_Manager* databaseManager = ewk_context_database_manager_get(context);
    ASSERT_TRUE(databaseManager);
    ASSERT_EQ(databaseManager, ewk_context_database_manager_get(context));
}

TEST_F(EWK2UnitTestBase, ewk_context_favicon_database_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    Ewk_Favicon_Database* faviconDatabase = ewk_context_favicon_database_get(context);
    ASSERT_TRUE(faviconDatabase);
    ASSERT_EQ(faviconDatabase, ewk_context_favicon_database_get(context));
}

TEST_F(EWK2UnitTestBase, ewk_context_storage_manager_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    Ewk_Storage_Manager* storageManager = ewk_context_storage_manager_get(context);
    ASSERT_TRUE(storageManager);
    ASSERT_EQ(storageManager, ewk_context_storage_manager_get(context));
}

static void schemeRequestCallback(Ewk_Url_Scheme_Request* request, void* userData)
{
    const char* scheme = ewk_url_scheme_request_scheme_get(request);
    ASSERT_STREQ("fooscheme", scheme);
    const char* url = ewk_url_scheme_request_url_get(request);
    ASSERT_STREQ("fooscheme:MyPath", url);
    const char* path = ewk_url_scheme_request_path_get(request);
    ASSERT_STREQ("MyPath", path);
    ASSERT_TRUE(ewk_url_scheme_request_finish(request, htmlReply, strlen(htmlReply), "text/html"));
}

TEST_F(EWK2UnitTestBase, ewk_context_url_scheme_register)
{
    ewk_context_url_scheme_register(ewk_view_context_get(webView()), "fooscheme", schemeRequestCallback, 0);
    ASSERT_TRUE(loadUrlSync("fooscheme:MyPath"));
    ASSERT_STREQ("Foo", ewk_view_title_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_context_cache_model)
{
    Ewk_Context* context = ewk_view_context_get(webView());

    ASSERT_EQ(EWK_CACHE_MODEL_DOCUMENT_VIEWER, ewk_context_cache_model_get(context));

    ASSERT_TRUE(ewk_context_cache_model_set(context, EWK_CACHE_MODEL_DOCUMENT_BROWSER));
    ASSERT_EQ(EWK_CACHE_MODEL_DOCUMENT_BROWSER, ewk_context_cache_model_get(context));

    ASSERT_TRUE(ewk_context_cache_model_set(context, EWK_CACHE_MODEL_PRIMARY_WEBBROWSER));
    ASSERT_EQ(EWK_CACHE_MODEL_PRIMARY_WEBBROWSER, ewk_context_cache_model_get(context));

    ASSERT_TRUE(ewk_context_cache_model_set(context, EWK_CACHE_MODEL_DOCUMENT_VIEWER));
    ASSERT_EQ(EWK_CACHE_MODEL_DOCUMENT_VIEWER, ewk_context_cache_model_get(context));
}

TEST_F(EWK2UnitTestBase, ewk_context_new)
{
    Ewk_Context* context = ewk_context_new();
    ASSERT_TRUE(context);
    ewk_object_unref(context);
}

TEST_F(EWK2UnitTestBase, ewk_context_new_with_injected_bundle_path)
{
    Ewk_Context* context = ewk_context_new_with_injected_bundle_path(environment->injectedBundleSample());
    ASSERT_TRUE(context);
    ewk_object_unref(context);
}

TEST_F(EWK2UnitTestBase, ewk_context_additional_plugin_path_set)
{
    Ewk_Context* context = ewk_view_context_get(webView());

    ASSERT_FALSE(ewk_context_additional_plugin_path_set(context, 0));

    ASSERT_TRUE(ewk_context_additional_plugin_path_set(context, "/plugins"));

    /* FIXME: Get additional plugin path and compare with the path. */
}

static char* s_acceptLanguages = 0;

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);
    s_acceptLanguages = strdup(soup_message_headers_get_one(message->request_headers, "Accept-Language"));

    soup_message_body_append(message->response_body, SOUP_MEMORY_COPY, s_acceptLanguages, strlen(s_acceptLanguages));
    soup_message_body_complete(message->response_body);
}

TEST_F(EWK2UnitTestBase, ewk_context_preferred_languages)
{
    Eina_List* acceptLanguages;
    acceptLanguages = eina_list_append(acceptLanguages, "ko_kr");
    acceptLanguages = eina_list_append(acceptLanguages, "fr");
    acceptLanguages = eina_list_append(acceptLanguages, "en");

    ewk_context_preferred_languages_set(acceptLanguages);

    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallback);

    ASSERT_TRUE(loadUrlSync(httpServer->getURLForPath("/index.html").data()));
    ASSERT_STREQ("ko-kr, fr;q=0.90, en;q=0.80", s_acceptLanguages);
    free(s_acceptLanguages);

    ewk_context_preferred_languages_set(0);
    ASSERT_TRUE(loadUrlSync(httpServer->getURLForPath("/index.html").data()));
    ASSERT_STREQ("en-US", s_acceptLanguages);
    free(s_acceptLanguages);
}
