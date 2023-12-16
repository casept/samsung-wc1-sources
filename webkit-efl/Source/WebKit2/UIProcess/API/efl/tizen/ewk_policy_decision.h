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

#ifndef ewk_policy_decision_h
#define ewk_policy_decision_h

#include <Eina.h>

//Workaround
#include "ewk_view.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \enum   Ewk_Policy_Decision_Type
 * @brief   Provides option to policy decision type.
 */
enum _Ewk_Policy_Decision_Type {
    EWK_POLICY_DECISION_USE,
    EWK_POLICY_DECISION_DOWNLOAD,
    EWK_POLICY_DECISION_IGNORE
};
typedef enum _Ewk_Policy_Decision_Type Ewk_Policy_Decision_Type;

typedef struct _Ewk_Policy_Decision Ewk_Policy_Decision;

enum _Ewk_Policy_Navigation_Type {
    EWK_POLICY_NAVIGATION_TYPE_LINK_CLICKED = 0,
    EWK_POLICY_NAVIGATION_TYPE_FORM_SUBMITTED = 1,
    EWK_POLICY_NAVIGATION_TYPE_BACK_FORWARD = 2,
    EWK_POLICY_NAVIGATION_TYPE_RELOAD = 3,
    EWK_POLICY_NAVIGATION_TYPE_FORM_RESUBMITTED = 4,
    EWK_POLICY_NAVIGATION_TYPE_OTHER = 5
};
typedef enum _Ewk_Policy_Navigation_Type Ewk_Policy_Navigation_Type;

/**
 * Returns cookie from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c cookie string on success or empty string on failure
 */
EAPI const char* ewk_policy_decision_cookie_get(Ewk_Policy_Decision* policy_decision);

/**
 * Returns url from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c url string on success or empty string on failure
 */
EAPI const char* ewk_policy_decision_url_get(Ewk_Policy_Decision* policy_decision);

/**
 * Returns scheme from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c scheme string on success or empty string on failure
 */
EAPI const char* ewk_policy_decision_scheme_get(Ewk_Policy_Decision* policy_decision);

/**
 * Returns host from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c host string on success or empty string on failure
 */
EAPI const char* ewk_policy_decision_host_get(Ewk_Policy_Decision* policy_decision);

// #if ENABLE(TIZEN_POLICY_DECISION_HTTP_METHOD_GET)
/**
 * Returns http method from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c http method string on success or empty string on failure
 */
EAPI const char* ewk_policy_decision_http_method_get(Ewk_Policy_Decision* policyDecision);
// #endif

/**
 * Returns mimetype for response data from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c mimetype string on success or empty string on failure
 */
EAPI const char* ewk_policy_decision_response_mime_get(Ewk_Policy_Decision* policy_decision);

/**
 * Returns http headers for response data from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c http headers on success or NULL on failure
 */
EAPI const Eina_Hash* ewk_policy_decision_response_headers_get(Ewk_Policy_Decision* policy_decision);

/*#if ENABLE(TIZEN_ON_REDIRECTION_REQUESTED)*/
/**
 * Returns http status code from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c http status code number
 */
EAPI int ewk_policy_decision_response_status_code_get(Ewk_Policy_Decision* policyDecision);
/*#endif*/

/**
 * Returns policy type from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c policy type
 */
EAPI Ewk_Policy_Decision_Type ewk_policy_decision_type_get(const Ewk_Policy_Decision* policy_decision);

/**
 * Suspend the operation for policy decision.
 *
 * This suspends the operation for policy decision when the signal for policy is emitted.
 * This is very usefull to decide the policy from the additional UI operation like the popup.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_policy_decision_suspend(Ewk_Policy_Decision* policy_decision);

/**
 * Accept the action which triggers this decision.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_policy_decision_use(Ewk_Policy_Decision* policy_decision);

/**
 * Ignore the action which triggers this decision.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_policy_decision_ignore(Ewk_Policy_Decision* policy_decision);

/**
 * Cause a download from this decision.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_policy_decision_download(Ewk_Policy_Decision* policy_decision);

/**
 * Returns navigation type from Policy Decision object.
 *
 * @param policy_decision policy decsision object
 *
 * @return @c navigation type
 */
EAPI Ewk_Policy_Navigation_Type ewk_policy_decision_navigation_type_get(Ewk_Policy_Decision* policy_decision);

EAPI Ewk_Frame_Ref ewk_policy_decision_frame_get(Ewk_Policy_Decision* policy_decision);

#ifdef __cplusplus
}
#endif
#endif // ewk_policy_decision_h
