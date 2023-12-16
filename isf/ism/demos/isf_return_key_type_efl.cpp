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
#include "isf_return_key_type_efl.h"


static Evas_Object *_create_ef_layout (Evas_Object *parent, const char *label, const char *guide_text, Elm_Input_Panel_Return_Key_Type type)
{
    Evas_Object *en;
    Evas_Object *ef = create_ef (parent, label, guide_text, &en);
    if (!ef || !en) return NULL;

    elm_entry_input_panel_return_key_type_set (en, type);

    return ef;
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *ef = NULL;

    Evas_Object *parent = ad->naviframe;

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    /* DEFAULT */
    ef = _create_ef_layout (parent, _("DEFAULT"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
    elm_box_pack_end (bx, ef);

    /* DONE */
    ef = _create_ef_layout (parent, _("DONE"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
    elm_box_pack_end (bx, ef);

    /* GO */
    ef = _create_ef_layout (parent, _("GO"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_GO);
    elm_box_pack_end (bx, ef);

    /* JOIN */
    ef = _create_ef_layout (parent, _("JOIN"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_JOIN);
    elm_box_pack_end (bx, ef);

    /* LOGIN */
    ef = _create_ef_layout (parent, _("LOGIN"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_LOGIN);
    elm_box_pack_end (bx, ef);

    /* NEXT */
    ef = _create_ef_layout (parent, _("NEXT"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
    elm_box_pack_end (bx, ef);

    /* SEARCH */
    ef = _create_ef_layout (parent, _("SEARCH"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH);
    elm_box_pack_end (bx, ef);

    /* SEND */
    ef = _create_ef_layout (parent, _("SEND"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEND);
    elm_box_pack_end (bx, ef);

    /* SIGNIN */
    ef = _create_ef_layout (parent, _("SIGNIN"), _("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_SIGNIN);
    elm_box_pack_end (bx, ef);

    return bx;
}

void ise_return_key_type_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("Return Key Type"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
