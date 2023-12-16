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

#ifndef __ISF_DEMO_EFL_H
#define __ISF_DEMO_EFL_H


#include <Elementary.h>
#include <appcore-efl.h>
#include <dlog.h>

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG "ISF_DEMO_EFL"

struct appdata {
    int root_w;
    int root_h;
    int root_x;
    int root_y;

    Evas *evas;
    Evas_Object *win_main;
    Evas_Object *conformant;
    Evas_Object *naviframe;
    Evas_Object *menu_popup;

    int is_frameview;

    // Add more variables here
    Evas_Object *li;
    Evas_Object *ev_li;
    Eina_Bool vkbd_state;
};

struct _menu_item {
    const char *name;
    void (*func)(void *data, Evas_Object *obj, void *event_info);
};

// Utility functions
Evas_Object *create_ef (Evas_Object *parent, const char *label, const char *guide_text, Evas_Object **entry = NULL);
void         add_layout_to_naviframe (void *data, Evas_Object *lay_in, const char *title);

#endif /* __ISF_DEMO_EFL_H */

/*
vi:ts=4:ai:nowrap:expandtab
*/
