/*
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_util_h
#define ewk_util_h

#if USE(EO)
typedef struct _Eo_Opaque Eo;
typedef Eo Evas;
typedef Eo Evas_Object;
#else
typedef struct _Evas Evas;
typedef struct _Evas_Object Evas_Object;
#endif

typedef struct _cairo_surface cairo_surface_t;

Evas_Object* ewk_util_image_from_cairo_surface_add(Evas* canvas, cairo_surface_t* surface);

#endif // ewk_util_h
