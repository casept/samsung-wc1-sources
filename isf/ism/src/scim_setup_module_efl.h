/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
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
 * $Id: scim_setup_module_efl.h,v 1.9 2005/01/10 08:30:45 suzhe Exp $
 */

#if !defined (__SCIM_SETUP_MODULE_EFL_H)
#define __SCIM_SETUP_MODULE_EFL_H


#include <Elementary.h>
#include <string.h>

using namespace scim;
typedef Evas_Object * (*SetupModuleCreateUIFunc) (Evas_Object *parent, Evas_Object *layout);
typedef String (*SetupModuleGetCategoryFunc) (void);
typedef String (*SetupModuleGetNameFunc) (void);
typedef String (*SetupModuleGetDescriptionFunc) (void);
typedef void (*SetupModuleLoadConfigFunc) (const ConfigPointer &config);
typedef void (*SetupModuleSaveConfigFunc) (const ConfigPointer &config);
typedef bool (*SetupModuleQueryChangedFunc) (void);
typedef bool (*SetupModuleKeyProceedingFunc) (int);
typedef bool (*SetupModuleOptionResetFunc) (const ConfigPointer &config);

class EAPI SetupModule
{
    Module      m_module;

    SetupModuleCreateUIFunc       m_create_ui;
    SetupModuleGetCategoryFunc    m_get_category;
    SetupModuleGetNameFunc        m_get_name;
    SetupModuleGetDescriptionFunc m_get_description;
    SetupModuleLoadConfigFunc     m_load_config;
    SetupModuleSaveConfigFunc     m_save_config;
    SetupModuleQueryChangedFunc   m_query_changed;
    SetupModuleKeyProceedingFunc  m_key_proceeding;
    SetupModuleOptionResetFunc    m_option_reset;

    SetupModule (const SetupModule &);
    SetupModule & operator= (const SetupModule &);

public:
    SetupModule ();
    SetupModule (const String &name);

    bool load  (const String &name);
    bool unload();
    bool valid () const;

    Evas_Object * create_ui (Evas_Object *,Evas_Object *) const;

    String get_category () const;
    String get_name () const;
    String get_description () const;

    void load_config (const ConfigPointer &config) const;
    void save_config (const ConfigPointer &config) const;

    bool query_changed () const;
    bool key_proceeding (int)const;
    bool option_reset (const ConfigPointer &config) const;
};

EAPI int scim_get_setup_module_list (std::vector <String>& mod_list);

#endif // __SCIM_SETUP_MODULE_EFL_H

/*
vi:ts=4:ai:nowrap:expandtab
*/
