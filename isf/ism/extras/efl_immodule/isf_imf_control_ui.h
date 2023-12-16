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

#ifndef __ISF_IMF_CONTROL_UI_H
#define __ISF_IMF_CONTROL_UI_H

#define Uses_SCIM_PANEL_CLIENT

#include <Ecore_IMF.h>
#include <Ecore_X.h>
#include "scim.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    Eina_Bool check_focus_out_by_popup_win (Ecore_IMF_Context *ctx);
    void input_panel_event_callback_call (Ecore_IMF_Input_Panel_Event type, int value);
    scim::TOOLBAR_MODE_T get_keyboard_mode ();
    bool process_update_input_context (int type, int value);
    Ecore_IMF_Context *get_using_ic (Ecore_IMF_Input_Panel_Event type, int value);
    void save_current_xid (Ecore_IMF_Context *ctx);
    void clear_hide_request ();
    Ecore_X_Window client_window_id_get (Ecore_IMF_Context *ctx);

    void isf_imf_input_panel_init ();
    void isf_imf_input_panel_shutdown ();
    void isf_imf_context_input_panel_input_mode_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Mode input_mode);
    void isf_imf_context_input_panel_show (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_hide (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_instant_hide (Ecore_IMF_Context *ctx);
    void isf_imf_context_control_panel_show (Ecore_IMF_Context *ctx);
    void isf_imf_context_control_panel_hide (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_language_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Lang lang);
    Ecore_IMF_Input_Panel_Lang isf_imf_context_input_panel_language_get (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_language_locale_get (Ecore_IMF_Context *ctx, char **locale);
    void isf_imf_context_input_panel_imdata_set (Ecore_IMF_Context *ctx, const void* data, int len);
    void isf_imf_context_input_panel_imdata_get (Ecore_IMF_Context *ctx, void* data, int* len);
    void isf_imf_context_input_panel_geometry_get (Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h);
    void isf_imf_context_input_panel_layout_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout);
    Ecore_IMF_Input_Panel_Layout isf_imf_context_input_panel_layout_get (Ecore_IMF_Context *ctx);
    Ecore_IMF_Input_Panel_State isf_imf_context_input_panel_state_get (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_caps_mode_set (Ecore_IMF_Context *ctx, unsigned int mode);
    void isf_imf_context_input_panel_event_callback_clear (Ecore_IMF_Context *ctx);

    void isf_imf_context_input_panel_return_key_type_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Return_Key_Type type);
    Ecore_IMF_Input_Panel_Return_Key_Type isf_imf_context_input_panel_return_key_type_get (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_return_key_disabled_set (Ecore_IMF_Context *ctx, Eina_Bool disabled);
    Eina_Bool isf_imf_context_input_panel_return_key_disabled_get (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_caps_lock_mode_set (Ecore_IMF_Context *ctx, Eina_Bool mode);
    void isf_imf_context_candidate_window_geometry_get (Ecore_IMF_Context *ctx, int *x, int *y, int *w, int *h);

    void isf_imf_context_input_panel_send_will_show_ack (Ecore_IMF_Context *ctx);
    void isf_imf_context_input_panel_send_will_hide_ack (Ecore_IMF_Context *ctx);
    void isf_imf_context_set_keyboard_mode (Ecore_IMF_Context *ctx, scim::TOOLBAR_MODE_T mode);

    void isf_imf_context_input_panel_send_candidate_will_hide_ack (Ecore_IMF_Context *ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ISF_IMF_CONTROL_UI_H */

