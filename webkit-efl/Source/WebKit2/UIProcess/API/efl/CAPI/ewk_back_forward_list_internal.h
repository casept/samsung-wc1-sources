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
 * @file    ewk_back_forward_list.h
 * @brief   Describes the Ewk Back Forward List API.
 */

#ifndef ewk_back_forward_list_interal_h
#define ewk_back_forward_list_interal_h

#include "ewk_back_forward_list_item.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates the list containing the items preceding the current item.
 *
 * The @c Ewk_Back_Forward_List_Item elements are located in the result list starting with the oldest one.
 *
 * @param list the back-forward list instance
 *
 * @return @c Eina_List containing @c Ewk_Back_Forward_List_Item elements or @c NULL in case of error,
 *            the Eina_List and its items should be freed after use. Use ewk_object_unref()
 *            to free the items
 *
 * @see ewk_back_forward_list_n_back_items_copy
 */
#define ewk_back_forward_list_back_items_copy(list) \
    ewk_back_forward_list_n_back_items_copy(list, -1)

/**
 * Creates the list containing the items following the current item.
 *
 * The @c Ewk_Back_Forward_List_Item elements are located in the result list starting with the oldest one.
 *
 * @param list the back-forward list instance
 *
 * @return @c Eina_List containing @c Ewk_Back_Forward_List_Item elements or @c NULL in case of error,
 *            the Eina_List and its items should be freed after use. Use ewk_object_unref()
 *            to free the items
 *
 * @see ewk_back_forward_list_n_forward_items_copy
 */
#define ewk_back_forward_list_forward_items_copy(list) \
    ewk_back_forward_list_n_forward_items_copy(list, -1)

#ifdef __cplusplus
}
#endif
#endif // ewk_back_forward_list_h
