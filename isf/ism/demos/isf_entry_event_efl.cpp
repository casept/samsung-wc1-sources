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

#include <Ecore_IMF.h>
#include "isf_demo_efl.h"

static Evas_Object * _entry1                 = NULL;
static Evas_Object * _entry2                 = NULL;
static Evas_Object * _key_event_label        = NULL;
static Evas_Object * _preedit_event_label    = NULL;
static Evas_Object * _commit_event_label     = NULL;
static Ecore_Event_Handler *_preedit_handler = NULL;
static Ecore_Event_Handler *_commit_handler  = NULL;

static void _input_panel_event_callback (void *data, Ecore_IMF_Context *ctx, int value)
{
    if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        LOGD ("ECORE_IMF_INPUT_PANEL_STATE_SHOW\n");
    } else if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        LOGD ("ECORE_IMF_INPUT_PANEL_STATE_HIDE\n");
    }
}

static void _evas_key_up_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    static char str [100];
    Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;

    if (obj == _entry1) {
        snprintf (str, sizeof (str), "entry 1  get keyEvent: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    } else if (obj == _entry2) {
        snprintf (str, sizeof (str), "entry 2  get keyEvent: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    }
}

static Eina_Bool _ecore_imf_event_changed_cb (void *data, int type, void *event)
{
    int len;
    static char str [100];
    char *preedit_string           = NULL;
    Ecore_IMF_Context *imf_context = NULL;

    if (elm_object_focus_get (_entry1) == EINA_TRUE) {
        imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry1);
        ecore_imf_context_preedit_string_get (imf_context, &preedit_string, &len);
        snprintf (str, sizeof (str), "entry 1 get preedit string: %s", preedit_string);
        elm_object_text_set (_preedit_event_label, str);
    } else if (elm_object_focus_get (_entry2) == EINA_TRUE) {
        imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry2);
        ecore_imf_context_preedit_string_get (imf_context, &preedit_string, &len);
        snprintf (str, sizeof (str), "entry 2 get preedit string: %s", preedit_string);
        elm_object_text_set (_preedit_event_label, str);
    }

    return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _ecore_imf_event_commit_cb (void *data, int type, void *event)
{
    static char str [100];
    Ecore_IMF_Event_Commit *ev = (Ecore_IMF_Event_Commit *) event;

    if (elm_object_focus_get (_entry1) == EINA_TRUE) {
        snprintf (str, sizeof (str), "entry 1 get commit string: %s", (char *)(ev->str));
        elm_object_text_set (_commit_event_label, str);
    } else if (elm_object_focus_get (_entry2) == EINA_TRUE) {
        snprintf (str, sizeof (str), "entry 2 get commit string: %s", (char *)(ev->str));
        elm_object_text_set (_commit_event_label, str);
    }
    return ECORE_CALLBACK_RENEW;
}

void isf_entry_event_demo_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *layout = NULL;
    Ecore_IMF_Context *ic = NULL;

    layout = elm_layout_add (ad->naviframe);
    evas_object_size_hint_weight_set (layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show (layout);

    _preedit_handler = ecore_event_handler_add (ECORE_IMF_EVENT_PREEDIT_CHANGED, _ecore_imf_event_changed_cb, NULL);
    _commit_handler  = ecore_event_handler_add (ECORE_IMF_EVENT_COMMIT, _ecore_imf_event_commit_cb, NULL);

    _entry1 = elm_entry_add (layout);
    elm_entry_entry_set (_entry1, "ENTRY 1");
    evas_object_move (_entry1, 0, 100);
    evas_object_resize (_entry1, ad->root_w, 50);
    evas_object_show (_entry1);
    evas_object_event_callback_add (_entry1, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, (void *)NULL);

    ic = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry1);
    if (ic != NULL)
        ecore_imf_context_input_panel_event_callback_add (ic, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, NULL);

    _entry2 = elm_entry_add (layout);
    elm_entry_entry_set (_entry2, "ENTRY 2");
    evas_object_move (_entry2, 0, 150);
    evas_object_resize (_entry2, ad->root_w, 50);
    evas_object_show (_entry2);
    evas_object_event_callback_add (_entry2, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, (void *)NULL);

    ic = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry2);
    if (ic != NULL)
        ecore_imf_context_input_panel_event_callback_add (ic, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, NULL);

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

    elm_naviframe_item_push (ad->naviframe, _("Entry Event"), NULL, NULL, layout, NULL);
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
