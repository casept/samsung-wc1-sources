/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Haifeng Deng <haifeng.deng@samsung.com>
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

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_HOTKEY


/* IM control UI part */
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <glib.h>
#include "scim.h"
#include "isf_imf_control_ui.h"
#include "isf_imf_context.h"
#include "isf_imf_control.h"
#include "ise_context.h"

using namespace scim;

/* IM control related variables */
static Ise_Context        iseContext;
static bool               IfInitContext     = false;
static Ecore_IMF_Context *show_req_ic       = NULL;
static Ecore_IMF_Context *hide_req_ic       = NULL;
static Ecore_Event_Handler *_prop_change_handler = NULL;
static Ecore_X_Atom       prop_x_keyboard_input_detected = 0;
static Ecore_X_Window     _control_win;
static TOOLBAR_MODE_T     kbd_mode = TOOLBAR_HELPER_MODE;
static Ecore_Timer       *hide_timer = NULL;
static Ecore_Timer       *will_show_timer = NULL;
Ecore_IMF_Input_Panel_State input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
Ecore_IMF_Input_Panel_State notified_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
static int                hide_context_id = -1;
static Evas              *active_context_canvas = NULL;
static Ecore_X_Window     active_context_window = -1;
Ecore_IMF_Context        *input_panel_ctx = NULL;
static Ecore_Event_Handler *_win_focus_out_handler = NULL;
static Eina_Bool          conformant_reset_done = EINA_FALSE;
static Eina_Bool          candidate_conformant_reset_done = EINA_FALSE;
static Eina_Bool          received_will_hide_event = EINA_FALSE;
static Eina_Bool          received_candidate_will_hide_event = EINA_FALSE;
static Eina_Bool          will_hide = EINA_FALSE;
static bool               _support_hw_keyboard_mode = false;

static void _send_input_panel_hide_request ();

Ecore_IMF_Context *get_using_ic (Ecore_IMF_Input_Panel_Event type, int value) {
    Ecore_IMF_Context *using_ic = NULL;
    if (show_req_ic)
        using_ic = show_req_ic;
    else if (get_focused_ic ())
        using_ic = get_focused_ic ()->ctx;

    if (type == ECORE_IMF_INPUT_PANEL_STATE_EVENT &&
        value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        if (hide_req_ic) {
            using_ic = hide_req_ic;
        }
    }
    return using_ic;
}

static void _render_post_cb (void *data, Evas *e, void *event_info)
{
    LOGD ("[_render_post_cb], conformant_reset_done = 1\n");
    evas_event_callback_del_full (e, EVAS_CALLBACK_RENDER_POST, _render_post_cb, NULL);
    conformant_reset_done = EINA_TRUE;
    isf_imf_context_input_panel_send_will_hide_ack (get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW));
}

static void _candidate_render_post_cb (void *data, Evas *e, void *event_info)
{
    LOGD ("[%s], candidate_conformant_reset_done = 1\n", __func__);
    evas_event_callback_del_full (e, EVAS_CALLBACK_RENDER_POST, _candidate_render_post_cb, NULL);
    candidate_conformant_reset_done = EINA_TRUE;
    isf_imf_context_input_panel_send_candidate_will_hide_ack (get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW));
}

static void _clear_will_show_timer ()
{
    if (will_show_timer) {
        ecore_timer_del (will_show_timer);
        will_show_timer = NULL;
    }
}

static Eina_Bool _clear_hide_timer ()
{
    if (hide_timer) {
        ecore_timer_del (hide_timer);
        hide_timer = NULL;
        return EINA_TRUE;
    }

    return EINA_FALSE;
}

static Eina_Bool _conformant_get ()
{
    return ecore_x_e_illume_conformant_get (active_context_window);
}

Ecore_X_Window client_window_id_get (Ecore_IMF_Context *ctx)
{
    Ecore_X_Window xid = 0;
    Evas *evas = NULL;
    Ecore_Evas *ee = NULL;

    evas = (Evas *)ecore_imf_context_client_canvas_get (ctx);

    if (evas) {
        ee = ecore_evas_ecore_evas_get (evas);
        if (ee)
            xid = (Ecore_X_Window)ecore_evas_window_get (ee);
    } else {
        xid = (Ecore_X_Window)ecore_imf_context_client_window_get (ctx);
    }

    return xid;
}

static Eina_Bool _prop_change (void *data, int ev_type, void *ev)
{
    Ecore_X_Event_Window_Property *event = (Ecore_X_Event_Window_Property *)ev;
    unsigned int val = 0;
    int sx = -1, sy = -1, sw = -1, sh = -1;

    if (event->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) {
        if (event->win == active_context_window) {
            Ecore_X_Virtual_Keyboard_State state = ecore_x_e_virtual_keyboard_state_get (event->win);

            if (!ecore_x_e_illume_keyboard_geometry_get (event->win, &sx, &sy, &sw, &sh))
                sx = sy = sw = sh = 0;

            if (state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) {
                LOGD ("conformant_reset_done = 0, candidate_conformant_reset_done = 0, registering _render_post_cb\n");
                conformant_reset_done = EINA_FALSE;
                candidate_conformant_reset_done = EINA_FALSE;
                if (active_context_canvas && _conformant_get ()) {
                    evas_event_callback_add (active_context_canvas, EVAS_CALLBACK_RENDER_POST, _render_post_cb, NULL);
                }

                LOGD ("[ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF] geometry x : %d, y : %d, w : %d, h : %d\n", sx, sy, sw, sh);
            }
            else if (state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON) {
                LOGD ("conformant_reset_done = 0, candidate_conformant_reset_done = 0, registering _candidate_render_post_cb\n");
                conformant_reset_done = EINA_FALSE;
                candidate_conformant_reset_done = EINA_FALSE;
                _clear_will_show_timer ();
                if (active_context_canvas && _conformant_get ()) {
                    evas_event_callback_add (active_context_canvas, EVAS_CALLBACK_RENDER_POST, _candidate_render_post_cb, NULL);
                }
                LOGD ("[ECORE_X_VIRTUAL_KEYBOARD_STATE_ON] geometry x : %d, y : %d, w : %d, h : %d\n", sx, sy, sw, sh);
            }
        }
    } else {
        if (event->win != _control_win) return ECORE_CALLBACK_PASS_ON;
        if (event->atom != prop_x_keyboard_input_detected) return ECORE_CALLBACK_PASS_ON;
        if (ecore_x_window_prop_card32_get (event->win, prop_x_keyboard_input_detected, &val, 1) <= 0)
            return ECORE_CALLBACK_PASS_ON;

        if (val == 0) {
            kbd_mode = TOOLBAR_HELPER_MODE;
            LOGD ("keyboard mode(0:H/W Keyboard, 1:S/W Keyboard): %d\n", kbd_mode);
        }
    }

    return ECORE_CALLBACK_PASS_ON;
}

void save_current_xid (Ecore_IMF_Context *ctx)
{
    Ecore_X_Window xid = 0, rootwin_xid = 0;

    xid = client_window_id_get (ctx);

    if (xid == 0)
        rootwin_xid = ecore_x_window_root_first_get ();
    else
        rootwin_xid = ecore_x_window_root_get (xid);

    active_context_window = xid;

    Ecore_X_Atom isf_active_window_atom = ecore_x_atom_get ("_ISF_ACTIVE_WINDOW");
    ecore_x_window_prop_property_set (rootwin_xid, isf_active_window_atom, ((Ecore_X_Atom) 33), 32, &xid, 1);
    ecore_x_flush ();
    ecore_x_sync ();
}

void clear_hide_request ()
{
    if (_clear_hide_timer ()) {
        hide_req_ic = NULL;
    }
    will_hide = EINA_FALSE;
}

static void _event_callback_call (Ecore_IMF_Input_Panel_Event type, int value)
{
    Ecore_IMF_Context *using_ic = get_using_ic (type, value);

    switch (type) {
        case ECORE_IMF_INPUT_PANEL_STATE_EVENT:
            switch (value) {
                case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
                    SECURE_LOGD ("[input panel has been hidden] ctx : %p\n", using_ic);
                    if (hide_req_ic == show_req_ic)
                        show_req_ic = NULL;

                    hide_req_ic = NULL;
                    will_hide = EINA_FALSE;
                    break;
                case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
                    SECURE_LOGD ("[input panel has been shown] ctx : %p\n", using_ic);
                    break;
                case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
                    SECURE_LOGD ("[input panel will be shown] ctx : %p\n", using_ic);
                    break;
                default:
                    break;
            }
            notified_state = (Ecore_IMF_Input_Panel_State)value;
            break;
        case ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT:
            SECURE_LOGD ("[language is changed] ctx : %p\n", using_ic);
            break;
        case ECORE_IMF_INPUT_PANEL_SHIFT_MODE_EVENT:
            SECURE_LOGD ("[shift mode is changed] ctx : %p\n", using_ic);
            break;
        case ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT:
            SECURE_LOGD ("[input panel geometry is changed] ctx : %p\n", using_ic);
            break;
        case ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT:
            SECURE_LOGD ("[candidate state is changed] ctx : %p, %d\n", using_ic, value);
            break;
        case ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT:
            SECURE_LOGD ("[candidate geometry is changed] ctx : %p\n", using_ic);
            break;
        default:
            break;
    }

    if (using_ic)
        ecore_imf_context_input_panel_event_callback_call (using_ic, type, value);

    if (type == ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT) {
        if (value == ECORE_IMF_CANDIDATE_PANEL_HIDE) {
            if (notified_state != ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
                SECURE_LOGD ("received_candidate_will_hide_event = 1");
                received_candidate_will_hide_event = TRUE;
                isf_imf_context_input_panel_send_candidate_will_hide_ack (using_ic);
            } else {
                SECURE_LOGD ("received_candidate_will_hide_event != 1, due to notified_state");
            }
        } else if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
            SECURE_LOGD ("received_candidate_will_hide_event = 0");
            received_candidate_will_hide_event = EINA_FALSE;
        }
    }
}

static void _isf_imf_context_init (void)
{
    iseContext.language            = ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC;
    iseContext.return_key_type     = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
    iseContext.return_key_disabled = FALSE;
    iseContext.prediction_allow    = TRUE;

    if (!IfInitContext) {
        IfInitContext = true;
    }
}

static int _get_context_id (Ecore_IMF_Context *ctx)
{
    EcoreIMFContextISF *context_scim = NULL;
    if (!ctx) return -1;

    context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);
    if (!context_scim) return -1;

    return context_scim->id;
}

static void _save_hide_context_info (Ecore_IMF_Context *ctx)
{
    hide_context_id = _get_context_id (ctx);
    active_context_window = client_window_id_get (ctx);
    active_context_canvas = (Evas *)ecore_imf_context_client_canvas_get (ctx);
}

static void _win_focus_out_handler_del ()
{
    if (_win_focus_out_handler) {
        ecore_event_handler_del (_win_focus_out_handler);
        _win_focus_out_handler = NULL;
    }
}

static void _send_input_panel_hide_request ()
{
    if (hide_context_id < 0) return;

    _win_focus_out_handler_del ();

    SECURE_LOGD ("Send input panel hide request\n");

    _isf_imf_context_input_panel_hide (get_panel_client_id (), hide_context_id);
    hide_context_id = -1;
}

static Eina_Bool _hide_timer_handler (void *data)
{
    _send_input_panel_hide_request ();

    hide_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _will_show_timer_handler (void *data)
{
    SECURE_LOGD ("reset input panel state as HIDE\n");

    input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
    will_show_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void _input_panel_hide_timer_start (void *data)
{
    Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
    _save_hide_context_info (ctx);

    if (!hide_timer)
        hide_timer = ecore_timer_add (HIDE_TIMER_INTERVAL, _hide_timer_handler, data);
}

static void _input_panel_hide (Ecore_IMF_Context *ctx, Eina_Bool instant)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    will_hide = EINA_TRUE;
    hide_req_ic = ctx;

    if (instant || (hide_timer && ecore_timer_pending_get (hide_timer) <= 0.0)) {
        _clear_hide_timer ();
        _save_hide_context_info (ctx);
        _send_input_panel_hide_request ();
    } else {
        _input_panel_hide_timer_start (ctx);
    }
}

static Eina_Bool _compare_context (Ecore_IMF_Context *ctx1, Ecore_IMF_Context *ctx2)
{
    if (!ctx1 || !ctx2) return EINA_FALSE;

    if ((ecore_imf_context_autocapital_type_get (ctx1) == ecore_imf_context_autocapital_type_get (ctx2)) &&
        (ecore_imf_context_input_panel_layout_get (ctx1) == ecore_imf_context_input_panel_layout_get (ctx2)) &&
        (ecore_imf_context_input_panel_language_get (ctx1) == ecore_imf_context_input_panel_language_get (ctx2)) &&
        (ecore_imf_context_input_panel_return_key_type_get (ctx1) == ecore_imf_context_input_panel_return_key_type_get (ctx2)) &&
        (ecore_imf_context_input_panel_return_key_disabled_get (ctx1) == ecore_imf_context_input_panel_return_key_disabled_get (ctx2)) &&
        (ecore_imf_context_input_panel_caps_lock_mode_get (ctx1) == ecore_imf_context_input_panel_caps_lock_mode_get (ctx2)))
        return EINA_TRUE;

    return EINA_FALSE;
}

static Eina_Bool _client_window_focus_out_cb (void *data, int ev_type, void *ev)
{
    Ecore_X_Event_Window_Focus_Out *e = (Ecore_X_Event_Window_Focus_Out *)ev;
    Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)data;
    if (!ctx || !e) return ECORE_CALLBACK_PASS_ON;
    Ecore_X_Window client_win = client_window_id_get (ctx);
    Ecore_X_Window focus_out_win = e->win;
    Ecore_X_Window focus_in_win = ecore_x_window_focus_get ();

    LOGD ("ctx : %p, client_window : %#x, focus-out win : %#x, focus-in win : %#x\n", ctx, client_win, focus_out_win, focus_in_win);

    if (check_focus_out_by_popup_win (ctx))
        return ECORE_CALLBACK_PASS_ON;

    if (client_win > 0) {
        if ((focus_out_win == client_win) && (focus_out_win != focus_in_win))
            isf_imf_context_input_panel_instant_hide (ctx);
    }
    else {
        _clear_hide_timer ();
        _send_input_panel_hide_request ();
    }

    return ECORE_CALLBACK_PASS_ON;
}

void input_panel_event_callback_call (Ecore_IMF_Input_Panel_Event type, int value)
{
    _event_callback_call (type, value);
}

Eina_Bool check_focus_out_by_popup_win (Ecore_IMF_Context *ctx)
{
    Ecore_X_Window focus_win = ecore_x_window_focus_get ();
    Eina_Bool ret = EINA_FALSE;
    Ecore_X_Window_Type win_type = ECORE_X_WINDOW_TYPE_UNKNOWN;
    Ecore_X_Window client_win = client_window_id_get (ctx);
    char *class_name = NULL;

    if (!ecore_x_netwm_window_type_get (focus_win, &win_type))
        return EINA_FALSE;

    LOGD ("win type : %d\n", win_type);

    if (win_type == ECORE_X_WINDOW_TYPE_POPUP_MENU ||
        win_type == ECORE_X_WINDOW_TYPE_NOTIFICATION) {
        LOGD ("client window : %#x, focus window : %#x\n", client_win, focus_win);

        if (client_win != focus_win) {
            ecore_x_icccm_name_class_get (focus_win, NULL, &class_name);

            if (class_name) {
                if (strncmp (class_name, "LOCK_SCREEN", 11) != 0) {
                    ret = EINA_TRUE;
                }

                free (class_name);
                class_name = NULL;
            }
        }
    }

    return ret;
}

TOOLBAR_MODE_T get_keyboard_mode ()
{
    return kbd_mode;
}

void isf_imf_context_control_panel_show (Ecore_IMF_Context *ctx)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }
    _isf_imf_context_control_panel_show (_get_context_id (ctx));
}

void isf_imf_context_control_panel_hide (Ecore_IMF_Context *ctx)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }
    _isf_imf_context_control_panel_hide (_get_context_id (ctx));
}

void isf_imf_input_panel_init (void)
{
    unsigned int val = 0;
    if (_prop_change_handler) return;

    Ecore_X_Atom atom = ecore_x_atom_get ("_ISF_CONTROL_WINDOW");
    ecore_x_window_prop_xid_get (ecore_x_window_root_first_get (), atom, ECORE_X_ATOM_WINDOW, &_control_win, 1);
    ecore_x_event_mask_set (_control_win, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);

    _prop_change_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_PROPERTY, _prop_change, NULL);
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    if (_support_hw_keyboard_mode){
        if (!prop_x_keyboard_input_detected)
            prop_x_keyboard_input_detected = ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED);

        if (ecore_x_window_prop_card32_get (_control_win, prop_x_keyboard_input_detected, &val, 1) > 0){
            if (val == 1){
                kbd_mode = TOOLBAR_KEYBOARD_MODE;
            } else {
                kbd_mode = TOOLBAR_HELPER_MODE;
            }
        } else {
            kbd_mode = TOOLBAR_HELPER_MODE;
        }
    }
    LOGD ("keyboard mode(0:H/W Keyboard, 1:S/W Keyboard): %d\n", kbd_mode);
}

void isf_imf_input_panel_shutdown (void)
{
    if (show_req_ic)
        isf_imf_context_input_panel_instant_hide (show_req_ic);
    else {
        if (hide_timer) {
            _send_input_panel_hide_request ();
        }
        else
            LOGD ("No hide timer\n");
    }

    candidate_conformant_reset_done = TRUE;
    SECURE_LOGD ("candidate_conformant_reset_done = 1");

    Ecore_IMF_Context *active_ctx = get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);
    if (active_ctx) {
        isf_imf_context_input_panel_send_candidate_will_hide_ack (active_ctx);
    }

    if (_prop_change_handler) {
        ecore_event_handler_del (_prop_change_handler);
        _prop_change_handler = NULL;
    }

    _win_focus_out_handler_del ();

    _clear_will_show_timer ();
    _clear_hide_timer ();
}

void isf_imf_context_input_panel_show (Ecore_IMF_Context* ctx)
{
    int length = -1;
    void *packet = NULL;
    char imdata[1024] = {0};
    bool input_panel_show = false;
    input_panel_ctx = ctx;

    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    _win_focus_out_handler_del ();
    _win_focus_out_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_FOCUS_OUT, _client_window_focus_out_cb, ctx);

    /* set password mode */
    iseContext.password_mode = !!(ecore_imf_context_input_mode_get (ctx) & ECORE_IMF_INPUT_MODE_INVISIBLE);

    /* set language */
    iseContext.language = ecore_imf_context_input_panel_language_get (ctx);

    /* set layout in ise context info */
    iseContext.layout = ecore_imf_context_input_panel_layout_get (ctx);

    /* set layout variation in ise context info */
    iseContext.layout_variation = ecore_imf_context_input_panel_layout_variation_get (ctx);

    /* set prediction allow */
    iseContext.prediction_allow = ecore_imf_context_prediction_allow_get (ctx);

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD)
        iseContext.password_mode = EINA_TRUE;

    if (iseContext.password_mode)
        iseContext.prediction_allow = EINA_FALSE;

    if (iseContext.layout == ECORE_IMF_INPUT_PANEL_LAYOUT_TERMINAL)
        iseContext.prediction_allow = EINA_FALSE;

    isf_imf_context_prediction_allow_set (ctx, iseContext.prediction_allow);

    active_context_canvas = (Evas *)ecore_imf_context_client_canvas_get (ctx);

    /* Set the current XID of the active window into the root window property */
    save_current_xid (ctx);

    if (kbd_mode == TOOLBAR_KEYBOARD_MODE) {
        LOGD ("H/W keyboard is existed.\n");
        return;
    }

    if (_clear_hide_timer ()) {
        hide_req_ic = NULL;
    }

    if ((show_req_ic == ctx) &&
        (_compare_context (show_req_ic, ctx) == EINA_TRUE) &&
        (!will_hide)) {
        Ecore_IMF_Input_Panel_State state = ecore_imf_context_input_panel_state_get (ctx);
        if (state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW ||
            state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
            LOGD ("already show\n");
            return;
        }
    }

    will_hide = EINA_FALSE;
    show_req_ic = ctx;

    /* set return key type */
    iseContext.return_key_type = ecore_imf_context_input_panel_return_key_type_get (ctx);

    /* set return key disabled */
    iseContext.return_key_disabled = ecore_imf_context_input_panel_return_key_disabled_get (ctx);

    /* set caps mode */
    iseContext.caps_mode = caps_mode_check (ctx, EINA_TRUE, EINA_FALSE);

    /* set X Client window ID */
    iseContext.client_window = client_window_id_get (ctx);

    /* set the size of imdata */
    context_scim_imdata_get (ctx, (void *)imdata, &iseContext.imdata_size);

    /* set the cursor position of the editable widget */
    ecore_imf_context_surrounding_get (ctx, NULL, &iseContext.cursor_pos);

    iseContext.autocapital_type = ecore_imf_context_autocapital_type_get (ctx);

    iseContext.input_hint = ecore_imf_context_input_hint_get (ctx);

    iseContext.bidi_direction = ecore_imf_context_bidi_direction_get (ctx);

    SECURE_LOGD ("ctx : %p, layout : %d, layout variation : %d\n", ctx, iseContext.layout, iseContext.layout_variation);
    SECURE_LOGD ("language : %d, cursor position : %d, caps mode : %d\n", iseContext.language, iseContext.cursor_pos, iseContext.caps_mode);
    SECURE_LOGD ("return_key_type : %d, return_key_disabled : %d, autocapital type : %d\n", iseContext.return_key_type, iseContext.return_key_disabled, iseContext.autocapital_type);
    SECURE_LOGD ("client_window : %#x, password mode : %d, prediction_allow : %d\n", iseContext.client_window, iseContext.password_mode, iseContext.prediction_allow);
    SECURE_LOGD ("input hint : %#x\n", iseContext.input_hint);
    SECURE_LOGD ("bidi direction : %d\n", iseContext.bidi_direction);

    if (iseContext.client_window != ecore_x_window_focus_get ()) {
        LOGW ("Client window is different from focus window. client win : %#x, focus win : %#x\n", iseContext.client_window, ecore_x_window_focus_get ());
    }

    /* calculate packet size */
    length = sizeof (iseContext);
    length += iseContext.imdata_size;

    /* create packet */
    packet = calloc (1, length);
    if (!packet)
        return;

    memcpy (packet, (void *)&iseContext, sizeof (iseContext));
    memcpy ((void *)((unsigned int)packet + sizeof (iseContext)), (void *)imdata, iseContext.imdata_size);

    int context_id = _get_context_id (ctx);

    _isf_imf_context_input_panel_show (get_panel_client_id (), context_id, packet, length, input_panel_show);

    if (input_panel_show == true && input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;

        _clear_will_show_timer ();
        will_show_timer = ecore_timer_add (WILL_SHOW_TIMER_INTERVAL, _will_show_timer_handler, NULL);
    }

    free (packet);

    caps_mode_check (ctx, EINA_TRUE, EINA_TRUE);

    if (notified_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE &&
        input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW)
        _event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);
}

void isf_imf_context_input_panel_hide (Ecore_IMF_Context *ctx)
{
    SECURE_LOGD ("ctx : %p\n", ctx);

    _input_panel_hide (ctx, EINA_FALSE);
}

void isf_imf_context_input_panel_instant_hide (Ecore_IMF_Context *ctx)
{
    _input_panel_hide (ctx, EINA_TRUE);
}

void isf_imf_context_input_panel_language_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Lang language)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();
    iseContext.language = language;

    if (context_scim == get_focused_ic ()) {
        SECURE_LOGD ("language mode : %d\n", language);
        _isf_imf_context_input_panel_language_set (_get_context_id (ctx), language);
    }
}

Ecore_IMF_Input_Panel_Lang isf_imf_context_input_panel_language_get (Ecore_IMF_Context *ctx)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    return iseContext.language;
}

void isf_imf_context_input_panel_language_locale_get (Ecore_IMF_Context *ctx, char **locale)
{
    if (!IfInitContext)
        _isf_imf_context_init ();

    if (locale)
        _isf_imf_context_input_panel_language_locale_get (_get_context_id (ctx), locale);
}

void isf_imf_context_input_panel_caps_mode_set (Ecore_IMF_Context *ctx, unsigned int mode)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    SECURE_LOGD ("ctx : %p, mode : %d\n", ctx, mode);
    _isf_imf_context_input_panel_caps_mode_set (_get_context_id (ctx), mode);
}

void isf_imf_context_input_panel_caps_lock_mode_set (Ecore_IMF_Context *ctx, Eina_Bool mode)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ())
        caps_mode_check (ctx, EINA_TRUE, EINA_TRUE);
}

/**
 * Set up an ISE specific data
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] data pointer of data to sets up to ISE
 * @param[in] length length of data
 */
void isf_imf_context_input_panel_imdata_set (Ecore_IMF_Context *ctx, const void* data, int length)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_imdata_set (_get_context_id (ctx), data, length);
}

/**
 * Get the ISE specific data from ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] data pointer of data to return
 * @param[out] length length of data
 */
void isf_imf_context_input_panel_imdata_get (Ecore_IMF_Context *ctx, void* data, int* length)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_imdata_get (_get_context_id (ctx), data, length);
}

/**
 * Get ISE's position and size, in screen coordinates of the ISE rectangle not the client area,
 * the represents the size and location of the ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] x the x position of ISE window
 * @param[out] y the y position of ISE window
 * @param[out] w the width of ISE window
 * @param[out] h the height of ISE window
 */
void isf_imf_context_input_panel_geometry_get (Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_geometry_get (_get_context_id (ctx), x, y, w, h);

    SECURE_LOGD ("ctx : %p, x : %d, y : %d, w : %d, h : %d\n", ctx, *x, *y, *w, *h);
}

/**
 * Set type of return key.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] type the type of return key
 */
void isf_imf_context_input_panel_return_key_type_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Return_Key_Type type)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ()) {
        SECURE_LOGD ("Return key type : %d\n", type);
        _isf_imf_context_input_panel_return_key_type_set (_get_context_id (ctx), type);
    }
}

/**
 * Get type of return key.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 *
 * @return the type of return key.
 */
Ecore_IMF_Input_Panel_Return_Key_Type isf_imf_context_input_panel_return_key_type_get (Ecore_IMF_Context *ctx)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    Ecore_IMF_Input_Panel_Return_Key_Type type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_return_key_type_get (_get_context_id (ctx), type);

    return type;
}

/**
 * Make return key to be disabled in active ISE keyboard layout for own's application.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] disabled the state
 */
void isf_imf_context_input_panel_return_key_disabled_set (Ecore_IMF_Context *ctx, Eina_Bool disabled)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ()) {
        SECURE_LOGD ("Return key disabled : %d\n", disabled);
        _isf_imf_context_input_panel_return_key_disabled_set (_get_context_id (ctx), disabled);
    }
}

/**
 * Get the disabled status of return key.
 *
 * @param[in] ctx a #Ecore_IMF_Context
 *
 * @return the disable state of return key.
 */
Eina_Bool isf_imf_context_input_panel_return_key_disabled_get (Ecore_IMF_Context *ctx)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    Eina_Bool disabled = EINA_FALSE;
    if (context_scim == get_focused_ic ())
        _isf_imf_context_input_panel_return_key_disabled_get (_get_context_id (ctx), disabled);

    return disabled;
}

/**
 * Sets up the layout information of active ISE
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[in] layout sets a layout ID to be shown. The layout ID will define by the configuration of selected ISE.
 */
void
isf_imf_context_input_panel_layout_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ()) {
        SECURE_LOGD ("layout type : %d\n", layout);
        _isf_imf_context_input_panel_layout_set (_get_context_id (ctx), layout);
        imengine_layout_set (ctx, layout);
    }
}

/**
* Get current ISE layout
*
* @param[in] ctx a #Ecore_IMF_Context
*
* @return the layout of current ISE.
*/
Ecore_IMF_Input_Panel_Layout isf_imf_context_input_panel_layout_get (Ecore_IMF_Context *ctx)
{
    Ecore_IMF_Input_Panel_Layout layout;
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_input_panel_layout_get (_get_context_id (ctx), &layout);

    return layout;
}

/**
 * Get current ISE state
 *
 * @param[in] ctx a #Ecore_IMF_Context
 *
 * @return the state of current ISE.
 */
Ecore_IMF_Input_Panel_State isf_imf_context_input_panel_state_get (Ecore_IMF_Context *ctx)
{
    Ecore_IMF_Input_Panel_State state;
    if (!IfInitContext)
        _isf_imf_context_init ();

    _isf_imf_context_input_panel_state_get (_get_context_id (ctx), state);
    LOGD ("    state:%d\n", state);
    return state;
}

void isf_imf_context_input_panel_event_callback_clear (Ecore_IMF_Context *ctx)
{
    ecore_imf_context_input_panel_event_callback_clear (ctx);

    if (input_panel_ctx == ctx)
        _win_focus_out_handler_del ();

    if (show_req_ic == ctx)
        show_req_ic = NULL;

    if (hide_req_ic == ctx)
        hide_req_ic = NULL;
}

/**
 * Get candidate window position and size, in screen coordinates of the candidate rectangle not the client area,
 * the represents the size and location of the candidate window
 *
 * @param[in] ctx a #Ecore_IMF_Context
 * @param[out] x the x position of candidate window
 * @param[out] y the y position of candidate window
 * @param[out] w the width of candidate window
 * @param[out] h the height of candidate window
 */
void isf_imf_context_candidate_window_geometry_get (Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h)
{
    if (!IfInitContext)
        _isf_imf_context_init ();
    _isf_imf_context_candidate_window_geometry_get (_get_context_id (ctx), x, y, w, h);

    SECURE_LOGD ("ctx : %p, x : %d, y : %d, w : %d, h : %d\n", ctx, *x, *y, *w, *h);
}

void isf_imf_context_input_panel_send_will_show_ack (Ecore_IMF_Context *ctx)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    _isf_imf_context_input_panel_send_will_show_ack (_get_context_id (ctx));
}

void isf_imf_context_input_panel_send_will_hide_ack (Ecore_IMF_Context *ctx)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    if (_conformant_get ()) {
        if (conformant_reset_done && received_will_hide_event) {
            LOGD ("Send will hide ack, conformant_reset_done = 0, received_will_hide_event = 0\n");
            _isf_imf_context_input_panel_send_will_hide_ack (_get_context_id (ctx));
            conformant_reset_done = EINA_FALSE;
            received_will_hide_event = EINA_FALSE;
        } else {
            LOGD ("conformant_reset_done=%d, received_will_hide_event=%d\n",
                conformant_reset_done, received_will_hide_event);
        }
    } else {
        _isf_imf_context_input_panel_send_will_hide_ack (_get_context_id (ctx));
    }
}

void isf_imf_context_set_keyboard_mode (Ecore_IMF_Context *ctx, TOOLBAR_MODE_T mode)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    if (_support_hw_keyboard_mode) {
        kbd_mode = mode;
        SECURE_LOGD ("keyboard mode : %d\n", kbd_mode);
        _isf_imf_context_set_keyboard_mode (_get_context_id (ctx), mode);
    }
}

void isf_imf_context_input_panel_send_candidate_will_hide_ack (Ecore_IMF_Context *ctx)
{
    if (IfInitContext == false) {
        _isf_imf_context_init ();
    }

    if (_conformant_get ()) {
        if (candidate_conformant_reset_done && received_candidate_will_hide_event) {
            LOGD ("Send candidate will hide ack, cand_conf_reset_done = 0, recv_cand_will_hide = 0\n");
            _isf_imf_context_input_panel_send_candidate_will_hide_ack (_get_context_id (ctx));
            candidate_conformant_reset_done = EINA_FALSE;
            received_candidate_will_hide_event = EINA_FALSE;
        }
        else {
            LOGD ("isf_imf_context_input_panel_send_will_hide_ack, cand_conf_reset_done=%d recv_cand_will_hide=%d\n",
                candidate_conformant_reset_done, received_candidate_will_hide_event);
        }
    } else {
        _isf_imf_context_input_panel_send_candidate_will_hide_ack (_get_context_id (ctx));
    }
}

void isf_imf_context_input_panel_input_mode_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Mode input_mode)
{
    EcoreIMFContextISF *context_scim = (EcoreIMFContextISF *)ecore_imf_context_data_get (ctx);

    if (!IfInitContext)
        _isf_imf_context_init ();

    if (context_scim == get_focused_ic ()) {
        SECURE_LOGD ("input mode : %d\n", input_mode);
        _isf_imf_context_input_panel_input_mode_set (_get_context_id (ctx), input_mode);
    }
}

/**
 * process input panel show message
 */
static bool _process_ise_panel_showed (void)
{
    /* When the size of ISE gets changed, STATE_SHOW is be delivered again to letting applications know the change.
       Later, an event type for notifying the size change of ISE needs to be added instead. */
    input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;

    /* Notify that ISE status has changed */
    _event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);

    received_will_hide_event = EINA_FALSE;
    LOGD ("received_will_hide_event = 0\n");

    return true;
}

/**
 * process input panel hide message
 */
static bool _process_ise_panel_hided (void)
{
    if (input_panel_state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        return false;
    }

    input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;

    /* Notify that ISE status has changed */
    _event_callback_call (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE);

    received_will_hide_event = EINA_TRUE;
    LOGD ("received_will_hide_event = 1\n");
    isf_imf_context_input_panel_send_will_hide_ack (get_using_ic (ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE));

    return true;
}

/**
 * process input panel related message
 */
bool process_update_input_context (int type, int value)
{
    if (type == (uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT) {
        switch (value) {
            case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
                _process_ise_panel_hided ();
                return true;
            case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
                _clear_will_show_timer ();
                _process_ise_panel_showed ();
                return true;
            case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
                input_panel_state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;
                _clear_will_show_timer ();
                break;
            case SCIM_INPUT_PANEL_STATE_WILL_HIDE:
                break;
            default:
                break;
        }
    }

    _event_callback_call ((Ecore_IMF_Input_Panel_Event)type, (int)value);

    return true;
}

