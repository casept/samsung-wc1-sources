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
#include "ewk_custom_handlers.h"

#include "ewk_custom_handlers_private.h"
#include <wtf/text/CString.h>

EwkCustomHandlers::EwkCustomHandlers(const String& target, const String& baseURL, const String& url, const String& title)
    : m_target(target.utf8().data())
    , m_baseURL(baseURL.utf8().data())
    , m_url(url.utf8().data())
    , m_title(title.utf8().data())
    , m_state(EWK_CUSTOM_HANDLERS_DECLINED)
{
}

const char* ewk_custom_handlers_data_target_get(Ewk_Custom_Handlers_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkCustomHandlers, data, impl, 0);

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    return impl->target();
#else
    return 0;
#endif
}

const char* ewk_custom_handlers_data_base_url_get(Ewk_Custom_Handlers_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkCustomHandlers, data, impl, 0);

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    return impl->baseURL();
#else
    return 0;
#endif
}

const char* ewk_custom_handlers_data_url_get(Ewk_Custom_Handlers_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkCustomHandlers, data, impl, 0);

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    return impl->url();
#else
    return 0;
#endif
}

const char* ewk_custom_handlers_data_title_get(Ewk_Custom_Handlers_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkCustomHandlers, data, impl, 0);

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    return impl->title();
#else
    return 0;
#endif
}

void ewk_custom_handlers_data_result_set(Ewk_Custom_Handlers_Data* data, Ewk_Custom_Handlers_State state)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkCustomHandlers, data, impl);

#if ENABLE(TIZEN_NAVIGATOR_CONTENT_UTILS)
    impl->setState(state);
#else
    UNUSED_PARAM(state);
#endif
}
