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

#include <stdio.h>
#include <stdlib.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <privilege-control.h>
#include <vconf.h>
#include <efl_extension.h>
#include "scim_private.h"
#include "isf_demo_efl.h"
#include "isf_imcontrol_efl.h"
#include "isf_layout_efl.h"
#include "isf_event_efl.h"
#include "isf_autocapital_efl.h"
#include "isf_prediction_efl.h"
#include "isf_return_key_type_efl.h"
#include "isf_return_key_disable_efl.h"
#include "isf_imdata_set_efl.h"
#include "isf_focus_movement_efl.h"
#include "isf_language_efl.h"
#include "isf_ondemand_efl.h"
#include "isf_input_hint_efl.h"

#if HAVE_UIGADGET
#include <ui-gadget.h>

static void isfsetting_bt (void *data, Evas_Object *obj, void *event_info);
#endif

static struct _menu_item isf_demo_menu_its[] = {
    { "ISF Layout", ise_layout_bt },
    { "ISF Autocapital", ise_autocapital_bt },
    { "ISF Prediction Allow", ise_prediction_bt },
    { "ISF Language", ise_language_bt },
    { "ISF Return Key Type", ise_return_key_type_bt },
    { "ISF Return Key Disable", ise_return_key_disable_bt },
    { "ISF Input hint", ise_input_hint_bt },
    { "ISF IM Data", ise_imdata_set_bt },
    { "ISF ondemand", ise_ondemand_bt },
    { "ISF Focus Movement", isf_focus_movement_bt },
    { "ISF Event", isf_event_demo_bt },
    { "ISF IM Control", imcontrolapi_bt },
#if HAVE_UIGADGET
    { "ISF Setting", isfsetting_bt },
#endif

    /* do not delete below */
    { NULL, NULL }
};

static void _quit_cb (void *data, Evas_Object *obj, void *event_info)
{
    elm_exit ();
}

static void _list_click (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    if (ad == NULL) return;

    Evas_Object *li = ad->li;
    if (li == NULL) return;

    Elm_Object_Item *it = (Elm_Object_Item *)elm_list_selected_item_get (li);

    if (it != NULL)
        elm_list_item_selected_set (it, EINA_FALSE);
}

#if HAVE_UIGADGET
static void layout_cb (ui_gadget_h ug, enum ug_mode mode, void *priv)
{
    struct appdata *ad = NULL;
    Evas_Object *base = NULL;

    if (ug == NULL || priv == NULL)
        return;

    ad = (appdata *)priv;

    base = (Evas_Object *)ug_get_layout (ug);
    if (base == NULL)
        return;

    switch (mode) {
    case UG_MODE_FULLVIEW:
        evas_object_size_hint_weight_set (base, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add (ad->win_main, base);
        evas_object_show (base);
        break;
    case UG_MODE_FRAMEVIEW:
        LOGD ("please set ug mode to UG_MODE_FULLVIEW!\n");
        break;
    default:
        break;
    }
}

static void result_cb (ui_gadget_h ug, app_control_h s, void *priv)
{
    char *name = NULL;
    app_control_get_extra_data (s, "name", &name);

    if (name) {
        free (name);
    }
}

static void destroy_cb (ui_gadget_h ug, void *priv)
{
    if (ug == NULL)
        return;

    ug_destroy (ug);
}

static void isfsetting_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    struct ug_cbs cbs = {0, 0, 0, 0, 0, {0, 0, 0}};

    UG_INIT_EFL (ad->win_main, UG_OPT_INDICATOR_ENABLE);

    cbs.layout_cb  = layout_cb;
    cbs.result_cb  = result_cb;
    cbs.destroy_cb = destroy_cb;
    cbs.priv       = ad;
    ug_create (NULL, "isfsetting-efl",
               UG_MODE_FULLVIEW,
               NULL, &cbs);
}
#endif

static int create_demo_view (struct appdata *ad)
{
    Evas_Object *li = NULL;
    int idx = 0;

    Evas_Object *l_button = elm_button_add (ad->naviframe);
    elm_object_style_set (l_button, "naviframe/end_btn/default");
    evas_object_smart_callback_add (l_button, "clicked", _quit_cb, NULL);

    // Create list
    ad->li = li = elm_list_add (ad->naviframe);
    elm_list_mode_set (li, ELM_LIST_COMPRESS);
    evas_object_smart_callback_add (ad->li, "selected", _list_click, ad);

    while (isf_demo_menu_its[idx].name != NULL) {
        elm_list_item_append (li, isf_demo_menu_its[idx].name, NULL, NULL, isf_demo_menu_its[idx].func, ad);
        ++idx;
    }

    elm_list_go (li);

    elm_naviframe_item_push (ad->naviframe, _("ISF Demo"), l_button, NULL, li, NULL);

    return 0;
}

static int lang_changed (void *event_info, void *data)
{
#if HAVE_UIGADGET
    ug_send_event (UG_EVENT_LANG_CHANGE);
#endif
    return 0;
}

static void win_del (void *data, Evas_Object *obj, void *event)
{
    elm_exit ();
}

static Evas_Object* create_win (const char *name)
{
    Evas_Object *eo = NULL;
    int w, h;
    const int rots[4] = { 0, 90, 180, 270 };

    eo = elm_win_util_standard_add (name, name);
    if (eo != NULL) {
        evas_object_smart_callback_add (eo, "delete,request",
                                        win_del, NULL);
        ecore_x_window_size_get (ecore_x_window_root_first_get (), &w, &h);
        evas_object_resize (eo, w, h);
    }

    if (elm_win_wm_rotation_supported_get (eo)) {
        elm_win_wm_rotation_available_rotations_set (eo, (const int *)&rots, 4);
    }

    return eo;
}

static void
_vkbd_state_on (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;

    LOGD ("input panel is shown\n");
    ad->vkbd_state = EINA_TRUE;
}

static void
_vkbd_state_off (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;

    LOGD ("input panel is hidden\n");
    ad->vkbd_state = EINA_FALSE;
}

static Evas_Object* create_conformant (struct appdata *ad)
{
    Evas_Object *win_main = ad->win_main;

    /* Put the layout inside conformant for drawing indicator in app side */
    Evas_Object *conformant = elm_conformant_add (win_main);
    evas_object_size_hint_weight_set (conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (conformant, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_win_resize_object_add (win_main, conformant);
    evas_object_show (conformant);

    evas_object_smart_callback_add (conformant, "virtualkeypad,state,on", _vkbd_state_on, ad);
    evas_object_smart_callback_add (conformant, "virtualkeypad,state,off", _vkbd_state_off, ad);

    elm_win_conformant_set (win_main, EINA_TRUE);

    return conformant;
}

static void
_naviframe_back_cb (void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *top_it = elm_naviframe_top_item_get (obj);
    Elm_Object_Item *bottom_it = elm_naviframe_bottom_item_get (obj);
    if (top_it && bottom_it && (elm_object_item_content_get (top_it) == elm_object_item_content_get (bottom_it))) {
        elm_exit ();
    } else {
        elm_naviframe_item_pop (obj);
    }
}

static Evas_Object* _create_naviframe_layout (Evas_Object *parent)
{
    Evas_Object *naviframe = elm_naviframe_add (parent);
    elm_naviframe_prev_btn_auto_pushed_set (naviframe, EINA_FALSE);
    eext_object_event_callback_add (naviframe, EEXT_CALLBACK_BACK, _naviframe_back_cb, NULL);
    elm_object_part_content_set (parent, "elm.swallow.content", naviframe);

    evas_object_show (naviframe);

    return naviframe;
}

static Eina_Bool _keydown_event (void *data, int type, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;
    struct appdata *ad = (struct appdata *)data;
    if (ad == NULL || ev == NULL) return ECORE_CALLBACK_PASS_ON;

    LOGD ("[ecore key down] keyname : '%s', key : '%s', string : '%s', compose : '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);

    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool _keyup_event (void *data, int type, void *event)
{
    Ecore_Event_Key *ev = (Ecore_Event_Key *)event;

    LOGD ("[ecore key up] keyname : '%s', key : '%s', string : '%s', compose : '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);

    return ECORE_CALLBACK_PASS_ON;
}

static void input_panel_state_changed_cb (keynode_t *key, void* data)
{
    int sip_status = vconf_keynode_get_int (key);

    switch (sip_status) {
        case VCONFKEY_ISF_INPUT_PANEL_STATE_HIDE:
            LOGD ("state : hide\n");
            break;
        case VCONFKEY_ISF_INPUT_PANEL_STATE_WILL_HIDE:
            LOGD ("state : will_hide\n");
            break;
        case VCONFKEY_ISF_INPUT_PANEL_STATE_SHOW:
            LOGD ("state : show\n");
            break;
        case VCONFKEY_ISF_INPUT_PANEL_STATE_WILL_SHOW:
            LOGD ("state : will_show\n");
            break;
        default :
            LOGD ("sip_status error!\n");
            break;
    }
}

static int app_create (void *data)
{
    struct appdata *ad = (struct appdata *)data;

    appcore_measure_start ();

    ad->win_main = create_win ("isf-demo-efl");
    evas_object_show (ad->win_main);

    ad->evas = evas_object_evas_get (ad->win_main);
    /* get width and height of main window */
    evas_object_geometry_get (ad->win_main, NULL, NULL, &ad->root_w, &ad->root_h);

    ad->conformant = create_conformant (ad);

    // Indicator
    elm_win_indicator_mode_set (ad->win_main, ELM_WIN_INDICATOR_SHOW);

    // Navigation Bar
    ad->naviframe = _create_naviframe_layout (ad->conformant);

    //init the content in conformant.
    create_demo_view (ad);

    lang_changed (NULL, ad);

    evas_object_show (ad->win_main);

    vconf_notify_key_changed (VCONFKEY_ISF_INPUT_PANEL_STATE, input_panel_state_changed_cb, NULL);

    /* add system event callback */
    appcore_set_event_callback (APPCORE_EVENT_LANG_CHANGE,
                                lang_changed, ad);

    ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _keydown_event, ad);
    ecore_event_handler_add (ECORE_EVENT_KEY_UP, _keyup_event, ad);

    appcore_measure_time ();

    return 0;
}

static int app_exit (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    if (ad->li != NULL) {
        evas_object_del (ad->li);
        ad->li = NULL;
    }

    if (ad->ev_li != NULL) {
        evas_object_del (ad->ev_li);
        ad->ev_li = NULL;
    }

    if (ad->conformant != NULL) {
        evas_object_del (ad->conformant);
        ad->conformant = NULL;
    }

    if (ad->win_main != NULL) {
        evas_object_del (ad->win_main);
        ad->win_main = NULL;
    }

    return 0;
}

static int app_pause (void *data)
{
    return 0;
}

static int app_resume (void *data)
{
    return 0;
}

int main (int argc, char *argv[])
{
    perm_app_set_privilege ("isf", NULL, NULL);

    struct appdata ad;
    struct appcore_ops ops;

    ops.create    = app_create;
    ops.terminate = app_exit;
    ops.pause     = app_pause;
    ops.resume    = app_resume;
    ops.reset     = NULL;

    memset (&ad, 0x0, sizeof (struct appdata));
    ops.data = &ad;

    int ret = -1;
    try {
        ret = appcore_efl_main ("isf-demo-efl", &argc, &argv, &ops);
    } catch (...) {
        LOGW ("Exception is thrown from appcore_efl_main ()!!!\n");
    }

    return ret;
}

// Utility functions
Evas_Object *create_ef (Evas_Object *parent, const char *label, const char *guide_text, Evas_Object **entry)
{
    Evas_Object *lb, *en;

    Evas_Object *bx;
    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    lb = elm_label_add (parent);
    evas_object_size_hint_weight_set (lb, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (lb, EVAS_HINT_FILL, 0.0);
    elm_object_text_set (lb, label);
    evas_object_show (lb);
    elm_box_pack_end (bx, lb);

    en = elm_entry_add (parent);
    elm_entry_single_line_set (en, EINA_TRUE);
    evas_object_size_hint_weight_set (en, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (en, EVAS_HINT_FILL, 0.0);
    elm_object_part_text_set (en, "elm.guide", guide_text);
    evas_object_show (en);
    elm_box_pack_end (bx, en);

    if (entry)
        *entry = en;

    evas_object_show (bx);

    return bx;
}

static void _back_btn_clicked_cb (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;

    if (!ad->vkbd_state) {
        elm_naviframe_item_pop (ad->naviframe);
    }
}

void add_layout_to_naviframe (void *data, Evas_Object *lay_in, const char *title)
{
    struct appdata *ad = (struct appdata *) data;

    Evas_Object *scroller = elm_scroller_add (ad->naviframe);
    elm_scroller_bounce_set (scroller, EINA_FALSE, EINA_TRUE);
    evas_object_show (scroller);

    elm_object_content_set (scroller, lay_in);

    // create back key
    Evas_Object *back_btn = elm_button_add (ad->naviframe);
    elm_object_style_set (back_btn, "naviframe/end_btn/default");
    evas_object_smart_callback_add (back_btn, "clicked",  _back_btn_clicked_cb, ad);

    elm_naviframe_item_push (ad->naviframe, title, back_btn, NULL, scroller, NULL);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
