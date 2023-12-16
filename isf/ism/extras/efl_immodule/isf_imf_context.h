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

#ifndef __ISF_IMF_CONTEXT_H
#define __ISF_IMF_CONTEXT_H

#include <Ecore.h>
#include <Ecore_IMF.h>
#include <dlog.h>

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG "IMMODULE"

const double DOUBLE_SPACE_INTERVAL = 1.0;
const double HIDE_TIMER_INTERVAL = 0.05;
const double WILL_SHOW_TIMER_INTERVAL = 5.0;

typedef struct _EcoreIMFContextISF      EcoreIMFContextISF;
typedef struct _EcoreIMFContextISFImpl  EcoreIMFContextISFImpl;

struct _EcoreIMFContextISF {
    Ecore_IMF_Context *ctx;

    EcoreIMFContextISFImpl *impl;

    int id; /* Input Context id*/
    struct _EcoreIMFContextISF *next;

    /* Constructor */
    _EcoreIMFContextISF () : ctx(NULL), impl(NULL), id(0), next(NULL) {}
};

void register_key_handler ();
void unregister_key_handler ();

int get_panel_client_id ();
Eina_Bool caps_mode_check (Ecore_IMF_Context *ctx, Eina_Bool force, Eina_Bool noti);

EcoreIMFContextISF *get_focused_ic ();

void context_scim_imdata_get (Ecore_IMF_Context *ctx, void* data, int* length);
void imengine_layout_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Panel_Layout layout);

void isf_imf_context_add (Ecore_IMF_Context *ctx);
void isf_imf_context_del (Ecore_IMF_Context *ctx);
void isf_imf_context_client_window_set (Ecore_IMF_Context *ctx, void *window);
void isf_imf_context_client_canvas_set (Ecore_IMF_Context *ctx, void *window);
void isf_imf_context_focus_in (Ecore_IMF_Context *ctx);
void isf_imf_context_focus_out (Ecore_IMF_Context *ctx);
void isf_imf_context_reset (Ecore_IMF_Context *ctx);
void isf_imf_context_cursor_position_set (Ecore_IMF_Context *ctx, int cursor_pos);
void isf_imf_context_cursor_location_set (Ecore_IMF_Context *ctx, int x, int y, int w, int h);
void isf_imf_context_input_mode_set (Ecore_IMF_Context *ctx, Ecore_IMF_Input_Mode input_mode);
void isf_imf_context_preedit_string_get (Ecore_IMF_Context *ctx, char** str, int *cursor_pos);
void isf_imf_context_preedit_string_with_attributes_get (Ecore_IMF_Context *ctx, char** str, Eina_List **attrs, int *cursor_pos);
void isf_imf_context_use_preedit_set (Ecore_IMF_Context* ctx, Eina_Bool use_preedit);
Eina_Bool  isf_imf_context_filter_event (Ecore_IMF_Context *ctx, Ecore_IMF_Event_Type type, Ecore_IMF_Event *event);
void isf_imf_context_prediction_allow_set (Ecore_IMF_Context* ctx, Eina_Bool prediction);
void isf_imf_context_autocapital_type_set (Ecore_IMF_Context* ctx, Ecore_IMF_Autocapital_Type autocapital_type);
void isf_imf_context_imdata_set (Ecore_IMF_Context* ctx, const void *data, int len);
void isf_imf_context_imdata_get (Ecore_IMF_Context* ctx, void *data, int *len);
void isf_imf_context_input_hint_set (Ecore_IMF_Context* ctx, Ecore_IMF_Input_Hints hint);
void isf_imf_context_bidi_direction_set (Ecore_IMF_Context* ctx, Ecore_IMF_BiDi_Direction direction);

EcoreIMFContextISF* isf_imf_context_new      (void);
void                isf_imf_context_shutdown (void);

#endif  /* __ISF_IMF_CONTEXT_H */

/*
vi:ts=4:expandtab:nowrap
*/
