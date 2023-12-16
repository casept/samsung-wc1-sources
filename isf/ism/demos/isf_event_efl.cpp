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


static Ecore_IMF_Context* _imf_context[2];
static Evas_Object *_label1              = NULL;
static Evas_Object *_label2              = NULL;
static Evas_Object *_key_event_label     = NULL;
static Evas_Object *_preedit_event_label = NULL;
static Evas_Object *_commit_event_label  = NULL;
static Evas_Object *_set_focus_button1   = NULL;
static Evas_Object *_set_focus_button2   = NULL;
static Evas_Object *_ise_show_button     = NULL;
static Ecore_Event_Handler *_preedit_handler = NULL;
static Ecore_Event_Handler *_commit_handler  = NULL;
static int focus_label_idx = 1;

void isf_entry_event_demo_bt (void *data, Evas_Object *obj, void *event_info);

static void _set_focus_button_bt (void *data, Evas_Object *obj, void *event_info)
{
    int i = (int) data;

    focus_label_idx = i;

    ecore_imf_context_focus_in (_imf_context[i-1]);
}

static void _button_bt (void *data, Evas_Object *obj, void *event_info)
{
    if (_imf_context[focus_label_idx-1] != NULL)
        ecore_imf_context_input_panel_show (_imf_context[focus_label_idx-1]);
}

static void _key_up_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    static char str [100];
    Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;

    if (obj == _label1) {
        snprintf (str, sizeof (str), "label 1  get key up event: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    } else if (obj == _label2) {
        snprintf (str, sizeof (str), "label 2  get key up event: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    }
}

static void _key_down_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    static char str [100];
    Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;

    if (obj == _label1) {
        snprintf (str, sizeof (str), "label 1  get key down event: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    } else if (obj == _label2) {
        snprintf (str, sizeof (str), "label 2  get key down event: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    }
}

static Eina_Bool _ecore_imf_event_changed_cb (void *data, int type, void *event)
{
    static char str [100];

    char *preedit_string = NULL;
    preedit_string = (char *)malloc (sizeof (char)*70);
    int cursor_pos;

    ecore_imf_context_preedit_string_get (_imf_context[focus_label_idx-1], &preedit_string, &cursor_pos);

    if (preedit_string == NULL)
        return ECORE_CALLBACK_CANCEL;

    snprintf (str, sizeof (str), "label %d get preedit string: %s", focus_label_idx, preedit_string);

    free (preedit_string);
    elm_object_text_set (_preedit_event_label, str);

    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _ecore_imf_event_commit_cb (void *data, int type, void *event)
{
    static char str [100];
    Ecore_IMF_Event_Commit *ev = (Ecore_IMF_Event_Commit *) event;

    snprintf (str,sizeof (str), "label %d get commit string: %s", focus_label_idx, (char *)(ev->str));
    elm_object_text_set (_commit_event_label, str);
    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _nf_back_event_cb (void *data, Elm_Object_Item *it)
{
    evas_object_event_callback_del (_label1, EVAS_CALLBACK_KEY_UP, NULL);
    evas_object_event_callback_del (_label2, EVAS_CALLBACK_KEY_UP, NULL);

    for (int i = 0; i < 2; i++) {
        if (_imf_context[i]) {
            ecore_imf_context_del (_imf_context[i]);
            _imf_context[i] = NULL;
        }
    }

    if (_preedit_handler != NULL) {
        ecore_event_handler_del (_preedit_handler);
        _preedit_handler = NULL;
    }

    if (_commit_handler != NULL) {
        ecore_event_handler_del (_commit_handler);
        _commit_handler = NULL;
    }

    return EINA_TRUE;
}

static void isf_label_event_demo_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *layout = NULL;
    Evas *evas = NULL;
    Ecore_Window ecore_win;

    layout = elm_layout_add (ad->naviframe);
    evas_object_size_hint_weight_set (layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show (layout);

    evas = evas_object_evas_get (ad->win_main);
    ecore_win = ecore_evas_window_get (ecore_evas_ecore_evas_get (evas));

    const char *ctx_id = ecore_imf_context_default_id_get ();

    /* register preedit (composing) event handler */
    _preedit_handler = ecore_event_handler_add (ECORE_IMF_EVENT_PREEDIT_CHANGED, _ecore_imf_event_changed_cb, NULL);
    /* register commit event handler */
    _commit_handler = ecore_event_handler_add (ECORE_IMF_EVENT_COMMIT, _ecore_imf_event_commit_cb, NULL);

    /* create label2 */
    _label1 = elm_label_add (layout);
    elm_object_text_set (_label1, "LABEL 1");
    evas_object_move (_label1, 0, 100);
    evas_object_resize (_label1, ad->root_w, 50);
    evas_object_show (_label1);
    evas_object_event_callback_add (_label1, EVAS_CALLBACK_KEY_UP, _key_up_cb, (void *)NULL);
    evas_object_event_callback_add (_label1, EVAS_CALLBACK_KEY_DOWN, _key_down_cb, (void *)NULL);

    /* create input context for label1 */
    _imf_context[0] = ecore_imf_context_add (ctx_id);
    ecore_imf_context_client_window_set (_imf_context[0], (void *)ecore_win);
    ecore_imf_context_client_canvas_set (_imf_context[0], evas_object_evas_get (_label1));
    ecore_imf_context_focus_in (_imf_context[0]);

    /* create label2 */
    _label2 = elm_label_add (layout);
    elm_object_text_set (_label2, "LABEL 2");
    evas_object_move (_label2, 0, 150);
    evas_object_resize (_label2, ad->root_w, 50);
    evas_object_show (_label2);
    evas_object_event_callback_add (_label2, EVAS_CALLBACK_KEY_UP, _key_up_cb, (void *)NULL);
    evas_object_event_callback_add (_label2, EVAS_CALLBACK_KEY_DOWN, _key_down_cb, (void *)NULL);

    /* create input context for label2 */
    _imf_context[1] = ecore_imf_context_add (ctx_id);
    ecore_imf_context_client_window_set (_imf_context[1], (void *)ecore_win);
    ecore_imf_context_client_canvas_set (_imf_context[1], evas_object_evas_get (_label2));

    _key_event_label = elm_button_add (layout);
    elm_object_text_set (_key_event_label, "KEY EVENT");
    evas_object_move (_key_event_label, 0, 200);
    evas_object_resize (_key_event_label, ad->root_w, 50);
    evas_object_show (_key_event_label);

    _preedit_event_label = elm_button_add (layout);
    elm_object_text_set (_preedit_event_label, "PREEDIT EVENT");
    evas_object_move (_preedit_event_label, 0, 250);
    evas_object_resize (_preedit_event_label, ad->root_w, 50);
    evas_object_show (_preedit_event_label);

    _commit_event_label = elm_button_add (layout);
    elm_object_text_set (_commit_event_label, "COMMIT EVENT");
    evas_object_move (_commit_event_label, 0, 300);
    evas_object_resize (_commit_event_label, ad->root_w, 50);
    evas_object_show (_commit_event_label);

    _set_focus_button1 = elm_button_add (layout);
    elm_object_text_set (_set_focus_button1, "SET FOCUS TO LABEL 1");
    evas_object_move (_set_focus_button1, 0, 350);
    evas_object_resize (_set_focus_button1, ad->root_w, 50);
    evas_object_show (_set_focus_button1);
    evas_object_smart_callback_add (_set_focus_button1, "clicked", _set_focus_button_bt, (void *)1);

    _set_focus_button2 = elm_button_add (layout);
    elm_object_text_set (_set_focus_button2, "SET FOCUS TO LABEL 2");
    evas_object_move (_set_focus_button2, 0, 400);
    evas_object_resize (_set_focus_button2, ad->root_w, 50);
    evas_object_show (_set_focus_button2);
    evas_object_smart_callback_add (_set_focus_button2, "clicked", _set_focus_button_bt, (void *)2);

    _ise_show_button = elm_button_add (layout);
    elm_object_text_set (_ise_show_button, "ISE SHOW");
    evas_object_move (_ise_show_button, 0, 450);
    evas_object_resize (_ise_show_button, ad->root_w, 50);
    evas_object_show (_ise_show_button);
    evas_object_smart_callback_add (_ise_show_button, "clicked", _button_bt, NULL);

    Elm_Object_Item *it = elm_naviframe_item_push (ad->naviframe, _("Label Event"), NULL, NULL, layout, NULL);
    elm_naviframe_item_pop_cb_set (it, _nf_back_event_cb, ad);
}

static void _list_click (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    if (ad == NULL) return;

    Evas_Object *li = ad->ev_li;
    if (li == NULL) return;

    Elm_Object_Item *it = (Elm_Object_Item *)elm_list_selected_item_get (li);

    if (it != NULL)
        elm_list_item_selected_set (it, EINA_FALSE);
}

void isf_event_demo_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;

    ad->ev_li = elm_list_add (ad->naviframe);
    elm_list_mode_set (ad->ev_li, ELM_LIST_COMPRESS);
    evas_object_smart_callback_add (ad->ev_li, "selected", _list_click, ad);
    elm_list_item_append (ad->ev_li, "Label Event", NULL, NULL, isf_label_event_demo_bt, ad);
    elm_list_item_append (ad->ev_li, "Entry Event", NULL, NULL, isf_entry_event_demo_bt, ad);
    elm_list_go (ad->ev_li);

    elm_naviframe_item_push (ad->naviframe, _("Event"), NULL, NULL, ad->ev_li, NULL);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
