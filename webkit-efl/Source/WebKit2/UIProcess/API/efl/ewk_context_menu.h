/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_context_menu.h
 * @brief   Describes the Ewk Context Menu API.
 */

#ifndef ewk_context_menu_h
#define ewk_context_menu_h

#include "ewk_defines.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new Ewk_Context_Menu to be used as a submenu of an existing
 * Ewk_Context_Menu. The context menu is created by the ewk_view and
 * passed as an argument of ewk_view smart callback.
 *
 * @return the pointer to the new context menu
 *
 * @see ewk_context_menu_new_with_items
 */
EAPI Ewk_Context_Menu *ewk_context_menu_new(void);

/**
 * Creates a new Ewk_Context_Menu to be used as a submenu of an existing
 * Ewk_Context_Menu with the given initial items. The context menu is
 * created by the ewk_view and passed as an argument of ewk_view smart callback.
 *
 * @param items the list of initial items
 * @return the pointer to the new context menu
 *
 * @see ewk_context_menu_new
 */
EAPI Ewk_Context_Menu *ewk_context_menu_new_with_items(Eina_List *items);

/**
 * Appends the item of the context menu.
 *
 * @param menu the context menu 
 * @param item the item to append
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_append_with_object(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

/**
 * Removes the item of the context menu.
 *
 * @param menu the context menu 
 * @param item the item to remove
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_remove(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

/**
 * Hides the context menu.
 *
 * @param menu the context menu to hide
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_hide(Ewk_Context_Menu *menu);

/**
 * Gets the list of items.
 *
 * @param o the context menu to get list of the items
 * @return the list of the items on success or @c NULL on failure
 */
EAPI const Eina_List *ewk_context_menu_items_get(const Ewk_Context_Menu *o);

/**
 * Selects the item from the context menu.
 *
 * @param menu the context menu
 * @param item the item is selected
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_select(Ewk_Context_Menu *menu, Ewk_Context_Menu_Item *item);

// #if ENABLE(TIZEN_CONTEXT_MENU)
/** Creates a type name for Ewk_Context_Menu_Item_Tag */
typedef uint32_t Ewk_Context_Menu_Item_Tag;

/**
 * Counts the number of the context menu item.
 *
 * @param menu the context menu object
 *
 * @return number of current context menu item
 */
EAPI unsigned ewk_context_menu_item_count(Ewk_Context_Menu *menu);

/**
 * Returns the nth item in a context menu.
 *
 * @param menu The context menu object
 * @param n The number of the item
 *
 * @return The nth item of context menu
 */
EAPI Ewk_Context_Menu_Item* ewk_context_menu_nth_item_get(Ewk_Context_Menu *menu, unsigned int n);

/**
 * Returns the tag of context menu item.
 *
 * @param item The context menu item object
 *
 * @return The tag of context menu item
 */
EAPI Ewk_Context_Menu_Item_Tag ewk_context_menu_item_tag_get(Ewk_Context_Menu_Item *item);
// #endif // #if ENABLE(TIZEN_CONTEXT_MENU)

/**
 * Returns the link url string of context menu item.
 *
 * @param menu the context menu item object
 *
 * @return current link url string on success or @c 0 on failure
 */
EAPI const char* ewk_context_menu_item_link_url_get(Ewk_Context_Menu_Item* item);

/**
 * Returns the image url string of context menu item.
 *
 * @param menu the context menu item object
 *
 * @return current image url string on success or @c 0 on failure
 */
EAPI const char* ewk_context_menu_item_image_url_get(Ewk_Context_Menu_Item* item);

/**
 * Adds the context menu item to the context menu object.
 *
 * @param menu the context menu object
 * @param tag the tag of context menu item
 * @param title the title of context menu item
 * @param icon_file the path of icon to be set on context menu item
 * @param enabled the enabled of context menu item
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_append(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, const char* icon_file, Eina_Bool enabled);

/**
 * Adds the context menu item to the context menu object.
 *
 * @param menu the context menu object
 * @param tag the tag of context menu item
 * @param title the title of context menu item
 * @param enabled the enabled of context menu item
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_menu_item_append_as_action(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, Eina_Bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* ewk_context_menu_h */
