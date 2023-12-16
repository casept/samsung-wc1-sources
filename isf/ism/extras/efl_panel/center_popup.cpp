/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>
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
#include "center_popup.h"
#include "isf_panel_efl.h"

#include <Ecore_X.h>

typedef struct _Center_Popup_Data Center_Popup_Data;
struct _Center_Popup_Data
{
   Evas_Object *parent;
   Evas_Object *parent_win;
   Evas_Object *block_events;
   Evas_Object *popup_win;
};

static void _center_popup_resize_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
    Center_Popup_Data *cp_data = (Center_Popup_Data *)data;
    if (!cp_data) return;

    Evas_Coord ox, oy, ow, oh;
    evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);
    evas_object_resize(cp_data->popup_win, ow, oh);
}

static void _center_popup_parent_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
    Evas_Object *object = (Evas_Object *)data;
    evas_object_del(object);
}

static void
_block_area_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
    evas_object_smart_callback_call((Evas_Object *)data, "block,clicked", NULL);
}

static void _popup_win_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
    Evas_Object *popup = (Evas_Object *)data;
    elm_win_resize_object_del(obj, popup);
}

static void _center_popup_del_cb(void *data, Evas * e, Evas_Object * obj, void *event_info)
{
    Center_Popup_Data *cp_data = (Center_Popup_Data *)data;
    if (!cp_data) return;

    elm_layout_signal_callback_del(cp_data->block_events, "elm,action,click", "elm", _block_area_clicked_cb);
    evas_object_event_callback_del_full(obj, EVAS_CALLBACK_RESIZE, _center_popup_resize_cb, cp_data);
    evas_object_event_callback_del_full(obj, EVAS_CALLBACK_DEL, _center_popup_del_cb, cp_data);
    if (cp_data->parent)
        evas_object_event_callback_del_full(cp_data->parent, EVAS_CALLBACK_DEL, _center_popup_parent_del_cb, obj);

    elm_win_resize_object_del(cp_data->popup_win, cp_data->block_events);
    evas_object_del(cp_data->block_events);
    evas_object_del(cp_data->popup_win);
    free(cp_data);
}

Evas_Object *
center_popup_win_get(Evas_Object *popup)
{
    return evas_object_top_get(evas_object_evas_get(popup));
}

Evas_Object *
center_popup_add(Evas_Object *parent, const char *name, const char *title)
{
    Center_Popup_Data *cp_data;
    Evas_Object *popup;

    cp_data = (Center_Popup_Data *)calloc(1, sizeof(Center_Popup_Data));
    if (!cp_data) return NULL;

    cp_data->parent = parent;
    if (parent)
        cp_data->parent_win = evas_object_top_get(evas_object_evas_get(parent));
    else
        cp_data->parent_win = NULL;

    // Create popup window
    cp_data->popup_win = elm_win_add(cp_data->parent_win, name, ELM_WIN_DIALOG_BASIC);
    if (!cp_data->popup_win) goto error;
    elm_win_alpha_set(cp_data->popup_win, EINA_TRUE);
    elm_win_title_set(cp_data->popup_win, title);
    elm_win_conformant_set(cp_data->popup_win, EINA_TRUE);
    elm_win_modal_set(cp_data->popup_win, EINA_TRUE);
    elm_win_fullscreen_set(cp_data->popup_win, EINA_TRUE);
    if (elm_win_wm_rotation_supported_get(cp_data->popup_win))
    {
        int rots[4] = { 0, 90, 180, 270 };
        elm_win_wm_rotation_available_rotations_set(cp_data->popup_win, (const int*)(&rots), 4);
    }
    evas_object_show(cp_data->popup_win);

    // Create Dimming object
    cp_data->block_events = elm_layout_add(cp_data->popup_win);
    if (!elm_layout_theme_set(cp_data->block_events, "notify", "block_events", "popup"))
        LOGW ("elm_layout_theme_set failed\n");

    evas_object_size_hint_weight_set(cp_data->block_events, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(cp_data->popup_win, cp_data->block_events);
    evas_object_show(cp_data->block_events);
    evas_object_repeat_events_set(cp_data->block_events, EINA_FALSE);

    // Create popup object
    popup = elm_popup_add(cp_data->popup_win);
    if (!popup) goto error;

    elm_popup_allow_events_set(popup, EINA_TRUE);
    evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    elm_layout_signal_callback_add(cp_data->block_events, "elm,action,click", "elm", _block_area_clicked_cb, popup);
    evas_object_event_callback_priority_add(popup, EVAS_CALLBACK_DEL, EVAS_CALLBACK_PRIORITY_BEFORE, _center_popup_del_cb, cp_data);
    if (parent)
        evas_object_event_callback_add(parent, EVAS_CALLBACK_DEL, _center_popup_parent_del_cb, popup);
    evas_object_event_callback_add(cp_data->popup_win, EVAS_CALLBACK_DEL, _popup_win_del_cb, popup);

    return popup;

error:
    if (cp_data->block_events)
    {
        elm_win_resize_object_del(cp_data->popup_win, cp_data->block_events);
        evas_object_del(cp_data->block_events);
    }
    if (cp_data->popup_win) evas_object_del(cp_data->popup_win);
    free(cp_data);
    return NULL;
}
