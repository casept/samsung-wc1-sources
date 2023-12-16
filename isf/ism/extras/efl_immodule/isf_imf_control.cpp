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
#define Uses_ISF_IMCONTROL_CLIENT
#define Uses_SCIM_PANEL_CLIENT

#include <stdio.h>
#include <Ecore.h>
#include <errno.h>
#include "isf_imf_control.h"
#include "scim.h"


using namespace scim;


//#define IMFCONTROLDBG(str...) printf(str)
#define IMFCONTROLDBG(str...)
#define IMFCONTROLERR(str...) printf(str)


extern PanelClient       _panel_client;


int _isf_imf_context_input_panel_show (int client_id, int context, void *data, int length, bool &input_panel_show)
{
    int temp = 0;
    _panel_client.prepare (context);
    _panel_client.show_ise (client_id, context, data, length, &temp);
    _panel_client.send ();
    input_panel_show = (bool)temp;
    return 0;
}

int _isf_imf_context_input_panel_hide (int client_id, int context)
{
    _panel_client.prepare (context);
    _panel_client.hide_ise (client_id, context);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_control_panel_show (int context)
{
    _panel_client.prepare (context);
    _panel_client.show_control_panel ();
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_control_panel_hide (int context)
{
    _panel_client.prepare (context);
    _panel_client.hide_control_panel ();
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_language_set (int context, Ecore_IMF_Input_Panel_Lang lang)
{
    _panel_client.prepare (context);
    _panel_client.set_ise_language (lang);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_language_locale_get (int context, char **locale)
{
    _panel_client.prepare (context);
    _panel_client.get_ise_language_locale (locale);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_imdata_set (int context, const void *data, int len)
{
    _panel_client.prepare (context);
    _panel_client.set_imdata ((const char *)data, len);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_imdata_get (int context, void *data, int *len)
{
    _panel_client.prepare (context);
    _panel_client.get_imdata ((char *)data, len);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_geometry_get (int context, int *x, int *y, int *w, int *h)
{
    _panel_client.prepare (context);
    _panel_client.get_ise_window_geometry (x, y, w, h);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_return_key_type_set (int context, Ecore_IMF_Input_Panel_Return_Key_Type type)
{
    _panel_client.prepare (context);
    _panel_client.set_return_key_type ((int)type);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_return_key_type_get (int context, Ecore_IMF_Input_Panel_Return_Key_Type &type)
{
    int temp = 0;
    _panel_client.prepare (context);
    _panel_client.get_return_key_type (temp);
    _panel_client.send ();
    type = (Ecore_IMF_Input_Panel_Return_Key_Type)temp;
    return 0;
}

int _isf_imf_context_input_panel_return_key_disabled_set (int context, Eina_Bool disabled)
{
    _panel_client.prepare (context);
    _panel_client.set_return_key_disable ((int)disabled);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_return_key_disabled_get (int context, Eina_Bool &disabled)
{
    int temp = 0;
    _panel_client.prepare (context);
    _panel_client.get_return_key_disable (temp);
    _panel_client.send ();
    disabled = (Eina_Bool)temp;
    return 0;
}

int _isf_imf_context_input_panel_layout_set (int context, Ecore_IMF_Input_Panel_Layout layout)
{
    _panel_client.prepare (context);
    _panel_client.set_layout (layout);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_layout_get (int context, Ecore_IMF_Input_Panel_Layout *layout)
{
    int layout_temp;
    _panel_client.prepare (context);
    _panel_client.get_layout (&layout_temp);
    _panel_client.send ();

    *layout = (Ecore_IMF_Input_Panel_Layout)layout_temp;
    return 0;
}

int _isf_imf_context_input_panel_state_get (int context, Ecore_IMF_Input_Panel_State &state)
{
    int temp = 0;
    _panel_client.prepare (context);
    _panel_client.get_ise_state (temp);
    _panel_client.send ();

    state = (Ecore_IMF_Input_Panel_State)temp;
    return 0;
}

int _isf_imf_context_input_panel_caps_mode_set (int context, unsigned int mode)
{
    _panel_client.prepare (context);
    _panel_client.set_caps_mode (mode);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_candidate_window_geometry_get (int context, int *x, int *y, int *w, int *h)
{
    _panel_client.prepare (context);
    _panel_client.get_candidate_window_geometry (x, y, w, h);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_send_will_show_ack (int context)
{
    _panel_client.prepare (context);
    _panel_client.send_will_show_ack ();
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_send_will_hide_ack (int context)
{
    _panel_client.prepare (context);
    _panel_client.send_will_hide_ack ();
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_set_keyboard_mode (int context, TOOLBAR_MODE_T mode)
{
    _panel_client.prepare (context);
    _panel_client.set_keyboard_mode (mode);
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_send_candidate_will_hide_ack (int context)
{
    _panel_client.prepare (context);
    _panel_client.send_candidate_will_hide_ack ();
    _panel_client.send ();
    return 0;
}

int _isf_imf_context_input_panel_input_mode_set (int context, Ecore_IMF_Input_Mode input_mode)
{
    _panel_client.prepare (context);
    _panel_client.set_input_mode ((int)input_mode);
    _panel_client.send ();
    return 0;
}

/*
vi:ts=4:expandtab:nowrap
*/
