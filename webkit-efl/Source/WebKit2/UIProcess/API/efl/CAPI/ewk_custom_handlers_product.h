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

/**
 * @file    ewk_custom_handlers.h
 * @brief   Custom scheme and content handlers. (http://www.w3.org/TR/html5/timers.html#custom-handlers)
 */

#ifndef ewk_custom_handlers_product_h
#define ewk_custom_handlers_product_h

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EwkObject Ewk_Custom_Handlers_Data;

/// Defines the handler states.
enum _Ewk_Custom_Handlers_State {
    /// Indicates that no attempt has been made to register the given handler.
    EWK_CUSTOM_HANDLERS_NEW,
    /// Indicates that the given handler has been registered or that the site is blocked from registering the handler.
    EWK_CUSTOM_HANDLERS_REGISTERED,
    /// Indicates that the given handler has been offered but was rejected.
    EWK_CUSTOM_HANDLERS_DECLINED
};

/// Creates a type name for @a _Ewk_Custom_Handlers_State.
typedef enum _Ewk_Custom_Handlers_State Ewk_Custom_Handlers_State;

/**
 * Get target(scheme or mime type) of custom handlers.
 *
 * @param data custom handlers's structure.
 *
 * @return @c target (scheme or mime type).
 */
EAPI const char* ewk_custom_handlers_data_target_get(Ewk_Custom_Handlers_Data* data);

/**
 * Get base url of custom handlers.
 *
 * @param data custom handlers's structure.
 *
 * @return @c base url.
 */
EAPI const char* ewk_custom_handlers_data_base_url_get(Ewk_Custom_Handlers_Data* data);

/**
 * Get url of custom handlers.
 *
 * @param data custom handlers's structure.
 *
 * @return @c url.
 */
EAPI const char* ewk_custom_handlers_data_url_get(Ewk_Custom_Handlers_Data* data);

/**
 * Get title of custom handlers.
 *
 * @param data custom handlers's structure.
 *
 * @return @c title.
 */
EAPI const char* ewk_custom_handlers_data_title_get(Ewk_Custom_Handlers_Data* data);

/**
 * Set result of isProtocolRegistered API.
 *
 * @param data custom handlers's structure
 * @param state(Ewk_Custom_Handlers_State) of isProtocolRegistered and isContentRegistered API
 */
EAPI void ewk_custom_handlers_data_result_set(Ewk_Custom_Handlers_Data* data, Ewk_Custom_Handlers_State state);

#ifdef __cplusplus
}
#endif

#endif // ewk_custom_handlers_product_h
