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

#ifndef ewk_popup_picker_h
#define ewk_popup_picker_h
#include "ewk_popup_menu.h"
#include "ewk_popup_menu_private.h"

#if USE(EO)
typedef struct _Eo_Opaque Eo;
typedef Eo Evas_Object;
#else
typedef struct _Evas_Object Evas_Object;
#endif

typedef struct _Elm_Object_Item Elm_Object_Item;
typedef struct _Eina_List Eina_List;

namespace WebKit {
class WebPopupMenuProxyEfl;
}

struct _Ewk_Popup_Picker {
    Evas_Object* parent;
    Evas_Object* popup;
    Evas_Object* genlist;
    Elm_Object_Item* firstItem;
    int selectedIndex;
    Ewk_Popup_Menu* ewk_menu;
#if ENABLE(TIZEN_CIRCLE_DISPLAY_SELECT)
    Evas_Object* win;
#endif
};
typedef struct _Ewk_Popup_Picker Ewk_Popup_Picker;

Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, const Eina_List* items, int selectedIndex, Ewk_Popup_Menu* ewk_menu, const Eina_Rectangle* rect);

void ewk_popup_picker_del(Ewk_Popup_Picker* picker);

#endif // ewk_popup_picker_h
