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

#define Uses_SCIM_PANEL_AGENT

#include <Elementary.h>
#include <Ecore_X.h>
#include <efl_extension.h>
#include <dlog.h>
#include "scim_private.h"
#include "scim.h"
#include "iseselector.h"
#include "center_popup.h"
#include "isf_panel_efl.h"
#if HAVE_UIGADGET
#include <ui-gadget.h>
#endif

using namespace scim;

extern std::vector<String>          _uuids;
extern std::vector<String>          _names;
extern std::vector<String>          _module_names;
extern std::vector<String>          _langs;
extern std::vector<String>          _icons;
extern std::vector<uint32>          _options;
extern std::vector<TOOLBAR_MODE_T>  _modes;

static const unsigned int ISE_SELECTOR_ITEM_HEIGHT = 98;
static const unsigned int ISE_SELECTOR_MAX_ITEM = 4;

static unsigned int _ise_selector_ise_idx = 0;

static Evas_Object *_ise_selector_radio_grp = NULL;
static Evas_Object *_ise_selector_popup = NULL;
static Ecore_X_Window _ise_selector_app_window = 0;

static Elm_Genlist_Item_Class itc;

static Ise_Selector_Selected_Cb _ise_selector_selected_cb;
static Ise_Selector_Deleted_Cb _ise_selector_deleted_cb;

static void _ise_selector_radio_grp_del_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    LOGD ("selector radio deleted\n");
    _ise_selector_radio_grp = NULL;
}

static char *
gl_ise_name_get (void *data, Evas_Object *obj, const char *part)
{
    int index = (int) data;
    if (index < 0 || index >= (int)_names.size ())
        return NULL;

    if (!strcmp (part, "elm.text.main.left") ||
        !strcmp (part, "elm.text")) {
        return strdup (_names[index].c_str ());
    }

    return NULL;
}

static Evas_Object *
gl_icon_get (void *data, Evas_Object *obj, const char *part)
{
    unsigned int index = (unsigned int)(data);

    if (index >= _uuids.size ())
        return NULL;

    if (!strcmp (part, "elm.icon.right")) {
        Evas_Object *radio = elm_radio_add (obj);
        elm_object_style_set (radio, "list");
        elm_radio_state_value_set (radio, index);
        if (_ise_selector_radio_grp == NULL) {
            _ise_selector_radio_grp = elm_radio_add (obj);
            evas_object_event_callback_add(_ise_selector_radio_grp, EVAS_CALLBACK_DEL, _ise_selector_radio_grp_del_cb, NULL);
        }
        elm_radio_group_add (radio, _ise_selector_radio_grp);
        evas_object_show (radio);

        if (_ise_selector_ise_idx == index)
            elm_radio_value_set (_ise_selector_radio_grp, index);

        return radio;
    }

    return NULL;
}

static void
gl_ise_selected_cb (void *data, Evas_Object *obj, void *event_info)
{
    unsigned int index = (unsigned int)(data);

    LOGD ("selected item : %d\n", index);

    if (index >= _uuids.size ())
        return;

    if (_ise_selector_radio_grp)
        elm_radio_value_set (_ise_selector_radio_grp, index);

    if (index == _ise_selector_ise_idx) {
        ise_selector_destroy ();
        return;
    }

    LOGD ("Set active ISE : %s\n", _uuids[index].c_str ());

    if (_ise_selector_selected_cb)
        _ise_selector_selected_cb (index);
}

static void
ise_selector_init ()
{
    _ise_selector_radio_grp = NULL;
    _ise_selector_popup = NULL;

    _ise_selector_app_window = 0;
    _ise_selector_ise_idx = 0;
    _ise_selector_selected_cb = NULL;
}

static void
ise_selector_block_clicked_cb (void *data, Evas_Object *obj, void *event_info)
{
    ise_selector_destroy ();
    _ise_selector_popup = NULL;
}

static void
ise_selector_popup_del_cb (void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    LOGD ("IME Selector is deleted\n");

    ise_selector_init ();

    if (_ise_selector_deleted_cb)
        _ise_selector_deleted_cb ();
}

static void
ise_selector_focus_out_cb (void *data, Evas *e, void *event_info)
{
    LOGD ("ISE selector window focus out\n");
    ise_selector_destroy ();
}

#if HAVE_UIGADGET
static void
isf_setting_cb (void *data, Evas_Object *obj, void *event_info)
{
    app_control_h app_control;
    app_control_create (&app_control);
    app_control_set_window (app_control, _ise_selector_app_window);
    app_control_set_app_id (app_control, "isfsetting-efl");

    evas_object_freeze_events_set (obj, EINA_TRUE);

    int ret = app_control_send_launch_request (app_control, NULL, NULL);
    if (0 != ret) {
        LOGW ("UG Launch Failed");
        ise_selector_destroy ();
    }

    if (app_control)
        app_control_destroy (app_control);
}
#endif

void ise_selector_create (unsigned ise_idx, Ecore_X_Window win, Ise_Selector_Selected_Cb sel_cb, Ise_Selector_Deleted_Cb del_cb)
{
    unsigned int index;
    Evas_Object *genlist;
    Evas_Object *box;
    int height;
    unsigned int item_count;

    ise_selector_destroy ();

    _ise_selector_ise_idx = ise_idx;
    _ise_selector_app_window = win;
    _ise_selector_selected_cb = sel_cb;
    _ise_selector_deleted_cb = del_cb;

    /* Create center popup */
    _ise_selector_popup = center_popup_add (NULL, "IMESelector", "IMESelector");
    if (!_ise_selector_popup)
        return;

    evas_object_event_callback_add (_ise_selector_popup, EVAS_CALLBACK_DEL, ise_selector_popup_del_cb, NULL);
    eext_object_event_callback_add (_ise_selector_popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
    evas_object_smart_callback_add (_ise_selector_popup, "block,clicked", ise_selector_block_clicked_cb, NULL);
    elm_object_domain_translatable_part_text_set (_ise_selector_popup, "title,text", GETTEXT_PACKAGE, "Select input method");

#if HAVE_UIGADGET
    /* Create "Set up input methods" button */
    Evas_Object *btn = elm_button_add (_ise_selector_popup);
    if (!elm_object_style_set (btn, "popup"))
        LOGW ("elm_object_style_set failed. obj : button, style : popup\n");
    elm_object_domain_translatable_part_text_set (btn, NULL, GETTEXT_PACKAGE, "Set up input methods");
    elm_object_part_content_set (_ise_selector_popup, "button1", btn);

    evas_object_smart_callback_add (btn, "clicked", isf_setting_cb, _ise_selector_popup);
#endif

    /* Create box for adjusting the height of list */
    box = elm_box_add (_ise_selector_popup);
    evas_object_size_hint_weight_set (box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    /* Create genlist for displaying IME list */
    genlist = elm_genlist_add (box);
    elm_genlist_mode_set (genlist, ELM_LIST_COMPRESS);
    elm_genlist_homogeneous_set (genlist, EINA_TRUE);
    evas_object_size_hint_weight_set (genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

    itc.item_style = "1line";
    itc.func.text_get = gl_ise_name_get;
    itc.func.content_get = gl_icon_get;
    itc.func.state_get = NULL;
    itc.func.del = NULL;

    for (index = 0; index < _uuids.size (); index++) {
        if (_modes[index] ==  TOOLBAR_HELPER_MODE)
            elm_genlist_item_append (genlist, &itc, (void *) index, NULL, ELM_GENLIST_ITEM_NONE, gl_ise_selected_cb, (void *)index);
    }

    elm_box_pack_end (box, genlist);
    evas_object_show (genlist);

    /* The height of popup being adjusted by application here based on app requirement */
    item_count = elm_genlist_items_count (genlist);

    if (item_count > ISE_SELECTOR_MAX_ITEM)
        height = ISE_SELECTOR_ITEM_HEIGHT * ISE_SELECTOR_MAX_ITEM;
    else
        height = ISE_SELECTOR_ITEM_HEIGHT * item_count;

    evas_object_size_hint_min_set (box, -1, height);
    evas_object_show (box);

    elm_object_content_set (_ise_selector_popup, box);
    evas_object_show (_ise_selector_popup);

    Evas_Object *center_popup_win = center_popup_win_get (_ise_selector_popup);
    if (center_popup_win) {
        evas_event_callback_add (evas_object_evas_get (center_popup_win), EVAS_CALLBACK_CANVAS_FOCUS_OUT, ise_selector_focus_out_cb, NULL);

        ecore_x_icccm_transient_for_set (elm_win_xwindow_get (center_popup_win), _ise_selector_app_window);
    }
}

void ise_selector_destroy ()
{
    if (_ise_selector_radio_grp) {
         evas_object_del (_ise_selector_radio_grp);
         _ise_selector_radio_grp = NULL;
    }

    if (_ise_selector_popup) {
        evas_object_del (_ise_selector_popup);
        _ise_selector_popup = NULL;
    }

    ise_selector_init ();
}
