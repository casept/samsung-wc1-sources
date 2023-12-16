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

/**
 * @file    ewk_cookie_manager.h
 * @brief   Describes the Ewk Cookie Manager API.
 */

#ifndef ewk_cookie_manager_h
#define ewk_cookie_manager_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Creates a type name for Ewk_Cookie_Manager
 */
typedef struct Ewk_Cookie_Manager Ewk_Cookie_Manager;

/**
 * \enum    Ewk_Cookie_Accept_Policy
 *
 * @brief   Contains accept policies for the cookies.
 */
enum Ewk_Cookie_Accept_Policy {
    /// Accepts every cookie sent from any page.
    EWK_COOKIE_ACCEPT_POLICY_ALWAYS,
    /// Rejects all cookies.
    EWK_COOKIE_ACCEPT_POLICY_NEVER,
    /// Accepts only cookies set by the main document loaded.
    EWK_COOKIE_ACCEPT_POLICY_NO_THIRD_PARTY
};

/**
 * @brief Creates a type name for the Ewk_Cookie_Accept_Policy.
 */
typedef enum Ewk_Cookie_Accept_Policy Ewk_Cookie_Accept_Policy;

/**
 * @brief Set @a policy as the cookie acceptance policy for @a manager.
 *
 * @details By default, only cookies set by the main document loaded are accepted.
 *
 * @param[in] manager The cookie manager to update
 * @param[in] policy a #Ewk_Cookie_Accept_Policy
 */
EAPI void ewk_cookie_manager_accept_policy_set(Ewk_Cookie_Manager *manager, Ewk_Cookie_Accept_Policy policy);

/**
 * @typedef Ewk_Cookie_Manager_Policy_Async_Get_Cb Ewk_Cookie_Manager_Policy_Async_Get_Cb
 * @brief Callback type for use with ewk_cookie_manager_accept_policy_async_get
 *
 * @param[in] policy a #Ewk_Cookie_Accept_Policy
 * @param[in] event_info user data
 */
typedef void (*Ewk_Cookie_Manager_Policy_Async_Get_Cb)(Ewk_Cookie_Accept_Policy policy, void *event_info);

/**
 * @brief Asynchronously get the cookie acceptance policy of @a manager.
 *
 * @details By default, only cookies set by the main document loaded are accepted.
 *
 * @param[in] manager The cookie manager to query
 * @param[in] callback The function to call when the policy is received
 * @param[in] data User data (may be @c NULL)
 */
EAPI void ewk_cookie_manager_accept_policy_async_get(const Ewk_Cookie_Manager *manager, Ewk_Cookie_Manager_Policy_Async_Get_Cb callback, void *data);

/**
 * @brief Delete all cookies of @a manager.
 *
 * @param[in] manager The cookie manager to update
 */
EAPI void ewk_cookie_manager_cookies_clear(Ewk_Cookie_Manager *manager);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_cookie_manager_h
