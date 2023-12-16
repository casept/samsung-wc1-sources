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

#ifndef __ISF_IMF_CONTROL_H
#define __ISF_IMF_CONTROL_H

#define Uses_SCIM_PANEL_CLIENT

#include <Ecore_IMF.h>
#include "scim.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    int _isf_imf_context_input_panel_show (int client_id, int context, void *data, int length, bool &input_panel_show);
    int _isf_imf_context_input_panel_hide (int client_id, int context);
    int _isf_imf_context_control_panel_show (int context);
    int _isf_imf_context_control_panel_hide (int context);

    int _isf_imf_context_input_panel_language_set (int context, Ecore_IMF_Input_Panel_Lang lang);
    int _isf_imf_context_input_panel_language_locale_get (int context, char **locale);

    int _isf_imf_context_input_panel_imdata_set (int context, const void *data, int len);
    int _isf_imf_context_input_panel_imdata_get (int context, void *data, int *len);
    int _isf_imf_context_input_panel_geometry_get (int context, int *x, int *y, int *w, int *h);
    int _isf_imf_context_input_panel_layout_set (int context, Ecore_IMF_Input_Panel_Layout layout);
    int _isf_imf_context_input_panel_layout_get (int context, Ecore_IMF_Input_Panel_Layout *layout);
    int _isf_imf_context_input_panel_state_get (int context, Ecore_IMF_Input_Panel_State &state);
    int _isf_imf_context_input_panel_return_key_type_set (int context, Ecore_IMF_Input_Panel_Return_Key_Type type);
    int _isf_imf_context_input_panel_return_key_type_get (int context, Ecore_IMF_Input_Panel_Return_Key_Type &type);
    int _isf_imf_context_input_panel_return_key_disabled_set (int context, Eina_Bool disabled);
    int _isf_imf_context_input_panel_return_key_disabled_get (int context, Eina_Bool &disabled);
    int _isf_imf_context_input_panel_caps_mode_set (int context, unsigned int mode);

    int _isf_imf_context_candidate_window_geometry_get (int context, int *x, int *y, int *w, int *h);

    int _isf_imf_context_input_panel_send_will_show_ack (int context);
    int _isf_imf_context_input_panel_send_will_hide_ack (int context);

    int _isf_imf_context_set_keyboard_mode (int context, scim::TOOLBAR_MODE_T mode);

    int _isf_imf_context_input_panel_send_candidate_will_hide_ack (int context);

    int _isf_imf_context_input_panel_input_mode_set (int context, Ecore_IMF_Input_Mode input_mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ISF_IMF_CONTROL_H */

