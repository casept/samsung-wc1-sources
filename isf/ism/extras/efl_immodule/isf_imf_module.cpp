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

#include <stdio.h>
#include <dlog.h>
#include "isf_imf_context.h"
#include "isf_imf_control_ui.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    static const Ecore_IMF_Context_Info isf_imf_info = {
        "isf",                                  /* ID */
        "Input Service Framework for Ecore",    /* Description */
        "*",                                    /* Default locales */
        NULL,                                   /* Canvas type */
        0                                       /* Canvas required */
    };

    static Ecore_IMF_Context_Class isf_imf_class = {
        isf_imf_context_add,                    /* add */
        isf_imf_context_del,                    /* del */
        isf_imf_context_client_window_set,      /* client_window_set */
        isf_imf_context_client_canvas_set,      /* client_canvas_set */
        isf_imf_context_input_panel_show,       /* show */
        isf_imf_context_input_panel_hide,       /* hide */
        isf_imf_context_preedit_string_get,     /* get_preedit_string */
        isf_imf_context_focus_in,               /* focus_in */
        isf_imf_context_focus_out,              /* focus_out */
        isf_imf_context_reset,                  /* reset */
        isf_imf_context_cursor_position_set,    /* cursor_position_set */
        isf_imf_context_use_preedit_set,        /* use_preedit_set */
        isf_imf_context_input_mode_set,         /* input_mode_set */
        isf_imf_context_filter_event,           /* filter_event */
        isf_imf_context_preedit_string_with_attributes_get,
        isf_imf_context_prediction_allow_set,
        isf_imf_context_autocapital_type_set,
        isf_imf_context_control_panel_show,
        isf_imf_context_control_panel_hide,
        isf_imf_context_input_panel_layout_set,
        isf_imf_context_input_panel_layout_get,
        isf_imf_context_input_panel_language_set,
        isf_imf_context_input_panel_language_get,
        isf_imf_context_cursor_location_set,
        isf_imf_context_imdata_set,
        isf_imf_context_imdata_get,
        isf_imf_context_input_panel_return_key_type_set,
        isf_imf_context_input_panel_return_key_disabled_set,
        isf_imf_context_input_panel_caps_lock_mode_set,
        isf_imf_context_input_panel_geometry_get,
        isf_imf_context_input_panel_state_get,
        NULL, /* input_panel_event_callback_add */
        NULL, /* input_panel_event_callback_del */
        isf_imf_context_input_panel_language_locale_get,
        isf_imf_context_candidate_window_geometry_get,
        isf_imf_context_input_hint_set,
        isf_imf_context_bidi_direction_set
    };

    static Ecore_IMF_Context *imf_module_create (void);
    static Ecore_IMF_Context *imf_module_exit (void);

    static Eina_Bool imf_module_init (void) {
        ecore_imf_module_register (&isf_imf_info, imf_module_create, imf_module_exit);
        register_key_handler ();
        return EINA_TRUE;
    }

    static void imf_module_shutdown (void) {
        unregister_key_handler ();
        isf_imf_context_shutdown ();
    }

    static Ecore_IMF_Context *imf_module_create (void) {
        Ecore_IMF_Context  *ctx = NULL;
        EcoreIMFContextISF *ctxd = NULL;

        ctxd = isf_imf_context_new ();
        if (!ctxd) {
            LOGW ("isf_imf_context_new () failed!!!\n");
            return NULL;
        }

        ctx = ecore_imf_context_new (&isf_imf_class);
        if (!ctx) {
            delete ctxd;
            return NULL;
        }

        ecore_imf_context_data_set (ctx, ctxd);

        return ctx;
    }

    static Ecore_IMF_Context *imf_module_exit (void) {
        return NULL;
    }

    EINA_MODULE_INIT(imf_module_init);
    EINA_MODULE_SHUTDOWN(imf_module_shutdown);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*
vi:ts=4:expandtab:nowrap
*/
