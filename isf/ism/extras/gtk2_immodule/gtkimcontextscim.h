/** @file gtkimcontextscim.h
 *  @brief immodule for GTK2.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * Modifications by Samsung Electronics Co., Ltd.
 * 1. Implement GTK context auto-restore when PanelAgent is crashed
 * 2. Add new interface APIs for helper ISE
 *    a. panel_slot_reset_keyboard_ise ()
 *    b. panel_slot_show_preedit_string (), panel_slot_hide_preedit_string () and panel_slot_update_preedit_string ()
 *
 * $Id: gtkimcontextscim.h,v 1.12 2005/06/27 15:31:49 suzhe Exp $
 */

#ifndef __GTK_IM_CONTEXT_SCIM_H__
#define __GTK_IM_CONTEXT_SCIM_H__

#include <gtk/gtkimcontext.h>

typedef struct _GtkIMContextSCIM       GtkIMContextSCIM;
typedef struct _GtkIMContextSCIMClass  GtkIMContextSCIMClass;
typedef struct _GtkIMContextSCIMImpl   GtkIMContextSCIMImpl;

struct _GtkIMContextSCIM
{
  GtkIMContext object;
  GtkIMContext *slave;

  GtkIMContextSCIMImpl *impl;

  int id; /* Input Context id*/
  struct _GtkIMContextSCIM *next;
};

struct _GtkIMContextSCIMClass
{
  GtkIMContextClass parent_class;
};

GtkIMContext *gtk_im_context_scim_new (void);

void gtk_im_context_scim_register_type (GTypeModule *type_module);
void gtk_im_context_scim_shutdown (void);

#endif /* __GTK_IM_CONTEXT_SCIM_H__ */
