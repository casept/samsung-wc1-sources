/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
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

#ifndef __ISF_PANEL_UTILITY_H
#define __ISF_PANEL_UTILITY_H

using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define COMPOSE_KEY_MODULE                         "COMPOSE_KEY_MODULE"


#if SCIM_USE_STL_EXT_HASH_MAP
typedef __gnu_cxx::hash_map <String, std::vector <size_t>, scim_hash_string>        MapStringVectorSizeT;
typedef std::map <String, std::vector <String> >                                    MapStringVectorString;
typedef std::map <String ,String>                                                   MapStringString;
#elif SCIM_USE_STL_HASH_MAP
typedef std::hash_map <String, std::vector <size_t>, scim_hash_string>              MapStringVectorSizeT;
typedef std::map <String, std::vector <String> >                                    MapStringVectorString;
typedef std::map <String ,String>                                                   MapStringString;
#else
typedef std::map <String, std::vector <size_t> >                                    MapStringVectorSizeT;
typedef std::map <String, std::vector <String> >                                    MapStringVectorString;
typedef std::map <String ,String>                                                   MapStringString;
#endif

typedef enum {
    ALL_ISE = 0,
    HELPER_ONLY,
    TYPE_END
} LOAD_ISE_TYPE;

void isf_get_all_languages (std::vector<String> &all_langs);
void isf_get_all_ises_in_languages (std::vector<String> lang_list, std::vector<String> &uuid_list, std::vector<String> &name_list);

void isf_get_keyboard_ise (const ConfigPointer &config, String &ise_uuid, String &ise_name, uint32 &ise_option);
void isf_get_keyboard_ises_in_languages (const std::vector<String> &lang_list, std::vector<String> &uuid_list, std::vector<String> &name_list, bool bCheckOption = true);
void isf_get_helper_ises_in_languages (const std::vector<String> &lang_list, std::vector<String> &uuid_list, std::vector<String> &name_list);

void isf_save_ise_information (void);
void isf_load_ise_information (LOAD_ISE_TYPE type, const ConfigPointer &config);
bool isf_update_ise_list (LOAD_ISE_TYPE type, const ConfigPointer &config);
bool isf_update_ise_module (const String strModulePath, const ConfigPointer &config);
String isf_get_normalized_language (String src_str);
bool isf_add_helper_ise (HelperInfo helper_info, String module_name);
bool isf_remove_helper_ise (const char *uuid, const ConfigPointer &config);

#endif /* __ISF_PANEL_UTILITY_H */

/*
vi:ts=4:ai:nowrap:expandtab
*/
