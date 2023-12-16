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

static void
_entry_activated_cb (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *parent = (Evas_Object *)data;
    elm_object_focus_next (parent, ELM_FOCUS_NEXT);
}

static Evas_Object *_create_ef_layout (Evas_Object *parent, const char *label, const char *guide_text, Elm_Input_Panel_Layout layout, int layout_variation = 0)
{
    Evas_Object *en;
    Evas_Object *ef = create_ef (parent, label, guide_text, &en);
    if (!ef || !en) return NULL;

    elm_entry_input_panel_layout_set (en, layout);
    elm_entry_input_panel_return_key_type_set (en, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
    evas_object_smart_callback_add (en, "activated", _entry_activated_cb, parent);

    return ef;
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *parent = ad->naviframe;
    Evas_Object *ef = NULL;

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    /* First Name */
    ef = _create_ef_layout (bx, "First Name", "First Name", ELM_INPUT_PANEL_LAYOUT_NORMAL);
    elm_box_pack_end (bx, ef);

    /* Last Name */
    ef = _create_ef_layout (bx, "Last Name", "Last Name", ELM_INPUT_PANEL_LAYOUT_NORMAL);
    elm_box_pack_end (bx, ef);

    /* Homepage */
    ef = _create_ef_layout (bx, "Homepage", "URL", ELM_INPUT_PANEL_LAYOUT_URL);
    elm_box_pack_end (bx, ef);

    return bx;
}

void isf_focus_movement_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("Focus Movement"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
