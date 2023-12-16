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

#ifndef __ISE_CONTEXT_H
#define __ISE_CONTEXT_H

#include <Ecore_IMF.h>
#include <Ecore_X.h>

typedef struct {
    Ecore_IMF_Input_Panel_Lang language;
    Ecore_IMF_Input_Panel_Layout layout;
    Ecore_IMF_Input_Panel_Return_Key_Type return_key_type;
    Ecore_X_Window client_window;
    int imdata_size;
    int cursor_pos;
    Eina_Bool return_key_disabled;
    Eina_Bool prediction_allow;
    Eina_Bool password_mode;
    Eina_Bool caps_mode;
    int layout_variation;
    Ecore_IMF_Autocapital_Type autocapital_type;
    Ecore_IMF_Input_Hints input_hint;
    Ecore_IMF_BiDi_Direction bidi_direction;
    int reserved[245];
} Ise_Context;

#endif  /* __ISE_CONTEXT_H */

/*
vi:ts=4:expandtab:nowrap
*/
