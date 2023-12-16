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
#include "isf_imdata_set_efl.h"


static Evas_Object *_create_ef_layout (Evas_Object *parent, const char *label, const char *guide_text)
{
    Evas_Object *ef = create_ef (parent, label, guide_text);
    return ef;
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *ef = NULL;
    Evas_Object *en = NULL;

    Evas_Object *parent = ad->naviframe;
    const char *imdata_ko = "LANG:ko_KR";
    const char *imdata_en = "LANG:en_US";

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    ef = _create_ef_layout (parent, _("ko_KR"), _("Korean Layout"));
    elm_box_pack_end (bx, ef);
    elm_entry_input_panel_imdata_set (en, imdata_ko, strlen (imdata_ko));

    ef = _create_ef_layout (parent, _("en_US"), _("English layout"));
    elm_box_pack_end (bx, ef);
    elm_entry_input_panel_imdata_set (en, imdata_en, strlen (imdata_en));

    return bx;
}

void ise_imdata_set_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("IM Data"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
