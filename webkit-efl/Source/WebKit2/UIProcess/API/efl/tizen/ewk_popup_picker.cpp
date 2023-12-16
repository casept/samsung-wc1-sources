/*
 * Copyright (C) 2012-2013 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(TIZEN_POPUP_INTERNAL)
#include "ewk_popup_picker.h"

#include "EwkView.h"
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
#include <efl_extension.h>
#include <ui_extension.h>
#endif
#include "ewk_view.h"
#include "ewk_view_private.h"
#include "ewk_popup_menu_item.h"
#include "ewk_popup_menu_item_private.h"
#include <Ecore_X.h>
#include <Elementary.h>
#include <libintl.h>

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <efl_assist.h>
#endif

#if !ENABLE(TIZEN_WEARABLE_PROFILE)
static void menuItemActivated(void* data, Evas_Object*, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    Elm_Object_Item* selected = static_cast<Elm_Object_Item*>(event_info);

    ewk_popup_menu_selected_index_set(picker->ewk_menu, elm_menu_item_index_get(selected));
    ewk_view_popup_menu_close(picker->parent);
}
#else
static void menuItemActivated(void* data, Evas_Object*, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    Elm_Object_Item* selected = static_cast<Elm_Object_Item*>(event_info);
    int index = 0;
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
    // A title list item with display string "Options" is added in the list in addition
    // to programmer inputs.
    // Hence selected index is substracted by 2, to get correct selected item index.
    index = elm_genlist_item_index_get(selected) - 2;
#else
    index = elm_genlist_item_index_get(selected);
#endif

    picker->selectedIndex = index;
    ewk_popup_menu_selected_index_set(picker->ewk_menu, index);
    ewk_view_popup_menu_close(picker->parent);
}
#endif

static void listClosed(void* data, Evas_Object*, void*)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    Ewk_Popup_Menu* ewk_menu = picker->ewk_menu;
    ewk_popup_menu_selected_index_set(ewk_menu, picker->selectedIndex);
    ewk_view_popup_menu_close(picker->parent);
}

static char* _listLabelGet(void* data, Evas_Object*, const char* part)
{
    Ewk_Popup_Menu_Item* menuItem = static_cast<Ewk_Popup_Menu_Item*>(data);

    if (!strncmp(part, "elm.text", strlen("elm.text")))
        return strdup(ewk_popup_menu_item_text_get(menuItem));
    return 0;
}

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
static char* _listTitleGet(void* data, Evas_Object*, const char* part)
{
    return strdup("Options");
}
#endif

#if !ENABLE(TIZEN_WEARABLE_PROFILE)
Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, const Eina_List*, int selectedIndex, Ewk_Popup_Menu* ewk_menu, const Eina_Rectangle* rect)
{
    Ewk_Popup_Picker* picker = new Ewk_Popup_Picker;
    picker->parent = parent;
    picker->ewk_menu = ewk_menu;
    picker->selectedIndex = selectedIndex;

    picker->popup = elm_menu_add(parent);
    evas_object_smart_callback_add(picker->popup, "clicked", listClosed, picker);

    const Eina_List* ewk_items = ewk_popup_menu_items_get(ewk_menu);
    const Eina_List* l;
    void* data;

    EINA_LIST_FOREACH(ewk_items, l, data) {
        Ewk_Popup_Menu_Item* ewk_item = static_cast<Ewk_Popup_Menu_Item*>(data);
        switch (ewk_popup_menu_item_type_get(ewk_item)) {
        case EWK_POPUP_MENU_SEPARATOR:
            elm_menu_item_separator_add(picker->popup, NULL);
            break;
        case EWK_POPUP_MENU_ITEM:
            if (ewk_popup_menu_item_is_label_get(ewk_item)) {
                Elm_Object_Item* item = elm_menu_item_add(picker->popup, NULL, NULL, ewk_popup_menu_item_text_get(ewk_item), NULL, NULL);
                elm_object_item_disabled_set(item, EINA_TRUE);
            } else {
                Elm_Object_Item* item = elm_menu_item_add(picker->popup, NULL, NULL, ewk_popup_menu_item_text_get(ewk_item), menuItemActivated, picker);
                const char* tooltip_text = ewk_popup_menu_item_tooltip_get(ewk_item);
                if (tooltip_text && tooltip_text[0] != '\0')
                    elm_object_item_tooltip_text_set(item, tooltip_text);
                elm_object_item_disabled_set(item, !ewk_popup_menu_item_enabled_get(ewk_item));
                elm_menu_item_selected_set(item, ewk_popup_menu_item_selected_get(ewk_item));
            }
            break;
        default:
            break;
        }
    }

    elm_menu_move(picker->popup, rect->x, rect->y);
    evas_object_show(picker->popup);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    ea_object_event_callback_add(picker->popup, EA_CALLBACK_BACK, listClosed, picker);
#endif

    return picker;
}

void ewk_popup_picker_del(Ewk_Popup_Picker* picker)
{
    elm_menu_close(picker->popup);
    evas_object_del(picker->popup);
    picker->firstItem = 0;
    delete picker;
}

#else
Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, const Eina_List* items, int selectedIndex, Ewk_Popup_Menu* ewk_menu, const Eina_Rectangle* /* rect */)
{
    Ewk_Popup_Picker* picker = new Ewk_Popup_Picker;
    picker->parent = parent;
    picker->ewk_menu = ewk_menu;
    picker->selectedIndex = selectedIndex;

    Evas_Coord width, height;
#ifdef HAVE_ECORE_X
    ecore_x_screen_size_get(ecore_x_default_screen_get(), &width, &height);
#else
    width = 360;
    height = 480;
#endif

    Elm_Genlist_Item_Class* itemClass;
    itemClass = elm_genlist_item_class_new();
    itemClass->item_style = "1text";
    itemClass->func.text_get = _listLabelGet;
    itemClass->func.content_get = 0;
    itemClass->func.state_get = 0;
    itemClass->func.del = 0;

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
    Elm_Genlist_Item_Class* titleItemClass;
    titleItemClass = elm_genlist_item_class_new();
    titleItemClass->item_style = "title";
    titleItemClass->func.text_get = _listTitleGet;
    titleItemClass->func.content_get = 0;
    titleItemClass->func.state_get = 0;
    titleItemClass->func.del = 0;

    Evas_Object *conform, *circle_genlist;
    Eext_Circle_Surface *circle_surface;

    // win
    picker->win = elm_win_add(parent, "combo_box Window", ELM_WIN_BASIC);
    elm_win_title_set(picker->win, "combo_box Window");
    elm_win_autodel_set(picker->win, EINA_TRUE);

    // conformant
    conform = elm_conformant_add(picker->win);
    evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(picker->win, conform);
    evas_object_show(conform);
    circle_surface = eext_circle_surface_conformant_add(conform);
    evas_object_resize(picker->win, width, height);

    // layout
    Evas_Object* layout = elm_layout_add(picker->win);
    elm_layout_theme_set(layout, "layout", "application", "default");
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(picker->win, layout);
    evas_object_show(layout);

    // Indicator
    elm_win_indicator_mode_set(picker->win, ELM_WIN_INDICATOR_HIDE);

    // naviframe
    Evas_Object* naviframe = elm_naviframe_add(layout);
    elm_object_part_content_set(layout, "elm.swallow.content", naviframe);
    evas_object_show(naviframe);

    // genlist
    picker->genlist = elm_genlist_add(naviframe);
    elm_object_style_set(picker->genlist, "focus_bg");

    circle_genlist = eext_circle_object_genlist_add(picker->genlist, circle_surface);
    eext_circle_object_genlist_scroller_policy_set(circle_genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
    eext_rotary_object_event_activated_set(circle_genlist, EINA_TRUE);

    evas_object_resize(picker->genlist, width, height);
    elm_object_part_content_set(naviframe, "content", picker->genlist);
    uxt_genlist_set_bottom_margin_enabled(picker->genlist, EINA_TRUE);

    Elm_Object_Item* nf_it = elm_naviframe_item_push(naviframe, NULL, NULL, NULL, picker->genlist, NULL);
    elm_naviframe_item_title_enabled_set(nf_it, EINA_FALSE, EINA_FALSE);

    // Appending title
    Elm_Object_Item* itemObject = elm_genlist_item_append(picker->genlist, titleItemClass, 0, 0, ELM_GENLIST_ITEM_NONE, 0, 0);
#else
    picker->popup = elm_layout_add(parent);
    elm_layout_file_set(picker->popup, EDJE_DIR "/control.edj", "elm/select");
    evas_object_resize(picker->popup, width, height);
    evas_object_move(picker->popup, 0, 0);
    picker->genlist = elm_genlist_add(picker->popup);
    elm_object_part_content_set(picker->popup, "content", picker->genlist);
#endif

    elm_genlist_homogeneous_set(picker->genlist, EINA_TRUE);

    void* itemv;
    const Eina_List* l;

    picker->firstItem = 0;
    int index = 0;
    EINA_LIST_FOREACH(items, l, itemv) {
        Ewk_Popup_Menu_Item* menuItem = static_cast<Ewk_Popup_Menu_Item*>(itemv);

        Elm_Object_Item* itemObject = elm_genlist_item_append(picker->genlist, itemClass, menuItem, 0, ELM_GENLIST_ITEM_NONE, 0, 0);

        if (menuItem->isLabel())
            elm_genlist_item_select_mode_set(itemObject, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
        if (!index)
            picker->firstItem = itemObject;
        if (selectedIndex == index)
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
            elm_genlist_item_show(itemObject,ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
#else
            elm_genlist_item_selected_set(itemObject, true);
#endif
        if (!menuItem->isEnabled() && !menuItem->isLabel())
            elm_object_item_disabled_set(itemObject, EINA_TRUE);
        index++;
    }

    evas_object_smart_callback_add(picker->genlist, "selected", menuItemActivated, picker);
    evas_object_show(picker->genlist);
    elm_genlist_item_class_free(itemClass);

#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
    evas_object_show(picker->win);
    elm_genlist_item_class_free(titleItemClass);
#else
    Evas_Object* btn;
    btn = elm_button_add(picker->popup);
    elm_object_text_set(btn, "Close");
    evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_part_content_set(picker->popup, "btn", btn);
    evas_object_smart_callback_add(btn, "clicked", listClosed, picker);
    evas_object_show(picker->popup);
#endif

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
    ea_object_event_callback_add(picker->win, EA_CALLBACK_BACK, listClosed, picker);
#else
    ea_object_event_callback_add(picker->popup, EA_CALLBACK_BACK, listClosed, picker);
#endif
#endif

    return picker;
}

void ewk_popup_picker_del(Ewk_Popup_Picker* picker)
{
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
    evas_object_smart_callback_del(picker->genlist, "selected", menuItemActivated);
    evas_object_del(picker->genlist);
    evas_object_del(picker->win);
#else
    evas_object_del(picker->popup);
#endif
    picker->firstItem = 0;
    delete picker;
}
#endif //TIZEN_WEARABLE_PROFILE
#endif
