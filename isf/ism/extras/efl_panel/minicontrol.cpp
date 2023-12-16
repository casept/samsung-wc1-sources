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
#include "minicontrol.h"
#include "isf_panel_efl.h"
#include <Elementary.h>
#include <minicontrol-provider.h>

#define MINI_CONTROLLER_WIDTH (698)
#define MINI_CONTROLLER_WIDTH_LANDSCAPE (1258)
#define MINI_CONTROLLER_HEIGHT (180)

MiniControl::MiniControl ()
    : win (0),
      layout (0),
      visibility (false)
{

}

MiniControl::~MiniControl ()
{
    destroy ();
}

bool
MiniControl::create (const char *name, const char *file, int angle)
{
    if (win)
        return false;

    char mcname[1024] = {0};
    snprintf (mcname, sizeof (mcname), "%s_minictrl_ongoing", name);

    win = minicontrol_win_add (mcname);
    if (!win) {
        return false;
    }

    elm_win_alpha_set (win, EINA_TRUE);

    double scale = elm_config_scale_get ();
    if (angle == 90 || angle == 270)
        evas_object_resize (win, MINI_CONTROLLER_WIDTH_LANDSCAPE * scale, MINI_CONTROLLER_HEIGHT * scale);
    else
        evas_object_resize (win, MINI_CONTROLLER_WIDTH * scale, MINI_CONTROLLER_HEIGHT * scale);

    /* load layout */
    layout = elm_layout_add (win);
    if (!elm_layout_file_set (layout, file , "mini_controller"))
        LOGW ("Failed to elm_layout_file_set : %s\n", file);

    evas_object_size_hint_weight_set (layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_win_resize_object_add (win, layout);
    evas_object_show (layout);

    return true;
}

void
MiniControl::destroy ()
{
    if (layout) {
        evas_object_del (layout);
        layout = NULL;
    }

    if (win) {
        evas_object_del (win);
        win = NULL;
    }

    visibility = false;
}

void
MiniControl::set_text (const char *title, const char *content)
{
    if (!layout) return;

    Evas_Object *layout_edj = elm_layout_edje_get (layout);
    if (layout_edj) {
        Eina_Strbuf *title_text = eina_strbuf_new ();
        Eina_Strbuf *content_text = eina_strbuf_new ();

        Evas_Object *title_label = elm_label_add (layout_edj);
        elm_label_line_wrap_set (title_label, ELM_WRAP_WORD);

        Evas_Object *content_label = elm_label_add (layout_edj);
        elm_label_line_wrap_set (content_label, ELM_WRAP_WORD);

        eina_strbuf_append_printf (title_text, "<font_size=45 color=#070707>%s</>", title);
        eina_strbuf_append_printf (content_text, "<font_size=42 color=#4c4c4c>%s</>", content);

        elm_object_text_set (title_label, eina_strbuf_string_get (title_text));
        elm_object_part_content_set (layout, "title_text", title_label);
        elm_object_text_set (content_label, eina_strbuf_string_get (content_text));
        elm_object_part_content_set (layout, "info_text", content_label);

        eina_strbuf_free (title_text);
        eina_strbuf_free (content_text);
    }
}

void
MiniControl::set_icon (const char *filepath, const char *group)
{
    Evas_Object *img = elm_image_add (layout);
    if (!img || !layout || !filepath) return;
    elm_image_file_set (img, filepath, group);
    evas_object_size_hint_aspect_set (img, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
    elm_object_part_content_set (layout, "keyboard_icon", img);
}

void
MiniControl::resize (Evas_Coord w, Evas_Coord h)
{
    if (!win) return;

    evas_object_resize (win, w, h);
}

void
MiniControl::show ()
{
    if (!win) return;

    evas_object_show (win);
    visibility = true;
}

void
MiniControl::hide ()
{
    if (!win) return;

    evas_object_hide (win);
    visibility = false;
}

void
MiniControl::rotate (int angle)
{
    if (!win) return;

    double scale = elm_config_scale_get ();
    if (angle == 90 || angle == 270)
        evas_object_resize (win, MINI_CONTROLLER_WIDTH_LANDSCAPE * scale, MINI_CONTROLLER_HEIGHT * scale);
    else
        evas_object_resize (win, MINI_CONTROLLER_WIDTH * scale, MINI_CONTROLLER_HEIGHT * scale);
}

bool
MiniControl::get_visibility ()
{
    return visibility;
}

void
MiniControl::set_selected_callback (Edje_Signal_Cb func, void *data)
{
    if (!layout) return;

    elm_object_signal_callback_add (layout, "selected", "edje", func, data);
}
