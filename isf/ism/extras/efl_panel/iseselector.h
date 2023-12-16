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

#ifndef __ISE_SELECTOR_H__
#define __ISE_SELECTOR_H__

#include <Elementary.h>

typedef void (*Ise_Selector_Selected_Cb)(unsigned int index);
typedef void (*Ise_Selector_Deleted_Cb)();

EAPI void ise_selector_create (unsigned ise_idx, Ecore_X_Window win, Ise_Selector_Selected_Cb sel_cb, Ise_Selector_Deleted_Cb del_cb);
EAPI void ise_selector_destroy ();

#endif /* __ISE_SELECTOR_H__ */
