/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Shuo Liu <shuo0805.liu@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "isf_demo_efl.h"
#include "isf_return_key_disable_efl.h"

static void
_cursor_changed_cb (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *en = obj;

    int pos = elm_entry_cursor_pos_get (en);

    if (pos == 0) {
        elm_entry_input_panel_return_key_disabled_set (en, EINA_TRUE);
    }
    else {
        elm_entry_input_panel_return_key_disabled_set (en, EINA_FALSE);
    }
}

static Evas_Object *_create_ef_layout (Evas_Object *parent, const char *label, const char *guide_text, Elm_Input_Panel_Layout layout, Elm_Input_Panel_Return_Key_Type return_key_type)
{
    Evas_Object *en;
    Evas_Object *ef = create_ef (parent, label, guide_text, &en);
    if (!ef || !en) return NULL;

    elm_entry_input_panel_layout_set (en, layout);
    elm_entry_input_panel_return_key_type_set (en, return_key_type);

    evas_object_smart_callback_add (en, "cursor,changed", _cursor_changed_cb, NULL);
    evas_object_smart_callback_add (en, "focused", _cursor_changed_cb, NULL);

    return ef;
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *ef = NULL;
    int i,j;
    char title[128];

    const char* title_layout[] = {"NORMAL", "NUMBER", "EMAIL", "URL", "PHONENUMBER", "IP", "MONTH", "NUMBERONLY", "INVALID", "HEX", "TERMINAL", "PASSWORD"};
    const char* title_key[] = {"DEFAULT", "DONE", "GO", "JOIN", "LOGIN", "NEXT", "SEARCH", "SEND"};

    Evas_Object *parent = ad->naviframe;

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    for (i = (int)ELM_INPUT_PANEL_LAYOUT_NORMAL; i <= (int)ELM_INPUT_PANEL_LAYOUT_PASSWORD; i++) {
        if (i == (int)ELM_INPUT_PANEL_LAYOUT_INVALID) continue;
        for (j = (int)ELM_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT; j <= (int)ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEND; j++) {
            snprintf (title, sizeof (title), "%s - %s", title_layout[i], title_key[j]);
            ef = _create_ef_layout (parent, _(title), _("Return Key will be activated when any key is pressed"), (Elm_Input_Panel_Layout)i, (Elm_Input_Panel_Return_Key_Type)j);
            elm_box_pack_end (bx, ef);
        }
    }
    return bx;
}

void ise_return_key_disable_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("Return Key Disable"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
