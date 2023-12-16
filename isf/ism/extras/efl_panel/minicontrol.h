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

#ifndef __MINI_CONTROL_H__
#define __MINI_CONTROL_H__

#include <Elementary.h>

class MiniControl
{
public:
    MiniControl ();
    ~MiniControl ();
    bool create (const char *name, const char *file, int angle);
    void destroy ();
    void set_text (const char *title, const char *content);
    void set_icon (const char *filepath, const char *group);
    void resize (Evas_Coord w, Evas_Coord h);
    void show ();
    void hide ();
    void rotate (int angle);
    bool get_visibility ();
    void set_selected_callback (Edje_Signal_Cb func, void *data);
private:
    Evas_Object *win;
    Evas_Object *layout;
    Eina_Bool    visibility;
};
#endif /* __MINI_CONTROL_H__ */
